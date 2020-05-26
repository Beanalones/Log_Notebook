#include "SignOutDialog.h"
#include "Common.h"

SignOutDialog::SignOutDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	ui.dateTimeEdit->setDateTime(QDateTime::currentDateTime());
	this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void SignOutDialog::showEvent(QShowEvent* event) {
	ui.whoAreYouCombo->clear();

	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	if(!openFile(&file, this, QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) return;
	
	bool done = false;
	QTextStream in(&file);
	QString line;
	while (!done) {
		line = in.readLine();
		if (!isSignedOut(line)) {
			QString first, last;
			getNames(&last, &first, line);
			ui.whoAreYouCombo->addItem(last + ", " + first);
		}
		if (in.atEnd()) done = true;
	}
	file.close();
}
SignOutDialog::~SignOutDialog()
{
}

void SignOutDialog::on_whoAreYouCombo_currentTextChanged(const QString& text)
{

	if (ui.whoAreYouCombo->currentText() != "") {
		ui.signOutBtn2->setEnabled(true);

		selPerson = ui.whoAreYouCombo->currentText();
		separateName(&selLast, &selFirst, text);
	}
	else {
		ui.signOutBtn2->setEnabled(false);
	}
}

void SignOutDialog::on_checkBox_stateChanged(int state)
{
	if (state) {
		ui.dateTimeEdit->setEnabled(true);
		ui.dateTimeLbl->setEnabled(true);
	}
	else {
		ui.dateTimeEdit->setEnabled(false);
		ui.dateTimeLbl->setEnabled(false);
	}
}

void SignOutDialog::on_signOutBtn2_clicked() {
	if (ui.checkBox->checkState()) {
		QMessageBox message;
		message.setText("You are attempting to sign out with a time that is not current. Do you wish to proceed?");
		message.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		if (message.exec() == QMessageBox::Cancel) return;
	}
	QString workedToday, workedTotal, lastReset;
	if (!signPersonOut(selFirst, selLast, &workedToday)) {
		QMessageBox::warning(this, "Failed to sign out", "Your exit time can not be before your entrance time");
		return;
	}
	QDateTime date = QDateTime::fromString(lastReset, "MM-dd-yyyy hh:mm");
	QMessageBox::information(this, "Thank You", "You have worked " + workedToday + " hours today.");

	this->close();
}



bool SignOutDialog::signPersonOut(QString first, QString last, QString* workedToday)
{
	QDateTime dateIn;
	QDateTime dateOut;
	/*
			
			Sign the person out of Log.csv
	
	*/

	// Open the files and make sure they can be read
	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	if (!openFile(&file, this, QFile::ReadWrite | QFile::Text | QIODevice::ExistingOnly)) return false;
	
	/*
		Sign the person out of Log.csv by adding a sign out date
	*/
	QTextStream stream(&file);

	QString line, task;
	bool done = false;
	int pos;
	int lines = 2;
	int lineNum = 0;
	QString fileStr = stream.readAll();
	stream.seek(0);
	while (!done) {
		line = stream.readLine();
		if (!isSignedOut(line)) {
			QString first, last, timeIn;
			getAll(&last, &first, &task, &timeIn, line);
			if ((selFirst == first) && (selLast == last)) {
				done = true;
				pos = stream.pos();
				lineNum = lines;
				dateIn = QDateTime::fromString(timeIn, "MM-dd-yyyy hh:mm");
			}
		}
		lines++;
		if ((stream.atEnd()) && (done == false) ) {
			done = true;
			QMessageBox::critical(this, "", "Your name could not be found in the log so you could not be signed out. Please write down your sign in and out times and tell whoever keeps the volunteer hours");
			return true;
		}
	}

	/*
		Calculate the time worked and put it at the end of the line
	*/
	stream.seek(0);
	if (ui.checkBox->checkState() == Qt::Checked) dateOut = ui.dateTimeEdit->dateTime();
	else dateOut = QDateTime::currentDateTime();

	long double diffMsec = dateIn.msecsTo(dateOut);
	long double diffHour = diffMsec / 3600000;							// The time worked in hours
	*workedToday = QString("%1").arg(diffHour, 6, 'f', 1);

	if (diffHour < 0) {
		return false;
	}
	fileStr.insert(pos - lineNum, dateOut.toString("MM-dd-yyyy hh:mm") + ',' + QString("%1").arg(diffHour, 6, 'f', 1, '0'));
	stream << fileStr;

	file.close();
	return true;
}