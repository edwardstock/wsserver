/**
 * wsserver
 * joinable.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_JOINABLE_H
#define WSSERVER_JOINABLE_H

namespace wss {

class StandaloneService {
 public:
    /// \brief Joins service threads
    virtual void joinThreads() = 0;
    /// \brief Detach service threads from main
    virtual void detachThreads() = 0;
    /// \brief Run service
    virtual void runService() = 0;
    /// \brief Stop service
    virtual void stopService() = 0;

};
}

#endif //WSSERVER_JOINABLE_H
