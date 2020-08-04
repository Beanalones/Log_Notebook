#pragma once

#include <QDialog>
#include "ui_SignInDialog.h"
#include "AddNameDialog.h"

class SignInDialog : public QDialog, public Ui::SignInDialog
{
	Q_OBJECT

public:
	SignInDialog(QWidget *parent = Q_NULLPTR);
	~SignInDialog();

	QString firstName;
	QString lastName;
	QString taskName;	

	void refreshList();
	void checkIfDone();
private:
	Ui::SignInDialog ui;
	void showEvent(QShowEvent* event);

private slots:
	void on_signInBtn2_clicked();
	void on_taskCombo_currentTextChanged(const QString& text);
	void on_search_textEdited(const QString& text);
	void on_nameList_itemSelectionChanged();
	void on_addNameBtn_clicked();

	void on_dateCheck_stateChanged(int state);
};
