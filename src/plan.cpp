//
// Created by StuBIT on 12/20/17.
//

#include "../include/plan.hpp"
#include <curl/curl.h>

bool VertretungsBoy::plan::curlGlobalInit = false;

VertretungsBoy::plan::plan(std::vector<std::string> urls, std::string dbPath, bool skipOutdated, size_t indexStart,
                           long long int timeout)
        : urls(urls), timeout(timeout), dbPath(dbPath), skipOutdated(skipOutdated), indexStart(indexStart) {
    if (!curlGlobalInit) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curlGlobalInit = true;
    }

    htmls.resize(urls.size());

    sqlite3 *db = nullptr;
    int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
    sqlite3_close(db);
    if (SQLiteReturn != SQLITE_OK) {
        throw std::string(sqlite3_errmsg(db));
    }
}

void VertretungsBoy::plan::update() {
    for (size_t i = 0; i < urls.size(); ++i) {
        download(i);
        writeTableToDB(i, std::move(parser(htmls[i])));
    }

    writeDatesToDB();
}

void VertretungsBoy::plan::download(size_t urlsIndex) {
    CURL *handle = curl_easy_init();
    if (handle == nullptr) {
        throw "Failed to init curl handle";
    }

    curl_easy_setopt(handle, CURLOPT_URL, urls[urlsIndex].c_str());
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlWriter);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &htmls[urlsIndex]);

    CURLcode res = curl_easy_perform(handle);
    if (res != CURLE_OK) {
        throw "Failed to perform download";
    }

    curl_easy_cleanup(handle);
}

size_t VertretungsBoy::plan::curlWriter(char *ptr, size_t size, size_t n, void *userData) {
    ((std::string *) userData)->append(ptr, 0, size * n);
    return size * n;
}

std::vector<std::vector<std::string>> VertretungsBoy::plan::parser(const std::string &html) {
    bool row = false, data = false, date = false;
    std::string dataBuffer;
    std::vector<std::string> rowBuffer;
    std::vector<std::vector<std::string>> table;

    for (size_t i = 0; i < html.size(); ++i) {
        if (html[i] == '<') {
            std::string element = html.substr(i + 1, 3);
            if (!row) {
                if (html.substr(i + 1, 14) == "tr class='list") {
                    i = i + 17;
                    row = true;
                } else if (element == "div") {
                    i = i + 20;
                    date = true;
                } else if (element == "/di") {
                    if (skipOutdated) {
                        if (!upToDate(dataBuffer)) {
                            dates.push_back("OUTDATED");
                            return table;
                        }
                    }
                    dates.push_back(dataBuffer);
                    dataBuffer = "";
                    date = false;
                }
                i = i + 3;
            } else {
                if (!data) {
                    if (element == "/tr") {
                        if (!rowBuffer.empty()) {
                            table.push_back(rowBuffer);
                            rowBuffer.clear();
                        }

                        i = i + 4;
                        row = false;

                    } else if (element == "td ") {
                        data = true;
                        i = i + 15;

                        while (html[i] != '>') {
                            i++;
                        }
                        i++;
                    }
                } else {
                    if (element == "/td") {
                        rowBuffer.push_back(dataBuffer);
                        dataBuffer = "";

                        i = i + 4;
                        data = false;
                        replace = false;
                        replaceCounter = 0;
                    }
                }
            }
        }

        if (data || date) {
            tableWriter(toUTF8(html[i]), dataBuffer);
        }
    }

    return table;
}

void VertretungsBoy::plan::tableWriter(const std::string &tokens, std::string &output) {
    if (tokens == "<") {
        styleElement = true;
    }

    if (!styleElement && tokens == "?") {
        output = " statt " + output;
        replace = true;
        return;
    }

    if (!styleElement) {
        if (!replace) {
            output += tokens;
        } else {
            output.insert(replaceCounter, tokens);
            if(tokens == "Ö" || tokens == "ö" ||
               tokens == "Ä" || tokens == "ä" ||
               tokens == "Ü" || tokens == "ü" ||
               tokens == "ß") {
                replaceCounter++;
            }
            replaceCounter++;
        }
    }

    if (styleElement && tokens == ">") {
        styleElement = false;
    }
}

std::string VertretungsBoy::plan::toUTF8(char token) {
    if (token == '\xD6') {
        return "Ö";
    } else if (token == '\xF6') {
        return "ö";
    } else if (token == '\xC4') {
        return "Ä";
    } else if (token == '\xE4') {
        return "ä";
    } else if (token == '\xDC') {
        return "Ü";
    } else if (token == '\xFC') {
        return "ü";
    } else if (token == '\xDF') {
        return "ß";
    } else {
        std::string strToken;
        strToken += token;
        return strToken;
    }
}

