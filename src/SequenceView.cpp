#include "SequenceView.h"
#include "MainWindow.h"
#include <cmath>

QWidget *SequenceView::NewWidget(
		bool(SequenceView::*paintEventHandler)(QWidget *, QPaintEvent *),
		bool(SequenceView::*mouseEventHandler)(QWidget *, QMouseEvent *),
		bool(SequenceView::*enterEventHandler)(QWidget *, QEvent *))
{
	QWidget *widget = new QWidget(this);
	widget->installEventFilter(this);
	if (paintEventHandler)
		paintEventDispatchTable.insert(widget, paintEventHandler);
	if (mouseEventHandler)
		mouseEventDispatchTable.insert(widget, mouseEventHandler);
	if (enterEventHandler)
		enterEventDispatchTable.insert(widget, enterEventHandler);
	return widget;
}

SequenceView::SequenceView(MainWindow *parent)
	: QAbstractScrollArea(parent)
	, mainWindow(parent)
	, document(nullptr)
	, documentReady(false)
{
	{
		const int lmargin = 5;
		const int wscratch = 36;
		const int wwhite = 24;
		const int wblack = 24;
		QColor cscratch(60, 26, 26);
		QColor cwhite(51, 51, 51);
		QColor cblack(26, 26, 60);
		QColor cbigv(180, 180, 180);
		QColor csmallv(90, 90, 90);
		QColor ncwhite(210, 210, 210);
		QColor ncblack(150, 150, 240);
		QColor ncscratch(240, 150, 150);
		lanes.insert(8, LaneDef(8, lmargin, wscratch, cscratch, ncscratch, cbigv));
		lanes.insert(1, LaneDef(1, lmargin+wscratch+wwhite*0+wblack*0, wwhite, cwhite, ncwhite, csmallv));
		lanes.insert(2, LaneDef(2, lmargin+wscratch+wwhite*1+wblack*0, wblack, cblack, ncblack, csmallv));
		lanes.insert(3, LaneDef(3, lmargin+wscratch+wwhite*1+wblack*1, wwhite, cwhite, ncwhite, csmallv));
		lanes.insert(4, LaneDef(4, lmargin+wscratch+wwhite*2+wblack*1, wblack, cblack, ncblack, csmallv));
		lanes.insert(5, LaneDef(5, lmargin+wscratch+wwhite*2+wblack*2, wwhite, cwhite, ncwhite, csmallv));
		lanes.insert(6, LaneDef(6, lmargin+wscratch+wwhite*3+wblack*2, wblack, cblack, ncblack, csmallv));
		lanes.insert(7, LaneDef(7, lmargin+wscratch+wwhite*3+wblack*3, wwhite, cwhite, ncwhite, csmallv, cbigv));

		penBigV = QPen(QBrush(QColor(180, 180, 180)), 1);
		penBigV.setCosmetic(true);
		penV = QPen(QBrush(QColor(90, 90, 90)), 1);
		penV.setCosmetic(true);
		penBar = QPen(QBrush(QColor(180, 180, 180)), 1);
		penBar.setCosmetic(true);
		penBeat = QPen(QBrush(QColor(135, 135, 135)), 1);
		penBeat.setCosmetic(true);
		penStep = QPen(QBrush(QColor(90, 90, 90)), 1);
		penStep.setCosmetic(true);

		playingWidth = lmargin+wscratch+wwhite*4+wblack*3 + 5;
	}

	setAttribute(Qt::WA_OpaquePaintEvent);
	setAcceptDrops(true);
	setFrameShape(QFrame::NoFrame);
	setLineWidth(0);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setCornerWidget(new QSizeGrip(this));
	setViewport(nullptr);	// creates new viewport widget
	setViewportMargins(timeLineWidth + playingWidth, headerHeight, 0, footerHeight);
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	viewport()->setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(QPalette::Background, QColor(135, 135, 135));
	viewport()->setPalette(pal);
	timeLine = NewWidget(&SequenceView::paintEventTimeLine);
	playingPane = NewWidget(&SequenceView::paintEventPlayingPane, &SequenceView::mouseEventPlayingPane, &SequenceView::enterEventPlayingPane);
	headerChannelsArea = NewWidget(&SequenceView::paintEventHeaderArea);
	headerCornerEntry = NewWidget(&SequenceView::paintEventHeaderEntity);
	headerPlayingEntry = NewWidget(&SequenceView::paintEventHeaderEntity);
	footerChannelsArea = NewWidget(&SequenceView::paintEventFooterArea);
	footerCornerEntry = NewWidget(&SequenceView::paintEventFooterEntity);
	footerPlayingEntry = NewWidget(&SequenceView::paintEventFooterEntity);
	playingPane->setMouseTracking(true);

	auto *tb = new QToolBar(headerPlayingEntry);
	tb->addAction("L");
	tb->addAction("R");
	tb->addSeparator();
	tb->addAction("S");
	tb->addAction("M");

	editMode = EditMode::EDIT_MODE;
	lockCreation = false;
	lockDeletion = false;
	lockVerticalMove = false;

	coarseGrid = GridSize(4);
	fineGrid = GridSize(16);
	snapToGrid = true;

	resolution = 240;
	viewLength = 240*4*32;

	zoomY = 48./240.;
	zoomXKey = 1.;
	zoomXBgm = 1.;

	currentLocation = 0;
	currentChannel = 0;
	playing = false;
	cursorExistingNote = nullptr;
	showCursorNewNote = false;
}

SequenceView::~SequenceView()
{	
}

void SequenceView::ReplaceDocument(Document *newDocument)
{
	documentReady = false;
	// unload document
	{
		selectedNotes.clear();
		for (auto *header : soundChannelHeaders){
			delete header;
		}
		soundChannelHeaders.clear();
		for (auto *footer : soundChannelFooters){
			delete footer;
		}
		soundChannelFooters.clear();
		for (auto *channelView : soundChannels){
			delete channelView;
		}
		soundChannels.clear();
	}
	document = newDocument;
	// load document
	{
		// follow current state of document
		int ichannel = 0;
		for (auto *channel : document->GetSoundChannels()){
			auto cview = new SoundChannelView(this, channel);
			cview->setParent(viewport());
			cview->setVisible(true);
			soundChannels.push_back(cview);
			auto *header = new SoundChannelHeader(this, cview);
			header->setParent(headerChannelsArea);
			header->setVisible(true);
			soundChannelHeaders.push_back(header);
			auto *footer = new SoundChannelFooter(this, cview);
			footer->setParent(footerChannelsArea);
			footer->setVisible(true);
			soundChannelFooters.push_back(footer);
			ichannel++;
		}
		connect(document, &Document::SoundChannelInserted, this, &SequenceView::SoundChannelInserted);
		connect(document, &Document::SoundChannelRemoved, this, &SequenceView::SoundChannelRemoved);
		connect(document, &Document::SoundChannelMoved, this, &SequenceView::SoundChannelMoved);
		connect(document, &Document::TotalLengthChanged, this, &SequenceView::TotalLengthChanged);

		resolution = document->GetTimeBase();
		viewLength = ViewLength(document->GetTotalLength());
		currentLocation = 0;
		currentChannel = -1;
		playing = false;
		cursorExistingNote = nullptr;
		showCursorNewNote = false;
		OnViewportResize();
		UpdateVerticalScrollBar(0.0); // daburi
	}
	documentReady = true;
}


