#include "ChannelFindTools.h"
#include "MainWindow.h"
#include "SequenceView.h"
#include "SymbolIconManager.h"


ChannelFindTools::ChannelFindTools(const QString &objectName, const QString &windowTitle, MainWindow *mainWindow)
	: QToolBar(windowTitle, mainWindow)
	, mainWindow(mainWindow)
	, sview(nullptr)
{
	setObjectName(objectName);
	setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

	actionChannelFind = new QAction(tr("Find..."), this);
	actionChannelFind->setIcon(SymbolIconManager::GetIcon(SymbolIconManager::Icon::Search));
	actionChannelFind->setShortcut(QKeySequence::Find);
	SharedUIHelper::RegisterGlobalShortcut(actionChannelFind);

	actionChannelFindNext = new QAction(tr("Find Next"), this);
	actionChannelFindNext->setIcon(SymbolIconManager::GetIcon(SymbolIconManager::Icon::Next));
	actionChannelFindNext->setShortcuts(QKeySequence::FindNext);
	SharedUIHelper::RegisterGlobalShortcut(actionChannelFindNext);

	actionChannelFindPrev = new QAction(tr("Find Previous"), this);
	actionChannelFindPrev->setIcon(SymbolIconManager::GetIcon(SymbolIconManager::Icon::Previous));
	actionChannelFindPrev->setShortcuts(QKeySequence::FindPrevious);
	SharedUIHelper::RegisterGlobalShortcut(actionChannelFindPrev);

	actionChannelFindFilterActive = new QAction(tr("Active Channels Only"), this);
	actionChannelFindFilterActive->setIcon(SymbolIconManager::GetIcon(SymbolIconManager::Icon::Search));
	actionChannelFindFilterActive->setShortcut(Qt::ControlModifier + Qt::Key_B);
	SharedUIHelper::RegisterGlobalShortcut(actionChannelFindFilterActive);
	actionChannelFindFilterActive->setCheckable(true);

	actionChannelFindHideOthers = new QAction(tr("Show Found Channels Only"), this);
	actionChannelFindHideOthers->setIcon(SymbolIconManager::GetIcon(SymbolIconManager::Icon::Search));
	actionChannelFindHideOthers->setShortcut(Qt::ControlModifier + Qt::Key_H);
	SharedUIHelper::RegisterGlobalShortcut(actionChannelFindHideOthers);
	actionChannelFindHideOthers->setCheckable(true);

	clear = new QAction(tr("Clear"), this);
	clear->setIcon(SymbolIconManager::GetIcon(SymbolIconManager::Icon::Clear));

	auto select = new QAction(tr("Find"), this);
	select->setIcon(SymbolIconManager::GetIcon(SymbolIconManager::Icon::Search));

	keyword = new QuasiModalEdit();
	keyword->setPlaceholderText(tr("Channel Name"));
	keyword->addAction(select, QLineEdit::ActionPosition::LeadingPosition);
	keyword->addAction(clear, QLineEdit::ActionPosition::TrailingPosition);
	keyword->setMaximumWidth(160);
	addWidget(keyword);

	addAction(actionChannelFindPrev);
	addAction(actionChannelFindNext);
	addSeparator();
	addAction(actionChannelFindFilterActive);
	addAction(actionChannelFindHideOthers);

	auto menuChannelFind = mainWindow->menuChannelFind;
	menuChannelFind->addAction(actionChannelFind);
	menuChannelFind->addAction(actionChannelFindNext);
	menuChannelFind->addAction(actionChannelFindPrev);
	menuChannelFind->addSeparator();
	menuChannelFind->addAction(actionChannelFindFilterActive);
	menuChannelFind->addAction(actionChannelFindHideOthers);

	connect(actionChannelFind, SIGNAL(triggered(bool)), this, SLOT(Activate()));
	connect(keyword, SIGNAL(returnPressed()), this, SLOT(Next()));
	connect(keyword, SIGNAL(EscPressed()), this, SLOT(Inactivate()));
	connect(select, SIGNAL(triggered(bool)), keyword, SLOT(selectAll()));
	connect(clear, SIGNAL(triggered(bool)), this, SLOT(Clear()));
	connect(actionChannelFindNext, SIGNAL(triggered(bool)), this, SLOT(Next()));
	connect(actionChannelFindPrev, SIGNAL(triggered(bool)), this, SLOT(Prev()));

	connect(keyword, SIGNAL(textChanged(QString)), mainWindow, SLOT(ChannelFindKeywordChanged(QString)));
	connect(this, SIGNAL(FindNext(QString)), mainWindow, SLOT(ChannelFindNext(QString)));
	connect(this, SIGNAL(FindPrev(QString)), mainWindow, SLOT(ChannelFindPrev(QString)));
}

ChannelFindTools::~ChannelFindTools()
{
}

void ChannelFindTools::ReplaceSequenceView(SequenceView *newSView)
{
	sview = newSView;
}



void ChannelFindTools::hideEvent(QHideEvent *event)
{
	emit (isVisible() ? Activated() : Inactivated());
}

void ChannelFindTools::showEvent(QShowEvent *event)
{
	emit (isVisible() ? Activated() : Inactivated());
}

void ChannelFindTools::Activate()
{
	show();
	keyword->selectAll();
	keyword->setFocus(Qt::FocusReason::PopupFocusReason);
}

void ChannelFindTools::Inactivate()
{
	hide();
	if (sview){
		sview->setFocus();
	}
}

void ChannelFindTools::Clear()
{
	keyword->clear();
}

void ChannelFindTools::Prev()
{
	if (keyword->text().isEmpty())
		return;
	emit FindPrev(keyword->text());
}

void ChannelFindTools::Next()
{
	if (keyword->text().isEmpty())
		return;
	emit FindNext(keyword->text());
}

