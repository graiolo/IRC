#pragma once

#include "client.hpp"

struct ChannelInfo;
struct ClientInfo;

class Channel {
protected:
    struct ChannelInfo _chInfo;
    
public:
    Channel();
    Channel(const std::string& name);
    Channel(const std::string& name, std::string passwd);
    ~Channel();
    Channel(const Channel &copy);
    Channel &operator=(const Channel &copy);
    ChannelInfo& getinfo();
    void setkey(std::string passwd);
    void addParticipant(int clientFd, const struct ClientInfo info);
    void addOperator(std::string Nickname, int fd);
    void removeOperator(std::string Nickname);
    void removeParticipant(std::string Nickname);
    void setInvite(bool set);
    void setTopic(const std::string& newTopic);
    bool channelIsEmpty();
    bool noOperators();
    void sendMessageToAll(const std::string& message);
    bool userIsInChannel(std::string Nickname);
    bool userIsNotInChannel(std::string Nickname);
    bool clientIsInChannel(std::string, int);
    bool userIsOperator(std::string Nickname);
    bool onlyInvite(std::string, int fd, std::string);
    bool channelIsFull(std::string Nickname, int fd, std::string ch_name);
    bool wrongPass(std::string Nickname, int fd, std::string passwd);
    void sendJoinMessageBackToClient(int fd, const std::string& channel_name, std::string nickname);
    void sendToAllUserModeChanges(std::string nickname, std::string mode_changes);
    void setLimitLock(bool set);
    void setLimit(int limit);
    void modeOperator(int fd, std::string Nickname, std::string pwdOperSiz);
    void modeSetKey(int fd, std::string Nickname, std::string pwdOperSiz, std::string &mode_changes);
    void modeSetLimits(int fd, std::string Nickname, std::string pwdOperSiz, std::string &mode_changes);
    void modeUnsetOperator(int fd, std::string Nickname, std::string pwdOperSiz);
    void addUserToInvited(std::string Nickname);
    void removeInvited(std::string Nickname);


};
