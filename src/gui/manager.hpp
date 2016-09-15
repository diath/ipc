#pragma once

#include <QtGui>
#include <QFileDialog>
#include <QMessageBox>

#include "ui_manager.h"

class Manager: public QDialog
{
	Q_OBJECT

	public:
		explicit Manager(QWidget *parent = nullptr);
		~Manager();

		Ui::Manager ui;

	private slots:
		void onBrowse();
		void onSave();
};
