// version 1.8.2
#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>
#include <string.h>
#include "strlib.hpp"


#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "windowsHeader.hpp"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define GETSOCKETERRNO() (WSAGetLastError())

#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define GETSOCKETERRNO() (errno)
#endif

struct sockaddress_t {
    std::string ip;
    int port;
};

struct sockrecv_t {
    std::string string;
    void* buffer;
    ssize_t length;
    sockaddress_t addr;
};

struct ret {
    int retval;
    int error;
};

class SSocket {
    int af;

    bool checkIp(std::string ipaddr) {
        for (auto &i : split(ipaddr, '.')) if (!std::all_of(i.begin(), i.end(), ::isdigit)) return false;
        return true;
    }

    sockaddr_in make_sockaddr_in(std::string ipaddr, int port) {
        sockaddr_in tmpaddr;

        if (ipaddr != "") {
            if (!checkIp(ipaddr)) ipaddr = sgethostbyname(ipaddr);

            tmpaddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
        }
        else {
            tmpaddr.sin_addr.s_addr = INADDR_ANY;
        }
        tmpaddr.sin_family = af;
        tmpaddr.sin_port = htons(port);

        return tmpaddr;
    }

    sockaddress_t sockaddr_in_to_sockaddress_t(sockaddr_in addr) { return { inet_ntoa(addr.sin_addr), htons(addr.sin_port)}; }
public:
#ifdef _WIN32
    WSADATA wsa;
    SOCKET s;
#elif __linux__
    int s;
#endif

