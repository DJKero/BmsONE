#ifndef VIEWMODE_H
#define VIEWMODE_H

#include <QtCore>

class ViewMode : public QObject
{
	Q_OBJECT

public:
	enum Mode{
		MODE_BEAT_5K       = 0x00010501,
		MODE_BEAT_7K       = 0x00010701,
		MODE_BEAT_10K      = 0x00010502,
		MODE_BEAT_14K      = 0x00010702,
		MODE_POPN_5K       = 0x00020501,
		MODE_POPN_9K       = 0x00020901,
		MODE_CIRC_SINGLE   = 0x00110401,
		MODE_CIRC_DOUBLE   = 0x00110402,
		MODE_GENERIC_6KEYS = 0x01000601,
		MODE_GENERIC_7KEYS = 0x01000701,
		MODE_PLAIN         = 0x10000000,
	//
		MODE_KEY_COUNT_SHIFT   = 0x00000100,
		MODE_GENERIC_KEYS_BASE = 0x01000001,
	};
	static Mode MODE_GENERIC_N_KEYS(int n){
		return Mode(MODE_GENERIC_KEYS_BASE | (n * (int)MODE_KEY_COUNT_SHIFT));
	}

	struct LaneDef
	{
		int Lane;
		QString Name;

		LaneDef(){}
		LaneDef(int lane, QString name) : Lane(lane), Name(name){}
	};

private:
	Mode mode;
	QString name;
	QMap<int, LaneDef> lanes;

	static QStringList ModeHints;
	static QMap<QString, ViewMode*> ModeLibrary;

	static ViewMode *VM_Beat5k;
	static ViewMode *VM_Beat7k;
	static ViewMode *VM_Beat10k;
	static ViewMode *VM_Beat14k;
	static ViewMode *VM_Popn5k;
	static ViewMode *VM_Popn9k;
	//static ViewMode *VM_Dance4;
	//static ViewMode *VM_Dance8;
	static ViewMode *VM_CircularSingle;
	static ViewMode *VM_CircularDouble;
	static ViewMode *VM_Generic6Keys;
	static ViewMode *VM_Generic7Keys;
	static ViewMode *VM_Plain;

private:
	ViewMode(QString name, Mode mode);
	virtual ~ViewMode();
	static void PrepareModeLibrary();

public:
	Mode GetMode() const{ return mode; }
	QString GetName() const{ return name; }
	QMap<int, LaneDef> GetLaneDefinitions() const{ return lanes; }

	static ViewMode *GetViewMode(QString modeHint);
	static ViewMode *GetViewModeNf(QString modeHint);
	static QList<ViewMode*> GetAllViewModes();
	static QStringList GetAllModeHints();

	static ViewMode *ViewModeBeat5k();
	static ViewMode *ViewModeBeat7k();
	static ViewMode *ViewModeBeat10k();
	static ViewMode *ViewModeBeat14k();
	static ViewMode *ViewModePopn5k();
	static ViewMode *ViewModePopn9k();
	static ViewMode *ViewModeCircularSingle();
	static ViewMode *ViewModeCircularDouble();
	static ViewMode *ViewModeGenericNKeys(int n);
	static ViewMode *ViewModePlain();
};

#endif // VIEWMODE_H
