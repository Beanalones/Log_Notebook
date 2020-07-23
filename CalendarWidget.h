#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QDate>

class CalendarWidget : public QWidget
{
	Q_OBJECT

public:
	CalendarWidget(QWidget *parent = Q_NULLPTR);
	~CalendarWidget();

	void setMonth(QDate date);
	QList<QDate> getDates();

signals:
	void datesChanged();

protected:
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:

	QDate getDateForCell(QPoint cell);
	const int rows = 5;
	const int columns = 7;
	
	int boxWidth;
	int boxHeight;
	
	int headerHeight = 50;
	QString daysOfTheWeek[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
	QString days[35];

	QPoint pressedCell;
	QList<QPoint> selectedCells;
	QList<QPoint> terminalCells;
	QList<QPoint> hoveredCells;
	bool movingTerminalCell = false;

	QDate visibleMonth;
};