void SequenceView::mouseMoveEventVp(QMouseEvent *event)
{
	QWidget::mouseMoveEvent(event);
}

void SequenceView::dragEnterEventVp(QDragEnterEvent *event)
{
	mainWindow->dragEnterEvent(event);
}

void SequenceView::dragMoveEventVp(QDragMoveEvent *event)
{
	mainWindow->dragMoveEvent(event);
}

void SequenceView::dragLeaveEventVp(QDragLeaveEvent *event)
{
	mainWindow->dragLeaveEvent(event);
}

void SequenceView::dropEventVp(QDropEvent *event)
{
	mainWindow->dropEvent(event);
}

qreal SequenceView::Time2Y(qreal time) const
{
	return (viewLength - time)*zoomY - verticalScrollBar()->value();
}

qreal SequenceView::Y2Time(qreal y) const
{
	return viewLength - (y + verticalScrollBar()->value()) / zoomY;
}

qreal SequenceView::TimeSpan2DY(qreal time) const
{
	return time*zoomY;
}

int SequenceView::X2Lane(int x) const
{
	for (QMap<int, LaneDef>::const_iterator i=lanes.begin(); i!=lanes.end(); i++){
		if (i->left <= x && i->left+i->width > x){
			return i.key();
		}
	}
	return -1;
}

QSet<int> SequenceView::FineGridsInRange(qreal tBegin, qreal tEnd)
{
	const QMap<int, BarLine> &bars = document->GetBarLines();
	const QMap<int, BarLine>::const_iterator ibar = bars.lowerBound(tBegin);
	QSet<int> fineGridLines;
	int prevBarLoc = ibar==bars.begin() ? -1 : (ibar-1)->Location;
	for (QMap<int, BarLine>::const_iterator b=ibar; ; prevBarLoc=b->Location, b++){
		if (prevBarLoc < 0){
			continue;
		}
		for (int i=0; ; i++){
			int t = prevBarLoc + fineGrid.NthGrid(resolution, i);
			if (t > tEnd)
				return fineGridLines;
			if (b!=bars.end() && t >= b->Location)
				break;
			if (t >= tBegin){
				fineGridLines.insert(t);
			}
		}
		if (b==bars.end())
			break;
	}
	return fineGridLines;
}

QSet<int> SequenceView::CoarseGridsInRange(qreal tBegin, qreal tEnd)
{
	const QMap<int, BarLine> &bars = document->GetBarLines();
	const QMap<int, BarLine>::const_iterator ibar = bars.lowerBound(tBegin);
	QSet<int> coarseGridLines;
	int prevBarLoc = ibar==bars.begin() ? -1 : (ibar-1)->Location;
	for (QMap<int, BarLine>::const_iterator b=ibar; ; prevBarLoc=b->Location, b++){
		if (prevBarLoc < 0){
			continue;
		}
		for (int i=0; ; i++){
			int t = prevBarLoc + coarseGrid.NthGrid(resolution, i);
			if (t > tEnd)
				return coarseGridLines;
			if (b!=bars.end() && t >= b->Location)
				break;
			if (t >= tBegin){
				coarseGridLines.insert(t);
			}
		}
		if (b==bars.end())
			break;
	}
	return coarseGridLines;
}

QSet<int> SequenceView::BarsInRange(qreal tBegin, qreal tEnd)
{
	const QMap<int, BarLine> &bars = document->GetBarLines();
	const QMap<int, BarLine>::const_iterator ibar = bars.lowerBound(tBegin);
	QSet<int> barLines;
	for (QMap<int, BarLine>::const_iterator i=ibar; i!=bars.end() && i->Location<=(int)tEnd; i++){
		barLines.insert(i->Location);
	}
	return barLines;
}

qreal SequenceView::SnapToFineGrid(qreal time) const
{
	const QMap<int, BarLine> &bars = document->GetBarLines();
	const QMap<int, BarLine>::const_iterator ibar = bars.lowerBound(time);
	if (ibar == bars.begin()){
		return time;
	}
	int prevBarLoc = (ibar-1)->Location;
	return prevBarLoc + fineGrid.NthGrid(resolution, fineGrid.GridNumber(resolution, time - prevBarLoc));
}

void SequenceView::SetNoteColor(QLinearGradient &g, int lane, bool active) const
{
	QColor c = lane==0 ? QColor(210, 210, 210) : lanes[lane].noteColor;
	QColor cl = active ? c : QColor(c.red()/2, c.green()/2, c.blue()/2);
	QColor cd(cl.red()*2/3, cl.green()*2/3, cl.blue()*2/3);
	g.setColorAt(0, cd);
	g.setColorAt(0.3, cl);
	g.setColorAt(0.7, cl);
	g.setColorAt(1, cd);
}

void SequenceView::VisibleRangeChanged() const
{
	if (!document)
		return;
	static const int my = 4;
	int scrollY = verticalScrollBar()->value();
	int top = 0 - my;
	int bottom = viewport()->height() + my;
	qreal tBegin = viewLength - (scrollY + bottom)/zoomY;
	qreal tEnd = viewLength - (scrollY + top)/zoomY;

	// sequence view instance is currently single
	QList<QPair<int, int>> regions;
	regions.append(QPair<int, int>(tBegin, tEnd));
	for (SoundChannel *channel : document->GetSoundChannels()){
		channel->UpdateVisibleRegions(regions);
	}
}

int SequenceView::ViewLength(int totalLength) const
{
	int totalBars = totalLength / (resolution*4) + 1;
	int vl = (totalBars + 32) * (resolution*4);
	return vl;
}

void SequenceView::wheelEventVp(QWheelEvent *event)
{
	qreal tOld = viewLength - (verticalScrollBar()->value() + viewport()->height())/zoomY;
	QPoint numPixels = event->pixelDelta();
	QPoint numDegrees = event->angleDelta() / 8;
	if (event->modifiers() & Qt::ControlModifier){
		int zoomY_B = zoomY;
		if (!numPixels.isNull()){
			zoomY *= std::pow(1.01, numPixels.y());
		}else if (!numDegrees.isNull()){
			QPoint numSteps = numDegrees / 15;
			if (numSteps.y() > 0){
				zoomY *= 1.25;
			}else if (numSteps.y() < 0){
				zoomY /= 1.25;
			}
		}
		zoomY = std::max(0.5*48./resolution, std::min(8.*48./resolution, zoomY));
		UpdateVerticalScrollBar(tOld);
		{
			timeLine->update();
			playingPane->update();
			headerChannelsArea->update();
			footerChannelsArea->update();
			for (SoundChannelView *cview : soundChannels){
				cview->update();
			}
			VisibleRangeChanged();
		}
		event->accept();
	}else{
		if (!numPixels.isNull()){
			horizontalScrollBar()->setValue(horizontalScrollBar()->value() - numPixels.x());
			verticalScrollBar()->setValue(verticalScrollBar()->value() - numPixels.y());
		}else if (!numDegrees.isNull()){
			QPoint numSteps = numDegrees / 15;
			verticalScrollBar()->setValue(verticalScrollBar()->value() - numSteps.y() * verticalScrollBar()->singleStep());
		}
		event->accept();
	}
}

