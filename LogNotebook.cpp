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

LogNotebook::LogNotebook(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	updateTable();

	/*
		Set the color of the buttons
	*/
	ui.signInBtn->setStyleSheet(
							"QPushButton{background-color: green;color:black; border-width: 4px; border-style: outset; border-color: black}\
							QPushButton::pressed{background-color: black;color:green; border-width: 4px; border-style: outset; border-color: green}");
	
	ui.signOutBtn->setStyleSheet(
								"QPushButton{background-color: red;color:black; border-width: 4px; border-style: outset; border-color: black}\
								QPushButton::pressed{background-color: black;color:red; border-width: 4px; border-style: outset; border-color: red}");

	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	/*
		Get the last time that data was backed up localy
	*/
	QDir logDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/LogBackup/");
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
}

LogNotebook::~LogNotebook()
{
	QSettings settings;
	settings.setValue("backupFrequency", backupFrequency);
	settings.setValue("token", dropBox->token());
	settings.setValue("userName", userName);
}
void LogNotebook::on_signInBtn_clicked() {
	SignInDialog inDialog;
	inDialog.exec();
	updateTable();
}

void LogNotebook::on_signOutBtn_clicked()
{
	SignOutDialog outDialog;
	outDialog.exec();
	updateTable();
}

void LogNotebook::addItem(int row, int column, QString text)
{
	QTableWidgetItem* pCell = ui.tableWidget->item(row , column);
	if (!pCell) {
		pCell = new QTableWidgetItem;
		ui.tableWidget->setItem(row, column, pCell);
	}
	pCell->setText(text);
}

void LogNotebook::resizeEvent(QResizeEvent* event)
{
	
}

