#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QObject>
#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QDialog>
#include <QFontDialog>
#include <QPushButton>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>


class MainWindow;

class PrefEditPage;
class PrefPreviewPage;
class PrefBmsPage;

class PrefGeneralPage : public QWidget
{
	Q_OBJECT

private:
	QComboBox *language;
	QComboBox *outputFormat;
	QStringList outputFormatList;
	QComboBox *saveJsonFormat;
	QStringList saveJsonFormatList;
	QPushButton *fontButton;

	QString LanguageKeyOf(int index);
	int LanguageIndexOf(QString key);

	bool uiFontDefault;
	QFont uiFont;

	void UpdateUIFont();

private slots:
	void OnFontButton();
	void OnFontResetButton();

public:
	PrefGeneralPage(QWidget *parent);

	void load();
	void store();

};


class Preferences : public QDialog
{
	Q_OBJECT

private:
	MainWindow *mainWindow;
	QListWidget *list;
	QStackedWidget *pages;

	PrefGeneralPage *generalPage;
	PrefEditPage *editPage;
	PrefPreviewPage *previewPage;
	PrefBmsPage *bmsPage;

	virtual void showEvent(QShowEvent *event);
	virtual void hideEvent(QHideEvent *event);

private slots:
	void PageChanged(QListWidgetItem *current, QListWidgetItem *previous);

public:
	Preferences(MainWindow *mainWindow);
	virtual ~Preferences();

};


#endif // PREFERENCES_H
