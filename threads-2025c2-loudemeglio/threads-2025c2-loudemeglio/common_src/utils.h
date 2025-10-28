
#ifndef UTILS_H
#define UTILS_H

#define QUIT 'q'


enum CodeActions { NITRO_ON = 0x07, NITRO_OFF = 0x08, MSG_SERVER = 0x10, MSG_CLIENT = 0x04 };

struct ServerMsg {
    std::uint8_t type;  // 0x10
    std::uint16_t cars_with_nitro;
    std::uint8_t nitro_status;  // 0x07 (activated) o 0x08 (expired)
} __attribute__((packed));

struct Command {
    std::uint8_t action;
    int id;
};

#endif  // UTILS_H
