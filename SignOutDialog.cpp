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
	
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qDebug() << "Could not open Log.csv";
		return;
	}
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
	signPersonOut(selFirst, selLast, &workedToday, &workedTotal, &lastReset);
	QDateTime date = QDateTime::fromString(lastReset, "MM-dd-yyyy hh:mm");
	QMessageBox::information(this, "Thank You", "You have worked " + workedToday + " hours today.\n\nYou have worked " + workedTotal + " hours since " + date.toString("MM-dd-yy"));

	this->close();
}



void SignOutDialog::signPersonOut(QString first, QString last, QString* workedToday, QString* workedTotal, QString* lastReset)
{
	QDateTime dateIn;
	QDateTime dateOut;
	/*
			
			Sign the person out of Log.csv
	
	*/

	// Open the files and make sure they can be read
	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	QFile volFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Volunteers.csv");

	if (!file.open(QFile::ReadWrite | QFile::Text | QIODevice::ExistingOnly)) {
		qDebug() << "Could not open Log.csv";
		QMessageBox::warning(this, "Error", "Could not find Log.csv. A new one will be created but will not have the date from the original file.");
		file.open(QFile::WriteOnly | QFile::Text);
		
		QTextStream out(&file);
		out << "First,Last,Task,Time In,Time Out,total\n\n";
		out.flush();
	}

	if (!volFile.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) {
		qDebug() << "Could not open Volunteer.csv";
		QMessageBox msg;
		msg.setText("Could not find Volunteers.csv. Would you like to create a new one? It will not have any data.");
		msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

		if (msg.exec() == QMessageBox::No) return;

		volFile.open(QFile::WriteOnly | QFile::Text);

		QTextStream out(&volFile);

		out << "Last Reset : " + dateOut.toString("MM-dd-yyyy hh:mm") + "\n" + "Last,First,Total Time Worked Since Last Reset\n\n";
		out.flush();
	}

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
			return;
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

	fileStr.insert(pos - lineNum, dateOut.toString("MM-dd-yyyy hh:mm") + ',' + QString("%1").arg(diffHour, 6, 'f', 1, '0'));
	stream << fileStr;

	file.close();

	/*
		Volunteers.csv file
	*/
	// Get the number for the column to write to
	int taskColumn;
	if (task == "BOD") taskColumn = 2;
	else if (task == "Management") taskColumn = 3;
	else if (task == "Collections Management") taskColumn = 4;
	else if (task == "Docent") taskColumn = 5;
	else if (task == "Events") taskColumn = 6;
	else if (task == "Exhibits") taskColumn = 7;
	else if (task == "Ham Radio") taskColumn = 8;
	else if (task == "Simulator Operation") taskColumn = 9;
	else if (task == "Simulator Maintenence") taskColumn = 10;
	else if (task == "Maintenance") taskColumn = 11;
	else if (task == "Other") taskColumn = 12;
	else {
		QMessageBox::critical(this, "", "Your task does not match any known tasks. Your time will not be logged.");
		return;
	}
	/*

			Add the time worked to the persons overall time
			This is done very poorly but it works so I dont care

	*/

	QTextStream volStream(&volFile);
	QString all = volStream.readAll(); // read the entire file into a QString

	volStream.seek(0);		// go back to the begining and skip the unwanted lines
	QString firstLine = volStream.readLine();
	*lastReset = firstLine.section(" : ", 1, 1);
	volStream.readLine();
	volStream.readLine();
	
	double taskTime = 0;
	double totalTime = 0;
	while (!volStream.atEnd()) {															// Go through each line of the file
		QString line = volStream.readLine();												// and read it into a string.
		if ((line.section(",", 0, 0) == last) && (line.section(",", 1, 1) == first)) {		// if the right name is found
			taskTime = line.section(',', taskColumn, taskColumn).toDouble();				// get the time that has been worked at that task
			totalTime = line.section(',', 13, 13).toDouble();								// and the total time worked
			break;
		}
	}
	
	
	taskTime += diffHour;
	totalTime += diffHour;
	*workedTotal = QString("%1").arg(totalTime, 6, 'f', 1);

	QString str = last + ',' + first;
	int index = all.indexOf(str);											// find the name in the string
	int totalIndex = index + str.count() + 78;								// get the index of the total column
	QString strTime = QString("%1").arg(totalTime, 6, 'f', 1, '0');			// format the time as a string (double, number of characters, format, decimal places, lead with zeros)
	all.replace(totalIndex, 6, strTime);									// replace the old time with the new time

	int taskIndex = index + str.count() + ((taskColumn - 2) * 7) + 1;		//do the same but for the task time
	QString strTaskTime = QString("%1").arg(taskTime, 6, 'f', 1, '0');
	all.replace(taskIndex, 6, strTaskTime);

	volStream.seek(0);
	volStream << all;

	volFile.close();

}