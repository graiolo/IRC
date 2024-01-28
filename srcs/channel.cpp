#include "../includes/channel.hpp"

//Channel construction class

Channel::Channel( ) 
{
	_chInfo.name = "";
	_chInfo.topic = "";
	_chInfo.key = "";
	_chInfo.inviteOnly = false;
	_chInfo.topicLock = false;
	_chInfo.limitLock = false;
	_chInfo.limit = MAX_CLIENTS;
	_chInfo.userIn = 0;

}

Channel::Channel(const std::string &name)
{
	_chInfo.name = name;
	_chInfo.topic = "";
	_chInfo.key = "";
	_chInfo.inviteOnly = false;
	_chInfo.topicLock = false;
	_chInfo.limitLock = false;
	_chInfo.limit = MAX_CLIENTS;
	_chInfo.userIn = 0;
}

Channel::Channel(const std::string &name, std::string passwd)
{
	_chInfo.name = name;
	_chInfo.topic = "";
	_chInfo.key = passwd;
	_chInfo.inviteOnly = false;
	_chInfo.topicLock = false;
	_chInfo.limitLock = false;
	_chInfo.limit = MAX_CLIENTS;
	_chInfo.userIn = 0;
}


Channel::~Channel( ) { }

Channel::Channel(const Channel &copy)
{
	*this = copy;
}

//overload of operator =
Channel &Channel::operator=(const Channel &copy)
{
	_chInfo = copy._chInfo;
	return *this;
}

ChannelInfo & Channel::getinfo()
{
	return _chInfo;
}

/**
 * setkey
 * @brief set password for channel
 * @param passwd password
*/
void Channel::setkey(std::string passwd)
{
	_chInfo.key = passwd;
}

/**
 * clientIsInChannel
 * @brief Check if client is in channel and send error message if yes
 * @param Nickname Nick of client
 * @param fd file descriptor of client
 * @param ch_name name of channel
 * @return true if client is in channel
*/
bool Channel::clientIsInChannel(std::string Nickname, int fd) 
{
	if (_chInfo.users.find(Nickname) != _chInfo.users.end( )) 
	{
		std::string ERR_USERONCHANNEL = ":ircserv 443 " + Nickname + " #" + _chInfo.name + " :is already on channel\r\n";
		send(fd, ERR_USERONCHANNEL.c_str(), ERR_USERONCHANNEL.length(), MSG);
		return true;
	}
	else
		return false;
}


bool Channel::onlyInvite(std::string Nickname, int fd, std::string ch_name)
{
if (_chInfo.inviteOnly) 
{
    if (_chInfo.invited.find(Nickname) == _chInfo.invited.end())
    {
        std::string ERR_INVITEONLYCHAN = ":ircserv " + Nickname +  " #" + ch_name + " :Cannot join channel (+i)\r\n";
        send(fd, ERR_INVITEONLYCHAN.c_str(), ERR_INVITEONLYCHAN.size(), MSG);
        return true;
    }
}
return false;
}

bool Channel::channelIsFull(std::string Nickname, int fd, std::string ch_name)
{
	if(_chInfo.limitLock && _chInfo.userIn + 1 >  _chInfo.limit) {
		std::string ERR_CHANNELISFULL = ":ircserv 471 " + Nickname + " #" + ch_name + " :Channel is full (+l)\r\n";
		send(fd, ERR_CHANNELISFULL.c_str(), ERR_CHANNELISFULL.length(), MSG);
		return true;
	}
	return false;
}

bool Channel::wrongPass(std::string Nickname, int fd, std::string passwd)
{
	if (!_chInfo.key.empty() && _chInfo.key != passwd) 
	{
		std::string ERR_BADCHANNELKEY = ":ircserv 475 " + Nickname + " #" + _chInfo.name + " :Cannot join channel (+k)\r\n";
		send(fd, ERR_BADCHANNELKEY.c_str(), ERR_BADCHANNELKEY.length(), MSG);
		return true;
	}
	return false;
}

/**
 * addOperator
 * @brief add operator to channel
 * @param Nickname Nick of operator
 * @param fd file descriptor of operator
*/
void Channel::addOperator(std::string Nickname, int fd)
{
	sendMessageToAll(":ircserv MODE #" + std::string(_chInfo.name + " +o " + Nickname + "\r\n"));
	_chInfo.operators[Nickname] = fd;
	
}

