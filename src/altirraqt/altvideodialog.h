//	altirraqt — Alt video output viewer (XEP80, etc).

#pragma once

class ATSimulator;
class QWidget;

// Show (or raise) the alt-video viewer. The viewer hosts a tab per
// IATDeviceVideoOutput registered by the simulator (XEP80, 1090, BIT3
// FullView, etc.). Each tab paints the device's framebuffer at ~60 Hz.
void ATShowAltVideoDialog(QWidget *parent, ATSimulator *sim);
