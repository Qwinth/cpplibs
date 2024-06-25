// version 1.9.9-c2
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <mutex>
#include <utility>
#include <cstring>
#include <cstdint>
#pragma once

#include "strlib.hpp"

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "windowsHeader.hpp"

#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif

#pragma comment(lib, "ws2_32.lib")

#define GETSOCKETERRNO() (WSAGetLastError())
#define poll(a, b, c) (WSAPoll(a, b, c))
#define MSG_CONFIRM 0
#define MSG_WAITALL 0

typedef long int64_t;
typedef int socklen_t;

#elif __linux__
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define GETSOCKETERRNO() (errno)

#endif

struct sockaddress_t {
    std::string ip;
    uint16_t port = 0;

    std::string str() { return strformat("%s:%d", ip.c_str(), port); }
};

struct sockrecv_t {
    std::string string;
    char* buffer = nullptr;
    int64_t size = 0;
    sockaddress_t addr;

    ~sockrecv_t() { delete[] buffer; }
    sockrecv_t() {}
    sockrecv_t(const sockrecv_t& other) noexcept {
        buffer = new char[other.size];
        std::memcpy(buffer, other.buffer, other.size);

        string = other.string;
        size = other.size;
        addr = other.addr;
    }

    sockrecv_t& operator=(const sockrecv_t& other) {
        sockrecv_t copy = other;
        swap(copy);

        return *this;
    }

    void swap(sockrecv_t& other) {
        std::swap(buffer, other.buffer);
        std::swap(size, other.size);
        std::swap(string, other.string);
        std::swap(addr, other.addr);
    }
};

class Socket {
    int sock_af = AF_INET;
    int sock_type = SOCK_STREAM;
    sockaddress_t address;

    bool blocking = true;

    std::shared_ptr<std::mutex> msgSendMtx = nullptr;
    std::shared_ptr<std::mutex> msgRecvMtx = nullptr;

#ifdef _WIN32
    WSADATA wsa;
    SOCKET s;
#else
    int s;
#endif

    bool checkIp(std::string ipaddr) {
        for (auto &i : split(ipaddr, '.')) if (!std::all_of(i.begin(), i.end(), ::isdigit)) return false;
        return true;
    }

    sockaddr_in make_sockaddr_in(std::string ipaddr, uint16_t port) {
        sockaddr_in tmpaddr;

        if (ipaddr != "") {
            if (!checkIp(ipaddr)) ipaddr = gethostbyname(ipaddr);

            tmpaddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
        }

        else tmpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        tmpaddr.sin_family = sock_af;
        tmpaddr.sin_port = htons(port);

        return tmpaddr;
    }

    void mutexInit() {
        msgSendMtx = std::make_shared<std::mutex>();
        msgRecvMtx = std::make_shared<std::mutex>();
    }

    sockaddress_t sockaddr_in_to_sockaddress_t(sockaddr_in addr) { return { inet_ntoa(addr.sin_addr), ntohs(addr.sin_port) }; }
    friend bool operator==(Socket arg1, Socket arg2);

    void copy(const Socket& obj) {
        s = obj.s;
        sock_af = obj.sock_af;
        sock_type = obj.sock_type;
        address = obj.address;
        blocking = obj.blocking;
    }

    void swap(Socket& obj) {
        std::swap(s, obj.s);
        std::swap(sock_af, obj.sock_af);
        std::swap(sock_type, obj.sock_type);
        std::swap(address, obj.address);
        std::swap(blocking, obj.blocking);
    }
public:
    Socket() {}
    Socket(int _af, int _type) {  open(_af, _type); }

    Socket(int fd, int _af, int _type, sockaddress_t addr) {
        s = fd;
        sock_af = _af;
        sock_type = _type;
        address = addr;
#ifdef _WIN32
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        mutexInit();
    }

    Socket(const Socket& obj) {
        copy(obj);
    }

    Socket(Socket&& obj) {
        swap(obj);
    }

