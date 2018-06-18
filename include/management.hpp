#include <vector>
#include <string>
#include <discordpp/bot.hh>

namespace VertretungsBoy {
    
	std::vector<std::string> parseMessage(const std::string &content);
	
	std::string createEntriesString(const std::vector<std::vector<std::string>> &table);

    std::string time_tToString(time_t t);

	bool needsUpdate(time_t lastUpdate);
	
	std::string getLastSearch(const std::string &dbPath, std::string userID);
	void saveSearch(const std::string &dbPath, const std::string &userID, std::string searchValue);

	void createErrorMsg(discordpp::Bot *bot, const std::string &error, const std::string &channelID);
    void createMsg(discordpp::Bot *bot, const std::string &msg, const std::string &channelID);
};
