#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LogNotebook.h"
#include "SignInDialog.h"
#include "SignOutDialog.h"
#include <QtNetworkAuth>

class CloudDialog;
void exportToCSV(QString fileName);
bool exportToExcel(QString fileName);
class LogNotebook : public QMainWindow
{
	Q_OBJECT

public:
	LogNotebook(QWidget *parent = Q_NULLPTR);
	~LogNotebook();
	void addItem(int row, int column, QString text);


	QDate lastVolBackup;
	QString volBackupName;

	QOAuth2AuthorizationCodeFlow* dropBox;
	//QNetworkReply* reply;

	QDate lastCloudBackup; // last time data was backed up to dropbox
	QString userName;	// the user name of the person who is logged in

	int backupFrequency = 7; // how often to backup to the cloud in days

	void logIntoDB();
	void logOutofDB();
	void getLastDBBackup();
	void backupToCloud(); // backs up data to dropbox
	
private:
	Ui::LogNotebookClass ui;
	
	void resizeEvent(QResizeEvent* event);
	void updateTable();

	void clearBackups();

	QFuture<bool> future;

	QTimer* backupTimer; // checks every hour to see if it should back up localy. Back ups happen daily

	QDate lastLogBackup; // last time Log.csv was backed up
	QString logBackupName; // name of the backup file

	CloudDialog* cloudManager;

	
private slots:
	void on_signInBtn_clicked();
	void on_signOutBtn_clicked();

	void on_menuData_triggered(QAction* action);
	void on_menuOptions_triggered(QAction* action);
	void on_menuAbout_triggered(QAction* action);
	void on_menuOption_triggered(QAction* action);

	void finishedExport();

	void checkBackup();

	void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
	void uploadDone();

	void loginDone();
	void gotLastBackup();
	void gotUsrName();
	void cloudError(const QString& error, const QString& errorDescription, const QUrl& uri);

signals:
	void authenticated();
};
