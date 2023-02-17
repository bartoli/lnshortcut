#ifndef WORKLAUNCHER_HPP
#define WORKLAUNCHER_HPP

#include <QObject>

//interface class for the classes tht can trigger a new analysis job

class WorkLauncher// : public QObject
{
    //Q_OBJECT
public:
    WorkLauncher();
//signals:
    void newWorkRequest(const QString& pubkey);
};

#endif // WORKLAUNCHER_HPP
