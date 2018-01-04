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
            std::make_shared<discordpp::RestCurlPPModule>(aios,
                                                          "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs"),
            std::make_shared<discordpp::WebsocketWebsocketPPModule>(aios,
                                                                    "Bot Mjg0NjUzODYyMTMwODEwODgw.DSwWvQ.mInKkiDy14zyt6CCKKp-SEIBGcs")
    );

    bot.addHandler("MESSAGE_CREATE", [](discordpp::Bot *bot, json msg) {
        std::string content = msg["content"];
        if (content.substr(0, 2) == ">>") {
            content.erase(0, 2);
            std::vector<std::string> arg = VertretungsBoy::parseMessage(content);

            if (!arg.empty()) {
                std::vector<std::string> urls{"dbg-metzingen.de/vertretungsplan/tage/subst_001.htm",
                                              "dbg-metzingen.de/vertretungsplan/tage/subst_002.htm"};
                VertretungsBoy::plan plan(urls, ".VertretungsBoy.db", true, 0, 10);

                const std::string errorMsg = "Ups, das hat nicht funktioniert :no_mouth: ";

                if (arg[0] == "info") {
                    if (VertretungsBoy::needsUpdate(plan.getDateOfLastUpdate())) {
                        try {
                            plan.update();
                        } catch (std::string error) {
                            bot->call(
                                    "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
                                    {{"content", errorMsg + "*" + error + "*"}},
                                    "POST"
                            );

                            return;
                        }
                    }
                    std::vector<std::string> dates;

                    try {
                        dates = plan.getDates();
                    } catch (std::string error) {
                        bot->call(
                                "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
                                {{"content", errorMsg + "*" + error + "*"}},
                                "POST"
                        );
                        return;
                    }

                    std::string output;

                    for (size_t i = 0; i < dates.size(); i++) {
                        if (dates[i] != "OUTDATED") {
                            std::vector<std::vector<std::string>> buffer;

                            if (arg.size() > 1) {
                                buffer = plan.getEntries(i, arg[1]); // TODO catch errors
                            } else {
                                // last
                            }
                            if (!buffer.empty()) {
                                output += "**__" + dates[i].substr(0, dates[i].find(" ")) + "__**\n\n";
                                output += VertretungsBoy::createEntriesString(buffer) + "\n";
                            }
                        }
                    }

                    bot->call(
                            "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
                            {{"content", output}},
                            "POST"
                    );

                } else if (arg[0] == "update") {
                    try {
                        plan.update();
                    } catch (std::string error) {
                        bot->call(
                                "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
                                {{"content", errorMsg + "*" + error + "*"}},
                                "POST"
                        );

                        return;
                    }

                    bot->call(
                            "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
                            {{"content", "Fertig, alles aktualisiert :blush:"}},
                            "POST"
                    );

                } else if (arg[0] == "date" || arg[0] == "dates") {
                    if (VertretungsBoy::needsUpdate(plan.getDateOfLastUpdate())) {
                        try {
                            plan.update();
                        } catch (std::string error) {
                            bot->call(
                                    "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
                                    {{"content",
                                             errorMsg + "*" + error + "*"}},
                                    "POST"
                            );

                            return;
                        }
                    }

                    std::vector<std::string> dates;

                    try {
                        dates = plan.getDates();
                    } catch (std::string error) {
                        bot->call(
                                "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
                                {{"content", errorMsg + "*" + error + "*"}},
                                "POST"
                        );
                        return;
                    }

                    if (!dates.empty()) {
                        std::string datesString;

                        for (size_t i = 0; i < dates.size(); i++) {
                            if (dates[i] != "OUTDATED") {
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
                            {{"content", "Neee, das ist definitiv kein Befehl... :thinking:"}},
                            "POST"
                    );
                }
            }
        }
    });

    aios->run();

    return 0;
}
