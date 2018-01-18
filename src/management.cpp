#include <../include/management.hpp>
#include <sqlite3.h>

static bool checkTableExistence(std::string dbPath, std::string tableName) {
    sqlite3 *db = nullptr;
    int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
    if (SQLiteReturn != SQLITE_OK) {
        sqlite3_close(db);
        throw std::string(sqlite3_errmsg(db));
    }
    sqlite3_stmt *res = nullptr;

    std::string sqlQuery = "SELECT count(*) "
                           "FROM sqlite_master "
                           "WHERE type='table' AND name='" + tableName + "';";

    SQLiteReturn = sqlite3_prepare_v2(db, sqlQuery.c_str(), -1, &res, 0);
    if (SQLiteReturn != SQLITE_OK) {
        sqlite3_close(db);
        throw std::string(sqlite3_errmsg(db));
    }

    sqlite3_step(res);

    int boolean = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);
    sqlite3_close(db);

    return boolean != 0;
}

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
    UpdateTime.tm_hour = 8;
    UpdateTime.tm_mday = now->tm_mday;
    UpdateTime.tm_mon = now->tm_mon;
    UpdateTime.tm_year = now->tm_year;

    time_t updateT = mktime(&UpdateTime);

    if (t > updateT) {
        if (lastUpdate < updateT) {
            return true;
        }
    }

    updateT = updateT + 26100;

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
        if (table[i][3] != "&nbsp;") {
            output += "Fach:               " + table[i][3] + "\n";
        }

        if (table[i][4] != "---") {
            output += "Raum:             " + table[i][4] + "\n";
        }
        if (table[i][5] != "&nbsp;") {
            output += "Text:               " + table[i][5] + "\n";
        }
        output += "\n";
    }

    return output;
}

void VertretungsBoy::createErrorMsg(discordpp::Bot *bot, std::string error, std::string channelID) {
    VertretungsBoy::createMsg(bot, "Ups, das hat nicht funktioniert :no_mouth: \n\n`" + error + "`", channelID);
}

void VertretungsBoy::createMsg(discordpp::Bot *bot, std::string msg, std::string channelID) {
    bot->call("/channels/" + channelID + "/messages", {{"content", msg}}, "POST");
}

std::string VertretungsBoy::getLastSearch(std::string dbPath, std::string userID){
    if(checkTableExistence(dbPath, "searchesRequests")) {
        sqlite3 *db = nullptr;
        int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
        if(SQLiteReturn != SQLITE_OK) {
            sqlite3_close(db);
            throw std::string(sqlite3_errmsg(db));
        }
        sqlite3_stmt *res = nullptr;

        std::string sqlQuery = "SELECT searches "
                               "FROM searchesRequests "
                               "WHERE userID = " + userID + ";";

        SQLiteReturn = sqlite3_prepare_v2(db, sqlQuery.c_str(), -1, &res, 0);
        if (SQLiteReturn != SQLITE_OK) {
            sqlite3_finalize(res);
            sqlite3_close(db);
            throw std::string(sqlite3_errmsg(db));
        }

        std::string entry;

        SQLiteReturn = sqlite3_step(res);
        if(SQLiteReturn == SQLITE_ROW) {
            entry = reinterpret_cast<const char *>(sqlite3_column_text(res, 0));
        }

        sqlite3_finalize(res);
        sqlite3_close(db);

        if(!entry.empty()) {
            return entry;
        } else {
            throw std::string("no last search");
        }

    } else {
        throw std::string("no last search");
    }
}

void VertretungsBoy::saveSearch(std::string dbPath, std::string userID, std::string searchValue) {
    sqlite3 *db = nullptr;
    int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
    if(SQLiteReturn != SQLITE_OK) {
        sqlite3_close(db);
        throw std::string(sqlite3_errmsg(db));
    }

    searchValue = sqlite3_mprintf("%Q", searchValue.c_str());

    std::string sqlQuery = "CREATE TABLE IF NOT EXISTS `searchesRequests` ("
                           "`userID`INTEGER UNIQUE,"
                           "`searches`TEXT"
                           ");"

                           "REPLACE INTO searchesRequests "
                           "VALUES (" + userID + ", " + searchValue + ")";

    SQLiteReturn = sqlite3_exec(db, sqlQuery.c_str(), nullptr, nullptr, nullptr);
    sqlite3_close(db);
    if(SQLiteReturn != SQLITE_OK) {
        throw std::string(sqlite3_errmsg(db));
    }
}

std::string VertretungsBoy::time_tToString(time_t t){
    if(!t) {
        return std::string ("unbekannt");
    }

    struct tm *time = localtime(&t);
    std::string dateString = std::to_string(time->tm_mday) + "." + std::to_string(time->tm_mon + 1) + "." +
                        std::to_string(time->tm_year + 1900) + " um " + std::to_string(time->tm_hour) + ":";

    if(time->tm_min < 10) {
	dateString += "0" + std::to_string(time->tm_min);
    } else {
	dateString += std::to_string(time->tm_min);
    }

    return dateString;
}
