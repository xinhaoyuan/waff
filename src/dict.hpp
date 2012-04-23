#ifndef __DICT_HPP__
#define __DICT_HPP__

#include <cstdio>
#include <string>
#include <map>

class Dict
{
    std::map<std::string, std::string> mMap;
    
public:
    int  Get(const std::string &key, std::string &slot);
    void Put(const std::string &key, const std::string &value);
    void Remove(const std::string &key);
    int  GetNextKey(const std::string &key, const std::string &prefix, std::string &slot);
    
    int DeserializeFromFile(FILE *in);
    int SerializeToFile(FILE *out);
};

#endif
