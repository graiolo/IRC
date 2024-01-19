#include "../includes/server.hpp"
#include "../includes/ircserver.hpp"



int main(int argc, char *argv[]) {
	if (argc != 3) {
			std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
			return (EXIT_FAILURE);
	}
    Server server(std::atoi(argv[1]), argv[2]);
    try {
        Server::serverInit( );
        Server::addServerToEpoll( );
        Server::runServer( );
    }
    catch (const std::exception &e) {
        std::cerr << e.what( ) << std::endl;
    }
}