bool SequenceView::viewportEvent(QEvent *event)
{
	switch (event->type()){
	case QEvent::Paint:
		return false;
	case QEvent::MouseMove:
		mouseMoveEventVp(dynamic_cast<QMouseEvent*>(event));
		return true;
	case QEvent::DragEnter:
		dragEnterEventVp(dynamic_cast<QDragEnterEvent*>(event));
		return true;
	case QEvent::DragLeave:
		dragLeaveEventVp(dynamic_cast<QDragLeaveEvent*>(event));
		return true;
	case QEvent::DragMove:
		dragMoveEventVp(dynamic_cast<QDragMoveEvent*>(event));
		return true;
	case QEvent::Drop:
		dropEventVp(dynamic_cast<QDropEvent*>(event));
		return true;
	case QEvent::Resize:
		OnViewportResize();
		return true;
	case QEvent::Wheel:
		wheelEventVp(dynamic_cast<QWheelEvent*>(event));
		return true;
	}
	return false;
}

void SequenceView::scrollContentsBy(int dx, int dy)
{
	if (dy){
		for (SoundChannelView *cview : soundChannels){
			cview->ScrollContents(dy);
		}
		timeLine->scroll(0, dy);
		playingPane->scroll(0, dy);
	}
	if (dx){
		viewport()->scroll(dx, 0);
		headerChannelsArea->scroll(dx, 0);
		footerChannelsArea->scroll(dx, 0);
	}
	VisibleRangeChanged();
}

void SequenceView::UpdateVerticalScrollBar(qreal newTimeBegin)
{
	verticalScrollBar()->setRange(0, std::max(0, int(viewLength*zoomY) - viewport()->height()));
	verticalScrollBar()->setPageStep(viewport()->height());
	verticalScrollBar()->setSingleStep(48); // not affected by zoomY!
	if (newTimeBegin >= 0.0){
		verticalScrollBar()->setValue((viewLength - newTimeBegin)*zoomY - viewport()->height());
	}
}

void SequenceView::OnViewportResize()
{
	QRect vr = viewport()->geometry();
	timeLine->setGeometry(0, headerHeight, timeLineWidth, vr.height());
	playingPane->setGeometry(timeLineWidth, headerHeight, playingWidth, vr.height());
	headerChannelsArea->setGeometry(timeLineWidth + playingWidth, 0, vr.width(), headerHeight);
	headerCornerEntry->setGeometry(0, 0, timeLineWidth, headerHeight);
	headerPlayingEntry->setGeometry(timeLineWidth, 0, playingWidth, headerHeight);
	footerChannelsArea->setGeometry(timeLineWidth + playingWidth, vr.bottom()+1, vr.width(), footerHeight);
	footerCornerEntry->setGeometry(0, vr.bottom()+1, timeLineWidth, footerHeight);
	footerPlayingEntry->setGeometry(timeLineWidth, vr.bottom()+1, playingWidth, footerHeight);
	for (int i=0; i<soundChannels.size(); i++){
		int x = i * 64 - horizontalScrollBar()->value();
		soundChannels[i]->setGeometry(x, 0, 64, vr.height());
		soundChannelHeaders[i]->setGeometry(x, 0, 64, headerHeight);
		soundChannelFooters[i]->setGeometry(x, 0, 64, footerHeight);
	}

	UpdateVerticalScrollBar();

	horizontalScrollBar()->setRange(0, std::max(0, 64*soundChannels.size() - viewport()->width()));
	horizontalScrollBar()->setPageStep(viewport()->width());
	horizontalScrollBar()->setSingleStep(64);

	VisibleRangeChanged();
}

void SequenceView::SoundChannelInserted(int index, SoundChannel *channel)
{
	//soundChannels.insert(index, new SoundChannelView(this, channel));
	//OnViewportResize();
}

void SequenceView::SoundChannelRemoved(int index, SoundChannel *channel)
{
	auto *chv = soundChannels.takeAt(index);
	delete chv;
	OnViewportResize();
}

void SequenceView::SoundChannelMoved(int indexBefore, int indexAfter)
{
	auto *chv = soundChannels.takeAt(indexBefore);
	soundChannels.insert(indexAfter, chv);
	OnViewportResize();
}

void SequenceView::TotalLengthChanged(int totalLength)
{
	int oldVL = viewLength;
	viewLength = ViewLength(totalLength);
	if (oldVL != viewLength){
		if (documentReady){
			qreal t = oldVL - (verticalScrollBar()->value() + viewport()->height())/zoomY;
			UpdateVerticalScrollBar(t);
			update();
		}
	}
}

void SequenceView::OnCurrentChannelChanged(int index)
{
	if (currentChannel != index){
		if (currentChannel >= 0){
			soundChannels[currentChannel]->SetCurrent(false);
		}
		currentChannel = index;
		if (currentChannel >= 0){
			soundChannels[currentChannel]->SetCurrent(true);
		}
		update();
	}
}

bool SequenceView::eventFilter(QObject *sender, QEvent *event)
{
	switch (event->type()){
	case QEvent::Wheel: {
		auto *widget = dynamic_cast<QWidget*>(sender);
		// cheat
		if (paintEventDispatchTable.contains(widget)){
			wheelEventVp(dynamic_cast<QWheelEvent*>(event));
		}
		return false;
	}
	case QEvent::Resize:
		return false;
	case QEvent::Paint: {
		auto *widget = dynamic_cast<QWidget*>(sender);
		if (paintEventDispatchTable.contains(widget)){
			return (this->*paintEventDispatchTable[widget])(widget, dynamic_cast<QPaintEvent*>(event));
		}
		return false;
	}
	case QEvent::MouseMove:
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
	case QEvent::MouseButtonDblClick:
	{
		auto *widget = dynamic_cast<QWidget*>(sender);
		if (mouseEventDispatchTable.contains(widget)){
			return (this->*mouseEventDispatchTable[widget])(widget, dynamic_cast<QMouseEvent*>(event));
		}
		return false;
	}
	case QEvent::Enter:
	case QEvent::Leave:
	{
		auto *widget = dynamic_cast<QWidget*>(sender);
		if (enterEventDispatchTable.contains(widget)){
			return (this->*enterEventDispatchTable[widget])(widget, dynamic_cast<QEvent*>(event));
		}
		return false;
	}
	default:
		return false;
	}
}

