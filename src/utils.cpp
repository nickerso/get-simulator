#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <libxml/uri.h>

// FIXME: need to do this better?
#ifdef _MSC_VER
#  include <direct.h>
#  define PATH_MAX_SIZE 4096
#  define getcwd _getcwd
#else
#  include <unistd.h>
#  define PATH_MAX_SIZE pathconf(".",_PC_PATH_MAX)
#endif

#include "utils.hpp"

std::string buildAbsoluteUri(const std::string& uri, const std::string& base)
{
    std::string b = base;
    if (b.empty())
    {
        int size = PATH_MAX_SIZE;
        char* cwd = (char*)malloc(size);
        if (!getcwd(cwd,size)) cwd[0] = '\0';
        b = "file://";
        b += cwd;
        b += "/";
        free(cwd);
    }
    xmlChar* fullURL = xmlBuildURI(BAD_CAST uri.c_str(), BAD_CAST b.c_str());
    std::string url((char*)fullURL);
    xmlFree(fullURL);
    return url;
}

std::vector<std::string>& splitString(const std::string &s, char delim, std::vector<std::string>& elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

std::string urlChildOf(const std::string& url, const std::string& base)
{
    std::string result;
    size_t found = url.find(base);
    if (found == 0)
    {
        result = url.substr(base.size());
    }
    return result;
}

std::string mapToStandardVariableXpath(const std::string& xpath)
{
    std::string result(xpath);
    result.erase(result.find("/@initial_value"));
    return result;
}
