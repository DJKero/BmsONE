
#include "SequenceView.h"
#include "SequenceViewContexts.h"
#include "SequenceViewInternal.h"
#include "../util/UIDef.h"
#include "EditConfig.h"

SequenceView::EditModeContext::EditModeContext(SequenceView *sview)
	: Context(sview)
{
}

SequenceView::EditModeContext::~EditModeContext()
{
}

SequenceView::Context *SequenceView::EditModeContext::KeyPress(QKeyEvent *e)
{
	if (e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9){
		// assume Qt::Key_$n$ = Qt::Key_0 + $n$
		int index = e->key() - Qt::Key_0;
		if (index == 0){
			sview->TransferSelectedNotesToLane(0);
		}else{
			auto lanes = sview->skin->GetLanes();
			if (lanes.size() >= index){
				sview->TransferSelectedNotesToLane(lanes[index-1].lane);
			}else{
				qApp->beep();
			}
		}
		return this;
	}
	return SequenceView::Context::KeyPress(e);
}

/*
SequenceView::Context *SequenceView::EditModeContext::Enter(QEnterEvent *)
{
	return this;
}

SequenceView::Context *SequenceView::EditModeContext::Leave(QEnterEvent *)
{
	return this;
}
*/
SequenceView::Context *SequenceView::EditModeContext::PlayingPane_MouseMove(QMouseEvent *event)
{
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
    [[maybe_unused]] int iTimeUpper = time;
	int lane = sview->X2Lane(event->x());
	if (sview->snapToGrid){
		iTime = sview->SnapToLowerFineGrid(time);
		iTimeUpper = sview->SnapToUpperFineGrid(time);
	}
    [[maybe_unused]] int laneX;
	if (lane < 0){
		if (event->x() < sview->sortedLanes[0].left){
			laneX = 0;
		}else{
			laneX = sview->sortedLanes.size() - 1;
		}
	}else{
		laneX = sview->sortedLanes.indexOf(sview->lanes[lane]);
	}
	QList<SoundNoteView *> notes;
	bool conflicts = false;
	NoteConflict conf;
	if (lane >= 0)
		notes = sview->HitTestPlayingPaneMulti(lane, event->y(), EditConfig::SnappedHitTestInEditMode() ? iTime : -1,
											   event->modifiers() & Qt::AltModifier, &conflicts, &conf);
	if (notes.size() > 0){
		sview->playingPane->setCursor(Qt::SizeAllCursor);
		if (conflicts){
			sview->cursor->SetLayeredSoundNotesWithConflict(notes, conf);
		}else{
			sview->cursor->SetExistingSoundNote(notes[0]);
		}
	}else if (lane >= 0){
		sview->playingPane->setCursor(Qt::ArrowCursor);
		sview->cursor->SetTimeWithLane(EditConfig::SnappedHitTestInEditMode() ? iTime : time, lane);
	}else{
		sview->playingPane->setCursor(Qt::ArrowCursor);
		sview->cursor->SetNothing();
	}
	return this;
}

