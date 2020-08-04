#include "LogNotebook.h"
#include "Common.h"
#include "AboutDialog.h"

#include <ActiveQt/qaxobject.h>
#include <ActiveQt/qaxbase.h>
#include <ActiveQt/qaxfactory.h>
#include <QtConcurrent/qtconcurrentrunbase.h>
#include <QtConcurrent/qtconcurrentrun.h>

#include <QtNetworkAuth>
#include <QtNetwork>
#include "CloudDialog.h"

#include "CalendarWidget.h"

QDate lastLogBackup; // last time Log.csv was backed up
QString logBackupName; // name of the backup file

QDate lastVolBackup; // last time Volunteers.csv was backed up
QString volBackupName; // name of the backup file

bool autoFillTask;

LogNotebook::LogNotebook(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	/*
		Get the last time that data was backed up localy
	*/
	QDir logDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/LogBackup/");
	if (!logDir.exists()) {
		logDir.mkdir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/LogBackup/");
	}
	auto logList = logDir.entryList(QDir::Files);
	if (logList.count() != 0) {
		logBackupName = logList[0];
		QString log = logBackupName.section("_", 1, 1);
		log = log.section(".", 0, 0);
		lastLogBackup = QDate::fromString(log, "MM-dd-yyyy");
	}
	else {
		logBackupName = "";
		lastLogBackup = QDate(1970, 1, 1);
	}

	QDir volDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/VolunteersBackup/");
	if (!volDir.exists()) {
		volDir.mkdir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/VolunteersBackup/");
	}
	auto volList = volDir.entryList(QDir::Files);
	if (volList.count() != 0) {
		volBackupName = volList[0];
		QString vol = volBackupName.section("_", 1, 1);
		vol = vol.section(".", 0, 0);
		lastVolBackup = QDate::fromString(vol, "MM-dd-yyyy");
	}
	else {
		volBackupName = "";
		lastVolBackup = QDate(1970, 1, 1);
	}

	/*
		check if backups are needed and set a timer for every hour to check again
	*/
	checkBackup();
	backupTimer = new QTimer(this);
	backupTimer->setTimerType(Qt::VeryCoarseTimer);
	QObject::connect(backupTimer, SIGNAL(timeout()), this, SLOT(checkBackup()));
	backupTimer->start(3600000); //time specified in ms

	// set up stuff for cloud backups
	cloudManager = new CloudDialog(this);
	dropBox = new QOAuth2AuthorizationCodeFlow;

	QSettings settings;
	backupFrequency = settings.value("backupFrequency").toInt();
	dropBox->setToken(settings.value("token").toString());
	userName = settings.value("userName").toString();

	cloudManager->setBackupFrequency(backupFrequency);
	cloudManager->setUserName(userName);
	if (dropBox->token() != Q_NULLPTR) {
		cloudManager->setLoggedIn(true);
		getLastDBBackup();
	}
	removeNameDlg = new RemoveNameDialog;

	/*
		Settings
	*/
	autoFillTask = settings.value("autoFillTask").toBool();
	ui.autoFillTask->setChecked(autoFillTask);

	connect(ui.calendar, SIGNAL(datesChanged(Dates)), this, SLOT(newDates(Dates)));
	//connect(calendar, SIGNAL(datesChanged(Dates)), // same sender and signal
	//	this,                 // context object to break this connection
	//	[this]() {            // debug output
	//		qDebug() << "Direct?" << QThread::currentThread() == this->thread();
	//	},
	//	Qt::DirectConnection);    // see below
	ui.taskCombo->setCurrentIndex(-1);
	ui.totalTime->setValidator(new QDoubleValidator(0,9,2));
	refreshList();
	//calendar->dumpObjectInfo();
	//QWidget* central = new QWidget;
	//QVBoxLayout* vBox = new QVBoxLayout;
	//vBox->addSpacing(100);
	//vBox->addWidget(calendar);
	//vBox->addSpacing(100);
	//central->setLayout(vBox);

	//setCentralWidget(central);
}

