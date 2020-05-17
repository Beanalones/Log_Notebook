#pragma once

#include <QDialog>
#include "ui_AddNameDialog.h"

class AddNameDialog : public QDialog
{
	Q_OBJECT

public:
	AddNameDialog(QWidget *parent = Q_NULLPTR);
	~AddNameDialog();

	QString first, last;
private:
	Ui::AddNameDialog ui;
private slots:
	void on_okAddName_clicked();
	void on_firstNameEdit_textEdited(const QString& text);
	void on_lastNameEdit_textEdited(const QString& text);
};