SequenceView::Context *SequenceView::EditModeContext::PlayingPane_MousePress(QMouseEvent *event)
{
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
    [[maybe_unused]] int iTimeUpper = time;
	int lane = sview->X2Lane(event->x());
	if (sview->snapToGrid){
		iTime = sview->SnapToLowerFineGrid(time);
		iTimeUpper = sview->SnapToUpperFineGrid(time);
	}
	int laneX;
	if (lane < 0){
		if (event->x() < sview->sortedLanes[0].left){
			laneX = 0;
		}else{
			laneX = sview->sortedLanes.size() - 1;
		}
	}else{
		laneX = sview->sortedLanes.indexOf(sview->lanes[lane]);
	}
	QList<SoundNoteView *> notes;
	bool conflicts = false;
	NoteConflict conf;
	if (lane >= 0)
		notes = sview->HitTestPlayingPaneMulti(lane, event->y(), EditConfig::SnappedHitTestInEditMode() ? iTime : time,
										  event->modifiers() & Qt::AltModifier, &conflicts, &conf);
	sview->ClearBpmEventsSelection();
	if (event->button() == Qt::RightButton && (event->modifiers() & Qt::AltModifier)){
		sview->ClearNotesSelection();
		return new PreviewContext(this, sview, event->pos(), event->button(), iTime);
	}
	if (notes.size() > 0){
		switch (event->button()){
            case Qt::LeftButton: {
                // select note
                if (event->modifiers() & Qt::ControlModifier){
                    for (auto note : notes)
                        sview->ToggleNoteSelection(note);
                }else{
                    QSet<SoundNoteView *> noteSet(notes.begin(), notes.end());
                    if (sview->selectedNotes.intersects(noteSet)){
                        // don't deselect other notes
                    }else{
                        sview->ClearNotesSelection();
                    }
                    for (auto note : notes)
                        sview->SelectAdditionalNote(note);
                }
                if (conflicts){
                    sview->cursor->SetLayeredSoundNotesWithConflict(notes, conf);
                }else{
                    sview->cursor->SetExistingSoundNote(notes[0]);
                }
                if ((event->modifiers() & Qt::ControlModifier) == 0){
                    sview->ClearChannelSelection();
                }
                for (auto note : notes){
                    sview->SetCurrentChannel(note->GetChannelView(), true);
                }
                sview->PreviewMultiNote(notes);

                //if (event->modifiers() & Qt::ShiftModifier){
                //	// edit long notes
                //}else{
                {
                    QMap<SoundChannel*, QSet<int>> noteLocations;
                    for (SoundNoteView *nv : sview->selectedNotes){
                        auto *channel = nv->GetChannelView()->GetChannel();
                        if (!noteLocations.contains(channel)){
                            noteLocations.insert(channel, QSet<int>());
                        }
                        noteLocations[channel].insert(nv->GetNote().location);
                    }
                    return new EditModeDragNotesContext(
                                this, sview, sview->document->BeginModalEditSoundNotes(noteLocations),
                                laneX, iTime,
                                (event->modifiers() & Qt::ShiftModifier) != 0);
                }
                //}
            }
            case Qt::RightButton: {
                // select note & cxt menu
                if (event->modifiers() & Qt::ControlModifier){
                    for (auto note : notes)
                        sview->SelectAdditionalNote(note);
                }else{
                    // check wheter some of `notes` are not selected yet
                    auto temp = notes;
                    for (auto sn : sview->selectedNotes)
                        temp.removeAll(sn);
                    if (!temp.empty()){
                        sview->ClearNotesSelection();
                        for (auto note : notes){
                            sview->SelectAdditionalNote(note);
                        }
                        sview->PreviewMultiNote(notes);
                    }
                }
                if (conflicts){
                    sview->cursor->SetLayeredSoundNotesWithConflict(notes, conf);
                }else{
                    sview->cursor->SetExistingSoundNote(notes[0]);
                }
                if ((event->modifiers() & Qt::ControlModifier) == 0){
                    sview->ClearChannelSelection();
                }
                for (auto note : notes){
                    sview->SetCurrentChannel(note->GetChannelView(), true);
                }
                QMetaObject::invokeMethod(sview, "ShowPlayingPaneContextMenu", Qt::QueuedConnection, Q_ARG(QPoint, event->globalPos()));
                break;
            }
            case Qt::MiddleButton:
                // preview only one channel
                sview->ClearNotesSelection();
                sview->SetCurrentChannel(notes[0]->GetChannelView());
                return new PreviewContext(this, sview, event->pos(), event->button(), iTime);
            default:
                break;
		}
	}else if (lane >= 0){
		// any button
		if (event->modifiers() & Qt::ControlModifier){
			// don't deselect notes (to add new selections)
		}else{
			// clear (to make new selections)
			sview->ClearNotesSelection();
		}
		return new EditModeSelectNotesContext(this, sview, event->button(), laneX,
											  EditConfig::SnappedSelectionInEditMode() ? iTime : time, event->pos());
	}
	return this;
}

SequenceView::Context *SequenceView::EditModeContext::PlayingPane_MouseRelease([[maybe_unused]] QMouseEvent *event)
{
	return this;
}

SequenceView::Context *SequenceView::EditModeContext::PlayingPane_MouseDblClick([[maybe_unused]] QMouseEvent *event)
{
	if (!sview->selectedNotes.empty()){
		sview->TransferSelectedNotesToBgm();
	}
	return this;
}

