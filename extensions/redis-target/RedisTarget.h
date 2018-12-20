/**
 * scatter
 * RedisTarget.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_REDISTARGET_H
#define SCATTER_REDISTARGET_H

#include <cpp_redis/core/client.hpp>
#include <scatter/Target.h>
#include <scatter/Message.h>
#include <nlohmann/json.hpp>

namespace wss {
namespace event {
class RedisTarget : public wss::event::Target {
 public:
    enum Mode {
      Queue,
      Channel
    };

    RedisTarget(const nlohmann::json &config);
    ~RedisTarget();

    void send(const wss::MessagePayload &payload,
              const OnSendSuccess &successCallback,
              const OnSendError &errorCallback) override;
    std::string getType() const override;

 private:
    cpp_redis::client client;
    std::string modeTargetName;
    Mode mode;

    void onConnected(const nlohmann::json &config);
};

extern "C" SCATTER_EXPORT Target *target_create(const nlohmann::json &config);
extern "C" SCATTER_EXPORT void target_release(Target *target);
}
}

#endif //SCATTER_REDISTARGET_H
