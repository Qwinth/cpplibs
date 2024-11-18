// version 2.0.2
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <utility>
#include <cstring>
#include <cstdint>
#include <memory>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "windowsHeader.hpp"

#ifndef _WINSOCKAPI_
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define GETSOCKETERRNO() (WSAGetLastError())
#define MSG_CONFIRM 0

#undef MSG_WAITALL
#define MSG_WAITALL 0

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
#define GETSOCKETERRNO() (errno)
#endif

#pragma once

#include "libstrmanip.hpp"
#include "libmsgpacket.hpp"
#include "libfd.hpp"

struct sockaddress_t {
    std::string ip;
    uint16_t port = 0;

    std::string str() const { return strformat("%s:%d", ip.c_str(), port); }
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

namespace SocketUtils {
    bool is_socket_fd(int fd) {
    #ifdef _WIN32
        int optval;
        int optlen = sizeof(optval);

        int result = ::getsockopt(fd, SOL_SOCKET, SO_TYPE, (char*)&optval, &optlen);

        return (result == 0);
    #else
        struct stat statbuf;
        if (fstat(fd, &statbuf) == -1) {
            perror("fstat");
            return false;
        }
        return S_ISSOCK(statbuf.st_mode);
    #endif
    }

    bool is_IPv4(std::string ipaddr) {
        replaceAll(ipaddr, ".", "");
        return std::all_of(ipaddr.begin(), ipaddr.end(), ::isdigit);
    }

    std::string gethostbyname(std::string name) {
        hostent* remoteHost;
        in_addr addr;

        remoteHost = ::gethostbyname(name.c_str());

        addr.s_addr = *(uint64_t*)remoteHost->h_addr_list[0];
        return std::string(inet_ntoa(addr));
    }
};

class Socket : public FileDescriptor {
    sockaddress_t laddress;
    sockaddress_t raddress;

    std::unique_ptr<std::mutex> msgRecvMtx = nullptr;

    bool blocking = true;

#ifdef _WIN32
    WSADATA wsa;
#endif

    sockaddr_in make_sockaddr_in(std::string ipaddr, uint16_t port) {
        sockaddr_in tmpaddr;

        if (ipaddr.size()) {
            if (!SocketUtils::is_IPv4(ipaddr)) ipaddr = SocketUtils::gethostbyname(ipaddr);

            tmpaddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
        }

        else tmpaddr.sin_addr.s_addr = htonl(INADDR_ANY);        

        tmpaddr.sin_family = getsockfamily();
        tmpaddr.sin_port = htons(port);

        return tmpaddr;
    }

    inline void mutexInit() {
        msgRecvMtx = std::make_unique<std::mutex>();
    }

    sockaddress_t sockaddr_in_to_sockaddress_t(sockaddr_in addr) { return { inet_ntoa(addr.sin_addr), ntohs(addr.sin_port) }; }

    void copy(const Socket& obj) {
        desc = obj.desc;
        laddress = obj.laddress;
        raddress = obj.raddress;
        blocking = obj.blocking;
    }

    void swap(Socket& obj) {
        std::swap(desc, obj.desc);
        std::swap(laddress, obj.laddress);
        std::swap(raddress, obj.raddress);
        std::swap(blocking, obj.blocking);
    }
public:
    Socket() {}
    Socket(int _af, int _type) { open(_af, _type); }

    Socket(int fd) {
        if (!SocketUtils::is_socket_fd(fd)) {
            std::cerr << "Error: Socket(int fd): Non-socket fd" << std::endl;
            throw INVALID_SOCKET;
        }

        desc = fd;
#ifdef _WIN32
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        try { laddress = getsockname(); } catch (int) {}
        try { raddress = getpeername(); } catch (int) {}

        mutexInit();
    }

    Socket(const Socket& obj) {
        copy(obj);
        mutexInit();
    }

    Socket(Socket&& obj) {
        swap(obj);
        mutexInit();
    }

    Socket& operator=(const Socket& obj) {
        copy(obj);
        mutexInit();

        return *this;
    }

    Socket& operator=(Socket&& obj) {
        swap(obj);
        mutexInit();

        return *this;
    }

    void setblocking(bool _blocking) {
    #ifdef __linux__
        if (_blocking) fcntl(desc, F_SETFL, fcntl(desc, F_GETFL) & ~O_NONBLOCK);
        else fcntl(desc, F_SETFL, fcntl(desc, F_GETFL) | O_NONBLOCK);
    #elif _WIN32
        unsigned long __blocking = !_blocking;
        ioctlsocket(desc, FIONBIO, &__blocking);
    #endif

        blocking = _blocking;
    }

    bool is_blocking() {
#ifdef __linux__
        return !(fcntl(desc, F_GETFL) & O_NONBLOCK);
#elif _WIN32
        return blocking;
#endif
    }

