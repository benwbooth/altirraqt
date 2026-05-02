//	Altirra - Qt port
//	Minimal Qt-backed implementation of vd2/Dita/services.h.
//	Copyright (C) 2026 Ben Booth
//
//	Upstream Dita/source/services.cpp uses Win32 GetOpenFileName/SHBrowseFor
//	/CoInitialize. We route the same public API through QFileDialog and
//	QColorDialog. The VDFileDialogOption structured-option array isn't
//	emulated — QFileDialog doesn't have a direct equivalent for the custom
//	option widgets (bool/int tweaks next to the filespec), and callers read
//	values back through pOptVals. The handful of call sites that use it pass
//	no options or accept defaults, so we ignore pOptions here.
//
//	Remembered-path persistence (VDSetLastLoadSavePath etc.) is kept in a
//	simple in-memory map for now; the UI layer's settings port can back it
//	with QSettings later.

#include <stdafx.h>

#include <unordered_map>

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QColorDialog>
#include <QFileDialog>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <vd2/Dita/services.h>

namespace {

QWidget *toWidget(VDGUIHandle h) {
	return reinterpret_cast<QWidget *>(h);
}

QString pathForKey(long key);
void setPathForKey(long key, const QString& path);

std::unordered_map<long, QString>& keyedPaths() {
	static std::unordered_map<long, QString> m;
	return m;
}

QString pathForKey(long key) {
	auto& m = keyedPaths();
	auto it = m.find(key);
	return it == m.end() ? QString() : it->second;
}

void setPathForKey(long key, const QString& path) {
	keyedPaths()[key] = path;
}

// Qt expects "Label (*.ext);;Label2 (*.ext2)" — upstream uses a |-separated
// "Label|*.ext|Label2|*.ext2|" format. Convert.
QString convertFilterSpec(const wchar_t *filters) {
	if (!filters)
		return {};

	QStringList out;
	const wchar_t *p = filters;
	while (*p) {
		const wchar_t *label = p;
		while (*p && *p != L'|') ++p;
		if (!*p)
			break;
		QString labelText = QString::fromWCharArray(label, (int)(p - label));
		++p;

		const wchar_t *pattern = p;
		while (*p && *p != L'|') ++p;
		QString patternText = QString::fromWCharArray(pattern, (int)(p - pattern));
		if (*p) ++p;

		out << QStringLiteral("%1 (%2)").arg(labelText, patternText);
	}
	return out.join(QStringLiteral(";;"));
}

} // namespace

const VDStringW VDGetLoadFileName(long nKey, VDGUIHandle ctxParent, const wchar_t *pszTitle,
                                  const wchar_t *pszFilters, const wchar_t *pszExt,
                                  const VDFileDialogOption *, int *) {
	const QString path = QFileDialog::getOpenFileName(
		toWidget(ctxParent),
		pszTitle ? QString::fromWCharArray(pszTitle) : QString(),
		pathForKey(nKey),
		convertFilterSpec(pszFilters)
	);

	if (path.isEmpty())
		return VDStringW();

	setPathForKey(nKey, path);

	std::wstring w = path.toStdWString();
	return VDStringW(w.c_str());
}

const VDStringW VDGetSaveFileName(long nKey, VDGUIHandle ctxParent, const wchar_t *pszTitle,
                                  const wchar_t *pszFilters, const wchar_t *pszExt,
                                  const VDFileDialogOption *, int *) {
	const QString path = QFileDialog::getSaveFileName(
		toWidget(ctxParent),
		pszTitle ? QString::fromWCharArray(pszTitle) : QString(),
		pathForKey(nKey),
		convertFilterSpec(pszFilters)
	);

	if (path.isEmpty())
		return VDStringW();

	setPathForKey(nKey, path);

	std::wstring w = path.toStdWString();
	return VDStringW(w.c_str());
}

void VDSetLastLoadSavePath(long nKey, const wchar_t *path) {
	setPathForKey(nKey, QString::fromWCharArray(path ? path : L""));
}

const VDStringW VDGetLastLoadSavePath(long nKey) {
	const QString p = pathForKey(nKey);
	std::wstring w = p.toStdWString();
	return VDStringW(w.c_str());
}

void VDSetLastLoadSaveFileName(long nKey, const wchar_t *fileName) {
	setPathForKey(nKey, QString::fromWCharArray(fileName ? fileName : L""));
}

const VDStringW VDGetDirectory(long nKey, VDGUIHandle ctxParent, const wchar_t *pszTitle) {
	const QString path = QFileDialog::getExistingDirectory(
		toWidget(ctxParent),
		pszTitle ? QString::fromWCharArray(pszTitle) : QString(),
		pathForKey(nKey)
	);

	if (path.isEmpty())
		return VDStringW();

	setPathForKey(nKey, path);

	std::wstring w = path.toStdWString();
	return VDStringW(w.c_str());
}

void VDLoadFilespecSystemData() {}
void VDSaveFilespecSystemData() {}
void VDClearFilespecSystemData() { keyedPaths().clear(); }
void VDInitFilespecSystem() {}

sint32 VDUIShowColorPicker(VDGUIHandle parent, uint32 initialColor, vdspan<const uint32>, const char *) {
	const QColor initial(
		(initialColor >> 16) & 0xff,
		(initialColor >>  8) & 0xff,
		(initialColor      ) & 0xff
	);
	const QColor chosen = QColorDialog::getColor(initial, toWidget(parent));
	if (!chosen.isValid())
		return -1;

	return (sint32)(((uint32)chosen.red() << 16) |
	                ((uint32)chosen.green() << 8) |
	                ((uint32)chosen.blue()));
}
