#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "tcpbridge.h"
#include "json.hpp"

using json = nlohmann::json;

static const int PORT = 8080;
static const int BUFFER_SIZE = 4096;
static const int MAX_EPOLL_EVENTS = 16;

/// @brief helper class to manage different data source: tcp socket, fifos
class FileDescriptor {
public:
    FileDescriptor() : fd_(-1) {}
    explicit FileDescriptor(int fd) : fd_(fd) {}

    ~FileDescriptor() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) {
        fd_ = other.fd_;
        other.fd_ = -1;
    }

    FileDescriptor& operator=(FileDescriptor&& other) {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }

    void reset(int newfd = -1) {
        if (fd_ >= 0) ::close(fd_);
        fd_ = newfd;
    }

private:
    int fd_;
};

/// @brief Set the opened file descriptor to non blocking
/// @param fd 
static void set_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/// @brief write len amount of data to the file descriptor
/// @param fd opened file descriptor
/// @param data 
/// @param len 
/// @return true if no error
static bool write_all_nonblocking(int fd, const char* data, std::size_t len) {
    std::size_t written = 0;

    while (written < len) {
        ssize_t n = ::write(fd, data + written, len - written);

        if (n > 0) {
            written += static_cast<std::size_t>(n);
        }
        else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

/// @brief Convert a raw byte array to SLIP encoded data
/// @param data raw data
/// @return SLIP encoded data
std::vector<uint8_t> slipEncoding(std::vector<uint8_t> data)
{
    std::vector<uint8_t> output;
    output.push_back(SLIP_END);
    for(auto value: data) {
        switch(value) {
        case SLIP_END:
            output.push_back(SLIP_ESC);
            output.push_back(SLIP_ESC_END);
            break;
        case SLIP_ESC:
            output.push_back(SLIP_ESC);
            output.push_back(SLIP_ESC_ESC);
            break;
        default:
            output.push_back(value);
        }
    }
    output.push_back(SLIP_END);
    return output;
}

json processPackage(std::vector<uint8_t> data) {
    if(data.size()) {
        json j_from_cbor;
        try {
            j_from_cbor = json::from_cbor(data);
        }
        catch (...) {
            return json();
        }
        return j_from_cbor;
    }
    return json();
}

int main() {
    // Create TCP server socket
    FileDescriptor server_fd(::socket(AF_INET, SOCK_STREAM, 0));
    if (!server_fd.valid()) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    int enable_reuse = 1;
    ::setsockopt(server_fd.get(), SOL_SOCKET, SO_REUSEADDR,
                 &enable_reuse, sizeof(enable_reuse));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(server_fd.get(), (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (::listen(server_fd.get(), 1) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    set_nonblocking(server_fd.get());
    std::cout << "[info] Listening on port " << PORT << "\n";

    // Open FIFOs
    FileDescriptor fifo_mosi(::open("mosi.fifo", O_RDONLY | O_NONBLOCK));
    FileDescriptor fifo_miso(::open("miso.fifo", O_WRONLY | O_NONBLOCK));
    FileDescriptor fifo_passive(::open("passive.fifo", O_RDONLY | O_NONBLOCK));

    if (!fifo_mosi.valid() || !fifo_miso.valid() || !fifo_passive.valid()) {
        std::cerr << "Failed to open FIFOs\n";
        return 1;
    }

    // Setup epoll
    FileDescriptor epfd(::epoll_create1(0));
    if (!epfd.valid()) {
        std::cerr << "epoll_create1 failed\n";
        return 1;
    }

    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));

    // Register server FD for incoming connections
    ev.events = EPOLLIN;
    ev.data.fd = server_fd.get();
    ::epoll_ctl(epfd.get(), EPOLL_CTL_ADD, server_fd.get(), &ev);

    // Register FIFO (mosi) for input
    ev.events = EPOLLIN;
    ev.data.fd = fifo_mosi.get();
    ::epoll_ctl(epfd.get(), EPOLL_CTL_ADD, fifo_mosi.get(), &ev);

    // Register FIFO (passive) for input
    ev.events = EPOLLIN;
    ev.data.fd = fifo_passive.get();
    ::epoll_ctl(epfd.get(), EPOLL_CTL_ADD, fifo_passive.get(), &ev);

    // Client socket (initially none)
    FileDescriptor client_fd;

    std::vector<uint8_t> buffer(BUFFER_SIZE);
    std::vector<uint8_t> buffer2;
    std::vector<uint8_t> rx_packet;
    std::vector<uint8_t> tx_packet;

    // Main event loop
    std::vector<epoll_event> events(MAX_EPOLL_EVENTS);
    bool slipEscaped = false;

    while (true) {
        int eventCount = ::epoll_wait(epfd.get(), &events[0],
                                 MAX_EPOLL_EVENTS, -1);

        if (eventCount < 0) {
            std::cerr << "epoll_wait error\n";
            continue;
        }

        for (int ii = 0; ii < eventCount; ii++) {
            int fd = events[ii].data.fd;
            uint32_t evmask = events[ii].events;

            // New client connection
            if (fd == server_fd.get()) {
                int newfd = ::accept(server_fd.get(), NULL, NULL);
                if (newfd >= 0) {
                    std::cout << "[info] Client connected\n";

                    // Clean up any old connection
                    client_fd.reset(newfd);
                    set_nonblocking(client_fd.get());

                    epoll_event cev;
                    std::memset(&cev, 0, sizeof(cev));
                    cev.events = EPOLLIN | EPOLLRDHUP;
                    cev.data.fd = client_fd.get();
                    ::epoll_ctl(epfd.get(), EPOLL_CTL_ADD,
                                client_fd.get(), &cev);
                }
            }

            // new data available in the mosi fifo, send it to the tcp client
            else if (fd == fifo_mosi.get()) {
                ssize_t n = ::read(fifo_mosi.get(), &buffer[0], BUFFER_SIZE);
                if (n > 0 && client_fd.valid()) {
                    for(uint32_t ii=0; ii<n; ii++) {
                        buffer2.push_back(buffer[ii]);
                    }
                    //std::string str(buffer2.begin(), buffer2.end());
                    //std::cout << "read data from FIFO:" << std::endl;
                    //std::cout << str << std::endl;
                    for(auto ii:buffer2) {
                        if(ii=='\n') {
                            if(tx_packet.size()>0) {
                                json j;
                                j = json::parse(tx_packet);
                                std::vector<uint8_t> v_cbor = json::to_cbor(j);
                                std::vector<uint8_t> v_slip = slipEncoding(v_cbor);
                                if (!write_all_nonblocking(client_fd.get(),reinterpret_cast<char*>(&v_slip[0]), v_slip.size())) {
                                    std::cerr << "[error] Failed writing to socket\n";
                                }
                                tx_packet.clear();
                            }
                        } else {
                            tx_packet.push_back(ii);
                        }
                    }
                    buffer2.clear();
                }
            }

            // new data available from the tcp socket, send it to the  miso fifo
            else if (client_fd.valid() && fd == client_fd.get()) {
                if (evmask & EPOLLRDHUP) {
                    std::cout << "[info] Client disconnected\n";
                    client_fd.reset();
                    continue;
                }

                ssize_t n = ::recv(client_fd.get(), &buffer[0],
                                   BUFFER_SIZE, 0);

                if (n > 0) {
                    for(uint32_t ii=0; ii<n; ii++) {
                        buffer2.push_back(buffer[ii]);
                    }
                    //std::string str(buffer2.begin(), buffer2.end());
                    //std::cout << "read data from socket:" << std::endl;
                    //std::cout << str << std::endl;
                    for(auto ii:buffer2) {
                        if(ii == SLIP_ESC) {
                            slipEscaped = true;
                        } else {
                            if (ii == SLIP_END) {
                                if(rx_packet.size() > 0) {
                                    /* process fully received packet */
                                    json res = processPackage(rx_packet);
                                    rx_packet.clear();
                                    auto str = res.dump();
                                    str.append("\n");
                                    std::vector<std::uint8_t> json_data(str.begin(), str.end());
                                    if (!write_all_nonblocking(fifo_miso.get(),
                                                    reinterpret_cast<char*>(&json_data[0]), json_data.size())) {
                                        std::cerr << "[error] Failed writing to FIFO\n";
                                    }
                                    //std::cout << "parsed JSON: " << std::endl;
                                    //std::cout << str << std::endl;
                                }
                            } else if (slipEscaped) {
                                if (ii == SLIP_ESC_END)
                                    rx_packet.push_back(SLIP_END);
                                else if (ii == SLIP_ESC_ESC)
                                    rx_packet.push_back(SLIP_ESC);
                            } else {
                                rx_packet.push_back(ii);
                            }
                            slipEscaped = false;
                        }
                    }
                    buffer2.clear();
                }
                else if (n == 0) {
                    std::cout << "[info] Client disconnected\n";
                    client_fd.reset();
                }
            }

            // new data available from the passive fifo, read it and discard it
            else if (fd == fifo_passive.get()) {
                ssize_t n = ::read(fifo_passive.get(), &buffer[0], BUFFER_SIZE);
            }
        }
    }

    return 0;
}
