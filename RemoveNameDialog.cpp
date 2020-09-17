#include "RemoveNameDialog.h"
#include "Common.h"

RemoveNameDialog::RemoveNameDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

RemoveNameDialog::~RemoveNameDialog()
{
}

void RemoveNameDialog::showEvent(QShowEvent* event)
{
	refreshList();
}

void RemoveNameDialog::on_addNameBtn_clicked()
{
	AddNameDialog addNameDlg;
	if (addNameDlg.exec() == QDialog::Accepted) {
		refreshList();
	}
}

void RemoveNameDialog::refreshList()
{
	ui.listWidget->clear();

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
		ui.listWidget->addItem(last + ", " + first);
	}
	ui.listWidget->sortItems();
	emit namesChanged();
}

void RemoveNameDialog::on_removeNameBtn_clicked()
{
	
	QListWidgetItem* current = ui.listWidget->currentItem();
	QString shownName = current->text();
	QString name = shownName.section(", ", 0, 0) + "," + shownName.section(", ", 1, 1);

	if (QMessageBox::warning(this, "", "Are you sure you want to remove " + shownName + "? This action is permanent.", QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Cancel) == QMessageBox::Cancel) return;

	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Volunteers.csv");
	if(!openFile(&file, this, QFile::ReadWrite | QFile::Text | QFile::ExistingOnly)) return;
	
	QTextStream stream(&file);

	QString fileStr = stream.readAll();
	stream.seek(0);

	fileStr.remove(name + "\n");
	file.resize(0);
	stream << fileStr;

	file.close();
	refreshList();
}

void RemoveNameDialog::on_listWidget_itemSelectionChanged()
{
	if (ui.listWidget->currentItem() == Q_NULLPTR) {
		ui.removeNameBtn->setEnabled(false);
	}
	else {
		ui.removeNameBtn->setEnabled(true);
	}

}

void RemoveNameDialog::on_search_textEdited(const QString& text)
{
	for (int i = 0; i < ui.listWidget->count(); i++) {
		ui.listWidget->item(i)->setHidden(false);			// unhide all items
	}

	for (int i = 0; i < ui.listWidget->count(); i++)
	{
		QListWidgetItem* current = ui.listWidget->item(i);
		QString last, first;
		separateName(&last, &first, current->text());
		if ((!last.startsWith(text, Qt::CaseInsensitive)) && (!first.startsWith(text, Qt::CaseInsensitive)))
		{
			current->setHidden(true);
			if (current->isSelected()) {
				current->setSelected(false);
			}
		}
	}
}
