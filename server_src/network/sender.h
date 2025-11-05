#ifndef SERVER_SENDER_H
#define SERVER_SENDER_H

#include "../../common_src/queue.h"
#include "../../common_src/socket.h"
#include "../../common_src/thread.h"
#include "../../common_src/dtos.h" 



class Sender: public Thread {
    Socket& socket;  
    Queue<ServerMsg>& mesgs_queue;
    std::atomic<bool> is_alive;

public:
    Sender(Socket& socket, Queue<ServerMsg>& mesg_queue);

    void run() override;
    void kill();
    bool status();

    Sender(const Sender&) = delete;
    Sender& operator=(const Sender&) = delete;
};

#endif  // SERVER_SENDER_H
