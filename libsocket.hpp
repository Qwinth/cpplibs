// version 2.5.4-c5
#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <utility>
#include <cstring>
#include <cstdint>
#include <memory>
#include <map>
#include <atomic>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "windowsHeader.hpp"

#ifndef _WINSOCKAPI_
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define GETSOCKETERRNO() (WSAGetLastError())
#define GETSOCKETERRNOMSG() ("WSAError: " + std::to_string(GETSOCKETERRNO()))
#define MSG_CONFIRM 0
#define _MSG_WAITALL 0

// typedef long int64_t;
typedef int socklen_t;
#endif

#elif __linux__
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define _MSG_WAITALL MSG_WAITALL
#define GETSOCKETERRNO() (errno)
#define GETSOCKETERRNOMSG() (strerror(GETSOCKETERRNO()))
#endif

#include "libstrmanip.hpp"
#include "libuuid4.hpp"
#include "libmsgpacket.hpp"
#include "libfd.hpp"
#include "libbytesarray.hpp"
#include "libpoll.hpp"

struct SocketAddress {
    std::string ip;
    uint16_t port = 0;

    std::string str() const { return strformat("%s:%d", ip.c_str(), port); }
};

struct SocketTable {
    std::atomic_bool working;
    std::atomic_bool opened;
    std::atomic_bool blocking;

    std::string param_id;

    int sock_af;
    int sock_type;

    SocketAddress laddress;
    SocketAddress raddress;

    std::mutex recvMtx;

    std::atomic_int n_links;
};

std::map<FileDescriptor, SocketTable> socket_table;

struct SocketData {
    BytesArray buffer;
    SocketAddress addr;
};

class Socket : public FileDescriptor {
    std::string param_id;

    bool is_IPv4(std::string ipaddr) const {
        replaceAll(ipaddr, ".", "");
        return std::all_of(ipaddr.begin(), ipaddr.end(), ::isdigit);
    }

    sockaddr_in make_sockaddr_in(std::string ipaddr, uint16_t port) const {
        sockaddr_in tmpaddr;

        if (ipaddr.size()) {
            if (!is_IPv4(ipaddr)) ipaddr = gethostbyname(ipaddr);

            tmpaddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
        }

        else tmpaddr.sin_addr.s_addr = htonl(INADDR_ANY);        

        tmpaddr.sin_family = getsockfamily();
        tmpaddr.sin_port = htons(port);

        return tmpaddr;
    }

    SocketAddress sockaddr_in_to_SocketAddress(sockaddr_in addr) const { return { inet_ntoa(addr.sin_addr), ntohs(addr.sin_port) }; }

public:
    Socket() {}
    Socket(int _af, int _type) { open(_af, _type); }

    Socket(FileDescriptor fd) {
        if (socket_table.find(fd) == socket_table.end()) throw std::runtime_error("Socket(int fd): Non-socket or closed fd" );

        desc = fd;
        param_id = socket_table.at(desc).param_id;

        socket_table.at(desc).n_links++;

        // std::cout << "New container by constructor(FD) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;
    }

    Socket(const Socket& obj) {
        desc = obj.desc;
        param_id = socket_table.at(desc).param_id;

        socket_table.at(desc).n_links++;

        // std::cout << "New container by constructor(COPY) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;
    }

    Socket(Socket&& obj) {
        std::swap(desc, obj.desc);
        param_id = socket_table.at(desc).param_id;

        // std::cout << "Move container by constructor(MOVE) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;
    }

    ~Socket() {
        if (socket_table.find(desc) == socket_table.end() || socket_table.at(desc).param_id != param_id) return;

        socket_table.at(desc).n_links--;

        // std::cout << "Deleting container: " << this << ", Links: " << socket_table.at(desc).n_links << ", fd: " << desc << std::endl;

        if (!socket_table.at(desc).n_links && !socket_table.at(desc).opened) {
            socket_table.erase(desc);

            // std::cout << "No containers for fd: " << desc << "! Removing socket param table for fd: " << desc << "." << std::endl;
            // std::cout << "Socket param table size: " << socket_table.size() << std::endl;         
        }
    }

    Socket& operator=(FileDescriptor fd) {
        desc = fd;
        param_id = socket_table.at(desc).param_id;

        socket_table.at(desc).n_links++;

        // std::cout << "New container by operator(FD) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;

        return *this;
    }

    Socket& operator=(const Socket& obj) {
        desc = obj.desc;
        param_id = socket_table.at(desc).param_id;

        socket_table.at(desc).n_links++;

        // std::cout << "New container by operator(COPY) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;

        return *this;
    }

