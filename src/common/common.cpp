#include "common.hpp"

const std::string RSA_CHUNK[] = {
	"1321277432058722840622950990822933849527763264961655079678763",
	"124710459426827943004376449897985582167801707960697037164044904",
};

const std::vector<std::string> HOSTS[] = {
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

const std::string RSA_OT = ""
	"1091201329673994292788609605089955415282375029027981291234687579"
	"3726629149257644633073969600111060390723088861007265581882535850"
	"3429057592827629436413108566029093628212635953836686562675849720"
	"6207862794310902180176810615217550567108238764764442605581471797"
	"07119674283982419152118103759076030616683978566631413";

const std::chrono::milliseconds MC_SLEEP_TIME = 10ms;
const std::chrono::milliseconds MC_TIMEOUT = 5000ms;

const char *ICON_NAME = "TibiaPlayerLinux";
const char *ATOM_NAME = "TIBIARUNNING";

std::string pad(const std::string &str, std::size_t length)
{
	std::string output = str;
	while (output.size() < length)
		output.push_back('\0');

	return output;
}

void patch(std::string &data, std::string::size_type pos, const std::string &replacement)
{
	auto size = replacement.size();
	for (decltype(size) i = 0; i < size; ++i, ++pos)
		data[pos] = replacement[i];
}
