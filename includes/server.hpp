#pragma once


# include "channel.hpp"
# include <string>

// # define MAX_CLIENTS 104 // Maximum number of clients the server will allow
// # define HEADER "CAP LS 302"



class Client;
class Channel;

class Server {
protected:
    static struct ServerData _server;
    static std::map<std::string, Channel> _channels; 
    static std::map<int, Client> _clients;
    static struct epoll_event _events[MAX_CLIENTS + 1];
    
public:
    Server(int, char *);
    ~Server();

    static void serverInit();
    static void addServerToEpoll();
    static bool serverIsRun();
    static int addClientToEpoll(int);
    static void newChannel(int, std::string, std::string);
    static void runServer();
    static void newClient();
    static void recvMessage(int);
    static void login(int fd);
    static void sendWelcomeBackToClient(int fd);
    static void multiJoin(int);
    static void singleJoin(int);
    static void commands(int fd);
    static void singlePart(int fd);
    static void multiPart(int fd);
    static bool noSuchChannel(int fd, std::string channel_name);
    static void quit(int);
    static void singleMessage(int fd);
    static void multiMessage(int fd);
    static void mode(int fd);
    static bool isOpSizPawd(std::vector<std::string> lMode);
    static std::vector<std::string> modeChanges(std::string mode);
    static void topic(int fd);
    static void invite(int fd);
    static void kick(int);
    static void addBot();
    static void botMode(std::string msg);
};
