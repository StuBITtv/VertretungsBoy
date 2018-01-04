#include <../include/management.hpp>

std::vector<std::string> VertretungsBoy::parseMessage(std::string &content) {
    std::string buffer;
    std::vector<std::string> arg;

    for (size_t i = 0; i < content.size(); i++) {
        if (content[i] != ' ') {
            if ((content[i] > 64 && content[i] < 91)) {
                buffer += content[i] + 32;
            } else {
                buffer += content[i];
            }
        } else {
            if (!buffer.empty()) {
                arg.push_back(buffer);
                buffer.clear();
            }
        }
    }

    if (!buffer.empty()) {
        arg.push_back(buffer);
    }

    return arg;
}

bool VertretungsBoy::needsUpdate(time_t lastUpdate) {
    time_t t = time(0);
    struct tm *now = localtime(&t);
    struct tm UpdateTime;

    if(t - lastUpdate > 63800) {
        return true;
    }

    UpdateTime.tm_sec = 0;
    UpdateTime.tm_min = 45;
    UpdateTime.tm_hour = 1;
    UpdateTime.tm_mday = now->tm_mday;
    UpdateTime.tm_mon = now->tm_mon;
    UpdateTime.tm_year = now->tm_year;

    time_t updateT = mktime(&UpdateTime);

    if (t > updateT) {
        if (lastUpdate < updateT) {
            return true;
        }
    }

    UpdateTime.tm_sec = 0;
    UpdateTime.tm_min = 15;
    UpdateTime.tm_hour = 0;
    UpdateTime.tm_mday = now->tm_mday;
    UpdateTime.tm_mon = now->tm_mon;
    UpdateTime.tm_year = now->tm_year;

    updateT = mktime(&UpdateTime);

    if (t > updateT) {
        if (lastUpdate < updateT) {
            return true;
        }
    }

    return false;
}

std::string VertretungsBoy::createEntriesString(std::vector<std::vector<std::string>> table) {
    std::string output;
    for (size_t i = 0; i < table.size(); i++) {
        output += "**Klasse(n):    " + table[i][0] + "**\n";
        output += "Stunde(n):    " + table[i][1] + "\n";
        output += "Art:                  " + table[i][2] + "\n";
        output += "Fach:               " + table[i][3] + "\n";
        if (table[i][4] != "---") {
            output += "Raum:             " + table[i][4] + "\n";
        }
        if (table[i][5] != "&nbsp;") {
            output += "Text:             " + table[i][5] + "\n";
        }
        output += "\n";
    }

    return output;
}

void VertretungsBoy::createErrorMsg(discordpp::Bot *bot, std::string error, std::string channelID) {
    VertretungsBoy::createMsg(bot, "Ups, das hat nicht funktioniert :no_mouth: *" + error + "*", channelID);
}

void VertretungsBoy::createMsg(discordpp::Bot *bot, std::string msg, std::string channelID) {
    bot->call("/channels/" + channelID + "/messages", {{"content", msg}}, "POST");
}
