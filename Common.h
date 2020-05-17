#pragma once

#include "QtWidgets"

// takes a full name such as "Ben Falco" and breaks it into "Ben" and "Falco"
void separateName(QString* first, QString* last, QString full);

// gets everything a signed in person can offer
void getAll(QString* first, QString* last, QString* task, QString* timeIn, QString line);

// takes a full line of the .csv and gets the first and last names
void getNames(QString* first, QString* last, QString line);

// determines whether or not a line in the .csv is signed out
bool isSignedOut(QString line);

QString copyFile(QString fileName);