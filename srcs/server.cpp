#include "../includes/server.hpp"
#include "../includes/ircserver.hpp"

// Static member initializion
ServerData						Server::_server;
std::map<std::string, Channel>			Server::_channels;
std::map<int, Client>					Server::_clients;
epoll_event 						Server::_events[MAX_CLIENTS + 1];

char				hostname[256] = "";

/**
 * Server construction class
 * 
 * @param port port
 * @param passwd password
 * 
 */
Server::Server(int port, char *passwd) {
	memset(reinterpret_cast<char*>(&_server), 0, sizeof(_server));
	_server.port = port;
	_server.passwd = passwd;
	memset(reinterpret_cast<char*>(&_events), 0, sizeof(_events));
}

Server::~Server( ) { }

/**
 * Server initialization, create socket, allow kernel, bind and listen
 * 
 * @return void 
 */
void Server::serverInit( ) {
	// Create a socket to manage connections and listen on
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
	if (gethostname(hostname, sizeof(hostname)) == -1)
		throw std::runtime_error ("ERROR: gethostname failed");

	// Create a sockaddr_in for the proper port and bind our socket to it
	_server.addr.sin_family = AF_INET;
	_server.addr.sin_port = htons(_server.port); // We will need to convert the port to network byte order
	_server.addr.sin_addr = *((in_addr*) gethostbyname(hostname)->h_addr_list[0]); //inet_addr("ip of local machine");
	_server.IP = inet_ntoa(_server.addr.sin_addr);

	//Damiano

	if (fcntl(_server.socket, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error ("ERROR: Failed to set non-blocking socket");

	// Bind our socket to the port (associate IP e port to socket)
	if (bind(_server.socket, (sockaddr *) &_server.addr, sizeof(_server.addr)) == -1)
		throw std::runtime_error ("ERROR: Failed to bind to socket");

	// Start listening on with the socket
	if (listen(_server.socket, 10000) == -1) // The 5 here is the maximum number of backlog connections
		throw std::runtime_error ("ERROR: listen failed");

	_server.isRun = true;
	std::cout << "Server started and listening on IP " <<  _server.IP << " port " << _server.port << std::endl;
}
/**
 * Add server to epoll
 * 
 * Add a new server socket to epoll instance
 *  
 */
void	Server::addServerToEpoll( ) {
		
	_server.epollFd = epoll_create1(0);
	if (_server.epollFd == -1)
		throw std::runtime_error ("ERROR: poll failed");

	Server::addClientToEpoll(_server.socket);
}

/**
 * ServerisRun
 * 
 * Check if server is run
 * 
 * @return true if server is run, false if server is not run
 * 
 */
bool Server::serverIsRun() {
	if (_server.isRun == false) 
		return false;
	else
		return true;
}

/**
 * Add a client to epoll
 * Add a new client socket to server epoll instance
 * 
 * @return 0 if success, -1 in case of error.
 */
int Server::addClientToEpoll(int newSocket) {
	epoll_event event;

	memset(reinterpret_cast<char*>(&event), 0, sizeof(event));
	event.events = EPOLLIN;
	event.data.fd = newSocket;
	if (epoll_ctl(_server.epollFd, EPOLL_CTL_ADD, newSocket, &event) == -1) {
		perror("Error adding socket to epoll");
		close(_server.epollFd);
		return -1;
	}
	return 0;
}

/**
 * Run server
 * 
 * Run server and wait for new client
 * 
 * @return void 
 */
void Server::runServer( ) {
	while (_server.isRun) {
		int eventDetect = epoll_wait(_server.epollFd, _events, MAX_CLIENTS, -1);
		
		for (int i = 0; i < eventDetect; i++) {
			int clientSocket = _events[i].data.fd;
			if (clientSocket == _server.socket)
				Server::newClient( );
			else if (clientSocket != -1)
				recvMessage(clientSocket);
			else
				exit(EXIT_FAILURE);
		}
		std::cout << "eventDetect: " << eventDetect << std::endl;
	}
	//Close all client_soket
	close(_server.epollFd);
}

/**
 * Add new Client to Server data
 * 
 * add new Client and fd to _clients and add send fd to Epoll to create newsocket
 * 
 * @return void
 *  */
void Server::newClient() {
	if (serverIsRun() == false) {
		return;
	}
	ClientInfo info;
	std::memset(reinterpret_cast<char*>(&info), 0, sizeof(info));  // Azzera la memoria della nuova istanza di ClientInfo
	std::cout << "New Client Request" << std::endl;
	// Accept client connection and create a new socket
	int clientFd = accept(_server.socket, (sockaddr*)&info.addr, &info.addrLen);
	info.fd = clientFd;
	// Set the client socket to non-blocking mode
	if (clientFd < 0 || fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
		clientFd < 0 ? perror("Accept error") : perror("Client socket non-blocking error");
		return;
	}
	_clients.insert(std::make_pair(clientFd, Client(info)));
	if (addClientToEpoll(clientFd) == -1) {
		_clients.erase(clientFd);
		close(clientFd);
	}
}

/**
 * Server reiceves msg from Client and launch login() or commands()
 * 
 * @param fd fd client.
 *  */
void Server::recvMessage(int fd) {
	char buffer[1024] = {0};
	int byte = recv(fd, buffer, 1023, 0);
	if (byte <= 0) {
		quit(fd);
		return ;
	}
	if (_clients.find(fd) != _clients.end())
	{
		std::string clientMsg =_clients[fd].getinfo().msg + buffer;
    	_clients[fd].setMsg(clientMsg);
	}
	else
	{
		std::cout << "Client not found" << std::endl;
		return ;
		exit (EXIT_FAILURE);
	}
	std::string clientMsg =_clients[fd].getinfo().msg;
	if (!std::strchr(buffer, '\n'))
		return ;
	std::deque<std::string> moreCommand = split(clientMsg, '\n');
	std::deque<std::string>::iterator itC = moreCommand.begin( );
	for ( ; itC != moreCommand.end( ); ++itC) {
		clientMsg = *itC;
		trimInput(clientMsg);
		_clients[fd].setMsg(clientMsg);
		std::cout << "[" << fd << "]" << clientMsg << std::endl;
		if (_clients[fd].getinfo().authorized == false)
			login(fd);
		else 
			commands(fd);
		}
		if (_clients.find(fd) != _clients.end())
			_clients[fd].clearMsg();
}

/**
 * Server check if client can join
 * 
 * in this function server check if the client is authorized else checks and connects the client.
 * if isn't the first connection, checks psw read message and launch commands.
 * if some error accours, returns. 
 *  
 * @param fd fd client.
 *  */
void Server::login(int fd)
{
		if (_clients.find(fd) == _clients.end())
			return ;
		std::string ERR_CONNREFUSE = "err (Connection refused by server)\r\n";
		std::istringstream iss(_clients[fd].getinfo().msg);
		std::string sNull, input;
		iss >> sNull >> input;
	
		// printf("sono entrato in login %d : con msg %s\n", fd, _clients[fd].getinfo().msg.c_str());
		// std::cout << "sono sttao autorizzato con fd " << fd << " sono autorizzato? : "  <<  _clients[fd].getinfo().authorized << std::endl;
		if (_clients[fd].getinfo().isFirstTime == true) {
			if (((sNull.compare("PASS\0") && _clients[fd].getinfo().msg.compare(HEADER)) || _server.connected_clients + 1 > MAX_CLIENTS)) {
				send(fd, ERR_CONNREFUSE.c_str( ), ERR_CONNREFUSE.size( ), MSG);
				_clients.erase(fd);
				close(fd);
				return ;
			}
		}
		_clients[fd].setIsFirstTime(false);
		if (!sNull.compare("PASS\0")) {
			if (!input.compare(":" + _server.passwd))
				_clients[fd].setPasswd(&input[1]);
			else {
				std::string ERR_PASSWDMISMATCH = ":ircserv 464 :Password incorrect\r\n";
				send(fd, ERR_PASSWDMISMATCH.c_str( ), ERR_PASSWDMISMATCH.size( ), MSG);
				send(fd, ERR_CONNREFUSE.c_str( ), ERR_CONNREFUSE.size( ), MSG);
				if (_clients.find(fd) != _clients.end())
					_clients.erase(fd);
				close(fd);
			}
		}
		else {
			if (!sNull.compare("NICK\0")) {
				std::map<int, Client>::iterator itC = _clients.begin( );
				for ( ; itC != _clients.end( ); ++itC) {
					if (input == itC->second.Nickname()) {
						printf("cilentfd %d username %s nickname %s\n", itC->first, itC->second.getinfo().username.c_str(), itC->second.getinfo().nickname.c_str());
						std::string ERR_NICKNAMEINUSE = ":ircserv 433 * " + _clients[fd].Nickname() + " :Nickname is already in use\r\n";
						send(fd, ERR_NICKNAMEINUSE.c_str(),ERR_NICKNAMEINUSE.length(), MSG);
						_clients[fd].clearMsg();
						return ;
					}
				}
				_clients[fd].setNickname(input);
			}
			if (!sNull.compare("USER\0"))
				_clients[fd].setUsername(input);
			if (_clients[fd].getinfo().passwd == _server.passwd && !_clients[fd].Nickname().empty( ) && !_clients[fd].getinfo().username.empty( )) {
				_server.connected_clients += 1;
				_clients[fd].setAuthorized(true);
				std::cout << "New Client connect" << std::endl;
				Server::sendWelcomeBackToClient(fd);
				if(_channels.empty())
					addBot();
				_clients[fd].setMsg("JOIN #WELCOME");
				singleJoin(fd);
				_clients[fd].clearMsg();
			}
		}
	}


/**
 * Server send welcomeback to client
 * 
 * @param fd fd client.
 *  */
void Server::sendWelcomeBackToClient(int fd) {
	std::string RPL_WELCOME =  ":ircserv 001 " + _clients[fd].Nickname() + " :Welcome to the Internet Relay Network " + _clients[fd].Nickname() + "\r\n";
	std::string RPL_YOURHOST = ":ircserv 002 " + _clients[fd].Nickname() + " :Your host is " + hostname + " , running version 42 \r\n";
	std::string RPL_CREATED =  ":ircserv 003 " + _clients[fd].Nickname() + " :This server was created 30/10/2023\r\n";
	std::string RPL_MYINFO = ":ircserv 004 " + _clients[fd].Nickname() + " ircsev 42 +o +b+l+i+k+t\r\n";
	std::string RPL_ISUPPORT = ":ircserv 005 " + _clients[fd].Nickname() + " operator ban limit invite key topic :are supported by this server\r\n";
	std::string RPL_MOTD = ":ircserv 372 " + _clients[fd].Nickname() + " : Welcome to the ircserv\r\n";
	std::string RPL_ENDOFMOTD = ":ircserv 376 " + _clients[fd].Nickname() + " :End of MOTD command\r\n";
	send(fd, RPL_WELCOME.c_str(), RPL_WELCOME.length(), MSG);
	send(fd, RPL_YOURHOST.c_str(), RPL_YOURHOST.length(), MSG);
	send(fd, RPL_CREATED.c_str(), RPL_CREATED.length(), MSG);
	send(fd, RPL_MYINFO.c_str(), RPL_MYINFO.length(), MSG);
	send(fd, RPL_ISUPPORT.c_str(), RPL_ISUPPORT.length(), MSG);
	send(fd, RPL_MOTD.c_str(), RPL_MOTD.length(), MSG);
	send(fd, RPL_ENDOFMOTD.c_str(), RPL_ENDOFMOTD.length(), MSG);
}

/**
 * newChannel
 * 
 * create a new channel
 * 
 * @param fd fd client.
 * @param channel_name name of channel.
 * @param passwd password of channel.
 */
void Server::newChannel(int fd, std::string channel_name, std::string passwd)
{
		_channels.insert(std::make_pair(&channel_name[1], Channel(&channel_name[1])));
		_channels[&channel_name[1]].addParticipant(fd, _clients[fd].getinfo());
		if (_channels[&channel_name[1]].noOperators( ))
			_channels[&channel_name[1]].addOperator(_clients[fd].Nickname(), fd);
		if (!passwd.empty( ) && passwd.find(".") == std::string::npos) 
			_channels[&channel_name[1]].setkey(passwd);
}

/**
 * singleJoin
 * 
 * Server join client in a channel
 * 
 * @param fd fd client.
 * 
 */
void Server::singleJoin(int fd) {
	std::string sNull, channel_name, passwd;
	std::istringstream iss(_clients[fd].getinfo().msg);
	iss >> sNull >> channel_name >> passwd;
	std::string ch_name = &channel_name[1];

    if (!channel_name.empty( ))
	    toUpper(channel_name);
	if (_channels.find(ch_name) == _channels.end()) {
		newChannel(fd, channel_name, passwd);
	if (!passwd.empty() && passwd.find(".") == std::string::npos) {
		_channels[ch_name].setkey(passwd);
		_channels[ch_name].sendToAllUserModeChanges(_clients[fd].Nickname(), "+k");
	}
	}else if (_channels[ch_name].wrongPass(_clients[fd].Nickname(), fd, passwd) ||
			_channels[ch_name].onlyInvite(_clients[fd].Nickname(),fd, ch_name) ||
			_channels[ch_name].channelIsFull(_clients[fd].Nickname(),fd, ch_name) ||
			_channels[ch_name].clientIsInChannel(_clients[fd].Nickname(), fd)) {
			return ;
	}
	else
		_channels[ch_name].addParticipant(fd, _clients[fd].getinfo());
	_clients[fd].addChannel(ch_name);
	_clients[fd].clearMsg();
}

/**
 * Server commands
 * 
 * Server launch commands
 * 
 * @param fd fd client.
 * 
 */
void	Server::commands(int fd) {
	std::string command(_clients[fd].getinfo().msg);
	trimInput(command);

	if (command.size( ) > 6 && command.substr(0, 5) == "PING ")
		_clients[fd].ping(fd);
	else if (command.size( ) > 6 && command.substr(0, 6) == "JOIN #") 
		multiJoin(fd);
	else if (command.size( ) > 6 && command.substr(0, 6) == "PART #")
		multiPart(fd);
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
		std::map<int, Client> cp_client = _clients;
		for (std::map<int, Client>::iterator itC = cp_client.begin( ) ; itC != cp_client.end( ) ; ++itC)
			quit(itC->first);
		std::cout << "[isRun]Shutdown of IRC" << std::endl;
		close(_server.socket);
		return ;
	} else if (!((command.size( ) > 4 && command.substr(0, 4) == "WHO ") || (command.size( ) > 9 && command.substr(0, 9) == "USERHOST "))) {
		std::string ERR_UNKNOWNCOMMAND = ":ircserv 421 " + _clients[fd].Nickname() + " " + command + " :Unknown command\r\n";
		send(fd, ERR_UNKNOWNCOMMAND.c_str(), ERR_UNKNOWNCOMMAND.length(), MSG);
	}
	_clients[fd].clearMsg();
}

/** Server multiPart
 * 
 * Server launch multiPart
 * 
 * @param fd fd client.
 * @param flag flag.
 * 
 */
void Server::multiPart(int fd) {
	std::istringstream iss(_clients[fd].getinfo().msg);
	std::string sNull, sChannel, reason;
	iss >> sNull >> sChannel;
	std::getline(iss, reason, (char)EOF);

	std::deque<std::string> channel = split(sChannel, ',');
	std::deque<std::string>::iterator itS = channel.begin( );
	for ( ; itS != channel.end( ); ++itS) {
		_clients[fd].getinfo().msg = sNull + " " + *itS + " " + reason;
		Server::singlePart(fd);
	}
}

/**
 * Server noSuchChannel
 * 
 * check if channel exist and send error message to client
 *  
 * @param fd fd client.
 * @param channel_name name of channel.
 * @return true if channel doesn't exist, false otherwise.
 */
bool Server::noSuchChannel(int fd, std::string channel_name) {
	if (_channels.find(channel_name) == _channels.end()) {
		std::string ERR_NOSUCHCHANNEL = ":ircserv 403 " + _clients[fd].getinfo().nickname + " " + &channel_name[1] + " :No such channel\r\n";
		send(fd, ERR_NOSUCHCHANNEL.c_str(), ERR_NOSUCHCHANNEL.length(), MSG);
		return true;
	}
	return false;
}

// static void printmap(std::map<std::string, int> map) {
// 	for (std::map<std::string, int>::iterator it = map.begin(); it != map.end(); ++it) {
// 		std::cout << it->first << " STAMPO IL SECONDO " << it->second << std::endl;
// 	}
// }

// static void printvector(std::vector<std::string> vec) {
// 	for (std::vector<std::string>::iterator it = vec.begin(); it != vec.end(); ++it) {
// 		std::cout << "JOINED: " << *it << std::endl;
// 	}
// }

/**
 * Server singlePart
 * 
 * user exit from channel
 * 
 * @param fd fd client.
 * @param flag flag to check if user is in channel.
 * 
 */
void Server::singlePart(int fd) {
	std::istringstream iss(_clients[fd].getinfo().msg);
	std::string sNull, channel_name, message;
	iss >> sNull >> channel_name;
	toUpper(channel_name);
	if (noSuchChannel(fd, &channel_name[1]))
		return ;
	if (_channels[&channel_name[1]].userIsNotInChannel(_clients[fd].Nickname()))
		return ;
	_channels[&channel_name[1]].removeParticipant(_clients[fd].Nickname());
	_clients[fd].removeChannel(&channel_name[1]);
	if (_channels[&channel_name[1]].channelIsEmpty()) {
		_channels.erase(&channel_name[1]);
		return ;
	}
	if (_channels[&channel_name[1]].noOperators( ))
		_channels[&channel_name[1]].addOperator(_channels[&channel_name[1]].getinfo().users.begin()->first, _channels[&channel_name[1]].getinfo().users.begin()->second);
    _clients[fd].clearMsg();
}

/**
 * multijoin
 * Server join client in more channel
 * @param fd file descriptor of client
*/
void Server::multiJoin(int fd) {
	std::string sNUll, channel_name, passwd;
	std::istringstream iss(_clients[fd].getinfo().msg);

	iss >> sNUll >> channel_name >> passwd;
	trimInput(channel_name);
	trimInput(passwd);
	std::deque<std::string> moreChannel = split(channel_name, ',');
	std::deque<std::string> morePasswd = split(passwd, ',');
	std::deque<std::string>::iterator itC = moreChannel.begin( );
	std::deque<std::string>::iterator itP = morePasswd.begin( );
	for ( ; itC != moreChannel.end( ); ++itC) {
		if(itP == morePasswd.end( )) {
			_clients[fd].setMsg("JOIN " + *itC + "\r\n");
		} else {
			_clients[fd].setMsg("JOIN " + *itC + " " + *itP + "\r\n");
            ++itP;
		}
		std::cout << "msg join : " << _clients[fd].getinfo().msg << std::endl;
		singleJoin(fd);
	}
}

/**
 * quit
 * @brief quit from server
 * @param fd file descriptor of client
 * 
*/

//AGGIUSTA TUTTO. 
void Server::quit(int fd) {
	if (_clients.empty() || _clients.find(fd) == _clients.end())
		return ;
	std::map<int, Client> tmp_client = _clients;
	std::vector<std::string>::iterator itC = tmp_client[fd].getinfo().joninedChannell.begin( );
	for ( ; itC != tmp_client[fd].getinfo().joninedChannell.end( ); ++itC) {
		std::string PART = "PART #" + *itC + "\r\n";
		_clients[fd].setMsg(PART);
		singlePart(fd);
	}
	std::string PTR_QUIT = ":" + _clients[fd].Nickname() + "!" + hostname + " QUIT :Quit: Bye for now!\r\n";
	send(fd, PTR_QUIT.c_str(), PTR_QUIT.length(), MSG);
	std::cout << _clients[fd].Nickname() << " :Quit" << std::endl;
    if (fd != 1000 && (epoll_ctl(_server.epollFd, EPOLL_CTL_DEL, fd, NULL) ==  -1)) {
        perror("Error removing client from epoll");
    }
	close(fd);
	if (_clients.find(fd) != _clients.end())
		_clients.erase(fd);
	_server.connected_clients -= 1;
}

/**
 * multiMessage
 * @brief send multiple messages to other clients or channels
 * @param fd file descriptor of client who send messages
 * */
void Server::multiMessage(int fd) {
	std::istringstream iss(_clients[fd].getinfo().msg);
	std::string sNull, sTarget, message;
	iss >> sNull >> sTarget;
	std::getline(iss, message, (char)EOF);
	std::deque<std::string> target = split(sTarget, ',');
	std::deque<std::string>::iterator itS = target.begin( );
	for ( ; itS != target.end( ); ++itS) {
		_clients[fd].getinfo().msg = sNull + " " + *itS + message;
		Server::singleMessage(fd);
	}
}

/**
 * singleMessage
 * @brief send single message to other client or channel
 * @param fd file descriptor of client who send message
*/
void Server::singleMessage(int fd)
{
	std::istringstream iss(_clients[fd].getinfo().msg);
	std::string sNull, target, message;
	iss >> sNull >> target;
	std::getline(iss, message, (char)EOF);
	trimInput(message);

	if (message[0] == ':') {
		message.erase(0, 1);
	}
	if (message.empty( )) {
		std::string ERR_NOTEXTTOSEND = ":ircserv 421 " + _clients[fd].Nickname() +  " :No text to send\r\n";
		send(fd, ERR_NOTEXTTOSEND.c_str(), ERR_NOTEXTTOSEND.length(), MSG);
		return ;
	}
	if (_clients[fd].Nickname() == target)
		return ;
	// If the target starts with a #, it's a channel
	if (target[0] == '#') {
		toUpper(target);
		if (_channels[&target[1]].userIsNotInChannel(_clients[fd].Nickname()))
			return ;
		else {
			for (std::map<std::string, int>::iterator it = _channels[&target[1]].getinfo().users.begin(); it != _channels[&target[1]].getinfo().users.end(); ++it)
			{
				if (it->first != _clients[fd].Nickname()) {
					std::string RPL_PRIVMSG = ":" + _clients[fd].Nickname() + "!~" + hostname + " PRIVMSG " + target + " :" + message + "\r\n";
					send(it->second, RPL_PRIVMSG.c_str(), RPL_PRIVMSG.size(), MSG);
				}
			}
			if (!target.compare("#WELCOME") && !message.compare(0, 4, "!WE "))
				botMode(message);
		}
	}
	// Otherwise, the target is a user
	else {
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++) {
			if (it->second.Nickname() == target) {
				std::string RPL_PRIVMSG = ":" + _clients[fd].Nickname() + "! PRIVMSG " + target + " :" + message + "\r\n";
				send(it->first, RPL_PRIVMSG.c_str(), RPL_PRIVMSG.size(), MSG);
				break;
			} else if (it == _clients.end( )) {
				std::string ERR_NOSUCHNICK = ":ircserv 401 " + _clients[fd].Nickname() + " " + target + " :No such nick\r\n";
				send(fd, ERR_NOSUCHNICK.c_str(), ERR_NOSUCHNICK.length(), MSG);
			}
		}
	}
}

