#include "ipc.hpp"

static const std::string RSA_CHUNK[] = {
	"1321277432058722840622950990822933849527763264961655079678763",
	"124710459426827943004376449897985582167801707960697037164044904",
};

static const std::vector<std::string> HOSTS[] = {
	{
		"login01.tibia.com",
		"login02.tibia.com",
		"login03.tibia.com",
		"login04.tibia.com",
		"login05.tibia.com",
	},
	{
		"tibia01.cipsoft.com",
		"tibia02.cipsoft.com",
		"tibia03.cipsoft.com",
		"tibia04.cipsoft.com",
		"tibia05.cipsoft.com",
	}
};

static const std::string RSA_OT = ""
	"1091201329673994292788609605089955415282375029027981291234687579"
	"3726629149257644633073969600111060390723088861007265581882535850"
	"3429057592827629436413108566029093628212635953836686562675849720"
	"6207862794310902180176810615217550567108238764764442605581471797"
	"07119674283982419152118103759076030616683978566631413";

static std::string pad(const std::string &str, std::size_t length)
{
	std::string output = str;
	while(output.size() < length)
		output.push_back('\0');

	return output;
}

static void patch(std::string &data, std::string::size_type pos, const std::string &replacement)
{
	auto size = replacement.size();
	for(decltype(size) i = 0; i < size; ++i, ++pos)
		data[pos] = replacement[i];
}

MainWindow::MainWindow(QWidget *parent/* = nullptr*/)
: QMainWindow{parent}
, manager{this}
, cwd{0}
{
	ui.setupUi(this);

	connect(ui.btnLaunch, SIGNAL(clicked()), this, SLOT(onLaunch()));

	connect(ui.btnAdd   , SIGNAL(clicked()), this, SLOT(onClientAdd()));
	connect(ui.btnRemove, SIGNAL(clicked()), this, SLOT(onClientRemove()));
	connect(ui.btnModify, SIGNAL(clicked()), this, SLOT(onClientModify()));

	getcwd(cwd, sizeof(cwd));
	load();
}

MainWindow::~MainWindow()
{
	chdir(cwd);
	save();
}

void MainWindow::load()
{
	std::ifstream stream{"ipc.cfg"};
	if(!stream.is_open())
		return;

	std::string line{};
	while(std::getline(stream, line))
	{
		const auto &position = line.find(":");
		if(position == std::string::npos)
			continue;

		const auto &name = line.substr(0, position);
		const auto &path = line.substr(position + 1);

		clients[name] = path;

		auto *item = new QTreeWidgetItem(ui.treeClients);
		item->setText(0, QString::fromStdString(name));
		item->setText(1, QString::fromStdString(path));

		ui.choiceClient->addItem(QString::fromStdString(name));
	}

	stream.close();
}

void MainWindow::save()
{
	std::ofstream stream{"ipc.cfg", std::ios::trunc};
	if(!stream.is_open())
		return;

	for(const auto &it: clients)
		stream << it.first << ':' << it.second << '\n';

	stream.close();
}

