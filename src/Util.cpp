
#include "UIDef.h"



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




void UIUtil::SetFont(QWidget *widget)
{
#ifdef Q_OS_WIN
	widget->setFont(QFont("Meiryo"));
#endif
}


