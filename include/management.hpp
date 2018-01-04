#include <vector>
#include <string>

namespace VertretungsBoy {
    
	std::vector<std::string> parseMessage(std::string &content);
	
	std::string createEntriesString(std::vector<std::vector<std::string>> table);
	
	bool needsUpdate(time_t lastUpdate);
};
