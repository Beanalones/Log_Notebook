#pragma once

#include <QDialog>
#include "ui_RemoveNameDialog.h"
#include "AddNameDialog.h"

class RemoveNameDialog : public QDialog
{
	Q_OBJECT

public:
	RemoveNameDialog(QWidget *parent = Q_NULLPTR);
	~RemoveNameDialog();

	void showEvent(QShowEvent* event);

private:
	Ui::RemoveNameDialog ui;

	void refreshList();
private slots:

	void on_addNameBtn_clicked();
	void on_removeNameBtn_clicked();
	void on_listWidget_itemSelectionChanged();
	void on_search_textEdited(const QString& text);
};
