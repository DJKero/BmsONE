
#include "UIDef.h"
#ifdef Q_OS_WIN
//#include <qpa/qplatformnativeinterface.h>
#include <windows.h>
#endif


IEdit *SharedUIHelper::globallyDirtyEdit = nullptr;
QSet<QAction*> SharedUIHelper::globalShortcuts;
int SharedUIHelper::globalShortcutsLockCounter = 0;


void SharedUIHelper::SetGloballyDirtyEdit(IEdit *edit)
{
	// don't commit old globallyDirtyEdit
	globallyDirtyEdit = edit;
}

void SharedUIHelper::CommitDirtyEdit()
{
	if (globallyDirtyEdit){
		globallyDirtyEdit->Commit();
		globallyDirtyEdit = nullptr;
	}
}

void SharedUIHelper::RegisterGlobalShortcut(QAction *action)
{
	globalShortcuts.insert(action);
}

void SharedUIHelper::LockGlobalShortcuts()
{
	if (globalShortcutsLockCounter++ == 0){
		for (auto action : globalShortcuts){
			action->blockSignals(true);
		}
	}
}

void SharedUIHelper::UnlockGlobalShortcuts()
{
	if (--globalShortcutsLockCounter == 0){
		for (auto action : globalShortcuts){
			action->blockSignals(false);
		}
	}
}



#ifdef Q_OS_WIN
#pragma comment(lib, "user32.lib")
static QFont defaultFont;
static bool defaultFontInitialized = false;
#endif


void UIUtil::SetFont(QWidget *widget)
{
#ifdef Q_OS_WIN
	if (defaultFontInitialized){
		widget->setFont(defaultFont);
	}else{
		LOGFONTW lf;
		SystemParametersInfoW(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
		defaultFont.setFamily(QString::fromUtf16((const ushort *)&lf.lfFaceName));
		widget->setFont(defaultFont);
		defaultFontInitialized = true;
	}
#endif
}

void UIUtil::SetFontMainWindow(QWidget *widget)
{
#ifdef Q_OS_WIN
	SetFont(widget);
#else
	SetFont(widget);
#endif
}

bool UIUtil::DragBegins(QPoint origin, QPoint pos)
{
	static const int DRAG_THRESHOLD = 8;
	return abs(pos.x() - origin.x()) > DRAG_THRESHOLD
			|| abs(pos.y() - origin.y()) > DRAG_THRESHOLD;
}


