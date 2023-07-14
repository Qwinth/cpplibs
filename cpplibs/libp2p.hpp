#include <SSocket.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <algorithm>
#include <strlib.hpp>
#include <chrono>
using namespace std;
class P2P {
	struct MSG {
		char * data;
		SSocket * sock;
		unsigned long long length = 0;
	};
	//string myid = uuid4();
	string srvaddr;
	int srvport = 1500;
	vector<MSG*> msgqueue;

	void listener(SSocket sock, string peerid) {
		//cout << "in listener" << endl;
		while (true) {
			auto data = sock.srecv_char(1024);
			//cout << "listener process" << endl;
			if (data.length == 0) {
				if (uuidpeers.find(peerid) != uuidpeers.end()) {
					uuidpeers[peerid]->sclose();
					uuidpeers.erase(peerid);
				}
				sock.sclose();
				break;
			}
			MSG* msg = new MSG;
			msg->data = data.value;
			msg->sock = new SSocket(sock);
			msg->length = data.length;
			msgqueue.push_back(msg);
			//cout << msgqueue.size() << endl;
		}
	}

	void listenSock(string ip, int port) {
		SSocket s(AF_INET, SOCK_STREAM);
		s.ssetsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
		s.sbind(ip, port);
		s.slisten(0);
		//cout << "in listensock " << ip << ":" << port << endl;
		while (true) {
			auto sock = s.saccept();
			sock.ssend(format("connected;%s", myid.c_str()));
			string peerid = sock.srecv(1024);
			//cout << "accept" << endl;
			while (uuidpeers.find(peerid) == uuidpeers.end()) {}
			//cout << "out ls while" << endl;
			sock.ssend("ok");
			//cout << "out ssend" << endl;
			thread threadObj(&P2P::listener, this, sock, peerid);
			threadObj.detach();
		}
	}

	void connectPeer() {
		SSocket signalsock(AF_INET, SOCK_STREAM);
		signalsock.ssetsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
		signalsock.sconnect(srvaddr, srvport);
		auto selfaddr = signalsock.sgetsockname();
		//cout << format("reg;%s:%d;%s", selfaddr.ip, selfaddr.port, myid.c_str()) << endl;
		signalsock.ssend(format("reg;%s:%d;%s", selfaddr.ip, selfaddr.port, myid.c_str()));
		thread threadObj(&P2P::listenSock, this, selfaddr.ip, selfaddr.port);
		threadObj.detach();
		while (true) {
			string id;
			//cout << "wait for answer" << endl;
			auto recv = signalsock.srecv(1024);
			//cout << recv << endl;
			SSocket sock(AF_INET, SOCK_STREAM);
			sock.ssetsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
			for (auto ii : split(recv, ";;")) {
				//cout << "in ii for" << endl;
				for (auto i : split(ii, ";")) {
					for (int g = 0; g < 5; g++) {
						try {
							sock.sconnect(split(i, ":")[0], stoi(split(i, ":")[1]));
							//cout << "connect?" << endl;
							auto data = sock.srecv(1024);
							//cout << data << endl;
							auto tmp = split(data, ";");
							if (tmp[0] == "connected") {
								sock.ssend(myid);
								sock.srecv(1024);
								//cout << "connected!" << endl;
								id = tmp[1];
								break;
							}
						}
						catch (string e) {
							//cout << "exception: " << e << endl;
							if (e == "10056") { break; }
						}
					}
				}
				uuidpeers[id] = new SSocket(sock);
			}
		}
	}

public:
	string myid = uuid4();
	map<string, SSocket*> uuidpeers;
	P2P(string ip, int port) {
		srvaddr = ip;
		srvport = port;
	}

	

	void connect() {
		thread threadObj(&P2P::connectPeer, this);
		threadObj.detach();
		while (uuidpeers.size() == 0) {}
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));

	}

	MSG* recvmsg() {
		while (msgqueue.size() == 0) {}
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		auto data = msgqueue[0];
		msgqueue.erase(msgqueue.begin());
		return data;
	}

};