    Socket& operator=(const Socket& obj) {
        copy(obj);

        return *this;
    }

    Socket& operator=(Socket&& obj) {
        swap(obj);

        return *this;
    }

    void setblocking(bool _blocking) {
    #ifdef __linux__
        if (_blocking) fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK);
        else fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);
    #else
        unsigned long __blocking = !_blocking;
        ioctlsocket(s, FIONBIO, &__blocking);
    #endif
        blocking = _blocking;
    }

    bool is_blocking() { return blocking; }

    void open(int _af, int _type) {
#ifdef _WIN32
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        sock_af = _af;
        sock_type = _type;
        if ((s = ::socket(_af, _type, 0)) == INVALID_SOCKET) throw GETSOCKETERRNO();

        mutexInit();
    }

    void baseServer(std::string ipaddr, uint16_t port, int _listen = 0, bool reuseaddr = false) {
        if (reuseaddr) setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
        bind(ipaddr, port);
        listen(_listen);
    }

    void connect(std::string ipaddr, uint16_t port) {
        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::connect(s, (sockaddr*)&sock, sizeof(sockaddr_in)) == SOCKET_ERROR) throw GETSOCKETERRNO();
    }

    void connect(std::string addr) {
        auto tmpaddr = split(addr, ':');

        connect(tmpaddr[0], std::stoi(tmpaddr[1]));
    }

    void bind(std::string ipaddr, uint16_t port) {
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::bind(s, (sockaddr*)&sock, sizeof(sockaddr_in)) == SOCKET_ERROR) throw GETSOCKETERRNO();
    }

    void bind(std::string addr) {
        auto tmpaddr = split(addr, ':');

        bind(tmpaddr[0], std::stoi(tmpaddr[1]));
    }

    std::string gethostbyname(std::string name) {
        hostent* remoteHost;
        in_addr addr;

        remoteHost = ::gethostbyname(name.c_str());

        addr.s_addr = *(uint64_t*)remoteHost->h_addr_list[0];
        return std::string(inet_ntoa(addr));
    }

    sockaddress_t getsockname() {
        sockaddr_in my_addr;
        socklen_t addrlen = sizeof(sockaddr_in);

        if (::getsockname(s, (sockaddr*)&my_addr, &addrlen) == SOCKET_ERROR) throw GETSOCKETERRNO();
        return sockaddr_in_to_sockaddress_t(my_addr);
    }

    void setsockopt(int level, int optname, void* optval, int size) {
#ifdef _WIN32
        if (::setsockopt(s, level, optname, (char*)optval, size) == SOCKET_ERROR) throw GETSOCKETERRNO();
#elif __linux__
        if (::setsockopt(s, level, optname, optval, size) == SOCKET_ERROR) throw GETSOCKETERRNO();
#endif
    }

    void setsockopt(int level, int optname, int optval) {
        setsockopt(level, optname, &optval, sizeof(int));
    }

    int getsockopt(int level, int optname) {
        int optval;
        socklen_t error_code_size = sizeof(int);
#ifdef _WIN32
        ::getsockopt(s, level, optname, (char*)&optval, &error_code_size);
#elif __linux__
        ::getsockopt(s, level, optname, &optval, &error_code_size);
#endif
        return optval;
    }

    void listen(int clients) {
        if (::listen(s, clients) == SOCKET_ERROR) throw GETSOCKETERRNO();
    }

    void setrecvtimeout(int seconds) {
        timeval timeout;
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;
        setsockopt(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval));
    }

    void setreuseaddr(int i) {
        setsockopt(SOL_SOCKET, SO_REUSEADDR, i);
    }

    void settcpnodelay(int i) {
        setsockopt(IPPROTO_TCP, TCP_NODELAY, &i, sizeof(int));
    }

    std::pair<Socket, sockaddress_t> accept() {
        sockaddr_in client;
        socklen_t c = sizeof(sockaddr_in);

        auto new_socket = ::accept(s, (sockaddr*)&client, (socklen_t*)&c);

        if (new_socket == INVALID_SOCKET) throw GETSOCKETERRNO();

        return { Socket(new_socket, this->sock_type, this->sock_af, sockaddr_in_to_sockaddress_t(client)), sockaddr_in_to_sockaddress_t(client) };
    }

    

    int64_t send(const void* data, int64_t size) {
#ifdef _WIN32
        return ::send(s, (const char*)data, size, 0);
#elif __linux__
        return ::send(s, data, size, MSG_NOSIGNAL);
#endif
    }

    int64_t send(std::string data) {
        return send(data.c_str(), data.size());
    }

    int64_t send(char ch) {
        char chbuf[1];
        chbuf[0] = ch;

        return send(chbuf, 1);
    }

    int64_t sendall(const void* chardata, int64_t size) {
        int64_t ptr = 0;

        while (ptr < size) ptr += send(((char*)chardata) + ptr, size - ptr);
        
        return ptr;
    }

    int64_t sendall(std::string data) {
        return sendall(data.c_str(), data.size());
    }

    int64_t sendmsg(const void* data, uint32_t size) {
        msgSendMtx->lock();

        uint32_t netsize = htonl(size);

        sendall(&netsize, sizeof(uint32_t));
        int64_t retsize = sendall(data, size);

        msgSendMtx->unlock();
        return retsize;
    }

    int64_t sendmsg(std::string data) {
        return sendmsg(data.c_str(), data.size());
    }

    int64_t send_file(std::ifstream& file) {
        char* buffer = new char[256 * 1024];
        int64_t size = 0;

        while (file.tellg() != -1) {
            file.read(buffer, 256 * 1024);
            size += send(buffer, file.gcount());
        }

        delete[] buffer;
        return size;
    }

    int64_t sendto(char* buf, int64_t size, std::string ipaddr, uint16_t port) {
        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        return ::sendto(s, buf, size, MSG_CONFIRM, (sockaddr*)&sock, sizeof(sockaddr_in));
    }

    int64_t sendto(char* buf, int64_t size, sockaddress_t addr) { return sendto(buf, size, addr.ip, addr.port); }

    int64_t sendto(std::string buf, std::string ipaddr, uint16_t port) { return sendto((char*)buf.c_str(), buf.size(), ipaddr, port); }

    int64_t sendto(std::string buf, sockaddress_t addr) { return sendto((char*)buf.c_str(), buf.size(), addr.ip, addr.port); }

    int64_t sendto(sockrecv_t data) { return sendto((char*)data.buffer, data.size, data.addr.ip, data.addr.port); }

    sockrecv_t recv(int64_t size) {
        char* buffer = new char[size];
        std::memset(buffer, 0, size);

        int preverrno = errno;

        sockrecv_t data;
        if ((data.size = ::recv(s, buffer, size, 0)) < 0) {
            errno = preverrno;

            delete[] buffer;
            return {};
        }

        data.buffer = new char[data.size];
        std::memcpy(data.buffer, buffer, data.size);

        data.string.assign(data.buffer, data.size);
        data.addr = address;

        delete[] buffer;
        return data;
    }

    sockrecv_t recvall(int64_t size) {
        sockrecv_t ret;
        ret.buffer = new char[size];

        std::memset(ret.buffer, 0, size);

        int64_t bufptr = 0;

        while (bufptr < size) {
            sockrecv_t part = recv(size - bufptr);

            std::memcpy(ret.buffer + bufptr, part.buffer, part.size);
            bufptr += part.size;
        }

        ret.addr = address;
        ret.size = bufptr;
        ret.string = std::string(ret.buffer, bufptr);

        return ret;
    }

    sockrecv_t recvmsg() {
        msgRecvMtx->lock();

        sockrecv_t recvsize = recv(sizeof(uint32_t));

        if (!recvsize.size) {
            msgRecvMtx->unlock();
            return {};
        }

        sockrecv_t ret = recvall(ntohl(*(uint32_t*)recvsize.buffer));

        msgRecvMtx->unlock();

        return ret;
    }

    char recvbyte() {
        sockrecv_t recvbyte = recv(1);

        return (recvbyte.size) ? recvbyte.buffer[0] : 0;
    }

    sockrecv_t recvfrom(int64_t size) {
        sockaddr_in sock;
        socklen_t len = sizeof(sockaddr_in);

        if (size > 65536) {
            std::cout << "Warning: srecvfrom max value 65536" << std::endl;
            size = 65536;
        }

        char* buffer = new char[size];
        std::memset(buffer, 0, size);

        int preverrno = errno;

        sockrecv_t data;

        if ((data.size = ::recvfrom(s, buffer, size, MSG_WAITALL, (sockaddr*)&sock, &len)) < 0 || errno == 104) {
            errno = preverrno;

            delete[] buffer;
            return {};
        }

        data.buffer = new char[data.size];

        std::memcpy(data.buffer, buffer, data.size);
        data.string.assign(buffer, data.size);
        data.addr = sockaddr_in_to_sockaddress_t(sock);

        delete[] buffer;
        return data;
    }

    size_t recvAvailable() const {
        size_t bytes_available;
#ifdef _WIN32
        ioctlsocket(s, FIONREAD, &bytes_available);
#else
        ioctl(s, FIONREAD, &bytes_available);
#endif
        return bytes_available;
    }

    const sockaddress_t remoteAddress() const {
        return address;
    }