void VertretungsBoy::plan::writeTableToDB(size_t tableID, std::vector<std::vector<std::string>> table) {
    std::string sqlQuery = "DROP TABLE IF EXISTS backupPlan" + std::to_string(tableID + indexStart) + ";"

                           "CREATE TABLE  IF NOT EXISTS `backupPlan" + std::to_string(tableID + indexStart) + "` ("
                           "`classes` TEXT,"
                           "`lessons` TEXT,"
                           "`type` TEXT,"
                           "`subjects` TEXT,"
                           "`rooms` TEXT,"
                           "`text` TEXT"
                           ");";

    if (!table.empty()) {
        sqlQuery += "INSERT INTO `backupPlan" + std::to_string(tableID + indexStart) +
                    "`(`classes`,`lessons`,`type`,`subjects`,`rooms`,`text`) VALUES ";

        for (size_t j = 0; j < table.size(); j++) {
            if (table[j][3] != table[j][4] && (table[j][4] != "---" && table[j][4] != "&nbsp;")) {
                table[j][3].insert(0, table[j][4] + " statt ");
            }

            sqlQuery += "('" + table[j][0] + "', '" + table[j][1] + "','"
                        + table[j][2] + "', '" + table[j][3] + "','"
                        + table[j][5] + "', '" + table[j][6] + "')";

            if (j + 1 != table.size()) {
                if (table[j + 1][0] == "&nbsp;" &&
                    table[j + 1][1] == "&nbsp;" &&
                    table[j + 1][2] == "&nbsp;" &&
                    table[j + 1][3] == "&nbsp;" &&
                    table[j + 1][4] == "&nbsp;" &&
                    table[j + 1][5] == "&nbsp;" &&
                    table[j + 1][6] != "&nbsp;") {
                    sqlQuery.insert(sqlQuery.find_last_of('\''), " " + table[j + 1][5]);
                    j++;
                } else if (table[j + 1][0] == "&nbsp;") {
                    table[j + 1][0] = table[j][0];
                }
            }

            if(j + 1 != table.size()) {
                sqlQuery += ", ";
            } else {
                sqlQuery += ";";
            }
        }
    }

    sqlite3 *db = nullptr;
    int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
    if (SQLiteReturn != SQLITE_OK) {
        sqlite3_close(db);
        throw std::string(sqlite3_errmsg(db));
    }

    SQLiteReturn = sqlite3_exec(db, sqlQuery.c_str(), nullptr, nullptr, nullptr);
    sqlite3_close(db);
    if (SQLiteReturn != SQLITE_OK) {
        throw std::string(sqlite3_errmsg(db));
    }
}

std::vector<std::vector<std::string>> VertretungsBoy::plan::getEntries(size_t tableID, std::string searchValue) {
    if (urls.size() - 1 < tableID) {
        throw std::string("Not a valid table number");
    }

    if (dates.empty()) {
        getDates();
    }

    if (dates[tableID] == "OUTDATED") {
        return std::vector<std::vector<std::string>>();
    }

    if (!checkTableExistence("backupPlan" + std::to_string(tableID + indexStart))) {
        update();
    }

    searchValue = sqlite3_mprintf("%Q", searchValue.c_str());
    searchValue.erase(searchValue.begin(), searchValue.begin() + 1);
    searchValue.erase(searchValue.end() - 1, searchValue.end());

    sqlite3_stmt *res;
    std::string sqlQuery = "SELECT * "
                           "FROM backupPlan" + std::to_string(tableID + indexStart) + " "
                           "WHERE classes LIKE '%" + searchValue + "%'";

    sqlite3 *db = nullptr;
    int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
    if (SQLiteReturn != SQLITE_OK) {
        sqlite3_close(db);
        throw std::string(sqlite3_errmsg(db));
    }

    SQLiteReturn = sqlite3_prepare_v2(db, sqlQuery.c_str(), -1, &res, 0);
    if (SQLiteReturn != SQLITE_OK) {
        sqlite3_close(db);
        throw std::string(sqlite3_errmsg(db));
    }

    std::vector<std::vector<std::string>> table;

    do {
        SQLiteReturn = sqlite3_step(res);
        if (SQLiteReturn == SQLITE_ROW) {
            std::vector<std::string> row(6);
            for (auto i = 0; i < 6; i++) {
                row[i] = reinterpret_cast<const char *>(sqlite3_column_text(res, i));
            }

            table.push_back(row);
        }
    } while (SQLiteReturn == SQLITE_ROW);

    sqlite3_finalize(res);
    sqlite3_close(db);

    return table;
}

