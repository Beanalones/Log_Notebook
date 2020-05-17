#include "LogNotebook.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setObjectName("OrganizationName");
	a.setOrganizationDomain("organizationDomain.com");
	a.setApplicationDisplayName("Log Notebook");
	LogNotebook w;
	w.show();
	return a.exec();
}
