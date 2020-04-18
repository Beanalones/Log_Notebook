#include "Log_Notebook.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Log_Notebook w;
	w.show();
	return a.exec();
}
