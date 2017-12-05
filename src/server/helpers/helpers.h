/**
 * wsserver
 * DateHelper.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_DATEHELPER_HPP
#define WSSERVER_DATEHELPER_HPP

#include <string>
#include <ctime>
#include <boost/date_time.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/crc.hpp>
#include <chrono>
#include <unordered_map>
#include <cmath>
#include <toolboxpp.h>

namespace wss {
namespace helpers {

namespace cal = boost::gregorian;
namespace pt = boost::posix_time;

using namespace std::chrono;

/// \brief STL chrono hi-resolution clock time point
using hires_time_t = std::chrono::high_resolution_clock::time_point;

/// \brief STL chrono hi-resolution clock
using hires_clock = std::chrono::high_resolution_clock;

/// \brief ISO 8601 date format without time zone at the end
static const char DATE_TIME_ISO_8601[] = "%Y-%m-%d %H:%M:%S";

/// \brief Parse string date to boost ptime
/// \param t input string date
/// \param format format string
/// \link http://www.boost.org/doc/libs/1_55_0/doc/html/date_time/date_time_io.html
/// \return Boost ptime
pt::ptime parseDate(const std::string &t, const char *format);

/// \brief parse string date in ISO-8601 format
/// \param t string: 2017-11-15 23:59:59
/// \return boost ptime
pt::ptime parseISODateTime(const std::string &t);

/// \brief Format boost ptime by format string
/// \param t boost::posix_time::ptime
/// \param format string format.
/// \link http://www.boost.org/doc/libs/1_55_0/doc/html/date_time/date_time_io.html
/// \return
std::string formatBoostPTime(const pt::ptime &t, const char *format);

/// \brief Returns Current date time
/// \return string formatted in ISO-8601 format
std::string getNowISODateTime();

/// \brief Format bytes to human readable string
/// \param bytes Bytes count
/// \param si Internationl SI standard or binary format
/// \return Human readable bytes. Example: 1024 will be 1kB in SI or 1KiB in binary, 7077888 bytes will be 7.1MB or 6.8MiB in binary
std::string humanReadableBytes(unsigned long bytes, bool si = true);

/// \brief Generates random crc32 string
/// \param length Generated string length. Default = 16 chars
/// \return Random alpha-num string wrapped with crc32 hash
const std::string generateRandomStringCRC32(unsigned short length = 16);

}
}

#endif //WSSERVER_DATEHELPER_HPP
