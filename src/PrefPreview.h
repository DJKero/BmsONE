#ifndef PREFPREVIEWPAGE_H
#define PREFPREVIEWPAGE_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QVector>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>


class MainWindow;

class PrefPreviewPage : public QWidget
{
	Q_OBJECT

private:
	static const int DelayRatioSliderLevels;
	QVector<qreal> durationValues;

	QSlider *delayRatioSlider;
	QLabel *delayRatioEdit;
	QCheckBox *smoothing;
	QSlider *singleMaxDuration;
	QLabel *singleMaxDurationValue;
	QCheckBox *singleSoftFadeOut;

public:
	PrefPreviewPage(QWidget *parent, MainWindow *mainWindow);

	void load();
	void store();

private slots:
	void DelayRatioSliderChanged(int value);
	//void DelayRatioEditChanged(QString value);
	void SingleMaxDurationChanged(int value);
};


#endif // PREFPREVIEWPAGE_H
