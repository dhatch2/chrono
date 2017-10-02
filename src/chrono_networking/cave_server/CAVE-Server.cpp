// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Dylan Hatch
// =============================================================================
//
// Main file for the Chrono simulation server.
//
// =============================================================================

#include <iostream>
#include <boost/asio.hpp>
#include <thread>

#include "MessageCodes.h"
//#include "ChronoMessages.pb.h"
#include "ChSafeQueue.h"
#include "ChNetworkHandler.h"
#include "World.h"

void handleConnections(World& world, ChSafeQueue<std::function<void()>>& worldQueue, boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount);

void processMessages(World& world, ChSafeQueue<std::function<void()>>& worldQueue, ChServerHandler& handler);

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: " << std::string(argv[0]) << " <port number>" << std::endl;
        return 1;
    }

    unsigned short portNumber = (unsigned short)std::stoi(std::string(argv[1]));
    World world;
    ChSafeQueue<std::function<void()>> worldQueue;

    int count = std::thread::hardware_concurrency();
    // For the main thread
    count--;
    ChServerHandler handler(portNumber,
        [&] (boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount) {
            handleConnections(world, worldQueue, tcpSocket, connectionCount);
        }
    );
    // For the listener threads
    count--;
    handler.beginListen();
    handler.beginSend();

    // Worker threads
    std::vector<std::thread> workers;
    for (count; count > 0; count--) {
        workers.emplace_back(processMessages, std::ref(world), std::ref(worldQueue), std::ref(handler));
    }

    while (true) {
        worldQueue.dequeue()();
    }

    for (size_t i = 0; i < workers.size(); i++) {
        workers[i].join();
    }

    return 0;
}

void handleConnections(World& world, ChSafeQueue<std::function<void()>>& worldQueue, boost::asio::ip::tcp::socket& tcpSocket, int& connectionCount){
    uint8_t requestMessage;
    tcpSocket.receive(boost::asio::buffer(&requestMessage, sizeof(uint8_t)));
    if (requestMessage == CONNECTION_REQUEST) {
        uint8_t acceptMessage = CONNECTION_ACCEPT;
        tcpSocket.send(boost::asio::buffer(&acceptMessage, sizeof(uint8_t)));
        tcpSocket.send(boost::asio::buffer((uint32_t *)(&connectionCount), sizeof(uint32_t)));
        worldQueue.enqueue([&, connectionCount] { world.registerConnectionNumber(connectionCount); });
        connectionCount++;
    } else {
        uint8_t declineMessage = CONNECTION_DECLINE;
        tcpSocket.send(boost::asio::buffer(&declineMessage, sizeof(uint8_t)));
    }
}

void processMessages(World& world, ChSafeQueue<std::function<void()>>& worldQueue, ChServerHandler& handler) {
    // TODO: Fix major memory issues. Make copies of everything to avoid memory errors.
    while (true) {
        auto messagePair = handler.popMessage();
        boost::asio::ip::udp::endpoint& endpoint = messagePair.first;
        std::shared_ptr<google::protobuf::Message>& message = messagePair.second;
        //message->CheckInitialized();

        auto reflection = message->GetReflection();
        auto descriptor = message->GetDescriptor();
        auto conDesc = descriptor->FindFieldByName(CONNECTION_NUMBER_FIELD);
        int connectionNumber = reflection->GetInt32(*message, conDesc);
        auto idDesc = descriptor->FindFieldByName(ID_NUMBER_FIELD);
        int idNumber = reflection->GetInt32(*message, idDesc);
        std::string type = descriptor->full_name();

        endpointProfile *profile = world.verifyConnection(connectionNumber, endpoint);
        double chtime;
        double currtime;
        if (type.compare(VEHICLE_MESSAGE_TYPE) == 0 && profile != NULL) {
            auto timeDesc = descriptor->FindFieldByName(CH_TIME_FIELD);
            chtime = reflection->GetDouble(*message, timeDesc);
            auto curr = world.getElement(connectionNumber, idNumber);
            auto currReflection = curr->GetReflection();
            auto currDescriptor = curr->GetDescriptor();
            auto currTimeDesc = descriptor->FindFieldByName(CH_TIME_FIELD);
            currtime = currReflection->GetDouble(*curr, currTimeDesc);
        }

        if (profile == NULL) {
            worldQueue.enqueue([&, message] {
                if(world.registerEndpoint(endpoint, connectionNumber)) {
                    profile = world.verifyConnection(connectionNumber, endpoint);
                    if (profile != NULL) {
                        if (type.compare(MESSAGE_PACKET_TYPE) == 0) {
                            world.updateElementsOfProfile(profile, message);
                        } else {
                            world.updateElement(message, profile, idNumber);
                        }
                        std::cout << "Endpoint " << endpoint << " registered" << std::endl;
                    }
                }
            });
        } else if (type.compare(MESSAGE_PACKET_TYPE) == 0) {
            worldQueue.enqueue([&, message] { world.updateElementsOfProfile(profile, message); });
        } else if (chtime > currtime){
            worldQueue.enqueue([&, message] { world.updateElement(message, profile, idNumber); });
        }
        if (profile != NULL) {
            auto packet = world.generateWorldPacket();
            //packet->CheckInitialized();
            handler.pushMessage(endpoint, *packet);
        }
    }
}
