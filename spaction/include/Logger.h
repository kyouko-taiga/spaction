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

/// A class that represents a Cost LTL formula.
/// @remarks
///     This class is a singleton, but is templated with the output stream it
///     should wright to. As a result, it will initialize once per output
///     stream it is compiled with.
/// @remarks
///     This singleton class is thread-safe.
template<std::ostream &os> class Logger {
public:
    /// Enumeration of the log levels.
    enum class LogLevel {DEBUG, INFO, WARNING, ERROR, FATAL};
    
    virtual ~Logger() {}
    
    /// Returns an instance of the logger.
    static Logger &instance() {
        std::call_once(_once_flag, [] {_instance.reset(new Logger);});
        return *_instance.get();
    }
    
    /// Logs a message with the given level.
    void log(const std::string &message, LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:
                this->debug(message);
                break;
            case LogLevel::INFO:
                this->info(message);
                break;
            case LogLevel::WARNING:
                this->warning(message);
                break;
            case LogLevel::ERROR:
                this->error(message);
                break;
            case LogLevel::FATAL:
                this->fatal(message);
                break;
        }
    }
    
    /// Logs a debug information.
    void debug(const std::string &message) {
        os << this->datetime() << " [debug] " << message << std::endl;
    }
    
    /// Logs a notice.
    void info(const std::string &message) {
        os << this->datetime() << " [info] " << message << std::endl;
    }
    
    /// Logs a warning.
    void warning(const std::string &message) {
        os << this->datetime() << " [warning] " << message << std::endl;
    }
    
    /// Logs an error.
    void error(const std::string &message) {
        os << this->datetime() << " [error] " << message << std::endl;
    }
    
    /// Logs a fatal (non-recoverable) error.
    void fatal(const std::string &message) {
        os << this->datetime() << " [fatal] " << message << std::endl;
    }
    
private:
    static std::unique_ptr<Logger> _instance;
    static std::once_flag _once_flag;
    
    Logger() {}
    
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

#endif  // SPACTION_INCLUDE_LOGGER_H_
