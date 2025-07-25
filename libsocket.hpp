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

class SocketParamTable {
    mutable std::shared_mutex table_mtx;

    std::atomic_bool working;
    std::atomic_bool opened;
    std::atomic_bool blocking;

    std::string param_table_id;

    int sock_af;
    int sock_type;

    SocketAddress laddress;
    SocketAddress raddress;

    std::recursive_mutex recvMtx;
    std::recursive_mutex sendMtx;

    std::atomic_int n_links;

public:
    bool isWorking() const {
        std::shared_lock lck(table_mtx);

        return working;
    }

    bool isOpened() const {
        std::shared_lock lck(table_mtx);

        return opened;
    }

    bool isBlocking() const {
        std::shared_lock lck(table_mtx);

        return blocking;
    }

    std::string getParamTableId() const {
        std::shared_lock lck(table_mtx);
        
        return param_table_id;
    }

    int getSocketFamily() const {
        std::shared_lock lck(table_mtx);

        return sock_af;
    }

    int getSocketType() const {
        std::shared_lock lck(table_mtx);

        return sock_type;
    }

    SocketAddress getSocketLocalAddress() const {
        std::shared_lock lck(table_mtx);

        return laddress;
    }

    SocketAddress getSocketRemoteAddress() const {
        std::shared_lock lck(table_mtx);

        return raddress;
    }

    std::recursive_mutex& getRecvMutex() {
        std::shared_lock lck(table_mtx);

        return recvMtx;
    }

    std::recursive_mutex& getSendMutex() {
        std::shared_lock lck(table_mtx);

        return sendMtx;
    }

    int getLinkCount() const {
        std::shared_lock lck(table_mtx);

        return n_links;
    }

    void setWorking(bool __val) {
        std::lock_guard lck(table_mtx);

        working = __val;
    }

    void setOpened(bool __val) {
        std::lock_guard lck(table_mtx);

        opened = __val;
    }

    void setBlocking(bool __val) {
        std::lock_guard lck(table_mtx);

        blocking = __val;
    }

    void setParamTableId(std::string __val) {
        std::lock_guard lck(table_mtx);

        param_table_id = __val;
    }

    void setSocketFamily(int __val) {
        std::lock_guard lck(table_mtx);

        sock_af = __val;
    }

    void setSocketType(int __val) {
        std::lock_guard lck(table_mtx);

        sock_type = __val;
    }

    void setSocketLocalAddress(SocketAddress __val) {
        std::lock_guard lck(table_mtx);

        laddress = __val;
    }

    void setSocketRemoteAddress(SocketAddress __val) {
        std::lock_guard lck(table_mtx);

        raddress = __val;
    }

    void setLinkCount(int __val) {
        std::lock_guard lck(table_mtx);

        n_links = __val;
    }

    void newLink() {
        std::lock_guard lck(table_mtx);

        n_links++;
    }

    void removeLink() {
        std::lock_guard lck(table_mtx);

        n_links--;
    }
};

class SocketTable {
    mutable std::shared_mutex socket_table_mtx;
    
    std::map<FileDescriptor, SocketParamTable> socket_table;

public:
    SocketParamTable& getParamTable(FileDescriptor fd) {
        std::shared_lock lck(socket_table_mtx);

        if (socket_table.find(fd) == socket_table.end()) throw std::runtime_error("getParamTable(FileDescriptor): Non-socket or closed fd");

        return socket_table.at(fd);
    }

    SocketParamTable& createParamTable(FileDescriptor fd) {
        std::lock_guard lck(socket_table_mtx);

        socket_table.try_emplace(fd);

        return socket_table.at(fd);
    }

    void removeParamTable(FileDescriptor fd) {
        std::lock_guard lck(socket_table_mtx);

        if (socket_table.find(fd) != socket_table.end()) socket_table.erase(fd);
    }

    bool existsParamTable(FileDescriptor fd) const {
        std::shared_lock lck(socket_table_mtx);

        return socket_table.find(fd) != socket_table.end();
    }
};

