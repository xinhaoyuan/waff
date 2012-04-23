#include "dict.hpp"

#include <sstream>

using namespace std;

int
Dict::DeserializeFromFile(FILE *in)
{
    ostringstream s;
    char c;
    mMap.clear();
    
    while (!feof(in))
    {
        s.str("");
        while (!feof(in))
        {
            c = fgetc(in);
            if (c == '|') break;
            else if (c == '\\' && !feof(in))
                c = fgetc(in);
            s << c;
        }

        if (c != '|') break;
        
        string key = s.str();
        s.str("");
        
        while (!feof(in))
        {
            c = fgetc(in);
            if (c == '|') break;
            else if (c == '\\' && !feof(in))
                c = fgetc(in);
            s << c;
        }

        if (c != '|') break;

        string value = s.str();

        mMap[key] = value;
    }

    return 0;
}

int
Dict::SerializeToFile(FILE *out)
{
    map<string, string>::iterator it;
    for (it = mMap.begin(); it != mMap.end(); ++ it)
    {
        const string *now;
        size_t i;
                
        now = &it->first;
        
        for (i = 0; i < now->length(); ++ i)
        {
            if ((*now)[i] == '\\' || (*now)[i] == '|')
                fputc('\\', out);
            fputc((*now)[i], out);
        }
        fputc('|', out);

        now = &it->second;
        
        for (i = 0; i < now->length(); ++ i)
        {
            if ((*now)[i] == '\\' || (*now)[i] == '|')
                fputc('\\', out);
            fputc((*now)[i], out);
        }
        fputc('|', out);
    }
    
    return 0;
}

int 
Dict::Get(const string &key, string &slot)
{
    if (mMap.find(key) == mMap.end())
        return -1;

    slot = mMap[key];
    return 0;
}

void
Dict::Put(const string &key, const string &value)
{
    mMap[key] = value;
}

void
Dict::Remove(const string &key)
{
    mMap.erase(key);
}

int
Dict::GetNextKey(const std::string &key, const std::string &prefix, std::string &slot)
{
    map<string, string>::iterator it = mMap.upper_bound(key);

    if (it == mMap.end())
        return -1;
    
    const string &nkey = it->first;

    for (size_t i = 0; i < prefix.length(); ++ i)
    {
        if (i >= nkey.length()) return -1;
        if (nkey[i] != prefix[i]) return -1;
    }

    slot = nkey;
    return 0;
}
