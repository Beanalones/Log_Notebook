#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Log_Notebook.h"

class Log_Notebook : public QMainWindow
{
	Q_OBJECT

public:
	Log_Notebook(QWidget *parent = Q_NULLPTR);

private:
	Ui::Log_NotebookClass ui;
};
