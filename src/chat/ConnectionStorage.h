/**
 * wsserver
 * ConnectionStorage.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_CONNECTIONSTORAGE_H
#define WSSERVER_CONNECTIONSTORAGE_H

#include <mutex>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <toolboxpp.hpp>
#include "../wsserver_core.h"

using toolboxpp::Logger;

namespace wss {

struct ConnectionNotFound : std::exception {
  const char *what() const throw() override {
      return "Connection not found or already disconnected";
  }
};

/// \brief Container for handling and storing client connections
class ConnectionStorage {
 private:
    mutable std::recursive_mutex m_connectionMutex;
    mutable std::mutex m_pongMutex;
    wss::UserMap<wss::ConnectionMap<WsConnectionPtr>> m_idMap;
    wss::ConnectionMap<std::pair<user_id_t, bool>> m_waitForPong;

 public:
    using ItemHandler = std::function<void(size_t, const wss::WsConnectionPtr &, wss::conn_id_t, wss::user_id_t)>;
    using ItemNotFoundHandler = std::function<void(wss::user_id_t, wss::conn_id_t)>;


    /// \brief Default empty constructor
    ConnectionStorage() = default;

    /// \brief Deleted copy ctr, cause contains mutex
    /// \param other
    ConnectionStorage(const ConnectionStorage &other) = delete;

    /// \brief Deleted move ctr, cause contains mutex
    /// \param other
    ConnectionStorage(ConnectionStorage &&other) = delete;

    /// \brief On destruction, all connections trying to disconnect, than map of connections will be cleaned up
    ~ConnectionStorage();

    /// \brief Check for existing connection using UserId
    /// \param id UserId
    /// \return true if connection with UserId in map
    bool exists(wss::user_id_t id) const;

    /// \brief Check connection is not nullptr, remove if null
    /// \param connection
    /// \return true if is valid connection, false othwerwise
    bool verify(uint8_t pingFlag);

    /// \brief Count total users in map
    /// \return Size of map user:connections
    std::size_t size() const;

    /// \brief Count total connections for entire user
    /// \param id UserId
    /// \return Size of vector user connections
    std::size_t size(wss::user_id_t id);

    /// \brief Add to map new connection (non-unique)
    /// \param id UserId
    /// \param connection SimpleWeb::Connection shared_ptr
    void add(wss::user_id_t id, const wss::WsConnectionPtr &connection);

    /// \brief Remove from map connections by user id
    /// \param id UserId
    void remove(wss::user_id_t id);

    /// \brief Remove from map connections by user id and connection unique id
    /// \param id UserId
    void remove(wss::user_id_t id, wss::conn_id_t connectionId);

    /// \brief Remove from connection by unique connection id : std::stoul("ip address without dots" + "remote port num").
    /// \param connection SimpleWeb::Connection shared_ptr
    void remove(const wss::WsConnectionPtr &connection);

    /// \brief Return from map connections for entire user
    /// \param id UserId
    /// \return  Vector of connections
    wss::ConnectionMap<wss::WsConnectionPtr> &get(wss::user_id_t id);

    /// \brief Return whole map by const reference
    /// \return unordered_map< UserId, vector<WsConnectionPtr> >
    const wss::UserMap<wss::ConnectionMap<wss::WsConnectionPtr>> &get() const;

    /// \brief Callback function to handle connections for entire UserId
    /// \param id UserId
    /// \param handler callback function (void<WsConnectionPtr &>)
    void handle(user_id_t id, std::function<void(WsConnectionPtr &)> &&handler);

    /// \brief Mark connection as waiting for pong response from client.
    /// \param connection
    void markPongWait(const WsConnectionPtr &connection);

    /// \brief Mark connection as received pong response from client
    /// \param connection
    void markPongReceived(const WsConnectionPtr &connection);

    /// \brief Disconnect all queued connections, that waits for pong response but not received
    /// \return count of disconnected connections
    std::size_t disconnectWithoutPong(int statusCode, const std::string &reason);

    /// \brief Handle connections by recipient. Uses recursive mutex to prevent data races
    /// \param recipient recipient id
    /// \param handler
    void forEach(wss::user_id_t recipient, const wss::ConnectionStorage::ItemHandler &handler, const wss::ConnectionStorage::ItemNotFoundHandler& = nullptr);
};

}

#endif //WSSERVER_CONNECTIONSTORAGE_H
