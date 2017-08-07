#include <iostream>
#include <boost/asio.hpp>
#include <thread>

#include "MessageCodes.h"
#include "ChronoMessages.pb.h"
#include "ChSafeQueue.h"
#include "ChNetworkHandler.h"

void handleConnections(ChServerHandler& handler, boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount);

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Usage: " << std::string(argv[0]) << " <ServerHostname> <port number>" << std::endl;
        return 1;
    }

    std::cout << "Connecting to CAVE Agent..." << std::endl;
    unsigned short portNumber = (unsigned short)std::stoi(std::string(argv[2]));
    ChServerHandler agentConnectionHandler(portNumber,
        [&] (boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount) {
            handleConnections(agentConnectionHandler, tcpSocket, connectionCount);
        }
    );
    agentConnectionHandler.beginListen();
    agentConnectionHandler.beginSend();
    std::cout << "Connected to CAVE Agent." << std::endl;

    std::cout << "Connecting to DSRC Server..." << std::endl;
    ChClientHandler serverConnectionHandler(argv[1], argv[2]);
    serverConnectionHandler.beginSend();
    serverConnectionHandler.beginListen();
    std::cout << "Connected to DSRC Server." << std::endl;
}

void handleConnections(ChServerHandler& handler, boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount){
    uint8_t requestMessage;
    tcpSocket.receive(boost::asio::buffer(&requestMessage, sizeof(uint8_t)));
    if (requestMessage == CONNECTION_REQUEST) {
        uint8_t acceptMessage = CONNECTION_ACCEPT;
        tcpSocket.send(boost::asio::buffer(&acceptMessage, sizeof(uint8_t)));
        tcpSocket.send(boost::asio::buffer((uint32_t *)(&connectionCount), sizeof(uint32_t)));
        connectionCount++;
    } else {
        uint8_t declineMessage = CONNECTION_DECLINE;
        tcpSocket.send(boost::asio::buffer(&declineMessage, sizeof(uint8_t)));
    }
    if (connectionCount > 0) handler.setAccepting(false);
}