#ifdef _WIN32
    const SOCKET fd() const {
        return s;
    }
#else
    const int fd() const {
        return s;
    }
#endif

    void close() {
#ifdef _WIN32
        ::shutdown(s, SD_BOTH);
        ::closesocket(s);
        ::WSACleanup();
#elif __linux__
        ::shutdown(s, SHUT_RDWR);
        ::close(s);
#endif
    }
};

struct sockevent_t {
    Socket sock;
    int events = 0;
};

class Poll {
    std::vector<pollfd> fds;
    std::map<int, Socket> usingSockets;

    pollfd sock_to_pollfd(Socket sock, short events) {
        return { sock.fd(), events, 0 };
    }
public:
    void addSocket(Socket sock, short events) {
        fds.push_back(sock_to_pollfd(sock, events));
        usingSockets[sock.fd()] = sock;
    }

    void removeSocket(Socket sock) {
        for (int i = 0; i < fds.size(); i++) {
            int fd = fds[i].fd;

            if (fd == sock.fd()) {
                fds.erase(fds.begin() + i);
                break;
            }
        }

        usingSockets.erase(sock.fd());
    }

    std::vector<sockevent_t> poll(int timeout = -1) {
        int nevents = ::poll(fds.data(), fds.size(), timeout);

        std::vector<sockevent_t> ret;

        for (pollfd& i : fds) {
            Socket sock = usingSockets[i.fd];

            if (i.revents & i.events) {
                ret.push_back(sockevent_t({sock, i.revents}));
                i.revents = 0;
            }
        }

        return ret;
    }
};

bool operator==(sockaddress_t arg1, sockaddress_t arg2) { return arg1.ip == arg2.ip && arg1.port == arg2.port; }
bool operator==(sockrecv_t arg1, sockrecv_t arg2) { return arg1.addr == arg2.addr && arg1.string == arg2.string && arg1.size == arg2.size; }
bool operator==(Socket arg1, Socket arg2) { return arg1.s == arg2.s; }

bool operator!=(sockaddress_t arg1, sockaddress_t arg2) { return !(arg1 == arg2); }
bool operator!=(sockrecv_t arg1, sockrecv_t arg2) { return !(arg1 == arg2); }
bool operator!=(Socket arg1, Socket arg2) { return !(arg1 == arg2); }