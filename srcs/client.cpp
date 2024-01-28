#include "../includes/client.hpp"
#include "../includes/ircserver.hpp"

//Client construction class

Client::Client() 
{
	_info.nickname = "";
	_info.username = "";
	_info.passwd = "";
	_info.msg = "";
	_info.fd = 0;
	_info.isFirstTime = true;
	_info.authorized = false;
	_info.joninedChannell.clear();
	_info.addrLen = sizeof(_info.addr);
	std::cout << "--- Client created ---" << std::endl;
}

Client::Client(ClientInfo newinfo) 
{
	_info.addrLen = newinfo.addrLen;
	_info.addr = newinfo.addr;
	_info.nickname = newinfo.nickname;
	_info.username = newinfo.username;
	_info.passwd = newinfo.passwd;
	_info.msg = newinfo.msg;
	_info.fd = newinfo.fd;
	_info.isFirstTime = true;
	_info.authorized = false;
	_info.joninedChannell.clear();
	std::cout << "--- Client created init ---" << std::endl;
}

//Client copy constructor
Client::Client(const Client &copy)
{
	*this = copy;
	//_info.authorized = true;
}

//overload of operator =
Client &Client::operator=(const Client &copy)
{
	_info.nickname = copy._info.nickname;
	_info.username = copy._info.username;
	_info.passwd = copy._info.passwd;
	_info.msg = copy._info.msg;
	_info.fd = copy._info.fd;
	_info.isFirstTime = copy._info.isFirstTime;
	_info.joninedChannell = copy._info.joninedChannell;
	_info.addrLen = copy._info.addrLen;
	_info.authorized = copy._info.authorized;
	_info.addr = copy._info.addr;
	return *this;
}

Client::~Client( ) { }

ClientInfo & Client::getinfo()
{
	return _info;
}

/**
 * Nickname
 * @brief return nickname of client
 * @return nickname
*/
std::string Client::Nickname()
{
	return _info.nickname;
}

void Client::setNickname(std::string nick)
{
	_info.nickname = nick;
}

/**
 * clearMsg
 * @brief clear message of client
 * */
void Client::clearMsg()
{
	_info.msg.clear();
}

/**
 * setMsg
 * @brief set message for client
 * @param msg message
*/
void Client::setMsg(std::string msg)
{
	_info.msg.clear();
	_info.msg = msg;
}


/**
 * ping
 * @brief send PONG back message to client
 * @param fd file descriptor of client
 * 
*/
void	Client::ping(int fd)
{
	std::string PONG = "PONG " + std::string(std::strtok(&_info.msg[5], "\n")) + "\r\n";
	send(fd, PONG.c_str(), PONG.size(), MSG);
} 

/**
 * removeChannel
 * @brief remove channel from channel joined from client
 * @param channel_name name of channel
 * 
*/
void Client::removeChannel(std::string channel_name)
{
	_info.joninedChannell.erase(std::remove(_info.joninedChannell.begin(), _info.joninedChannell.end(), channel_name), _info.joninedChannell.end());
}

/**
 * addChannel
 * @brief add channel to channel joined from client
 * @param channel_name name of channel
*/
void Client::addChannel(std::string channel_name)
{
	_info.joninedChannell.push_back(channel_name);
}

/**
 * setAuthorized
 * @brief set authorized flag
 * @param flag flag
*/
void Client::setAuthorized(bool flag)
{
	_info.authorized = flag;
}

/**
 * setUsername
 * @brief set username
 * @param username username
*/
void Client::setUsername(std::string username)
{
	_info.username = username;
}

/**
 * setPasswd
 * @brief set password
 * @param passwd password
*/
void Client::setPasswd(std::string passwd)
{
	_info.passwd = passwd;
}

/**
 * setIsFirstTime
 * @brief set isFirstTime flag
 * @param flag flag
*/
void Client::setIsFirstTime(bool flag)
{
	_info.isFirstTime = flag;
}