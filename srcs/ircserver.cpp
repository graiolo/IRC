#include "../includes/ircserver.hpp"

void toUpper(std::string &s)
{
	for (size_t i = 0; i < s.size( ); i++)
		s[i] = std::toupper(s[i]);
}

void trimInput(std::string &s)
{
	if (s.find('\n') != std::string::npos)
		s.erase(s.find('\n'));
	if(s.find('\r') != std::string::npos)
		s.erase(s.find('\r'));
	while (true) {
		size_t at = s.find_last_of(" ");
		if (s.size( ) != 0 && at == s.size( ) - 1)
			s.erase(at);
		else
			break ;
	}
	while (true) {
		size_t at = s.find_first_of(" ");
		if (s.size( ) != 0 && at == 0)
			s.erase(at, 1);
		else
			break ;
	}
}


std::deque<std::string> split(std::string string, char del)
{
	std::deque<std::string> split;
	std::string token;
	std::istringstream iss(string);

	while (std::getline(iss, token, del))
		split.push_back(token);
	return split;
}

bool ClientInfo::operator<(const ClientInfo& rhs) const { return (fd < rhs.fd); }

bool ClientInfo::operator==(const ClientInfo& rhs) const { return (fd == rhs.fd); }