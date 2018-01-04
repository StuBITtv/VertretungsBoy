#include <../include/management.hpp>
#include <iostream>
#include <ctime>

std::vector<std::string> VertretungsBoy::parseMessage(std::string &content) {
	std::string buffer;
	std::vector<std::string> arg;
	
	for(size_t i = 0; i < content.size(); i++) {
		if(content[i] != ' '){
			if((content[i] > 64 && content[i] < 91)) {
				buffer += content[i] + 32;
			} else {
				buffer += content[i];
			}
		} else {
			if(!buffer.empty()) {
				arg.push_back(buffer);
				buffer.clear();
			}
		}
	}
	
	if(!buffer.empty()) {
		arg.push_back(buffer);
	}
	
	return arg;
}

bool VertretungsBoy::needsUpdate(time_t lastUpdate) {
	time_t t= time(0);
	std::tm *now = localtime(&t);
	std::tm UpdateTime;
	
	UpdateTime.tm_sec = 0;
	UpdateTime.tm_min = 45;
	UpdateTime.tm_hour = 1;
	UpdateTime.tm_mday = now->tm_mday;
	UpdateTime.tm_mon = now->tm_mon;
	UpdateTime.tm_year = now->tm_year;
	
	time_t updateT = mktime(&UpdateTime);
	
	if(t > updateT) {
		if(lastUpdate < updateT) {
			return true;
		}
	}
	
	UpdateTime.tm_sec = 0;
	UpdateTime.tm_min = 15;
	UpdateTime.tm_hour = 0;
	UpdateTime.tm_mday = now->tm_mday;
	UpdateTime.tm_mon = now->tm_mon;
	UpdateTime.tm_year = now->tm_year;
	
	updateT = mktime(&UpdateTime);
	
	if(t > updateT) {
		if(lastUpdate < updateT) {
			return true;
		}
	}
	
	return false;
}
