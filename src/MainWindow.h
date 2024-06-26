#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QDockWidget>
#include <QMainWindow>
#include <QApplication>
#include <QSettings>
#include <QMenuBar>
#include <QStatusBar>
#include <QMimeData>
#include "AudioPlayer.h"
#include "ChannelFindTools.h"
#include "SequenceTools.h"
#include "document/Document.h"
#include "document/SoundChannel.h"


class ViewMode;
class SequenceView;
class InfoView;
class ChannelInfoView;
class BpmEditView;
class NoteEditView;
class ExternalViewer;
class Preferences;
class SequenceTools;
class ChannelFindTools;
class ExternalViewerTools;
class MainWindow;


class StatusBarSection : public QWidget
{
	Q_OBJECT

public:
	static const int BaseHeight;

private:
	QString name;
	QIcon icon;
	QString text;
	int baseWidth;

private:
	virtual QSize minimumSizeHint() const;
	virtual QSize sizeHint() const;
	virtual void paintEvent(QPaintEvent *event);

public:
	StatusBarSection(QString name, QIcon icon, int baseWidth);
	virtual ~StatusBarSection();

	void SetIcon(QIcon icon=QIcon());
	void SetText(QString text=QString());
};


class StatusBar : public QStatusBar
{
	Q_OBJECT

private:
	MainWindow *mainWindow;
	StatusBarSection *objectSection;
	StatusBarSection *absoluteLocationSection;
	StatusBarSection *compositeLocationSection;
	StatusBarSection *realTimeSection;
	StatusBarSection *laneSection;

public:
	StatusBar(MainWindow *mainWindow);
	virtual ~StatusBar();

	StatusBarSection *GetObjectSection() const{ return objectSection; }
	StatusBarSection *GetAbsoluteLocationSection() const{ return absoluteLocationSection; }
	StatusBarSection *GetCompositeLocationSection() const{ return compositeLocationSection; }
	StatusBarSection *GetRealTimeSection() const{ return realTimeSection; }
	StatusBarSection *GetLaneSection() const{ return laneSection; }
};




class MainWindow : public QMainWindow
{
	Q_OBJECT

	friend class SequenceTools;
	friend class ChannelFindTools;

public:
	static const char *SettingsGroup;
	static const char *SettingsGeometryKey;
	static const char *SettingsWindowStateKey;
	static const char *SettingsWidgetsStateKey;
	static const char *SettingsHideInactiveSelectedViewKey;

private:
	QSettings *settings;
	Preferences *preferences;
	StatusBar *statusBar;
	AudioPlayer *audioPlayer;
	SequenceTools *sequenceTools;
	ChannelFindTools *channelFindTools;
	ExternalViewerTools *externalViewerTools;
	SequenceView *sequenceView;
	InfoView *infoView;
	ChannelInfoView *channelInfoView;
	ExternalViewer *externalViewer;

	QDockWidget *selectedObjectsDockWidget;
	BpmEditView *bpmEditView;
	NoteEditView *noteEditView;

	Document *document;

	ViewMode *viewMode;
	int currentChannel;

private:
	QAction *actionFileNew;
	QAction *actionFileOpen;
	QAction *actionFileSave;
	QAction *actionFileSaveAs;
	QAction *actionFileMasterOut;
	QAction *actionFileQuit;

	QAction *actionEditUndo;
	QAction *actionEditRedo;
	QAction *actionEditCut;
	QAction *actionEditCopy;
	QAction *actionEditPaste;
	QAction *actionEditSelectAll;
	QAction *actionEditDelete;
	QAction *actionEditTransferToKey;
	QAction *actionEditTransferToBgm;
	QAction *actionEditSeparateLayeredNotes;
	QAction *actionEditModeEdit;
	QAction *actionEditModeWrite;
	QAction *actionEditModeInteractive;
	QAction *actionEditLockCreation;
	QAction *actionEditLockDeletion;
	QAction *actionEditLockVerticalMove;
	QAction *actionEditPlay;
	QAction *actionEditClearMasterCache;
	QAction *actionEditPreferences;

	QAction *actionViewTbSeparator;
	QAction *actionViewDockSeparator;
	QAction *actionViewFullScreen;
	QAction *actionViewSnapToGrid;
	QAction *actionViewDarkenNotesInInactiveChannels;
	QAction *actionViewZoomIn;
	QAction *actionViewZoomOut;
	QAction *actionViewZoomReset;

