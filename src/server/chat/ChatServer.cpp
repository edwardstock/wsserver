/**
 * wsserver
 * Server.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include <fmt/format.h>
#include "ChatServer.h"
#include "../helpers/helpers.h"
#include "../base/Settings.hpp"

#ifdef USE_SECURE_SERVER
wss::ChatServer::ChatServer(
    const std::string &crtPath, const std::string &keyPath,
    const std::string &host, unsigned short port, const std::string &regexPath) :
    useSSL(true),
    maxMessageSize(10 * 1024 * 1024),
    server(std::make_unique<WsServer>(crtPath, keyPath)),
    connectionStorage(std::make_unique<wss::ConnectionStorage>())
{
    server->config.port = port;
    server->config.thread_pool_size = std::thread::hardware_concurrency();
    server->config.max_message_size = maxMessageSize;

    if (host.length() == 15) {
        server->config.address = host;
    };

    endpoint = &server->endpoint[regexPath];
    endpoint->on_message = [this](WsConnectionPtr connectionPtr, WsMessagePtr messagePtr) {
      if (messagePtr->fin_rsv_opcode == FLAG_PONG) {
          onPong(connectionPtr, messagePtr);
          return;
      }
      onMessage(connectionPtr, messagePtr);
    };

    endpoint->on_open = std::bind(&wss::ChatServer::onConnected, this, std::placeholders::_1);
    endpoint->on_close = std::bind(&wss::ChatServer::onDisconnected,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3);

}
#else
wss::ChatServer::ChatServer(const std::string &host, unsigned short port, const std::string &regexPath) :
    useSSL(false),
    maxMessageSize(10 * 1024 * 1024),
    server(std::make_unique<WsServer>()),
    connectionStorage(std::make_unique<wss::ConnectionStorage>()) {
    server->config.port = port;
    server->config.thread_pool_size = std::thread::hardware_concurrency();
    server->config.max_message_size = maxMessageSize;

    if (host.length() == 15) {
        server->config.address = host;
    };

    endpoint = &server->endpoint[regexPath];
    endpoint->on_message = [this](WsConnectionPtr connectionPtr, WsMessagePtr messagePtr) {
      if (messagePtr->fin_rsv_opcode == FLAG_PONG) {
          onPong(connectionPtr, messagePtr);
          return;
      }
      onMessage(connectionPtr, messagePtr);
    };

    endpoint->on_open = std::bind(&wss::ChatServer::onConnected, this, std::placeholders::_1);
    endpoint->on_close = std::bind(&wss::ChatServer::onDisconnected,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3);

}
#endif

wss::ChatServer::~ChatServer() {
    stopService();
    joinThreads();
}

void wss::ChatServer::setThreadPoolSize(std::size_t size) {
    server->config.thread_pool_size = size;
}
void wss::ChatServer::joinThreads() {
    if (workerThread && workerThread->joinable()) {
        workerThread->join();
    }

    if (watchdogThread && watchdogThread->joinable()) {
        watchdogThread->join();
    }
}
void wss::ChatServer::detachThreads() {
    if (workerThread) {
        workerThread->detach();
    }
    if (watchdogThread) {
        watchdogThread->detach();
    }
}
void wss::ChatServer::runService() {
    std::string hostname = "[any:address]";
    if (not server->config.address.empty()) {
        hostname = server->config.address;
    }
    const char *proto = useSSL ? "wss" : "ws";
    L_INFO_F("WebSocket Server", "Started at %s://%s:%d", proto, hostname.c_str(), server->config.port);
    workerThread = std::make_unique<boost::thread>([this] {
      this->server->start();
    });

    if (wss::Settings::get().server.watchdog.enabled) {

        const long lifetime = wss::Settings::get().server.watchdog.connectionLifetimeSeconds;
        L_INFO_F("Watchdog", "Started with interval in 1 minute and lifetime=%lu", lifetime);
        watchdogThread =
            std::make_unique<boost::thread>(boost::bind(&wss::ChatServer::watchdogWorker, this, lifetime));
    }
}
void wss::ChatServer::stopService() {
    this->server->stop();
    if (watchdogThread) {
        watchdogThread->interrupt();
    }
}

void wss::ChatServer::watchdogWorker(long lifetime) {
    try {
        while (true) {
            boost::this_thread::sleep_for(boost::chrono::minutes(1));
            for (auto &t: connectionStorage->get()) {
                const auto &ref = getStat(t.first);
                for (const auto &conn: t.second) {
                    if (!conn.second) {
                        connectionStorage->remove(t.first, conn.first);
                        continue;
                    }

                    if (ref->getInactiveTime() >= lifetime) {
                        conn.second->send_close(STATUS_INACTIVE_CONNECTION,
                                                fmt::format("Inactive more than {0:d} seconds ({1:d})",
                                                            lifetime,
                                                            ref->getInactiveTime()));
                    } else {
                        auto pingStream = std::make_shared<WsMessageStream>();
                        *pingStream << ".";
                        conn.second->send(pingStream, [conn, this](const SimpleWeb::error_code &err) {
                          if (err) {
                              // does not matter, what happens, anyway, this mean connection is bad, broken pipe, eof or something else
                              connectionStorage->remove(conn.second);
                          } else {
                              // lazy ping pong
                              connectionStorage->markPongWait(conn.second);
                          }
                        }, FLAG_PING);
                    }
                }
            }

            boost::this_thread::sleep_for(boost::chrono::seconds(2));
            std::size_t disconnected = connectionStorage->disconnectWithoutPong();
            if (disconnected > 0) {
                L_DEBUG_F("Watchdog", "Disconnected %lu dangling connections", disconnected);
            }
        }
    } catch (const boost::thread_interrupted &) {
        L_INFO("Watchdog", "Stopping...");
    }
}

void wss::ChatServer::onPong(wss::WsConnectionPtr &connection, wss::WsMessagePtr) {
    connectionStorage->markPongReceived(connection);
}

void wss::ChatServer::onMessage(WsConnectionPtr &connection, WsMessagePtr message) {
    MessagePayload payload;
    const short opcode = message->fin_rsv_opcode;

    if (opcode != FLAG_FRAME_TEXT && opcode != FLAG_FRAME_BINARY) {
        // fragmented frame message
        user_id_t senderId = connection->getId();

        if (opcode == FLAG_FRAGMENT_BEGIN_TEXT || opcode == FLAG_FRAGMENT_BEGIN_BINARY) {
            L_DEBUG_F("Chat::Message", "Fragmented frame begin (flag: 0x%08x)", opcode);
            writeFrameBuffer(senderId, message->string(), true);
            return;
        } else if (opcode == FLAG_FRAGMENT_CONTINUE) {
            writeFrameBuffer(senderId, message->string(), false);
            return;
        } else if (opcode == FLAG_FRAGMENT_END) {
            L_DEBUG("Chat::Message", "Fragmented frame end");
            std::stringstream final;
            final << readFrameBuffer(senderId, true);
            final << message->string();
            std::string buffered = final.str();
            final.str("");
            final.clear();

            /**
             * @Deprecated
             */
            if (buffered.length() > maxMessageSize) {
                connection->send_close(STATUS_MESSAGE_TOO_BIG,
                                       "Message to big. Maximum size: "
                                           + wss::helpers::humanReadableBytes(maxMessageSize));
                return;
            }

            payload = MessagePayload(buffered);
        }
    } else {
        // one frame message
        payload = MessagePayload(message->string());
    }

    if (!payload.isValid()) {
        connection->send_close(STATUS_INVALID_MESSAGE_PAYLOAD, "Invalid payload. " + payload.getError());
        return;
    }

    if (wss::Settings::get().chat.enableSendBack) {
        sendTo(payload.getSender(), payload);

    }

    send(payload);
}

