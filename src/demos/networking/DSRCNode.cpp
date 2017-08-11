#include "ChronoMessages.pb.h"
#include "DSRCNode.h"
#include "MessageCodes.h"

#include <iostream>

DSRCNode::DSRCNode(std::string hostname, std::string port) :
    socket(*(new boost::asio::io_service)) {
    boost::asio::ip::tcp::resolver tcpResolver(socket.get_io_service());
    boost::asio::ip::tcp::resolver::query tcpQuery(hostname, port);
    boost::asio::ip::tcp::resolver::iterator endpointIterator = tcpResolver.resolve(tcpQuery);
    boost::asio::connect(socket, endpointIterator);
    socket.non_blocking(true);
}

DSRCNode::~DSRCNode() {
    socket.close();
    listener->join();
    sender->join();
    delete &socket.get_io_service();
}

void DSRCNode::send(ChronoMessages::DSRCMessage& message) {
    std::vector<uint8_t> buffer(sizeof(uint8_t) + sizeof(uint32_t) + message.ByteSize());
    uint8_t *messageType = buffer.data();
    *messageType = DSRC_MESSAGE;
    messageType++;
    uint32_t *size = (uint32_t *)messageType;
    *size = message.ByteSize();
    message.SerializeToArray(buffer.data() + sizeof(uint32_t), buffer.size());
    sendQueue.enqueue(buffer);
}

ChronoMessages::DSRCMessage DSRCNode::receive() {
    ChronoMessages::DSRCMessage message;
    std::vector<uint8_t> buffer;
    do {
        buffer = receiveQueue.dequeue();
        if (!message.ParseFromArray(buffer.data(), buffer.size())) {
            std::vector<uint8_t> request(1, RESET_REQUEST);
            sendQueue.enqueue(request);
        }
    } while(!message.IsInitialized());
    return message;
}

int DSRCNode::waiting() {
    return receiveQueue.size();
}

void DSRCNode::startSend() {
    sender = new std::thread([&] {
        while (socket.is_open()) {
            std::vector<uint8_t> buffer = sendQueue.dequeue();
            std::unique_lock<std::mutex> lock(socketMutex);
            socket.send(boost::asio::buffer(buffer));
        }
    });
}

void DSRCNode::startReceive(){
    listener = new std::thread([&] {
        int size = 0;
        boost::system::error_code error;
        while (socket.is_open()) {
            uint8_t messageType;
            int received = 0;
            do {
                std::unique_lock<std::mutex> lock(socketMutex);
                received = boost::asio::read(socket, boost::asio::buffer(&messageType, sizeof(uint8_t)), error);
            } while(received < sizeof(uint8_t) && socket.is_open());
            std::cout << "message type: " << (int)messageType << std::endl;
            if (messageType == RESET_REQUEST) {
                std::vector<uint8_t> nullBuff(100, NULL_MESSAGE);
                sendQueue.enqueue(nullBuff);
            } else if (messageType == DSRC_MESSAGE) {
                std::cout << "receiving dsrc message..." << std::endl;
                boost::system::error_code error;
                uint32_t size;
                int s = sizeof(uint8_t);
                uint8_t *buff = (uint8_t *)&size;
                s = sizeof(uint32_t);
                received = 0;
                do {
                    std::unique_lock<std::mutex> lock(socketMutex);
                    received = boost::asio::read(socket, boost::asio::buffer(buff, s), error);
                    s -= received;
                    buff += received;
                } while(s > 0 && socket.is_open());
                s = size;
                std::cout << size << std::endl;
                std::vector<uint8_t> buffer(size);
                buff = buffer.data();
                if (size > 0) do {
                    std::unique_lock<std::mutex> lock(socketMutex);
                    received = boost::asio::read(socket, boost::asio::buffer(buff, s), error);
                    s -= received;
                    buff += received;
                } while(s > 0 && socket.is_open());
                if (size > 0) receiveQueue.enqueue(buffer);
                std::cout << buffer.size() << std::endl;
            }
            /*int available;
            do {
                std::unique_lock<std::mutex> lock(socketMutex);
                // Receives bytes without removing any
                socket.receive(boost::asio::null_buffers(), 0, error);
                //TODO: Handle error message here
                boost::system::error_code availableError;
                available = socket.available(availableError);
                // Once the udp stack has something, we receive it into the buffer and return.
                std::cout << error << std::endl;
                if (!availableError && available >= sizeof(uint32_t)) {
                    socket.receive(boost::asio::buffer(&size, sizeof(uint32_t)), 0, error);
                }
                //TODO: Handle error message again here
            } while ((available < sizeof(uint32_t) || error == boost::asio::error::would_block) && socket.is_open());

            do {
                std::unique_lock<std::mutex> lock(socketMutex);
                // Receives bytes without removing any
                socket.receive(boost::asio::null_buffers(), 0, error);
                //TODO: Handle error message here
                boost::system::error_code availableError;
                available = socket.available(availableError);
                // Once the udp stack has something, we receive it into the buffer and return.
                std::cout << available << std::endl;
                if (!availableError && available >= size) {
                    std::vector<uint8_t> buffer(size);
                    std::unique_lock<std::mutex> lock(socketMutex);
                    socket.receive(boost::asio::buffer(buffer));
                    lock.unlock();
                    receiveQueue.enqueue(buffer);
                    std::cout << "message received" << std::endl;
                }
                //TODO: Handle error message again here
            } while ((available < size || error == boost::asio::error::would_block) && socket.is_open());*/

            /*uint32_t size;
            std::unique_lock<std::mutex> lock(socketMutex);
            boost::system::error_code error;
            int available = 0;
            while (available < sizeof(uint32_t) && !error) {
                std::unique_lock<std::mutex> lock(socketMutex);
                socket.receive(boost::asio::null_buffers(), 0, error);
                lock.unlock();
                available = socket.available();
            }
            socket.receive(boost::asio::buffer(&size, sizeof(uint32_t)), 0, error);
            while (available < size && !error) {
                std::unique_lock<std::mutex> lock(socketMutex);
                socket.receive(boost::asio::null_buffers(), 0, error);
                lock.unlock();
                available = socket.available();
            }
            if (!error) {
                std::vector<uint8_t> buffer(size);
                std::unique_lock<std::mutex> lock(socketMutex);
                socket.receive(boost::asio::buffer(buffer.data(), size));
                lock.unlock();
                receiveQueue.enqueue(buffer);
                std::cout << "message received" << std::endl;
            }*/
        }
    });
}
