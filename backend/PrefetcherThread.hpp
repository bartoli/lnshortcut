#ifndef PREFETCHERTHREAD_H
#define PREFETCHERTHREAD_H

class Hopness;
class NetworkSummary;

#include <QSet>
#include <QString>
#include <QThread>

/*
 * Thread for pre-analysing the network graph, in order to make actual analysis for a node faster
 */
class PrefetcherThread : public QThread
{
public:
    static PrefetcherThread* getInstance();
    ~PrefetcherThread();
    void run() override;

    //Latest fullty analyzed network info
    NetworkSummary* _currentNetwork = nullptr;

public slots:
    void analyzeGraph(int block_height, const QByteArray& graphJson);

private:
        PrefetcherThread();
        static PrefetcherThread* _instance;

};

#endif // PREFETCHERTHREAD_H
