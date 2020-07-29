#include "CalendarWidget.h"
#include <QPainter>
#include <QPaintEvent>

CalendarWidget::CalendarWidget(QWidget *parent)
	: QWidget(parent)
{
	setMouseTracking(true);

	QDate first(QDate::currentDate().year(), QDate::currentDate().month(), 1);
	QDate date;
	int i = 0;
	date = first.addDays(-first.dayOfWeek());
	for (int i = 0; i < (columns * rows); i++) {
		cells.append(CalendarCell(date.addDays(i)));
		if (date.addDays(i) == QDate::currentDate()) cells[i].isToday = true;
	}
	firstShownCell = 0;
}

CalendarWidget::~CalendarWidget()
{
}

QList<QDate> CalendarWidget::getSelectedDates()
{
	CalendarCell* firstTerminal;
	CalendarCell* secondTerminal;

	bool one = false;
	bool two = false;
	for(int i = 0; i < cells.count(); i++) {
		if ((cells[i].isTerminal) && (firstTerminal == nullptr)) {
			firstTerminal = &cells[i];
			one = true;
		}
		else if (cells[i].isTerminal) {
			secondTerminal = &cells[i];
			two = true;
		}
	}
	if ((one == false) || (two == false)) {
		return QList<QDate>();
	}
	QList<QDate> dates;
	dates.append(firstTerminal->date);
	dates.append(secondTerminal->date);
	return dates;
}

void CalendarWidget::clearSelected()
{
	for (int i = 0; i < cells.count(); i++) {
		cells[i].isSelected = false;
		cells[i].isTerminal = false;
	}
	terminalCellsCount = 0;
}

void CalendarWidget::mouseMoveEvent(QMouseEvent* event)
{
	if ((event->x() > ((boxWidth * columns)) - 1) || (event->y() < headerHeight)) {
		clearHoveredCells();
		return;
	}
	QPoint point(event->x() / boxWidth, (event->y() - headerHeight) / boxHeight);
	int cell = cellForPoint(point);
	if (cell == -1) return;
	cells[cell].isHovered = true;

	for (int i = firstShownCell; i < finalShownCell(); i++) {
		if(i != cell) cells[i].isHovered = false;
	}

	if (movingTerminalCell) {
		int first = -1;
		int second;
		for (int i = 0; i < finalShownCell(); i++) {
			if (cells[i].isTerminal) {
				first = i;
			}
			cells[i].isSelected = false;
		}
		if (first == -1) return;
		if (cell < first) {
			second = first;
			first = cell;
		}
		else {
			second = cell;
		}
		for (int j = (first + 1); j < second; j++) {
			cells[j].isSelected = true;
		}
	}
	repaint();
}

void CalendarWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		QPoint point = QPoint(event->x() / boxWidth, (event->y() - headerHeight) / boxHeight);
		int cell = cellForPoint(point);
		if (cell == -1) return;
		pressedCell = &cells[cell];
		
		if (pressedCell->isTerminal && (terminalCellsCount == 2)) {
			movingTerminalCell = true;
			pressedCell->isTerminal = false;
		}
		else if (terminalCellsCount == 0) {
			pressedCell->isTerminal = true;
			terminalCellsCount = 2;
			movingTerminalCell = true;
			repaint();
		}
		else if ((terminalCellsCount == 1) && pressedCell->isTerminal) {
			terminalCellsCount = 2;
			movingTerminalCell = true;
			repaint();
		}
	}
}

void CalendarWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		if (pressedCell == nullptr) return;
		QPoint point(event->x() / boxWidth, (event->y() - headerHeight) / boxHeight);
		int cell = cellForPoint(point);
		if (cell == -1) return;

		if (movingTerminalCell) {
			if (cells[cell].isTerminal) {
				terminalCellsCount--;
			}
			else {
				cells[cell].isTerminal = true;
			}
			emit datesChanged();
		}
		pressedCell = nullptr;
		movingTerminalCell = false;
		repaint();
	}
}

