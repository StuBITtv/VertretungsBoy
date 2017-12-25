//
// Created by StuBIT on 12/20/17.
//

#include "plan.hpp"
#include <curl/curl.h>

bool VertretungsBoy::plan::curlGlobalInit = false;

static int pushToTable(void *table, int argc, char **argv, char **notUsed) {
    std::vector<std::string> row;
    for(size_t i = 0; i < argc; i++) {
        std::string toCpp(argv[i]);
        row.push_back(toCpp);
    }

    static_cast<std::vector<std::vector<std::string>> *>(table)->push_back(row);
    return 0;
}

VertretungsBoy::plan::plan(std::vector<std::string> urls, std::string dbPath) : urls(urls), dbPath(dbPath) {
    if(!curlGlobalInit){
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curlGlobalInit = true;
    }

    htmls.resize(urls.size());
    tables.resize(urls.size());

    int SQLiteReturn = sqlite3_open(dbPath.c_str(), &db);
    if(!SQLiteReturn) {
        for (size_t i = 0; i < urls.size(); ++i) {
            std::vector<std::vector<std::string>> table;
            std::string sqlQuery = "SELECT count(*) "
                                   "FROM sqlite_master "
                                   "WHERE type = 'table' "
                                   "AND name = 'backupPlan"  + std::to_string(i) + "';";
            SQLiteReturn = sqlite3_exec(db, sqlQuery.c_str(), pushToTable, &table, nullptr);
            if (SQLiteReturn != SQLITE_OK) {
                sqlite3_close(db);
                throw SQLiteReturn;
            }

            if(std::stoi(table[0][0])) {
                sqlQuery = "SELECT *"
                           "FROM backupPlan" + std::to_string(i) + ";";
                SQLiteReturn = sqlite3_exec(db, sqlQuery.c_str(), pushToTable, &table, nullptr);
                if (SQLiteReturn != SQLITE_OK) {
                    sqlite3_close(db);
                    throw SQLiteReturn;
                }

                tables.push_back(table);
            }
        }
    } else {
        sqlite3_close(db);
        throw SQLiteReturn;
    }
    sqlite3_close(db);
}

int VertretungsBoy::plan::update(){
    for(size_t i = 0; i<urls.size(); ++i){
        if(!download(i))
            return 1;
        tables[i] = parser(htmls[i]);
    }

    return writeTablesToDatabase();
}

bool VertretungsBoy::plan::download(size_t urlsIndex) {
    CURL *handle = curl_easy_init();
    if(handle == nullptr)
        return  1;

    curl_easy_setopt(handle, CURLOPT_URL, urls[urlsIndex].c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlWriter);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &htmls[urlsIndex]);

    CURLcode res = curl_easy_perform(handle);
    if(res != CURLE_OK)
        return false;

    curl_easy_cleanup(handle);

    return true;
}

size_t VertretungsBoy::plan::curlWriter(char *ptr, size_t size, size_t n, void *userData) {
    ((std::string *) userData) -> append(ptr, 0, size * n);
    return size * n;
}

std::vector<std::vector<std::string>> VertretungsBoy::plan::parser(const std::string &html) {
    bool row = false, data = false, date = false;
    std::string dataBuffer;
    std::vector<std::string> rowBuffer;
    std::vector<std::vector<std::string>> table;

    for(size_t i = 0; i < html.size(); ++i) {
        if(html[i] == '<') {
            std::string element = html.substr(i+1, 3);
            if(!row) {
                if(html.substr(i+1, 14) == "tr class='list") {
                    i = i + 17;
                    row = true;
                } else if (element == "div") {
                    i = i + 20;
                    date = true;
                } else if (element == "/di") {
                    dates.push_back(dataBuffer);
                    dataBuffer = "";
                    date = false;
                }
                i = i + 3;
            } else {
                if(!data) {
                    if(element == "/tr") {
                        if(!rowBuffer.empty()) {
                            table.push_back(rowBuffer);
                            rowBuffer.clear();
                        }
                        i = i + 4;
                        row = false;
                    } else if (element == "td ") {
                        data = true;
                        i = i + 31;
                        while (html[i] != '>')
                            i++;
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

        if(data||date)
            tableWriter(toUTF8(html[i]), dataBuffer);
    }

    return table;
}

void VertretungsBoy::plan::tableWriter(std::string tokens, std::string &output) {
    if(tokens == "<")
        styleElement = true;

    if(!styleElement && tokens == "?") {
        output = " statt " + output;
        replace = true;
        return;
    }

    if(!styleElement) {
        if (!replace) {
            output += tokens;
        }
        else {
            output.insert(replaceCounter, tokens);
            replaceCounter++;
        }
    }

    if (styleElement && tokens == ">")
        styleElement = false;
}

std::string VertretungsBoy::plan::toUTF8(char token) {
    if (token == '\xD6')
        return "Ö";
    else if (token == '\xF6')
        return "ö";
    else if (token == '\xC4')
        return "Ä";
    else if (token == '\xE4')
        return "ä";
    else if (token == '\xDC')
        return "Ü";
    else if (token == '\xFC')
        return "ü";
    else if (token == '\xDF')
        return "ß";
    else {
        std::string strToken;
        strToken += token;
        return strToken;
    }
}

int VertretungsBoy::plan::writeTablesToDatabase() {
    for (size_t i = 0; i < tables.size(); i++) {
        std::string sqlQuery = "DROP TABLE IF EXISTS backupPlan" + std::to_string(i) + ";"

                               "CREATE TABLE  IF NOT EXISTS `backupPlan" + std::to_string(i) + "` ("
                               "`classes` TEXT,"
                               "`lessons` TEXT,"
                               "`type` TEXT,"
                               "`subjects` TEXT,"
                               "`rooms` TEXT,"
                               "`text` TEXT"
                               ");"

                               "INSERT INTO `backupPlan" + std::to_string(i) + "`(`classes`,`lessons`,`type`,`subjects`,`rooms`,`text`) VALUES ";

        for (size_t j = 0; j < tables[i].size(); j++) {
            sqlQuery += "('" + tables[i][j][0] + "', '" + tables[i][j][1] + "','"
                            + tables[i][j][2] + "', '" + tables[i][j][3] + "','"
                            + tables[i][j][4] + "', '" + tables[i][j][5] + "')";
            if(j + 1 != tables[i].size())
                sqlQuery += ", ";
            else
                sqlQuery += ";";

        }

        sqlite3_open(dbPath.c_str(), &db);
        int SQLiteReturn = sqlite3_exec(db, sqlQuery.c_str(), nullptr, nullptr, nullptr);
        sqlite3_close(db);
        if(SQLiteReturn != SQLITE_OK)
            return SQLiteReturn;
    }
    return 0;
}
