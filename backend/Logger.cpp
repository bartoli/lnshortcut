#include "Logger.hpp"

#include <QDateTime>
#include <QCoreApplication>
#include <QString>

Logger::Logger(const QString& path)
 : _file(QFile(path)){
  _file.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append);
  log(QString("Logger started for PID %1").arg(QCoreApplication::applicationPid()).toUtf8());
}

Logger::~Logger()
{
    log(QString("Logger stopped for PID %1").arg(QCoreApplication::applicationPid()).toUtf8());
}

void Logger::log(const QByteArray& msg)
{
  //deleted while runing? Try to recreate it
  if(!_file.exists())
  {
      if(!_file.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append))
          return; //or throw?
  }
  _file.write(QDateTime::currentDateTimeUtc().toString().toUtf8() + ": " + msg + "\n");
  _file.flush();
}

#ifdef QT_DEBUG
  QScopedPointer<Logger> Logger::routeLogger(new Logger("/tmp/route.log"));
#else
  QScopedPointer<Logger> Logger::routeLogger;
#endif
