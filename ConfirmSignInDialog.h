#pragma once

#include <QDialog>
#include "ui_ConfirmSignInDialog.h"

class ConfirmSignInDialog : public QDialog
{
	Q_OBJECT

public:
	ConfirmSignInDialog(QWidget *parent = Q_NULLPTR, QString first = "", QString last = "", QString task = "", QString date = "", QString time = "", bool newPerson = false);
	~ConfirmSignInDialog();

private:
	Ui::ConfirmSignInDialog ui;
};
