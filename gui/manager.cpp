#include "manager.hpp"

Manager::Manager(QWidget *parent/* = nullptr*/)
: QDialog{parent}
{
	ui.setupUi(this);

	connect(ui.btnBrowse, SIGNAL(clicked()), this, SLOT(onBrowse()));
	connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(onSave()));

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

Manager::~Manager()
{
}

void Manager::onBrowse()
{
	const auto &path = QFileDialog::getOpenFileName(
		this, "Browse Clients", "~"
	);
	if (!path.size())
		return;

	ui.editPath->setText(path);
}

void Manager::onSave()
{
	hide();
}