void wss::ChatServer::onMessageSent(wss::MessagePayload &&payload, std::size_t bytesTransferred, bool hasSent) {
    if (payload.isTypeOfSentStatus()) return;

    getStat(payload.getSender())
        ->addSendMessage()
        .addBytesTransferred(bytesTransferred);

    for (auto &i: payload.getRecipients()) {
        if (hasSent) {
            getStat(i)->addReceivedMessage().addBytesTransferred(bytesTransferred);
        }
    }

    if (enableMessageDeliveryStatus && hasSent) {
        wss::MessagePayload status = MessagePayload::createSendStatus(payload);
        send(status);
    }
}

void wss::ChatServer::onConnected(WsConnectionPtr connection) {
    wss::web::Request request;
    request.parseParamsString(connection->query_string);
    request.setHeaders(connection->header);

    if (!auth->validateAuth(request)) {
        connection->send_close(STATUS_UNAUTHORIZED, "Unauthorized");
        return;
    }

    if (request.getParams().empty()) {
        L_WARN_F("Chat::Connect::Error", "Invalid request: %s", connection->query_string.c_str());
        connection->send_close(STATUS_INVALID_QUERY_PARAMS, "Invalid request");
        return;
    } else if (!request.hasParam("id") || request.getParam("id").empty()) {
        L_WARN("Chat::Connect::Error", "Id required in query parameter: ?id={id}");

        connection->send_close(STATUS_INVALID_QUERY_PARAMS, "Id required in query parameter: ?id={id}");
        return;
    }

    user_id_t id;
    try {
        id = std::stoul(request.getParam("id"));
    } catch (const std::invalid_argument &e) {
        const std::string errReason = "Passed invalid id: id=" + request.getParam("id") + ". " + e.what();
        L_WARN("Chat::Connect::Error", errReason);
        connection->send_close(STATUS_INVALID_QUERY_PARAMS, errReason);
        return;
    }

    connectionStorage->add(id, connection);
    getStat(id)->addConnection();

    L_DEBUG_F("Chat::Connect", "User %lu connected (%s:%d) on thread %lu",
              id,
              connection->remote_endpoint_address().c_str(),
              connection->remote_endpoint_port(),
              getThreadName()
    );

    redeliverMessagesTo(id);
}
void wss::ChatServer::onDisconnected(WsConnectionPtr connection, int status, const std::string &reason) {
    if (!connectionStorage->exists(connection->getId())) {
        return;
    }

    L_DEBUG_F("Chat::Disconnect", "User %lu (%lu) has disconnected by reason: %s[%d]",
              connection->getId(),
              connection->getUniqueId(),
              reason.c_str(),
              status
    );

    getStat(connection->getId())->addDisconnection();
    connectionStorage->remove(connection);
}

