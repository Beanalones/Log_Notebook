#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LogNotebook.h"
#include "SignInDialog.h"
#include "SignOutDialog.h"
#include <QtNetworkAuth>
#include "RemoveNameDialog.h"

class CloudDialog;
class LogNotebook : public QMainWindow
{
	Q_OBJECT

public:
	LogNotebook(QWidget *parent = Q_NULLPTR);
	~LogNotebook();

	/*
		Cloud backup stuff
	*/
	QOAuth2AuthorizationCodeFlow* dropBox;

	QDate lastCloudBackup; // last time data was backed up to dropbox
	QString userName;	// the user name of the person who is logged in

	int backupFrequency = 7; // how often to backup to the cloud in days

	void logIntoDB();
	void logOutofDB();
	void getLastDBBackup();
	void backupToCloud(); // backs up data to dropbox

private:
	Ui::LogNotebookClass ui;

	void exportToCSV(QString fileName);
	bool exportToExcel(QString fileName);

	QFuture<bool> future;
	
	QTimer* backupTimer; // checks every hour to see if it should back up localy. Back ups happen daily

	CloudDialog* cloudManager;
	RemoveNameDialog* removeNameDlg;

	// calendar stuff
	QDate startDate;
	QDate endDate;

	// name stuff
	QString firstName;
	QString lastName;
	QString taskName;

	void refreshList();
	void checkIfDone();

public slots:
	void checkBackup();

private slots:

	void on_menuFile_triggered(QAction* action);
	void on_menuEdit_triggered(QAction* action);
	void on_menuOptions_triggered(QAction* action);
	void on_menuAbout_triggered(QAction* action);

	void finishedExport();

	void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
	void uploadDone();

	void loginDone();
	void gotLastBackup();
	void gotUsrName();
	void cloudError(const QString& error, const QString& errorDescription, const QUrl& uri);

	void newDates(Dates dates);


	void on_taskCombo_currentTextChanged(const QString& text);
	void on_search_textEdited(const QString& text);
	void on_nameList_itemSelectionChanged();
	void on_totalTime_textChanged(const QString& text);

	void on_submitBtn_released();
signals:
	void authenticated();
};
