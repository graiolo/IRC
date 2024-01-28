#pragma once

# include "ircserver.hpp"




class Client
{
protected:
    struct ClientInfo _info;

public:
    Client();
    Client(ClientInfo newinfo);
    ~Client();
    Client(const Client &copy);
    Client &operator=(const Client &copy);
    ClientInfo& getinfo();
    std::string Nickname();
    void setMsg(std::string msg);
    void setNickname(std::string nick);
    void removeChannel(std::string channel_name);
    void ping(int);
    void addChannel(std::string channel_name);
    void setAuthorized(bool flag);
    void setUsername(std::string username);
    void setIsFirstTime(bool flag);
    void setPasswd(std::string passwd);
    void clearMsg();
};

#include "channel.hpp"