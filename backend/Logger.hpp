#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QFile>
#include <QString>

#define MYLOG(type, message) {if(!Logger::type.isNull()) Logger::type->log(message);}

/*
 * Basic clas for logging to file
 * For now, it just concatenates messages to the given file, and flushes the write buffer immediately to avoid losing logs at crashes
 *
*/
class Logger
{
public:
    Logger(const QString& logPath);
    ~Logger();
    void log(const QByteArray& message);

    static QScopedPointer<Logger> routeLogger;
private:
    QFile _file;
};

#endif // LOGGER_HPP
