#ifndef SERVER_H
#define SERVER_H

#include <string>

class Server {
private:
    std::string servicename;

public:
    /* Constructor del Server */
    explicit Server(const std::string& servicename);

    void run();

    // No se pueden hacer copias
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
};

#endif  // SERVER_H
