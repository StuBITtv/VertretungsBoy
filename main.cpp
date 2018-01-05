#define TOKEN "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs" //THIS IS NOT A VALIDE TOKEN
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

            if (!arg.empty()) {
                std::vector<std::string> urls{"dbg-metzingen.de/vertretungsplan/tage/subst_001.htm",
                                              "dbg-metzingen.de/vertretungsplan/tage/subst_002.htm"};
                VertretungsBoy::plan plan(urls, DBPATH, true, 0, 10);

                if (arg[0] == "info") {
                    if (VertretungsBoy::needsUpdate(plan.getDateOfLastUpdate())) {
                        try {
                            plan.update();
                        } catch (std::string error) {
                            VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                            return;
                        }
                    }
                    std::vector<std::string> dates;

                    try {
                        dates = plan.getDates();
                    } catch (std::string error) {
                        VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                        return;
                    }

                    std::string output;

                    for (size_t i = 0; i < dates.size(); i++) {
                        if (dates[i] != "OUTDATED") {
                            std::vector<std::vector<std::string>> buffer;

                            if (arg.size() > 1) {
                                if(arg[1][0] == 'k') {
                                    arg[1][0] = 'K';
                                }

                                try {
                                    buffer = plan.getEntries(i, arg[1]);
                                } catch (std::string error) {
                                    VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                                    return;
                                }

                                try {
                                    VertretungsBoy::saveSearch(DBPATH, msg["author"]["id"], arg[1]);
                                } catch (std::string error) {
                                    VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                                    return;
                                }

                            } else {
                                try {
                                    buffer = plan.getEntries(i, VertretungsBoy::getLastSearch(DBPATH, msg["author"]["id"]));
                                } catch (std::string error) {
                                    VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                                    return;
                                }
                            }
                            if (!buffer.empty()) {
                                output += "**__" + dates[i].substr(0, dates[i].find(" ")) + "__**\n\n";
                                output += VertretungsBoy::createEntriesString(buffer) + "\n";
                            } else {
                                output = "Nope, nichts da :neutral_face:";
                            }
                        }
                    }
                    VertretungsBoy::createMsg(bot, output, msg["channel_id"]);

                } else if (arg[0] == "update") {
                    try {
                        plan.update();
                    } catch (std::string error) {
                        VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                        return;
                    }
                    VertretungsBoy::createMsg(bot, "Fertig, alles aktualisiert :blush:", msg["channel_id"]);

                } else if (arg[0] == "date" || arg[0] == "dates") {
                    if (VertretungsBoy::needsUpdate(plan.getDateOfLastUpdate())) {
                        try {
                            plan.update();
                        } catch (std::string error) {
                            VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                            return;
                        }
                    }

                    std::vector<std::string> dates;

                    try {
                        dates = plan.getDates();
                    } catch (std::string error) {
                        VertretungsBoy::createErrorMsg(bot, error, msg["channel_id"]);
                        return;
                    }

                    if (!dates.empty()) {
                        std::string datesString;

                        for (size_t i = 0; i < dates.size(); i++) {
                            if (dates[i] != "OUTDATED") {
                                datesString += dates[i] + "\n";
                            }
                        }

                        VertretungsBoy::createMsg(bot, datesString, msg["channel_id"]);
                    }

                } else {
                    VertretungsBoy::createMsg(bot, "Neee, das ist definitiv kein Befehl... :thinking:", msg["channel_id"]);
                }
            }
        }
    });

    aios->run();

    return 0;
}
