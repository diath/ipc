#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>
#include <sys/stat.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

// Chrono ms literal
using namespace std::chrono_literals;

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

static const auto MC_SLEEP_TIME = 10ms;
static const auto MC_TIMEOUT = 5000ms;

static const char *ICON_NAME = "TibiaPlayerLinux";
static const char *ATOM_NAME = "TIBIARUNNING";

static std::map<std::string, std::string> clients{};

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
			if(client != 0)
				return client;
		}
	}

	return 0;
}

static void load()
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
	}

	stream.close();
}

static void save()
{
	std::ofstream stream{"ipc.cfg", std::ios::trunc};
	if(!stream.is_open())
		return;

	for(const auto &it: clients)
		stream << it.first << ':' << it.second << '\n';

	stream.close();
}

static void add(const std::vector<std::string> &args)
{
	if(args.size() < 2)
	{
		fprintf(stderr, "Action 'add' requires two arguments.\n");
		return;
	}

	const auto it = clients.find(args[0]);
	if(it != clients.end())
	{
		fprintf(stderr, "A client with name '%s' already exists, use the modify action to change the path.\n", args[0].c_str());
		return;
	}

	struct stat sb{};
	if(::stat(args[1].c_str(), &sb) != 0)
	{
		fprintf(stdout, "The specified path is not valid.\n");
		return;
	}

	if((sb.st_mode & S_IFMT) != S_IFREG)
	{
		fprintf(stdout, "The specified path is not valid.\n");
		return;
	}

	clients[args[0]] = args[1];
	fprintf(stdout, "The client has been added.\n");
}

static void remove(const std::vector<std::string> &args)
{
	if(args.size() < 1)
	{
		fprintf(stderr, "Action 'remove' requires one argument.\n");
		return;
	}

	const auto it = clients.find(args[0]);
	if(it == clients.end())
	{
		fprintf(stderr, "A client with name '%s' does not exist.\n", args[0].c_str());
		return;
	}

	clients.erase(it);
	fprintf(stdout, "The client has been removed.\n");
}

static void modify(const std::vector<std::string> &args)
{
	if(args.size() < 2)
	{
		fprintf(stderr, "Action 'modify' requires two arguments.\n");
		return;
	}

	const auto it = clients.find(args[0]);
	if(it == clients.end())
	{
		fprintf(stderr, "A client with name '%s' does not exist.\n", args[0].c_str());
		return;
	}

	clients[it->first] = args[1];
	fprintf(stdout, "The client has been modified.\n");
}

static void launch(const std::vector<std::string> &args)
{
	if(args.size() < 2)
	{
		fprintf(stderr, "Action 'launch' requires two arguments.\n");
		return;
	}

	const auto it = clients.find(args[0]);
	if(it == clients.end())
	{
		fprintf(stderr, "A client with name '%s' does not exist.\n", args[0].c_str());
		return;
	}

	std::string host{};
	std::string port{};
	std::string directory{};

	{
		const auto &position = args[1].find(':');
		if(position != std::string::npos)
		{
			host = args[1].substr(0, position);
			port = args[1].substr(position + 1);
		}
		else
		{
			host = args[1];
			port = "7171";
		}
		directory = it->second.substr(0, it->second.rfind('/'));
	}

	if(host.size() > 17)
	{
		struct hostent *he;
		if((he = gethostbyname(host.c_str())) == nullptr)
		{
			fprintf(stderr, "Failed to look up the host address.\n");
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

	std::unique_ptr<char[]> path{new char[18]};
	std::strncpy(path.get(), "/tmp/Tibia_XXXXXX\0", 18);

	const auto fd = mkstemp(path.get());
	if(fd == -1)
	{
		fprintf(stderr, "Failed to open the temporary file.\n");
		return;
	}

	if(fchmod(fd, 0755) == -1)
	{
		fprintf(stderr, "Failed to set the permissions for the temporary file.\n");
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
			return;
		}

		int fork_pid = fork();
		if (fork_pid == -1) {
			fprintf(stderr, "Failed to fork the process.\n");
		} else if (fork_pid == 0) {
			execl(path.get(), path.get(), nullptr);
		} else {
			Display *display = XOpenDisplay(0);
			if (!display) {
				fprintf(stderr, "Failed to open the display. Multiclient functionality will not work\n");
				return;
			}

			Window root = XDefaultRootWindow(display);
			if (!root) {
				fprintf(stderr, "Failed to find the root WM window.\n");
				return;
			}

			Window client;
			auto passed = 0ms;

			while ((client = FindTibiaWindow(display, root)) == 0) {
				std::this_thread::sleep_for(MC_SLEEP_TIME);
				passed += MC_SLEEP_TIME;

				if(passed >= MC_TIMEOUT) {
					fprintf(stderr, "Failed to find the Tibia window after 5 seconds, giving up.\nMulticlient functionality will not work\n");
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

static void list(const std::vector<std::string> &)
{
	fprintf(stdout, "Stored configurations:\n");
	for(const auto &it: clients)
		fprintf(stdout, "\t%s => %s\n", it.first.c_str(), it.second.c_str());
}

int main(int argc, const char **argv)
{
	std::vector<std::string> arguments(argv + 1, argv + argc);
	if(!arguments.size())
	{
		fprintf(stdout,
			"ipc usage:\n"
			"\t--add name path\n"
			"\t\tStore a new configuration entry.\n"
			"\t--remove name\n"
			"\t\tRemove existing configuration entry.\n"
			"\t--modfiy name path\n"
			"\t\tModify the path of existing configuration entry.\n"
			"\t--launch name host:port\n"
			"\t\tLaunch the client.\n"
			"\t--list\n"
			"\t\tDisplay a list of configuration entries.\n"
		);
		return 0;
	}

	char cwd[512] = {0};
	if(getcwd(cwd, sizeof(cwd)) == nullptr)
	{
		fprintf(stderr, "Failed to get the current working directory.\n");
		return 0;
	}

	load();

	const auto &action = arguments[0];
	const auto &slice = arguments.size() > 1 ? std::vector<std::string>(arguments.begin() + 1, arguments.end()) : std::vector<std::string>{};
	if(action == "--add")
		add(slice);
	else if(action == "--remove")
		remove(slice);
	else if(action == "--modify")
		modify(slice);
	else if(action == "--launch")
		launch(slice);
	else if(action == "--list")
		list(slice);
	else
		fprintf(stderr, "ipc: Unknown option. Run without arguments for help.\n", argv[0]);

	// NOTE: We need to change the dir back to the original cwd
	//       otherwise the config would get saved in client's directory
	chdir(cwd);
	save();

	return 0;
}
