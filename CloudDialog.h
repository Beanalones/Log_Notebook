#pragma once

#include <QDialog>
#include "ui_CloudDialog.h"
#include "LogNotebook.h"

class CloudDialog : public QDialog
{
	Q_OBJECT

public:
	CloudDialog(LogNotebook* main, QWidget *parent = Q_NULLPTR);
	~CloudDialog();

	void setLatestBackup(QDate date);
	void setLoggedIn(bool loggedIn);
	void setUserName(QString name);
	void setBackupFrequency(int frequency);
	void writeLine(QString line);

private:
	Ui::CloudDialog ui;
	LogNotebook* notebook;

private slots:

	void on_logInBtn_clicked();
	void on_backupBtn_clicked();
	void on_refreshBtn_clicked();
	void on_frequencySpin_valueChanged(int i);
};
