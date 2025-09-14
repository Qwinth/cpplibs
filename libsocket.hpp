// version 2.5.5
#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
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

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
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

struct SocketParamTable {
    std::string param_table_id;

    std::atomic_bool working;
    std::atomic_bool opened;
    std::atomic_bool blocking;

    int sock_af;
    int sock_type;

    SocketAddress laddress;
    SocketAddress raddress;

    std::recursive_mutex recvMtx;
    std::recursive_mutex sendMtx;

    std::atomic_int n_links;
};

std::recursive_mutex socket_table_mtx;
std::map<FileDescriptor, SocketParamTable> socket_table;

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
        std::lock_guard lck(socket_table_mtx);

        if (socket_table.find(fd) == socket_table.end()) throw std::runtime_error("Socket(FileDescriptor): Non-socket or closed fd");

        desc = fd;
        param_id = socket_table.at(desc).param_table_id;

        socket_table.at(desc).n_links++;

        // std::cout << "New container by constructor(FD) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;
    }

    Socket(const Socket& obj) {
        std::lock_guard lck(socket_table_mtx);

        desc = obj.desc;
        param_id = socket_table.at(desc).param_table_id;

        socket_table.at(desc).n_links++;

        // std::cout << "New container by constructor(COPY) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;
    }

    Socket(Socket&& obj) {
        std::lock_guard lck(socket_table_mtx);

        std::swap(desc, obj.desc);
        param_id = socket_table.at(desc).param_table_id;

        // std::cout << "Move container by constructor(MOVE) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;
    }

    ~Socket() {
        std::lock_guard lck(socket_table_mtx);

        if (socket_table.find(desc) == socket_table.end() || socket_table.at(desc).param_table_id != param_id) return;

        socket_table.at(desc).n_links--;

        // std::cout << "Deleting container: " << this << ", Links: " << socket_table.at(desc).n_links << ", fd: " << desc << std::endl;

        if (!socket_table.at(desc).n_links && !socket_table.at(desc).opened) {
            socket_table.erase(desc);

            // std::cout << "No containers for fd: " << desc << "! Removing socket param table for fd: " << desc << "." << std::endl;
            // std::cout << "Socket param table size: " << socket_table.size() << std::endl;         
        }
    }

    Socket& operator=(FileDescriptor fd) {
        std::lock_guard lck(socket_table_mtx);

        desc = fd;
        param_id = socket_table.at(desc).param_table_id;

        socket_table.at(desc).n_links++;

        // std::cout << "New container by operator(FD) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;

        return *this;
    }

    Socket& operator=(const Socket& obj) {
        std::lock_guard lck(socket_table_mtx);

        desc = obj.desc;
        param_id = socket_table.at(desc).param_table_id;

        socket_table.at(desc).n_links++;

        // std::cout << "New container by operator(COPY) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;

        return *this;
    }

    Socket& operator=(Socket&& obj) {
        std::lock_guard lck(socket_table_mtx);

        std::swap(desc, obj.desc);
        param_id = socket_table.at(desc).param_table_id;

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
        std::lock_guard lck(socket_table_mtx);

        socket_table.at(desc).blocking = _blocking;
    }

    bool isBlocking() const {
#ifdef __linux__
        return !(fcntl(desc, F_GETFL) & O_NONBLOCK);
#elif _WIN32
        std::lock_guard lck(socket_table_mtx);

        return socket_table.at(desc).isBlocking();
#endif
    }

    void open(int _af = AF_INET, int _type = SOCK_STREAM) {
        std::lock_guard lck(socket_table_mtx);

#ifdef _WIN32
        WSAData wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        if ((desc = ::socket(_af, _type, 0)) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());

        param_id = uuid4();
        
        socket_table.try_emplace(desc);
        socket_table.at(desc).param_table_id = param_id;
        socket_table.at(desc).opened = true;
        socket_table.at(desc).working = true;
        socket_table.at(desc).sock_af = _af;
        socket_table.at(desc).sock_type = _type;
        socket_table.at(desc).n_links = 1;
        // std::cout << "New container by constructor(OPEN) for fd: " << desc << ", Links: " << socket_table.at(desc).n_links << ", container: " << this << std::endl;
    }

    bool isOpened() const {
        std::lock_guard lck(socket_table_mtx);

        return socket_table.find(desc) != socket_table.end() && socket_table.at(desc).opened;
    }

    bool isWorking() const {
        std::lock_guard lck(socket_table_mtx);

        return socket_table.find(desc) != socket_table.end() && socket_table.at(desc).working;
    }

    void connect(std::string ipaddr, uint16_t port) {
        std::lock_guard lck(socket_table_mtx);

        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::connect(desc, reinterpret_cast<sockaddr*>(&sock), sizeof(sockaddr_in)) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());

        socket_table.at(desc).laddress = getsockname();
        socket_table.at(desc).raddress = sockaddr_in_to_SocketAddress(sock);
    }

    void connect(std::string addr) {
        auto tmpaddr = split(addr, ':');

        connect(tmpaddr[0], std::stoi(tmpaddr[1]));
    }

    void bind(std::string ipaddr, uint16_t port) {
        std::lock_guard lck(socket_table_mtx);

        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::bind(desc, reinterpret_cast<sockaddr*>(&sock), sizeof(sockaddr_in)) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());

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

        addr.s_addr = *reinterpret_cast<uint32_t*>(remoteHost->h_addr_list[0]);
        return std::string(inet_ntoa(addr));
    }

    SocketAddress getsockname() const {
        sockaddr_in my_addr;
        socklen_t addrlen = sizeof(sockaddr_in);

        if (::getsockname(desc, reinterpret_cast<sockaddr*>(&my_addr), &addrlen) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());
        return sockaddr_in_to_SocketAddress(my_addr);
    }

    SocketAddress getpeername() const {
        sockaddr_in my_addr;
        socklen_t addrlen = sizeof(sockaddr_in);

        if (::getpeername(desc, reinterpret_cast<sockaddr*>(&my_addr), &addrlen) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());
        return sockaddr_in_to_SocketAddress(my_addr);
    }

    template<typename T>
        void setsockopt(int level, int optname, T optval) {
    #ifdef _WIN32
            if (::setsockopt(desc, level, optname, reinterpret_cast<char*>(&optval), sizeof(T)) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());
    #elif __linux__
            if (::setsockopt(desc, level, optname, &optval, sizeof(T)) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());
    #endif
        }

        template<typename T>
        int getsockopt(int level, int optname, T& optval, socklen_t size = sizeof(T)) const {
    #ifdef _WIN32
            if (::getsockopt(desc, level, optname, reinterpret_cast<char*>(&optval), &size) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());
    #elif __linux__
            if (::getsockopt(desc, level, optname, &optval, &size) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());
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
        if (::listen(desc, clients) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());
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

    Socket accept() {
        std::lock_guard lck(socket_table_mtx);

        sockaddr_in client;
        socklen_t c = sizeof(sockaddr_in);

        FileDescriptor new_socket = ::accept(desc, reinterpret_cast<sockaddr*>(&client), &c);

        if (new_socket == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());

        socket_table.try_emplace(new_socket);
        socket_table.at(new_socket).param_table_id = uuid4();
        socket_table.at(new_socket).opened = true;
        socket_table.at(new_socket).working = true;
        socket_table.at(new_socket).sock_af = socket_table.at(desc).sock_af;
        socket_table.at(new_socket).sock_type = socket_table.at(desc).sock_type;
        socket_table.at(new_socket).raddress = sockaddr_in_to_SocketAddress(client);
        socket_table.at(new_socket).laddress = socket_table.at(desc).laddress;
        socket_table.at(new_socket).n_links = 0;

        return new_socket;
    }

    int64_t send(const void* data, int64_t size, int flags = 0) {
        std::lock_guard lck(socket_table_mtx);
        std::lock_guard lck1(socket_table.at(desc).sendMtx);

#ifdef _WIN32
        return ::send(desc, reinterpret_cast<const char*>(data), size, flags);
#elif __linux__
        return ::send(desc, data, size, MSG_NOSIGNAL | flags);
#endif
    }

    int64_t send(std::string data, int flags = 0) {
        return send(data.c_str(), data.size(), flags);
    }

    int64_t send(MsgPacket data, int flags = 0) {
        return send(data.c_str(), data.size(), flags);
    }

    int64_t send(BytesArray data, int flags = 0) {
        return send(data.c_str(), data.size(), flags);
    }

    int64_t send(uint8_t ch, int flags = 0) {
        return send(&ch, 1, flags);
    }

    int64_t sendall(const void* data, int64_t size, int flags = 0) {
        std::lock_guard lck(socket_table_mtx);
        std::lock_guard lck1(socket_table.at(desc).sendMtx);

        int64_t ptr = 0;
        int preverrno = errno;

        while (ptr < size) {
            int64_t n = send(((uint8_t*)data) + ptr, size - ptr, flags);

            if (n < 0) {
                errno = preverrno;
                shutdown();

                break;
            }

            ptr += n;
        }     
        
        return ptr;
    }

    int64_t sendall(std::string data, int flags = 0) {
        return sendall(data.c_str(), data.size(), flags);
    }

    int64_t sendall(MsgPacket data, int flags = 0) {
        return sendall(data.c_str(), data.size(), flags);
    }

    int64_t sendall(BytesArray data, int flags = 0) {
        return sendall(data.c_str(), data.size(), flags);
    }

    int64_t sendmsg(const void* data, uint32_t size) {
        std::lock_guard lck(socket_table_mtx);
        std::lock_guard lck1(socket_table.at(desc).sendMtx);

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
        std::lock_guard lck(socket_table_mtx);
        std::lock_guard lck1(socket_table.at(desc).sendMtx);

        BytesArray buffer;
        buffer.resize(256 * 1024);

        int64_t size = 0;

        while (file.tellg() != -1) {
            file.read(buffer.data(), 256 * 1024);
            size += send(buffer.c_str(), file.gcount());
        }

        return size;
    }

    int64_t sendto(const void* buf, int64_t size, std::string ipaddr, uint16_t port) {
        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

#ifdef _WIN32
        return ::sendto(desc, reinterpret_cast<const char*>(buf), size, MSG_CONFIRM, reinterpret_cast<sockaddr*>(&sock), sizeof(sockaddr_in));
#elif __linux__
        return ::sendto(desc, buf, size, MSG_CONFIRM, reinterpret_cast<sockaddr*>(&sock), sizeof(sockaddr_in));
#endif

        
    }

    int64_t sendto(void* buf, int64_t size, SocketAddress addr) { return sendto(buf, size, addr.ip, addr.port); }

    int64_t sendto(std::string buf, std::string ipaddr, uint16_t port) { return sendto(buf.c_str(), buf.size(), ipaddr, port); }
    
    int64_t sendto(MsgPacket buf, std::string ipaddr, uint16_t port) { return sendto(buf.c_str(), buf.size(), ipaddr, port); }

    int64_t sendto(std::string buf, SocketAddress addr) { return sendto(buf.c_str(), buf.size(), addr.ip, addr.port); }

    int64_t sendto(SocketData data) { return sendto(data.buffer.c_str(), data.buffer.size(), data.addr.ip, data.addr.port); }

    SocketData recv(uint32_t size) {
        std::lock_guard lck(socket_table_mtx);
        std::lock_guard lck1(socket_table.at(desc).recvMtx);

        BytesArray buffer;
        buffer.resize(size);

        int preverrno = errno;

        int64_t rsize = ::recv(desc, buffer.data(), size, 0);

        if (rsize < 0 || (!rsize && isBlocking())) {
            errno = preverrno;
            rsize = 0;

            shutdown();
            // std::cout << "Call close() from recv() due to error or closed socket." << std::endl;
        }

        SocketData data;
        data.buffer.set(buffer.c_str(), rsize);
        data.addr = socket_table.at(desc).raddress;

        return data;
    }

    SocketData recvall(uint32_t size) {
        std::lock_guard lck(socket_table_mtx);
        std::lock_guard lck1(socket_table.at(desc).recvMtx);

        SocketData ret;

        do ret.buffer.append(recv(size - ret.buffer.size()).buffer);
        while (ret.buffer.size() < size && isWorking());

        ret.addr = socket_table.at(desc).raddress;

        return ret;
    }

    SocketData recvmsg() {
        std::lock_guard lck(socket_table_mtx);
        std::lock_guard lck1(socket_table.at(desc).recvMtx);

        SocketData recvsize = recvall(sizeof(uint32_t));

        if (!recvsize.buffer.size()) {
            return {};
        }

        SocketData ret = recvall(ntohl(*reinterpret_cast<const uint32_t*>(recvsize.buffer.c_str())));

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

        BytesArray buffer;
        buffer.resize(size);

        int preverrno = errno;
        int64_t rsize = 0;

        if ((rsize = ::recvfrom(desc, buffer.data(), size, _MSG_WAITALL, reinterpret_cast<sockaddr*>(&sock), &len)) < 0 || errno == 104) {
            errno = preverrno;

            return {};
        }

        SocketData data;
        data.buffer.set(buffer.c_str(), rsize);
        data.addr = sockaddr_in_to_SocketAddress(sock);

        return data;
    }

    uint32_t tcpRecvAvailable() const {
        uint32_t bytes_available;
#ifdef _WIN32
        ioctlsocket(desc, FIONREAD, reinterpret_cast<unsigned long*>(&bytes_available));
#else
        ioctl(desc, FIONREAD, &bytes_available);
#endif
        return bytes_available;
    }

    const SocketAddress localSocketAddress() const {
        std::lock_guard lck(socket_table_mtx);

        return socket_table.at(desc).laddress;
    }

    const SocketAddress remoteSocketAddress() const {
        std::lock_guard lck(socket_table_mtx);

        return socket_table.at(desc).raddress;
    }
    
    void shutdown() {
        std::lock_guard lck(socket_table_mtx);

#ifdef _WIN32
        ::shutdown(desc, SD_BOTH);
#elif __linux__
        ::shutdown(desc, SHUT_RDWR);
#endif

        socket_table.at(desc).working = false;
    }

    void close() {
        std::lock_guard lck(socket_table_mtx);

        // std::cout << "Closing fd: " << desc << std::endl;
        if (isWorking()) shutdown();
        
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
bool operator==(fd_t fd, Socket& arg2) { return fd == arg2.fd(); }

bool operator!=(SocketAddress& arg1, SocketAddress& arg2) { return !(arg1 == arg2); }
bool operator!=(SocketData& arg1, SocketData& arg2) { return !(arg1 == arg2); }
bool operator!=(Socket& arg1, Socket& arg2) { return !(arg1 == arg2); }