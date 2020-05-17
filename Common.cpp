#include "Common.h"

void separateName(QString* first, QString* last, QString full)
{
	*first = full.section(", ", 0, 0);
	*last = full.section(", ", 1, 1);
}

void getAll(QString* first, QString* last, QString* task, QString* timeIn, QString line) {
	*first = line.section(",", 0, 0);
	*last = line.section(",", 1, 1);
	*task = line.section(",", 2, 2);
	*timeIn = line.section(",", 3, 3);
}
void getNames(QString* first, QString* last, QString line)
{
	*first = line.section(",", 0, 0);
	*last = line.section(",", 1, 1);
}

bool isSignedOut(QString line)
{
	return !line.endsWith(",");
}

QString copyFile(QString fileName)
{
	QFile file(fileName);

	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qDebug() << "Could not open file";
		return QString();
	}

	QTextStream in(&file);
	QString read = in.readAll();
	file.close();
	
	return read;
}


