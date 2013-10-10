#ifndef UTILS_HPP
#define UTILS_HPP

std::string getUrlContent(const std::string& url);

std::vector<std::string>& splitString(const std::string &s, char delim, std::vector<std::string>& elems);

/**
  * Check if the given URL falls under the given base; if so then return the 'relative' portion of the URL.
  */
std::string urlChildOf(const std::string& url, const std::string& base);

#endif // UTILS_HPP
