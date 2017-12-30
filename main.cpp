#include <iostream>
#include "include/plan.hpp"

//At the moment just a demonstration of the plan class

int main() {


    std::vector<std::string> urls { "dbg-metzingen.de/vertretungsplan/tage/subst_001.htm", "dbg-metzingen.de/vertretungsplan/tage/subst_002.htm" };
    VertretungsBoy::plan plan1(urls, ".planBackup.db", true, 0, 10);
    VertretungsBoy::plan plan2(urls, ".planBackup.db", true, 2, 10);

    std::vector<std::string> dates = plan1.getDates();

    for(size_t k = 0; k < urls.size(); k ++) {

        std::cout << dates[k] << std::endl << std::endl;
        auto entries = plan1.getEntries(k, "10");
        for (size_t i = 0; i < entries.size(); i++) {
            for (size_t j = 0; j < entries[i].size(); j++) {
                std::cout << entries[i][j] << "   ";
            }

            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    dates = plan2.getDates();

    for(size_t k = 0; k < urls.size(); k ++) {

        std::cout << dates[k] << std::endl << std::endl;
        auto entries = plan2.getEntries(k, "10");
        for (size_t i = 0; i < entries.size(); i++) {
            for (size_t j = 0; j < entries[i].size(); j++) {
                std::cout << entries[i][j] << "   ";
            }

            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    time_t t = plan1.getDateOfLastUpdate();
    struct tm * now = localtime(&t);
    if(t) {
        std::cout << std::endl << "LAST UPDATED: " << now->tm_hour << ":" << now->tm_min << " "
                  << now->tm_mday << "." << now->tm_mon + 1 << "." << now->tm_year + 1900 << std::endl;
    } else {
        std::cout << std::endl << "LAST UPDATED: UNKNOWN" << std::endl;
    }
   // std::string stop;
   // std::cin >> stop;

    return  0;
}