bool SequenceView::paintEventTimeLine(QWidget *timeLine, QPaintEvent *event)
{
	static const int mx = 4, my = 4;
	QPainter painter(timeLine);
	QRect rect = event->rect();
	painter.fillRect(rect, QColor(34, 34, 34));

	int scrollY = verticalScrollBar()->value();

	int left = rect.x() - mx;
	int right = rect.right() + mx;
	int top = rect.y() - my;
	int bottom = rect.bottom() + my;
	int height = bottom - top;

	qreal tBegin = viewLength - (scrollY + bottom)/zoomY;
	qreal tEnd = viewLength - (scrollY + top)/zoomY;

	QSet<int> bars = BarsInRange(tBegin, tEnd);
	QSet<int> coarseGrids = CoarseGridsInRange(tBegin, tEnd) - bars;
	QSet<int> fineGrids = FineGridsInRange(tBegin, tEnd) - bars - coarseGrids;
	{
		QVector<QLine> lines;
		for (int t : fineGrids){
			int y = Time2Y(t) - 1;
			lines.append(QLine(left, y, right, y));
		}
		painter.setPen(penStep);
		painter.drawLines(lines);
	}
	{
		QVector<QLine> lines;
		for (int t : coarseGrids){
			int y = Time2Y(t) - 1;
			lines.append(QLine(left, y, right, y));
		}
		painter.setPen(penBeat);
		painter.drawLines(lines);
	}
	{
		QVector<QLine> lines;
		for (int t : bars){
			int y = Time2Y(t) - 1;
			lines.append(QLine(left, y, right, y));
		}
		painter.setPen(penBar);
		painter.drawLines(lines);
	}

	if (showCursorNewNote){
		painter.setPen(QPen(QBrush(QColor(255, 255, 255)), 1));
		int y = Time2Y(cursorNewNote.location) - 1;
		painter.drawLine(left, y, right, y);
	}

	return true;
}

bool SequenceView::paintEventPlayingPane(QWidget *playingPane, QPaintEvent *event)
{
	static const int mx = 4, my = 4;
	QPainter painter(playingPane);
	QRect rect = event->rect();
	painter.fillRect(rect, QColor(255, 255, 255));

	int scrollX = horizontalScrollBar()->value();
	int scrollY = verticalScrollBar()->value();

	int left = rect.x() - mx;
	int right = rect.right() + mx;
	int top = rect.y() - my;
	int bottom = rect.bottom() + my;
	int height = bottom - top;

	qreal tBegin = viewLength - (scrollY + bottom)/zoomY;
	qreal tEnd = viewLength - (scrollY + top)/zoomY;

	// lanes
	for (LaneDef lane : lanes){
		painter.fillRect(lane.left, top, lane.width, height, lane.color);
		painter.setPen(QPen(QBrush(lane.leftLine), 1.0));
		painter.drawLine(lane.left, top, lane.left, bottom);
		painter.setPen(QPen(QBrush(lane.rightLine), 1.0));
		painter.drawLine(lane.left+lane.width, top, lane.left+lane.width, bottom);
	}
	// horizontal lines
	{
		QSet<int> bars = BarsInRange(tBegin, tEnd);
		QSet<int> coarseGrids = CoarseGridsInRange(tBegin, tEnd) - bars;
		QSet<int> fineGrids = FineGridsInRange(tBegin, tEnd) - bars - coarseGrids;
		{
			QVector<QLine> lines;
			for (int t : fineGrids){
				int y = Time2Y(t) - 1;
				lines.append(QLine(left, y, right, y));
			}
			painter.setPen(penStep);
			painter.drawLines(lines);
		}
		{
			QVector<QLine> lines;
			for (int t : coarseGrids){
				int y = Time2Y(t) - 1;
				lines.append(QLine(left, y, right, y));
			}
			painter.setPen(penBeat);
			painter.drawLines(lines);
		}
		{
			QVector<QLine> lines;
			for (int t : bars){
				int y = Time2Y(t) - 1;
				lines.append(QLine(left, y, right, y));
			}
			painter.setPen(penBar);
			painter.drawLines(lines);
		}
	}

	// notes
	{
		int i=0;
		for (SoundChannelView *channelView : soundChannels){
			for (SoundNoteView *nview : channelView->GetNotes()){
				SoundNote note = nview->GetNote();
				if (note.location > tEnd){
					break;
				}
				if (note.location + note.length < tBegin){
					continue;
				}
				if (note.lane > 0 && lanes.contains(note.lane)){
					if (nview == cursorExistingNote){
						// hover
						LaneDef &laneDef = lanes[note.lane];
						QRect rect(laneDef.left+1, Time2Y(note.location + note.length) - 8, laneDef.width-1, TimeSpan2DY(note.length) + 8);
						QLinearGradient g(QPointF(rect.left(), 0), QPointF(rect.right(), 0));
						SetNoteColor(g, note.lane, true);
						painter.fillRect(rect, QBrush(g));
					}else if (selectedNotes.contains(nview)){
						LaneDef &laneDef = lanes[note.lane];
						QRect rect(laneDef.left+1, Time2Y(note.location + note.length) - 8, laneDef.width-1, TimeSpan2DY(note.length) + 8);
						QLinearGradient g(QPointF(rect.left(), 0), QPointF(rect.right(), 0));
						SetNoteColor(g, note.lane, i==currentChannel);
						painter.fillRect(rect, QBrush(g));
						painter.setBrush(Qt::NoBrush);
						painter.setPen(QPen(QBrush(QColor(255, 255, 255)), 1));
						QRect rect2(laneDef.left+1, Time2Y(note.location + note.length) - 8, laneDef.width-2, TimeSpan2DY(note.length) + 7);
						painter.drawRect(rect2);
					}else{
						LaneDef &laneDef = lanes[note.lane];
						QRect rect(laneDef.left+1, Time2Y(note.location + note.length) - 8, laneDef.width-1, TimeSpan2DY(note.length) + 8);
						QLinearGradient g(QPointF(rect.left(), 0), QPointF(rect.right(), 0));
						SetNoteColor(g, note.lane, i==currentChannel);
						painter.fillRect(rect, QBrush(g));
					}
				}
			}
			i++;
		}
	}

	// horz. cursor line etc.
	if (showCursorNewNote && currentChannel >= 0){
		painter.setPen(QPen(QBrush(QColor(255, 255, 255)), 1));
		int y = Time2Y(cursorNewNote.location) - 1;
		painter.drawLine(left, y, right, y);

		if (soundChannels[currentChannel]->GetNotes().contains(cursorNewNote.location)){
			SoundNoteView *nview = soundChannels[currentChannel]->GetNotes()[cursorNewNote.location];
			SoundNote note = nview->GetNote();
			if (note.lane > 0){
				LaneDef &laneDef = lanes[note.lane];
				int noteStartY = Time2Y(note.location);
				int noteEndY = Time2Y(note.location + note.length);
				switch (note.noteType){
				case 0: {
					painter.setBrush(Qt::NoBrush);
					painter.setPen(QPen(QBrush(QColor(255, 0, 0)), 1));
					QPolygon polygon;
					polygon.append(QPoint(laneDef.left, noteStartY));
					polygon.append(QPoint(laneDef.left+laneDef.width, noteStartY));
					polygon.append(QPoint(laneDef.left+laneDef.width/2, noteStartY - 8));
					painter.drawPolygon(polygon);
					if (note.length > 0){
						QRect rect(laneDef.left+1, noteEndY - 8, laneDef.width-2, noteStartY - noteEndY + 7);
						painter.drawRect(rect);
					}
					break;
				}
				case 1: {
					painter.setBrush(Qt::NoBrush);
					painter.setPen(QPen(QBrush(QColor(255, 0, 0)), 1));
					QRect rect(laneDef.left+1, noteEndY - 8, laneDef.width-2, noteStartY - noteEndY + 7);
					painter.drawRect(rect);
					break;
				}
				default:
					break;
				}
			}
		}
	}

	// cursors
	if (cursorExistingNote){
		SoundNote note = cursorExistingNote->GetNote();
		LaneDef &laneDef = lanes[note.lane];
		int noteStartY = Time2Y(note.location);
		int noteEndY = Time2Y(note.location + note.length);
		switch (note.noteType){
		case 0: {
			painter.setBrush(Qt::NoBrush);
			painter.setPen(QPen(QBrush(QColor(255, 0, 0)), 1));
			QPolygon polygon;
			polygon.append(QPoint(laneDef.left, noteStartY));
			polygon.append(QPoint(laneDef.left+laneDef.width, noteStartY));
			polygon.append(QPoint(laneDef.left+laneDef.width/2, noteStartY - 8));
			painter.drawPolygon(polygon);
			if (note.length > 0){
				QRect rect(laneDef.left+1, noteEndY - 8, laneDef.width-2, noteStartY - noteEndY + 7);
				painter.drawRect(rect);
			}
			break;
		}
		case 1: {
			painter.setBrush(Qt::NoBrush);
			painter.setPen(QPen(QBrush(QColor(255, 0, 0)), 1));
			QRect rect(laneDef.left+1, noteEndY - 8, laneDef.width-2, noteStartY - noteEndY + 7);
			painter.drawRect(rect);
			break;
		}
		default:
			break;
		}
	}else if (showCursorNewNote && lanes.contains(cursorNewNote.lane)){
		LaneDef &laneDef = lanes[cursorNewNote.lane];
		int noteStartY = Time2Y(cursorNewNote.location);
		int noteEndY = Time2Y(cursorNewNote.location + cursorNewNote.length);
		switch (cursorNewNote.noteType){
		case 0: {
			painter.setBrush(Qt::NoBrush);
			painter.setPen(QPen(QBrush(QColor(255, 255, 255)), 1));
			QPolygon polygon;
			polygon.append(QPoint(laneDef.left, noteStartY - 1));
			polygon.append(QPoint(laneDef.left+laneDef.width-1, noteStartY - 1));
			polygon.append(QPoint(laneDef.left+laneDef.width/2-1, noteStartY - 8));
			painter.drawPolygon(polygon);
			if (cursorNewNote.length > 0){
				QRect rect(laneDef.left+1, noteEndY - 8, laneDef.width-2, noteStartY - noteEndY + 7);
				painter.drawRect(rect);
			}
			break;
		}
		case 1: {
			painter.setBrush(Qt::NoBrush);
			painter.setPen(QPen(QBrush(QColor(255, 255, 255)), 1));
			QRect rect(laneDef.left+1, noteEndY - 8, laneDef.width-2, noteStartY - noteEndY + 7);
			painter.drawRect(rect);
			break;
		}
		default:
			break;
		}
	}

	return true;
}