void VertretungsBoy::plan::writeDatesToDB() {
    std::string sqlQuery;

    if (!checkTableExistence("tableDates")) {
        sqlQuery = "CREATE TABLE `tableDates` ("
                   "`tableID` INTEGER UNIQUE,"
                   "`dates` TEXT"
                   ");";

        sqlite3 *db = nullptr;
        int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
        if (SQLiteReturn != SQLITE_OK) {
            sqlite3_close(db);
            throw std::string(sqlite3_errmsg(db));
        }

        SQLiteReturn = sqlite3_exec(db, sqlQuery.c_str(), nullptr, nullptr, nullptr);
        if (SQLiteReturn != SQLITE_OK) {
            sqlite3_close(db);
            throw std::string(sqlite3_errmsg(db));
        }

        sqlite3_close(db);
    }


    sqlQuery = "REPLACE INTO tableDates "
               "VALUES ";

    for (size_t i = 0; i < dates.size(); i++) {
        sqlQuery += "(" + std::to_string(i + indexStart) + ", '" + dates[i] + "'), ";
    }

    lastUpdate = time(0);
    sqlQuery += "(-1, '" + std::to_string(lastUpdate) + "');";

    sqlite3 *db = nullptr;
    int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
    if (SQLiteReturn != SQLITE_OK) {
        sqlite3_close(db);
        throw std::string(sqlite3_errmsg(db));
    }

    SQLiteReturn = sqlite3_exec(db, sqlQuery.c_str(), nullptr, nullptr, nullptr);
    sqlite3_close(db);
    if (SQLiteReturn != SQLITE_OK) {
        throw std::string(sqlite3_errmsg(db));
    }
}

std::vector<std::string> VertretungsBoy::plan::getDates() {
    if (dates.empty()) {
        if (!checkTableExistence("tableDates")) {
            update();
        } else {
            sqlite3_stmt *res = nullptr;

            sqlite3 *db = nullptr;
            int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
            if (SQLiteReturn != SQLITE_OK) {
                sqlite3_close(db);
                throw std::string(sqlite3_errmsg(db));
            }

            std::string sqlQuery = "SELECT * FROM tableDates "
                                   "WHERE tableID "
                                   "BETWEEN " + std::to_string(indexStart) + " AND "
                                   + std::to_string(indexStart + urls.size() - 1) + ";";

            SQLiteReturn = sqlite3_prepare_v2(db, sqlQuery.c_str(), -1, &res, nullptr);
            if (SQLiteReturn != SQLITE_OK) {
                sqlite3_finalize(res);
                sqlite3_close(db);
                throw std::string(sqlite3_errmsg(db));
            }

            do {
                SQLiteReturn = sqlite3_step(res);
                if (SQLiteReturn == SQLITE_ROW) {
                    std::string date = reinterpret_cast<const char *>(sqlite3_column_text(res, 1));
                    if (skipOutdated) {
                        if (upToDate(date)) {
                            dates.push_back(date);
                        } else {
                            dates.push_back("OUTDATED");
                        }
                    } else {
                        dates.push_back(date);
                    }
                }
            } while (SQLiteReturn == SQLITE_ROW);

            sqlite3_finalize(res);
            sqlite3_close(db);

        }

        if (dates.size() != urls.size()) {
            dates.clear();
            update();
        }
    }

    return dates;
}

bool VertretungsBoy::plan::checkTableExistence(std::string tableName) {
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

bool VertretungsBoy::plan::upToDate(const std::string &date) {

    if (date == "OUTDATED") {
        return false;
    }

    time_t now = time(0);
    struct tm plan;

    size_t firstDot = date.find(".") + 1;
    size_t lastDot = date.find_last_of(".") + 1;

    plan.tm_sec = 59;
    plan.tm_min = 59;
    plan.tm_hour = 23;

    plan.tm_mday = std::stoi(date.substr(0, firstDot - 1));
    plan.tm_mon = std::stoi(date.substr(firstDot, lastDot - firstDot)) - 1;
    plan.tm_year = std::stoi(date.substr(lastDot, date.find(" ") - lastDot)) - 1900;

    time_t planT = mktime(&plan);

    return planT > now;
}

time_t VertretungsBoy::plan::getDateOfLastUpdate() {
    if (!lastUpdate) {
        if (checkTableExistence("tableDates")) {
            sqlite3 *db = nullptr;
            int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
            if (SQLiteReturn != SQLITE_OK) {
                sqlite3_close(db);
                throw std::string(sqlite3_errmsg(db));
            }

            sqlite3_stmt *res = nullptr;
            SQLiteReturn = sqlite3_prepare_v2(db,
                                              "SELECT dates "
                                              "FROM tableDates "
                                              "WHERE tableID = -1",
                                              -1, &res, nullptr);

            if (SQLiteReturn != SQLITE_OK) {
                sqlite3_close(db);
                throw std::string(sqlite3_errmsg(db));
            }

            SQLiteReturn = sqlite3_step(res);
            if (SQLiteReturn == SQLITE_ROW) {
                lastUpdate = sqlite3_column_int(res, 0);
            }

            sqlite3_finalize(res);
            sqlite3_close(db);
        }
    }

    return lastUpdate;
}