void MainWindow::onLaunch()
{
	const auto &text = ui.editAddress->text();
	if(!text.size())
	{
		QMessageBox::information(this, "Information", "Please enter the address.");
		return;
	}

	const auto index = ui.choiceClient->currentIndex();
	if(index == -1)
	{
		QMessageBox::information(this, "Information", "Please add a client first.");
		return;
	}

	const auto it = clients.find(ui.choiceClient->itemText(index).toStdString());
	if(it == clients.end())
	{
		QMessageBox::information(this, "Information", "The selected client does not exist.");
		return;
	}

	const auto &parts = text.split(":");
	std::string host{};
	std::string port{};
	std::string directory{};

	if(parts.size() == 2)
	{
		host = parts[0].toStdString();
		port = parts[1].toStdString();
	}
	else
	{
		host = parts[0].toStdString();
		port = "7171";
	}

	directory = it->second.substr(0, it->second.rfind('/'));

	if(host.size() > 17)
	{
		struct hostent *he;
		if((he = gethostbyname(host.c_str())) == nullptr)
		{
			QMessageBox::about(this, "Information", "Failed to look up the host address.");
			return;
		}

		host = inet_ntoa(*reinterpret_cast<in_addr *>(he->h_addr_list[0]));
	}

	std::ifstream stream{it->second, std::ios::binary};
	if(!stream.is_open())
	{
		fprintf(stderr, "Failed to open the client file.\n");
		return;
	}

	std::string data(
		(std::istreambuf_iterator<char>(stream)),
		std::istreambuf_iterator<char>()
	);

	{
		std::string::size_type pos = 0;
		if((pos = data.find(RSA_CHUNK[0])) != std::string::npos)
			patch(data, pos, pad(RSA_OT, 310));
		else if((pos = data.find(RSA_CHUNK[1])) != std::string::npos)
			patch(data, pos, pad(RSA_OT, 310));
		else
		{
			fprintf(stderr, "Failed to patch the RSA key.\n");
			return;
		}
	}

	{
		const std::string &host1 = pad(host, 17);
		const std::string &host2 = pad(host, 19);

		for(const auto &host: HOSTS[0])
		{
			std::string::size_type pos = 0;
			if((pos = data.find(host)) != std::string::npos)
				patch(data, pos, host1);
		}

		for(const auto &host: HOSTS[1])
		{
			std::string::size_type pos = 0;
			if((pos = data.find(host)) != std::string::npos)
				patch(data, pos, host2);
		}
	}

	if(port != "7171")
	{
		std::string::size_type pos = 0;
		std::string::size_type base = data.find(host);
		if((pos = data.find("\x03\x1c\x00\x00", base - 1000)) != std::string::npos)
		{
			uint32_t port_num = std::stoi(port);
			uint8_t lower = port_num & 0xFF;
			uint8_t upper = port_num >> 8;

			for(uint8_t i = 0; i < 10; ++i)
			{
				patch(data, pos + 0, std::string(1, lower));
				patch(data, pos + 1, std::string(1, upper));

				pos += 8;
			}
		}
		else
		{
			fprintf(stderr, "Failed to patch the port.\n");
			return;
		}
	}

	char *path = new char[18];
	std::strncpy(path, "/tmp/Tibia_XXXXXX\0", 18);

	const auto fd = mkstemp(path);
	if(fd == -1)
	{
		fprintf(stderr, "Failed to open the temporary file.\n");
		delete[] path;
		return;
	}

	if(fchmod(fd, 0755) == -1)
	{
		fprintf(stderr, "Failed to set the permissions for the temporary file.\n");
		delete[] path;
		return;
	}

	FILE *fp = fdopen(fd, "w+");
	if(fp != nullptr)
	{
		std::fwrite(data.data(), sizeof(std::string::value_type), data.size(), fp);
		std::fclose(fp);

		if(chdir(directory.c_str()) == -1)
		{
			fprintf(stdout, "Failed to change the directory.\n");
			delete[] path;
			return;
		}

		if(fork() == 0)
			execl(path, path, nullptr);
	}
	else
		fprintf(stderr, "Failed to open the temporary file.\n");

	delete[] path;
}

void MainWindow::onClientAdd()
{
	manager.exec();

	const auto &name = manager.ui.editName->text();
	const auto &path = manager.ui.editPath->text();

	if(!name.size() || !path.size())
	{
		QMessageBox::about(this, "Information", "The specified name or path is invalid.");
		return;
	}

	auto it = clients.find(name.toStdString());
	if(it != clients.end())
	{
		QMessageBox::about(this, "Information", "A client with this name already exists.");
		return;
	}

	auto *item = new QTreeWidgetItem(ui.treeClients);
	item->setText(0, name);
	item->setText(1, path);

	ui.choiceClient->addItem(name);

	clients[name.toStdString()] = path.toStdString();

	manager.ui.editName->setText("");
	manager.ui.editPath->setText("");
}

void MainWindow::onClientRemove()
{
	auto *item = ui.treeClients->currentItem();
	if(!item)
	{
		QMessageBox::about(this, "Information", "Please select a client first.");
		return;
	}

	if(QMessageBox::information(this, "Information", "Are you sure you want to remove this client?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	const auto &name = item->text(0);

	auto it = clients.find(name.toStdString());
	if(it != clients.end())
		clients.erase(it);

	auto index = ui.choiceClient->findText(name);
	if(index != -1)
		ui.choiceClient->removeItem(index);

	delete item;
}

void MainWindow::onClientModify()
{
	auto *item = ui.treeClients->currentItem();
	if(!item)
	{
		QMessageBox::about(this, "Information", "Please select a client first.");
		return;
	}

	const auto &nameOrig = item->text(0);
	const auto &pathOrig = item->text(1);

	manager.ui.editName->setText(nameOrig);
	manager.ui.editPath->setText(pathOrig);

	manager.exec();

	const auto &name = manager.ui.editName->text();
	const auto &path = manager.ui.editPath->text();

	if(!name.size() || !path.size())
	{
		QMessageBox::about(this, "Information", "The specified name or path is invalid.");
		return;
	}

	if(nameOrig != name)
	{
		auto it = clients.find(nameOrig.toStdString());
		if(it != clients.end())
			clients.erase(it);

		auto index = ui.choiceClient->findText(nameOrig);
		if(index != -1)
			ui.choiceClient->setItemText(index, name);
	}

	clients[name.toStdString()] = path.toStdString();

	item->setText(0, name);
	item->setText(1, path);
}

void MainWindow::onClientDoubleClicked()
{
	emit onClientModify();
}