bool wss::ChatServer::hasFrameBuffer(wss::user_id_t senderId) {
    return frameBuffer.find(senderId) != frameBuffer.end();
}

bool wss::ChatServer::writeFrameBuffer(wss::user_id_t senderId, const std::string &input, bool clear) {
    std::lock_guard<std::mutex> fbLock(frameBufferMutex);
    if (!hasFrameBuffer(senderId)) {
        frameBuffer[senderId] = std::make_shared<std::stringstream>();
    } else if (clear) {
        frameBuffer[senderId]->str("");
        frameBuffer[senderId]->clear();
    }

    (*frameBuffer[senderId]) << input;
    return true;
}
const std::string wss::ChatServer::readFrameBuffer(wss::user_id_t senderId, bool clear) {
    std::lock_guard<std::mutex> fbLock(frameBufferMutex);
    if (!hasFrameBuffer(senderId)) {
        return std::string();
    }

    const std::string out = frameBuffer[senderId]->str();
    if (clear) {
        frameBuffer.erase(senderId);
    }
    return out;
}

int wss::ChatServer::redeliverMessagesTo(const wss::MessagePayload &payload) {
    int cnt = 0;
    for (user_id_t id: payload.getRecipients()) {
        cnt += redeliverMessagesTo(id);
    }

    return cnt;
}
bool wss::ChatServer::hasUndeliveredMessages(user_id_t recipientId) {
    std::lock_guard<std::mutex> locker(undeliveredMutex);
    L_DEBUG_F("Chat::Underlivered",
              "Check for undelivered messages for user %lu: %lu",
              recipientId,
              undeliveredMessagesMap[recipientId].size());
    return !undeliveredMessagesMap[recipientId].empty();
}
wss::MessageQueue &wss::ChatServer::getUndeliveredMessages(user_id_t recipientId) {
    std::lock_guard<std::mutex> locker(undeliveredMutex);
    return undeliveredMessagesMap[recipientId];
}

std::vector<wss::MessageQueue *>
wss::ChatServer::getUndeliveredMessages(const MessagePayload &payload) {
    std::vector<wss::MessageQueue *> out;
    {
        std::lock_guard<std::mutex> locker(undeliveredMutex);
        for (user_id_t id: payload.getRecipients()) {
            out.push_back(&undeliveredMessagesMap[id]);
        }
    }

    return out;
}

void wss::ChatServer::enqueueUndeliveredMessage(const wss::MessagePayload &payload) {
    std::unique_lock<std::mutex> uniqueLock(undeliveredMutex);
    for (auto recipient: payload.getRecipients()) {
        undeliveredMessagesMap[recipient].push(payload);
    }
}
int wss::ChatServer::redeliverMessagesTo(user_id_t recipientId) {
    if (not wss::Settings::get().chat.enableUndeliveredQueue) {
        return 0;
    }

    if (!hasUndeliveredMessages(recipientId)) {
        return 0;
    }

    int cnt = 0;
    auto &queue = getUndeliveredMessages(recipientId);
    L_DEBUG_F("Chat::Undelivered", "Redeliver %lu message(s) to user %lu", queue.size(), recipientId);
    while (!queue.empty()) {
        MessagePayload payload = queue.front();
        queue.pop();
        send(payload);
        cnt++;
    }

    return cnt;
}

void wss::ChatServer::send(const wss::MessagePayload &payload) {
    // if recipient is a BOT, than we don't need to find conneciton, just trigger event notifier ilsteners
    if (payload.isForBot()) {
        callOnMessageListeners(payload);
        L_DEBUG("Chat::Send", "Sending message to bot");
        return;
    }

    callOnMessageListeners(payload);

    for (user_id_t uid: payload.getRecipients()) {
        if (uid == 0L) {
            // just in case, prevent sending bot-only message to nobody
            continue;
        }

        sendTo(uid, payload);
    }
}

