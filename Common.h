#pragma once

#include <QtWidgets>


enum Tasks {
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
};
/*
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
struct Line {
	QString lastName;
	QString firstName;
	Tasks task;
	QDateTime timeIn;
	QDateTime timeOut;
	double totalTime;
	bool isSignedOut;

	Line(QString line);
};

// takes a full name such as "Falco, Ben" and breaks it into "Ben" and "Falco"
void separateName(QString* last, QString* first, QString full);

// gets everything a signed in person can offer
void getAll(QString* first, QString* last, QString* task, QString* timeIn, QString line);

// takes a full line of the .csv and gets the first and last names
void getNames(QString* first, QString* last, QString line);

// determines whether or not a line in the .csv is signed out
bool isSignedOut(QString line);

int getTaskNum(QString task);

// Cleares the log file. Also used to replace it if missing.
void clearLog();

extern QDate lastLogBackup; // last time Log.csv was backed up
extern QString logBackupName; // name of the backup file

extern QDate lastVolBackup; // last time Volunteers.csv was backed up
extern QString volBackupName; // name of the backup file

extern bool autoFillTask; // whether or not to automatically fill the task when signing in based on the last task used.

bool openFile(QFile* file, QWidget* widget, QFile::OpenMode mode);


// copies the log file from the last backup into a new one
bool restoreFromBackup(QFile* file);