QProgressDialog* progress;
void LogNotebook::on_menuData_triggered(QAction* action)
{
	if (action->objectName() == "exportExcel") {
		QString fileName = QFileDialog::getSaveFileName(this,
			tr("Export Data"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
			tr("Excel Workbook (*.xlsx)"));

		if (fileName.isEmpty()) return;

		progress = new QProgressDialog(this);
		progress->setWindowModality(Qt::WindowModal);
		progress->setLabelText("Exporting to Excel...");
		progress->setCancelButton(0);
		progress->setRange(0, 0);
		progress->setMinimumDuration(0);
		progress->show();

		future = QtConcurrent::run(exportToExcel, fileName);
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

void exportToCSV(QString fileName) {
	QFile logFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	if (!logFile.open(QFile::ReadOnly | QFile::ExistingOnly | QFile::Text));

	QTextStream logIn(&logFile);
	QString logTxt = logIn.readAll();
	logFile.close();

	QFile volFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Volunteers.csv");
	if (!volFile.open(QFile::ReadOnly | QFile::ExistingOnly | QFile::Text));

	QTextStream volIn(&volFile);
	QString volTxt = volIn.readAll();
	volFile.close();

	QFile newFile(fileName);

	newFile.open(QFile::WriteOnly | QFile::Text);
	QTextStream out(&newFile);

	int lines = 0;
	int logLines = logTxt.count("\n");
	int volLines = volTxt.count("\n");
	if (logLines > volLines) lines = logLines;
	else lines = volLines;


	for (int i = 0; i < lines; i++) {
		QString log = logTxt.section("\n", i, i);
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



bool exportToExcel(QString fileName){
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
		Write the log file to the left of the excel sheet
	*/
	QFile logFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	logFile.open(QFile::ReadOnly | QFile::Text);
	QTextStream in(&logFile);
	int lineNum = 0;
	while (!in.atEnd()) {
		QString line = in.readLine();
		if (line != "") {
			lineNum++;
			for (int i = 0; i < 6; i++) {
				QString txt = line.section(',', i, i);
				QAxObject* cell = worksheet->querySubObject("Cells(int,int)", lineNum + topPaddingLines, i + 1);
				cell->setProperty("Value", txt);
			}
		}
	}
	logFile.close();

	/*
		Write the Volunteers file to the right of the excel sheet
	*/
	QFile volFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Volunteers.csv");
	volFile.open(QFile::ReadOnly | QFile::Text);
	QTextStream volIn(&volFile);
	int volLineNum = 0;
	QString lastReset = volIn.readLine().section(": ", 1, 1);
	while (!volIn.atEnd()) {
		QString line = volIn.readLine();
		if (line != "") {
			volLineNum++;
			for (int i = 0; i < 14; i++) {
				QString txt = line.section(',', i, i);
				QAxObject* cell = worksheet->querySubObject("Cells(int,int)", volLineNum + topPaddingLines, i + 10);
				cell->setProperty("Value", txt);
			}
		}
	}
	volFile.close();

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
	QAxObject* cell3 = worksheet->querySubObject("Cells(int,int)", topPaddingLines - 2, 11);
	cell3->setProperty("Value", "Number Of People");
	QAxObject* interior3 = cell3->querySubObject("Interior");
	interior3->setProperty("Color", QColor(248, 203, 173));

	QAxObject* cell4 = worksheet->querySubObject("Cells(int,int)", topPaddingLines - 1, 11);
	cell4->setProperty("Value", "Hours Total");
	QAxObject* interior4 = cell4->querySubObject("Interior");
	interior4->setProperty("Color", QColor(255,230,153));

	QString letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	for (int i = 12; i < 24; i++) {
		QAxObject* cell = worksheet->querySubObject("Cells(int,int)", topPaddingLines - 2, i);
		cell->setProperty("Value", "=COUNTIF(" + letters[i - 1] + QString::number(topPaddingLines + 2) + ":" + letters[i - 1] + QString::number(topPaddingLines + volLineNum) + ",\">0\")");
		QAxObject* interior = cell->querySubObject("Interior");
		interior->setProperty("Color", QColor(248, 203, 173));

		QAxObject* cell2 = worksheet->querySubObject("Cells(int,int)", topPaddingLines - 1, i);
		cell2->setProperty("Value", "=SUM(" + letters[i - 1] + QString::number(topPaddingLines + 2) + ":" + letters[i - 1] + QString::number(topPaddingLines + volLineNum) + ")");
		QAxObject* interior2 = cell2->querySubObject("Interior");
		interior2->setProperty("Color", QColor(255, 230, 153));

		QAxObject* cell3 = worksheet->querySubObject("Cells(int,int)", topPaddingLines, i);
		cell3->setProperty("Value", "Hours");
	}

	workbook->dynamicCall("SaveAs(const QString&)", fileName);
	excel->dynamicCall("SetVisible(bool)", true);
	return true;
}

void LogNotebook::finishedExport()
{
	if (future.result() == false) QMessageBox::warning(this, "", "Could not export to Excel because Excel is not installed. Please install Excel or export to a .csv file.");
	delete progress;
}

void LogNotebook::clearBackups() {
	/*
			Clear Log.csv
		*/
	checkBackup();
	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");
	file.remove();
	file.open(QFile::WriteOnly | QFile::Text);

	QTextStream out(&file);
	out << "First,Last,Task,Time In,Time Out,total\n\n";
	file.close();



	/*
		Clear Volunteers.csv
	*/
	QFile volFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Volunteers.csv");
	if (!volFile.open(QIODevice::ReadWrite | QIODevice::Text | QFile::ExistingOnly)) {
		qDebug() << "Could not open Volunteer.csv";
		QMessageBox::critical(this, "", "Could not find Volunteers.csv");
	}

	QTextStream stream(&volFile);
	QString all = stream.readAll();
	stream.seek(0);

	int numb = all.section('\n', 0, 0).count();
	all.replace(0, numb, "Last Reset : " + QDateTime::currentDateTime().toString("MM-dd-yyyy hh:mm"));

	bool done = false;
	int secNum = 3;
	while (!done) {
		QString section = all.section('\n', secNum, secNum);
		if (section != "") {
			QString numbSection = section.section(',', 0, 1);

			int indexAll = all.indexOf(section);
			for (int i = 2; i < 14; i++) {
				int taskIndex = indexAll + numbSection.count() + ((i - 2) * 7) + 1;
				all.replace(taskIndex, 6, "0000.0");
			}
		}
		else {
			done = true;
		}
		secNum++;
	}
	stream << all;
	volFile.close();
	updateTable();
}
void LogNotebook::on_menuOptions_triggered(QAction* action)
{
	QMessageBox warning(this);
	warning.setIcon(QMessageBox::Icon::Warning);
	warning.setText("Are you sure you want to clear the log? All data not saved elsewhere will be lost.");
	warning.addButton("Yes", QMessageBox::ButtonRole::YesRole);
	warning.addButton("Save and Continue", QMessageBox::ButtonRole::AcceptRole);
	warning.addButton("Cancel", QMessageBox::ButtonRole::RejectRole);

	int role = warning.exec();
	
	if (role == QMessageBox::YesRole) {
		clearBackups();
	}
	else if (role == QMessageBox::AcceptRole) {
		QString fileName = QFileDialog::getSaveFileName(this,
			tr("Export Data"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
			tr("Comma Separated Values (*.csv)"));

		if (fileName.isEmpty()) {
			QMessageBox::information(this, "Data not cleared", "The data was not cleared because it could not be saved");
			return;
		}

		exportToCSV(fileName);
		clearBackups();
	}
}

void LogNotebook::on_menuAbout_triggered(QAction* action)
{
	AboutDialog dlg;
	dlg.exec();
}

void LogNotebook::on_menuOption_triggered(QAction* action)
{
	if (action->objectName() == "manageCloud") {
		cloudManager->show();
	}
}


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

void LogNotebook::updateTable()
{
	ui.tableWidget->setRowCount(0);
	QFile file(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + "Log.csv");

	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		return;
	}

	QTextStream in(&file);

	QString line;
	while (!in.atEnd()) {
		line = in.readLine();
		if (!isSignedOut(line)) {
			QString first, last, task, timeIn;
			getAll(&last, &first, &task, &timeIn, line);
			int row = ui.tableWidget->rowCount() + 1; // create a new row
			ui.tableWidget->setRowCount(row);

			addItem(row - 1, 0, last);
			addItem(row - 1, 1, first);
			addItem(row - 1, 2, task);
			addItem(row - 1, 3, timeIn);
		}
	}
	file.close();
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

		lastLogBackup = QDate::currentDate();
	}

	if (lastCloudBackup.daysTo(QDate::currentDate()) >= backupFrequency) {
		backupToCloud();
	}
}
