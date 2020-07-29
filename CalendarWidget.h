#pragma once

#include <QWidget>
#include <QDebug>
#include <QDate>
#include "CalendarCell.h"

class CalendarWidget : public QWidget
{
	Q_OBJECT

public:
	CalendarWidget(QWidget *parent = Q_NULLPTR);
	~CalendarWidget();

	QList<QDate> getSelectedDates();
	void clearSelected();

signals:
	void datesChanged();

protected:
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

	void wheelEvent(QWheelEvent* event) override;

	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	void clearHoveredCells();
	int cellForPoint(QPoint point);

	const int rows = 5;
	const int columns = 7;
	
	int boxWidth;
	int boxHeight;
	
	int headerHeight = 50;
	int monthSideWidth = 175;

	QString daysOfTheWeek[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
	
	CalendarCell* pressedCell;
	bool movingTerminalCell = false;

	QList<CalendarCell> cells;
	int firstShownCell = 0;
	int finalShownCell() { return (columns * rows) + firstShownCell; }

	int terminalCellsCount = 0;
};
