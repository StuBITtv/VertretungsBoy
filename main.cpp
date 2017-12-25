#include <iostream>
#include "plan.hpp"

int main() {

    std::vector<std::string> urls { "dbg-metzingen.de/vertretungsplan/tage/subst_001.htm", "dbg-metzingen.de/vertretungsplan/tage/subst_002.htm" };
    VertretungsBoy::plan test(urls, ".planBackup.db");

    test.update();

    return  0;
}