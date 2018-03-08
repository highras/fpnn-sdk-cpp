#ifndef FPNN_StringUtil_H
#define FPNN_StringUtil_H

#include <set>
#include <unordered_set>
#include <string>
#include <vector>
#include <map>
#include <string.h>

namespace fpnn {
namespace StringUtil{

	char * rtrim(char *s);							//-- trim right side.
	char * ltrim(char *s);							//-- cp: change pointer.
	char * trim(char *s);							//-- cp: change pointer.
	std::string& rtrim(std::string& s);
	std::string& ltrim(std::string& s);
	std::string& trim(std::string& s);

	bool replace(std::string& str, const std::string& from, const std::string& to);
        
	//will discard empty field
	std::vector<std::string> &split(const std::string &s, const std::string& delim, std::vector<std::string> &elems);

	template<typename T>
	std::vector<T> &split(const std::string &s, const std::string& delim, std::vector<T> &elems)
	{
		std::vector<std::string> e;
		e = split(s, delim, e); 
		for(size_t i = 0; i < e.size(); ++i){
			elems.push_back(stol(e[i]));
		}   
		return elems;
	}

	template<typename T>
	std::set<T> &split(const std::string &s, const std::string& delim, std::set<T> &elems)
	{
		std::vector<std::string> e;
		e = split(s, delim, e); 
		for(size_t i = 0; i < e.size(); ++i){
			elems.insert(stol(e[i]));
		}   
		return elems;
	}

	template<typename T>
	std::unordered_set<T> &split(const std::string &s, const std::string& delim, std::unordered_set<T> &elems)
	{
		std::vector<std::string> e;
		e = split(s, delim, e); 
		for(size_t i = 0; i < e.size(); ++i){
			elems.insert(stol(e[i]));
		}   
		return elems;
	}

	std::set<std::string> &split(const std::string &s, const std::string& delim, std::set<std::string> &elems);
	std::unordered_set<std::string> &split(const std::string &s, const std::string& delim, std::unordered_set<std::string> &elems);
	/*
		join func():
		if necessary, please extend to the following format.
		std::string join(const std::vector<std::string> &v, const std::string& delim, bool jsonArray = false, bool escapseQuot = false);
	*/
	template<typename T>
	std::string join(const std::set<T> &v, const std::string& delim)
	{
		std::string ret;
		for (auto& t: v)
		{
			if(ret.length())
				ret += delim;
			
			ret += std::to_string(t);
		}
		return ret;
	}

	template<typename T>
	std::string join(const std::vector<T> &v, const std::string& delim)
	{
		std::string ret;
		for (size_t i = 0; i < v.size(); ++i)
		{
			if(i != 0)
				ret += delim;
			
			ret += std::to_string(v[i]);
		}
		return ret;
	}
	std::string join(const std::set<std::string> &v, const std::string& delim);
	std::string join(const std::vector<std::string> &v, const std::string& delim);
	std::string join(const std::map<std::string, std::string> &v, const std::string& delim);
	
	inline std::string escapseString(const std::string& s){
		if(s.empty()) return "\"\"";
		return "\"" + s + "\"";
	}

	class CharsChecker
	{
	private:
		bool _map[256];

	public:
		CharsChecker(const unsigned char *targets)
		{
			bzero(_map, sizeof(_map));
			for (unsigned char *p = (unsigned char *)targets; *p; p++)
				_map[*p] = true;
		}
		CharsChecker(const unsigned char *targets, int len)
		{
			bzero(_map, sizeof(_map));
			for (int i = 0; i < len; i++)
				_map[targets[i]] = true;
		}
		CharsChecker(const std::string& targets): CharsChecker((const unsigned char*)targets.data(), (int)targets.length()) {}

		inline bool operator[] (unsigned char c)
		{
			return _map[c];
		}
	};
}
}
#endif
