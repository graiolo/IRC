#ifndef SERVER_HPP
# define SERVER_HPP

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

# define MAX_CLIENTS 104 // Maximum number of clients the server will allow
# define HEADER "CAP LS 302"

class Server {
    protected :
        static struct ServerData						_server;
        static std::map<std::string, struct Channel>	_channels;
        static std::map<int, struct ClientInfo>		    _client_info;
		static struct epoll_event 						_events[MAX_CLIENTS + 1];

	public:
		Server(int, char *);
		~Server( );

        static void	serverInit( );
		static void	addServerToEpoll( );
		static int	addClinetToEpoll(int);
		static void	runServer( );

		static void	recvMessage(int);

		static void newClient( );
		static void	login(int);

		static void	sendWelcomeBackToClient(int);
		static void sendJoinMessageBackToClient(int, const std::string&);

		static void commands(int);
		static void ping(int);
        static void	singleJoin(int);
		static void	multiJoin(int);
        static void singlePart(int, bool);
        static void multiPart(int, bool);
        static void quit(int);
        static void mode(int);
        static bool isOpSizPawd(std::vector<std::string>);
        static void kick(int);
        static void topic(int);
        static void invite(int);

        static void multiMessage(int);
        static void singleMessage(int);

        static std::vector<std::string> modeChanges(std::string);
        static void sendToAllUserModeChanges(int, std::string, Channel, std::string);

};

#endif