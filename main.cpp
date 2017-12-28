#include <iostream>
#include "plan.hpp"

int main() {

    std::string lookup;
    std::cin >> lookup;

    std::vector<std::string> urls { "dbg-metzingen.de/vertretungsplan/tage/subst_001.htm", "dbg-metzingen.de/vertretungsplan/tage/subst_002.htm" };
    VertretungsBoy::plan test(urls, 10, ".planBackup.db");

    test.update();
    std::vector<std::string> dates = test.getDates();


    for(size_t k = 0; k < urls.size(); k ++) {

        std::cout << dates[k] << std::endl << std::endl;
        std::vector<std::vector<std::string>> entries = test.getEntries(k, lookup);
        for (size_t i = 0; i < entries.size(); i++) {
            for (size_t j = 0; j < entries[i].size(); j++) {
                std::cout << entries[i][j] << "   ";
            }

            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    return  0;
}