#include "../includes/server.hpp"
#include "../includes/ircserver.hpp"

struct ServerData						Server::_server;
std::map<std::string, struct Channel>	Server::_channels;
std::map<int, struct ClientInfo>		Server::_client_info;
struct epoll_event 						Server::_events[MAX_CLIENTS + 1];

Server::Server(int port, char *passwd) {
	memset(reinterpret_cast<char*>(&_server), 0, sizeof(_server));
	_server.port = port;
	_server.passwd = passwd;

	memset(reinterpret_cast<char*>(&_events), 0, sizeof(_events));
}

Server::~Server( ) { }

void Server::serverInit( ) {
	// Create a socket
	_server.socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_server.socket < 0)
		throw std::runtime_error ("ERROR: Failed to open socket");
	// Allow the kernel to reuse the port
	int enable = 1;
	if (setsockopt(_server.socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
		throw std::runtime_error ("ERROR: setsockopt failed");
	int recicle = -1;
	if (setsockopt(_server.socket, SOL_SOCKET, SO_REUSEADDR, &recicle, sizeof(recicle)) == -1)
		throw std::runtime_error("Socket can't be reused");
	if (gethostname(_server.hostname, sizeof(_server.hostname)) == -1)
		throw std::runtime_error ("ERROR: gethostname failed");

	// Create a sockaddr_in struct for the proper port and bind our socket to it
	_server.addr.sin_family = AF_INET;
	_server.addr.sin_port = htons(_server.port); // We will need to convert the port to network byte order
	_server.addr.sin_addr = *((struct in_addr*) gethostbyname(_server.hostname)->h_addr_list[0]); //inet_addr("ip of local machine");
	_server.IP = inet_ntoa(_server.addr.sin_addr);

	// Bind our socket to the port
	if (bind(_server.socket, (struct sockaddr *) &_server.addr, sizeof(_server.addr)) == -1)
		throw std::runtime_error ("ERROR: Failed to bind to socket");

	// Start listening on with the socket
	if (listen(_server.socket, 5) == -1) // The 5 here is the maximum number of backlog connections
		throw std::runtime_error ("ERROR: listen failed");

	_server.isRun = true;
	std::cout << "Server started and listening on IP " <<  _server.IP << " port " << _server.port << std::endl;
}

void	Server::addServerToEpoll( ) {
		
	_server.epollFd = epoll_create1(0);
	if (_server.epollFd == -1)
		throw std::runtime_error ("ERROR: poll failed");

	Server::addClinetToEpoll(_server.socket);
}

int Server::addClinetToEpoll(int newSocket) {
	struct epoll_event event;

	memset(reinterpret_cast<char*>(&event), 0, sizeof(event));
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = newSocket;
	if (epoll_ctl(_server.epollFd, EPOLL_CTL_ADD, newSocket, &event) == -1) {
		perror("Error adding socket to epoll");
		close(_server.epollFd);
		return -1;
	}
	return 0;
}

void Server::runServer( ) {
	while (_server.isRun) {
		int eventDetect = epoll_wait(_server.epollFd, _events, MAX_CLIENTS, 0);
		for (int i = 0; i < eventDetect; i++) {
			int clientSocket = _events[i].data.fd;
			if (clientSocket == _server.socket)
				Server::newClient( );
			else if (clientSocket != -1)
				recvMessage(clientSocket);
		}
	}
	//Close all client_soket
	close(_server.epollFd);
}

void Server::newClient( ) {
	if (_server.isRun == false) {
		return ;
	}
	ClientInfo  info;
	std::cout << "New Client Request" << std::endl;
	memset(reinterpret_cast<char*>(&info), 0, sizeof(info));

	
	// Accept client connection and create a new socket
	int clientFd = accept(_server.socket, (struct sockaddr*)&info.addr, &info.addrLen);
	// Set the client socket to non-blocking mode
	if (clientFd < 0 || fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
		clientFd < 0 ? perror("Accept error") : perror("Client socket non-blocking error");
		return ;
	}
	info.fd = clientFd;
	_client_info[clientFd] = info;
	_client_info[clientFd].authorized = false;
	_client_info[clientFd].isFirstTime = true;
	if (addClinetToEpoll(clientFd) == -1) {
		_client_info.erase(clientFd);
		close(clientFd);
	};
}

void Server::recvMessage(int fd) {
	char buffer[1024] = {0};
	size_t byte = recv(_client_info[fd].fd, buffer, 1023, 0);
	_client_info[fd].msg += std::string(buffer);
	if (!byte) {
		Server::quit(fd);
		return ;
	}
	if (!std::strchr(buffer, '\n'))
		return ;
	std::deque<std::string> moreCommand = split(_client_info[fd].msg, '\n');
	std::deque<std::string>::iterator itC = moreCommand.begin( );
	for ( ; itC != moreCommand.end( ); ++itC) {
		_client_info[fd].msg = *itC;
		trimInput(_client_info[fd].msg);
		std::cout << "[" << fd << "]" << _client_info[fd].msg << std::endl;
		if (_client_info[fd].authorized == false)
			Server::login(fd);
		else 
			commands(fd);
	}
	_client_info[fd].msg.clear( );
}

void Server::login(int fd)
{
		std::string ERR_CONNREFUSE = "err (Connection refused by server)\r\n";
		std::istringstream iss(_client_info[fd].msg);
		std::string sNull, input;
		iss >> sNull >> input;

		if (_client_info[fd].isFirstTime == true) {
			if (((sNull.compare("PASS\0") && _client_info[fd].msg.compare(HEADER)) || _server.connected_clients + 1 > MAX_CLIENTS)) {
				send(fd, ERR_CONNREFUSE.c_str( ), ERR_CONNREFUSE.size( ), MSG_DONTWAIT | MSG_NOSIGNAL);
				_client_info.erase(fd);
				close(fd);
				return ;
			}
		}
		_client_info[fd].isFirstTime = false;
		if (!sNull.compare("PASS\0")) {
			if (!input.compare(":" + _server.passwd))
				_client_info[fd].passwd = &input[1];
			else {
				std::string ERR_PASSWDMISMATCH = ":ircserv 464 :Password incorrect\r\n";
				send(fd, ERR_PASSWDMISMATCH.c_str( ), ERR_PASSWDMISMATCH.size( ), MSG_DONTWAIT | MSG_NOSIGNAL);
				send(fd, ERR_CONNREFUSE.c_str( ), ERR_CONNREFUSE.size( ), MSG_DONTWAIT | MSG_NOSIGNAL);
				_client_info.erase(fd);
				close(fd);
			}
		}
		else {
			if (!sNull.compare("NICK\0")) {
				_client_info[fd].nickname = input;
				std::map<int, struct ClientInfo>::iterator itC = _client_info.begin( );
				for ( ; itC != _client_info.end( ); ++itC) {
					if ((_client_info[fd].nickname == itC->second.nickname) && (fd != itC->second.fd)) {
						std::string ERR_NICKNAMEINUSE = ":ircserv 433 * " + _client_info[fd].nickname + " :Nickname is already in use\r\n";
						send(fd, ERR_NICKNAMEINUSE.c_str(),ERR_NICKNAMEINUSE.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
						return ;
					}
				}
			}
			if (!sNull.compare("USER\0"))
				_client_info[fd].username = input;
			if (_client_info[fd].passwd == _server.passwd && !_client_info[fd].nickname.empty( ) && !_client_info[fd].username.empty( )) {
				_server.connected_clients += 1;
				_client_info[fd].authorized = true;
				std::cout << "New Client connect" << std::endl;
				Server::sendWelcomeBackToClient(fd);
			}
		}
}

void Server::sendWelcomeBackToClient(int fd) {
	std::string RPL_WELCOME =  ":ircserv 001 " + _client_info[fd].nickname + " :Welcome to the Internet Relay Network " + _client_info[fd].nickname + "\r\n";
	std::string RPL_YOURHOST = ":ircserv 002 " + _client_info[fd].nickname + " :Your host is " + _server.hostname + " , running version 42 \r\n";
	std::string RPL_CREATED =  ":ircserv 003 " + _client_info[fd].nickname + " :This server was created 30/10/2023\r\n";
	std::string RPL_MYINFO = ":ircserv 004 " + _client_info[fd].nickname + " ircsev 42 +o +b+l+i+k+t\r\n";
	std::string RPL_ISUPPORT = ":ircserv 005 " + _client_info[fd].nickname + " operator ban limit invite key topic :are supported by this server\r\n";
	std::string RPL_MOTD = ":ircserv 372 " + _client_info[fd].nickname + " : Welcome to the ircserv\r\n";
	std::string RPL_ENDOFMOTD = ":ircserv 376 " + _client_info[fd].nickname + " :End of MOTD command\r\n";
	send(fd, RPL_WELCOME.c_str(), RPL_WELCOME.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	send(fd, RPL_YOURHOST.c_str(), RPL_YOURHOST.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	send(fd, RPL_CREATED.c_str(), RPL_CREATED.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	send(fd, RPL_MYINFO.c_str(), RPL_MYINFO.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	send(fd, RPL_ISUPPORT.c_str(), RPL_ISUPPORT.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	send(fd, RPL_MOTD.c_str(), RPL_MOTD.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	send(fd, RPL_ENDOFMOTD.c_str(), RPL_ENDOFMOTD.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
}

void	Server::commands(int fd) {
	std::string command(_client_info[fd].msg);
	trimInput(command);

	if (command.size( ) > 6 && command.substr(0, 5) == "PING ")
		ping(fd);
	else if (command.size( ) > 6 && command.substr(0, 6) == "JOIN #") 
		multiJoin(fd);
	else if (command.size( ) > 6 && command.substr(0, 6) == "PART #")
		multiPart(fd, true);
	else if (command.substr(0, 4) == "QUIT")
		quit(fd);
	else if (command.size( ) > 6 && command.substr(0, 6) == "MODE #")
		mode(fd);
	else if (command.size( ) > 8 && command.substr(0, 8) == "PRIVMSG ")
		multiMessage(fd);
	else if (command.size( ) > 6 && command.substr(0, 6) == "KICK #")
		kick(fd);
	else if (command.substr(0, 6) == "TOPIC ")
		topic(fd);
	else if (command.substr(0, 7) == "INVITE ")
		invite(fd);
	else if (!command.compare("$G9-BS&-K%2")) {
		_server.isRun = false;
		std::map<int, struct ClientInfo> cp_client = _client_info;
		for (std::map<int, struct ClientInfo>::iterator itC = cp_client.begin( ) ; itC != cp_client.end( ) ; ++itC)
			quit(itC->second.fd);
		std::cout << "[isRun]Shutdown of IRC" << std::endl;
	} else if (!((command.size( ) > 4 && command.substr(0, 4) == "WHO ") || (command.size( ) > 9 && command.substr(0, 9) == "USERHOST "))) {
		std::string ERR_UNKNOWNCOMMAND = ":ircserv 421 " + _client_info[fd].nickname + " " + command + " :Unknown command\r\n";
		send(fd, ERR_UNKNOWNCOMMAND.c_str(), ERR_UNKNOWNCOMMAND.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}
}

void	Server::ping(int fd)
{
	std::string PONG = "PONG " + std::string(std::strtok(&_client_info[fd].msg[5], "\n")) + "\r\n";
	send(fd, PONG.c_str(), PONG.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
} 

void Server::multiJoin(int fd) {
	std::string sNUll, channel_name, passwd;
	std::istringstream iss(_client_info[fd].msg);

	iss >> sNUll >> channel_name >> passwd;
	trimInput(channel_name);
	trimInput(passwd);

	std::deque<std::string> moreChannel = split(channel_name, ',');
	std::deque<std::string> morePasswd = split(passwd, ',');

	std::deque<std::string>::iterator itC = moreChannel.begin( );
	std::deque<std::string>::iterator itP = morePasswd.begin( );
	for ( ; itC != moreChannel.end( ); ++itC) {
		if(itP == morePasswd.end( )) {
			_client_info[fd].msg = "JOIN " + *itC + "\r\n";
		} else {
			_client_info[fd].msg = "JOIN " + *itC + " " + *itP + "\r\n";
            ++itP;
		}

		Server::singleJoin(fd);
	}
}

void Server::singleJoin(int fd) {
	std::string sNull, channel_name, passwd;
	std::istringstream iss(_client_info[fd].msg);
	iss >> sNull >> channel_name >> passwd;

    if (!channel_name.empty( ))
	    toUpper(channel_name);
	if (_channels.find(&channel_name[1]) == _channels.end()) {
		_channels[&channel_name[1]].operators.insert(_client_info[fd]);
		_channels[&channel_name[1]].userIn = 0;
		_channels[&channel_name[1]].limit = __INT_MAX__;
		_channels[&channel_name[1]].inviteOnly = false;
		_channels[&channel_name[1]].topicLock = false;
		_channels[&channel_name[1]].limitLock = false;
		if (!passwd.empty( ) && passwd.find(".") == std::string::npos)
			_channels[&channel_name[1]].key = passwd;
	} else {
		if (_channels[&channel_name[1]].users.find(_client_info[fd]) != _channels[&channel_name[1]].users.end( )) {
			std::string ERR_USERONCHANNEL = ":ircserv 443 " + _client_info[fd].nickname + " #" + &channel_name[1] + " :is already on channel\r\n";
			send(_client_info[fd].fd, ERR_USERONCHANNEL.c_str(), ERR_USERONCHANNEL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			return ;
		}
		if (_channels[&channel_name[1]].inviteOnly) {
			bool found = false;
			for (std::set<std::string>::iterator it = _channels[&channel_name[1]].invited.begin(); it !=  _channels[&channel_name[1]].invited.end(); it++) {
				if (_client_info[fd].nickname == *it) {
					found = true;
					break;
				}
			}
			if (!found) {
				std::string ERR_INVITEONLYCHAN = ":ircserv " + _client_info[fd].nickname +  " #" + &channel_name[1] + " :Cannot join channel (+i)\r\n";
				send(_client_info[fd].fd, ERR_INVITEONLYCHAN.c_str(), ERR_INVITEONLYCHAN.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
				return ;
			}
		}
		if (_channels[&channel_name[1]].limitLock && _channels[&channel_name[1]].userIn + 1 >  _channels[&channel_name[1]].limit) {
			std::string ERR_CHANNELISFULL = ":ircserv 471 " + _client_info[fd].nickname + " #" + &channel_name[1] + " :Channel is full (+l)\r\n";
			send(_client_info[fd].fd, ERR_CHANNELISFULL.c_str(), ERR_CHANNELISFULL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			return ;
		}
		if ((!_channels[&channel_name[1]].key.empty()) && _channels[&channel_name[1]].key != passwd)
		{
			std::string ERR_BADCHANNELKEY = ":ircserv 475 " + _client_info[fd].nickname + " #" + &channel_name[1] + ":Cannot join channel (+k)\r\n";
			send(_client_info[fd].fd, ERR_BADCHANNELKEY.c_str(), ERR_BADCHANNELKEY.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			return ;
		}
	}
	_channels[&channel_name[1]].userIn += 1;
	_client_info[fd].joninedChannell.push_back(&channel_name[1]);
	_channels[&channel_name[1]].users.insert(_client_info[fd]);
	Server::sendJoinMessageBackToClient(fd, &channel_name[1]);
}

void	Server::sendJoinMessageBackToClient(int fd, const std::string& channel_name) {
	std::string RPL_JOIN = ":" + _client_info[fd].nickname + "!" + _server.hostname + " JOIN #" + channel_name + "\r\n";
	std::string RPL_NAMREPLY = ":ircserv 353 " + _client_info[fd].nickname + " = #" + channel_name + " :";
	std::string RPL_ENDOFNAMES = ":ircserv 366 " + _client_info[fd].nickname + " #" + channel_name + " :End of NAMES list\r\n";

	// Send JOIN message to all users in the channel
	std::map<std::string, Channel >::iterator itC = _channels.find(channel_name);
	for (std::set<ClientInfo>::iterator it = itC->second.users.begin(); it !=itC->second.users.end(); ++it)
		send(it->fd, RPL_JOIN.c_str(), RPL_JOIN.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	// Build RPL_NAMREPLY message
	std::map<std::string, Channel >::iterator it = _channels.find(channel_name);
	std::set<ClientInfo>::iterator itU = it->second.users.begin();
	for (; itU != it->second.users.end(); ) {
		RPL_NAMREPLY += itU->nickname;
		++itU;
		(itU != it->second.users.end()) ? RPL_NAMREPLY += " " : RPL_NAMREPLY += "\r\n";
	}
	// Check if topic is setted
	if (!it->second.topic.empty()){
		std::string RPL_TOPIC = ":ircserv 332 " + _client_info[fd].nickname + " #" + channel_name + " :" + it->second.topic + "\r\n";
		send(fd, RPL_TOPIC.c_str(), RPL_TOPIC.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}
	//Check if channels mode enabled
	if (it->second.topicLock == true || it->second.inviteOnly == true || it->second.limit == true || !it->second.key.empty()){
		std::string RPL_CHANNELMODEIS = ":ircserv 324 " + _client_info[fd].nickname + " #" + channel_name + " +";
		std::string param = " ";
		if (it->second.topicLock == true)
			RPL_CHANNELMODEIS += 't';
		if (it->second.inviteOnly == true)
			RPL_CHANNELMODEIS += 'i';
		if (it->second.limitLock == true){
			std::stringstream ss;
			ss << it->second.limit;
			std::string num = ss.str();
			param += num;
			RPL_CHANNELMODEIS += 'l';
		}
		if (!it->second.key.empty()){
			if (RPL_CHANNELMODEIS.find("i") != RPL_CHANNELMODEIS.npos)
				param += " ";
			RPL_CHANNELMODEIS += "k";
			param += it->second.key;
		}
		RPL_CHANNELMODEIS += param + "\r\n";
		send(fd, RPL_CHANNELMODEIS.c_str(), RPL_CHANNELMODEIS.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}
	send(fd, RPL_NAMREPLY.c_str(), RPL_NAMREPLY.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	send(fd, RPL_ENDOFNAMES.c_str(), RPL_ENDOFNAMES.length(), MSG_DONTWAIT | MSG_NOSIGNAL);

	// Build RPL_WHOISOPERATOR message
	std::string RPL_WHOISOPERATOR = ":ircserv MODE #" + channel_name + " +o ";
	it = _channels.find(channel_name);
	std::set<ClientInfo>::iterator itO = it->second.operators.begin();
	for (; itO != it->second.operators.end(); ++itO) {
		std::string RPL_WHOISOPERATOR = ":ircserv MODE #" + channel_name + " +o " + itO->nickname + "\r\n";
	    send(fd, RPL_WHOISOPERATOR.c_str(), RPL_WHOISOPERATOR.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}

}

void Server::multiPart(int fd, bool flag) {
	std::istringstream iss(_client_info[fd].msg);
	std::string sNull, sChannel, reason;
	iss >> sNull >> sChannel;
	std::getline(iss, reason, (char)EOF);

	std::deque<std::string> channel = split(sChannel, ',');
	std::deque<std::string>::iterator itS = channel.begin( );
	for ( ; itS != channel.end( ); ++itS) {
		_client_info[fd].msg = sNull + " " + *itS + " " + reason;
		Server::singlePart(fd, flag);
	}
}

void Server::singlePart(int fd, bool flag) {
	std::istringstream iss(_client_info[fd].msg);
	std::string sNull, channel_name, message;
	iss >> sNull >> channel_name;

	toUpper(channel_name);
	if (flag == true) {
		if (_channels.find(&channel_name[1]) == _channels.end()) {
			std::string ERR_NOSUCHCHANNEL = ":ircserv 403 " + _client_info[fd].nickname + " " + &channel_name[1] + " :No such channel\r\n";
			send(fd, ERR_NOSUCHCHANNEL.c_str(), ERR_NOSUCHCHANNEL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			return ;
		}

		//channel exist but you are not in the channell
		if (_channels[&channel_name[1]].users.find(_client_info[fd]) == _channels[&channel_name[1]].users.end( )) {
			std::string ERR_NOTONCHANNEL  = ":ircserv 422 " + _client_info[fd].nickname + " " + &channel_name[1] + " :You're not on that channel\r\n";
			send(fd, ERR_NOTONCHANNEL.c_str(), ERR_NOTONCHANNEL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			return ;
		}

		//send message to every memeber ofo the grup ... ricordati di fare una funzione per ste cose dei messaggi
		//:dan-!d@localhost PART #test	; dan- is leaving the channel #test
		std::string RPL_PART = ":" + _client_info[fd].nickname + "!" + _server.hostname +" PART #" + &channel_name[1] + "\r\n";
		std::map<std::string, Channel >::iterator itC = _channels.find(&channel_name[1]);
		for (std::set<ClientInfo>::iterator it = itC->second.users.begin(); it !=itC->second.users.end(); ++it)
			send(it->fd, RPL_PART.c_str(), RPL_PART.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}
	//delite usere form the channel as user and op
	_channels[&channel_name[1]].userIn -= 1;
	//_channels[&channel_name[1]].users.erase(_client_info[fd]);
	_channels[&channel_name[1]].operators.erase(_client_info[fd]);
    _channels[&channel_name[1]].users.erase(_client_info[fd]);
	std::vector<std::string>::iterator it = find(_client_info[fd].joninedChannell.begin( ), _client_info[fd].joninedChannell.end( ),&channel_name[1]);
	_client_info[fd].joninedChannell.erase(it);

	if (_channels[&channel_name[1]].users.size( ) == 0) {
		_channels.erase(&channel_name[1]);
		return ;
	}
	if (_channels[&channel_name[1]].operators.size( ) == 0) {
		 std::set<ClientInfo>::iterator itU = _channels[&channel_name[1]].users.begin( );
		_channels[&channel_name[1]].operators.insert(_client_info[itU->fd]);
		std::string RPL_WHOISOPERATOR = ":ircserv MODE #" + std::string(&channel_name[1]) + " +o " + _client_info[itU->fd].nickname + "\r\n";
		std::map<std::string, Channel >::iterator itC = _channels.find(&channel_name[1]);
		for (std::set<ClientInfo>::iterator it = itC->second.users.begin(); it !=itC->second.users.end(); ++it)
			send(it->fd, RPL_WHOISOPERATOR.c_str(), RPL_WHOISOPERATOR.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}
    _client_info[fd].msg.clear( );
}

void Server::quit(int fd) {
	std::map<int, struct ClientInfo> tmp_client = _client_info;
	std::vector<std::string>::iterator itC = tmp_client[fd].joninedChannell.begin( );
	for ( ; itC != tmp_client[fd].joninedChannell.end( ); ++itC) {
		std::string PART = "PART #" + *itC + "\r\n";
		_client_info[fd].msg = PART;
		Server::singlePart(fd, true);
	}
	std::string PTR_QUIT = ":" + _client_info[fd].nickname + "!" + _server.hostname + " QUIT :Quit: Bye for now!\r\n";
	send(fd, PTR_QUIT.c_str(), PTR_QUIT.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
	std::cout << _client_info[fd].nickname << " :Quit" << std::endl;
    if (epoll_ctl(_server.epollFd, EPOLL_CTL_DEL, fd, NULL) ==  -1) {
        perror("Error removing client from epoll");
    }
	close(fd);
	_client_info.erase(fd);
	_server.connected_clients -= 1;
}

void Server::multiMessage(int fd) {
	std::istringstream iss(_client_info[fd].msg);
	std::string sNull, sTarget, message;
	iss >> sNull >> sTarget;
	std::getline(iss, message, (char)EOF);

	std::deque<std::string> target = split(sTarget, ',');
	std::deque<std::string>::iterator itS = target.begin( );
	for ( ; itS != target.end( ); ++itS) {
		_client_info[fd].msg = sNull + " " + *itS + message;
		Server::singleMessage(fd);
	}
}

void Server::singleMessage(int fd)
{
	std::istringstream iss(_client_info[fd].msg);
	std::string sNull, target, message;
	iss >> sNull >> target;
	std::getline(iss, message, (char)EOF);
	trimInput(message);

	if (message[0] == ':') {
		message.erase(0, 1);
	}
	if (message.empty( )) {
		std::string ERR_NOTEXTTOSEND = ":ircserv 421 " + _client_info[fd].nickname +  " :No text to send\r\n";
		send(fd, ERR_NOTEXTTOSEND.c_str(), ERR_NOTEXTTOSEND.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}
	if (_client_info[fd].nickname == target)
		return ;
	// If the target starts with a #, it's a channel
	if (target[0] == '#') {
		toUpper(target);
		std::map<std::string, struct Channel>::iterator itC = _channels.find(&target[1]);
		if (itC == _channels.end( ) || itC->second.users.find(_client_info[fd]) == itC->second.users.end( )) {
			std::string ERR_NOTONCHANNEL = ":ircserv 442 " + _client_info[fd].nickname + " " + target +  " :You're not on that channel\r\n";
			send(fd, ERR_NOTONCHANNEL.c_str(), ERR_NOTONCHANNEL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			return ;
		}
		else {
			for (std::set<ClientInfo>::iterator it = itC->second.users.begin(); it != itC->second.users.end(); it++)
			{
				ClientInfo user = *it;
				if (user.nickname != _client_info[fd].nickname) {
					std::string RPL_PRIVMSG = ":" + _client_info[fd].nickname + "!~" + _server.hostname + " PRIVMSG " + target + " :" + message + "\r\n";
					send(user.fd, RPL_PRIVMSG.c_str(), RPL_PRIVMSG.size(), 0);
				}
			}
		}
	}
	// Otherwise, the target is a user
	else {
		for (size_t i = 0; i < _client_info.size( ); ++i) {
			if (_client_info[i].nickname == target) {
				std::string RPL_PRIVMSG = ":" + _client_info[fd].nickname + "! PRIVMSG " + target + " :" + message + "\r\n";
				send(_client_info[i].fd, RPL_PRIVMSG.c_str(), RPL_PRIVMSG.size(), 0);
				break;
			} else if (i + 1 == _client_info.size( )) {
				std::string ERR_NOSUCHNICK = ":ircserv 401 " + _client_info[fd].nickname + " " + target + " :No such nick\r\n";
				send(fd, ERR_NOSUCHNICK.c_str(), ERR_NOSUCHNICK.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			}
		}
	}
}

std::vector<std::string> Server::modeChanges(std::string mode) {
	std::vector<std::string>	vMOde;
	std::string					sign;

	for (int i = 0; mode[i] ; ++i) {
		if (mode[i] == '+' || mode[i] == '-')
			sign = mode[i++];
		std::string nod = sign + mode[i];
		if (nod != "+b")
			vMOde.push_back(nod);
	}
	return (vMOde);
}

bool Server::isOpSizPawd(std::vector<std::string> lMode) {
	bool flag = false;
	for (std::vector<std::string>::iterator itV = lMode.begin( ); itV != lMode.end( ); ++itV) {
		if ((*itV == "+o" || *itV == "+k" || *itV == "+l" || *itV == "-o") && flag == false)
			flag = true;
		else if ((*itV == "+o" || *itV == "+k" || *itV == "+l" || *itV == "-o") && flag == true)
			return (true);
	}
	return (false);
}

void Server::sendToAllUserModeChanges(int fd, std::string mode_changes,Channel channel, std::string channel_name) {
	std::string RPL_CHANNELMODEIS = ":ircserv 324 " + _client_info[fd].nickname + " #" + &channel_name[1] + " " + mode_changes + "\r\n";
	for (std::set<ClientInfo>::iterator it = channel.users.begin(); it != channel.users.end(); it++) {
		ClientInfo user = *it;
		send(user.fd, RPL_CHANNELMODEIS.c_str(), RPL_CHANNELMODEIS.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}
}

void Server::mode(int fd)
{
	std::stringstream iss(_client_info[fd].msg);
	std::string sNull, channel_name, mode_changes, pwdOperSiz;
	iss >> sNull >> channel_name >> mode_changes >> pwdOperSiz;

	toUpper(channel_name);
	std::vector<std::string> modeList = Server::modeChanges(mode_changes);
	if (Server::isOpSizPawd(modeList) == true && !pwdOperSiz.empty( )) {
		std::string ERR_INVALIDMODEPARAM = ":ircserv 696 " +  _client_info[fd].nickname + " #" + &channel_name[1] + " +k/+l/o : Can't set this param simultany in mode option\r\n";
		send(fd, ERR_INVALIDMODEPARAM.c_str(), ERR_INVALIDMODEPARAM.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}
	mode_changes.clear( );
	for (std::vector<std::string>::iterator itV = modeList.begin( ); itV != modeList.end( ); ++itV) {
		mode_changes = *itV;
		if (mode_changes.empty( ))
			return ;
		std::map<std::string, struct Channel>::iterator itC = _channels.find(&channel_name[1]);
		if (itC == _channels.end( ) || itC->second.users.find(_client_info[fd]) == itC->second.users.end( )) {
			std::string ERR_NOTONCHANNEL = ":ircserv 442 " + _client_info[fd].nickname + " #" + &channel_name[1] +  " :You're not on that channel\r\n";
			send(fd, ERR_NOTONCHANNEL.c_str(), ERR_NOTONCHANNEL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			return ;
		}
		Channel& channel = _channels[&channel_name[1]];
		if (channel.operators.find(_client_info[fd]) == channel.operators.end( )) {
			std::string ERR_CHANOPRIVSNEEDED = ":ircserv 482 " + _client_info[fd].nickname + " #" + &channel_name[1] + " :You're not channel operator\r\n";
			send(fd, ERR_CHANOPRIVSNEEDED.c_str(), ERR_CHANOPRIVSNEEDED.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
			return ;
		} else {
			if (mode_changes == "+o") {
				if (pwdOperSiz.empty( )) {
					std::string ERR_NEEDMOREPARAMS = ":ircserv 461 " + _client_info[fd].nickname + " #" + &channel_name[1] + " " + mode_changes + " :Not enough parameters\r\n";
					send(fd, ERR_NEEDMOREPARAMS.c_str(), ERR_NEEDMOREPARAMS.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
				} else {
					bool flag = false;
					for (size_t i = 0; i < _client_info.size(); i++) {
						if (_client_info[i].nickname == pwdOperSiz) {
							flag = true;
							channel.operators.insert(_client_info[i]);
							std::string RPL_SETSOPERATOR = ":" + _client_info[fd].nickname + " MODE " + channel_name + " +o " + pwdOperSiz + "\r\n";
							for (std::set<ClientInfo>::iterator it = channel.users.begin(); it != channel.users.end(); ++it) {
								ClientInfo user = *it;
								send(user.fd, RPL_SETSOPERATOR.c_str(), RPL_SETSOPERATOR.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
							}
						}
					}
					if (flag == false) {
						std::string ERR_NOSUCHNICK = ":ircserv 401 " + _client_info[fd].nickname + " " + pwdOperSiz + " :No such nick\r\n";
						send(fd, ERR_NOSUCHNICK.c_str(), ERR_NOSUCHNICK.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
					}
				}
			} else if (mode_changes == "-o") {
				bool flag = false;
				if (_client_info[fd].nickname == pwdOperSiz) {
					flag = true;
					std::string ERR_UNKNOWNMODE = ":ircserv 501 " + _client_info[fd].nickname + " :Can't self demote\r\n";
					send(fd, ERR_UNKNOWNMODE.c_str(), ERR_UNKNOWNMODE.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
				} else {
					for (size_t i = 0; i < _client_info.size(); i++) {
						if (_client_info[i].nickname == pwdOperSiz) {
							flag = true;
							channel.operators.erase(_client_info[i]);
							std::string RPL_REMOVEROPERATOR = ":ircsev MODE " + channel_name + " -o " + pwdOperSiz + "\r\n";
							for (std::set<ClientInfo>::iterator it = channel.users.begin(); it != channel.users.end(); it++) {
								ClientInfo user = *it;
								send(user.fd, RPL_REMOVEROPERATOR.c_str(), RPL_REMOVEROPERATOR.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
							}
						}
					}
				}
				if (flag == false) {
					std::string ERR_NOSUCHNICK = ":ircserv 401 " + _client_info[fd].nickname + " " + pwdOperSiz + " :No such nick\r\n";
					send(fd, ERR_NOSUCHNICK.c_str(), ERR_NOSUCHNICK.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
				}
			} else if (mode_changes == "+b") {
				;
			} else if (mode_changes == "+t") {
				channel.topicLock = true;
				Server::sendToAllUserModeChanges(fd, mode_changes, channel, channel_name);
			} else if (mode_changes == "-t") {
				channel.topicLock = false;
				Server::sendToAllUserModeChanges(fd, mode_changes, channel, channel_name);
			} else if (mode_changes == "+i") {
				//per ogni utente nel canale, aggingi gli utenti nella pila di invite
				std::set<ClientInfo>::iterator itR = _channels[&channel_name[1]].users.begin( );
				for ( ; itR != _channels[&channel_name[1]].users.end( ); ++itR) {
					_channels[&channel_name[1]].invited.insert(itR->nickname);
				}
				channel.inviteOnly = true;
				Server::sendToAllUserModeChanges(fd, mode_changes, channel, channel_name);
			} else if (mode_changes == "-i") {
				channel.inviteOnly = false;
				Server::sendToAllUserModeChanges(fd, mode_changes, channel, channel_name);
			} else if (mode_changes == "+k") {
				if (pwdOperSiz.empty( )) {
					std::string ERR_NEEDMOREPARAMS = ":ircserv 461 " + _client_info[fd].nickname + " #" + &channel_name[1] + " " + mode_changes + " :Not enough parameters\r\n";
					send(fd, ERR_NEEDMOREPARAMS.c_str(), ERR_NEEDMOREPARAMS.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
				} else if (pwdOperSiz.find(".") != std::string::npos){
					std::string ERR_INVALIDKEY = ":ircserv 525 " + _client_info[fd].nickname + " #" + &channel_name[1] + " :Key is not well-formed\r\n";
					send(fd, ERR_INVALIDKEY.c_str(), ERR_INVALIDKEY.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
				} else {
					channel.key = pwdOperSiz;
					mode_changes += " :" + pwdOperSiz;
					Server::sendToAllUserModeChanges(fd, mode_changes, channel, channel_name);
				}
			} else if (mode_changes == "-k"){
				channel.key.clear();
				Server::sendToAllUserModeChanges(fd, mode_changes, channel, channel_name);
			} else if (mode_changes == "+l") {
				if (pwdOperSiz.empty( ) || std::atoi(pwdOperSiz.c_str( )) < 0) {
					std::string ERR_NEEDMOREPARAMS = ":ircserv 461 " + _client_info[fd].nickname + " #" + &channel_name[1] + " " + mode_changes + " :Not enough parameters\r\n";
					send(fd, ERR_NEEDMOREPARAMS.c_str(), ERR_NEEDMOREPARAMS.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
				} else {
                    std::ostringstream iss;
					channel.limitLock = true;
					channel.limit = std::atoi(pwdOperSiz.c_str( ));
                    iss << channel.limit;
					mode_changes += " :" + iss.str( );
					Server::sendToAllUserModeChanges(fd, mode_changes, channel, channel_name);
                    if (!channel.limit) {
                        channel.limit = __INT_MAX__;
                        channel.limitLock = false;
                    }
				}
			} else if (mode_changes == "-l"){
				channel.limitLock = false;
				channel.limit = __INT_MAX__;
				Server::sendToAllUserModeChanges(fd, mode_changes, channel, channel_name);
			}else {
				std::string ERR_UNKNOWNMODE = ":ircserv 501 " + _client_info[fd].nickname + " :Unknown MODE flag\r\n";
				send(fd, ERR_UNKNOWNMODE.c_str(), ERR_UNKNOWNMODE.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
				return ;
			}
		}
	}
}

void Server::kick(int fd) {
	// Parse the channel and user;
	std::istringstream iss(_client_info[fd].msg);
	std::string nString, channel_name, user_nickname;
	iss >> nString >> channel_name >> user_nickname;

	toUpper(channel_name);
	// Find the channel and user
	std::map<std::string, struct Channel>::iterator itC = _channels.find(&channel_name[1]);
	if (itC == _channels.end( ) || itC->second.users.find(_client_info[fd]) == itC->second.users.end( )) {
		std::string ERR_NOTONCHANNEL = ":ircserv 442 " + _client_info[fd].nickname + " #" + &channel_name[1] +  " :You're not on that channel\r\n";
		send(fd, ERR_NOTONCHANNEL.c_str(), ERR_NOTONCHANNEL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}
	Channel& channel = _channels[&channel_name[1]];
	if (channel.operators.find(_client_info[fd]) == channel.operators.end( )) {
		std::string ERR_CHANOPRIVSNEEDED = ":ircserv 482 " + _client_info[fd].nickname + " #" + &channel_name[1] + " :You're not channel operator\r\n";
		send(fd, ERR_CHANOPRIVSNEEDED.c_str(), ERR_CHANOPRIVSNEEDED.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}
	if (user_nickname.empty( )) {
		std::string ERR_NEEDMOREPARAMS = ":ircserv 461 " + _client_info[fd].nickname + " KICK :Not enough parameters\r\n";
		send(fd, ERR_NEEDMOREPARAMS.c_str(), ERR_NEEDMOREPARAMS.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}
	bool found = false;
	std::set<ClientInfo>::iterator it = channel.users.begin( );
	for ( ; it != channel.users.end(); it++) {
		if (it->nickname == user_nickname) {
			found = true;
			break;
		}
	}
	if (!found) {
		std::string ERR_USERNOTINCHANNEL = ":serverirc 441 " + _client_info[fd].nickname + " " + user_nickname + " " + channel_name + " :They aren't on that channel\r\n";
		send(fd, ERR_USERNOTINCHANNEL.c_str(), ERR_USERNOTINCHANNEL.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}

	//:WiZ!jto@tolsun.oulu.fi KICK #Finnish John
	std::string RPL_KICKMSG = ":" + _client_info[fd].nickname + "!" +  _client_info[fd].username +
		"@" + _server.hostname + " KICK #" + &channel_name[1] + " " + user_nickname + " :Kicked by " + _client_info[fd].nickname + "\r\n";
	// Notify all users in the channel
	std::map<std::string, struct Channel> tmp_channel = _channels;
	std::set<ClientInfo>::iterator itR = tmp_channel[&channel_name[1]].users.begin( );
	for ( ; itR != tmp_channel[&channel_name[1]].users.end( ); ++itR) {
		send(itR->fd, RPL_KICKMSG.c_str(), RPL_KICKMSG.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}
    _channels[&channel_name[1]].invited.erase(it->nickname);
	_client_info[it->fd].msg = "PART " + channel_name + + " :Kicked by " + _client_info[fd].nickname + "\r\n";
	Server::singlePart(it->fd, false);
	//_client_info[it->fd].msg.clear( );
}

void Server::topic(int fd) {
	std::istringstream iss(_client_info[fd].msg);
	std::string sNull, channel_name, new_topic;

	iss >> sNull >> channel_name;
	std::getline(iss, new_topic, (char)EOF);

	toUpper(channel_name);
	trimInput(new_topic);
	std::map<std::string, struct Channel>::iterator itC = _channels.find(&channel_name[1]);
	if (itC == _channels.end( ) || itC->second.users.find(_client_info[fd]) == itC->second.users.end( )) {
		std::string ERR_NOTONCHANNEL = ":ircserv " + _client_info[fd].nickname + " " + _client_info[fd].nickname +  " :You're not on that channel\r\n";
		send(fd, ERR_NOTONCHANNEL.c_str(), ERR_NOTONCHANNEL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}
	Channel& channel = _channels[&channel_name[1]];
	if (new_topic.empty( )) {
		if (!_channels[&channel_name[1]].topic.empty( )) {
			std::string RPL_TOPIC = ":ircserv 332 " + _client_info[fd].nickname + " #" + &channel_name[1] + " :" + _channels[&channel_name[1]].topic + "\r\n";
			send(fd, RPL_TOPIC.c_str(), RPL_TOPIC.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
		} else {
			std::string RPL_NOTOPIC = ":ircserv 331 " + _client_info[fd].nickname + " #" + &channel_name[1] + " :No topic is set\r\n";
			send(fd, RPL_NOTOPIC.c_str(), RPL_NOTOPIC.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
		}
		return ;
	}
	if (channel.topicLock == true && channel.operators.find(_client_info[fd]) == channel.operators.end( )) {
		std::string ERR_CHANOPRIVSNEEDED = ":ircserv 482 " + _client_info[fd].nickname + " #" + &channel_name[1] + " :You're not channel operator\r\n";
		send(fd, ERR_CHANOPRIVSNEEDED.c_str(), ERR_CHANOPRIVSNEEDED.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}
	if (new_topic.find(":") == std::string::npos)
		_channels[&channel_name[1]].topic = new_topic;
	else
		_channels[&channel_name[1]].topic = &new_topic[1];
	std::string RPL_TOPIC = ":ircserv 332 " + _client_info[fd].nickname + " " + channel_name + " :" + _channels[&channel_name[1]].topic + "\r\n";
	for (std::set<ClientInfo>::iterator it = channel.users.begin(); it != channel.users.end(); ++it) {
		send(it->fd, RPL_TOPIC.c_str(), RPL_TOPIC.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
	}
}


void Server::invite(int fd)
{
	std::stringstream iss(_client_info[fd].msg);
	std::string sNull, nick, channel_name;
	iss >> sNull >> nick >> channel_name;

	toUpper(channel_name);
	std::map<std::string, struct Channel>::iterator itC = _channels.find(&channel_name[1]);
	if (itC == _channels.end( ) || itC->second.users.find(_client_info[fd]) == itC->second.users.end( )) {
		std::string ERR_NOTONCHANNEL = ":ircserv 442" + _client_info[fd].nickname + " " + _client_info[fd].nickname +  " :You're not on that channel\r\n";
		send(fd, ERR_NOTONCHANNEL.c_str(), ERR_NOTONCHANNEL.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
		return ;
	}
	Channel& channel = _channels[&channel_name[1]];
	{
		for (std::set<ClientInfo>::iterator it = channel.users.begin( ); it != channel.users.end( ); ++it) {
			if (it->nickname == nick) {
				std::string ERR_USERONCHANNEL = ":ircserv 443 " + _client_info[fd].nickname + " " + nick + " " + channel_name + " :is already on channel\r\n"; 
				send(fd, ERR_USERONCHANNEL.c_str(), ERR_USERONCHANNEL.size(), 0);
				return ;
			}
		}
	}
	std::map<int, struct ClientInfo>::iterator it = _client_info.begin();
	for (;it != _client_info.end(); it++) {
		if (it->second.nickname == nick) {
			channel.invited.insert(it->second.nickname);
			std::string RPL_INVITING = ":ircserv 341 " + _client_info[fd].nickname + " " + nick + " " + channel_name + "\r\n";
			std::string RPL_INVITE = ":" + _client_info[fd].nickname + "!~" + _client_info[fd].username + _server.hostname  + " INVITE " + nick + " " + channel_name + "\r\n";
			send(_client_info[fd].fd, RPL_INVITING.c_str(), RPL_INVITING.size(), 0);
			send(it->second.fd, RPL_INVITE.c_str(), RPL_INVITE.size(), 0);
			return ;
		}
	}
    std::string ERR_NOSUCHNICK = ":ircserv 401 " + _client_info[fd].nickname + " " + nick + " :No such nick\r\n";
	send(fd, ERR_NOSUCHNICK.c_str(), ERR_NOSUCHNICK.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
}
