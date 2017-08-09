#include "ChronoMessages.pb.h"
#include "DSRCNode.h"

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
    std::vector<uint8_t> buffer(sizeof(uint32_t) + message.ByteSize());
    uint32_t *size = (uint32_t *)buffer.data();
    *size = message.ByteSize();
    message.SerializeToArray(buffer.data() + sizeof(uint32_t), buffer.size());
    sendQueue.enqueue(buffer);
}

ChronoMessages::DSRCMessage DSRCNode::receive() {
    ChronoMessages::DSRCMessage message;
    auto buffer = receiveQueue.dequeue();
    message.ParseFromArray(buffer.data(), buffer.size());
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
            socket.send(boost::asio::buffer(buffer.data(), buffer.size()));
        }
    });
}

void DSRCNode::startReceive(){
    listener = new std::thread([&] {
        while (socket.is_open()) {
            uint32_t size;
            std::unique_lock<std::mutex> lock(socketMutex);
            socket.receive(boost::asio::buffer(&size, sizeof(uint32_t)));
            std::vector<uint8_t> buffer(size);
            socket.receive(boost::asio::buffer(buffer.data(), size));
            lock.unlock();
            receiveQueue.enqueue(buffer);
        }
    });
}