    Socket& operator=(Socket&& obj) {
        std::swap(desc, obj.desc);
        param_id = socket_table.at(desc).param_id;

        // std::cout << "Move container by operator(MOVE) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;

        return *this;
    }

    void setBlocking(bool _blocking) {
    #ifdef __linux__
        if (_blocking) fcntl(desc, F_SETFL, fcntl(desc, F_GETFL) & ~O_NONBLOCK);
        else fcntl(desc, F_SETFL, fcntl(desc, F_GETFL) | O_NONBLOCK);
    #elif _WIN32
        unsigned long __blocking = !_blocking;
        ioctlsocket(desc, FIONBIO, &__blocking);
    #endif

        socket_table.at(desc).blocking = _blocking;
    }

    bool is_blocking() const {
#ifdef __linux__
        return !(fcntl(desc, F_GETFL) & O_NONBLOCK);
#elif _WIN32
        return socket_table.at(desc).blocking;
#endif
    }

    void open(int _af = AF_INET, int _type = SOCK_STREAM) {
#ifdef _WIN32
        WSAData wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        if ((desc = ::socket(_af, _type, 0)) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());

        param_id = uuid4();

        socket_table.try_emplace(desc);
        socket_table.at(desc).opened = true;
        socket_table.at(desc).working = true;
        socket_table.at(desc).param_id = param_id;
        socket_table.at(desc).sock_af = _af;
        socket_table.at(desc).sock_type = _type;
        socket_table.at(desc).n_links = 1;
        // std::cout << "New container by constructor(OPEN) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;
    }

    bool is_opened() const {
        return socket_table.find(desc) != socket_table.end() && socket_table.at(desc).opened;
    }

    bool is_working() const {
        return socket_table.find(desc) != socket_table.end() && socket_table.at(desc).working;
    }

    void connect(std::string ipaddr, uint16_t port) {
        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::connect(desc, (sockaddr*)&sock, sizeof(sockaddr_in)) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());

        socket_table.at(desc).laddress = getsockname();
        socket_table.at(desc).raddress = sockaddr_in_to_SocketAddress(sock);
    }

    void connect(std::string addr) {
        auto tmpaddr = split(addr, ':');

        connect(tmpaddr[0], std::stoi(tmpaddr[1]));
    }

    void bind(std::string ipaddr, uint16_t port) {
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::bind(desc, (sockaddr*)&sock, sizeof(sockaddr_in)) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());

        socket_table.at(desc).laddress = sockaddr_in_to_SocketAddress(sock);
    }

    void bind(std::string addr) {
        auto tmpaddr = split(addr, ':');

        bind(tmpaddr[0], std::stoi(tmpaddr[1]));
    }

    std::string gethostbyname(std::string name) const {
        hostent* remoteHost;
        in_addr addr;

        remoteHost = ::gethostbyname(name.c_str());

        addr.s_addr = *(uint64_t*)remoteHost->h_addr_list[0];
        return std::string(inet_ntoa(addr));
    }

    SocketAddress getsockname() const {
        sockaddr_in my_addr;
        socklen_t addrlen = sizeof(sockaddr_in);

        if (::getsockname(desc, (sockaddr*)&my_addr, &addrlen) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());
        return sockaddr_in_to_SocketAddress(my_addr);
    }

    SocketAddress getpeername() const {
        sockaddr_in my_addr;
        socklen_t addrlen = sizeof(sockaddr_in);

        if (::getpeername(desc, (sockaddr*)&my_addr, &addrlen) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());
        return sockaddr_in_to_SocketAddress(my_addr);
    }

    template<typename T>
        void setsockopt(int level, int optname, T& optval) {
    #ifdef _WIN32
            if (::setsockopt(desc, level, optname, (char*)&optval, sizeof(T)) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());
    #elif __linux__
            if (::setsockopt(desc, level, optname, &optval, sizeof(T)) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());
    #endif
        }

        template<typename T>
        int getsockopt(int level, int optname, T& optval, socklen_t size = sizeof(T)) const {
    #ifdef _WIN32
            if (::getsockopt(desc, level, optname, (char*)&optval, &size) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());
    #elif __linux__
            if (::getsockopt(desc, level, optname, &optval, &size) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());
    #endif
            return size;
        }

    int getsocktype() {
        int type;
        getsockopt(SOL_SOCKET, SO_TYPE, type);

        return type;
    }
