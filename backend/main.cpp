#include <QJsonObject>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBitArray>
#include <time.h>

#include <vector>
#include <list>
#include <set>

#include <LndThread.hpp>
#include <MainThread.hpp>
#include <MainWindow.hpp>
#include <PrefetcherThread.hpp>
#include <QApplication>
#include <RestThread.hpp>

#include <AnalysisThread.hpp>
#include <ui_main.h>

/*typedef enum
{
    UNKNOWN,
    NEARER,
    SAME,
    FURTHER
}EdgeDirection;

class Node
{
public:
    QString pubKey;
    std::vector<qint32> edges;
    std::vector<EdgeDirection> edges_direction; //is edge bringing nearer, further, or at same dist from us?
    bool clearnet=false;
    int min_chan_size=INT_MAX;
    QString alias;
};
class Edge
{
public:
  //Node ranks. First is the smallest, not the initiator
  qint32 node1, node2;
  qint32 capacity;
  int capacity_counted=-1;
};*/
class Island
{
public:
    QSet<qint32> nodes;
    QVector<qint32> edges;
};

//Nodes not uesable. Do not use them and their edges in graph analysis
/*QSet<QString> ignored_routing_nodes;
QSet<QString> ignored_endpoint_nodes=
{
    //satoshibox, channel disabled half of the time, could not do initial rebalance and no traffic for more than a week
    "03fa3be3b4528c1d588e60060d7f10e37c38f5aa02f867632a5c7b816cf5afa775",
    //NateNate60s-node, tcp connection refused when adding as peer
    "033a7fca1cb089b33ac232dd73c2c32aeacffe16319b72bec16208f3a9002e6d17",
    //Satoshi Radio Lightning Node 2, Not really accepting clearnet connection
    "03dc8f04a944b498caca1bd666d09184d5a180a50818316ea447508a5940800b40"

};*/