/**
 * isOpSizPawd
 * @brief check if mode is valid
 * @param lMode list of mode
 * @return true if mode is valid, false otherwise
 * 
*/
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

/**
 * modeChanges
 * @brief get mode changes
 * @param mode mode
 * @return vector of mode changes
*/
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

static void sendErrorMode(int fd, std::string channel_name, std::string nickname, std::string mode_changes) 
{
	std::string ERR_INVALIDMODEPARAM = ":ircserv 696 " + nickname + " #" + channel_name + " " + mode_changes + " : need an arg \r\n";
	send(fd, ERR_INVALIDMODEPARAM.c_str(), ERR_INVALIDMODEPARAM.length(), MSG);
	return ;
}

/**
 * mode
 * @brief set mode
 * @param fd file descriptor of client
*/
void Server::mode(int fd)
{
	std::stringstream iss(_clients[fd].getinfo().msg);
	std::string sNull, channel_name, mode_changes;
	iss >> sNull >> channel_name >> mode_changes;
	std::vector<std::string> argList;
	std::vector<std::string> modeList = modeChanges(mode_changes);
	while (iss >> mode_changes)
		argList.push_back(mode_changes);
	
	toUpper(channel_name);
	// if (isOpSizPawd(modeList) == true && !pwdOperSiz.empty( )) {
	// 	std::string ERR_INVALIDMODEPARAM = ":ircserv 696 " +  _clients[fd].getinfo().nickname + " #" + &channel_name[1] + " +k/+l/o : Can't set this param simultany in mode option\r\n";
	// 	send(fd, ERR_INVALIDMODEPARAM.c_str(), ERR_INVALIDMODEPARAM.length(), MSG);
	// 	return ;
	// }
	mode_changes.clear( );
	for (std::vector<std::string>::iterator itV = modeList.begin( ); itV != modeList.end( ); ++itV) {
		mode_changes = *itV;
		if (mode_changes.empty( ))
			return ;
		if (_channels[&channel_name[1]].userIsNotInChannel(_clients[fd].Nickname()))
			return ;
		Channel& channel = _channels[&channel_name[1]];
		if (!channel.userIsOperator(_clients[fd].Nickname())) {
			return ;
		} else {
			if (mode_changes == "+o") {
				if(argList.empty())	{
					sendErrorMode(fd, &channel_name[1], _clients[fd].getinfo().nickname, mode_changes);
					return ;
				}
				channel.modeOperator(fd, _clients[fd].Nickname(), argList[0]);
				argList.erase(argList.begin( ));
			} else if (mode_changes == "-o") {
				if(argList.empty())	{
					sendErrorMode(fd, &channel_name[1], _clients[fd].getinfo().nickname, mode_changes);
					return ;
				}
				channel.modeUnsetOperator(fd, _clients[fd].Nickname(), argList[0]);	
				argList.erase(argList.begin( ));
			} else if (mode_changes == "+b") {
				;
			} else if (mode_changes == "+t") {
				channel.getinfo().topicLock = true;
				channel.sendToAllUserModeChanges(_clients[fd].Nickname(), mode_changes);
			} else if (mode_changes == "-t") {
				channel.getinfo().topicLock = false;
				channel.sendToAllUserModeChanges(_clients[fd].Nickname(), mode_changes);
			} else if (mode_changes == "+i") {
				//per ogni utente nel canale, aggingi gli utenti nella pila di invite
				for (std::map<std::string, int>::iterator it = channel.getinfo().users.begin(); it != channel.getinfo().users.end(); ++it) {
					channel.addUserToInvited(it->first);
				}
				channel.setInvite(true);
				channel.sendToAllUserModeChanges(_clients[fd].Nickname(), mode_changes);
			} else if (mode_changes == "-i") {
				channel.setInvite(false);
				channel.sendToAllUserModeChanges(_clients[fd].Nickname(), mode_changes);
			} else if (mode_changes == "+k") {
				if(argList.empty())	{
					sendErrorMode(fd, &channel_name[1], _clients[fd].getinfo().nickname, mode_changes);
					return ;
				}
				channel.modeSetKey(fd, _clients[fd].Nickname(), argList[0], mode_changes);
				argList.erase(argList.begin( ));
			} else if (mode_changes == "-k"){
				channel.setkey("");
				channel.sendToAllUserModeChanges(_clients[fd].Nickname(), mode_changes);
			} else if (mode_changes == "+l") {
				if(argList.empty())	{
					sendErrorMode(fd, &channel_name[1], _clients[fd].getinfo().nickname, mode_changes);
					return ;
				}
				channel.modeSetLimits(fd, _clients[fd].Nickname(), argList[0], mode_changes);
				argList.erase(argList.begin( ));
			} else if (mode_changes == "-l"){
				channel.getinfo().limitLock = false;
				channel.getinfo().limit = __INT_MAX__;
				channel.sendToAllUserModeChanges(_clients[fd].Nickname(), mode_changes);
			} else {
				std::string ERR_UNKNOWNMODE = ":ircserv 501 " + _clients[fd].getinfo().nickname + " :Unknown MODE flag\r\n";
				send(fd, ERR_UNKNOWNMODE.c_str(), ERR_UNKNOWNMODE.size(), MSG);
				return ;
			}
		}
	}
}

