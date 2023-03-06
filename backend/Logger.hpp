#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QFile>
#include <QString>



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
private:
    QFile _file;
};

#endif // LOGGER_HPP