SequenceView::Context *SequenceView::EditModeContext::BpmArea_MouseMove(QMouseEvent *event){
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
	if (sview->snapToGrid){
		iTime = sview->SnapToLowerFineGrid(iTime);
	}
	const auto events = sview->document->GetBpmEvents();
	int hitTime = time;
	if ((event->modifiers() & Qt::AltModifier) == 0){
		// Alt to bypass absorption
		auto i = events.upperBound(hitTime);
		if (i != events.begin()){
			i--;
			if (i != events.end() && sview->Time2Y(i.key()) - 16 <= event->y()){
				hitTime = i.key();
			}
		}
	}
	if (events.contains(hitTime)){
		sview->timeLine->setCursor(Qt::ArrowCursor);
		sview->cursor->SetExistingBpmEvent(events[hitTime]);
	}else{
		sview->timeLine->setCursor(Qt::ArrowCursor);
		sview->cursor->SetTime(EditConfig::SnappedHitTestInEditMode() ? iTime : time);
	}
	return this;
}

SequenceView::Context *SequenceView::EditModeContext::BpmArea_MousePress(QMouseEvent *event){
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
	if (sview->snapToGrid){
		iTime = sview->SnapToLowerFineGrid(iTime);
	}
	const auto events = sview->document->GetBpmEvents();
	int hitTime = iTime;
	if ((event->modifiers() & Qt::AltModifier) == 0){
		// Alt to bypass absorption
		auto i = events.upperBound(iTime);
		if (i != events.begin()){
			i--;
			if (i != events.end() && sview->Time2Y(i.key()) - 16 <= event->y()){
				hitTime = i.key();
			}
		}
	}
	sview->ClearNotesSelection();
	if (events.contains(hitTime)){
		switch (event->button()){
		case Qt::LeftButton:
			// edit one
			if (event->modifiers() & Qt::ControlModifier){
				sview->ToggleBpmEventSelection(events[hitTime]);
			}else{
				sview->SelectSingleBpmEvent(events[hitTime]);
			}
			break;
		case Qt::RightButton: {
			// select & context menu?
			if (event->modifiers() & Qt::ControlModifier){
				sview->ToggleBpmEventSelection(events[hitTime]);
			}else{
				sview->SelectSingleBpmEvent(events[hitTime]);
			}
			break;
		}
		default:
			break;
		}
	}else{
		// any button
		if (event->modifiers() & Qt::ControlModifier){
			// don't deselect events
		}else{
			// clear (to make new selections)
			sview->ClearBpmEventsSelection();
		}
		return new EditModeSelectBpmEventsContext(this, sview, event->button(),
												  EditConfig::SnappedSelectionInEditMode() ? iTime : time, event->globalPos());
	}
	return this;
}

SequenceView::Context *SequenceView::EditModeContext::BpmArea_MouseRelease([[maybe_unused]] QMouseEvent *event){
	return this;
}





SequenceView::EditModeSelectNotesContext::EditModeSelectNotesContext(SequenceView::EditModeContext *parent,
		SequenceView *sview, Qt::MouseButton button,
		int laneX, int iTime, QPoint pos
		)
	: Context(sview, parent), locker(sview)
	, mouseButton(button)
	, dragBegan(false)
	, dragOrigin(pos)
{
	rubberBand = new QRubberBand(QRubberBand::Rectangle, sview->playingPane);
	rubberBandOriginLaneX = laneX;
	rubberBandOriginTime = iTime;
	rubberBand->setGeometry(pos.x(), pos.y(), 0 ,0);
	rubberBand->show();
}

SequenceView::EditModeSelectNotesContext::~EditModeSelectNotesContext()
{
	delete rubberBand;
}

