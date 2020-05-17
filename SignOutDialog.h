#pragma once

#include <QDialog>
#include "ui_SignOutDialog.h"

class SignOutDialog : public QDialog
{
	Q_OBJECT

public:
	SignOutDialog(QWidget *parent = Q_NULLPTR);
	void showEvent(QShowEvent* event);
	~SignOutDialog();

	QString selPerson;
	QString selFirst;
	QString selLast;

	void signPersonOut(QString first, QString last, QString *workedToday, QString *workedTotal, QString* lastReset);
private:
	Ui::SignOutDialog ui;
private slots:
	void on_signOutBtn2_clicked();
	void on_whoAreYouCombo_currentTextChanged(const QString& text);

	void on_checkBox_stateChanged(int state);
};
