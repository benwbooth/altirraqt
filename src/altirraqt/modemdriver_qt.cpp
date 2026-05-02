//	Altirra - Qt port
//	IATModemDriver implementation backed by QTcpSocket / QTcpServer.
//
//	Replaces the upstream Win32 modemtcp.cpp (winsock + worker thread + WSA
//	events). The Qt port runs the simulator on the main GUI thread, the same
//	thread that pumps Qt's event loop, so QTcpSocket can be used purely
//	asynchronously without an extra worker thread:
//
//	  - Outbound connect: connectToHost() returns immediately; the `connected`
//	    or `errorOccurred` signal fires on the GUI thread.
//	  - Inbound listen: QTcpServer::listen() and `newConnection` signal.
//	  - Reads: `readyRead` drains QTcpSocket into our internal byte deque.
//	  - Writes: write() goes straight to QTcpSocket; on `bytesWritten` we
//	    notify the modem-emulator callback that more space is available.
//
//	Threading: all access happens on the GUI thread. The class also lives
//	on the GUI thread. The destructor disconnects every Qt signal so a
//	dangling socket whose deletion is deferred via `deleteLater` cannot
//	deliver another notification into a destroyed driver.
//
//	Telnet protocol negotiation is intentionally NOT implemented here.
//	The upstream Win32 driver bundles telnet inside the driver because of
//	how it uses overlapped I/O on a worker thread. With Qt's bytes-only
//	socket model the cleanest approach is to keep the driver as a pure
//	transport — `mbTelnetEmulation` from the SetConfig payload is honored
//	to the degree that we don't strip IAC bytes ourselves; raw bytes go
//	through. This matches what most modern telnet/dial clients connecting
//	to a host (e.g. raw TCP BBS) actually want — the SX212/850 emulators
//	don't depend on the driver doing telnet negotiation, only on the byte
//	stream being intact.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>

#include <QHostAddress>
#include <QHostInfo>
#include <QObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <vd2/system/VDString.h>

#include "modemtcp.h"
#include "rs232.h"

namespace {

class ATModemDriverQt final : public QObject, public IATModemDriver {
public:
	ATModemDriverQt();
	~ATModemDriverQt() override;

	bool Init(const char *address, const char *service, uint32 port, bool loggingEnabled, IATModemDriverCallback *callback) override;
	void Shutdown() override;

	bool GetLastIncomingAddress(VDStringA& address, uint32& port) override;

	uint32 Write(const void *data, uint32 len) override;
	uint32 Read(void *buf, uint32 len) override;
	bool ReadLogMessages(VDStringA& messages) override;

	void SetLoggingEnabled(bool enabled) override;
	void SetConfig(const ATRS232Config& config) override;

private:
	void OnSocketConnected();
	void OnSocketDisconnected();
	void OnSocketReadyRead();
	void OnSocketError(QAbstractSocket::SocketError err);
	void OnSocketBytesWritten(qint64 n);
	void OnNewConnection();
	void OnHostLookupFinished(const QHostInfo& info);

	void TearDownSocket();
	void TearDownServer();
	void ReportEvent(ATModemPhase phase, ATModemEvent event);
	void LogF(const char *fmt, ...);

	IATModemDriverCallback *mpCallback = nullptr;

	QTcpSocket *mpSocket = nullptr;
	QTcpServer *mpServer = nullptr;

	// Pending host lookup id, so we can cancel on Shutdown.
	int mLookupId = -1;
	uint16 mDialPort = 0;
	QString mDialAddress;
	bool mbConnected = false;
	bool mbReportedConnect = false;

	VDStringA mIncomingAddress;
	uint32 mIncomingPort = 0;

	bool mbLoggingEnabled = false;
	bool mbListenIPv6 = true;
	VDStringA mLogMessages;

