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
	if(!openFile(&file, this, QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) return;
	
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

	if (autoFillTask) {
		QFile logFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
		if (!openFile(&logFile, this, QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) return;
		QTextStream in(&logFile);
		QString all = in.readAll();
		logFile.close();
		QString fullName = lastName + "," + firstName;
		int index = all.lastIndexOf(fullName);
		if (index == -1) {
			ui.taskCombo->setCurrentIndex(-1);
			return;
		}
		index += fullName.length() + 1;
		QString str;
		for(index; index < all.length();index++) {
			if (all.at(index) != ",") {
				str += all.at(index);
			}
			else {
				break;
			}
		}
		ui.taskCombo->setCurrentIndex(getTaskNum(str) - 2);
	}
}

void SignInDialog::on_addNameBtn_clicked()
{
	AddNameDialog addNameDlg;
	if (addNameDlg.exec() == QDialog::Accepted) {
		// user clicked ok
		refreshList();

		checkIfDone();

		on_search_textEdited(addNameDlg.last);
		for (int i = 0; i < ui.nameList->count(); i++)
		{
			QListWidgetItem* current = ui.nameList->item(i);
			QString last, first;
			separateName(&last, &first, current->text());
			if ((last == addNameDlg.last) && (first == addNameDlg.first))
			{
				ui.nameList->setCurrentItem(current);
			}
		}
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
	if(!openFile(&file, this, QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) return;
	
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