void Channel::removeOperator(std::string Nickname)
{
	sendMessageToAll(":ircserv MODE #" + std::string(_chInfo.name + " -o " + Nickname + "\r\n"));
	_chInfo.operators.erase(Nickname);
}

/**
 * addParticipant
 * @brief add participant to channel
 * @param clientFd file descriptor of client
 * @param info information of client
*/
void Channel::addParticipant(int clientFd, const struct ClientInfo info) 
{
	_chInfo.users[info.nickname] = clientFd;
	_chInfo.userIn++;
	sendJoinMessageBackToClient(clientFd, _chInfo.name, info.nickname);
}

/**
 * removeParticipant
 * @brief remove participant from channel
 * @param Nickname Nick of participant
*/
void Channel::removeParticipant(std::string Nickname)
{
	sendMessageToAll(":" + Nickname + "!" + hostname + " PART #" + _chInfo.name + "\r\n");
	this->_chInfo.users.erase(Nickname);
	this->_chInfo.operators.erase(Nickname);
	this->_chInfo.userIn--;
	std::map<std::string, int>::iterator it = _chInfo.users.begin();
	for (; it != _chInfo.users.end(); ++it) {
		std::cout << "user: " << it->first << std::endl;
	}
}

/**
 * addUsersToInvited
 * @brief add user to invited list
 * @param nickname Nick of user
 * */
void Channel::addUserToInvited(std::string Nickname)
{
	_chInfo.invited.insert(Nickname);
}

/**
 * removeInvited
 * @brief remove user from invited list
 * @param nickname Nick of user
 * */
void Channel::removeInvited(std::string Nickname)
{
	_chInfo.invited.erase(Nickname);
}

void Channel::setLimit(int limit)
{
	_chInfo.limit = limit;
}

void Channel::setInvite(bool set)
{
	_chInfo.inviteOnly = set;
}

/**
 * setLimitLock
 * @brief set limitLock of channel
 * @param set true or false
 * 
*/
void Channel::setLimitLock(bool set)
{
	_chInfo.limitLock = set;
}

void Channel::setTopic(const std::string& newTopic)
{
	_chInfo.topic = newTopic;
}

/**
 * channelIsEmpty
 * @brief Check if channel has no users
 * @return true if channel is empty
*/
bool Channel::channelIsEmpty()
{
	if (_chInfo.users.empty())
		return true;
	return false;
}

/**
 * noOperators
 * @brief Check if channel has operators
 * @return true if channel has no operators
*/
bool Channel::noOperators()
{
	if (_chInfo.operators.empty())
		return true;
	return false;
}

/**
 * userIsOperator
 * @brief Check if user is operator and send error message if not
 * @param Nickname Nick of user
 * @return true if user is operator
*/
bool Channel::userIsOperator(std::string Nickname)
{
	if (_chInfo.operators.find(Nickname) != _chInfo.operators.end())
		return true;
	std::string ERR_CHANOPRIVSNEEDED = ":ircserv 482 " + Nickname + " #" + _chInfo.name + " :You're not channel operator\r\n";
	send(_chInfo.users[Nickname], ERR_CHANOPRIVSNEEDED.c_str(), ERR_CHANOPRIVSNEEDED.size(), MSG);
	return false;
}

/**
 * userIsNotInChannel
 * @brief Check if user is in channel and send error message if not
 * @param Nickname Nick of user
 * @return true if user is not in channel
*/
bool Channel::userIsNotInChannel(std::string Nickname)
{
	if (_chInfo.users.find(Nickname) == _chInfo.users.end())
			return true;
	return false;
}  

/**
 * userIsInChannel
 * @brief Check if user is in channel
 * @param Nickname Nick of user
 * @return true if user is in channel
*/
bool Channel::userIsInChannel(std::string Nickname)
{
	if (_chInfo.users.find(Nickname) != _chInfo.users.end())
		return true;
	return false;
}

/**
 * sendMessageToAll
 * @brief Send message to all users in channel
 * @param message message to send
*/
void Channel::sendMessageToAll(const std::string& message)
{
	for (std::map<std::string, int>::iterator it = _chInfo.users.begin(); it != _chInfo.users.end(); ++it) {
		send(it->second, message.c_str(), message.length(), MSG);
	}
}

