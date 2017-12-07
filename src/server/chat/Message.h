/**
 * wsserver
 * Message.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_MESSAGE_HPP
#define WSSERVER_MESSAGE_HPP

#include <string>
#include <iostream>
#include <type_traits>
#include <toolboxpp.h>
#include "json.hpp"
#include "../defs.h"

namespace wss {

extern const char *TYPE_TEXT;
extern const char *TYPE_BINARY;
extern const char *TYPE_B64_IMAGE;
extern const char *TYPE_URL_IMAGE;
extern const char *TYPE_NOTIFICATION_RECEIVED;

/// \brief Main structured message payload
/// \todo Protobuf support
class MessagePayload {
 private:
    UserId sender;
    std::vector<UserId> recipients;
    std::string text;
    std::string type;
    json data;
    bool valid = true;
    std::string errorCause;

    void fromJson(const json &json);
    void validate();
    void handleJsonException(const std::exception &e, const std::string &data);

    friend void to_json(wss::json &j, const wss::MessagePayload &in);
    friend void from_json(const wss::json &j, wss::MessagePayload &in);
 public:
    /// \brief Creates simple send-status messages, using only UserId
    /// \param to sender UserId
    /// \return valid payload object
    static MessagePayload createSendStatus(UserId to);

    /// \brief Creates simple send-status messages, using other payload (extracts sender)
    /// \param payload
    /// \return
    static MessagePayload createSendStatus(const MessagePayload &payload);

    MessagePayload() = default;
    MessagePayload(UserId from, UserId to, const std::string &message);
    MessagePayload(UserId from, UserId to, std::string &&message);
    MessagePayload(UserId from, const std::vector<UserId> &to, const std::string &message);
    MessagePayload(UserId from, std::vector<UserId> &&to, std::string &&message);
    MessagePayload(const MessagePayload &payload) = default;
    MessagePayload(MessagePayload &&payload) = default;
    MessagePayload &operator=(const MessagePayload &payload) = default;
    MessagePayload &operator=(MessagePayload &&payload) = default;

    explicit MessagePayload(const std::string &json) noexcept;
    explicit MessagePayload(const nlohmann::json &obj) noexcept;

    /// \brief Return sender UserId
    /// \return Sender UserId
    UserId getSender() const;

    /// \brief Recipients ids
    /// \return std::vector<UserId>
    const std::vector<UserId> getRecipients() const;

    /// \brief Message type
    /// \return string type. Predefined types:
    /// \see constants TYPE_TEXT, TYPE_BINARY, TYPE_B64_IMAGE, TYPE_URL_IMAGE, TYPE_NOTIFICATION_RECEIVED
    const std::string getType() const {
        return type;
    }

    /// \brief Text message
    /// \return string or empty if type not a TYPE_TEXT
    const std::string getText() const;

    /// \brief Converts this payload to json string
    /// \return valid json string
    const std::string toJson() const;

    /// \brief Checks by passed id, that current payload belongs to sender
    /// \param id UserId
    /// \return true if is my message, otherwise message belongs to my chat-friend
    bool isMyMessage(UserId id) const;

    /// \brief If can't parse input json or some input data is not valid, false will returned
    /// \return
    bool isValid() const;

    /// \brief Check message type
    /// \param type
    /// \return
    bool typeIs(const char *type) const;

    /// \brief Whether message type is just send status
    /// \return
    bool isSentStatus() const;

    /// \brief Whether message type is binary
    /// \return
    bool isBinary() const;

    /// \brief Check recipients count is only one
    /// \return
    bool isSingleRecipient() const;

    /// \brief Error message
    /// \return Empty string if no one error. Check @see isValid() before
    const std::string getError() const;

    MessagePayload &setRecipient(UserId id);
    MessagePayload &setRecipients(const std::vector<UserId> &recipients);
    MessagePayload &setRecipients(std::vector<UserId> &&recipients);
    MessagePayload &addRecipient(UserId to);
};

void to_json(wss::json &j, const wss::MessagePayload &in);
void from_json(const wss::json &j, wss::MessagePayload &in);

}

#endif //WSSERVER_MESSAGE_HPP