void Server::topic(int fd) {
	std::istringstream iss(_clients[fd].getinfo().msg);
	std::string sNull, channel_name, new_topic;

	iss >> sNull >> channel_name;
	std::getline(iss, new_topic, (char)EOF);

	toUpper(channel_name);
	trimInput(new_topic);
	if (_channels[&channel_name[1]].userIsNotInChannel(_clients[fd].Nickname()))
		return ;
	Channel& channel = _channels[&channel_name[1]];
	if (new_topic.empty( )) {
		if (!channel.getinfo().topic.empty( )) {
			std::string RPL_TOPIC = ":ircserv 332 " + _clients[fd].getinfo().nickname + " #" + &channel_name[1] + " :" + channel.getinfo().topic + "\r\n";
			send(fd, RPL_TOPIC.c_str(), RPL_TOPIC.size(), MSG);
		} else {
			std::string RPL_NOTOPIC = ":ircserv 331 " + _clients[fd].getinfo().nickname + " #" + &channel_name[1] + " :No topic is set\r\n";
			send(fd, RPL_NOTOPIC.c_str(), RPL_NOTOPIC.size(), MSG);
		}
		return ;
	}
	if (channel.getinfo().topicLock == true && !channel.userIsOperator(_clients[fd].Nickname()))
		return ;
	if (new_topic.find(":") == std::string::npos)
		channel.setTopic(new_topic);
	else
		channel.setTopic(&new_topic[1]);
	std::string RPL_TOPIC = ":ircserv 332 " + _clients[fd].getinfo().nickname + " " + channel_name + " :" + channel.getinfo().topic + "\r\n";
	channel.sendMessageToAll(RPL_TOPIC);
}


