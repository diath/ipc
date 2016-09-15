#ifndef CONST_HPP
#define CONST_HPP

#include <chrono>
#include <string>
#include <vector>

// NOTE: The FindTibiaWindow function cannot be moved here due to
//       how the headers need to be included differently when
//       using Qt with Xlib

// Chrono ms literal
using namespace std::chrono_literals;

extern const std::string RSA_CHUNK[];
extern const std::vector<std::string> HOSTS[];

extern const std::string RSA_OT;

extern const std::chrono::milliseconds MC_SLEEP_TIME;
extern const std::chrono::milliseconds MC_TIMEOUT;

extern const char *ICON_NAME;
extern const char *ATOM_NAME;

std::string pad(const std::string &str, std::size_t length);
void patch(std::string &data, size_t pos, const std::string &replacement);

#endif
