#ifndef ANALYSISTHREAD_HPP
#define ANALYSISTHREAD_HPP

#include <QThread>
#include <vector>

class WorkLauncher;


class NetworkSummary;
class Hopness;

typedef struct
{
  int node2, node3;
  uint64_t cap2, cap3;

} Result;

class AnalysisThread : public QThread
{
    Q_OBJECT
public:
    static AnalysisThread* getInstance();
    void run() override;
    void registerLauncher(WorkLauncher*);

    //analyse nodes found for each hop number
    static void analyseHops(const NetworkSummary&, const QString& node0, int min_capacity, Result&);

public slots:
    void newWork(const QString&);

private:
    AnalysisThread();

    QList<WorkLauncher*> _launchers;
    static AnalysisThread* _instance;
};

typedef struct
{
  //number of nodes reach for each new hop level
  std::vector<int> reachable_nodes;
  std::vector<qint64> reachable_capacity;
} NodeReach;

typedef struct
{
    QString pubkey;
    QString alias;
    int n_channels;
    int n_distinct_peers;
    qint64 min_chan_size;
    qint64 avec_chan_size;
    qint64 max_chan_size;
    NodeReach node_reach;
} NodeInfoResult;

#endif // ANALYSISTHREAD_HPP