void Server::invite(int fd)
{
	std::stringstream iss(_clients[fd].getinfo().msg);
	std::string sNull, nick, channel_name;
	iss >> sNull >> nick >> channel_name;

	toUpper(channel_name);
	if (_channels[&channel_name[1]].userIsNotInChannel(_clients[fd].Nickname()))
		return ;
	Channel& channel = _channels[&channel_name[1]];
	if (channel.clientIsInChannel(nick, fd)){
			std::string ERR_USERONCHANNEL = ":ircserv 443 " + _clients[fd].getinfo().nickname + " " + nick + " " + channel_name + " :is already on channel\r\n"; 
			send(fd, ERR_USERONCHANNEL.c_str(), ERR_USERONCHANNEL.size(), MSG);
			return ;
	}
	std::map<int, Client>::iterator it = _clients.begin();
	for (;it != _clients.end(); it++) {
		if (it->second.Nickname() == nick) {
			channel.addUserToInvited(it->second.Nickname());
			std::string RPL_INVITING = ":ircserv 341 " + _clients[fd].getinfo().nickname + " " + nick + " " + channel_name + "\r\n";
			std::string RPL_INVITE = ":" + _clients[fd].getinfo().nickname + "!~" + _clients[fd].getinfo().username + hostname  + " INVITE " + nick + " " + channel_name + "\r\n";
			send(_clients[fd].getinfo().fd, RPL_INVITING.c_str(), RPL_INVITING.size(), MSG);
			send(it->second.getinfo().fd, RPL_INVITE.c_str(), RPL_INVITE.size(), MSG);
			return ;
		}
	}
    std::string ERR_NOSUCHNICK = ":ircserv 401 " + _clients[fd].getinfo().nickname + " " + nick + " :No such nick\r\n";
	send(fd, ERR_NOSUCHNICK.c_str(), ERR_NOSUCHNICK.length(), MSG);
}

