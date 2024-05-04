// version 1.9.7-c1
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


typedef long ssize_t;
typedef int socklen_t;

#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
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
    int port = 0;

    std::string str() { return strformat("%s:%d", ip.c_str(), port); }
};

struct sockrecv_t {
    std::string string;
    char* buffer = nullptr;
    ssize_t size = 0;
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

    std::mutex msgSendMtx;
    std::mutex msgRecvMtx;

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

    sockaddr_in make_sockaddr_in(std::string ipaddr, int port) {
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

    sockaddress_t sockaddr_in_to_sockaddress_t(sockaddr_in addr) { return { inet_ntoa(addr.sin_addr), ntohs(addr.sin_port) }; }
        friend bool operator==(Socket arg1, Socket arg2);
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
    }

    void baseServer(std::string ipaddr, int port, int _listen = 0, bool reuseaddr = false) {
        if (reuseaddr) setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
        bind(ipaddr, port);
        listen(_listen);
    }

    void connect(std::string ipaddr, int port) {
        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::connect(s, (sockaddr*)&sock, sizeof(sockaddr_in)) == SOCKET_ERROR) throw GETSOCKETERRNO();
    }

    void bind(std::string ipaddr, int port) {
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::bind(s, (sockaddr*)&sock, sizeof(sockaddr_in)) == SOCKET_ERROR) throw GETSOCKETERRNO();
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

    std::pair<Socket, sockaddress_t> accept() {
        sockaddr_in client;
        socklen_t c = sizeof(sockaddr_in);

        auto new_socket = ::accept(s, (sockaddr*)&client, (socklen_t*)&c);

        if (new_socket == INVALID_SOCKET) throw GETSOCKETERRNO();

        return { Socket(new_socket, this->sock_type, this->sock_af, sockaddr_in_to_sockaddress_t(client)), sockaddr_in_to_sockaddress_t(client) };
    }

     size_t send(const void* data, size_t size) {
#ifdef _WIN32
        return ::send(s, (const char*)data, size, 0);
#elif __linux__
        return ::send(s, data, size, MSG_NOSIGNAL);
#endif
    }

    size_t send(std::string data) {
        return send(data.c_str(), data.size());
    }

    size_t send(char ch) {
        char chbuf[1];
        chbuf[0] = ch;

        return send(chbuf, 1);
    }

    size_t sendall(const void* chardata, size_t size) {
        size_t sended = 0;
        size_t ptr = 0;

        while (sended < size) {
            size_t retszie = send(&((char*)chardata)[ptr], size - sended);
            sended += retszie;
            ptr += retszie;
        }

        return sended;
    }

    size_t sendall(std::string data) {
        return sendall(data.c_str(), data.size());
    }

    size_t sendmsg(const void* data, uint64_t size) {
        msgSendMtx.lock();

        sendall(&size, sizeof(uint64_t));
        size_t retsize = sendall(data, size);

        msgSendMtx.unlock();
        return retsize;
    }

    size_t sendmsg(std::string data) {
        return sendmsg(data.c_str(), data.size());
    }

    size_t send_file(std::ifstream& file) {
        char* buffer = new char[65536];
        size_t size = 0;

        while (file.tellg() != -1) {
            std::memset(buffer, 0, 65536);

            file.read(buffer, 65536);
            size += send(buffer, file.gcount());
        }

        delete[] buffer;
        return size;
    }

    size_t sendto(char* buf, size_t size, std::string ipaddr, int port) {
        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        return ::sendto(s, buf, size, MSG_CONFIRM, (sockaddr*)&sock, sizeof(sockaddr_in));
    }

    size_t sendto(char* buf, size_t size, sockaddress_t addr) { return sendto(buf, size, addr.ip, addr.port); }

    size_t sendto(std::string buf, std::string ipaddr, int port) { return sendto((char*)buf.c_str(), buf.size(), ipaddr, port); }

    size_t sendto(std::string buf, sockaddress_t addr) { return sendto((char*)buf.c_str(), buf.size(), addr.ip, addr.port); }

    size_t sendto(sockrecv_t data) { return sendto((char*)data.buffer, data.size, data.addr.ip, data.addr.port); }

    sockrecv_t recv(size_t size) {
        char* buffer = new char[size];
        std::memset(buffer, 0, size);

        int preverrno = errno;

        sockrecv_t data;
        if ((data.size = ::recv(s, buffer, size, 0)) < 0 || errno == 104) {
            errno = preverrno;

            delete[] buffer;
            return {};
        }

        data.buffer = new char[data.size];

        std::memcpy(data.buffer, buffer, data.size);
        data.string.assign(buffer, data.size);
        data.addr = address;

        delete[] buffer;
        return data;
    }

    sockrecv_t recvmsg() {
        msgRecvMtx.lock();

        sockrecv_t recvsize = recv(sizeof(uint64_t));
        if (!recvsize.size) return {};

        sockrecv_t ret;
        ret.size = *(uint64_t*)recvsize.buffer;
        ret.buffer = new char[ret.size];

        memset(ret.buffer, 0, ret.size);

        uint64_t received = 0;
        uint64_t bufptr = 0;

        while (received < (uint64_t)ret.size) {
            sockrecv_t part = recv(ret.size - received);

            memcpy(&ret.buffer[bufptr], part.buffer, part.size);
            bufptr += part.size;
            received += part.size;

            ret.addr = part.addr;
        }

        ret.string = std::string(ret.buffer, ret.size);

        msgRecvMtx.unlock();
        return ret;
    }

    char recvbyte() {
        sockrecv_t recvbyte = recv(1);

        return (recvbyte.size) ? recvbyte.buffer[0] : 0;
    }

    sockrecv_t recvfrom(size_t size) {
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

    sockaddress_t remoteAddress() {
        return address;
    }

#ifdef _WIN32
    SOCKET fd() {
        return s;
    }
#else
    int fd() {
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

bool operator==(sockaddress_t arg1, sockaddress_t arg2) { return arg1.ip == arg2.ip && arg1.port == arg2.port; }
bool operator==(sockrecv_t arg1, sockrecv_t arg2) { return arg1.addr == arg2.addr && arg1.string == arg2.string && arg1.size == arg2.size; }
bool operator==(Socket arg1, Socket arg2) { return arg1.s == arg2.s; }

bool operator!=(sockaddress_t arg1, sockaddress_t arg2) { return !(arg1 == arg2); }
bool operator!=(sockrecv_t arg1, sockrecv_t arg2) { return !(arg1 == arg2); }
bool operator!=(Socket arg1, Socket arg2) { return !(arg1 == arg2); }