bool SequenceView::paintEventHeaderArea(QWidget *header, QPaintEvent *event)
{
	QPainter painter(header);
	QRect rect(event->rect().left(), 0, event->rect().width(), header->height());
	QLinearGradient g(rect.topLeft(), rect.bottomLeft());
	QColor cd(57, 57, 57);
	QColor cl(91, 91, 91);
	g.setColorAt(0, cd);
	g.setColorAt(1, cl);
	painter.fillRect(rect, QBrush(g));
	return true;
}

bool SequenceView::paintEventFooterArea(QWidget *footer, QPaintEvent *event)
{
	QPainter painter(footer);
	QRect rect(event->rect().left(), 0, event->rect().width(), footer->height());
	QLinearGradient g(rect.topLeft(), rect.bottomLeft());
	QColor cd(57, 57, 57);
	QColor cl(91, 91, 91);
	g.setColorAt(0, cd);
	g.setColorAt(1, cl);
	painter.fillRect(rect, QBrush(g));
	return true;
}

bool SequenceView::enterEventPlayingPane(QWidget *playingPane, QEvent *event)
{
	switch (event->type()){
	case QEvent::Enter:
		return true;
	case QEvent::Leave:
		if (cursorExistingNote){
			cursorExistingNote = nullptr;
		}else if (showCursorNewNote){
			showCursorNewNote = false;
		}
		timeLine->update();
		playingPane->update();
		for (auto cview : soundChannels){
			cview->update();
		}
		return true;
	default:
		return false;
	}
}

SoundNoteView *SequenceView::HitTestPlayingPane(int lane, int y, int time)
{
	// space-base hit test (using `y`) is prior to time-base (using `time`)
	SoundNoteView *nviewS = nullptr;
	SoundNoteView *nviewT = nullptr;
	for (SoundChannelView *cview : soundChannels){
		for (SoundNoteView *nv : cview->GetNotes()){
			const SoundNote &note = nv->GetNote();
			if (note.lane == lane && Time2Y(note.location + note.length) - 8 < y && Time2Y(note.location) >= y){
				nviewS = nv;
			}
			if (note.lane == lane && time >= note.location && time <= note.location+note.length){ // LN contains its End
				nviewT = nv;
			}
		}
	}
	if (nviewS){
		return nviewS;
	}
	return nviewT;
}

bool SequenceView::mouseEventPlayingPane(QWidget *playingPane, QMouseEvent *event)
{
	qreal time = Y2Time(event->y());
	int lane = X2Lane(event->x());
	if (snapToGrid){
		time = SnapToFineGrid(time);
	}
	SoundNoteView *noteHit = lane >= 0 ? HitTestPlayingPane(lane, event->y(), time) : nullptr;
	switch (event->type()){
	case QEvent::MouseMove: {
		if (noteHit){
			playingPane->setCursor(Qt::SizeAllCursor);
			if (noteHit != cursorExistingNote){
				cursorExistingNote = noteHit;
				showCursorNewNote = false;
			}
		}else{
			if (currentChannel >= 0){
				if (lane >= 0){
					playingPane->setCursor(Qt::CrossCursor);
					cursorNewNote.location = time;
					cursorNewNote.lane = lane;
					cursorNewNote.length = 0;
					cursorNewNote.noteType = event->modifiers() & Qt::ShiftModifier ? 0 : 1;
					showCursorNewNote = true;
					cursorExistingNote = nullptr;
				}else if (showCursorNewNote || cursorExistingNote){
					playingPane->setCursor(Qt::ArrowCursor);
					showCursorNewNote = false;
					cursorExistingNote = nullptr;
				}
			}else{
				// no current channel
				playingPane->setCursor(Qt::ArrowCursor);
				if (showCursorNewNote || cursorExistingNote){
					showCursorNewNote = false;
					cursorExistingNote = nullptr;
				}
			}
		}
		timeLine->update();
		playingPane->update();
		for (auto cview : soundChannels){
			cview->update();
		}
		return true;
	}
	case QEvent::MouseButtonPress:
		if (noteHit){
			switch (event->button()){
			case Qt::LeftButton:
				// select note
				if (event->modifiers() & Qt::ControlModifier){
					if (selectedNotes.contains(noteHit)){
						selectedNotes.remove(noteHit);
					}else{
						selectedNotes.insert(noteHit);
					}
				}else{
					selectedNotes.clear();
					selectedNotes.insert(noteHit);
				}
				SelectSoundChannel(noteHit->GetChannelView());
				PreviewSingleNote(noteHit);
				break;
			case Qt::RightButton:
				// delete note
				if (soundChannels[currentChannel]->GetNotes().contains(noteHit->GetNote().location)
					&& soundChannels[currentChannel]->GetChannel()->RemoveNote(noteHit->GetNote()))
				{
					selectedNotes.clear();
				}else{
					// noteHit was in inactive channel, or failed to delete note
					selectedNotes.clear();
					selectedNotes.insert(noteHit);
				}
				SelectSoundChannel(noteHit->GetChannelView());
				break;
			case Qt::MidButton:
				selectedNotes.clear();
				SelectSoundChannel(noteHit->GetChannelView());
				break;
			}
		}else{
			if (currentChannel >= 0 && lane > 0 && event->button() == Qt::LeftButton){
				// insert note (maybe moving existing note)
				selectedNotes.clear();
				SoundNote note(time, lane, 0, event->modifiers() & Qt::ShiftModifier ? 0 : 1);
				if (soundChannels[currentChannel]->GetChannel()->InsertNote(note)){
					// select the note
					const QMap<int, SoundNoteView*> &notes = soundChannels[currentChannel]->GetNotes();
					if (notes.contains(time)){
						selectedNotes.insert(notes[time]);
						PreviewSingleNote(notes[time]);
					}
					timeLine->update();
					playingPane->update();
					for (auto cview : soundChannels){
						cview->update();
					}
				}else{
					// note was not created
				}
			}
		}
		return true;
	case QEvent::MouseButtonRelease:
		return true;
	case QEvent::MouseButtonDblClick:
		return false;
	default:
		return false;
	}
}

