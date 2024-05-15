#ifndef COLLAPSEBUTTON_H
#define COLLAPSEBUTTON_H

#include <QObject>
#include <QToolButton>


class CollapseButton : public QToolButton
{
	Q_OBJECT

private:
	QWidget *content;
	QString text;

	virtual void paintEvent(QPaintEvent *e);

private slots:
	void Clicked();

public slots:
	void SetText(QString text);

signals:
	void Changed();

public:
	CollapseButton(QWidget *content, QWidget *parent=nullptr);

	void Expand();
	void Collapse();

};

#endif // COLLAPSEBUTTON_H
