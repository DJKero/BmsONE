#ifndef SCROLLABLEFORM_H
#define SCROLLABLEFORM_H

#include <QObject>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QFormLayout>


class ScrollableForm : public QScrollArea
{
	Q_OBJECT

private:
	QWidget *form;
	QFormLayout *layout;

	virtual bool viewportEvent(QEvent *event);


public:
	ScrollableForm(QWidget *parent=nullptr);
	virtual ~ScrollableForm();

	QWidget *Form(){ return form; }
	void Initialize(QFormLayout *layout);
};


#endif // SCROLLABLEFORM_H
