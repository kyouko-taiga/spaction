// This source file is part of spaction
//
// Copyright 2014 Software Modeling and Verification Group
// University of Geneva
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SPACTION_INCLUDE_LOGGER_H_
#define SPACTION_INCLUDE_LOGGER_H_

#include <ctime>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>

namespace spaction {

/// A ostream that outputs nothing.
//@{
/// @remarks
///             the code below is taken from
///             http://stackoverflow.com/questions/760301/implementing-a-no-op-stdostream
///             I just beautified it to match our style, and turned it to a singleton.
template<class cT, class traits = std::char_traits<cT>>
class basic_nullbuf: public std::basic_streambuf<cT, traits> {
    typename traits::int_type overflow(typename traits::int_type c) override {
        return traits::not_eof(c);  // indicate success
    }
};

template<class cT, class traits = std::char_traits<cT>>
class basic_nullstream: public std::basic_ostream<cT, traits> {
 public:
    basic_nullstream()
    : std::basic_ios<cT, traits>(&_m_sbuf)
    , std::basic_ostream<cT, traits>(&_m_sbuf) {
        this->init(&_m_sbuf);
    }

    static basic_nullstream &instance() {
        std::call_once(_once_flag, [] {_instance.reset(new basic_nullstream);});
        return *_instance.get();
    }

 private:
    basic_nullbuf<cT, traits> _m_sbuf;

    static std::unique_ptr<basic_nullstream> _instance;
    static std::once_flag _once_flag;
};

typedef basic_nullstream<char>      nullstream;
typedef basic_nullstream<wchar_t>   wnullstream;

template<class cT, class traits> std::unique_ptr<basic_nullstream<cT, traits>> basic_nullstream<cT, traits>::_instance;
template<class cT, class traits> std::once_flag basic_nullstream<cT, traits>::_once_flag;
//@}

/// A class for logging.
/// @remarks
///     This class is a singleton, but is templated with the output stream it
///     should wright to. As a result, it will initialize once per output
///     stream it is compiled with.
/// @remarks
///     This singleton class is thread-safe.
template<std::ostream &os> class Logger {
public:
    /// Enumeration of the log levels.
    enum class LogLevel: char {
        kDEBUG      = '4',
        kINFO       = '3',
        kWARNING    = '2',
        kERROR      = '1',
        kFATAL      = '0'
    };

    virtual ~Logger() {}

    /// Returns an instance of the logger.
    static Logger &instance() {
        std::call_once(_once_flag, [] {_instance.reset(new Logger);});
        return *_instance.get();
    }

    /// Logs a message with the given level.
    std::ostream & log(LogLevel level) {
        switch (level) {
            case LogLevel::kDEBUG:
                return this->debug();
                break;
            case LogLevel::kINFO:
                return this->info();
                break;
            case LogLevel::kWARNING:
                return this->warning();
                break;
            case LogLevel::kERROR:
                return this->error();
                break;
            case LogLevel::kFATAL:
                return this->fatal();
                break;
        }
    }

    /// Logs a debug information.
    std::ostream & debug() {
        if (_loglevel < LogLevel::kDEBUG)
            return nullstream::instance();
        os << this->datetime() << " [debug] ";
        return os;
    }

    /// Logs a notice.
    std::ostream & info() {
        if (_loglevel < LogLevel::kINFO)
            return nullstream::instance();
        os << this->datetime() << " [info] ";
        return os;
    }

    /// Logs a warning.
    std::ostream & warning() {
        if (_loglevel < LogLevel::kWARNING)
            return nullstream::instance();
        os << this->datetime() << " [warning] ";
        return os;
    }

    /// Logs an error.
    std::ostream & error() {
        if (_loglevel < LogLevel::kERROR)
            return nullstream::instance();
        os << this->datetime() << " [error] ";
        return os;
    }

    /// Logs a fatal (non-recoverable) error.
    std::ostream & fatal() {
        if (_loglevel < LogLevel::kFATAL)
            return nullstream::instance();
        os << this->datetime() << " [fatal] ";
        return os;
    }

    /// Sets the verbosity level.
    /// @remarks
    ///             level 0 catches only fatal (non-recoverable) errors.
    ///             default is 3, and recommended for most usage.
    ///             debug is 4.
    void set_verbose(LogLevel l) {
        _loglevel = l;
    }

    inline bool is_fatal() const { return _loglevel >= LogLevel::kFATAL; }
    inline bool is_error() const { return _loglevel >= LogLevel::kERROR; }
    inline bool is_warning() const { return _loglevel >= LogLevel::kWARNING; }
    inline bool is_info() const { return _loglevel >= LogLevel::kINFO; }
    inline bool is_debug() const { return _loglevel >= LogLevel::kDEBUG; }

private:
    /// the current verbosity level (default is INFO)
    LogLevel _loglevel;

    static std::unique_ptr<Logger> _instance;
    static std::once_flag _once_flag;

    Logger(): _loglevel(LogLevel::kINFO) {}

    const std::string datetime() {
        std::time_t raw_time;
        std::tm *time_info;
        char buffer[80];

        std::time(&raw_time);
        time_info = std::localtime(&raw_time);

        std::strftime(buffer, 80, "[%Y-%m-%d %H:%M:%S]", time_info);
        return std::string(buffer);
    }
};

template<std::ostream &os> std::unique_ptr<Logger<os>> Logger<os>::_instance;
template<std::ostream &os> std::once_flag Logger<os>::_once_flag;

}  // namespace spaction

#define LOG_FATAL   if (spaction::Logger<std::cerr>::instance().is_fatal())     spaction::Logger<std::cerr>::instance().fatal()
#define LOG_ERROR   if (spaction::Logger<std::cerr>::instance().is_error())     spaction::Logger<std::cerr>::instance().error()
#define LOG_WARNING if (spaction::Logger<std::cerr>::instance().is_warning())   spaction::Logger<std::cerr>::instance().warning()
#define LOG_INFO    if (spaction::Logger<std::cerr>::instance().is_info())      spaction::Logger<std::cerr>::instance().info()
#define LOG_DEBUG   if (spaction::Logger<std::cerr>::instance().is_debug())     spaction::Logger<std::cerr>::instance().debug()

#endif  // SPACTION_INCLUDE_LOGGER_H_
