#include "CalendarWidget.h"
#include <QPainter>
#include <QPaintEvent>

CalendarWidget::CalendarWidget(QWidget *parent)
	: QWidget(parent)
{
	setMonth(QDate::currentDate());
	setMouseTracking(true);
}

CalendarWidget::~CalendarWidget()
{
}

void CalendarWidget::setMonth(QDate date)
{
	date.setDate(date.year(), date.month(), 1);
	int totalDays = date.daysInMonth();
	int firstDay = date.dayOfWeek();
	bool started = false;

	for (int i = 0; i < (columns * rows); i++) {
		if (!started) {
			if ((firstDay) == i) {
				days[i] = QString::number(i - firstDay + 1);
				started = true;
			}
			else {
				days[i] = "";
			}
		}
		else {
			if ((i - firstDay) < totalDays) {
				days[i] = QString::number(i - firstDay + 1);
			}
			else {
				days[i] = "";
			}
		}
	}
	visibleMonth = date;
	repaint();
}

QList<QDate> CalendarWidget::getDates()
{
	QPoint firstCell;
	QPoint secondCell;

	if (((terminalCells.first().y() * columns) + terminalCells.first().x()) > ((terminalCells.last().y() * columns) + terminalCells.last().x())) {
		firstCell = terminalCells.last();
		secondCell = terminalCells.first();
	}
	else {
		firstCell = terminalCells.first();
		secondCell = terminalCells.last();
	}
	QList<QDate> dates;
	dates.append(getDateForCell(firstCell));
	dates.append(getDateForCell(secondCell));
	return dates;
}

void CalendarWidget::mouseMoveEvent(QMouseEvent* event)
{
	hoveredCells.clear();
	QPoint cell(event->x() / boxWidth, (event->y() - headerHeight) / boxHeight);
	hoveredCells.append(cell);

	if (movingTerminalCell) {
		if (terminalCells.count() > 1) terminalCells.removeLast();
		terminalCells.append(cell);

		QPoint firstCell;
		QPoint secondCell;

		if (((terminalCells.first().y() * columns) + terminalCells.first().x()) > ((terminalCells.last().y() * columns) + terminalCells.last().x())) {
			firstCell = terminalCells.last();
			secondCell = terminalCells.first();
		}
		else {
			firstCell = terminalCells.first();
			secondCell = terminalCells.last();
		}
		selectedCells.clear();
		if (firstCell != secondCell) {
			for (int i = (firstCell.y() * columns) + firstCell.x(); i < (secondCell.y() * columns) + secondCell.x(); i++) {
				int y = i / columns;
				int x = i % columns;
				selectedCells.append(QPoint(x, y));
			}
		}
	}
	repaint();
}

void CalendarWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		pressedCell = QPoint(event->x() / boxWidth, (event->y() - headerHeight) / boxHeight);
		if (terminalCells.contains(pressedCell)) {
			movingTerminalCell = true;
			terminalCells.removeOne(pressedCell);
		}
		else if (terminalCells.count() == 0) {
			terminalCells.append(pressedCell);
			movingTerminalCell = true;
			repaint();
		}
	}
}

void CalendarWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		if (pressedCell == QPoint(-1, -1)) return;
		QPoint releaseCell(event->x() / boxWidth, (event->y() - headerHeight) / boxHeight);
		QDate date = getDateForCell(releaseCell);

		if (date.day() != 0) {
			if (movingTerminalCell) {
				if (terminalCells.count() > 2) terminalCells.removeOne(pressedCell);
				terminalCells.append(releaseCell);
				emit datesChanged();
			}
		}
		pressedCell = QPoint(-1, -1);
		movingTerminalCell = false;
		repaint();
	}
}

void CalendarWidget::leaveEvent(QEvent* event)
{
	hoveredCells.clear();
	repaint();
}

void CalendarWidget::paintEvent(QPaintEvent* event)
{

	QBrush selectedBrush(Qt::SolidPattern);
	selectedBrush.setColor(QColor(139, 204, 247, 150));

	QBrush hoverdBrush(Qt::SolidPattern);
	hoverdBrush.setColor(QColor(99, 112, 120, 100));

	QBrush terminalBrush(Qt::SolidPattern);
	terminalBrush.setColor(QColor(130, 38, 38, 200));

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(event->rect(), QBrush(Qt::white));
	painter.setPen(QPen(Qt::gray));

	boxWidth = event->rect().width() / columns;
	boxHeight = (event->rect().height() - headerHeight) / rows;

	QFont font = painter.font();
	font.setPixelSize(22);
	painter.setFont(font);

	for (int j = 0; j < columns; j++) {
		QRect rect(j * boxWidth, 0, boxWidth, headerHeight);
		painter.setPen(QPen(Qt::black));
		painter.drawText(rect, Qt::AlignCenter, daysOfTheWeek[j]);
	}

	font.setPixelSize(48);
	painter.setFont(font);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			QRect rect(j * boxWidth, (i * boxHeight) + headerHeight, boxWidth, boxHeight);

			if (hoveredCells.contains(QPoint(j, i))) {
				painter.fillRect(rect, hoverdBrush);
			}

			if (selectedCells.contains(QPoint(j, i))) {
				painter.fillRect(rect, selectedBrush);
			}

			if (terminalCells.contains(QPoint(j, i))) {
				painter.fillRect(rect, terminalBrush);
			}
			/*painter.setPen(QPen(Qt::gray));
			painter.drawRect(rect);*/

			painter.setPen(QPen(Qt::black));
			painter.drawText(rect, Qt::AlignCenter, days[(i * columns) + j]);
		}
	}
}

QDate CalendarWidget::getDateForCell(QPoint cell)
{
	QString day(days[(cell.y() * columns) + cell.x()]);
	return QDate(visibleMonth.year(), visibleMonth.month(), day.toInt());
}

