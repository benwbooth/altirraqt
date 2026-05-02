//	Altirra - Qt port
//	Printer Output dialog. Surfaces text/ESC-P output from any
//	IATPrinterOutput instances created by simulated printer devices,
//	rendered into a read-only QPlainTextEdit. Listens to the
//	ATPrinterOutputManager so newly-created outputs (e.g. when the user
//	attaches a printer device after the dialog is open) appear
//	automatically.

#pragma once

class QWidget;
class ATSimulator;

void ATShowPrinterOutputDialog(QWidget *parent, ATSimulator *sim);