/**
 * sendJoinMessageBackToClient
 * @brief Send join message to client
 * @param fd file descriptor of client
 * @param channel_name name of channel
 * @param nickname Nick of client
 * 
*/
void	Channel::sendJoinMessageBackToClient(int fd, const std::string& channel_name, std::string nickname) {
	std::string RPL_JOIN = ":" + nickname + "!" + hostname + " JOIN #" + channel_name + "\r\n";
	std::string RPL_NAMREPLY = ":ircserv 353 " + nickname + " = #" + channel_name + " :";
	//Send JOIN message to all users in the channel
	
		for (std::map<std::string, int>::iterator it = _chInfo.users.begin(); it != _chInfo.users.end(); it++) {
			send(it->second, RPL_JOIN.c_str(), RPL_JOIN.length(), MSG);
		// Build RPL_NAMREPLY message
			RPL_NAMREPLY += it->first;
			if (++it != _chInfo.users.end()) {  // Move the iterator back one step if it's not the last user
        		RPL_NAMREPLY += " ";
    		} else {
        		RPL_NAMREPLY += "\r\n";
			}
			--it;
	}
	//Check if topic is setted
	if (_chInfo.topic.empty()){
		std::string RPL_TOPIC = ":ircserv 332 " + nickname + " #" + channel_name + " :" + _chInfo.name + "\r\n";
		send(fd, RPL_TOPIC.c_str(), RPL_TOPIC.length(), MSG);
	}
	//Check if channels mode enabled
	if (_chInfo.topicLock == true || _chInfo.inviteOnly == true || _chInfo.limit == true || !_chInfo.key.empty()){
		std::string RPL_CHANNELMODEIS = ":ircserv 324 " + nickname + " #" + channel_name + " +";
		std::string param = " ";
		if (_chInfo.topicLock == true)
			RPL_CHANNELMODEIS += 't';
		if (_chInfo.inviteOnly == true)
			RPL_CHANNELMODEIS += 'i';
		if (_chInfo.limitLock == true){
			std::stringstream ss;
			ss << _chInfo.limit;
			std::string num = ss.str();
			param += num;
			RPL_CHANNELMODEIS += 'l';
		}
		if (!_chInfo.key.empty()){
			if (RPL_CHANNELMODEIS.find("i") != RPL_CHANNELMODEIS.npos)
				param += " ";
			RPL_CHANNELMODEIS += "k";
			param += _chInfo.key;
		}
		RPL_CHANNELMODEIS += param + "\r\n";
		send(fd, RPL_CHANNELMODEIS.c_str(), RPL_CHANNELMODEIS.length(), MSG);
	}
	send(fd, RPL_NAMREPLY.c_str(), RPL_NAMREPLY.length(), MSG);

	// Build RPL_WHOISOPERATOR message
	std::string RPL_WHOISOPERATOR = ":ircserv MODE #" + channel_name + " +o ";
	std::map<std::string, int>::iterator itO = _chInfo.operators.begin();
	for (; itO != _chInfo.operators.end(); ++itO) {
		std::string RPL_WHOISOPERATOR = ":ircserv MODE #" + channel_name + " +o " + itO->first + "\r\n";
	    send(fd, RPL_WHOISOPERATOR.c_str(), RPL_WHOISOPERATOR.length(), MSG);
	}

}

/**
 * sendToAllUserModeChanges
 * @brief Send mode changes to all users in channel
 * @param nickname Nick of user
 * @param mode_changes mode changes
*/
void Channel::sendToAllUserModeChanges(std::string nickname, std::string mode_changes) {
	std::string RPL_CHANNELMODEIS = ":ircserv 324 " + nickname + " #" + _chInfo.name + " " + mode_changes + "\r\n";
	sendMessageToAll(RPL_CHANNELMODEIS);
}

/**
 * modeOperator
 * @brief Set mode operator
 * @param fd file descriptor of client
 * @param Nickname Nick of client
 * @param pwdOperSiz password of operator
*/
void Channel::modeOperator(int fd, std::string Nickname, std::string pwdOperSiz)
{
	if (pwdOperSiz.empty( )) {
			std::string ERR_NEEDMOREPARAMS = ":ircserv 461 " + Nickname + " #" + _chInfo.name + " " + "+o" + " :Not enough parameters\r\n";
			send(fd, ERR_NEEDMOREPARAMS.c_str(), ERR_NEEDMOREPARAMS.length(), MSG);
		} else {
			bool flag = false;
			for (std::map<std::string, int>::iterator it = _chInfo.users.begin(); it != _chInfo.users.end(); ++it) {
				if (it->first == pwdOperSiz) {
					flag = true;
					addOperator(it->first, it->second);
					std::string RPL_SETSOPERATOR = ":" + Nickname + " MODE " + _chInfo.name + " +o " + pwdOperSiz + "\r\n";
					sendMessageToAll(RPL_SETSOPERATOR);
					}
				}
			if (flag == false) {
				std::string ERR_NOSUCHNICK = ":ircserv 401 " + Nickname + " " + pwdOperSiz + " :No such nick\r\n";
				send(fd, ERR_NOSUCHNICK.c_str(), ERR_NOSUCHNICK.length(), MSG);
			}
		}
}