    SSocket(int _af, int type) {
        af = _af;
#ifdef _WIN32
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        if ((s = socket(af, type, 0)) == INVALID_SOCKET) throw GETSOCKETERRNO();
    }
#ifdef _WIN32
    SSocket(SOCKET ss) {
        s = ss;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
#elif __linux__
    SSocket(int ss) { s = ss; }
#endif
    /*~SSocket() {
        this->sclose();
        std::cout << "SSocket removed!" << std::endl;
    }*/


    void sconnect(std::string ipaddr, int port) {
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (connect(s, (sockaddr*)&sock, sizeof(sockaddr_in)) == SOCKET_ERROR) throw GETSOCKETERRNO();
    }

    void sbind(std::string ipaddr, int port) {
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        if (bind(s, (sockaddr*)&sock, sizeof(sockaddr_in)) == SOCKET_ERROR) throw GETSOCKETERRNO();
    }

    std::string sgethostbyname(std::string name) {
        hostent* remoteHost;
        in_addr addr;

        remoteHost = gethostbyname(name.c_str());

        addr.s_addr = *(uint64_t*)remoteHost->h_addr_list[0];
        return std::string(inet_ntoa(addr));
    }


    sockaddress_t sgetsockname() {
        sockaddr_in my_addr;
        int addrlen = sizeof(sockaddr_in);

#ifdef _WIN32
        if (getsockname(s, (sockaddr*)&my_addr, &addrlen) == SOCKET_ERROR) throw GETSOCKETERRNO();
#elif __linux__
        if (getsockname(s, (sockaddr*)&my_addr, (socklen_t*)&addrlen) == SOCKET_ERROR) throw GETSOCKETERRNO();
#endif
        return sockaddr_in_to_sockaddress_t(my_addr);
    }

    void _ssetsockopt(int level, int optname, void* optval, int size) {
#ifdef _WIN32
        if (setsockopt(s, level, optname, reinterpret_cast<const char*>(optval), sizeof(reinterpret_cast<const char*>(&optval))) == SOCKET_ERROR) throw GETSOCKETERRNO();
#elif __linux__
        if (setsockopt(s, level, optname, optval, size) == SOCKET_ERROR) throw GETSOCKETERRNO();
#endif
    }

    void ssetsockopt(int level, int optname, int optval) {
        _ssetsockopt(level, optname, &optval, sizeof(int));
    }

    int sgetsockopt(int level, int optname, int optval) {
        int error_code_size = sizeof(int);
#ifdef _WIN32
        int r = getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)optval, &error_code_size);
#elif __linux__
        int r = getsockopt(s, SOL_SOCKET, SO_ERROR, &optval, (socklen_t*)&error_code_size);
#endif
        return r;
    }

    void slisten(int clients) {
        if (listen(s, clients) == SOCKET_ERROR) throw GETSOCKETERRNO();
    }

    void setrecvtimeout(int seconds) {
        timeval timeout;
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;
        _ssetsockopt(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval));
    }

    std::pair<SSocket, sockaddress_t> saccept() {
        sockaddr_in client;
        int c = sizeof(sockaddr_in);

#ifdef _WIN32
        auto new_socket = accept(s, (sockaddr*)&client, &c);
#elif __linux__
        auto new_socket = accept(s, (sockaddr*)&client, (socklen_t*)&c);
#endif
        if (new_socket == INVALID_SOCKET) throw GETSOCKETERRNO();

        return { new_socket, sockaddr_in_to_sockaddress_t(client) };
    }

    size_t ssend(std::string data) {
#ifdef _WIN32
        return send(s, data.c_str(), data.length(), 0);
#elif __linux__
        return send(s, data.c_str(), data.length(), MSG_NOSIGNAL);
#endif
    }

     size_t ssend(const void* data, size_t length) {
#ifdef _WIN32
        return send(s, (const char*)data, length, 0);
#elif __linux__
        return send(s, data, length, MSG_NOSIGNAL);
#endif
    }

    size_t ssendall(std::string data) {
        size_t size = data.length();
        size_t sended = 0;

        while (sended < size) {
            int i = ssend(data.substr(sended, size - sended));
            sended += i;
        }
        return sended;
    }

    size_t ssendall(const void* chardata, size_t length) {
        size_t sended = 0;
        char *buff = new char[length];

        while (sended < length) {
            substr(sended, length - sended, (const char*)chardata, buff);
            int i = ssend(buff, length - sended);
            sended += i;
        }

        delete[] buff;
        return sended;
    }

    size_t ssend_file(std::ifstream& file) {
        char buffer[65535];
        size_t size = 0;
        while (file.tellg() != -1) {
            file.read(buffer, 65535);
#ifdef _WIN32
            size += send(s, buffer, file.gcount(), 0);
#elif __linux__
            size += send(s, buffer, file.gcount(), MSG_NOSIGNAL);
#endif
            memset(buffer, 0, sizeof(buffer));
        }
        return size;
    }

    size_t ssendto(char* buf, size_t size, std::string ipaddr, int port) {
        sockaddr_in sock = make_sockaddr_in(ipaddr, port);

        return sendto(s, buf, size, MSG_CONFIRM, (sockaddr*)&sock, sizeof(sockaddr_in));
    }

    size_t ssendto(char* buf, size_t size, sockaddress_t addr) { return ssendto(buf, size, addr.ip, addr.port); }

    size_t ssendto(std::string buf, std::string ipaddr, int port) { return ssendto((char*)buf.c_str(), buf.size(), ipaddr, port); }

    size_t ssendto(std::string buf, sockaddress_t addr) { return ssendto((char*)buf.c_str(), buf.size(), addr.ip, addr.port); }
    
    size_t ssendto(sockrecv_t data) { return ssendto((char*)data.buffer, data.length, data.addr.ip, data.addr.port); }

    sockrecv_t srecv(int size) {
#ifndef _DISABLE_RECV_LIMIT
        if (size > 32768) {
            std::cout << "Error: srecv max value 32768" << std::endl;
            exit(EXIT_FAILURE);
        }
        char buffer[32768];
#else
        char buffer[65535];
#endif
        memset(buffer, 0, size);

        int preverrno = errno;

        sockrecv_t data;
        if ((data.length = recv(s, buffer, size, 0)) < 0 || errno == 104) { errno = preverrno; return {}; }
        data.buffer = buffer;
        data.string.assign(buffer, data.length);

        return data;
    }

    sockrecv_t srecvfrom(size_t size) {
        sockaddr_in sock;
        socklen_t len = sizeof(sockaddr_in);

        #ifndef _DISABLE_RECV_LIMIT
        if (size > 32768) {
            std::cout << "Error: srecv_char max value 32768" << std::endl;
            exit(EXIT_FAILURE);
        }
        char buffer[32768];
#else
        char buffer[65535];
#endif  
        int preverrno = errno;
        
        sockrecv_t data;
        if ((data.length = recvfrom(s, buffer, size, MSG_WAITALL, (sockaddr*)&sock, &len)) < 0 || errno == 104) { errno = preverrno; return {}; }
        data.buffer = buffer;
        data.string.assign(buffer, data.length);
        data.addr = sockaddr_in_to_sockaddress_t(sock);

        return data;
    }

    void sclose() {
#ifdef _WIN32
        shutdown(s, SD_BOTH);
        closesocket(s);
        WSACleanup();
#elif __linux__
        shutdown(s, SHUT_RDWR);
        close(s);
#endif
    }
};
