/**
 * scatter
 * PostbackTarget.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_POSTBACKTARGET_H
#define SCATTER_POSTBACKTARGET_H

#include <string>
#include <nlohmann/json.hpp>
#include "scatter/Target.h"
#include "scatter/HttpClient.h"
#include "../chat/ChatServer.h"
#include "scatter/Auth.h"

namespace wss {
namespace event {

class PostbackTarget : public Target {
 public:
    /// \brief json object of postback configuration
    /// \param config
    explicit PostbackTarget(const json &config);

    void send(const wss::MessagePayload &payload,
              OnSendSuccess successCallback,
              OnSendError errorCallback) override;

    std::string getType() const override;

 protected:
    /// \brief Return http client
    /// \see wss::web::HttpClient
    /// \return
    wss::web::HttpClient &getClient();

    /// \brief Return auth object for authentication while sending event
    /// \see wss::WebAuth
    /// \return
    std::unique_ptr<wss::Auth> &getAuth();

 private:
    template<class T>
    void setAuth(T &&auth);

    wss::web::Request::Method m_httpMethod;
    std::unique_ptr<wss::Auth> m_auth;
    wss::web::HttpClient m_client;
    std::string m_url;

};

}
}

#endif //SCATTER_POSTBACKTARGET_H
