#ifndef BPMEDITTOOL_H
#define BPMEDITTOOL_H

#include <QApplication>
#include <QLabel>
#include "util/QuasiModalEdit.h"
#include "util/ScrollableForm.h"
#include "util/CollapseButton.h"
#include "document/DocumentDef.h"


class MainWindow;

class BpmEditView : public ScrollableForm
{
	Q_OBJECT

private:
	MainWindow *mainWindow;
	QFormLayout *formLayout;
	QLabel *message;
	QuasiModalEdit *edit;
	CollapseButton *buttonShowExtraFields;
	QuasiModalMultiLineEdit *editExtraFields;
	QWidget *dummy;
	bool automated;

	QList<BpmEvent> bpmEvents;

private:
	void Update();
	void SetBpm(float bpm, bool uniform=true);
	void SetExtraFields(const QMap<QString, QJsonValue> &fields, bool uniform=true);

private slots:
	void Edited();
	void EscPressed();

	void ExtraFieldsEdited();
	void ExtraFieldsEscPressed();

	void UpdateFormGeom();

public:
	BpmEditView(MainWindow *mainWindow);
	virtual ~BpmEditView();

	QList<BpmEvent> GetBpmEvents() const{ return bpmEvents; }

	void UnsetBpmEvents();
	void SetBpmEvent(BpmEvent event);
	void SetBpmEvents(QList<BpmEvent> events);

signals:
	void Updated();

};


#endif // BPMEDITTOOL_H