    void open(int _af = AF_INET, int _type = SOCK_STREAM) {
#ifdef _WIN32
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        if ((desc = ::socket(_af, _type, 0)) == INVALID_SOCKET) throw GETSOCKETERRNO();

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

        if (::connect(desc, (sockaddr*)&sock, sizeof(sockaddr_in)) == INVALID_SOCKET) throw GETSOCKETERRNO();
    }

    void connect(std::string addr) {
        auto tmpaddr = split(addr, ':');

        connect(tmpaddr[0], std::stoi(tmpaddr[1]));
    }

    void bind(std::string ipaddr, uint16_t port) {
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (::bind(desc, (sockaddr*)&sock, sizeof(sockaddr_in)) == INVALID_SOCKET) throw GETSOCKETERRNO();

        laddress = sockaddr_in_to_sockaddress_t(sock);
    }

    void bind(std::string addr) {
        auto tmpaddr = split(addr, ':');

        bind(tmpaddr[0], std::stoi(tmpaddr[1]));
    }

    

    sockaddress_t getsockname() {
        sockaddr_in my_addr;
        socklen_t addrlen = sizeof(sockaddr_in);

        if (::getsockname(desc, (sockaddr*)&my_addr, &addrlen) == INVALID_SOCKET) throw GETSOCKETERRNO();
        return sockaddr_in_to_sockaddress_t(my_addr);
    }

    sockaddress_t getpeername() {
        sockaddr_in my_addr;
        socklen_t addrlen = sizeof(sockaddr_in);

        if (::getpeername(desc, (sockaddr*)&my_addr, &addrlen) == INVALID_SOCKET) throw GETSOCKETERRNO();
        return sockaddr_in_to_sockaddress_t(my_addr);
    }

    void setsockopt(int level, int optname, void* optval, int size) {
#ifdef _WIN32
        if (::setsockopt(desc, level, optname, (char*)optval, size) == INVALID_SOCKET) throw GETSOCKETERRNO();
#elif __linux__
        if (::setsockopt(desc, level, optname, optval, size) == INVALID_SOCKET) throw GETSOCKETERRNO();
#endif
    }

    void setsockopt(int level, int optname, int optval) {
        setsockopt(level, optname, &optval, sizeof(int));
    }

    int getsockopt(int level, int optname, void* optval, socklen_t size) {
#ifdef _WIN32
        if (::getsockopt(desc, level, optname, (char*)optval, &size) == INVALID_SOCKET) throw GETSOCKETERRNO();
#elif __linux__
        if (::getsockopt(desc, level, optname, optval, &size) == INVALID_SOCKET) throw GETSOCKETERRNO();
#endif
        return size;
    }

    int getsocktype() {
        int type;
        getsockopt(SOL_SOCKET, SO_TYPE, &type, sizeof(int));

        return type;
    }
#ifdef _WIN32
    int getsockfamily() {
        WSAPROTOCOL_INFO proto;
        WSADuplicateSocket(desc, GetCurrentProcessId(), &proto);

        return proto.iAddressFamily;
    }
#elif __linux__
    uint16_t getsockfamily() {
        sockaddr af;
        getsockopt(SOL_SOCKET, SO_DOMAIN, &af, sizeof(sockaddr));

        return af.sa_family;
    }
#endif

