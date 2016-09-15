#include "ipc.hpp"

#include <QX11Info>
#include <X11/Xlib.h>

static Window FindTibiaWindow(Display *display, Window window)
{
	char *propName = nullptr;
	if (XGetIconName(display, window, &propName) != 0) {
		if (propName != nullptr && std::strcmp(ICON_NAME, propName) == 0) {
			XFree(propName);
			return window;
		}

		if (propName != nullptr)
			XFree(propName);
	}

	Window windowRoot;
	Window windowParent;
	Window *children;
	unsigned int childrenCount;

	if (XQueryTree(display, window, &windowRoot, &windowParent, &children, &childrenCount) != 0 && children != nullptr) {
		for (unsigned int i = 0; i < childrenCount; ++i) {
			Window client = FindTibiaWindow(display, children[i]);
			if (client != 0)
				return client;
		}
	}

	return 0;
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
	if (!stream.is_open())
		return;

	std::string line{};
	while (std::getline(stream, line)) {
		const auto &position = line.find(":");
		if (position == std::string::npos)
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
	if (!stream.is_open())
		return;

	for (const auto &it: clients)
		stream << it.first << ':' << it.second << '\n';

	stream.close();
}

void MainWindow::onLaunch()
{
	const auto &text = ui.editAddress->text();
	if (!text.size()) {
		QMessageBox::information(this, "Information", "Please enter the address.");
		return;
	}

	const auto index = ui.choiceClient->currentIndex();
	if (index == -1) {
		QMessageBox::information(this, "Information", "Please add a client first.");
		return;
	}

	const auto it = clients.find(ui.choiceClient->itemText(index).toStdString());
	if (it == clients.end()) {
		QMessageBox::information(this, "Information", "The selected client does not exist.");
		return;
	}

	const auto &parts = text.split(":");
	std::string host{};
	std::string port{};
	std::string directory{};

	if (parts.size() == 2) {
		host = parts[0].toStdString();
		port = parts[1].toStdString();
	} else {
		host = parts[0].toStdString();
		port = "7171";
	}

	directory = it->second.substr(0, it->second.rfind('/'));

	if (host.size() > 17) {
		struct hostent *he;
		if ((he = gethostbyname(host.c_str())) == nullptr) {
			QMessageBox::about(this, "Information", "Failed to look up the host address.");
			return;
		}

		host = inet_ntoa(*reinterpret_cast<in_addr *>(he->h_addr_list[0]));
	}

	std::ifstream stream{it->second, std::ios::binary};
	if (!stream.is_open()) {
		fprintf(stderr, "Failed to open the client file.\n");
		return;
	}

	std::string data(
		(std::istreambuf_iterator<char>(stream)),
		std::istreambuf_iterator<char>()
	);

	{
		std::string::size_type pos = 0;
		if ((pos = data.find(RSA_CHUNK[0])) != std::string::npos) {
			patch(data, pos, pad(RSA_OT, 310));
		} else if ((pos = data.find(RSA_CHUNK[1])) != std::string::npos) {
			patch(data, pos, pad(RSA_OT, 310));
		} else {
			fprintf(stderr, "Failed to patch the RSA key.\n");
			return;
		}
	}

	{
		const std::string &host1 = pad(host, 17);
		const std::string &host2 = pad(host, 19);

		for (const auto &host: HOSTS[0]) {
			std::string::size_type pos = 0;
			if ((pos = data.find(host)) != std::string::npos)
				patch(data, pos, host1);
		}

		for (const auto &host: HOSTS[1]) {
			std::string::size_type pos = 0;
			if ((pos = data.find(host)) != std::string::npos)
				patch(data, pos, host2);
		}
	}

	if (port != "7171") {
		std::string::size_type pos = 0;
		std::string::size_type base = data.find(host);
		if ((pos = data.find("\x03\x1c\x00\x00", base - 1000)) != std::string::npos) {
			uint32_t port_num = std::stoi(port);
			uint8_t lower = port_num & 0xFF;
			uint8_t upper = port_num >> 8;

			for (uint8_t i = 0; i < 10; ++i) {
				patch(data, pos + 0, std::string(1, lower));
				patch(data, pos + 1, std::string(1, upper));

				pos += 8;
			}
		} else {
			fprintf(stderr, "Failed to patch the port.\n");
			return;
		}
	}

	std::unique_ptr<char[]> path{new char[18]};
	std::strncpy(path.get(), "/tmp/Tibia_XXXXXX\0", 18);

	const auto fd = mkstemp(path.get());
	if (fd == -1) {
		fprintf(stderr, "Failed to open the temporary file.\n");
		return;
	}

	if (fchmod(fd, 0755) == -1) {
		fprintf(stderr, "Failed to set the permissions for the temporary file.\n");
		return;
	}

	FILE *fp = fdopen(fd, "w+");
	if (fp != nullptr) {
		std::fwrite(data.data(), sizeof(std::string::value_type), data.size(), fp);
		std::fclose(fp);

		if (chdir(directory.c_str()) == -1) {
			fprintf(stdout, "Failed to change the directory.\n");
			return;
		}

		int fork_pid = fork();
		if (fork_pid == -1) {
			QMessageBox::about(this, "Information", "Failed to fork the proecess.");
		} else if (fork_pid == 0) {
			execl(path.get(), path.get(), nullptr);
		} else {
			Display *display = XOpenDisplay(0);
			if (!display) {
				QMessageBox::about(this, "Information", "Failed to open the display.\nMulticlient functionality will not work.");
				return;
			}

			Window root = XDefaultRootWindow(display);
			if (!root) {
				QMessageBox::about(this, "Information", "Failed to find the root WM window.");
				return;
			}

			Window client;
			auto passed = 0ms;

			while ((client = FindTibiaWindow(display, root)) == 0) {
				std::this_thread::sleep_for(MC_SLEEP_TIME);
				passed += MC_SLEEP_TIME;

				if (passed >= MC_TIMEOUT) {
					QMessageBox::about(this, "Information", "Failed to find the Tibia window after 5 seconds, giving up.\nMulticlient functionality will not work\n");
					break;
				}
			}

			if (client != 0) {
				Atom selection = XInternAtom(display, ATOM_NAME, 0);
				XSetSelectionOwner(display, selection, None, CurrentTime);
				XGetSelectionOwner(display, selection); // XSetSelectionOwner alone won't work for some reason ¯\_(ツ)_/¯
			}
		}
	}
	else
		fprintf(stderr, "Failed to open the temporary file.\n");
}

void MainWindow::onClientAdd()
{
	manager.exec();

	const auto &name = manager.ui.editName->text();
	const auto &path = manager.ui.editPath->text();

	if (!name.size() || !path.size()) {
		QMessageBox::about(this, "Information", "The specified name or path is invalid.");
		return;
	}

	auto it = clients.find(name.toStdString());
	if (it != clients.end()) {
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
	if (!item) {
		QMessageBox::about(this, "Information", "Please select a client first.");
		return;
	}

	if (QMessageBox::information(this, "Information", "Are you sure you want to remove this client?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	const auto &name = item->text(0);

	auto it = clients.find(name.toStdString());
	if (it != clients.end())
		clients.erase(it);

	auto index = ui.choiceClient->findText(name);
	if (index != -1)
		ui.choiceClient->removeItem(index);

	delete item;
}

void MainWindow::onClientModify()
{
	auto *item = ui.treeClients->currentItem();
	if (!item) {
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

	if (!name.size() || !path.size()) {
		QMessageBox::about(this, "Information", "The specified name or path is invalid.");
		return;
	}

	if (nameOrig != name) {
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
