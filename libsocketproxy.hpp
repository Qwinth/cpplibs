#pragma once
#include <thread>
#include <functional>
#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include "libsocket.hpp"
#include "libpoll.hpp"
#include "libuuid4.hpp"

struct ProxyTable {
    Socket sock1;
    Socket sock2;

    std::thread proxy_thread;

    std::function<void(Socket, Socket)> proxy_sniff_callback;
    std::atomic_bool callback_enabled;

    std::atomic_bool working;
    std::atomic_bool auto_close;
    std::atomic_int n_links;
};

std::mutex proxyTableMtx;

std::map<std::string, ProxyTable> proxyTable;

inline Socket getTo(Socket sock1, Socket sock2, Socket from) {
    return (from == sock1) ? sock2 : sock1;
}

void proxy_manager(std::string id) {
    ProxyTable& table = proxyTable[id];

    Poller poller;
    poller.addDescriptor(table.sock1, POLLIN | POLLPRI | POLLHUP);
    poller.addDescriptor(table.sock2, POLLIN | POLLPRI | POLLHUP);

    while (table.working) {
        auto ev = poller.poll(10);

        if (!ev.size()) continue;
        
        Socket from = ev.front().fd;
        Socket to = getTo(table.sock1, table.sock2, from);

        if (table.callback_enabled) table.proxy_sniff_callback(from, to);
        else to.sendall(from.recvall(from.tcpRecvAvailable()).buffer);

        if (!table.sock1.is_opened() || !table.sock2.is_opened()) table.working = false;
    }

    if (table.auto_close) {
        table.sock1.close();
        table.sock2.close();
    }

}

class SocketProxy {
    std::string proxy_id;
public:
    SocketProxy() {}
    SocketProxy(Socket socket1, Socket socket2, std::function<void(Socket, Socket)> func = nullptr) {
        create(socket1, socket2, func);
    }

    SocketProxy(std::string id) : proxy_id(id) {
        proxyTable[proxy_id].n_links++;
    }

    SocketProxy(const SocketProxy& obj) {
        proxy_id = obj.proxy_id;
        proxyTable[proxy_id].n_links++;
    }

    SocketProxy(SocketProxy&& obj) {
        std::swap(proxy_id, obj.proxy_id);
    }

    ~SocketProxy() {
        if (proxyTable.find(proxy_id) == proxyTable.end()) return;

        proxyTable[proxy_id].n_links--;

        if (!proxyTable[proxy_id].n_links) terminate();
    }

    SocketProxy& operator=(const SocketProxy& obj) {
        proxy_id = obj.proxy_id;
        proxyTable[proxy_id].n_links++;

        return *this;
    }

    SocketProxy& operator=(SocketProxy&& obj) {
        std::swap(proxy_id, obj.proxy_id);
        
        return *this;
    }

    void create(Socket socket1, Socket socket2, std::function<void(Socket, Socket)> func = nullptr) {
        if (!socket1.is_opened() || !socket2.is_opened()) return;

        proxy_id = uuid4();

        proxyTableMtx.lock();

        // proxyTable[proxy_id].poller.addDescriptor(socket1, POLLIN | POLLERR | POLLHUP);
        // proxyTable[proxy_id].poller.addDescriptor(socket2, POLLIN | POLLERR | POLLHUP);
        proxyTable[proxy_id].sock1 = socket1;
        proxyTable[proxy_id].sock2 = socket2;
        proxyTable[proxy_id].proxy_sniff_callback = func;
        proxyTable[proxy_id].callback_enabled = func != nullptr;
        proxyTable[proxy_id].auto_close = false;
        proxyTable[proxy_id].n_links = 1;

        proxyTableMtx.unlock();
    }

    void start() {
        waitStop();

        proxyTable[proxy_id].working = true;
        proxyTable[proxy_id].proxy_thread = std::thread(proxy_manager, proxy_id);
    }

    void stop() {
        proxyTable[proxy_id].working = false;
    }

    void terminate() {
        stop();
        waitStop();

        if (proxyTable.find(proxy_id) != proxyTable.end()) proxyTable.erase(proxy_id);
        
    }

    void setSniffer(std::function<void(Socket, Socket)> func) {
        proxyTable[proxy_id].proxy_sniff_callback = func;
        proxyTable[proxy_id].callback_enabled = func != nullptr;
    }

    void autoClose(bool state) {
        proxyTable[proxy_id].auto_close = state;
    }

    void waitStop() {
        if (proxyTable[proxy_id].proxy_thread.joinable()) proxyTable[proxy_id].proxy_thread.join();
    }
};