void wss::ChatServer::sendTo(user_id_t recipient, const wss::MessagePayload &payload) {
    const std::string payloadString = payload.toJson();
    std::size_t payloadSize = payloadString.length();
    uint8_t fin_rsv_opcode = 129;//@TODO static_cast<uint8_t>(payload.isBinary() ? 130 : 129);

    std::lock_guard<std::recursive_mutex> locker(connectionMutex);

    if (!connectionStorage->size(recipient)) {
        handleUndeliverable(recipient, payload);
        MessagePayload sent = payload; // copy to move, referenced payload will goes out of scope
        sent.setRecipient(recipient);
        onMessageSent(std::move(sent), payloadSize, false);
        return;
    }

    try {
        const auto &connections = connectionStorage->get(recipient);
        int i = 0;
        for (auto &connection: connections) {

            if (!connection.second) {
                connectionStorage->remove(recipient, connection.first);
                continue;
            }

            // DO NOT reuse stream, it will die after first sending
            auto sendStream = std::make_shared<WsMessageStream>();
            *sendStream << payloadString;

            L_DEBUG_F("Chat::Send",
                      "Sending message to recipient %lu, connection[%d]=%lu",
                      recipient,
                      i,
                      connection.second->getUniqueId());

            // connection->send is an asynchronous function
            connection.second
                      ->send(sendStream,
                             [this, recipient, payload, payloadSize,
                                 cid = connection.first](const SimpleWeb::error_code &errorCode) {
                               if (errorCode) {
                                   // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
                                   L_DEBUG_F("Chat::Send::Error", "Unable to send message. Cause: %s, message: %s",
                                             errorCode.category().name(),
                                             errorCode.message().c_str()
                                   );
                                   if (errorCode.value() == boost::system::errc::broken_pipe) {
                                       L_DEBUG_F("Chat::Send::Error",
                                                 "Disconnecting Broken connection %lu (%lu)",
                                                 recipient,
                                                 cid);
                                       connectionStorage->remove(recipient, cid);
                                   }
                                   handleUndeliverable(recipient, payload);
                               } else {
                                   MessagePayload sent = payload;
                                   sent.setRecipient(recipient);
                                   onMessageSent(std::move(sent), payloadSize, true);
                               }
                             }, fin_rsv_opcode);

            i++;
        }
    } catch (const ConnectionNotFound &) {
        L_DEBUG("Chat::Send", "Connection not found exception. Adding payload to undelivered");
        handleUndeliverable(recipient, payload);
    }
}

void wss::ChatServer::handleUndeliverable(wss::user_id_t uid, const wss::MessagePayload &payload) {
    if (!wss::Settings::get().chat.enableUndeliveredQueue) {
        L_DEBUG_F("Chat::Send", "User %lu is unavailable. Skipping message.", uid);
        return;
    }
    MessagePayload inaccessibleUserPayload = payload;
    inaccessibleUserPayload.setRecipient(uid);
    // as messages did not sent to exact user, we add to undelivered queue exact this user in payload recipients
    enqueueUndeliveredMessage(inaccessibleUserPayload);
    L_DEBUG_F("Chat::Send", "User %lu is unavailable. Adding message to queue", uid);
}

std::size_t wss::ChatServer::getThreadName() {
    const std::thread::id id = std::this_thread::get_id();
    static std::size_t nextindex = 0;
    static std::mutex my_mutex;
    static std::map<std::thread::id, std::size_t> ids;
    std::lock_guard<std::mutex> lock(my_mutex);
    if (ids.find(id) == ids.end())
        ids[id] = nextindex++;

    return ids[id];
}
void wss::ChatServer::setMessageSizeLimit(size_t bytes) {
    maxMessageSize = bytes;
    server->config.max_message_size = maxMessageSize;
}
void wss::ChatServer::setAuth(const nlohmann::json &config) {
    auth = wss::auth::createFromConfig(config);
}

void wss::ChatServer::setEnabledMessageDeliveryStatus(bool enabled) {
    enableMessageDeliveryStatus = enabled;
}

void wss::ChatServer::addMessageListener(wss::ChatServer::OnMessageSentListener callback) {
    messageListeners.push_back(callback);
}
void wss::ChatServer::addStopListener(wss::ChatServer::OnServerStopListener callback) {
    stopListeners.push_back(callback);
}
std::unique_ptr<wss::Statistics> &wss::ChatServer::getStat(wss::user_id_t id) {
    if (statistics.find(id) == statistics.end()) {
        statistics[id] = std::make_unique<wss::Statistics>(id);
    }

    return statistics[id];
}

const wss::UserMap<std::unique_ptr<wss::Statistics>> &wss::ChatServer::getStats() {
    return statistics;
}
void wss::ChatServer::callOnMessageListeners(wss::MessagePayload payload) {
    for (auto &listener: messageListeners) {
        listener(std::move(payload));
    }
}