#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::string getUrlContent(const std::string& url);

std::vector<std::string>& splitString(const std::string &s, char delim, std::vector<std::string>& elems);

/**
 * Ensure that we have an absolute URI. If <base> is an empty string the current working directory is
 * used as the base.
 */
std::string buildAbsoluteUri(const std::string& uri, const std::string& base);

/**
  * Check if the given URL falls under the given base; if so then return the 'relative' portion of the URL.
  */
std::string urlChildOf(const std::string& url, const std::string& base);

/**
 * Looks for /model/component/variable[@initial_value] type xpath expression and turns it into
 * /model/component/variable.
 *
 * FIXME: this is a dirty hack to quickly get set-value style changes in repeated tasks working. Needs to be
 * removed ASAP.
 *
 * @param xpath
 * @return
 */
std::string mapToStandardVariableXpath(const std::string& xpath);

#endif // UTILS_HPP
