#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <string>
#include <thread>
#include "strlib.hpp"
#include <chrono>
#include "nlohmann/json.hpp"
using namespace std;
using nlohmann::json;
string url = "https://api.telegram.org";
string token;

class Message {
    string tmp;
    public:

        struct {
            int msg_id;
            int date;
            string text;
        } message;

        struct {
            int id;
            bool is_bot;
            string first_name;
            string last_name;
            string username;
            string language_code;
        } author;

        struct {
            int id;
            string first_name;
            string last_name;
            string username;
            string type;
        } chat;

        void send(string sendtext) {
            httplib::Client cli(url);
            while (sendtext.find("&") != string::npos) { sendtext.replace(sendtext.find("&"), 1, "%26"); }
            tmp = format("/bot%s/sendMessage?chat_id=%d&text=%s", token.c_str(), chat.id, sendtext.c_str());
            cli.Get(tmp.c_str());
        }
};


class Telebot {
    protected:
        string update = "";
        string oldupdate = "";
        string tmp;

        string http_update() {
            httplib::Client cli(url);
            tmp = format("/bot%s/getUpdates", token.c_str());
            return cli.Get(tmp.c_str())->body;
        }

        Message configure(json data){
            Message msg;
            msg.message.msg_id = data["result"].back()["message"]["message_id"];

            msg.message.date = data["result"].back()["message"]["date"];

            if (data["result"].back()["message"]["text"].is_null()) {msg.message.text = "";} else {msg.message.text = data["result"].back()["message"]["text"];}

            msg.author.id = data["result"].back()["message"]["from"]["id"];

            msg.author.is_bot = data["result"].back()["message"]["from"]["is_bot"];

            if (data["result"].back()["message"]["from"]["first_name"].is_null()) { msg.author.first_name = "null"; } else { msg.author.first_name = data["result"].back()["message"]["from"]["first_name"]; }

            if (data["result"].back()["message"]["from"]["last_name"].is_null()) {msg.author.last_name = "null";} else { msg.author.last_name = data["result"].back()["message"]["from"]["last_name"]; }

            if (data["result"].back()["message"]["from"]["username"].is_null()) { msg.author.username = "null"; } else { msg.author.username = data["result"].back()["message"]["from"]["username"]; }

            msg.author.language_code = data["result"].back()["message"]["from"]["language_code"];

            msg.chat.id = data["result"].back()["message"]["chat"]["id"];

            if (data["result"].back()["message"]["chat"]["first_name"].is_null()) { msg.chat.first_name = "null"; } else { msg.chat.first_name = data["result"].back()["message"]["chat"]["first_name"]; }

            if (data["result"].back()["message"]["chat"]["last_name"].is_null()){msg.chat.last_name = "null";} else {msg.author.last_name = data["result"].back()["message"]["chat"]["last_name"];}

            if (data["result"].back()["message"]["chat"]["username"].is_null()) { msg.chat.username = "null"; } else { msg.chat.username = data["result"].back()["message"]["chat"]["username"]; }

            msg.chat.type = data["result"].back()["message"]["chat"]["type"];
            return msg;
        }
    public:
        Telebot(string tk) {
            token = tk;
            oldupdate = http_update();
        }
        Message polling(){
            json j;
            while (update == oldupdate || update.length() == 0){
                update = http_update();
                if (update.length() < oldupdate.length() && update.length() > 0) {
                    oldupdate = update;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            oldupdate = update;
            return configure(j.parse(update));
        }
};