/*
SequenceView::Context *SequenceView::EditModeSelectNotesContext::Enter(QEnterEvent *)
{
	return this;
}

SequenceView::Context *SequenceView::EditModeSelectNotesContext::Leave(QEnterEvent *)
{
	return this;
}
*/
SequenceView::Context *SequenceView::EditModeSelectNotesContext::PlayingPane_MouseMove(QMouseEvent *event)
{
	if (!dragBegan){
		if (UIUtil::DragBegins(dragOrigin, event->pos())){
			dragBegan = true;
		}else{
			return this;
		}
	}
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
	int iTimeUpper = time;
	int lane = sview->X2Lane(event->x());
	if (sview->snapToGrid && EditConfig::SnappedSelectionInEditMode()){
		iTime = sview->SnapToLowerFineGrid(time);
		iTimeUpper = sview->SnapToUpperFineGrid(time);
	}
	int laneX;
	if (lane < 0){
		if (event->x() < sview->sortedLanes[0].left){
			laneX = 0;
		}else{
			laneX = sview->sortedLanes.size() - 1;
		}
	}else{
		laneX = sview->sortedLanes.indexOf(sview->lanes[lane]);
	}
	int cursorTime = iTime;
	int timeBegin, timeEnd;
	int laneXBegin, laneXLast;
	if (laneX > rubberBandOriginLaneX){
		laneXBegin = rubberBandOriginLaneX;
		laneXLast = laneX;
	}else{
		laneXBegin = laneX;
		laneXLast = rubberBandOriginLaneX;
	}
	if (iTime >= rubberBandOriginTime){
		timeBegin = rubberBandOriginTime;
		timeEnd = iTimeUpper;
		cursorTime = iTimeUpper;
	}else{
		timeBegin = iTime;
		timeEnd = rubberBandOriginTime;
	}
	QRect rectRb;
	QList<int> laneList;
	rectRb.setBottom(sview->Time2Y(timeBegin) - 2);
	rectRb.setTop(sview->Time2Y(timeEnd) - 1);
	rectRb.setLeft(sview->sortedLanes[laneXBegin].left);
	rectRb.setRight(sview->sortedLanes[laneXLast].left + sview->sortedLanes[laneXLast].width - 1);
	for (int ix=laneXBegin; ix<=laneXLast; ix++){
		laneList.append(sview->sortedLanes[ix].lane);
	}
	rubberBand->setGeometry(rectRb);
	sview->playingPane->setCursor(Qt::ArrowCursor);
	sview->cursor->SetKeyNotesSelection(cursorTime, timeBegin, timeEnd, laneList, -1);
	return this;
}

SequenceView::Context *SequenceView::EditModeSelectNotesContext::PlayingPane_MousePress([[maybe_unused]] QMouseEvent *event)
{
	return this;
}

SequenceView::Context *SequenceView::EditModeSelectNotesContext::PlayingPane_MouseRelease(QMouseEvent *event)
{
	if (event->button() != mouseButton){
		// ignore
		return this;
	}
	if (!dragBegan){
		// release before drag began
		return Escape();
	}
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
	int iTimeUpper = time;
	int lane = sview->X2Lane(event->x());
	if (sview->snapToGrid && EditConfig::SnappedSelectionInEditMode()){
		iTime = sview->SnapToLowerFineGrid(time);
		iTimeUpper = sview->SnapToUpperFineGrid(time);
	}
	int laneX;
	if (lane < 0){
		if (event->x() < sview->sortedLanes[0].left){
			laneX = 0;
		}else{
			laneX = sview->sortedLanes.size() - 1;
		}
	}else{
		laneX = sview->sortedLanes.indexOf(sview->lanes[lane]);
	}
    [[maybe_unused]] int cursorTime = iTime;
	int timeBegin, timeEnd;
    [[maybe_unused]] int laneXBegin, laneXLast;
	if (laneX > rubberBandOriginLaneX){
		laneXBegin = rubberBandOriginLaneX;
		laneXLast = laneX;
	}else{
		laneXBegin = laneX;
		laneXLast = rubberBandOriginLaneX;
	}
	if (iTime >= rubberBandOriginTime){
		timeBegin = rubberBandOriginTime;
		timeEnd = iTimeUpper;
		cursorTime = iTimeUpper;
	}else{
		timeBegin = iTime;
		timeEnd = rubberBandOriginTime;
	}
	QList<int> lns;
	if (laneX > rubberBandOriginLaneX){
		for (int ix=rubberBandOriginLaneX; ix<=laneX; ix++){
			lns.append(sview->sortedLanes[ix].lane);
		}
	}else{
		for (int ix=laneX; ix<=rubberBandOriginLaneX; ix++){
			lns.append(sview->sortedLanes[ix].lane);
		}
	}
	if (event->modifiers() & Qt::AltModifier){
		if (sview->currentChannel >= 0){
			auto *cview = sview->soundChannels[sview->currentChannel];
			for (SoundNoteView *nv : cview->GetNotes()){
				const SoundNote &note = nv->GetNote();
				if (lns.contains(note.lane)){
					if (note.location+note.length >= timeBegin && note.location < timeEnd){
						if (event->modifiers() & Qt::ShiftModifier){
							sview->ToggleNoteSelection(nv);
						}else{
							sview->SelectAdditionalNote(nv);
						}
					}
				}
			}
		}
	}else{
		for (SoundChannelView *cview : sview->soundChannels){
			for (SoundNoteView *nv : cview->GetNotes()){
				const SoundNote &note = nv->GetNote();
				if (lns.contains(note.lane)){
					if (note.location+note.length >= timeBegin && note.location < timeEnd){
						if (event->modifiers() & Qt::ShiftModifier){
							sview->ToggleNoteSelection(nv);
						}else{
							sview->SelectAdditionalNote(nv);
						}
					}
				}
			}
		}
	}
	rubberBand->hide();
	if (mouseButton == Qt::RightButton){
		QMetaObject::invokeMethod(sview, "ShowPlayingPaneContextMenu", Qt::QueuedConnection, Q_ARG(QPoint, event->globalPos()));
	}
	return Escape();
}




