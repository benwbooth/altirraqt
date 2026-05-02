//	Altirra - Atari 800/800XL/5200 emulator
//	ATNetworkSockets - POSIX stub
//	Copyright (C) 2024 Ben Booth (Qt port)
//
//	Upstream ATNetworkSockets bridges ATNetwork's virtual IP stack to the real
//	host network using Winsock's WSAAsyncSelect + hidden-window message pump.
//	That architecture is Win32-specific; porting it means a full rewrite
//	around select()/poll()/epoll in a worker thread.
//
//	For now this file provides no-op implementations of the public factory
//	functions. The emulator's internal virtual networking (DHCP, TCP/UDP
//	between guest programs, etc.) continues to work since ATNetwork is fully
//	self-contained. What does NOT work until the real port lands:
//	  * ATNetConnect / ATNetListen / ATNetBind - no host-side sockets
//	  * ATNetLookup - no DNS
//	  * ATCreateNetSockWorker - guest cannot talk to the outside world
//	  * ATCreateNetSockVxlanTunnel - no VXLAN emulated LAN
//
//	When replacing this: implement ATNetConnect/Listen/Bind atop POSIX
//	socket()/bind()/connect()/listen(); implement the async dispatcher on a
//	background thread using select() or epoll; implement ATNetLookup with
//	getaddrinfo_a() or a thread-pool wrapper around getaddrinfo().

#include <stdafx.h>
#include <vd2/system/refcount.h>
#include <at/atnetwork/socket.h>
#include <at/atnetwork/emusocket.h>
#include <at/atnetworksockets/nativesockets.h>
#include <at/atnetworksockets/worker.h>
#include <at/atnetworksockets/vxlantunnel.h>

bool ATSocketInit()        { return true; }
void ATSocketPreShutdown() {}
void ATSocketShutdown()    {}

namespace {

class ATNetLookupFailed final : public vdrefcounted<IATNetLookupResult> {
public:
	void SetOnCompleted(IATAsyncDispatcher *, vdfunction<void(const ATSocketAddress&)> fn, bool callIfReady) override {
		if (callIfReady)
			fn(mAddr);
	}
	bool Completed() const override { return true; }
	bool Succeeded() const override { return false; }
	const ATSocketAddress& Address() const override { return mAddr; }

	ATSocketAddress mAddr {};
};

class ATNetSockWorkerStub final
	: public vdrefcounted<IATNetSockWorker>
	, public IATEmuNetSocketListener
	, public IATEmuNetUdpSocketListener
{
public:
	IATEmuNetSocketListener    *AsSocketListener() override { return this; }
	IATEmuNetUdpSocketListener *AsUdpListener()    override { return this; }

	void ResetAllConnections() override {}

	bool GetHostAddressesForLocalAddress(bool, uint32, uint16, uint32, uint16,
	                                     ATSocketAddress&, ATSocketAddress&) const override {
		return false;
	}

	bool OnSocketIncomingConnection(uint32, uint16, uint32, uint16, IATStreamSocket *, IATSocketHandler **) override {
		return false;
	}

	void OnUdpDatagram(const ATEthernetAddr&, uint32, uint16, uint32, uint16, const void *, uint32) override {}
};

class ATNetSockVxlanTunnelStub final : public vdrefcounted<IATNetSockVxlanTunnel> {};

} // namespace

vdrefptr<IATNetLookupResult> ATNetLookup(const wchar_t *, const wchar_t *) {
	return vdrefptr<IATNetLookupResult>(new ATNetLookupFailed);
}

vdrefptr<IATStreamSocket> ATNetConnect(const wchar_t *, const wchar_t *, bool)       { return {}; }
vdrefptr<IATStreamSocket> ATNetConnect(const ATSocketAddress&, bool)                 { return {}; }
vdrefptr<IATListenSocket> ATNetListen(const ATSocketAddress&, bool)                  { return {}; }
vdrefptr<IATListenSocket> ATNetListen(ATSocketAddressType, uint16, bool)             { return {}; }
vdrefptr<IATDatagramSocket> ATNetBind(const ATSocketAddress&, bool)                  { return {}; }

void ATCreateNetSockWorker(IATEmuNetUdpStack *, IATEmuNetTcpStack *, bool, uint32, uint16, IATNetSockWorker **pp) {
	auto *p = new ATNetSockWorkerStub;
	p->AddRef();
	*pp = p;
}

void ATCreateNetSockVxlanTunnel(uint32, uint16, uint16, IATEthernetSegment *, uint32, IATAsyncDispatcher *, IATNetSockVxlanTunnel **pp) {
	auto *p = new ATNetSockVxlanTunnelStub;
	p->AddRef();
	*pp = p;
}
