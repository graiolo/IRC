# IRC
IRC (Internet Relay Chat) - an application-level Protocol for exchanging messages in real-time. Designed primarily for group communication, it also allows you to communicate via private messages and exchange data, including files. IRC uses the TCP transport protocol and cryptographic TLS (optional). IRC began to gain particular popularity after Operation "Desert Storm" (1991), when messages from all over the world were collected in one place and broadcast on-line to IRC. Due to the technical simplicity of implementation, the IRC protocol was used in the organization of botnets as a means of transmitting control commands to the computers participating in the botnet from the owner. 🌎 IRC network According to the protocol specifications, an IRC network is a group of servers connected to each other. The simplest network is a single server. The network should have the form of a connected tree, in which each server is the central node for the rest of the network. A client is anything that is connected to the server, except for other servers. There are two types of customers: custom settings; service stations. Forwarding messages in the IRC network IRC provides both group and private communication. There are several possibilities for group communication. A user can send a message to a list of users, and a list is sent to the server, the server selects individual users from it, and sends a copy of the message to each of them. More efficient is the use of channels. In this case, the message is sent directly to the server, and the server sends it to all users in the channel. In both group and private communication, messages are sent to clients via the shortest path and are visible only to the sender, recipient, and incoming servers. It is also possible to send a broadcast message. Client messages regarding changes in the network state (for example, channel mode or user status) must be sent to all servers that are part of the network. All messages originating from the server must also be sent to all other servers.

# Authors
[iragusa](https://github.com/IvanaRagusa)
[gfantech](https://github.com/Gabzert)
[graiolo](https://github.com/graiolo)

# Usage

# :notebook_with_decorative_cover: Table of Contents

- [About the Project](#star2-about-the-project)


## :star2: About the Project

This project involves the development of a multi-client chat server using C++ and leveraging the epoll mechanism for efficient event handling. The system is structured around three main classes: Server, Client, and Channel. The Server class manages the core functionality of the chat server, including handling incoming connections, managing clients, and facilitating communication between clients through channels. The Client class represents individual client connections to the server, handling tasks such as sending and receiving messages. Channels, implemented through the Channel class, provide a means for organizing and managing group communications among clients.

### Detailed Description:
The project is implemented in C++98, adhering to the standard while utilizing advanced features and techniques for robustness and efficiency. The use of epoll, a scalable I/O event notification mechanism available on Linux systems, ensures optimal handling of multiple client connections without unnecessary resource consumption.

### Server Class:

Manages incoming client connections and facilitates communication between clients.
Utilizes epoll for efficient event-driven handling of client interactions.
Maintains data structures such as maps to manage clients and channels.
Provides methods for creating, joining, and leaving channels.
Implements thread safety mechanisms to handle concurrent client requests.

### Client Class:

Represents individual client connections to the server.
Handles sending and receiving messages to and from the server.
Manages the state of the client connection, including connection status and associated channel membership.
Implements error handling and recovery mechanisms for robust communication.

### Channel Class:

Represents communication channels for organizing group interactions among clients.
Provides methods for adding and removing clients from channels.
Supports sending messages to all clients within a channel.
Implements channel-specific settings and permissions.

### Additional Features:

Support for command-line interface for server administration and client interaction.
Implementation of user authentication and access control mechanisms.
Integration of logging functionality for monitoring and debugging purposes.
Implementation of signal handling for graceful server shutdown and error recovery.

## :toolbox: Getting Started

### :bangbang: Prerequisites

- install Konversation : is a user-friendly Internet Relay Chat (IRC) client by KDE. It provides easy access to standard IRC networks such as Libera, where the KDE IRC channels can be found.<a href="https://snapcraft.io/konversation"> Here</a>


### :running: Run Locally

Clone the project

```bash
https://github.com/graiolo/IRC.git
```
Go to the project directory
```bash
cd IRC
```
compile
```bash
make
```
make the server listen
```bash
./ircserv <port number> <password>
```
the shell shows you a message like this
```bash
Server started and listening on IP <ip> port <port>
```
open konversation, up left side click on file-> server list-> new
choos a name for you server and click add under field Servers
insert the listening ip of the server message, and port and password choosed on launching
save and select your server in teh list and click connect
Now you're connected as client and you're on welome channel
you can use some command:
To create a channel
```bash
/JOIN #<name_of_channel>
```
you can send prv message or chat in the channel
You can moderate channel (User modes: +o, Channel modes: +b+l+i+k+t -b-l-i-k-t) you can use single or combinate mode commands
example of user mode to give operator privilages
```bash
/MODE #CHANNEL +o user_name
```
example of channel mode to set a password to enter in channel
```bash
/MODE #CHANNEL +k password
```
You can chat also with bot in WELCOME channel, try to write in the channel !WE followed by one of this words: -ciao -ping -aiuto -Gabry -Beppe -Nelly -Simo -Alegre -Niz -Victor -Ste -Btani -Lupe -Pacci -Dip -Fratm
```bash
!WE ciao
```
you can kick someone frome a channel if you are operator
```bash
/KICK #CHANNEL username
```
fun with other command like: /PART /INVITE /PRIVMSG /QUIT /TOPIC

## Conclusion

This project demonstrates a comprehensive implementation of a multi-client chat server in C++, leveraging advanced features such as epoll for efficient event handling. The modular design allows for scalability and flexibility, making it suitable for various chat application scenarios. Additionally, adherence to C++98 standards ensures compatibility and portability across different environments.
