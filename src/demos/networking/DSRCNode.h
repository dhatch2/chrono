#include <boost/asio.hpp>
#include <thread>

#include "ChSafeQueue.h"

class DSRCNode {
public:
    DSRCNode(std::string hostname, unsigned short portNumber);
    ~DSRCNode();

    void send(ChronoMessages::DSRCMessage& message);
    ChronoMessages::DSRCMessage receive();
    int waiting();

private:
    void startSend();
    void startReceive();

    std::thread *sender;
    std::thread *listener;
    boost::asio::ip::tcp::socket socket;
    ChSafeQueue<std::string> sendQueue;
    ChSafeQueue<std::string> receiveQueue;
    std::mutex socketMutex;
};
