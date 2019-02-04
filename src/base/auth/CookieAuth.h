/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef WSSERVER_COOKIEAUTH_H
#define WSSERVER_COOKIEAUTH_H

#include "Auth.h"

namespace wss {

/// \brief
/// Example config.json
/// "auth": {
///      "type": "cookie",
///      "name": "cookie_name",
///      "value": "cookie_value"
///    }
class CookieAuth : public Auth {
 public:
    CookieAuth(const std::string &cookieName, const std::string &cookieValue);

    CookieAuth(std::string &&cookieName, std::string &&cookieValue);

    std::string getType() override;
    void performAuth(httb::request &request) const override;
    bool validateAuth(const httb::request &request) const override;

    std::string getLocalValue() const override;
    std::string getRemoteValue(const httb::request &request) const override;
 private:
    std::string m_name, m_value;
};

}

#endif //WSSERVER_COOKIEAUTH_H
