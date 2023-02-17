#ifndef ANALYSISTHREAD_HPP
#define ANALYSISTHREAD_HPP

#include <QThread>
class WorkLauncher;


class NetworkSummary;
class Hopness;

class AnalysisThread : public QThread
{
    Q_OBJECT
public:
    static AnalysisThread* getInstance();
    void run() override;
    void registerLauncher(WorkLauncher*);

public slots:
    void newWork(const QString&);

private:
        AnalysisThread();

    //analyse nodes found for each hop number
    void _analyseHops(const NetworkSummary&, const QString& node0, int min_capacity, Hopness&);

    QList<WorkLauncher*> _launchers;
    static AnalysisThread* _instance;
};

#endif // ANALYSISTHREAD_HPP
