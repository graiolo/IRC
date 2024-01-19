#pragma once

# include <iostream>
# include <cstdlib>
# include <cstring>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <sys/epoll.h>
# include <map>
# include <unistd.h>
# include <errno.h>
# include <stdio.h>
# include <istream>
# include <sstream>
# include <fcntl.h>
# include <vector>
# include <set>
# include <algorithm>
# include <ifaddrs.h>
# include <netinet/in.h>
# include <stack>
# include <arpa/inet.h>
# include <netdb.h>

extern char				hostname[256];

// Structure to hold information about each client
struct ClientInfo {
	std::string					nickname;
	std::string					username;
    std::string 				passwd;
    std::string 				msg;
	int							fd;
	bool					authorized;
    bool                        isFirstTime;
    std::vector<std::string>	joninedChannell;
	struct sockaddr_in			addr;
	socklen_t					addrLen;

	// operator overloads so find() can work
	bool        operator<(const ClientInfo& rhs) const;
	bool        operator==(const ClientInfo& rhs) const;
};

// Structure to hold information about each channel
struct ChannelInfo {
	std::string				name;
	std::string				topic;
	std::string				key;
	bool					inviteOnly;
	bool					topicLock;
	bool					limitLock;
	int         			limit;
    int                     userIn;
	std::map<std::string, int>	users;	 // nickname, fd
	std::map<std::string, int>	operators; // nickname, fd
	std::set<std::string>	invited; // Set of invited fds
};
//quanto entri entri in un canale MAIN cosi puoi vedere tutti gli utenti


struct ServerData {
	std::string			name;
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
