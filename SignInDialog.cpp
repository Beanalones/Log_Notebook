#include "SignInDialog.h"
#include "ConfirmSignInDialog.h"
#include "Common.h"
#include <iostream>
#include <fstream>

SignInDialog::SignInDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	
	ui.dateTimeEdit->setDateTime(QDateTime::currentDateTime());
	this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui.taskCombo->setCurrentIndex(-1);
}

SignInDialog::~SignInDialog()
{
}


void SignInDialog::refreshList()
{
	ui.nameList->clear();

	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Volunteers.csv");

	if (!file.open(QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) { // Open the file as read only, a text document
		qDebug() << "Could not open Volunteers.csv";
		QMessageBox::warning(this, "Error", "Could not open Volunteers.csv. This operation is not critical but the file is required for operation.");
		return;
	}

	QTextStream in(&file);

	in.readLine();
	in.readLine();
	in.readLine();

	while (!in.atEnd()) {
		QString line = in.readLine();
		QString last = line.section(",", 0, 0);
		QString first = line.section(",", 1, 1);
		ui.nameList->addItem(last + ", " + first);
	}
	ui.nameList->sortItems();
}

void SignInDialog::checkIfDone()
{
	if ((firstName != "") && (lastName != "") && (taskName != "")) {
		ui.signInBtn2->setEnabled(true);
	}
	else {
		ui.signInBtn2->setEnabled(false);
	}
}

void SignInDialog::showEvent(QShowEvent* event)
{
	refreshList();
}


void SignInDialog::on_taskCombo_currentTextChanged(const QString& text)
{
	taskName = ui.taskCombo->currentText();
	
	checkIfDone();

	ui.taskEdit->setText(taskName);
}

void SignInDialog::on_search_textEdited(const QString& text)
{
	for (int i = 0; i < ui.nameList->count(); i++){
		ui.nameList->item(i)->setHidden(false);			// unhide all items
	}

	for (int i = 0; i < ui.nameList->count(); i++)
	{
		QListWidgetItem* current = ui.nameList->item(i);
		QString last, first;
		separateName(&last, &first, current->text());
		if ((!last.startsWith(text, Qt::CaseInsensitive)) && (!first.startsWith(text, Qt::CaseInsensitive)))
		{
			current->setHidden(true);
			if (current->isSelected()) {
				current->setSelected(false);
				ui.firstEdit->setText("");
				ui.lastEdit->setText("");
			}
		}
	}
}

void SignInDialog::on_nameList_itemSelectionChanged()
{
	QListWidgetItem* current = ui.nameList->currentItem();
	QString text = current->text();
	lastName = text.section(", ", 0, 0);
	firstName = text.section(", ", 1, 1);

	checkIfDone();

	ui.firstEdit->setText(firstName);
	ui.lastEdit->setText(lastName);
}

void SignInDialog::on_addNameBtn_clicked()
{
	AddNameDialog addNameDlg;
	if (addNameDlg.exec() == QDialog::Accepted) {
		// user clicked ok
		refreshList();
	}
}

void SignInDialog::on_dateCheck_stateChanged(int state)
{
	if (state) ui.dateTimeEdit->setEnabled(true);
	else {
		ui.dateTimeEdit->setEnabled(false);
		ui.dateTimeEdit->setDateTime(QDateTime::currentDateTime());
	}
}

void SignInDialog::on_signInBtn2_clicked() {
	
	ConfirmSignInDialog confirm(this, firstName, lastName, taskName, ui.dateTimeEdit->dateTime().toString("MM-dd-yyyy"), ui.dateTimeEdit->dateTime().toString("hh:mm"), false);
	if (confirm.exec() == QDialog::Rejected) {
		return;
	}
	
	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	
	if (!file.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) { // Open the file as write only, a text document
		qDebug() << "Could not open Log.csv";
		QMessageBox::warning(this, "Error", "Could not find Log.csv. A new one will be created but will not have the date from the original file.");
		file.open(QFile::WriteOnly | QFile::Text);

		QTextStream out(&file);
		out << "First,Last,Task,Time In,Time Out\n\n";
		out.flush();
	}

	QTextStream out(&file);

	while (!out.atEnd()) {				// make sure people cant sign in twice
		QString line = out.readLine();
		QString first, last, task, timeIn;
		getAll(&last, &first, &task, &timeIn, line);

		if ((first == firstName) && (last == lastName) && (!isSignedOut(line))) {
			QMessageBox::warning(this, "Problem", "You are still signed in from " + timeIn + ". Please sign out before signing back in. Make sure to change the time when signing out to the time when you actualy left.");
			return;
		}
	}

	file.readAll(); // go to the end of the file

	out << lastName << ',' << firstName << ',' << taskName << ',' <<
		QDateTime::currentDateTime().toString("MM-dd-yyyy hh:mm") << ',' << '\n';

	file.flush();
	file.close();

	this->close();
}