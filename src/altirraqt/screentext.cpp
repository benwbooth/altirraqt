//	altirraqt — Atari screen-text grabber.

#include "screentext.h"

#include <QString>
#include <QStringList>

#undef signals
#undef slots

#include <vd2/system/vdtypes.h>
#include <vd2/system/VDString.h>

#include <at/atcore/atascii.h>

#include <antic.h>
#include <irqcontroller.h>
#include <memorymanager.h>
#include <simulator.h>

namespace {

// Port of ATUIVideoDisplayWindow::ReadText — reads up to 48 bytes of a
// character-mode row, returning the actual length read. `intl` reports
// whether the international character set was active.
int readTextRow(ATSimulator& sim, int yc, uint8 *dst, bool& intl) {
	auto *mem = sim.GetMemoryManager();
	if (!mem) return 0;

	const ATAnticEmulator::DLHistoryEntry *dlhist = sim.GetAntic().GetDLHistory();
	const auto& dle = dlhist[yc];
	intl = false;
	if (!dle.mbValid) return 0;

	switch (dle.mControl & 15) {
		case 2: case 3: case 6: case 7: break;
		default: return 0;
	}

	static const int kWidthLookup[2][4] = {
		{ 0, 16, 20, 24 }, { 0, 20, 24, 24 },
	};
	const int len = (dle.mControl & 4 ? 1 : 2)
	              * kWidthLookup[(dle.mControl & 0x10) != 0][dle.mDMACTL & 3];
	if (len <= 0) return 0;

	uint8 raw[48];
	if (len > 48) return 0;
	mem->DebugAnticReadMemory(raw, dle.mPFAddress, len);

	static const uint8 kInternalToATASCIIXorTab[4] = { 0x20, 0x60, 0x40, 0x00 };
	const uint8 mask  = (dle.mControl & 4) ? 0x3f : 0xff;
	const uint8 xorv  = (dle.mControl & 4) && (dle.mCHBASE & 1) ? 0x40 : 0x00;
	if (!(dle.mControl & 4) && dle.mCHBASE == (0xCC >> 1))
		intl = true;

	for (int i = 0; i < len; ++i) {
		uint8 c = raw[i];
		c &= mask;
		c ^= xorv;
		c ^= kInternalToATASCIIXorTab[(c & 0x60) >> 5];
		dst[i] = c;
	}
	return len;
}

void appendEscaped(QString& out, const uint8 *data, int n, bool intl) {
	const auto& tab = kATATASCIITables.mATASCIIToUnicode[intl ? 1 : 0];
	uint8 inv = 0;
	bool started = false;
	for (int i = 0; i < n; ++i) {
		uint8 c = data[i];
		if (!started) {
			if (c == 0x20) continue;
			started = true;
		}
		if ((c ^ inv) & 0x80) {
			inv ^= 0x80;
			out += QStringLiteral("{inv}");
		}
		c &= 0x7F;
		const uint16 wc = tab[c];
		if (wc < 0x100) {
			out += QChar((ushort)wc);
		} else if (c >= 0x01 && c < 0x1B) {
			out += QStringLiteral("{^}");
			out += QChar((char)('a' + (c - 0x01)));
		} else if (c == 0x1B) out += QStringLiteral("{esc}{esc}");
		else if (c == 0x1C) out += inv ? QStringLiteral("{esc}{+delete}") : QStringLiteral("{esc}{up}");
		else if (c == 0x1D) out += inv ? QStringLiteral("{esc}{+insert}") : QStringLiteral("{esc}{down}");
		else if (c == 0x1E) out += inv ? QStringLiteral("{esc}{^tab}")    : QStringLiteral("{esc}{left}");
		else if (c == 0x1F) out += inv ? QStringLiteral("{esc}{+tab}")    : QStringLiteral("{esc}{right}");
		else                 out += QChar((ushort)wc);
	}
	while (out.endsWith(QChar(' '))) out.chop(1);
	if (inv) out += QStringLiteral("{inv}");
}

} // namespace

QString ATGrabScreenText(ATSimulator& sim, ATScreenCopyMode mode) {
	QStringList rows;
	uint8 buf[48];
	for (int y = 0; y < 248; ++y) {
		bool intl = false;
		const int n = readTextRow(sim, y, buf, intl);
		if (n <= 0) continue;

		QString line;
		switch (mode) {
			case ATScreenCopyMode::Hex: {
				for (int i = 0; i < n; ++i) {
					if (i) line += QChar(' ');
					line += QStringLiteral("%1")
						.arg(buf[i], 2, 16, QLatin1Char('0')).toUpper();
				}
				break;
			}
			case ATScreenCopyMode::Escaped:
				appendEscaped(line, buf, n, intl);
				break;
			case ATScreenCopyMode::Unicode:
			case ATScreenCopyMode::ASCII: {
				const auto& tab = kATATASCIITables.mATASCIIToUnicode[intl ? 1 : 0];
				int base = 0;
				int actual = n;
				while (base < actual && (buf[base] & 0x7F) == 0x20) ++base;
				while (actual > base && (buf[actual - 1] & 0x7F) == 0x20) --actual;
				for (int i = base; i < actual; ++i) {
					uint16 wc = tab[buf[i] & 0x7F];
					if (mode == ATScreenCopyMode::Unicode)
						line += QChar((ushort)wc);
					else
						line += (wc < 0x100) ? QChar((ushort)wc) : QChar(' ');
				}
				break;
			}
		}
		rows << line;
	}
	return rows.join(QChar('\n'));
}
