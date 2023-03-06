#include "Logger.hpp"

#include <QDateTime>


Logger::Logger(const QString& path)
 : _file(QFile(path)){
  _file.open(QIODevice::WriteOnly|QIODevice::Text);
}

void Logger::log(const QByteArray& msg)
{
  _file.write(QDateTime::currentDateTimeUtc().toString().toUtf8() + ": " + msg + "\n");
  _file.flush();
}