int main(int argc, char** argv)
{
  LndThread lnd_thread;
  lnd_thread.setLncliCmd("/softs/lnd/current/lncli");
  lnd_thread.start();

  PrefetcherThread* prefetcher_thread(PrefetcherThread::getInstance());
  prefetcher_thread->start();
  QObject::connect(&lnd_thread, &LndThread::newGraphJson, prefetcher_thread, &PrefetcherThread::analyzeGraph);

  RestThread rest_thread;
  rest_thread.start();

  AnalysisThread::getInstance()->start();

  QApplication app(argc, argv);
  MainWindow *mw = new MainWindow();

  mw->show();
  return app.exec();
  /*

  //ignored_endpoint_nodes.append("");

  //find current node
  //QString node0_pubkey="03c436af41160a355fc1ed230a64f6a64bcbd2ae50f12171d1318f9782602be601";//""02c521e5e73e40bad13fb589635755f674d6a159fd9f7b248d286e38c3a46f8683";
  QString node0_pubkey="02c521e5e73e40bad13fb589635755f674d6a159fd9f7b248d286e38c3a46f8683";
  int node0_rank = node_index.value(node0_pubkey, -1);
  if(node0_rank<0)
  {
      printf("Target node not found\n");
      return 1;
  }
  printf("Target node found\n");
  Node& node0 = nodes[node0_rank];

  //loop on basic node connectivity at each hop depth


  //for each node, minimum search depth (hop count-1) to reach it
  QVector<int> reached_nodes(nodes.size(), -1);
  reached_nodes[node0_rank] = 0;//1-based depth where the node was reached (number of hops, 0 for root, -1 for unreached)

  //Look for islands of nodes connected by at least the given capacity
  QList<Island> islands;
  auto find_island = [&islands](int node)
  {
    for (int i=0, cnt=islands.size(); i<cnt; ++i)
    {
      if(islands.at(i).nodes.contains(node))
          return i;
    }
    return -1;
  };
  auto merge_islands = [&islands](int i1, int i2)
  {
    islands[i1].nodes.unite(islands[i2].nodes);
    islands[i1].edges.append(islands[i2].edges);
    islands.removeAt(i2);
  };

  for(int i=0, cnt=edges.size(); i<cnt; ++i)
  {
    Edge& edge = edges[i];
    if(edge.capacity<min_edge_capacity)
        continue;
    int n1_island = find_island(edge.node1);
    int n2_island = find_island(edge.node2);
    if(n1_island>=0 && n2_island<0)
    {
        islands[n1_island].nodes.insert(edge.node2);
        islands[n1_island].edges.append(i);
    }
    else if(n2_island>=0 && n1_island<0)
    {
        islands[n2_island].nodes.insert(edge.node1);
        islands[n2_island].edges.append(i);
    }
    else if(n1_island<0 && n2_island<0)
    {
        islands.push_back(Island());
        Island& isle = islands.last();
        isle.nodes.insert(edge.node1);
        isle.nodes.insert(edge.node2);
        isle.edges.append(i);
    }
    else
    {
        if(n1_island != n2_island)
          merge_islands(n1_island, n2_island);
        else
        {
            islands[n1_island].edges.append(i);
        }
    }
  }
  printf("%d islands of 2M channels have been found\n", islands.size());
  int node0_island = -1;
  //Calculate capacity of those islands. Search island of target node
  QMultiMap<qint64, qint32> islands_cap;
  {
      for (int i=0, cnt=islands.size();i<cnt;++i)
      {
          const Island& isle = islands[i];
          qint64 capacity = 0;
          for(int j=0; j<isle.edges.size();++j)
          {
              if(edges[isle.edges[j]].capacity>=min_edge_capacity)
                capacity += edges[isle.edges[j]].capacity;
          }
          islands_cap.insert(capacity, i);
          if(node0_island<0 && isle.nodes.contains(node0_rank))
              node0_island = i;
      }
  }
  //print, ordered by increasing capacity
  QMapIterator<qint64, qint32> it(islands_cap);
  while(it.hasNext())
  {
      it.next();
      printf("Island %d : %lld sats (%d nodes, %d edges)\n", it.value(), it.key(), islands[it.value()].nodes.size(), islands[it.value()].edges.size());
  }
  printf("Node 0(%d) is on island %d (%d nodes, %d edges)\n", node0_rank, node0_island, islands[it.value()].nodes.size(), islands[it.value()].edges.size());
  if(islands.size()>1)
  {
      //Find first bigest island that is not ours
      it.toBack();
      int target_island_id = -1;
      while(it.hasPrevious())
      {
          it.previous();
          if(it.value() != node0_island)
          {
              target_island_id = it.value();
              break;
          }
      }
      if(target_island_id>=0)
      {
          printf("Target island : %d\n", target_island_id);
          Island& isle = islands[target_island_id];
          printf("Island has  %d nodes\n", isle.nodes.size());

          //find center of that island
          const int   N = isle.nodes.size(); // Number of nodes in graph
          const int   INF = -1; // Infinity
          std::vector<std::vector<qint32>> d(N); // Distances between nodes
          std::vector<int> e(N); // Eccentricity of nodes
          QSet <int>   c; // Center of graph
          int         rad = INF; // Radius of graph
          int         diam; // Diamater of graph

          //reindex nodes with only nodes in this island
          QVector<qint32> n_ind;
          QMap<qint32,qint32> n_rind;
          n_ind.reserve(N);
          QSetIterator<qint32> n_it(isle.nodes);
          while(n_it.hasNext())
          {
              int nodeid = n_it.next();
              n_ind.append(nodeid);
              int i = n_ind.size()-1;
              d[i].resize(N,0xBABABABA);              
              n_rind[nodeid] = n_ind.size()-1;
          }
          //fill distances of each edge
          for(int i=0; i<isle.edges.size();++i)
          {
              int n1 = n_rind[edges[isle.edges[i]].node1];
              int n2 = n_rind[edges[isle.edges[i]].node2];
              d[n1][n2] = edges[isle.edges[i]].capacity;
              d[n2][n1] = edges[isle.edges[i]].capacity;
          }

          // Floyd-Warshall's algorithm
          for (int k = 0; k < N; k++) {
              for (int j = 0; j < N; j++) {
                  for (int i = 0; i < N; i++) {
                      d[i][j] = std::min(d[i][j], d[i][k] + d[k][j]);
                  }
              }
          }

          // Counting values of eccentricity
          for (int i = 0; i < N; i++) {
              for (int j = 0; j < N; j++) {
                  e[i] = std::max(e[i], d[i][j]);
              }
          }

          for (int i = 0; i < N; i++) {
              rad = std::min(rad, e[i]);
              diam = std::max(diam, e[i]);
          }

          for (int i = 0; i < N; i++) {
              if (e[i] == rad) {
                  c.insert(i);
                  printf("%d\n", i);
              }
          }

          //print center
          puts("Done");
      }
  }*/


  return 0;
}
