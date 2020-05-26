#include "CloudDialog.h"

CloudDialog::CloudDialog(LogNotebook* main, QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	notebook = main;
}

CloudDialog::~CloudDialog()
{
}

void CloudDialog::setLatestBackup(QDate date)
{
	if (date == QDate(1970, 1, 1)) {
		ui.lastBackupLbl->setText("No Backups");
	}
	else {
		ui.lastBackupLbl->setText(date.toString("MM-dd-yyyy"));
	}
}

void CloudDialog::setLoggedIn(bool loggedIn)
{
	if (loggedIn) {
		ui.statusLbl->setText("Logged In");
		ui.logInBtn->setText("Log out");
		ui.backupBtn->setEnabled(true);
		ui.refreshBtn->setEnabled(true);
	}
	else {
		ui.statusLbl->setText("Logged Out");
		ui.logInBtn->setText("Log In");
		ui.backupBtn->setEnabled(false);
		ui.refreshBtn->setEnabled(false);
	}
}

void CloudDialog::setUserName(QString name)
{
	ui.userNameLbl->setText(name);
}

void CloudDialog::setBackupFrequency(int frequency)
{
	ui.frequencySpin->setValue(frequency);
}

void CloudDialog::writeLine(QString line)
{
	ui.textBrowser->append(line);
}


void CloudDialog::on_backupBtn_clicked()
{
	if (notebook->dropBox->token() == Q_NULLPTR) {
		ui.textBrowser->append("You must sign into Dropbox before backing up to it.");
		return;
	}
	notebook->backupToCloud();
}

void CloudDialog::on_refreshBtn_clicked()
{
	if (notebook->dropBox->token() == Q_NULLPTR) {
		ui.textBrowser->append("You must sign into Dropbox before refreshing backups up to it.");
		return;
	}
	notebook->getLastDBBackup();
}

void CloudDialog::on_frequencySpin_valueChanged(int i)
{
	notebook->backupFrequency = i;
	QSettings settings;
	settings.setValue("backupFrequency", i);
	notebook->checkBackup();
}

void CloudDialog::on_logInBtn_clicked() {
	if (ui.logInBtn->text() == "Log In") notebook->logIntoDB();
	else notebook->logOutofDB();
}