void CalendarWidget::wheelEvent(QWheelEvent* event)
{
	if (event->angleDelta().y() > 0) {
		if (firstShownCell < 7) {
			for (int i = 0; i < 7; i++) {
				cells.prepend(CalendarCell(cells[0].date.addDays(-1)));
			}
		}
		else {
			firstShownCell -= 7;
		}
	}
	else if (event->angleDelta().y() < 0) {
		if (cells.count() <= firstShownCell + (columns * rows)) {
			for (int i = 0; i < 7; i++) {
				cells.append(CalendarCell(cells[cells.count() - 1].date.addDays(1)));
			}
		}
		firstShownCell += 7;
	}
	repaint();
}

void CalendarWidget::leaveEvent(QEvent* event)
{
	clearHoveredCells();
}

void CalendarWidget::paintEvent(QPaintEvent* event)
{
	QBrush selectedBrush(Qt::SolidPattern);
	selectedBrush.setColor(QColor(139, 204, 247, 150));

	QBrush hoverdBrush(Qt::SolidPattern);
	hoverdBrush.setColor(QColor(99, 112, 120, 100));

	QBrush terminalBrush(Qt::SolidPattern);
	terminalBrush.setColor(QColor(130, 38, 38, 200));

	QBrush todayBrush(Qt::SolidPattern);
	todayBrush.setColor(QColor(255, 137, 33, 255));

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(event->rect(), QBrush(Qt::white));
	painter.setPen(QPen(Qt::gray));

	boxWidth = (event->rect().width() - monthSideWidth) / columns;
	boxHeight = (event->rect().height() - headerHeight) / rows;

	QFont font = painter.font();
	font.setPixelSize(22);
	painter.setFont(font);

	for (int j = 0; j < columns; j++) {
		QRect rect(j * boxWidth, 0, boxWidth, headerHeight);
		painter.setPen(QPen(Qt::black));
		painter.drawText(rect, Qt::AlignCenter, daysOfTheWeek[j]);
	}

	

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			QRect rect((j * boxWidth), (i * boxHeight) + headerHeight, boxWidth, boxHeight);
			int cellNum = ((i * columns) + j) + firstShownCell;
			if (cells[cellNum].isHovered) {
				painter.fillRect(rect, hoverdBrush);
			}

			if (cells[cellNum].isSelected) {
				painter.fillRect(rect, selectedBrush);
			}

			if (cells[cellNum].isTerminal) {
				painter.fillRect(rect, terminalBrush);
			}

			if (cells[cellNum].isToday) {
				painter.setPen(QPen(Qt::gray));
				painter.drawRect(rect);
				painter.fillRect(QRect(rect.topLeft(), QPoint(rect.bottomRight().x(), rect.topRight().y() + ((rect.bottomRight().y() - rect.topRight().y()) /6))), todayBrush);
			}
			
			painter.setPen(QPen(Qt::black));
			if (cells[cellNum].date.day() == 15) {
				font.setPixelSize(35);
				painter.setFont(font);
				if (cells[cellNum].date.month() == QDate::currentDate().month()) {
					painter.setPen(QPen(todayBrush.color()));
				}
				QRect monthRect(event->rect().width() - monthSideWidth, (i * boxHeight) + headerHeight, monthSideWidth, boxHeight * 2);
				painter.drawText(monthRect, Qt::AlignCenter, cells[cellNum].date.toString("MMM\nyyyy"));
			}

			font.setPixelSize(48);
			painter.setFont(font);
			painter.setPen(QPen(Qt::black));
			painter.drawText(rect, Qt::AlignCenter, QString::number(cells[cellNum].date.day()));
		}
	}
}

void CalendarWidget::clearHoveredCells() {
	for (int i = firstShownCell; i < finalShownCell(); i++) {
		cells[i].isHovered = false;
	}
	repaint();
}

int CalendarWidget::cellForPoint(QPoint point)
{
	QDate date = cells[firstShownCell].date.addDays((point.y() * columns) + point.x());
	for (int i = firstShownCell; i < finalShownCell(); i++) {
		if (cells[i].date == date) {
			return i;
		}
	}
	return -1;
}