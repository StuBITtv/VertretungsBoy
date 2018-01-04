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
#include "plan.hpp"

using json = nlohmann::json;
using aios_ptr = std::shared_ptr<asio::io_service>;

int main() {

    aios_ptr aios = std::make_shared<asio::io_service>();

    discordpp::Bot bot(
            aios,
            "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs",
            std::make_shared<discordpp::RestCurlPPModule>(aios, "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs"),
            std::make_shared<discordpp::WebsocketWebsocketPPModule>(aios, "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs")
    );

    bot.addHandler("MESSAGE_CREATE", [](discordpp::Bot* bot, json msg){
		std::string content = msg["content"];
		if(content[0] == '*'){
			content.erase(0, 1);
			std::vector<std::string> arg = VertretungsBoy::parseMessage(content);
			
			for(size_t i = 0; i < arg.size(); i++) {
				std::cout << arg[i] << std::endl;
			}
			
			if(!arg.empty()) {
				std::vector<std::string> urls {"dbg-metzingen.de/vertretungsplan/tage/subst_001.htm", "dbg-metzingen.de/vertretungsplan/tage/subst_002.htm"};
				VertretungsBoy::plan plan(urls, ".VertretungsBoy.db", true, 0, 10);
				
				if(arg[0] == "info") {
					if(arg.size() > 1) {
						// specific
					} else {
						// last
					}
					
				} else if (arg[0] == "update") {
					
					
					try {
							plan.update();
					} catch (std::string error) {
							 bot->call(
								 "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
								 {{"content", "Oh, da ist wohl etwas schief gelaufen :disappointed: (" + error + ")"}},
								 "POST"
							 );
							 
							 return -1;	
					}
					
					bot->call(
							  "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
							  {{"content", "Fertig, alles aktualisiert :blush:"}},
							  "POST"
							 );	

				} else if (arg[0] == "date" || arg[0] == "dates") {

					if(VertretungsBoy::needsUpdate(plan.getDateOfLastUpdate())) {
						try {
								plan.update();
						} catch (std::string error) {
								 bot->call(
									 "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
									 {{"content", "Oh, da ist wohl etwas schief gelaufen :disappointed: (" + error + ")"}},
									 "POST"
								 );	
								 
								 return -1;
						}
					}
					
					std::vector<std::string> dates;
					
					try {
							dates = plan.getDates();
					} catch (std::string error) {
							 bot->call(
								 "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
								 {{"content", "Oh, da ist wohl etwas schief gelaufen :disappointed: (" + error + ")"}},
								 "POST"
							 );	
						return -1;
					}
					
					if(!dates.empty()) {
						
						std::string datesString;
						
						for(size_t i = 0; i < dates.size(); i++) {
							if(dates[i] != "OUTDATED") {
								datesString += dates[i] + "\n";
							}
						}
						
						bot->call(
							 "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
							 {{"content", datesString}},
							 "POST"
						);	
					}
					
				} else {
					bot->call(
								"/channels/" + msg["channel_id"].get<std::string>() + "/messages",
								{{"content", "Ne, das ist definitiv kein Befehl... :thinking:"}},
								"POST"
                    );
				}
			}
		}
    });

    aios->run();

    return 0;
}
