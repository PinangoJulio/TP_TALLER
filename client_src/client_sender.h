
#ifndef CLIENT_SENDER_H
#define CLIENT_SENDER_H
#include "common_src/dtos.h"
#include "common_src/queue.h"
#include "common_src/thread.h"

class ClientProtocol;

class ClientSender : public Thread {
private:
    ClientProtocol& protocol;
    Queue<ComandMatchDTO>& commands_queue;

public:
    explicit ClientSender(ClientProtocol& protocol, Queue<ComandMatchDTO>& cmd_queue);
    void run() override;
    ClientSender(const ClientSender&) = delete;
    ClientSender& operator=(const ClientSender&) = delete;
    ClientSender(ClientSender&&) = default;
    ClientSender& operator=(ClientSender&&) = default;

    ~ClientSender();
};

#endif  // CLIENT_SENDER_H
