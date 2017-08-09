#include <boost/asio.hpp>
#include <thread>

#include "ChSafeQueue.h"

class DSRCNode {
public:
    DSRCNode(std::string hostname, std::string port);
    ~DSRCNode();

    void send(ChronoMessages::DSRCMessage& message);
    ChronoMessages::DSRCMessage receive();
    int waiting();
    void startSend();
    void startReceive();

private:

    std::thread *sender;
    std::thread *listener;
    boost::asio::ip::tcp::socket socket;
    ChSafeQueue<std::vector<uint8_t>> sendQueue;
    ChSafeQueue<std::vector<uint8_t>> receiveQueue;
    std::mutex socketMutex;
};