SocketTable socket_table;

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
        if (!socket_table.existsParamTable(fd)) throw std::runtime_error("Socket(FileDescriptor): Non-socket or closed fd");

        desc = fd;
        param_id = socket_table.getParamTable(desc).getParamTableId();

        socket_table.getParamTable(desc).newLink();

        // std::cout << "New container by constructor(FD) for fd: " << desc << ", Links: " << socket_table.getParamTable(desc).n_links << ", container: " << this << std::endl;
    }

    Socket(const Socket& obj) {
        desc = obj.desc;
        param_id = socket_table.getParamTable(desc).getParamTableId();

        socket_table.getParamTable(desc).newLink();

        // std::cout << "New container by constructor(COPY) for fd: " << desc << ", Links: " << socket_table.getParamTable(desc).n_links << ", container: " << this << std::endl;
    }

    Socket(Socket&& obj) {
        std::swap(desc, obj.desc);
        param_id = socket_table.getParamTable(desc).getParamTableId();

        // std::cout << "Move container by constructor(MOVE) for fd: " << desc << ", Links: " << socket_table.getParamTable(desc).n_links << ", container: " << this << std::endl;
    }

    ~Socket() {
        if (!socket_table.existsParamTable(desc) || socket_table.getParamTable(desc).getParamTableId() != param_id) return;

        socket_table.getParamTable(desc).removeLink();

        // std::cout << "Deleting container: " << this << ", Links: " << socket_table.getParamTable(desc).n_links << ", fd: " << desc << std::endl;

        if (!socket_table.getParamTable(desc).getLinkCount() && !socket_table.getParamTable(desc).isOpened()) {
            socket_table.removeParamTable(desc);

            // std::cout << "No containers for fd: " << desc << "! Removing socket param table for fd: " << desc << "." << std::endl;
            // std::cout << "Socket param table size: " << socket_table.size() << std::endl;         
        }
    }

    Socket& operator=(FileDescriptor fd) {
        desc = fd;
        param_id = socket_table.getParamTable(desc).getParamTableId();

        socket_table.getParamTable(desc).newLink();

        // std::cout << "New container by operator(FD) for fd: " << desc << ", Links: " << socket_table.getParamTable(desc).n_links << ", container: " << this << std::endl;

        return *this;
    }

    Socket& operator=(const Socket& obj) {
        desc = obj.desc;
        param_id = socket_table.getParamTable(desc).getParamTableId();

        socket_table.getParamTable(desc).newLink();

        // std::cout << "New container by operator(COPY) for fd: " << desc << ", Links: " << socket_table.getParamTable(desc).n_links << ", container: " << this << std::endl;

        return *this;
    }

    Socket& operator=(Socket&& obj) {
        std::swap(desc, obj.desc);
        param_id = socket_table.getParamTable(desc).getParamTableId();

        // std::cout << "Move container by operator(MOVE) for fd: " << desc << ", Links: " << socket_table.getParamTable(desc).n_links << ", container: " << this << std::endl;

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

        socket_table.getParamTable(desc).setBlocking(_blocking);
    }

    bool isBlocking() const {
#ifdef __linux__
        return !(fcntl(desc, F_GETFL) & O_NONBLOCK);
#elif _WIN32
        return socket_table.getParamTable(desc).isBlocking();
#endif
    }

    void open(int _af = AF_INET, int _type = SOCK_STREAM) {
#ifdef _WIN32
        WSAData wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        if ((desc = ::socket(_af, _type, 0)) == INVALID_SOCKET) throw std::runtime_error(GETSOCKETERRNOMSG());

        param_id = uuid4();
        
        socket_table.createParamTable(desc);
        socket_table.getParamTable(desc).setOpened(true);
        socket_table.getParamTable(desc).setWorking(true);
        socket_table.getParamTable(desc).setParamTableId(param_id);
        socket_table.getParamTable(desc).setSocketFamily(_af);
        socket_table.getParamTable(desc).setSocketType(_type);
        socket_table.getParamTable(desc).setLinkCount(1);
        // std::cout << "New container by constructor(OPEN) for fd: " << desc << ", Links: " << socket_table.getParamTable(desc).n_links << ", container: " << this << std::endl;
    }

    bool isOpened() const {
        return socket_table.existsParamTable(desc) && socket_table.getParamTable(desc).isOpened();
    }

    bool isWorking() const {
        return socket_table.existsParamTable(desc) && socket_table.getParamTable(desc).isWorking();
    }

    void connect(std::string ipaddr, uint16_t port) {
        if (ipaddr == "") ipaddr = "127.0.0.1";
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::connect(desc, reinterpret_cast<sockaddr*>(&sock), sizeof(sockaddr_in)) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());

        socket_table.getParamTable(desc).setSocketLocalAddress(getsockname());
        socket_table.getParamTable(desc).setSocketRemoteAddress(sockaddr_in_to_SocketAddress(sock));
    }

    void connect(std::string addr) {
        auto tmpaddr = split(addr, ':');

        connect(tmpaddr[0], std::stoi(tmpaddr[1]));
    }

    void bind(std::string ipaddr, uint16_t port) {
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::bind(desc, reinterpret_cast<sockaddr*>(&sock), sizeof(sockaddr_in)) == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());

        socket_table.getParamTable(desc).setSocketLocalAddress(sockaddr_in_to_SocketAddress(sock));
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
        sockaddr_in client;
        socklen_t c = sizeof(sockaddr_in);

        FileDescriptor new_socket = ::accept(desc, reinterpret_cast<sockaddr*>(&client), &c);

        if (new_socket == SOCKET_ERROR) throw std::runtime_error(GETSOCKETERRNOMSG());

        socket_table.createParamTable(new_socket);
        socket_table.getParamTable(new_socket).setOpened(true);
        socket_table.getParamTable(new_socket).setWorking(true);
        socket_table.getParamTable(new_socket).setParamTableId(uuid4());
        socket_table.getParamTable(new_socket).setSocketFamily(socket_table.getParamTable(desc).getSocketFamily());
        socket_table.getParamTable(new_socket).setSocketType(socket_table.getParamTable(desc).getSocketType());
        socket_table.getParamTable(new_socket).setSocketRemoteAddress(sockaddr_in_to_SocketAddress(client));
        socket_table.getParamTable(new_socket).setSocketLocalAddress(socket_table.getParamTable(desc).getSocketLocalAddress());
        socket_table.getParamTable(new_socket).setLinkCount(0);

        return new_socket;
    }

    int64_t send(const void* data, int64_t size, int flags = 0) {
        std::lock_guard lck(socket_table.getParamTable(desc).getSendMutex());

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

    int64_t send(char ch, int flags = 0) {
        return send(&ch, 1, flags);
    }

    int64_t sendall(const void* data, int64_t size, int flags = 0) {
        std::lock_guard lck(socket_table.getParamTable(desc).getSendMutex());

        int64_t ptr = 0;
        int preverrno = errno;

        while (ptr < size) {
            int64_t n = send(((char*)data) + ptr, size - ptr, flags);

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
        std::lock_guard lck(socket_table.getParamTable(desc).getSendMutex());

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
        std::lock_guard lck(socket_table.getParamTable(desc).getSendMutex());

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
        std::lock_guard lck(socket_table.getParamTable(desc).getRecvMutex());

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
        data.addr = socket_table.getParamTable(desc).getSocketRemoteAddress();

        return data;
    }

    SocketData recvall(uint32_t size) {
        std::lock_guard lck(socket_table.getParamTable(desc).getRecvMutex());

        SocketData ret;

        do ret.buffer.append(recv(size - ret.buffer.size()).buffer);
        while (ret.buffer.size() < size && isWorking());

        ret.addr = socket_table.getParamTable(desc).getSocketRemoteAddress();

        return ret;
    }

    SocketData recvmsg() {
        std::lock_guard lck(socket_table.getParamTable(desc).getRecvMutex());

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
        return socket_table.getParamTable(desc).getSocketLocalAddress();
    }

    const SocketAddress remoteSocketAddress() const {
        return socket_table.getParamTable(desc).getSocketRemoteAddress();
    }
    
    void shutdown() {
#ifdef _WIN32
        ::shutdown(desc, SD_BOTH);
#elif __linux__
        ::shutdown(desc, SHUT_RDWR);
#endif

        socket_table.getParamTable(desc).setWorking(false);
    }

    void close() {
        // std::cout << "Closing fd: " << desc << std::endl;
        if (isWorking()) shutdown();
        
#ifdef _WIN32
        ::closesocket(desc);
        ::WSACleanup();
#elif __linux__
        ::close(desc);
#endif

        socket_table.getParamTable(desc).setOpened(false);
    }
};

bool operator==(SocketAddress& arg1, SocketAddress& arg2) { return arg1.ip == arg2.ip && arg1.port == arg2.port; }
bool operator==(SocketData& arg1, SocketData& arg2) { return arg1.addr == arg2.addr && arg1.buffer == arg2.buffer; }
bool operator==(Socket& arg1, Socket& arg2) { return arg1.fd() == arg2.fd(); }
bool operator==(fd_t fd, Socket& arg2) { return fd == arg2.fd(); }

bool operator!=(SocketAddress& arg1, SocketAddress& arg2) { return !(arg1 == arg2); }
bool operator!=(SocketData& arg1, SocketData& arg2) { return !(arg1 == arg2); }
bool operator!=(Socket& arg1, Socket& arg2) { return !(arg1 == arg2); }