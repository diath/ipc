#pragma once

#include <QtGui>
#include <QMessageBox>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string>

#include <unistd.h>
#include <sys/stat.h>

#include <netdb.h>
#include <arpa/inet.h>

#include "ui_ipc.h"
#include "manager.hpp"

class MainWindow: public QMainWindow
{
	Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = nullptr);
		~MainWindow();

	private slots:
		void onLaunch();
		void onClientAdd();
		void onClientRemove();
		void onClientModify();
		void onClientDoubleClicked();

	private:
		void load();
		void save();

		Ui::MainWindow ui;
		Manager manager;
		std::map<std::string, std::string> clients;
		char cwd[512];
};