#ifdef _WIN32
    int getsockfamily() const {
        WSAPROTOCOL_INFO proto;
        WSADuplicateSocket(desc, GetCurrentProcessId(), &proto);

        return proto.iAddressFamily;
    }
#elif __linux__
    uint16_t getsockfamily() const {
        sockaddr af;
        getsockopt(SOL_SOCKET, SO_DOMAIN, af);

        return af.sa_family;
    }
#endif

    void listen(int clients = 0) {
        if (::listen(desc, clients) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());
    }

    void setrecvtimeout(int64_t millis) {
        timeval timeout;
        timeout.tv_sec = millis / 1000;
        timeout.tv_usec = (millis % 1000) * 1000;
        setsockopt(SOL_SOCKET, SO_RCVTIMEO, timeout);
    }

    void setsendtimeout(int64_t millis) {
        timeval timeout;
        timeout.tv_sec = millis / 1000;
        timeout.tv_usec = (millis % 1000) * 1000;
        setsockopt(SOL_SOCKET, SO_SNDTIMEO, timeout);
    }

    void setreuseaddr(int i) {
        setsockopt(SOL_SOCKET, SO_REUSEADDR, i);
    }

    void settcpnodelay(int i) {
        setsockopt(IPPROTO_TCP, TCP_NODELAY, i);
    }

    Socket accept() {
        sockaddr_in client;
        socklen_t c = sizeof(sockaddr_in);

        FileDescriptor new_socket = ::accept(desc, (sockaddr*)&client, (socklen_t*)&c);

        if (new_socket == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());

        socket_table.try_emplace(new_socket);
        socket_table.at(new_socket).opened = true;
        socket_table.at(new_socket).working = true;
        socket_table.at(new_socket).param_id = uuid4();
        socket_table.at(new_socket).sock_af = socket_table.at(desc).sock_af;
        socket_table.at(new_socket).sock_type = socket_table.at(desc).sock_type;
        socket_table.at(new_socket).raddress = sockaddr_in_to_SocketAddress(client);
        socket_table.at(new_socket).laddress = socket_table.at(desc).laddress;
        socket_table.at(new_socket).n_links = 0;

        return new_socket;
    }

    int64_t send(const void* data, int64_t size) {
#ifdef _WIN32
        return ::send(desc, (const char*)data, size, 0);
#elif __linux__
        return ::send(desc, data, size, MSG_NOSIGNAL);
#endif
    }

    int64_t send(std::string data) {
        return send(data.c_str(), data.size());
    }

    int64_t send(MsgPacket data) {
        return send(data.c_str(), data.size());
    }

    int64_t send(BytesArray data) {
        return send(data.c_str(), data.size());
    }

    int64_t send(char ch) {
        return send(&ch, 1);
    }

    int64_t sendall(const void* data, int64_t size) {
        int64_t ptr = 0;
        int preverrno = errno;

        while (ptr < size) {
            int64_t n = send(((char*)data) + ptr, size - ptr);

            if (n < 0) {
                errno = preverrno;
                shutdown();

                break;
            }

            ptr += n;
        }     
        
        return ptr;
    }

    int64_t sendall(std::string data) {
        return sendall(data.c_str(), data.size());
    }

    int64_t sendall(MsgPacket data) {
        return sendall(data.c_str(), data.size());
    }

    int64_t sendall(BytesArray data) {
        return sendall(data.c_str(), data.size());
    }

    int64_t sendmsg(const void* data, uint32_t size) {
        MsgPacket packet;

        packet.write(htonl(size));
        packet.write(data, size);

        int64_t retsize = sendall(packet);

        packet.clear();

        return retsize;
    }

    int64_t sendmsg(std::string data) {
        return sendmsg(data.c_str(), data.size());
    }

    int64_t sendmsg(MsgPacket data) {
        return sendmsg(data.c_str(), data.size());
    }

    int64_t sendmsg(BytesArray data) {
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

    int64_t sendto(const char* buf, int64_t size, std::string ipaddr, uint16_t port) {
        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        return ::sendto(desc, buf, size, MSG_CONFIRM, (sockaddr*)&sock, sizeof(sockaddr_in));
    }

    int64_t sendto(char* buf, int64_t size, SocketAddress addr) { return sendto(buf, size, addr.ip, addr.port); }

    int64_t sendto(std::string buf, std::string ipaddr, uint16_t port) { return sendto(buf.c_str(), buf.size(), ipaddr, port); }
    
    int64_t sendto(MsgPacket buf, std::string ipaddr, uint16_t port) { return sendto(buf.c_str(), buf.size(), ipaddr, port); }

    int64_t sendto(std::string buf, SocketAddress addr) { return sendto(buf.c_str(), buf.size(), addr.ip, addr.port); }

    int64_t sendto(SocketData data) { return sendto(data.buffer.c_str(), data.buffer.size(), data.addr.ip, data.addr.port); }

    SocketData recv(int64_t size) {
        char* buffer = new char[size];

        int preverrno = errno;

        int64_t rsize = ::recv(desc, buffer, size, 0);

        if (rsize < 0 || (!rsize && is_blocking())) {
            errno = preverrno;
            rsize = 0;

            shutdown();
            // std::cout << "Call close() from recv() due to error or closed socket." << std::endl;
        }

        SocketData data;
        data.buffer.set(buffer, rsize);
        data.addr = socket_table.at(desc).raddress;

        delete[] buffer;
        return data;
    }

    SocketData recvall(int64_t size, bool mtx_bypass = false) {
        SocketData ret;
        
        if (!mtx_bypass) socket_table.at(desc).recvMtx.lock();

        do ret.buffer.append(recv(size - ret.buffer.size()).buffer);
        while (ret.buffer.size() < size && is_working());

        if (!mtx_bypass) socket_table.at(desc).recvMtx.unlock();

        ret.addr = socket_table.at(desc).raddress;

        return ret;
    }

    SocketData recvmsg() {
        socket_table.at(desc).recvMtx.lock();

        SocketData recvsize = recvall(sizeof(uint32_t), true);

        if (!recvsize.buffer.size()) {
            socket_table.at(desc).recvMtx.unlock();
            return {};
        }

        SocketData ret = recvall(ntohl(*(uint32_t*)recvsize.buffer.c_str()), true);

        socket_table.at(desc).recvMtx.unlock();

        return ret;
    }

    uint8_t recvbyte() {
        SocketData recvbyte = recv(1);

        return recvbyte.buffer.front();
    }

    SocketData recvfrom(int64_t size) {
        sockaddr_in sock;
        socklen_t len = sizeof(sockaddr_in);

        if (size > 65536) {
            // std::cout << "Warning: srecvfrom max value 65536" << std::endl;
            size = 65536;
        }

        char* buffer = new char[size];

        int preverrno = errno;
        int64_t rsize = 0;

        if ((rsize = ::recvfrom(desc, buffer, size, _MSG_WAITALL, (sockaddr*)&sock, &len)) < 0 || errno == 104) {
            errno = preverrno;

            delete[] buffer;
            return {};
        }

        SocketData data;
        data.buffer.set(buffer, rsize);
        data.addr = sockaddr_in_to_SocketAddress(sock);

        delete[] buffer;
        return data;
    }

    uint32_t tcpRecvAvailable() const {
        uint32_t bytes_available;
#ifdef _WIN32
        ioctlsocket(desc, FIONREAD, (unsigned long*)&bytes_available);
#else
        ioctl(desc, FIONREAD, &bytes_available);
#endif
        return bytes_available;
    }

    const SocketAddress localSocketAddress() const {
        return socket_table.at(desc).laddress;
    }

    const SocketAddress remoteSocketAddress() const {
        return socket_table.at(desc).raddress;
    }
    
    void shutdown() {
#ifdef _WIN32
        ::shutdown(desc, SD_BOTH);
#elif __linux__
        ::shutdown(desc, SHUT_RDWR);
#endif

        socket_table.at(desc).working = false;
    }

    void close() {
        // std::cout << "Closing fd: " << desc << std::endl;
        if (is_working()) shutdown();
        
#ifdef _WIN32
        ::closesocket(desc);
        ::WSACleanup();
#elif __linux__
        ::close(desc);
#endif

        socket_table.at(desc).opened = false;
    }
};

bool operator==(SocketAddress& arg1, SocketAddress& arg2) { return arg1.ip == arg2.ip && arg1.port == arg2.port; }
bool operator==(SocketData& arg1, SocketData& arg2) { return arg1.addr == arg2.addr && arg1.buffer == arg2.buffer; }
bool operator==(Socket& arg1, Socket& arg2) { return arg1.fd() == arg2.fd(); }
bool operator==(int fd, Socket& arg2) { return fd == arg2.fd(); }

bool operator!=(SocketAddress& arg1, SocketAddress& arg2) { return !(arg1 == arg2); }
bool operator!=(SocketData& arg1, SocketData& arg2) { return !(arg1 == arg2); }
bool operator!=(Socket& arg1, Socket& arg2) { return !(arg1 == arg2); }