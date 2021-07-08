#ifndef FPNN_TimeUtil_H
#define FPNN_TimeUtil_H

#include <string>
#include <stdint.h>

namespace fpnn {
namespace TimeUtil{

	//Fast but less precision
	//All not accurate
	
	//For HTTP Date Format
	std::string getTimeRFC1123();

	//example: "yyyy-MM-dd HH:mm:ss"
	std::string getDateTime();
	std::string getDateTime(int64_t t);

	//example: "yyyy-MM-dd"
	//example: "yyyy/MM/dd"
	std::string getDateStr(char sep = '-');
	std::string getDateStr(int64_t t, char sep = '-');
	
	//example: "yyyy-MM-dd-HH-mm-ss"
	//example: "yyyy/MM/dd/HH/mm/ss"
	//std::string getTimeStr(char sep = '-');
	//std::string getTimeStr(int64_t t, char sep = '-');

	//example: "yyyy-MM-dd HH:mm:ss,SSS"
	std::string getDateTimeMS(); 
	std::string getDateTimeMS(int64_t t);

	int64_t curr_sec();
	int64_t curr_msec();
}
}
#endif
