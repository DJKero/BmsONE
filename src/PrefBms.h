#ifndef PREFBMS_H
#define PREFBMS_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>


class Preferences;

class PrefBmsPage : public QWidget
{
	Q_OBJECT

private:
	QCheckBox *askTextEncoding;
	QCheckBox *askRandomValues;
	QCheckBox *askGameMode;

	QComboBox *defaultTextEncoding;
	QCheckBox *useRandomValues;
	QCheckBox *trustPlayerCommand;
	QCheckBox *ignoreExtension;
	QCheckBox *preferExModes;

	QLineEdit *minResolution;
	QLineEdit *maxResolution;
	QCheckBox *skipBetweenRandomAndIf;

public:
	PrefBmsPage(QWidget *parent);

	void load();
	void store();
};


#endif // PREFBMS_H
