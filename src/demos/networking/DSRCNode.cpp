#include "ChronoMessages.pb.h"
#include "DSRCNode.h"

DSRCNode::DSRCNode(std::string hostname, unsigned short portNumber) :
    socket(*(new boost::asio::io_service)) {
    boost::asio::ip::tcp::acceptor acceptor(socket.get_io_service(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), portNumber));
    boost::system::error_code acceptError;
    acceptor.accept(socket);
    socket.non_blocking(true);
}

DSRCNode::~DSRCNode() {
    socket.close();
}

void DSRCNode::send(ChronoMessages::DSRCMessage& message) {
    std::string buffer;
    message.SerializeToString(&buffer);
    sendQueue.enqueue(buffer);
}

ChronoMessages::DSRCMessage DSRCNode::receive() {
    ChronoMessages::DSRCMessage message;
    message.ParseFromString(receiveQueue.dequeue());
    return message;
}

int DSRCNode::waiting() {
    return receiveQueue.size();
}

void DSRCNode::startSend() {
    sender = new std::thread([&] {
        while (socket.is_open()) {
            std::string buffer = sendQueue.dequeue();
            std::unique_lock<std::mutex> lock(socketMutex);
            socket.send(boost::asio::buffer(buffer));
        }
    });
}

void DSRCNode::startReceive(){
    listener = new std::thread([&] {
        while (socket.is_open()) {
            std::string buffer;
            std::unique_lock<std::mutex> lock(socketMutex);
            //socket.receive(boost::asio::buffer(buffer, 2048));
            receiveQueue.enqueue(buffer);
        }
    });
}
