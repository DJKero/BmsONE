#ifndef SEQUENCEVIEWINTERNAL
#define SEQUENCEVIEWINTERNAL

#include <QObject>
#include <QWidget>
#include <QAction>
#include <QContextMenuEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include "SequenceViewDef.h"
#include "../document/Document.h"
#include "../document/SoundChannel.h"


class MainWindow;
class StatusBar;
class SequenceView;
class SoundNoteView;
class SoundChannelHeader;
class SoundChannelFooter;
class SoundChannelView;
class SequenceViewCursor;



class SoundNoteView : public QObject
{
	Q_OBJECT

private:
	SoundChannelView *cview;
	SoundNote note;
	//bool selected;

public:
	SoundNoteView(SoundChannelView *cview, SoundNote note);
	virtual ~SoundNoteView();

	SoundChannelView *GetChannelView() const{ return cview; }
	SoundNote GetNote() const{ return note; }

	void UpdateNote(SoundNote note);
	//void SetSelected(bool selected);

};

/*
class SoundChannelHeader : public QWidget
{
	Q_OBJECT

private:
	SequenceView *sview;
	SoundChannelView *cview;

public:
	SoundChannelHeader(SequenceView *sview, SoundChannelView *cview);
	virtual ~SoundChannelHeader();

	virtual void paintEvent(QPaintEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent * event);
};
*/

class SoundChannelFooter : public QWidget
{
	Q_OBJECT

private:
	SequenceView *sview;
	SoundChannelView *cview;
	int internalWidth;

public:
	SoundChannelFooter(SequenceView *sview, SoundChannelView *cview);
	virtual ~SoundChannelFooter();

	virtual void paintEvent(QPaintEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent * event);

	void SetInternalWidth(int width);
};


class SoundChannelView : public QWidget
{
	Q_OBJECT

	friend class SoundChannelHeader;
	friend class SoundChannelFooter;

private:
	class Context{
	protected:
		SoundChannelView *cview;
		SequenceView *sview;
		Context *parent;
		Context(SoundChannelView *cview, Context *parent=nullptr);
	public:
		virtual ~Context();
		virtual bool IsTop() const{ return parent == nullptr; }
		virtual Context* Escape();

		virtual SoundChannelView::Context* KeyPress(QKeyEvent*);
		//virtual Context* KeyUp(QKeyEvent*){ return this; }
		//virtual Context* Enter(QEnterEvent*){ return this; }
		//virtual Context* Leave(QEnterEvent*){ return this; }
		virtual Context* MouseMove(QMouseEvent*){ return this; }
		virtual Context* MousePress(QMouseEvent*){ return this; }
		virtual Context* MouseRelease(QMouseEvent*){ return this; }
		virtual Context* MouseDoubleClick(QMouseEvent*){ return this; }
	};
	class EditModeContext;
	class EditModeSelectNotesContext;
	class WriteModeContext;
	class PreviewContext;

private:
	SequenceView *sview;
	SoundChannel *channel;
	QMap<int, SoundNoteView*> notes;
	bool current;
	bool selected;

	bool collapsed;
	qreal animation;
	int internalWidth;
	QImage *backBuffer;

	Context *context;

private:
	QAction *actionPreview;
	QAction *actionMoveLeft;
	QAction *actionMoveRight;
	QAction *actionDestroy;

	QAction *actionDeleteNotes;
	QAction *actionTransferNotes;

private:
	SoundNoteView *HitTestBGPane(int y, int time);
	void OnChannelMenu(QContextMenuEvent * event);
	bool DrawsWaveform() const;

private slots:
	void NoteInserted(SoundNote note);
	void NoteRemoved(SoundNote note);
	void NoteChanged(int oldLocation, SoundNote note);

	void RmsUpdated();

	void NameChanged();
	void Show();
	void ShowNoteLocation(int location);

	void Preview();
	void MoveLeft();
	void MoveRight();
	void Destroy();

	void DeleteNotes();
	void TransferNotes();

public:
	SoundChannelView(SequenceView *sview, SoundChannel *channel);
	virtual ~SoundChannelView();

	void UpdateBackBuffer(const QRect &rect);
	void UpdateWholeBackBuffer();
	void RemakeBackBuffer();
	void ScrollContents(int dy);
	void SetInternalWidth(int width);

