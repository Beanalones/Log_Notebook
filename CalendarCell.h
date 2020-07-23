#pragma once

#include <QDate>
struct CalendarCell
{
	CalendarCell(QDate date);

	bool isSelected = false;
	bool isTerminal = false;
	bool isHovered = false;
	bool isToday = false;
	QDate date;
};