LogNotebook::~LogNotebook()
{
	QSettings settings;
	settings.setValue("backupFrequency", backupFrequency);
	settings.setValue("token", dropBox->token());
	settings.setValue("userName", userName);
	settings.setValue("autoFillTask", autoFillTask);
	delete removeNameDlg;
}

/*
	CALENDAR STUFF
*/
void LogNotebook::newDates(Dates dates)
{
	if (dates.count() == 0) return;
	startDate = dates.first();
	if (dates.count() < 2) {
		endDate = startDate;
	}
	else {
		endDate = dates.last();
	}
	
	ui.startDateLbl->setText("From: " + startDate.toString("MMM d, yyyy"));
	ui.endDateLbl->setText("To: " + endDate.toString("MMM d, yyyy"));
	checkIfDone();
}


void LogNotebook::refreshList()
{
	ui.nameList->clear();

	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Volunteers.csv");
	if (!openFile(&file, this, QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) return;

	QTextStream in(&file);

	in.readLine();
	in.readLine();
	in.readLine();

	while (!in.atEnd()) {
		QString line = in.readLine();
		QString last = line.section(",", 0, 0);
		QString first = line.section(",", 1, 1);
		ui.nameList->addItem(last + ", " + first);
	}
	ui.nameList->sortItems();
}

void LogNotebook::checkIfDone()
{
	if ((firstName != "") && (lastName != "") && (taskName != "") && (startDate != QDate()) && (endDate != QDate()) && (ui.totalTime->text() != "")) {
		ui.submitBtn->setEnabled(true);
	}
	else {
		ui.submitBtn->setEnabled(false);
	}
}

void LogNotebook::on_taskCombo_currentTextChanged(const QString& text)
{
	taskName = ui.taskCombo->currentText();

	checkIfDone();

	//ui.taskEdit->setText(taskName);
}

void LogNotebook::on_search_textEdited(const QString& text)
{
	for (int i = 0; i < ui.nameList->count(); i++) {
		ui.nameList->item(i)->setHidden(false);			// unhide all items
	}

	for (int i = 0; i < ui.nameList->count(); i++)
	{
		QListWidgetItem* current = ui.nameList->item(i);
		QString last, first;
		separateName(&last, &first, current->text());
		if ((!last.startsWith(text, Qt::CaseInsensitive)) && (!first.startsWith(text, Qt::CaseInsensitive)))
		{
			current->setHidden(true);
			if (current->isSelected()) {
				current->setSelected(false);
				//ui.firstEdit->setText("");
				//ui.lastEdit->setText("");
			}
		}
	}
}

void LogNotebook::on_nameList_itemSelectionChanged()
{
	QListWidgetItem* current = ui.nameList->currentItem();
	QString text = current->text();
	lastName = text.section(", ", 0, 0);
	firstName = text.section(", ", 1, 1);

	checkIfDone();

	//ui.firstEdit->setText(firstName);
	//ui.lastEdit->setText(lastName);

	if (autoFillTask) {
		QFile logFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
		if (!openFile(&logFile, this, QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) return;
		QTextStream in(&logFile);
		QString all = in.readAll();
		logFile.close();
		QString fullName = lastName + "," + firstName;
		int index = all.lastIndexOf(fullName);
		if (index == -1) {
			ui.taskCombo->setCurrentIndex(-1);
			return;
		}
		index += fullName.length() + 1;
		QString str;
		for (index; index < all.length(); index++) {
			if (all.at(index) != ",") {
				str += all.at(index);
			}
			else {
				break;
			}
		}
		ui.taskCombo->setCurrentIndex(getTaskNum(str) - 2);
	}
}

void LogNotebook::on_totalTime_textChanged(const QString& text)
{
	checkIfDone();
}

void LogNotebook::on_submitBtn_released() {
	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	if (!openFile(&file, this, QFile::ReadWrite | QFile::Text | QFile::ExistingOnly | QFile::Append)) return;

	QTextStream out(&file);

	out << lastName << ',' << firstName << ',' << taskName << ',' <<
		startDate.toString("MM-dd-yyyy") << ',' << endDate.toString("MM-dd-yyyy") << ',' <<
		ui.totalTime->text() << '\n';

	file.flush();
	file.close();
	ui.taskCombo->setCurrentIndex(-1);
	ui.nameList->clearSelection();
	ui.search->setText("");
	ui.totalTime->setText("");
	ui.calendar->clearSelected();

}
/*
*
*
*	EXPORTING
*
*/
QProgressDialog* progress;
void LogNotebook::on_menuFile_triggered(QAction* action)
{
	if (action->objectName() == "exportExcel") {
		QString fileName = QFileDialog::getSaveFileName(this,
			tr("Export Data"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
			tr("Excel Workbook (*.xlsx)"));

		if (fileName.isEmpty()) return;

		QFile logFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
		if (!openFile(&logFile, this, QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) return;

		progress = new QProgressDialog(this);
		progress->setWindowModality(Qt::WindowModal);
		progress->setLabelText("Exporting to Excel...");
		progress->setCancelButton(0);
		progress->setRange(0, 0);
		progress->setMinimumDuration(0);
		progress->show();

		future = QtConcurrent::run(this,&LogNotebook::exportToExcel, fileName);
		QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
		connect(watcher, SIGNAL(finished()), this, SLOT(finishedExport()));
		// delete the watcher when finished too
		connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));
		watcher->setFuture(future);
	}
	else if (action->objectName() == "exportCSV") {
		QString fileName = QFileDialog::getSaveFileName(this,
			tr("Export Data"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
			tr("Comma Separated Values (*.csv)"));

		if (fileName.isEmpty()) return;

		exportToCSV(fileName);
	}
}

struct Person {
	QString lastName;
	QString firstName;
	double times[11] = { 0,0,0,0,0,0,0,0,0,0,0 };
	double getTotal() {
		int n = 0;
		for (int i = 0; i < 11; i++) {
			n += times[i];
		}
		return n;
	}
};
void LogNotebook::exportToCSV(QString fileName) {
	QFile logFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	if(!openFile(&logFile, this, QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) return;

	QTextStream in(&logFile);
	QString all = in.readAll();
	in.seek(0);
	in.readLine();
	in.readLine();
	in.readLine();
	QMap<QString, Person> map;
	while (!in.atEnd()) {
		Line line(in.readLine());
		if (line.isSignedOut) {
			if (map.contains(line.lastName + line.firstName)) {
				map[line.lastName + line.firstName].times[line.task - 2] += line.totalTime;
			}
			else {
				Person dude;
				dude.lastName = line.lastName;
				dude.firstName = line.firstName;
				dude.times[line.task - 2] = line.totalTime;
				map.insert(line.lastName + line.firstName, dude);
			}
		}
	}
	logFile.close();

	all.remove(0, 13);
	all.insert(0, "From: ");
	all.insert(22, "\nTo: " + QDateTime::currentDateTime().toString("MM-dd-yyyy hh:mm") + "\n");
	QFile newFile(fileName);

	newFile.open(QFile::WriteOnly | QFile::Text);
	QTextStream out(&newFile);

	int lines = all.count("\n");
	
	QString volTxt = "\n\n\nLast,First,Docent,Simulator Operation,Management,Collections Management,Maintenance,Events,Exhibits,Simulator Maintenance,Ham Radio,BOD,Other,Total\n\n";
	QMap<QString, Person>::const_iterator i = map.constBegin();
	while (i != map.constEnd()) {
		volTxt += i.value().lastName + "," + i.value().firstName + ",";
		for (int j = 0; j < 11; j++) {
			volTxt += QString::number(i.value().times[j]) + ",";
		}
		double n = 0;
		for (int u = 0; u < 11; u++) {
			n += i.value().times[u];
		}
		volTxt += QString::number(n);
		volTxt += "\n";
		++i;
	}
	
	for (int i = 0; i < lines; i++) {
		QString log = all.section("\n", i,i);
		QString vol = volTxt.section("\n", i, i);

		if (log != "") {
			if (isSignedOut(log)) {
				out << log + ",,";
			}
			else {
				out << log + ",,,";
			}

		}
		else {
			out << ",,,,,,,";
		}

		if (vol != "") {
			out << vol + "\n";
		}
		else {
			out << ",,,,,,,,,,,,,\n";
		}

	}
	newFile.close();
}



bool LogNotebook::exportToExcel(QString fileName){
	fileName.replace("/", "\\");

	
	/*
		Setup the excel spread sheet
	*/
	QAxObject* excel = new QAxObject("Excel.Application", 0);
	
	excel->dynamicCall("SetVisible(bool)", false);
	excel->setProperty("DisplayAlerts", false);//Show warning to see effect

	QAxObject* workbooks = excel->querySubObject("Workbooks");
	if (workbooks == Q_NULLPTR) return false;
	QAxObject* workbook = workbooks->querySubObject("Add");
	QAxObject* worksheet = workbook->querySubObject("Sheets(int)", 1);     //Get Form 1

	int topPaddingLines = 5; // how far down from the top to start writing

	/*
		Read and Parse the log file
	*/
	QFile logFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	if (!logFile.open(QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) {

	}

	QTextStream in(&logFile);
	QString all = in.readAll();
	in.seek(0);
	QString lastReset = all.section("\n", 0, 0);
	lastReset = lastReset.section(" : ", 1, 1);
	in.seek(0);
	in.readLine();
	in.readLine();
	in.readLine();
	QMap<QString, Person> map;
	while (!in.atEnd()) {
		Line line(in.readLine());
		if (line.isSignedOut) {
			if (map.contains(line.lastName + line.firstName)) {
				map[line.lastName + line.firstName].times[line.task - 2] += line.totalTime;
			}
			else {
				Person dude;
				dude.lastName = line.lastName;
				dude.firstName = line.firstName;
				dude.times[line.task - 2] = line.totalTime;
				map.insert(line.lastName + line.firstName, dude);
			}
		}
	}
	logFile.close();

	/*
		Create a string for the right of the sheet
	*/
	QString volTxt = "\nLast,First,Docent,Simulator Operation,Management,Collections Management,Maintenance,Events,Exhibits,Simulator Maintenance,Ham Radio,BOD,Other,Total\n";
	QMap<QString, Person>::const_iterator i = map.constBegin();
	int line = topPaddingLines + 2;
	while (i != map.constEnd()) {
		volTxt += i.value().lastName + "," + i.value().firstName + ",";
		for (int j = 0; j < 11; j++) {
			volTxt += QString::number(i.value().times[j]) + ",";
		}

		QString lineStr = QString::number(line);
		volTxt += "=SUM(L" + lineStr + ":" + "V" + lineStr + ")\n";
		line++;
		++i;
	}

	/*
		Write the log file to the left of the screen
	*/
	all.remove(69,1);
	QString tempLog;
	int currLine = 0;
	int currColumn = 1;
	for (int i = 29; i < all.length(); i++) {
		if (all.at(i) == "\n") {
			QAxObject* cell = worksheet->querySubObject("Cells(int,int)", currLine + topPaddingLines, currColumn);
			cell->setProperty("Value", tempLog);
			tempLog = "";
			currLine += 1;
			currColumn = 1;
		}
		else if (all.at(i) == ",") {
			QAxObject* cell = worksheet->querySubObject("Cells(int,int)", currLine + topPaddingLines, currColumn);
			cell->setProperty("Value", tempLog);
			tempLog = "";
			currColumn += 1;
		}
		else {
			tempLog += all.at(i);
		}
	}

	tempLog = "";
	currLine = 0;
	currColumn = 10;
	for (int i = 0; i < volTxt.length(); i++) {
		if (volTxt.at(i) == "\n") {
			QAxObject* cell = worksheet->querySubObject("Cells(int,int)", currLine + topPaddingLines, currColumn);
			cell->setProperty("Value", tempLog);
			tempLog = "";
			currLine += 1;
			currColumn = 10;
		}
		else if (volTxt.at(i) == ",") {
			QAxObject* cell = worksheet->querySubObject("Cells(int,int)", currLine + topPaddingLines, currColumn);
			cell->setProperty("Value", tempLog);
			tempLog = "";
			currColumn += 1;
		}
		else {
			tempLog += volTxt.at(i);
		}
	}
	/*
		Write the timespan for the file
	*/
	QAxObject* cell1 = worksheet->querySubObject("Cells(int,int)", 2, 4);
	cell1->setProperty("Value", "From: " + lastReset);

	QAxObject* cell2 = worksheet->querySubObject("Cells(int,int)", 3, 4);
	cell2->setProperty("Value", "To: " + QDateTime::currentDateTime().toString("MM-dd-yyyy hh:mm"));

	/*
		Visuals and color
	*/
	int volLineNum = volTxt.count("\n");
	int lineNum = all.count("\n");
	int num = 0;
	if (volLineNum > lineNum) num = volLineNum + topPaddingLines;
	else num = lineNum + topPaddingLines;

	for (int i = 1; i < num + 1; i++) { // create a separator between the two tables
		QAxObject* cell = worksheet->querySubObject("Cells(int,int)", i, 8);
		QAxObject* interior = cell->querySubObject("Interior");
		interior->setProperty("Color", QColor(0, 32, 96));
	}


	for (int i = 1; i < 7; i++) { // Color the header/titles for the log
		QAxObject* cell = worksheet->querySubObject("Cells(int,int)", topPaddingLines + 1, i);
		QAxObject* interior = cell->querySubObject("Interior");
		interior->setProperty("Color", QColor(180, 198, 231));
	}
	for (int i = 1; i < 15; i++) { // Color the header/titles for the other one
		QAxObject* cell = worksheet->querySubObject("Cells(int,int)", topPaddingLines + 1, i + 9);
		QAxObject* interior = cell->querySubObject("Interior");
		interior->setProperty("Color", QColor(169, 208, 142));
	}

	/*
		Create titles and functions for the Volunteer list
	*/
	QAxObject* cell3 = worksheet->querySubObject("Cells(int,int)", topPaddingLines - 1, 11);
	cell3->setProperty("Value", "Number Of People");
	QAxObject* interior3 = cell3->querySubObject("Interior");
	interior3->setProperty("Color", QColor(248, 203, 173));

	QAxObject* cell4 = worksheet->querySubObject("Cells(int,int)", topPaddingLines, 11);
	cell4->setProperty("Value", "Hours Total");
	QAxObject* interior4 = cell4->querySubObject("Interior");
	interior4->setProperty("Color", QColor(255,230,153));

	QString letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	for (int i = 12; i < 24; i++) {
		QAxObject* cell = worksheet->querySubObject("Cells(int,int)", topPaddingLines - 1, i);
		cell->setProperty("Value", "=COUNTIF(" + letters[i - 1] + QString::number(topPaddingLines + 2) + ":" + letters[i - 1] + QString::number(topPaddingLines + volLineNum) + ",\">0\")");
		QAxObject* interior = cell->querySubObject("Interior");
		interior->setProperty("Color", QColor(248, 203, 173));

		QAxObject* cell2 = worksheet->querySubObject("Cells(int,int)", topPaddingLines, i);
		cell2->setProperty("Value", "=SUM(" + letters[i - 1] + QString::number(topPaddingLines + 2) + ":" + letters[i - 1] + QString::number(topPaddingLines + volLineNum) + ")");
		QAxObject* interior2 = cell2->querySubObject("Interior");
		interior2->setProperty("Color", QColor(255, 230, 153));
	}

	workbook->dynamicCall("SaveAs(const QString&)", fileName);
	excel->dynamicCall("SetVisible(bool)", true);
	return true;
}

void LogNotebook::finishedExport()
{
	if (future.result() == false) QMessageBox::warning(this, "", "Could not export to Excel. This could be because Excel is not installed.");
	delete progress;
}


/*
*
*
*	MENU ACTIONS
*
*
*/
void LogNotebook::on_menuEdit_triggered(QAction* action)
{
	if (action->objectName() == "resetData") {
		QMessageBox warning(this);
		warning.setIcon(QMessageBox::Icon::Warning);
		warning.setText("Are you sure you want to clear the log? All data not saved elsewhere will be lost.");
		warning.addButton("Yes", QMessageBox::ButtonRole::YesRole);
		warning.addButton("Save and Continue", QMessageBox::ButtonRole::AcceptRole);
		warning.addButton("Cancel", QMessageBox::ButtonRole::RejectRole);

		int role = warning.exec();

		if (role == 0) {
			checkBackup();
			clearLog();
		}
		else if (role == 1) {
			checkBackup();
			QString fileName = QFileDialog::getSaveFileName(this,
				tr("Export Data"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
				tr("Comma Separated Values (*.csv)"));

			if (fileName.isEmpty()) {
				QMessageBox::information(this, "Data not cleared", "The data was not cleared because it could not be saved");
				return;
			}

			exportToCSV(fileName);
			clearLog();
		}
	}
	else if (action->objectName() == "removeName") {
		removeNameDlg->show();
		removeNameDlg->raise();
		removeNameDlg->activateWindow();
	}
}

void LogNotebook::on_menuAbout_triggered(QAction* action)
{
	AboutDialog dlg;
	dlg.exec();
}

void LogNotebook::on_menuOptions_triggered(QAction* action)
{
	if (action->objectName() == "manageCloud") {
		cloudManager->show();
	}
	else if (action->objectName() == "autoFillTask") {
		autoFillTask = ui.autoFillTask->isChecked();
	}
}

/*
*
*
*	BACKUPS
*
*
*/
void LogNotebook::logIntoDB()
{

	delete dropBox;
	dropBox = new QOAuth2AuthorizationCodeFlow;

	QFile jsonFile(QApplication::applicationDirPath() + "/dropbox_autho.json");
	jsonFile.open(QFile::ReadOnly);
	QJsonDocument document;
	document = QJsonDocument().fromJson(jsonFile.readAll());
	jsonFile.close();

	connect(dropBox, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
		&QDesktopServices::openUrl);

	const auto object = document.object();
	const auto settingsObject = object["web"].toObject();
	const QUrl authUri(settingsObject["auth_uri"].toString());
	const auto clientId = settingsObject["client_id"].toString();
	const QUrl tokenUri(settingsObject["token_uri"].toString());
	const auto clientSecret(settingsObject["client_secret"].toString());
	const auto redirectUris = settingsObject["redirect_uris"].toArray();
	const QUrl redirectUri(redirectUris[0].toString()); // Get the first URI
	const auto port = static_cast<quint16>(redirectUri.port()); // Get the port

	auto replyHandler = new QOAuthHttpServerReplyHandler(port, dropBox);
	dropBox->setReplyHandler(replyHandler);

	dropBox->setAuthorizationUrl(authUri);
	dropBox->setClientIdentifier(clientId);
	dropBox->setAccessTokenUrl(tokenUri);
	dropBox->setClientIdentifierSharedKey(clientSecret);

	dropBox->grant();
	connect(dropBox, SIGNAL(granted()),
		this, SLOT(loginDone()));
	//connect(dropBox, SIGNAL(error(const QString &, const QString &, const QUrl &)),
		//this, SLOT(cloudError(const QString &, const QString &, const QUrl &)));
}



void LogNotebook::loginDone()
{
	QNetworkAccessManager* networkManager = new QNetworkAccessManager;
	QNetworkRequest request(QUrl("https://api.dropboxapi.com/2/users/get_current_account"));

	request.setRawHeader("Authorization", QString("Bearer " + dropBox->token()).toUtf8());

	QByteArray data;
	QNetworkReply* reply = networkManager->post(request, data);

	connect(reply, SIGNAL(finished()),
		this, SLOT(gotUsrName()));

	cloudManager->setLoggedIn(true);
	getLastDBBackup();
	cloudManager->writeLine("Logged In");
}

void LogNotebook::gotUsrName()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

	QString str = reply->readAll();
	QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());

	auto object = doc.object();
	auto name = object["name"].toObject();
	userName = name["display_name"].toString();

	cloudManager->setUserName(userName);

	reply->deleteLater();
	cloudManager->writeLine("Got user name \"" + userName + "\"");
}
void LogNotebook::cloudError(const QString& error, const QString& errorDescription, const QUrl& uri)
{
	cloudManager->writeLine("Error :" + error + "	" + errorDescription);
}

void LogNotebook::getLastDBBackup()
{
	if (dropBox->token() == Q_NULLPTR) return;
	QNetworkAccessManager* networkManager = new QNetworkAccessManager;
	QNetworkRequest request(QUrl("https://api.dropboxapi.com/2/files/list_folder"));

	request.setRawHeader("Authorization", QString("Bearer " + dropBox->token()).toUtf8());
	request.setRawHeader("Content-Type", "application/json");

	QNetworkReply* reply = networkManager->post(request, "{\"path\": \"\",\"recursive\": false,\"include_media_info\": false,\"include_deleted\": false,\"include_has_explicit_shared_members\": false,\"include_mounted_folders\": true,\"include_non_downloadable_files\": true}");

	connect(reply, SIGNAL(finished()),
		this, SLOT(gotLastBackup()));
}

void LogNotebook::gotLastBackup()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	QString str = reply->readAll();
	QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());

	auto object = doc.object();
	auto entries = object["entries"].toArray();

	QDate newestDate;
	for (int i = 0; i < entries.count(); i++) {
		auto obj = entries[i].toObject();
		QString name = obj["name"].toString();
		name = name.section("_", 1, 1);
		QDate date = QDate::fromString(name.section(".", 0, 0), "MM-dd-yyyy");

		if (i == 0) newestDate = date;
		else if (date.daysTo(newestDate) < 0) newestDate = date;
	}
	if (newestDate.isNull()) { // if no backup was found set the date to January 1st, 1970
		newestDate = QDate(1970,1,1);
	}
	lastCloudBackup = newestDate;
	cloudManager->setLatestBackup(newestDate);

	if (newestDate == QDate(1970, 1, 1)) cloudManager->writeLine("No Backups Found");
	else cloudManager->writeLine("Last backup was " + newestDate.toString("MM-dd-yyyy"));
	
	reply->deleteLater();

	checkBackup();
}




void LogNotebook::backupToCloud()
{
	if (dropBox->token() == Q_NULLPTR) return;
	QString tmpFileName = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "backup.tmp";
	exportToCSV(tmpFileName);

	QByteArray data;
	QFile* file = new QFile(tmpFileName);
	file->open(QFile::ReadOnly | QFile::ExistingOnly);
	data = file->readAll();
	file->close();

	QNetworkAccessManager* networkManager = new QNetworkAccessManager;
	QNetworkRequest request(QUrl("https://content.dropboxapi.com/2/files/upload"));

	request.setRawHeader("Authorization", QString("Bearer " + dropBox->token()).toUtf8());
	request.setRawHeader("Dropbox-API-Arg", QString("{\"path\": \"/Backup_" + QDate::currentDate().toString("MM-dd-yyyy") + ".csv\",\"mode\": \"add\",\"autorename\": true,\"mute\": false,\"strict_conflict\": false}").toUtf8());
	request.setRawHeader("Content-Type", "application/octet-stream");

	QNetworkReply* reply = networkManager->post(request, data);

	connect(reply, SIGNAL(finished()),
		this, SLOT(uploadDone()));

	connect(reply, SIGNAL(uploadProgress(qint64, qint64)),
		this, SLOT(uploadProgress(qint64, qint64)));

}

void LogNotebook::uploadProgress(qint64 bytesSent, qint64 bytesTotal) {
	qDebug() << "---------Uploaded--------------" << bytesSent << "of" << bytesTotal;
	cloudManager->writeLine("---------Uploaded-------------- " + QString::number(bytesSent) + " of " + QString::number(bytesTotal));
}

void LogNotebook::uploadDone() {
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

	qDebug() << "----------Finished--------------";
	cloudManager->writeLine("----------Finished--------------");

	reply->deleteLater();
	QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
	dir.remove("backup.tmp");

	cloudManager->setLatestBackup(QDate::currentDate());
	cloudManager->writeLine("New backup on " + QDate::currentDate().toString("MM-dd-yyyy"));
}

void LogNotebook::logOutofDB()
{
	/*
		Log out here
	*/
	QNetworkAccessManager* networkManager = new QNetworkAccessManager;
	QNetworkRequest request(QUrl("https://api.dropboxapi.com/2/auth/token/revoke"));

	request.setRawHeader("Authorization", QString("Bearer " + dropBox->token()).toUtf8());

	QByteArray data;
	QNetworkReply* reply = networkManager->post(request, data);
	delete dropBox;
	dropBox = new QOAuth2AuthorizationCodeFlow;

	cloudManager->setLoggedIn(false);
	cloudManager->writeLine("Logged out");
	dropBox->setToken(Q_NULLPTR);
	userName = "";
	cloudManager->setUserName(userName);
	cloudManager->setLatestBackup(QDate());
}


void LogNotebook::checkBackup()
{
	if (lastLogBackup.daysTo(QDate::currentDate()) >= 1) {
		bool failed = false;
		QFile tmp(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/LogBackup/backup.tmp");
		if (!QFile::copy(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/Log.csv", tmp.fileName())) {
			qDebug() << "Failed to copy the log to a temporary file";
			failed = true;
		}

		QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/LogBackup/");
		if (!failed) {
			if (!dir.remove(logBackupName)) {
				qDebug() << "Failed to remove the old log backup";
			}
			QFile newFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/LogBackup/Log_" + QDate::currentDate().toString("MM-dd-yyyy") + ".csv");
			if (!QFile::copy(tmp.fileName(), newFile.fileName())) {
				qDebug() << "Failed to copy the temporary log to the new backup";
			}
		}

		if (!dir.remove("backup.tmp")) {
			qDebug() << "Failed to remove the temporary log backup file";
		}
		lastLogBackup = QDate::currentDate();
	}
	if (lastVolBackup.daysTo(QDate::currentDate()) >= 1) {
		bool failed = false;
		QFile tmp(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/VolunteersBackup/backup.tmp");
		if (!QFile::copy(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/Volunteers.csv", tmp.fileName())) {
			qDebug() << "Failed to copy the volunteers to a temporary file";
			failed = true;
		}
		QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/VolunteersBackup/");
		if (!failed) {
			if (!dir.remove(volBackupName)) {
				qDebug() << "Failed to remove the old volunteers backup";
			}

			QFile newFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/VolunteersBackup/Volunteers_" + QDate::currentDate().toString("MM-dd-yyyy") + ".csv");
			if (!QFile::copy(tmp.fileName(), newFile.fileName())) {
				qDebug() << "Failed to copy the temporary volunteers to the new backup";
			}
		}
		if (!dir.remove("backup.tmp")) {
			qDebug() << "Failed to remove the temporary volunteers backup file";
		}

		lastVolBackup = QDate::currentDate();
	}
	if (lastCloudBackup.daysTo(QDate::currentDate()) >= backupFrequency) {
		backupToCloud();
	}
}