/**
 * kick
 * @brief kick user from channel
 * @param fd file descriptor of client
 * */
void Server::kick(int fd) {
	// Parse the channel and user;git@github.com:graiolo/irc.git
	std::istringstream iss(_clients[fd].getinfo().msg);
	std::string nString, channel_name, user_nickname;
	iss >> nString >> channel_name >> user_nickname;

	toUpper(channel_name);
	// Find the channel and user
	if (_channels[&channel_name[1]].userIsNotInChannel(_clients[fd].Nickname()))
		return ;
	Channel& channel = _channels[&channel_name[1]];
	if (channel.userIsOperator(_clients[fd].Nickname()) == false)
		return ;
	if (user_nickname.empty( )) {
		std::string ERR_NEEDMOREPARAMS = ":ircserv 461 " + _clients[fd].Nickname() + " KICK :Not enough parameters\r\n";
		send(fd, ERR_NEEDMOREPARAMS.c_str(), ERR_NEEDMOREPARAMS.length(), MSG);
		return ;
	}
	if(!channel.userIsInChannel(user_nickname)) {
		std::string ERR_USERNOTINCHANNEL = ":serverirc 441 " + _clients[fd].Nickname() + " " + user_nickname + " " + channel_name + " :They aren't on that channel\r\n";
		send(fd, ERR_USERNOTINCHANNEL.c_str(), ERR_USERNOTINCHANNEL.size(), MSG);
		return ;
	}
	//:WiZ!jto@tolsun.oulu.fi KICK #Finnish John
	std::string RPL_KICKMSG = ":" + _clients[fd].Nickname() + "!" +  _clients[fd].getinfo().username +
		"@" + hostname + " KICK #" + &channel_name[1] + " " + user_nickname + " :Kicked by " + _clients[fd].Nickname() + "\r\n";
	// Notify all users in the channel
	channel.sendMessageToAll(RPL_KICKMSG);
	std::string RPL_YOUKICK = ":ircserv 441 " + _clients[fd].Nickname() + "kicked you from " + channel_name + "\r\n";
	channel.removeInvited(user_nickname);
	_clients[channel.getinfo().users[user_nickname]].setMsg("PART " + channel_name);
	send (_channels[&channel_name[1]].getinfo().users[user_nickname], RPL_YOUKICK.c_str(), RPL_YOUKICK.size(), MSG);
	multiPart(channel.getinfo().users[user_nickname]);

}

 /**
 * addBot
 * @brief Add bot to channel
*/
void Server::addBot()
{
	ClientInfo info;
	info.fd = 1000;
	info.nickname = "Bottone";
	info.username = "Bottone";
	info.isFirstTime = false;
	info.authorized = true;
	info.msg = "JOIN #WELCOME";
 	_clients.insert(std::make_pair(info.fd, Client(info)));
	singleJoin(1000);
}

