#include "Common.h"

Line::Line(QString line) {
	lastName = line.section(",", 0, 0);
	firstName = line.section(",", 1, 1);
	task = Tasks(getTaskNum(line.section(",", 2, 2)));
	timeIn = QDateTime::fromString(line.section(",", 3, 3), "MM-dd-yyyy hh:mm");
	timeOut = QDateTime::fromString(line.section(",", 4, 4), "MM-dd-yyyy hh:mm");
	totalTime = line.section(",", 5, 5).toDouble();
	isSignedOut = !line.endsWith(",");
}

void separateName(QString* last, QString* first, QString full)
{
	*last = full.section(", ", 0, 0);
	*first = full.section(", ", 1, 1);
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

int getTaskNum(QString task) {
	int taskColumn;
	if (task == "Docent") taskColumn = 2;
	else if (task == "Simulator Operation") taskColumn = 3;
	else if (task == "Management") taskColumn = 4;
	else if (task == "Collections Management") taskColumn = 5;
	else if (task == "Maintenance") taskColumn = 6;
	else if (task == "Events") taskColumn = 7;
	else if (task == "Exhibits") taskColumn = 8;
	else if (task == "Simulator Maintenance") taskColumn = 9;
	else if (task == "Ham Radio") taskColumn = 10;
	else if (task == "BOD") taskColumn = 11;
	else if (task == "Other") taskColumn = 12;
	else {
		taskColumn = -1;
		qDebug() << "Could not find the task " + task;
	}
	return taskColumn;
}
/*
if (task == "BOD") taskColumn = 2;
	else if (task == "Management") taskColumn = 3;
	else if (task == "Collections Management") taskColumn = 4;
	else if (task == "Docent") taskColumn = 5;
	else if (task == "Events") taskColumn = 6;
	else if (task == "Exhibits") taskColumn = 7;
	else if (task == "Ham Radio") taskColumn = 8;
	else if (task == "Simulator Operation") taskColumn = 9;
	else if (task == "Simulator Maintenance") taskColumn = 10;
	else if (task == "Maintenance") taskColumn = 11;
	else if (task == "Other") taskColumn = 12;


	Docent = 2,
	SimulatorOp = 3,
	Management = 4,
	Collections = 5,
	Maintenence = 6,
	Events = 7,
	Exhibits = 8,
	SimulatorMaint = 9,
	HamRadio = 10,
	BOD = 11,
	Other = 12
*/

void clearLog() {
	/*
			Clear Log.csv
	*/
	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	file.remove();
	file.open(QFile::WriteOnly | QFile::Text);

	QTextStream out(&file);
	out << "Last Reset : " + QDateTime::currentDateTime().toString("MM-dd-yyyy hh:mm") + "\n";
	out << "Last,First,Task,Time In,Time Out,Total\n\n";
	file.close();
}

bool openFile(QFile* file, QWidget* widget, QFile::OpenMode mode)
{
	if (!file->open(mode)) {
		QString message;
		if (file->fileName().contains("Log.csv")) message = "Could not find Log.csv. Would you like to restore from the last backup on " + lastLogBackup.toString("MM-dd-yyyy") + "?";
		else if (file->fileName().contains("Volunteers.csv")) message = "Could not open Volunteers.csv. Would you like to restore from the last backup on " + lastLogBackup.toString("MM-dd-yyyy") + "?";

		if (QMessageBox::warning(widget, "Error", message,
			QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::No) return false;
		if (!restoreFromBackup(file)) {
			QMessageBox::critical(widget, "Error", "No backup found. Cannot restore.");
			return false;
		}
		file->open(mode);
	}
	return true;
}

bool restoreFromBackup(QFile* file)
{
	file->remove();
	if (file->fileName().contains("Log.csv")) {
		return QFile::copy(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/LogBackup/" + logBackupName, file->fileName());
	}
	else if (file->fileName().contains("Volunteers.csv")) {
		return QFile::copy(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/VolunteersBackup/" + volBackupName, file->fileName());
	}
	return false;
}