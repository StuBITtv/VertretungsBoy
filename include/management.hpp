#include <vector>
#include <string>
#include <discordpp/bot.hh>

namespace VertretungsBoy {
    
	std::vector<std::string> parseMessage(std::string &content);
	
	std::string createEntriesString(std::vector<std::vector<std::string>> table);
	
	bool needsUpdate(time_t lastUpdate);
	
	std::string getLastRequest(std::string dbPath, std::string userID);
	void saveRequest(std::string dbPath, std::string userID, std::string request);

	void createErrorMsg(discordpp::Bot *bot, std::string error, std::string channelID);
    void createMsg(discordpp::Bot *bot, std::string msg, std::string channelID);
};
