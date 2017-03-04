#ifndef SEQUENCEVIEWCONTEXTS
#define SEQUENCEVIEWCONTEXTS

#include "SequenceView.h"


class SequenceView::EditModeContext
		: public SequenceView::Context
{
public:
	EditModeContext(SequenceView *sview);
	~EditModeContext();

	//virtual SequenceView::Context* Enter(QEnterEvent*);
	//virtual SequenceView::Context* Leave(QEnterEvent*);
	virtual SequenceView::Context* PlayingPane_MouseMove(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MousePress(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MouseRelease(QMouseEvent*);

	virtual Context* BpmArea_MouseMove(QMouseEvent*);
	virtual Context* BpmArea_MousePress(QMouseEvent*);
	virtual Context* BpmArea_MouseRelease(QMouseEvent*);
};


class SequenceView::EditModeSelectNotesContext
		: public SequenceView::Context
{
	SequenceView::CommandsLocker locker;
	Qt::MouseButton mouseButton;
	QRubberBand *rubberBand;
	int rubberBandOriginLaneX;
	int rubberBandOriginTime;

	~EditModeSelectNotesContext();
public:
	EditModeSelectNotesContext(SequenceView::EditModeContext *parent, SequenceView *sview, Qt::MouseButton button, int laneX, int iTime, QPoint pos);

	//virtual SequenceView::Context* Enter(QEnterEvent*);
	//virtual SequenceView::Context* Leave(QEnterEvent*);
	virtual SequenceView::Context* PlayingPane_MouseMove(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MousePress(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MouseRelease(QMouseEvent*);
};


class SequenceView::EditModeDragNotesContext
		: public SequenceView::Context
{
	SequenceView::CommandsLocker locker;
	Document::DocumentUpdateSoundNotesContext *updateNotesCxt;
	bool editLN;
	int dragNotesOriginLaneX;
	int dragNotesOriginTime;
	int dragNotesPreviousLaneX;
	int dragNotesPreviousTime;

	~EditModeDragNotesContext();
public:
	EditModeDragNotesContext(SequenceView::EditModeContext *parent, SequenceView *sview,
							 Document::DocumentUpdateSoundNotesContext *cxt, int laneX, int iTime, bool editLN=false);

	//virtual SequenceView::Context* Enter(QEnterEvent*);
	//virtual SequenceView::Context* Leave(QEnterEvent*);
	virtual SequenceView::Context* PlayingPane_MouseMove(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MousePress(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MouseRelease(QMouseEvent*);
};


class SequenceView::EditModeSelectBpmEventsContext
		: public SequenceView::Context
{
	SequenceView::CommandsLocker locker;
	Qt::MouseButton mouseButton;
	QRubberBand *rubberBand;
	int rubberBandOriginTime;

	~EditModeSelectBpmEventsContext();
public:
	EditModeSelectBpmEventsContext(SequenceView::EditModeContext *parent, SequenceView *sview, Qt::MouseButton button, int iTime, QPoint pos);

	virtual Context* MeasureArea_MouseMove(QMouseEvent*);
	virtual Context* MeasureArea_MousePress(QMouseEvent*);
	virtual Context* MeasureArea_MouseRelease(QMouseEvent*);

	virtual Context* BpmArea_MouseMove(QMouseEvent*);
	virtual Context* BpmArea_MousePress(QMouseEvent*);
	virtual Context* BpmArea_MouseRelease(QMouseEvent*);
};


class SequenceView::WriteModeContext
		: public SequenceView::Context
{
public:
	WriteModeContext(SequenceView *sview);
	~WriteModeContext();

	//virtual SequenceView::Context* Enter(QEnterEvent*);
	//virtual SequenceView::Context* Leave(QEnterEvent*);
	virtual SequenceView::Context* PlayingPane_MouseMove(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MousePress(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MouseRelease(QMouseEvent*);

	virtual Context* MeasureArea_MouseMove(QMouseEvent*);
	virtual Context* MeasureArea_MousePress(QMouseEvent*);
	virtual Context* MeasureArea_MouseRelease(QMouseEvent*);

	virtual Context* BpmArea_MouseMove(QMouseEvent*);
	virtual Context* BpmArea_MousePress(QMouseEvent*);
	virtual Context* BpmArea_MouseRelease(QMouseEvent*);
};


class SoundChannelPreviewer;

class SequenceView::PreviewContext
		: public QObject, public SequenceView::Context
{
	Q_OBJECT

private:
	SequenceView::CommandsLocker locker;
	Qt::MouseButton mouseButton;
	SoundChannelPreviewer *previewer;

	~PreviewContext();

signals:
	void stop();

private slots:
	void Progress(int currentTicks);

public:
	PreviewContext(SequenceView::Context *parent, SequenceView *sview, Qt::MouseButton button, int time);

	//virtual SequenceView::Context* Enter(QEnterEvent*);
	//virtual SequenceView::Context* Leave(QEnterEvent*);
	virtual SequenceView::Context* PlayingPane_MouseMove(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MousePress(QMouseEvent*);
	virtual SequenceView::Context* PlayingPane_MouseRelease(QMouseEvent*);
};

#endif // SEQUENCEVIEWCONTEXTS
