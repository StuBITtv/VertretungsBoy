#define TOKEN "Bot TOKEN"
#define DBPATH ".VertretungsBoy.db"

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
            TOKEN,
            std::make_shared<discordpp::RestCurlPPModule>(aios, TOKEN),
            std::make_shared<discordpp::WebsocketWebsocketPPModule>(aios,  TOKEN));

    bot.addHandler("MESSAGE_CREATE", [](discordpp::Bot *bot, json msg) {
        std::string content = msg["content"];
        if (content.substr(0, 2) == ">>") {
            content.erase(0, 2);
            std::vector<std::string> arg = VertretungsBoy::parseMessage(content);

			std::cout << "command received: " << content << std::endl << std::endl;
	
            if (!arg.empty()) {
                std::vector<std::string> urls{"https://dbg-metzingen.de/vertretungsplan/tage/subst_001.htm",
                                              "https://dbg-metzingen.de/vertretungsplan/tage/subst_002.htm"};
                VertretungsBoy::plan plan(urls, DBPATH, true, 0, 10);

                if (arg[0] == "info" || arg[0] == "i") {
                    if (VertretungsBoy::needsUpdate(plan.getDateOfLastUpdate())) {
                        try {
                            plan.update();
                        } catch (std::string &error) {
                            VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                            return;
                        }
                    }
                    std::vector<std::string> dates;

                    try {
                        dates = plan.getDates();
                    } catch (std::string &error) {
                        VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                        return;
                    }

                    std::string output;

                    if (arg.size() == 1) {
                        try {
                            arg.push_back(VertretungsBoy::getLastSearch(DBPATH, msg["author"]["id"]));
                        } catch (std::string &error) {
                            VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                            return;

                        }
                    }

                    for (size_t i = 0; i < dates.size(); i++) {
                        if (dates[i] != "OUTDATED") {
                            std::vector<std::vector<std::string>> buffer;

                            if (arg.size() > 1) {
                                if (arg[1][0] == 'k') {
                                    arg[1][0] = 'K';
                                }

                                try {
                                    VertretungsBoy::saveSearch(DBPATH, msg["author"]["id"], arg[1]);
                                } catch (std::string &error) {
                                    VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                                    return;
                                }
                            }

                            try {
								buffer = plan.getEntries(i, arg[1]);
                            } catch (std::string &error) {
								VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
								return;
                            }

                            if (!buffer.empty()) {
                                output += "**__" + dates[i].substr(0, dates[i].find(' ')) + "__**\n\n";
                                output += VertretungsBoy::createEntriesString(buffer) + "\n";
                            }
                        }
                    }

                    if(output.empty()) {
                        output = "Nope, nichts da für `" + arg[1] + "` :neutral_face:";
                    }
                    
                    if(output.size() > 1999) {
						VertretungsBoy::createErrorMsg(bot, "message for " + arg[1] + " to big and developer to lazy to add workaround, sorry :(", msg["channel_id"]);
						return;
					}
					
                    VertretungsBoy::createMsg(bot, output, msg["channel_id"]);

                } else if (arg[0] == "update" || arg[0] == "u") {
                    try {
                        plan.update();
                    } catch (std::string &error) {
                        VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                        return;
                    }
                    VertretungsBoy::createMsg(bot, "Fertig, alles aktualisiert :blush:", msg["channel_id"]);

                } else if (arg[0] == "date" || arg[0] == "d") {
                    if (VertretungsBoy::needsUpdate(plan.getDateOfLastUpdate())) {
                        try {
                            plan.update();
                        } catch (std::string &error) {
                            VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                            return;
                        }
                    }

                    std::vector<std::string> dates;

                    try {
                        dates = plan.getDates();
                    } catch (std::string &error) {
                        VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                        return;
                    }

                    std::string datesString;

                    if (!dates.empty()) {


                        for (const auto &date : dates) {
                            if (date != "OUTDATED") {
                                datesString += date + "\n";
                            }
                        }
                    }

                    try {
                        datesString += "\nZuletzt aktualisiert am " + VertretungsBoy::time_tToString(plan.getDateOfLastUpdate());
                    } catch (std::string &error) {
                        VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                        return;
                    }

                    VertretungsBoy::createMsg(bot, datesString, msg["channel_id"]);
                } else if (arg[0] == "help" || arg[0] == "h"){
                    VertretungsBoy::createMsg(bot, "__**Befehle:**__\n"
                                                   "\n"
                                                   "Befehle werden durch `>>` am Anfang gekennzeichnet, alle weitere Parameter werden durch Leerzeichen getrennt. Groß- und Kleinschreibung wird nicht beachtet\n"
                                                   "\n"
                                                   "`i` oder `info`:\n"
                                                   "Zeigt die Einträge für die Klasse in der Datenbank. Als Paramter kann eine Klasse angegeben werden, ansonsten wird die zuletzte angegebene Klasse verwendet\n"
                                                   "\n"
                                                   "`u` oder `update`\n"
                                                   "Aktualisiert die Datenbank. Sollte nur bei Fehlern in der Datenbank aufgerufen werden, da sich die Datenbank ansonst automatisch aktualisiert\n"
                                                   "\n"
                                                   "`d` oder `date`\n"
                                                   "Zeigt die Daten zu den Datums in der Datenbank\n"
                                                   "\n"
                                                   "`h` oder `help`\n"
                                                   "Zeigt diese Hilfe, offensichtlich :joy:",
                                              msg["channel_id"]);
                } else {
                    VertretungsBoy::createMsg(bot, "Neee, das ist definitiv kein Befehl...\n"
                                                   "Versuch mal `h` oder `help` für die Hilfe :thinking: ",
                                              msg["channel_id"]);
                }
            }
        }
    });

    aios->run();

    return 0;
}
