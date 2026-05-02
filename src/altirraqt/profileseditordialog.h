//	Altirra - Qt port
//	Profiles editor dialog. Lets the user rename, duplicate, and delete
//	profiles in the `profiles/<name>/...` QSettings namespace already used
//	by `System -> Save Profile.../Load Profile`.

#pragma once

class QWidget;
class QSettings;

void ATShowProfilesEditorDialog(QWidget *parent, QSettings *settings);
