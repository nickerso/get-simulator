#include <curl/curl.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <libxml/uri.h>

#include "utils.hpp"

class CurlData
{
public:
    CurlData()
    {
        std::cout << "Creating a persistent CurlData handle" << std::endl;
        mCurl = curl_easy_init();
    }
    ~CurlData()
    {
        std::cout << "Destroying a persistent CurlData handle" << std::endl;
        if (mCurl) curl_easy_cleanup(mCurl);
    }

    CURL* mCurl;
};

static size_t retrieveContent(char* buffer, size_t size, size_t nmemb, void* string)
{
    std::string* s = static_cast<std::string*>(string);
    size_t bytes = size * nmemb;
    s->append(buffer, bytes);
    return bytes;
}


std::string getUrlContent(const std::string &url)
{
    xmlChar* fullURL = xmlBuildURI(BAD_CAST url.c_str(), BAD_CAST "file:///Users/dnic019/shared-folders/projects/kidney/GeneralEpithelialTransport/build/get-simulator/Debug/");
    std::string _url((char*)fullURL);
    xmlFree(fullURL);
    std::cout << "URL to fetch: " << _url.c_str() << std::endl;
    static CurlData curlHandle;
    std::string data, headerData;
    CURL* curl = curlHandle.mCurl;
    curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, retrieveContent);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, static_cast<void*>(&headerData));
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&data));
    //curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    CURLcode res = curl_easy_perform(curl);
    if(CURLE_OK != res)
    {
        /* we failed */
        std::cerr << "curl told us " << res << std::endl;
        return "";
    }
    // check headers
    std::vector<std::string> headers;
    headers = splitString(headerData, '\n', headers);
    if (headers.size() > 0)
    {
        // have some headers to check
        if (headers[0].find("200") == std::string::npos)
        {
            // HTTP 200 OK header response not seen so delete any returned data
            data.clear();
        }
    }
    return data;
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