void SequenceView::SelectSoundChannel(SoundChannelView *cview){
	for (int i=0; i<soundChannels.size(); i++){
		if (soundChannels[i] == cview){
			if (currentChannel != i){
				OnCurrentChannelChanged(i);
				emit CurrentChannelChanged(i);
				break;
			}
		}
	}
}

void SequenceView::PreviewSingleNote(SoundNoteView *nview)
{
	SoundChannelNotePreviewer *previewer = new SoundChannelNotePreviewer(nview->GetChannelView()->GetChannel(), nview->GetNote().location, this);
	connect(previewer, SIGNAL(Stopped()), previewer, SLOT(deleteLater())); // toriaezu
	mainWindow->GetAudioPlayer()->Play(previewer);
}

bool SequenceView::paintEventHeaderEntity(QWidget *widget, QPaintEvent *event)
{
	QPainter painter(widget);
	QRect rect(0, 0, widget->width(), widget->height());
	QLinearGradient g(rect.topLeft(), rect.bottomLeft());
	QColor cd(204, 204, 204);
	QColor cl(238, 238, 238);
	g.setColorAt(0, cl);
	g.setColorAt(1, cd);
	painter.setBrush(QBrush(g));
	painter.setPen(QColor(170, 170, 170));
	painter.drawRect(rect.adjusted(0, 0, -1, -1));
	return true;
}

bool SequenceView::paintEventFooterEntity(QWidget *widget, QPaintEvent *event)
{
	QPainter painter(widget);
	QRect rect(0, 0, widget->width(), widget->height());
	QLinearGradient g(rect.topLeft(), rect.bottomLeft());
	QColor cd(204, 204, 204);
	QColor cl(238, 238, 238);
	g.setColorAt(0, cl);
	g.setColorAt(1, cd);
	painter.setBrush(QBrush(g));
	painter.setPen(QColor(170, 170, 170));
	painter.drawRect(rect.adjusted(0, 0, -1, -1));
	return true;
}






SoundChannelView::SoundChannelView(SequenceView *sview, SoundChannel *channel)
	: QWidget(sview)
	, sview(sview)
	, channel(channel)
	, current(false)
{
	// this make scrolling fast, but I must treat redrawing region carefully.
	//setAttribute(Qt::WA_OpaquePaintEvent);

	setMouseTracking(true);

	// follow current state
	for (SoundNote note : channel->GetNotes()){
		notes.insert(note.location, new SoundNoteView(this, note));
	}

	connect(channel, &SoundChannel::NoteInserted, this, &SoundChannelView::NoteInserted);
	connect(channel, &SoundChannel::NoteRemoved, this, &SoundChannelView::NoteRemoved);
	connect(channel, &SoundChannel::NoteChanged, this, &SoundChannelView::NoteChanged);
	connect(channel, &SoundChannel::RmsUpdated, this, &SoundChannelView::RmsUpdated);
}

SoundChannelView::~SoundChannelView()
{
}

void SoundChannelView::ScrollContents(int dy)
{
	scroll(0, dy);
}


void SoundChannelView::NoteInserted(SoundNote note)
{
	notes.insert(note.location, new SoundNoteView(this, note));
	update();
	sview->playingPane->update();
}

void SoundChannelView::NoteRemoved(SoundNote note)
{
	delete notes.take(note.location);
	update();
	sview->playingPane->update();
}

void SoundChannelView::NoteChanged(int oldLocation, SoundNote note)
{
	SoundNoteView *nview = notes.take(oldLocation);
	nview->UpdateNote(note);
	notes.insert(note.location, nview);
	update();
	sview->playingPane->update();
}

void SoundChannelView::RmsUpdated()
{
	update();
	//sview->playingPane->update();
}



