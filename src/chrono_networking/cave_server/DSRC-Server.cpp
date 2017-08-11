#include <iostream>
#include <boost/asio.hpp>
#include <thread>

#include "MessageCodes.h"
#include "ChronoMessages.pb.h"
#include "ChSafeQueue.h"
#include "ChNetworkHandler.h"

std::map<int, boost::asio::ip::udp::endpoint> endpoints;
std::set<int> connectionNumbers;

void handleConnections(ChServerHandler& handler, boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount);

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: " << std::string(argv[0]) << " <Portnumber>" << std::endl;
        return 1;
    }

    unsigned short portNumber = (unsigned short)std::stoi(std::string(argv[1]));
    ChServerHandler handler(portNumber,
        [&] (boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount) {
            handleConnections(handler, tcpSocket, connectionCount);
        }
    );

    handler.beginSend();
    handler.beginListen();

    std::cout << "server handler created" << std::endl;
    while (true) {
        auto messPair = handler.popMessage();
        auto message = std::static_pointer_cast<ChronoMessages::DSRCMessage>(messPair.second);
        if (connectionNumbers.find(message->idnumber()) != connectionNumbers.end()) {
            for (auto endpointPair : endpoints) {
                if (endpointPair.first != message->idnumber()) {
                    handler.pushMessage(endpointPair.second, *messPair.second);
                }
            }
            if (endpoints.find(message->idnumber()) == endpoints.end()) {
                endpoints.insert(std::make_pair(message->idnumber(), messPair.first));
            }
        }
    }
}

void handleConnections(ChServerHandler& handler, boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount){
    uint8_t requestMessage;
    tcpSocket.receive(boost::asio::buffer(&requestMessage, sizeof(uint8_t)));
    if (requestMessage == CONNECTION_REQUEST) {
        uint8_t acceptMessage = CONNECTION_ACCEPT;
        tcpSocket.send(boost::asio::buffer(&acceptMessage, sizeof(uint8_t)));
        tcpSocket.send(boost::asio::buffer((uint32_t *)(&connectionCount), sizeof(uint32_t)));
        connectionNumbers.insert(connectionCount);
        std::cout << "connection " << connectionCount << " added" << std::endl;
        connectionCount++;
    } else {
        uint8_t declineMessage = CONNECTION_DECLINE;
        tcpSocket.send(boost::asio::buffer(&declineMessage, sizeof(uint8_t)));
    }
}
