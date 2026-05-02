//	Altirra - Qt port
//	Cheater dialog. Lets the user search 6502 RAM for a value, narrow
//	candidates with successive snapshots (equal/not-equal/less/greater/
//	equal-to-ref), and freeze chosen addresses to a fixed value.

#pragma once

class QWidget;
class ATSimulator;

void ATShowCheaterDialog(QWidget *parent, ATSimulator *sim);
