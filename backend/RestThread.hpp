#ifndef RESTTHREAD_HPP
#define RESTTHREAD_HPP

#include <Logger.hpp>
#include <QThread>

class RestThread : public QThread
{
public:
    RestThread();
    void run() override;

    static QScopedPointer<Logger> logger;
};

#endif // RESTTHREAD_HPP
