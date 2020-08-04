#pragma once

#include <QDate>
struct CalendarCell
{
	CalendarCell(QDate d);
	CalendarCell();

	bool isSelected = false;
	bool isTerminal = false;
	bool isHovered = false;
	bool monthColor = 0;

	QDate date;
};