/**
 * modeUnsetOperator
 * @brief Unset mode operator
 * @param fd file descriptor of client
 * @param Nickname Nick of client
 * @param pwdOperSiz password of operator
*/
void Channel::modeUnsetOperator(int fd, std::string Nickname, std::string pwdOperSiz)
{
	bool flag = false;
	if (Nickname == pwdOperSiz) {
		flag = true;
		std::string ERR_UNKNOWNMODE = ":ircserv 501 " + Nickname + " :Can't self demote\r\n";
		send(fd, ERR_UNKNOWNMODE.c_str(), ERR_UNKNOWNMODE.size(), MSG);
	} else {
		for (std::map<std::string, int>::iterator it = _chInfo.users.begin(); it != _chInfo.users.end(); ++it) {
			if (it->first == pwdOperSiz) {
				flag = true;
				removeOperator(pwdOperSiz);
				std::string RPL_REMOVEROPERATOR = ":ircsev MODE " + _chInfo.name + " -o " + pwdOperSiz + "\r\n";
				sendMessageToAll(RPL_REMOVEROPERATOR);
			}
		}
	}
	if (flag == false) {
		std::string ERR_NOSUCHNICK = ":ircserv 401 " + Nickname + " " + pwdOperSiz + " :No such nick\r\n";
		send(fd, ERR_NOSUCHNICK.c_str(), ERR_NOSUCHNICK.length(), MSG);
	}
}

/**
 * modeSetKey
 * @brief Set mode key
 * @param fd file descriptor of client
 * @param Nickname Nick of client
 * @param pwdOperSiz password of operator
 * @param mode_changes mode changes
*/
void Channel::modeSetKey(int fd, std::string Nickname, std::string pwdOperSiz, std::string &mode_changes)
{
	if (pwdOperSiz.empty( )) {
		std::string ERR_NEEDMOREPARAMS = ":ircserv 461 " + Nickname + " #" + _chInfo.name + " " + mode_changes + " :Not enough parameters\r\n";
		send(fd, ERR_NEEDMOREPARAMS.c_str(), ERR_NEEDMOREPARAMS.length(), MSG);
	} else if (pwdOperSiz.find(".") != std::string::npos){
		std::string ERR_INVALIDKEY = ":ircserv 525 " + Nickname + " #" + _chInfo.name + " :Key is not well-formed\r\n";
		send(fd, ERR_INVALIDKEY.c_str(), ERR_INVALIDKEY.length(), MSG);
	} else {
		setkey(pwdOperSiz);
		sendToAllUserModeChanges(Nickname, "+k");
	}
}

/**
 * modeSetLimits
 * @brief Set mode limits
 * @param fd file descriptor of client
 * @param Nickname Nick of client
 * @param pwdOperSiz password of operator
 * @param mode_changes mode changes
*/
void Channel::modeSetLimits(int fd, std::string Nickname, std::string pwdOperSiz, std::string &mode_changes)
{
	if (pwdOperSiz.empty( ) || std::atoi(pwdOperSiz.c_str( )) < 0) {
		std::string ERR_NEEDMOREPARAMS = ":ircserv 461 " + Nickname + " #" + _chInfo.name + " " + mode_changes + " :Not enough parameters\r\n";
		send(fd, ERR_NEEDMOREPARAMS.c_str(), ERR_NEEDMOREPARAMS.length(), MSG);
	} else {
		std::ostringstream iss;
		setLimitLock(true);
		setLimit(std::atoi(pwdOperSiz.c_str( )));
		iss << _chInfo.limit;
		mode_changes += " :" + iss.str( );
		sendToAllUserModeChanges(Nickname, mode_changes);
		if (!_chInfo.limit) {
			setLimit(__INT_MAX__);
			setLimitLock(false);
		}	
	}
}

