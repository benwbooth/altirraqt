//	altirraqt — Qt implementation of the generic-dialog helper.
//
//	Upstream's `ATUIShowGenericDialog` is a Win32 dialog box. The Qt port
//	maps it to QMessageBox::question/information/warning/critical with
//	StandardButton overrides so the Allow/Deny + ignore-tag semantics line
//	up. Only the device-side users of these helpers (Browser, etc.) call
//	in; the rest of the Qt UI uses QMessageBox directly.

#include <QApplication>
#include <QCheckBox>
#include <QMessageBox>
#include <QPointer>
#include <QSettings>
#include <QString>
#include <QWidget>

#undef signals
#undef slots

#include <vd2/system/vdtypes.h>
#include <vd2/system/VDString.h>
#include <vd2/system/vectors.h>

#include <at/atnativeui/genericdialog.h>

namespace {

QString defaultCaption = QStringLiteral("Altirra");

QSettings& ignoreStore() {
	static QSettings s;
	return s;
}

QString ignoreKey(const char *tag) {
	return QStringLiteral("dialog/ignore/") + QString::fromUtf8(tag ? tag : "");
}

bool isIgnored(const char *tag) {
	if (!tag || !*tag) return false;
	return ignoreStore().value(ignoreKey(tag), false).toBool();
}

void setIgnored(const char *tag, bool on) {
	if (!tag || !*tag) return;
	ignoreStore().setValue(ignoreKey(tag), on);
}

QMessageBox::StandardButton resultToButton(ATUIGenericResult r) {
	switch (r) {
		case kATUIGenericResult_OK:    return QMessageBox::Ok;
		case kATUIGenericResult_Allow: return QMessageBox::Yes;
		case kATUIGenericResult_Yes:   return QMessageBox::Yes;
		case kATUIGenericResult_Deny:  return QMessageBox::No;
		case kATUIGenericResult_No:    return QMessageBox::No;
		case kATUIGenericResult_Cancel:
		default:                       return QMessageBox::Cancel;
	}
}

} // namespace

void ATUISetDefaultGenericDialogCaption(const wchar_t *s) {
	defaultCaption = QString::fromWCharArray(s ? s : L"");
}

void ATUIGenericDialogUndoAllIgnores() {
	auto& s = ignoreStore();
	s.beginGroup(QStringLiteral("dialog/ignore"));
	s.remove(QString());
	s.endGroup();
}

ATUIGenericResult ATUIShowGenericDialog(const ATUIGenericDialogOptions& opts) {
	if (opts.mpIgnoreTag && isIgnored(opts.mpIgnoreTag)) {
		// User previously chose "ignore" — auto-pick the first allowed
		// non-Cancel result mask bit (Allow > OK > Yes > No > Deny).
		if (opts.mValidIgnoreMask & kATUIGenericResultMask_Allow)  return kATUIGenericResult_Allow;
		if (opts.mValidIgnoreMask & kATUIGenericResultMask_OK)     return kATUIGenericResult_OK;
		if (opts.mValidIgnoreMask & kATUIGenericResultMask_Yes)    return kATUIGenericResult_Yes;
		if (opts.mValidIgnoreMask & kATUIGenericResultMask_No)     return kATUIGenericResult_No;
		if (opts.mValidIgnoreMask & kATUIGenericResultMask_Deny)   return kATUIGenericResult_Deny;
		return kATUIGenericResult_Cancel;
	}

	QMessageBox box(static_cast<QWidget *>(nullptr));
	box.setWindowTitle(opts.mpTitle
		? QString::fromWCharArray(opts.mpTitle)
		: defaultCaption);
	box.setText(opts.mpMessage ? QString::fromWCharArray(opts.mpMessage) : QString());

	switch (opts.mIconType) {
		case kATUIGenericIconType_Info:    box.setIcon(QMessageBox::Information); break;
		case kATUIGenericIconType_Warning: box.setIcon(QMessageBox::Warning);     break;
		case kATUIGenericIconType_Error:   box.setIcon(QMessageBox::Critical);    break;
		default:                           box.setIcon(QMessageBox::NoIcon);      break;
	}

	QMessageBox::StandardButtons stdBtns = QMessageBox::NoButton;
	if (opts.mResultMask & kATUIGenericResultMask_OK)     stdBtns |= QMessageBox::Ok;
	if (opts.mResultMask & kATUIGenericResultMask_Cancel) stdBtns |= QMessageBox::Cancel;
	if (opts.mResultMask & kATUIGenericResultMask_Yes)    stdBtns |= QMessageBox::Yes;
	if (opts.mResultMask & kATUIGenericResultMask_No)     stdBtns |= QMessageBox::No;
	if (opts.mResultMask & kATUIGenericResultMask_Allow)  stdBtns |= QMessageBox::Yes;
	if (opts.mResultMask & kATUIGenericResultMask_Deny)   stdBtns |= QMessageBox::No;
	if (stdBtns == QMessageBox::NoButton) stdBtns = QMessageBox::Ok;
	box.setStandardButtons(stdBtns);

	QPointer<QCheckBox> ignoreCb;
	if (opts.mpIgnoreTag && opts.mValidIgnoreMask) {
		ignoreCb = new QCheckBox(QObject::tr("Don't ask again"), &box);
		box.setCheckBox(ignoreCb);
	}

	const auto pressed = (QMessageBox::StandardButton)box.exec();

	ATUIGenericResult result = kATUIGenericResult_Cancel;
	switch (pressed) {
		case QMessageBox::Ok:     result = kATUIGenericResult_OK;     break;
		case QMessageBox::Yes:
			result = (opts.mResultMask & kATUIGenericResultMask_Allow)
				? kATUIGenericResult_Allow
				: kATUIGenericResult_Yes;
			break;
		case QMessageBox::No:
			result = (opts.mResultMask & kATUIGenericResultMask_Deny)
				? kATUIGenericResult_Deny
				: kATUIGenericResult_No;
			break;
		case QMessageBox::Cancel:
		default: result = kATUIGenericResult_Cancel; break;
	}

	if (ignoreCb && ignoreCb->isChecked()
		&& opts.mpIgnoreTag
		&& (opts.mValidIgnoreMask & (UINT32_C(1) << result)))
	{
		setIgnored(opts.mpIgnoreTag, true);
		if (opts.mpCustomIgnoreFlag) *opts.mpCustomIgnoreFlag = true;
	}

	return result;
}

ATUIGenericResult ATUIShowGenericDialogAutoCenter(const ATUIGenericDialogOptions& opts) {
	return ATUIShowGenericDialog(opts);
}

bool ATUIConfirm(VDGUIHandle, const char *ignoreTag, const wchar_t *message, const wchar_t *title) {
	ATUIGenericDialogOptions opts {};
	opts.mpMessage = message;
	opts.mpTitle = title;
	opts.mpIgnoreTag = ignoreTag;
	opts.mIconType = kATUIGenericIconType_Warning;
	opts.mResultMask = kATUIGenericResultMask_OKCancel;
	opts.mValidIgnoreMask = kATUIGenericResultMask_OK;
	return ATUIShowGenericDialog(opts) == kATUIGenericResult_OK;
}

// Browser device + a few other paths call ATUIGetMainWindow() to get a
// parent HWND. On Qt we just hand back nullptr — QMessageBox without a
// parent floats independently, which is fine for the device-driven
// modal popups.
VDGUIHandle ATUIGetMainWindow();
VDGUIHandle ATUIGetMainWindow() { return nullptr; }