void SoundChannelView::paintEvent(QPaintEvent *event)
{
	static const int mx = 4, my = 4;
	QPainter painter(this);
	QRect rect = event->rect();
	painter.fillRect(rect, current ? QColor(68, 68, 68) : QColor(34, 34, 34));

	int scrollY = sview->verticalScrollBar()->value();

	int left = rect.x() - mx;
	int right = rect.right() + mx;
	int top = rect.y() - my;
	int bottom = rect.bottom() + my;
	int height = bottom - top;

	qreal tBegin = sview->viewLength - (scrollY + bottom)/sview->zoomY;
	qreal tEnd = sview->viewLength - (scrollY + top)/sview->zoomY;

	{
		QSet<int> bars = sview->BarsInRange(tBegin, tEnd);
		QSet<int> coarseGrids = sview->CoarseGridsInRange(tBegin, tEnd) - bars;
		QSet<int> fineGrids = sview->FineGridsInRange(tBegin, tEnd) - bars - coarseGrids;
		{
			QVector<QLine> lines;
			for (int t : fineGrids){
				int y = sview->Time2Y(t) - 1;
				lines.append(QLine(left, y, right, y));
			}
			painter.setPen(QPen(QBrush(QColor(17, 17, 17)), 1));
			painter.drawLines(lines);
		}
		{
			QVector<QLine> lines;
			for (int t : coarseGrids){
				int y = sview->Time2Y(t) - 1;
				lines.append(QLine(left, y, right, y));
			}
			painter.setPen(QPen(QBrush(QColor(17, 17, 17)), 1));
			painter.drawLines(lines);
		}
		{
			QVector<QLine> lines;
			for (int t : bars){
				int y = sview->Time2Y(t) - 1;
				lines.append(QLine(left, y, right, y));
			}
			painter.setPen(QPen(QBrush(QColor(0, 0, 0)), 1));
			painter.drawLines(lines);
		}
	}

	// waveform(rms)
	{
		QMap<int, SoundNoteView*>::const_iterator inote = notes.upperBound(tBegin);
		if (inote != notes.begin()){
			auto iprev = inote-1;
			if ((*iprev)->GetNote().lane == 0){
				painter.setPen(current ? QColor(153, 153, 153) : QColor(85, 85, 85));
			}else{
				painter.setPen(current ? QColor(0, 170, 0) : QColor(0, 102, 0));
			}
		}else{
			painter.setPen(current ? QColor(0, 170, 0) : QColor(0, 85, 0));
		}
		int noteY = inote==notes.end() ? INT_MIN : sview->Time2Y((*inote)->GetNote().location) - 1;
		int y = bottom - 1;
		channel->DrawRmsGraph(tBegin, sview->zoomY, [&](Rms rms){
			if (rms.IsValid()){
				if (y <= noteY){
					if ((*inote)->GetNote().lane == 0){
						painter.setPen(current ? QColor(153, 153, 153) : QColor(85, 85, 85));
					}else{
						painter.setPen(current ? QColor(0, 170, 0) : QColor(0, 102, 0));
					}
					inote++;
					noteY = inote==notes.end() ? INT_MIN : sview->Time2Y((*inote)->GetNote().location) - 1;
				}
				static const float gain = 2.0f;
				painter.drawLine(width()/2, y, width()/2*(1.0 + rms.R*gain), y);
				painter.drawLine(width()/2, y, width()/2*(1.0 - rms.L*gain), y);
			}
			if (--y < top){
				return false;
			}
			return true;
		});
	}

	// notes
	for (SoundNoteView *nview : notes){
		SoundNote note = nview->GetNote();
		if (note.location > tEnd){
			break;
		}
		if (note.location + note.length < tBegin){
			continue;
		}
		int noteStartY = sview->Time2Y(note.location) - 1;
		int noteEndY = sview->Time2Y(note.location + note.length) - 1;

		if (note.lane == 0){
			if (sview->cursorExistingNote == nview){
				painter.setPen(QPen(QBrush(QColor(255, 0, 0)), 1));
				painter.setBrush(Qt::NoBrush);
				switch (note.noteType){
				case 0: {
					QPolygon polygon;
					polygon.append(QPoint(width()/2, noteStartY - 8));
					polygon.append(QPoint(6, noteStartY));
					polygon.append(QPoint(width()-7, noteStartY));
					painter.drawPolygon(polygon);
					break;
				}
				case 1: {
					QRect rect(6, noteStartY - 4, width() - 12, 4); // don't show as long notes
					painter.drawRect(rect);
					break;
				}
				default:
					break;
				}
			}else{
				painter.setPen(QPen(QBrush(current ? QColor(255, 255, 255) : QColor(127, 127, 127)), 1));
				painter.setBrush(Qt::NoBrush);
				switch (note.noteType){
				case 0: {
					QPolygon polygon;
					polygon.append(QPoint(width()/2, noteStartY - 8));
					polygon.append(QPoint(6, noteStartY));
					polygon.append(QPoint(width()-7, noteStartY));
					painter.drawPolygon(polygon);
					break;
				}
				case 1: {
					QRect rect(6, noteStartY - 4, width() - 12, 4); // don't show as long notes
					painter.drawRect(rect);
					break;
				}
				default:
					break;
				}
			}
		}else{
			painter.setPen(QPen(QBrush(current ? QColor(127, 127, 127) : QColor(95, 95, 95)), 1));
			painter.setBrush(Qt::NoBrush);
			painter.drawLine(1, noteStartY, width()-1, noteStartY);
		}
		if (note.noteType == 0){
			painter.setBrush(QBrush(QColor(255, 0, 0)));
			painter.setPen(Qt::NoPen);
			QPolygon polygon;
			polygon.append(QPoint(0, noteStartY - 4));
			polygon.append(QPoint(0, noteStartY + 4));
			polygon.append(QPoint(4, noteStartY));
			painter.drawPolygon(polygon);
			polygon[0] = QPoint(width(), noteStartY - 5);
			polygon[1] = QPoint(width(), noteStartY + 5);
			polygon[2] = QPoint(width()-5, noteStartY);
			painter.drawPolygon(polygon);
		}
	}

	// horz. cursor line
	if (sview->showCursorNewNote){
		painter.setPen(QPen(QBrush(QColor(255, 255, 255)), 1));
		int y = sview->Time2Y(sview->cursorNewNote.location) - 1;
		painter.drawLine(left, y, right, y);

		if (current && notes.contains(sview->cursorNewNote.location)){
			SoundNoteView *nview = notes[sview->cursorNewNote.location];
			const SoundNote &note = nview->GetNote();
			if (note.lane == 0){
				painter.setPen(QPen(QBrush(QColor(255, 0, 0)), 1));
				painter.setBrush(Qt::NoBrush);
				int noteStartY = sview->Time2Y(note.location) - 1;
				switch (note.noteType){
				case 0:{
					QPolygon polygon;
					polygon.append(QPoint(width()/2, noteStartY - 8));
					polygon.append(QPoint(6, noteStartY));
					polygon.append(QPoint(width()-7, noteStartY));
					painter.drawPolygon(polygon);
					break;
				}
				case 1:
					QRect rect(6, noteStartY - 4, width() - 12, 4); // don't show as long notes
					painter.drawRect(rect);
					break;
				}
			}
		}
		if (current && sview->cursorNewNote.lane == 0){
			painter.setPen(QPen(QBrush(QColor(255, 255, 255)), 1));
			painter.setBrush(Qt::NoBrush);
			int noteStartY = sview->Time2Y(sview->cursorNewNote.location) - 1;
			switch (sview->cursorNewNote.noteType){
			case 0:{
				QPolygon polygon;
				polygon.append(QPoint(width()/2, noteStartY - 8));
				polygon.append(QPoint(6, noteStartY));
				polygon.append(QPoint(width()-7, noteStartY));
				painter.drawPolygon(polygon);
				break;
			}
			case 1:
				QRect rect(6, noteStartY - 4, width() - 12, 4); // don't show as long notes
				painter.drawRect(rect);
				break;
			}
		}
	}
}

SoundNoteView *SoundChannelView::HitTestBGPane(int y, qreal time)
{
	// space-base hit test (using `y`) is prior to time-base (using `time`)
	SoundNoteView *nviewS = nullptr;
	SoundNoteView *nviewT = nullptr;
	for (SoundNoteView *nv : notes){
		const SoundNote &note = nv->GetNote();
		if (note.lane == 0 && sview->Time2Y(note.location + note.length) - 4 < y && sview->Time2Y(note.location) >= y){
			nviewS = nv;
		}
		if (note.lane == 0 && time >= note.location && time <= note.location+note.length){ // LN contains its End
			nviewT = nv;
		}
	}
	if (nviewS){
		return nviewS;
	}
	return nviewT;
}