    void listen(int clients) {
        if (::listen(desc, clients) == INVALID_SOCKET) throw GETSOCKETERRNO();
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

    Socket accept() {
        using std::cout, std::endl;
        sockaddr_in client;
        socklen_t c = sizeof(sockaddr_in);

        auto new_socket = ::accept(desc, (sockaddr*)&client, (socklen_t*)&c);

        if (new_socket == INVALID_SOCKET) throw GETSOCKETERRNO();

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

    int64_t send(char ch) {
        char chbuf[1];
        chbuf[0] = ch;

        return send(chbuf, 1);
    }

    int64_t sendall(const void* chardata, int64_t size) {
        int64_t ptr = 0;

        while (ptr < size) {
            int64_t n = send(((char*)chardata) + ptr, size - ptr);

            if (n < 0) break;

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

    int64_t sendmsg(const void* data, uint32_t size) {
        MsgPacket packet;

        uint32_t netsize = htonl(size);
        packet.write(&netsize, sizeof(netsize));
        packet.write(data, size);

        int64_t retsize = sendall(packet);

        return retsize;
    }

    int64_t sendmsg(std::string data) {
        return sendmsg(data.c_str(), data.size());
    }

    int64_t sendmsg(MsgPacket data) {
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

    int64_t sendto(char* buf, int64_t size, sockaddress_t addr) { return sendto(buf, size, addr.ip, addr.port); }

    int64_t sendto(std::string buf, std::string ipaddr, uint16_t port) { return sendto(buf.c_str(), buf.size(), ipaddr, port); }
    
    int64_t sendto(MsgPacket buf, std::string ipaddr, uint16_t port) { return sendto(buf.c_str(), buf.size(), ipaddr, port); }

    int64_t sendto(std::string buf, sockaddress_t addr) { return sendto(buf.c_str(), buf.size(), addr.ip, addr.port); }

    int64_t sendto(sockrecv_t data) { return sendto(data.buffer, data.size, data.addr.ip, data.addr.port); }

    sockrecv_t recv(int64_t size) {
        char* buffer = new char[size];
        std::memset(buffer, 0, size);

        int preverrno = errno;

        sockrecv_t data;
        if ((data.size = ::recv(desc, buffer, size, 0)) < 0) {
            errno = preverrno;

            delete[] buffer;
            return {};
        }

        data.buffer = new char[data.size];
        std::memcpy(data.buffer, buffer, data.size);

        data.string.assign(data.buffer, data.size);
        data.addr = raddress;

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

            if (part.size < 0 || is_blocking() && !part.size) break;

            std::memcpy(ret.buffer + bufptr, part.buffer, part.size);
            bufptr += part.size;
        }

        ret.addr = raddress;
        ret.size = bufptr;
        ret.string = std::string(ret.buffer, bufptr);

        return ret;
    }

    sockrecv_t recvmsg() {
        msgRecvMtx->lock();

        sockrecv_t recvsize = recvall(sizeof(uint32_t));

        if (!recvsize.size) {
            msgRecvMtx->unlock();
            return {};
        }

        sockrecv_t ret = recvall(ntohl(*(uint32_t*)recvsize.buffer));

        msgRecvMtx->unlock();

        return ret;
    }

    uint8_t recvbyte() {
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

        if ((data.size = ::recvfrom(desc, buffer, size, MSG_WAITALL, (sockaddr*)&sock, &len)) < 0 || errno == 104) {
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

    uint32_t tcpRecvAvailable() const {
        uint32_t bytes_available;
#ifdef _WIN32
        ioctlsocket(desc, FIONREAD, (unsigned long*)&bytes_available);
#else
        ioctl(desc, FIONREAD, &bytes_available);
#endif
        return bytes_available;
    }

    const sockaddress_t localSocketAddress() const {
        return laddress;
    }

    const sockaddress_t remoteSocketAddress() const {
        return raddress;
    }

    void close() {
#ifdef _WIN32
        ::shutdown(desc, SD_BOTH);
        ::closesocket(desc);
        ::WSACleanup();
#elif __linux__
        ::shutdown(desc, SHUT_RDWR);
        ::close(desc);
#endif
    }
};

// struct sockevent_t {
//     Socket sock;
//     int events = 0;
// };

// class Poll {
//     std::vector<pollfd> fds;
//     std::map<int, Socket> usingSockets;

//     pollfd sock_to_pollfd(Socket sock, short events) {
//         return { sock.fd(), events, 0 };
//     }
// public:
//     void addSocket(Socket sock, short events) {
//         fds.push_back(sock_to_pollfd(sock, events));
//         usingSockets[sock.fd()] = sock;
//     }

//     void removeSocket(Socket sock) {
//         for (int i = 0; i < fds.size(); i++) {
//             int fd = fds[i].fd;

//             if (fd == sock.fd()) {
//                 fds.erase(fds.begin() + i);
//                 break;
//             }
//         }

//         usingSockets.erase(sock.fd());
//     }

//     std::vector<sockevent_t> poll(int timeout = -1) {
//         int nevents = ::poll(fds.data(), fds.size(), timeout);

//         std::vector<sockevent_t> ret;

//         for (pollfd& i : fds) {
//             Socket sock = usingSockets[i.fd];

//             if (i.revents & i.events) {
//                 ret.push_back(sockevent_t({sock, i.revents}));
//                 i.revents = 0;
//             }
//         }

//         return ret;
//     }
// };

bool operator==(sockaddress_t& arg1, sockaddress_t& arg2) { return arg1.ip == arg2.ip && arg1.port == arg2.port; }
bool operator==(sockrecv_t& arg1, sockrecv_t& arg2) { return arg1.addr == arg2.addr && arg1.string == arg2.string && arg1.size == arg2.size; }
bool operator==(Socket& arg1, Socket& arg2) { return arg1.fd() == arg2.fd(); }
bool operator==(int fd, Socket& arg2) { return fd == arg2.fd(); }

bool operator!=(sockaddress_t& arg1, sockaddress_t& arg2) { return !(arg1 == arg2); }
bool operator!=(sockrecv_t& arg1, sockrecv_t& arg2) { return !(arg1 == arg2); }
bool operator!=(Socket& arg1, Socket& arg2) { return !(arg1 == arg2); }