SequenceView::EditModeDragNotesContext::EditModeDragNotesContext(SequenceView::EditModeContext *parent,
		SequenceView *sview,
		Document::DocumentUpdateSoundNotesContext *cxt, int laneX, int iTime
		, bool editLN)
	: Context(sview, parent), locker(sview)
	, updateNotesCxt(cxt), editLN(editLN)
{
	dragNotesOriginLaneX = dragNotesPreviousLaneX = laneX;
	dragNotesOriginTime  = dragNotesPreviousTime  = iTime;
}

SequenceView::EditModeDragNotesContext::~EditModeDragNotesContext()
{
	if (updateNotesCxt){
		updateNotesCxt->Cancel();
		delete updateNotesCxt;
	}
}

/*
SequenceView::Context *SequenceView::EditModeDragNotesContext::Enter(QEnterEvent *)
{
	return this;
}

SequenceView::Context *SequenceView::EditModeDragNotesContext::Leave(QEnterEvent *)
{
	return this;
}
*/
SequenceView::Context *SequenceView::EditModeDragNotesContext::PlayingPane_MouseMove(QMouseEvent *event)
{
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
    [[maybe_unused]] int iTimeUpper = time;
	int lane = sview->X2Lane(event->x());
	if (sview->snapToGrid){
		iTime = sview->SnapToLowerFineGrid(time);
		iTimeUpper = sview->SnapToUpperFineGrid(time);
	}
	int laneX;
	if (lane < 0){
		if (event->x() < sview->sortedLanes[0].left){
			laneX = 0;
		}else{
			laneX = sview->sortedLanes.size() - 1;
		}
	}else{
		laneX = sview->sortedLanes.indexOf(sview->lanes[lane]);
	}
	if (editLN){
		if (iTime != dragNotesPreviousTime){
			QMap<SoundChannel*, QMap<int, SoundNote>> notes;
			QMap<SoundChannel*, QMap<int, SoundNote>> originalNotes = updateNotesCxt->GetOldNotes();
			for (SoundNoteView *nv : sview->selectedNotes){
				auto *channel = nv->GetChannelView()->GetChannel();
				if (!notes.contains(channel)){
					notes.insert(channel, QMap<int, SoundNote>());
				}
				SoundNote note = originalNotes[channel][nv->GetNote().location];
				note.length = std::max(0, note.length + iTime - dragNotesOriginTime);
				notes[channel].insert(note.location, note);
			}
			updateNotesCxt->Update(notes);
		}
	}else{
		if (laneX != dragNotesPreviousLaneX){
			bool passBoundaryTest = true;
			QMap<SoundChannel*, QMap<int, SoundNote>> notes;
			QMap<SoundChannel*, QMap<int, SoundNote>> originalNotes = updateNotesCxt->GetOldNotes();
			for (SoundNoteView *nv : sview->selectedNotes){
				auto *channel = nv->GetChannelView()->GetChannel();
				if (!notes.contains(channel)){
					notes.insert(channel, QMap<int, SoundNote>());
				}
				SoundNote note = originalNotes[channel][nv->GetNote().location];
				int lX = sview->sortedLanes.indexOf(sview->lanes[note.lane]) + laneX - dragNotesOriginLaneX;
				if (lX < 0 || lX >= sview->sortedLanes.size()){
					passBoundaryTest = false;
					break;
				}
				note.lane = sview->sortedLanes[lX].lane;
				notes[channel].insert(note.location, note);
			}
			if (passBoundaryTest){
				updateNotesCxt->Update(notes);
			}
		}
	}
	dragNotesPreviousTime = iTime;
	dragNotesPreviousLaneX = laneX;
	return this;
}

SequenceView::Context *SequenceView::EditModeDragNotesContext::PlayingPane_MousePress([[maybe_unused]] QMouseEvent *event)
{
	return this;
}