void SoundChannelView::mouseMoveEvent(QMouseEvent *event)
{
	if (current){
		qreal time = sview->Y2Time(event->y());
		if (sview->snapToGrid){
			time = sview->SnapToFineGrid(time);
		}
		SoundNoteView *noteHit = HitTestBGPane(event->y(), time);

		if (noteHit){
			setCursor(Qt::SizeAllCursor);
			if (noteHit != sview->cursorExistingNote){
				sview->cursorExistingNote = noteHit;
				sview->showCursorNewNote = false;
			}
		}else{
			setCursor(Qt::CrossCursor);
			sview->cursorNewNote.location = time;
			sview->cursorNewNote.lane = 0;
			sview->cursorNewNote.length = 0;
			sview->cursorNewNote.noteType = event->modifiers() & Qt::ShiftModifier ? 0 : 1;
			sview->showCursorNewNote = true;
			sview->cursorExistingNote = nullptr;
		}
		sview->timeLine->update();
		sview->playingPane->update();
		for (auto cview : sview->soundChannels){
			cview->update();
		}
	}else{
		setCursor(Qt::ArrowCursor);
		if (sview->showCursorNewNote || sview->cursorExistingNote!=nullptr){
			sview->showCursorNewNote = false;
			sview->cursorExistingNote = nullptr;
			sview->timeLine->update();
			sview->playingPane->update();
			for (auto cview : sview->soundChannels){
				cview->update();
			}
		}
	}
}

void SoundChannelView::mousePressEvent(QMouseEvent *event)
{
	if (current){
		// do something
		qreal time = sview->Y2Time(event->y());
		if (sview->snapToGrid){
			time = sview->SnapToFineGrid(time);
		}
		SoundNoteView *noteHit = HitTestBGPane(event->y(), time);
		if (noteHit){
			switch (event->button()){
			case Qt::LeftButton:
				// select note
				if (event->modifiers() & Qt::ControlModifier){
					if (sview->selectedNotes.contains(noteHit)){
						sview->selectedNotes.remove(noteHit);
					}else{
						sview->selectedNotes.insert(noteHit);
					}
				}else{
					sview->selectedNotes.clear();
					sview->selectedNotes.insert(noteHit);
				}
				break;
			case Qt::RightButton:
				// delete note
				if (channel->GetNotes().contains(noteHit->GetNote().location)
					&& channel->RemoveNote(noteHit->GetNote()))
				{
					sview->selectedNotes.clear();
				}else{
					// noteHit was in inactive channel, or failed to delete note
					sview->selectedNotes.clear();
					sview->selectedNotes.insert(noteHit);
				}
				break;
			case Qt::MidButton:
				sview->selectedNotes.clear();
				break;
			}
		}else{
			if (event->button() == Qt::LeftButton){
				// insert note (maybe moving existing note)
				sview->selectedNotes.clear();
				SoundNote note(time, 0, 0, event->modifiers() & Qt::ShiftModifier ? 0 : 1);
				if (channel->InsertNote(note)){
					// select the note
					if (notes.contains(time)){
						sview->selectedNotes.insert(notes[time]);
					}
					sview->timeLine->update();
					sview->playingPane->update();
					for (auto cview : sview->soundChannels){
						cview->update();
					}
				}else{
					// note was not created
				}
			}
		}
	}else{
		// select this channel
		sview->selectedNotes.clear();
		sview->SelectSoundChannel(this);
	}
	return;
}

void SoundChannelView::mouseReleaseEvent(QMouseEvent *event)
{
}

void SoundChannelView::mouseDoubleClickEvent(QMouseEvent *event)
{
}

void SoundChannelView::enterEvent(QEvent *event)
{
}

void SoundChannelView::leaveEvent(QEvent *event)
{
	if (sview->cursorExistingNote){
		sview->cursorExistingNote = nullptr;
	}else if (sview->showCursorNewNote){
		sview->showCursorNewNote = false;
	}
	sview->timeLine->update();
	sview->playingPane->update();
	for (auto cview : sview->soundChannels){
		cview->update();
	}
}





SoundNoteView::SoundNoteView(SoundChannelView *cview, SoundNote note)
	: QObject(cview)
	, cview(cview)
	, note(note)
{
}

SoundNoteView::~SoundNoteView()
{
}

void SoundNoteView::UpdateNote(SoundNote note)
{
	this->note = note;
}





SoundChannelHeader::SoundChannelHeader(SequenceView *sview, SoundChannelView *cview)
	: QWidget(sview)
	, sview(sview)
	, cview(cview)
{
	auto *tb = new QToolBar(this);
	tb->addAction("S");
	tb->addAction("M");
}

SoundChannelHeader::~SoundChannelHeader()
{
}

void SoundChannelHeader::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	QRect rect(0, 0, width(), height());
	QLinearGradient g(rect.topLeft(), rect.bottomLeft());
	QColor cd(204, 204, 204);
	QColor cl(238, 238, 238);
	g.setColorAt(0, cl);
	g.setColorAt(1, cd);
	painter.setBrush(QBrush(g));
	painter.setPen(QColor(170, 170, 170));
	painter.drawRect(rect.adjusted(0, 0, -1, -1));
}

void SoundChannelHeader::mouseMoveEvent(QMouseEvent *event)
{

}

void SoundChannelHeader::mousePressEvent(QMouseEvent *event)
{
	sview->SelectSoundChannel(cview);
}

void SoundChannelHeader::mouseReleaseEvent(QMouseEvent *event)
{

}

void SoundChannelHeader::mouseDoubleClickEvent(QMouseEvent *event)
{

}






SoundChannelFooter::SoundChannelFooter(SequenceView *sview, SoundChannelView *cview)
	: QWidget(sview)
	, sview(sview)
	, cview(cview)
{
	QFont f = font();
	f.setPixelSize((height()-10)/2);
	setFont(f);
}

SoundChannelFooter::~SoundChannelFooter()
{
}

void SoundChannelFooter::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	QRect rect(0, 0, width(), height());
	QLinearGradient g(rect.topLeft(), rect.bottomLeft());
	QColor cd(204, 204, 204);
	QColor cl(238, 238, 238);
	g.setColorAt(0, cl);
	g.setColorAt(1, cd);
	painter.setBrush(QBrush(g));
	painter.setPen(QColor(170, 170, 170));
	painter.drawRect(rect.adjusted(0, 0, -1, -1));

	QRect rectText = rect.marginsRemoved(QMargins(4, 4, 4, 4));
	QString name = cview->GetName();
	static const QString prefix = "...";
	bool prefixed = false;
	QRect rectBB = painter.boundingRect(rectText, Qt::TextWrapAnywhere, name);
	while (!rectText.contains(rectBB)){
		name = name.mid(1);
		rectBB = painter.boundingRect(rectText, Qt::TextWrapAnywhere, prefix + name);
		prefixed = true;
	};
	painter.setPen(QColor(0, 0, 0));
	painter.drawText(rectText, Qt::TextWrapAnywhere, prefixed ? prefix + name : name);
}

void SoundChannelFooter::mouseMoveEvent(QMouseEvent *event)
{

}

void SoundChannelFooter::mousePressEvent(QMouseEvent *event)
{
	sview->SelectSoundChannel(cview);
	if (event->button() == Qt::MidButton){
		sview->mainWindow->GetAudioPlayer()->PreviewSoundChannelSource(cview->channel);
	}
}

void SoundChannelFooter::mouseReleaseEvent(QMouseEvent *event)
{

}

void SoundChannelFooter::mouseDoubleClickEvent(QMouseEvent *event)
{

}





