#include <../include/management.hpp>

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
