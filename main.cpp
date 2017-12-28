#include <iostream>
#include "plan.hpp"

int main() {

    std::vector<std::string> urls { "dbg-metzingen.de/vertretungsplan/tage/subst_001.htm", "dbg-metzingen.de/vertretungsplan/tage/subst_002.htm" };
    VertretungsBoy::plan test(urls, ".planBackup.db");

    test.update();
    std::vector<std::string> dates = test.getDates();
    for(int i = 0; i < dates.size(); i++) {
        std::cout << dates[i] << std::endl;
    }

    return  0;
}