/**
 * botMode
 * @brief Set mode bot
 * @param message message to send
*/
void Server::botMode(std::string msg)
{
	std::istringstream iss(msg);
	std::string nString, message;
	iss >> nString >> message;
	trimInput(message);
	if (!message.empty()) {
		toUpper(message);
		if (!message.compare("CIAO"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Cia'\r\n");
		else if (!message.compare("AIUTO"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Chiedi a Damiano, lui sapra'!\r\n ");
		else if (!message.compare("BTANI"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Ma nudo? BLU?\r\n ");
		else if (!message.compare("STE"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Se indossi una maglietta cripto, non sei nessuno\r\n ");
		else if (!message.compare("VICTOR"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Non ha voce e grida fa, non ha ali e a volo va, non ha denti e morsi dà, non ha bocca e versi fa... La risposta era scritta sul quaderno, pero' l'ho buttato. MANNAGGIA!\r\n ");
		else if (!message.compare("LUPE"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Ti va un penetration test?\r\n ");
		else if (!message.compare("NIZ"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Bella prova, ma resta sempre una prova, facciamo una Anna?\r\n ");			
		else if (!message.compare("SIMO"))
			_clients[1000].setMsg("PRIVMSG #WELCOME aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaah!\r\n");
		else if (!message.compare("ALEGRE"))
			_clients[1000].setMsg("PRIVMSG #WELCOME 🙌");
		else if (!message.compare("PING"))
			_clients[1000].setMsg("PRIVMSG #WELCOME e ja, veramente ti aspettavi un pong?!\r\n");
		else if (!message.compare("NELLY"))
			_clients[1000].setMsg("PRIVMSG #WELCOME No.\r\n");
		else if (!message.compare("FRATM"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Eh succede sorm...");
		else if (!message.compare("DIP"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Che bella la vita tatantatan tatantatan ...\r\n");
		else if (!message.compare("PACCI"))
			_clients[1000].setMsg("PRIVMSG #WELCOME Se nimmondo ci fosse un po' di bene, e ognun si considerasse i su fratello... \r\n");
		else if (!message.compare("GABRY"))
		{
			int i = rand() % 4;
			std::string msg = "PRIVMSG #WELCOME ";
			switch (i)
			{
				case 0:
					_clients[1000].setMsg(msg + "Chiude una lavanderia. Faceva affari sporchi.\r\n");
					break;
				case 1:
					_clients[1000].setMsg(msg + "Ragazza stufa scappa di casa. Genitori morti dal freddo.\r\n");
					break;
				case 2:
					_clients[1000].setMsg(msg + "Abbiamo riso abbastanza, adesso pasta.\r\n");
					break;
				case 3:
					_clients[1000].setMsg(msg + "Chiude una fabbrica di carta igienica: andava a rotoli.\r\n");
					break;
				default:
					_clients[1000].setMsg(msg + "Abbiamo riso abbastanza, adesso pasta.\r\n");
					break;
			}
		} else if (!message.compare("BEPPE"))
		{
			int i = rand() % 4;
			std::string msg = "PRIVMSG #WELCOME ";
			switch (i)
			{
				case 0:
					_clients[1000].setMsg(msg + "Lo sapevi che  se dici Jesus al contrario suona come sausage.\r\n");
					break;
				case 1:
					_clients[1000].setMsg(msg + "Lo sapevi che gli orsi polari sono mancini.\r\n");
					break;
				case 2:
					_clients[1000].setMsg(msg + "Lo sapevi che il gatto é l'unico animale domestico non menzionato nella Bibbia.\r\n");
					break;
				case 3:
					_clients[1000].setMsg(msg + "Lo sapevi che non ti puoi leccare i gomiti.\r\n");
					break;
				default:
					_clients[1000].setMsg(msg + "Lo sapevi che Se tu urlassi per 8 anni 7 mesi e 6 giorni, produrresti abbastanza energia sonora per riscaldare una tazza di caffè.\r\n");
					break;
			}
		}
	else
		_clients[1000].setMsg("PRIVMSG #WELCOME Sono proprio io, il Bot piu' inutile del mondo! E tu mi stai veramente sopravvalutando, so rispondere solo a: -ciao -ping -aiuto -Gabry -Beppe -Nelly -Simo -Alegre -Niz -Victor -Ste -Btani -Lupe -Pacci -Dip -Fratm\r\n");
	} else {
		_clients[1000].setMsg("PRIVMSG #WELCOME Sono proprio io, il Bot piu' inutile del mondo! scegli un comando tra: -ciao -ping -aiuto -Gabry -Beppe -Nelly -Simo -Alegre -Niz -Victor -Ste -Btani -Lupe -Pacci -Dip -Fratm\r\n");
	}
	singleMessage(1000);
	return ;
}