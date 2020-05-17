#include "AddNameDialog.h"
#include "Common.h"

AddNameDialog::AddNameDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
}

AddNameDialog::~AddNameDialog()
{
}

void AddNameDialog::on_firstNameEdit_textEdited(const QString& text)
{
	first = text;
	if ((first != "") && (last != "")) {
		ui.okAddName->setEnabled(true);
	}
	else {
		ui.okAddName->setEnabled(false);
	}
}

void AddNameDialog::on_lastNameEdit_textEdited(const QString& text)
{
	last = text;
	if ((first != "") && (last != "")) {
		ui.okAddName->setEnabled(true);
	}
	else {
		ui.okAddName->setEnabled(false);
	}
}


void AddNameDialog::on_okAddName_clicked() {

	if ((first.contains(',')) ||
		(last.contains(','))) {
		QMessageBox::warning(this, "", "The names can not contain spaces or commas.");
		return;
	}

	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" +  "Volunteers.csv");

	if (!file.open(QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) { // Open the file as write only, a text document, and go to the end
		qDebug() << "Could not open Log.csv";
		QErrorMessage errorDialog;
		errorDialog.showMessage("Could not open Volunteer.csv");
		return;
	}
	
	QTextStream out(&file);
	QString all = out.readAll();

	if (all.contains(last + ',' + first)) {
		QMessageBox::information(this, "", "This name already exists.");
		return;
	}

	out << last << ',' << first;
	for (int i = 0; i < 12; i++) {
		out << ",0000.0";
	}
	out << '\n';
	out.flush();

	this->accept();
}