#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
namespace asio = boost::asio;

#include <discordpp/bot.hh>
#include <discordpp/rest-curlpp.hh>
#include <discordpp/websocket-websocketpp.hh>

//#include <lib/nlohmannjson/src/json.hpp>
//#include <nlohmann/json.hpp>

#include "management.hpp"

using json = nlohmann::json;
using aios_ptr = std::shared_ptr<asio::io_service>;

int main() {
    std::cout << "Starting bot...\n\n";

    aios_ptr aios = std::make_shared<asio::io_service>();

    discordpp::Bot bot(
            aios,
            "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs",
            std::make_shared<discordpp::RestCurlPPModule>(aios, "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs"),
            std::make_shared<discordpp::WebsocketWebsocketPPModule>(aios, "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs")
    );

    bot.addHandler("MESSAGE_CREATE", [](discordpp::Bot* bot, json msg){
		std::string content = msg["content"];
		if(content[0] == '$'){
			content.erase(0, 1);
			std::vector<std::string> arg = VertretungsBoy::parseMessage(content);
			
			for(size_t i = 0; i < arg.size(); i++) {
				std::cout << arg[i] << std::endl;
			}
		}
    });

    aios->run();

    return 0;
}