SequenceView::Context *SequenceView::EditModeDragNotesContext::PlayingPane_MouseRelease(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton){
		// ignore
		return this;
	}
	updateNotesCxt->Finish();
	delete updateNotesCxt;
	updateNotesCxt = nullptr;
	return Escape();
}



SequenceView::EditModeSelectBpmEventsContext::EditModeSelectBpmEventsContext(
		SequenceView::EditModeContext *parent, SequenceView *sview,
		Qt::MouseButton button, int iTime, QPoint pos
		)
	: Context(sview, parent), locker(sview)
	, mouseButton(button)
{
	rubberBand = new QRubberBand(QRubberBand::Rectangle, sview->timeLine);
	rubberBandOriginTime = iTime;
	rubberBand->setGeometry(pos.x(), pos.y(), 0 ,0);
	rubberBand->show();
}

SequenceView::EditModeSelectBpmEventsContext::~EditModeSelectBpmEventsContext()
{
	delete rubberBand;
}

SequenceView::Context *SequenceView::EditModeSelectBpmEventsContext::MeasureArea_MouseMove(QMouseEvent *event)
{
	return BpmArea_MouseMove(event);
}

SequenceView::Context *SequenceView::EditModeSelectBpmEventsContext::MeasureArea_MousePress(QMouseEvent *event)
{
	return BpmArea_MousePress(event);
}

SequenceView::Context *SequenceView::EditModeSelectBpmEventsContext::MeasureArea_MouseRelease(QMouseEvent *event)
{
	return BpmArea_MouseRelease(event);
}

SequenceView::Context *SequenceView::EditModeSelectBpmEventsContext::BpmArea_MouseMove(QMouseEvent *event)
{
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
	int iTimeUpper = time;
	if (sview->snapToGrid && EditConfig::SnappedSelectionInEditMode()){
		iTime = sview->SnapToLowerFineGrid(time);
		iTimeUpper = sview->SnapToUpperFineGrid(time);
	}
	int cursorTime = iTime;
	int timeBegin, timeEnd;
	if (iTime >= rubberBandOriginTime){
		timeBegin = rubberBandOriginTime;
		timeEnd = iTimeUpper;
		cursorTime = iTimeUpper;
	}else{
		timeBegin = iTime;
		timeEnd = rubberBandOriginTime;
	}
	QRect rectRb;
	rectRb.setBottom(sview->Time2Y(timeBegin) - 2);
	rectRb.setTop(sview->Time2Y(timeEnd) - 1);
	rectRb.setLeft(sview->timeLineMeasureWidth);
	rectRb.setRight(sview->timeLine->width() - 1);
	rubberBand->setGeometry(rectRb);
	sview->playingPane->setCursor(Qt::ArrowCursor);
	sview->cursor->SetBpmEventsSelection(cursorTime, timeBegin, timeEnd, -1);
	return this;
}

SequenceView::Context *SequenceView::EditModeSelectBpmEventsContext::BpmArea_MousePress(QMouseEvent *)
{
	return this;
}

SequenceView::Context *SequenceView::EditModeSelectBpmEventsContext::BpmArea_MouseRelease(QMouseEvent *event)
{
	if (event->button() != mouseButton){
		// ignore
		return this;
	}
	qreal time = sview->Y2Time(event->y());
	int iTime = time;
	int iTimeUpper = time;
	if (sview->snapToGrid){
		iTime = sview->SnapToLowerFineGrid(time);
		iTimeUpper = sview->SnapToUpperFineGrid(time);
	}
    [[maybe_unused]] int cursorTime = iTime;
	int timeBegin, timeEnd;
	if (iTime >= rubberBandOriginTime){
		timeBegin = rubberBandOriginTime;
		timeEnd = iTimeUpper;
		cursorTime = iTimeUpper;
	}else{
		timeBegin = iTime;
		timeEnd = rubberBandOriginTime;
	}
	const auto events = sview->document->GetBpmEvents();
	for (auto bpm : events){
		if (bpm.location >= timeBegin && bpm.location < timeEnd){
			if (event->modifiers() & Qt::ShiftModifier){
				sview->ToggleBpmEventSelection(bpm);
			}else{
				sview->SelectAdditionalBpmEvent(bpm);
			}
		}
	}
	rubberBand->hide();
	// maybe shows context menu?
	return Escape();
}

