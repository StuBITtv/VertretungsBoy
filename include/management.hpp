#include <vector>
#include <string>

namespace VertretungsBoy {
    
	std::vector<std::string> parseMessage(std::string &content);
	
	std::string createEntriesString(std::vector<std::vector<std::string>> table);
	
	bool needsUpdate(time_t lastUpdate);
	
	std::string getLastRequest(std::string dbPath, std::string userID);
	void saveRequest(std::string dbPath, std::string userID, std::string request);
};
