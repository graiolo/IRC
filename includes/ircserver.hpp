#ifndef IRCSERVER_HPP
# define IRCSERVER_HPP

#include "server.hpp"

// Structure to hold information about each client
struct ClientInfo {
	std::string					nickname;
	std::string					username;
    std::string 				passwd;
    std::string 				msg;
	int							fd;
	bool						authorized;
    bool                        isFirstTime;
    std::vector<std::string>	joninedChannell;
	struct sockaddr_in			addr;
	socklen_t					addrLen;

	// operator overloads so find() can work
	bool        operator<(const ClientInfo& rhs) const;
	bool        operator==(const ClientInfo& rhs) const;
};

// Structure to hold information about each channel
struct Channel {
	std::string				topic;
	std::string				key;
	bool					inviteOnly;
	bool					topicLock;
	bool					limitLock;
	int         			limit;
    int                     userIn;
	std::set<ClientInfo>	users;	 // A set of all the infos of users in this channel
	std::set<ClientInfo>	operators; // A set of all the infos of operators in this channel
	std::set<std::string>	invited; // Set of invited fds
};

struct ServerData {
	std::string			name;
    char				hostname[256];
	std::string			IP;
	struct sockaddr_in	addr;
	std::string			passwd;
	int					socket;
    int					port;
	int					connected_clients;
	int                 epollFd;
    bool                isRun;
};

void toUpper(std::string &s);
void trimInput(std::string &s);
std::deque<std::string> split(std::string string, char del);

#endif