	QAction *actionChannelNew;
	QAction *actionChannelPrev;
	QAction *actionChannelNext;
	QAction *actionChannelMoveLeft;
	QAction *actionChannelMoveRight;
	QAction *actionChannelDestroy;
	QAction *actionChannelSelectFile;
	QAction *actionChannelPreviewSource;

	QAction *actionHelpAbout;
	QAction *actionHelpAboutQt;

	QMap<ViewMode*, QAction*> viewModeActionMap;

	bool closing;

	QMenu *menuView;
	QMenu *menuViewMode;
	QMenu *menuViewChannelLane;
	QMenu *menuChannelFind;
	QAction *actionViewSkinSettingSeparator;
	QMenu *menuPreview;

private:
	void ReplaceDocument(Document *newDocument);
	bool Save();
	bool EnsureClosingFile();
	static bool IsSourceFileExtension(const QString &ext);
	static bool IsSoundFileExtension(const QString &ext);

private slots:
	void FileNew();
	void FileOpen();
	void FileOpen(QString path);
	void FileSave();
	void FileSaveAs();
	void MasterOut();
	void EditUndo();
	void EditRedo();
	void EditPreferences();
	void ViewFullScreen();
	void ChannelNew();
	void ChannelPrev();
	void ChannelNext();
	void ChannelMoveLeft();
	void ChannelMoveRight();
	void ChannelDestroy();
	void ChannelSelectFile();
	void ChannelPreviewSource();
	void ChannelsNew(QList<QString> filePaths);
	void HelpAbout();

	void FilePathChanged();
	void HistoryChanged();

	void OnCurrentChannelChanged(int ichannel);
	void OnTimeMappingChanged();
	void OnNotesEdited();
	void OnBpmEdited();

	void SaveFormatChanged(BmsonIO::BmsonVersion version);

	void ChannelFindKeywordChanged(QString keyword);
	void ChannelFindNext(QString keyword);
	void ChannelFindPrev(QString keyword);

	void EnableMasterChannelChanged(bool value);
	void ClearMasterCache();

public slots:
	void SetViewMode(ViewMode *mode);

signals:
	void ViewModeChanged(ViewMode *mode);
	void CurrentChannelChanged(int ichannel);

public:
	explicit MainWindow(QSettings *settings);
	virtual ~MainWindow();

	//virtual bool eventFilter(QObject *object, QEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dragMoveEvent(QDragMoveEvent *event);
	virtual void dragLeaveEvent(QDragLeaveEvent *event);
	virtual void dropEvent(QDropEvent *event);
	virtual void closeEvent(QCloseEvent *event);

	QSettings *GetSettings() const{ return settings; }
	StatusBar *GetStatusBar() const{ return statusBar; }
	AudioPlayer *GetAudioPlayer() const{ return audioPlayer; }
	BpmEditView *GetBpmEditTool() const{ return bpmEditView; }
	NoteEditView *GetNoteEditView() const{ return noteEditView; }
	ExternalViewer *GetExternalViewer() const{ return externalViewer; }

	SequenceView *GetActiveSequenceView() const;

	QMenu *GetMenuView() const{ return menuView; }
	QAction *GetViewSkinSettingSeparator() const{ return actionViewSkinSettingSeparator; }
	QMenu *GetMenuPreview() const{ return menuPreview; }

	ExternalViewerTools *GetExternalViewerTools() const{ return externalViewerTools; }

	int GetCurrentChannel() const{ return currentChannel; }
	ViewMode *GetCurrentViewMode() const{ return viewMode; }

	void SetSelectedObjectsView(QWidget *view);
	void UnsetSelectedObjectsView(QWidget *view);

	void OpenFiles(QStringList filePaths);
	void OpenBmson(QString path);
	void OpenBms(QString path);

	bool WarningFileTraversals(QStringList filePaths);
};


class App : public QApplication
{
	Q_OBJECT

public:
	static const char* SettingsLanguageKey;

private:
	static const char* SettingsVersionKey;
	static const int SettingsVersion;

	QSettings *settings;
	MainWindow *mainWindow;
	virtual bool event(QEvent *e);

public:
	App(int argc, char *argv[]);
	virtual ~App();

	static App* Instance();
	QSettings *GetSettings() const{ return settings; }
	MainWindow *GetMainWindow() const{ return mainWindow; }
};


#endif // MAINWINDOW_H