	std::deque<uint8> mReadBuffer;
};

ATModemDriverQt::ATModemDriverQt() = default;

ATModemDriverQt::~ATModemDriverQt() {
	// Defensive: callers should call Shutdown() first, but the destructor
	// must still leave nothing dangling. TearDown* both disconnect every
	// Qt signal and arrange QObject deletion so any in-flight queued slot
	// can no longer reach our destroyed `this`.
	TearDownSocket();
	TearDownServer();
	mpCallback = nullptr;
}

bool ATModemDriverQt::Init(const char *address, const char *service, uint32 port, bool loggingEnabled, IATModemDriverCallback *callback) {
	// Re-init drops any prior session.
	TearDownSocket();
	TearDownServer();
	mReadBuffer.clear();
	mIncomingAddress.clear();
	mIncomingPort = 0;
	mbConnected = false;
	mbReportedConnect = false;
	mLookupId = -1;

	mpCallback = callback;
	mbLoggingEnabled = loggingEnabled;

	const bool isListening = !address || !*address;

	if (isListening) {
		// Inbound: listen on the configured port. QTcpServer::listen with
		// QHostAddress::Any binds dual-stack on systems where supported,
		// matching upstream's "IPv4 + optional IPv6" behavior.
		mpServer = new QTcpServer(this);
		QObject::connect(mpServer, &QTcpServer::newConnection,
			this, &ATModemDriverQt::OnNewConnection);

		const QHostAddress bindAddr = mbListenIPv6 ? QHostAddress(QHostAddress::Any)
		                                           : QHostAddress(QHostAddress::AnyIPv4);

		if (!mpServer->listen(bindAddr, (quint16)port)) {
			LogF("ModemTCP: listen on port %u failed: %s\n",
				(unsigned)port, mpServer->errorString().toUtf8().constData());

			QAbstractSocket::SocketError err = mpServer->serverError();
			TearDownServer();

			ATModemEvent ev = kATModemEvent_GenericError;
			if (err == QAbstractSocket::AddressInUseError)
				ev = kATModemEvent_LineInUse;
			else if (err == QAbstractSocket::NetworkError)
				ev = kATModemEvent_NoDialTone;

			ReportEvent(kATModemPhase_Listen, ev);
			return false;
		}

		LogF("ModemTCP: listening on port %u\n", (unsigned)port);
		return true;
	}

	// Outbound: resolve host then connect. We accept a `service` (port name
	// or numeric string) per upstream; if it's numeric, parse it directly,
	// otherwise fall back to `port`. atoi works for both.
	uint16 dialPort = (uint16)port;
	if (service && *service) {
		int v = std::atoi(service);
		if (v > 0 && v < 65536)
			dialPort = (uint16)v;
	}
	if (!dialPort) {
		ReportEvent(kATModemPhase_Connecting, kATModemEvent_ConnectFailed);
		return false;
	}

	mDialAddress = QString::fromUtf8(address);
	mDialPort = dialPort;

	mpSocket = new QTcpSocket(this);
	QObject::connect(mpSocket, &QTcpSocket::connected,
		this, &ATModemDriverQt::OnSocketConnected);
	QObject::connect(mpSocket, &QTcpSocket::disconnected,
		this, &ATModemDriverQt::OnSocketDisconnected);
	QObject::connect(mpSocket, &QTcpSocket::readyRead,
		this, &ATModemDriverQt::OnSocketReadyRead);
	QObject::connect(mpSocket, &QTcpSocket::errorOccurred,
		this, &ATModemDriverQt::OnSocketError);
	QObject::connect(mpSocket, &QTcpSocket::bytesWritten,
		this, &ATModemDriverQt::OnSocketBytesWritten);

	mpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

	LogF("ModemTCP: dialing %s:%u\n",
		address, (unsigned)dialPort);

	// QTcpSocket::connectToHost is itself async (it fires connected /
	// errorOccurred). It also performs the DNS lookup internally, so we
	// don't need a separate QHostInfo step. The signal phase reported
	// will be Connecting either way; that's consistent with upstream.
	mpSocket->connectToHost(mDialAddress, mDialPort);
	return true;
}

void ATModemDriverQt::Shutdown() {
	mpCallback = nullptr;	// stop callbacks immediately
	TearDownSocket();
	TearDownServer();
	mReadBuffer.clear();
	mbConnected = false;
}

bool ATModemDriverQt::GetLastIncomingAddress(VDStringA& address, uint32& port) {
	address = mIncomingAddress;
	port = mIncomingPort;
	return !mIncomingAddress.empty();
}

uint32 ATModemDriverQt::Write(const void *data, uint32 len) {
	if (!len || !mpSocket || !mbConnected)
		return 0;

	qint64 written = mpSocket->write(static_cast<const char *>(data), (qint64)len);
	if (written <= 0)
		return 0;

	return (uint32)written;
}

uint32 ATModemDriverQt::Read(void *buf, uint32 len) {
	if (!len || mReadBuffer.empty())
		return 0;

	const uint32 avail = (uint32)mReadBuffer.size();
	const uint32 take = avail < len ? avail : len;

	uint8 *out = static_cast<uint8 *>(buf);
	for (uint32 i = 0; i < take; ++i) {
		out[i] = mReadBuffer.front();
		mReadBuffer.pop_front();
	}
	return take;
}

bool ATModemDriverQt::ReadLogMessages(VDStringA& messages) {
	if (mLogMessages.empty())
		return false;
	messages = mLogMessages;
	mLogMessages.clear();
	return true;
}

void ATModemDriverQt::SetLoggingEnabled(bool enabled) {
	mbLoggingEnabled = enabled;
}

void ATModemDriverQt::SetConfig(const ATRS232Config& config) {
	// We honor the IPv6-listen preference (used at server bind) and stash
	// any other future-relevant flags. Telnet emulation bytes are passed
	// through transparently — see file header.
	mbListenIPv6 = config.mbListenForIPv6;
}

// ---- internal --------------------------------------------------------------

void ATModemDriverQt::OnSocketConnected() {
	mbConnected = true;
	if (!mbReportedConnect) {
		mbReportedConnect = true;
		LogF("ModemTCP: connected to %s:%u\n",
			mDialAddress.toUtf8().constData(), (unsigned)mDialPort);
		ReportEvent(kATModemPhase_Connected, kATModemEvent_Connected);
	}
}

void ATModemDriverQt::OnSocketDisconnected() {
	if (!mbConnected && !mbReportedConnect)
		return;
	mbConnected = false;
	LogF("ModemTCP: connection closed\n");
	ReportEvent(kATModemPhase_Connected, kATModemEvent_ConnectionDropped);
}

void ATModemDriverQt::OnSocketReadyRead() {
	if (!mpSocket)
		return;

	// Drain whatever is available into the byte queue. The modem emulator
	// pulls from us via Read() in chunks; a deque keeps memory bounded by
	// however much the simulator hasn't consumed yet without forcing a
	// fixed buffer size.
	QByteArray data = mpSocket->readAll();
	if (data.isEmpty())
		return;

	const uint32 oldSize = (uint32)mReadBuffer.size();
	mReadBuffer.insert(mReadBuffer.end(),
		reinterpret_cast<const uint8 *>(data.constData()),
		reinterpret_cast<const uint8 *>(data.constData()) + data.size());

	if (mpCallback)
		mpCallback->OnReadAvail(this, (uint32)data.size() + oldSize);
}

void ATModemDriverQt::OnSocketError(QAbstractSocket::SocketError err) {
	if (!mpSocket)
		return;

	LogF("ModemTCP: socket error %d (%s)\n", (int)err,
		mpSocket->errorString().toUtf8().constData());

	// Map to upstream's event taxonomy. Pre-connect failures are reported
	// as ConnectFailed; post-connect drops as ConnectionDropped.
	if (!mbReportedConnect) {
		ATModemEvent ev = kATModemEvent_ConnectFailed;
		if (err == QAbstractSocket::HostNotFoundError)
			ev = kATModemEvent_NameLookupFailed;
		ReportEvent(kATModemPhase_Connecting, ev);
		return;
	}

	ATModemEvent ev = kATModemEvent_GenericError;
	if (err == QAbstractSocket::RemoteHostClosedError) {
		mbConnected = false;
		ev = kATModemEvent_ConnectionDropped;
	} else if (err == QAbstractSocket::NetworkError) {
		mbConnected = false;
		ev = kATModemEvent_ConnectionDropped;
	}

	ReportEvent(kATModemPhase_Connected, ev);
}

void ATModemDriverQt::OnSocketBytesWritten(qint64 /*n*/) {
	// Qt's write() buffers internally — we never refuse a Write() above —
	// but the modem emulator wants OnWriteAvail() pings so it knows space
	// is available to drain its outgoing queue.
	if (mpCallback)
		mpCallback->OnWriteAvail(this);
}

void ATModemDriverQt::OnNewConnection() {
	if (!mpServer)
		return;

	QTcpSocket *incoming = mpServer->nextPendingConnection();
	if (!incoming) return;

	if (mpSocket) {
		// Already have a connection — refuse additional incoming peers.
		incoming->disconnectFromHost();
		incoming->deleteLater();
		return;
	}

	// Stop accepting further connections; we're a one-line modem.
	mpServer->close();

	mpSocket = incoming;
	mpSocket->setParent(this);
	mpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

	QObject::connect(mpSocket, &QTcpSocket::disconnected,
		this, &ATModemDriverQt::OnSocketDisconnected);
	QObject::connect(mpSocket, &QTcpSocket::readyRead,
		this, &ATModemDriverQt::OnSocketReadyRead);
	QObject::connect(mpSocket, &QTcpSocket::errorOccurred,
		this, &ATModemDriverQt::OnSocketError);
	QObject::connect(mpSocket, &QTcpSocket::bytesWritten,
		this, &ATModemDriverQt::OnSocketBytesWritten);

	const QHostAddress peer = mpSocket->peerAddress();
	mIncomingAddress = peer.toString().toUtf8().constData();
	mIncomingPort = (uint32)mpSocket->peerPort();

	LogF("ModemTCP: accepted incoming connection from %s:%u\n",
		mIncomingAddress.c_str(), (unsigned)mIncomingPort);

	mbConnected = true;
	mbReportedConnect = true;
	ReportEvent(kATModemPhase_Connected, kATModemEvent_Connected);

	// If any data already arrived in the socket's read buffer before we
	// got our slot wired (Qt may have buffered it), pull it through now.
	if (mpSocket->bytesAvailable() > 0)
		OnSocketReadyRead();
}

void ATModemDriverQt::OnHostLookupFinished(const QHostInfo&) {
	// Reserved for explicit QHostInfo flow; not currently used because
	// QTcpSocket::connectToHost handles DNS internally. Kept for future
	// use should we want progress reporting in the NameLookup phase.
}

void ATModemDriverQt::TearDownSocket() {
	if (!mpSocket) return;

	// Disconnect every signal so any queued events that fire after this
	// point are no-ops. Pass `this` as both sender filter and receiver
	// filter so we don't accidentally tear down unrelated connections.
	QObject::disconnect(mpSocket, nullptr, this, nullptr);

	if (mpSocket->state() != QAbstractSocket::UnconnectedState) {
		mpSocket->disconnectFromHost();
		// If the peer doesn't ack the FIN promptly, force the abort so
		// we don't sit on a half-closed socket. QTcpSocket aborts safely
		// even from inside a slot.
		if (mpSocket->state() != QAbstractSocket::UnconnectedState)
			mpSocket->abort();
	}

	mpSocket->deleteLater();
	mpSocket = nullptr;
}

void ATModemDriverQt::TearDownServer() {
	if (!mpServer) return;

	QObject::disconnect(mpServer, nullptr, this, nullptr);
	if (mpServer->isListening())
		mpServer->close();
	mpServer->deleteLater();
	mpServer = nullptr;
}

void ATModemDriverQt::ReportEvent(ATModemPhase phase, ATModemEvent event) {
	if (mpCallback)
		mpCallback->OnEvent(this, phase, event);
}

void ATModemDriverQt::LogF(const char *fmt, ...) {
	if (!mbLoggingEnabled) return;
	va_list val;
	va_start(val, fmt);
	mLogMessages.append_vsprintf(fmt, val);
	va_end(val);
}

} // namespace

IATModemDriver *ATCreateModemDriverTCP() {
	return new ATModemDriverQt;
}
