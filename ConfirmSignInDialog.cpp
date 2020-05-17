#include "ConfirmSignInDialog.h"

ConfirmSignInDialog::ConfirmSignInDialog(QWidget* parent, QString first, QString last, QString task, QString date, QString time, bool newPerson)
	: QDialog(parent)
{
	ui.setupUi(this);

	ui.firstNameLbl->setText("First Name: " + first);
	ui.lastNameLbl->setText("Last Name: " + last);
	ui.taskLbl->setText("Task: " + task);
	ui.dateLbl->setText("Date: " + date);
	ui.timeLbl->setText("Time: " + time);

	if (!newPerson) {
		ui.warningLbl->setHidden(true);
	}
	this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

ConfirmSignInDialog::~ConfirmSignInDialog()
{
}