	SoundChannel *GetChannel() const{ return channel; }
	QString GetName() const{ return channel->GetName(); }
	const QMap<int, SoundNoteView*> GetNotes() const{ return notes; }
	bool IsCurrent() const{ return current; }
	void SetCurrent(bool c){ current = c; update(); }
	bool IsSelected() const{ return selected; }
	void SetSelected(bool s){ selected = s; update(); }
	bool IsCollapsed() const{ return collapsed; }
	void SetCollapsed(bool c){ collapsed = c; }
	qreal GetAnimation() const{ return animation; }
	void SetAnimation(qreal a){ animation = a; }
	int GetInternalWidth() const{ return internalWidth; }

	virtual void paintEvent(QPaintEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void enterEvent(QEvent *event);
	virtual void leaveEvent(QEvent *event);

	void SetMode(SequenceEditMode mode);
};



class SequenceViewCursor : public QObject
{
	Q_OBJECT

public:
	enum class State{
		NOTHING,
		TIME,
		TIME_WITH_LANE,
		NEW_SOUND_NOTE,
		EXISTING_SOUND_NOTE,
		LAYERED_SOUND_NOTES_WITH_CONFLICT,
		NEW_BPM_EVENT,
		EXISTING_BPM_EVENT,
		NEW_BAR_LINE,
		EXISTING_BAR_LINE,
		TIME_SELECTION,
		KEY_NOTES_SELECTION,
		BGM_NOTES_SELECTION,
		BPM_EVENTS_SELECTION,
	};

private:
	SequenceView *sview;
	StatusBar *statusBar;
	State state;
	int time;
	int lane;
	SoundNote newSoundNote;
	SoundNoteView *existingSoundNote;
	QList<SoundNoteView*> layeredSoundNotes;
	BpmEvent bpmEvent;
	BarLine barLine;
	int timeBegin, timeEnd;
	QList<int> laneRange;
	int itemCountInRange;
	bool forceShowHLine;

private:
	QString AbsoluteLocationString(int t) const;
	QString CompositeLocationString(int t) const;
	QString RealTimeString(int t) const;
	QString GetAbsoluteLocationString() const;
	QString GetCompositeLocationString() const;
	QString GetRealTimeString() const;
	QString GetLaneString() const;
	QString GetAbsoluteLocationRangeString() const;
	QString GetCompositeLocationRangeString() const;
	QString GetRealTimeRangeString() const;
	QString GetLaneListString() const;

public:
	SequenceViewCursor(SequenceView *sview);
	virtual ~SequenceViewCursor();

	State GetState() const{ return state; }
	int GetTime() const{ return time; }
	int GetLane() const{ return lane; }
	SoundNote GetNewSoundNote() const{ return newSoundNote; }
	SoundNoteView *GetExistingSoundNote() const{ return existingSoundNote; }
	BpmEvent GetBpmEvent() const{ return bpmEvent; }
	BarLine GetBarLine() const{ return barLine; }

	void SetNothing();
	void SetTime(int time);
	void SetTimeWithLane(int time, int lane);
	void SetNewSoundNote(SoundNote note);
	void SetExistingSoundNote(SoundNoteView *note);
	void SetLayeredSoundNotesWithConflict(QList<SoundNoteView*> layeredNotes, NoteConflict conflict);
	void SetNewBpmEvent(BpmEvent event);
	void SetExistingBpmEvent(BpmEvent event);
	void SetNewBarLine(BarLine bar);
	void SetExistingBarLine(BarLine bar);
	void SetTimeSelection(int time, int timeBegin, int timeEnd);
	void SetKeyNotesSelection(int time, int timeBegin, int timeEnd, QList<int> lanes, int itemCount);
	void SetBgmNotesSelection(int time, int timeBegin, int timeEnd, int itemCount);
	void SetBpmEventsSelection(int time, int timeBegin, int timeEnd, int itemCount);

	bool IsNothing() const{ return state == State::NOTHING; }
	bool IsTimeWithLane() const{ return state == State::TIME_WITH_LANE; }
	bool IsNewSoundNote() const{ return state == State::NEW_SOUND_NOTE; }
	bool IsExistingSoundNote() const{ return state == State::EXISTING_SOUND_NOTE; }
	bool IsNewBpmEvent() const{ return state == State::NEW_BPM_EVENT; }
	bool IsExistingBpmEvent() const{ return state == State::EXISTING_BPM_EVENT; }
	bool IsNewBarLine() const{ return state == State::NEW_BAR_LINE; }
	bool IsExistingBarLine() const{ return state == State::EXISTING_BAR_LINE; }

	bool HasTime() const;
	bool HasLane() const;
	bool HasTimeRange() const;
	bool HasItemCount() const;

	void SetForceShowHLine(bool show);
	bool ShouldShowHLine() const;

signals:
	void Changed();
};



#endif // SEQUENCEVIEWINTERNAL

