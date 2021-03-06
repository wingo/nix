#pragma once

#include "types.hh"

namespace nix {

typedef enum {
    lvlError = 0,
    lvlInfo,
    lvlTalkative,
    lvlChatty,
    lvlDebug,
    lvlVomit
} Verbosity;

typedef enum {
    actUnknown = 0,
    actCopyPath = 100,
    actDownload = 101,
    actRealise = 102,
    actCopyPaths = 103,
    actBuilds = 104,
    actBuild = 105,
    actOptimiseStore = 106,
    actVerifyPaths = 107,
} ActivityType;

typedef enum {
    resFileLinked = 100,
    resBuildLogLine = 101,
    resUntrustedPath = 102,
    resCorruptedPath = 103,
} ResultType;

typedef uint64_t ActivityId;

class Logger
{
    friend class Activity;

public:

    virtual ~Logger() { }

    virtual void log(Verbosity lvl, const FormatOrString & fs) = 0;

    void log(const FormatOrString & fs)
    {
        log(lvlInfo, fs);
    }

    virtual void warn(const std::string & msg);

    virtual void startActivity(ActivityId act, ActivityType type, const std::string & s) { };

    virtual void stopActivity(ActivityId act) { };

    virtual void progress(ActivityId act, uint64_t done = 0, uint64_t expected = 0, uint64_t running = 0, uint64_t failed = 0) { };

    virtual void setExpected(ActivityId act, ActivityType type, uint64_t expected) { };

    struct Field
    {
        // FIXME: use std::variant.
        enum { tInt, tString } type;
        uint64_t i = 0;
        std::string s;
        Field(const std::string & s) : type(tString), s(s) { }
        Field(const char * s) : type(tString), s(s) { }
        Field(const uint64_t & i) : type(tInt), i(i) { }
    };

    virtual void result(ActivityId act, ResultType type, const std::vector<Field> & fields) { };
};

struct Activity
{
    Logger & logger;

    const ActivityId id;

    Activity(Logger & logger, ActivityType type, const std::string & s = "");

    ~Activity()
    { logger.stopActivity(id); }

    void progress(uint64_t done = 0, uint64_t expected = 0, uint64_t running = 0, uint64_t failed = 0) const
    { logger.progress(id, done, expected, running, failed); }

    void setExpected(ActivityType type2, uint64_t expected) const
    { logger.setExpected(id, type2, expected); }

    template<typename... Args>
    void result(ResultType type, const Args & ... args)
    {
        std::vector<Logger::Field> fields;
        nop{(fields.emplace_back(Logger::Field(args)), 1)...};
        logger.result(id, type, fields);
    }

    friend class Logger;
};

extern Logger * logger;

Logger * makeDefaultLogger();

extern Verbosity verbosity; /* suppress msgs > this */

/* Print a message if the current log level is at least the specified
   level. Note that this has to be implemented as a macro to ensure
   that the arguments are evaluated lazily. */
#define printMsg(level, args...) \
    do { \
        if (level <= nix::verbosity) { \
            logger->log(level, fmt(args)); \
        } \
    } while (0)

#define printError(args...) printMsg(lvlError, args)
#define printInfo(args...) printMsg(lvlInfo, args)
#define printTalkative(args...) printMsg(lvlTalkative, args)
#define debug(args...) printMsg(lvlDebug, args)
#define vomit(args...) printMsg(lvlVomit, args)

template<typename... Args>
inline void warn(const std::string & fs, Args... args)
{
    boost::format f(fs);
    nop{boost::io::detail::feed(f, args)...};
    logger->warn(f.str());
}

void warnOnce(bool & haveWarned, const FormatOrString & fs);

void writeToStderr(const string & s);

}
