#include "NetworkSummary.hpp"
#include "PrefetcherThread.hpp"

#include <Config.hpp>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

PrefetcherThread* PrefetcherThread::_instance = nullptr;

PrefetcherThread* PrefetcherThread::getInstance()
{
    if(!_instance)
    {
        _instance = new PrefetcherThread();
    }
    return _instance;
}

PrefetcherThread::PrefetcherThread()
{}

PrefetcherThread::~PrefetcherThread()
{}

void PrefetcherThread::analyzeGraph(int block_height, const QByteArray& graphJson)
{
  QScopedPointer<NetworkSummary> network(new NetworkSummary());
  network->block_height = block_height;

  Config* config = Config::getInstance();

  QJsonDocument d = QJsonDocument::fromJson(graphJson);

  QJsonObject graph_object = d.object();
  QJsonArray nodes_array = graph_object.value("nodes").toArray();
  //std::vector<Node> nodes;
  network->nodes.reserve(nodes_array.size());
  qint32 rank=0;
  //qint64 now = time(NULL);
  for (const QJsonValue& node_value : nodes_array)
  {
      QJsonObject node_object = node_value.toObject();
      const QString pubkey = node_object.value("pub_key").toString();
      QJsonArray node_addresses = node_object.value("addresses").toArray();
      QJsonArray node_features = node_object.value("features").toArray();
      qint32 last_update = node_object.value("last_update").toInt();

      //filter nodes based on their info
      if(last_update<=0/*now-(3600*100)*/|| node_addresses.isEmpty()/* || node_features.isEmpty()*/)
      {
        //Nodes we know exist but could not reach? (only when tor is disabled?)
        config->ignored_routing_nodes.insert(pubkey);
        continue;
      }

      //check if there is a clearnet address
      bool clearnet = false;
      for(const QJsonValue& address : node_addresses)
      {
          const QJsonObject addr_obj = address.toObject();
          if(addr_obj.value("addr").toString().count('.')==3)
          {
              clearnet = true;
              break;
          }
      }

      /*for(const QJsonValue& address : node_addresses)
      {
          QJsonObject address_object = address.toObject();
          QString addr = address_object.value("addr").toString();
          if(addr.startsWith("105."))
              printf("%s %s\n", pubkey.toUtf8().constData(), addr.toUtf8().constData());
      }*/

      //add accesible nodes
      network->nodes.push_back(Node());
      Node& node(network->nodes.back());
      node.pubKey = pubkey;
      node.clearnet = clearnet;
      node.alias = node_object.value("alias").toString();
      network->node_index[node.pubKey] = rank;
      ++rank;
  }

  QJsonArray edges_array = graph_object.value("edges").toArray();
  //std::vector<Edge> edges;
  network->edges.reserve(edges_array.size());
  rank = 0;
  for (const QJsonValue& edge_value : edges_array)
  {
      QJsonObject edge_object = edge_value.toObject();
      QString pubkey1 = edge_object.value("node1_pub").toString();
      QString pubkey2 = edge_object.value("node2_pub").toString();

      if(config->ignored_routing_nodes.contains(pubkey1) || config->ignored_routing_nodes.contains(pubkey2))
      {
        continue;
      }
      const QJsonValue policy1 = edge_object.value("node1_policy");
      const QJsonValue policy2 = edge_object.value("node2_policy");

      //filter channels where both nodeX_policy are not null and with attribute disabled=false
      auto valid_policy = [](const QJsonValue& policy) -> bool
      {
        if(policy==QJsonValue::Null)
            return false;
        auto object = policy.toObject();
        if(object.value("disabled").toBool())
            return false;
        return true;
      };
      if(!(valid_policy(policy1) || valid_policy(policy2)))
          continue;

      qint32 node1_rank = network->node_index[pubkey1];
      qint32 node2_rank = network->node_index[pubkey2];
      Node& node1 = network->nodes[node1_rank];
      Node& node2 = network->nodes[node2_rank];

      network->edges.push_back(Edge());
      Edge& edge = network->edges.back();
      edge.node1 = std::min(node1_rank, node2_rank);
      edge.node2 = std::max(node1_rank, node2_rank);
      QString cap_val = edge_object.value("capacity").toString();
      edge.capacity = atoi(cap_val.toUtf8().constData());

      network->total_capacity += edge.capacity;
      node1.edges.push_back(rank);
      node2.edges.push_back(rank);
      node1.minChanSize = std::min(node1.minChanSize, edge.capacity);
      node2.minChanSize = std::min(node2.minChanSize, edge.capacity);


      //analyze zbf for the channel
      auto is_zbf = [](const QJsonValue& policy) -> bool
      {
        auto object = policy.toObject();
        if(object.value("fee_base_msat").toInt() == 0)
          return true;
        return true;
      };
      if(node1.isZbf && !is_zbf(policy1))
          node1.isZbf = false;
      if(node2.isZbf && !is_zbf(policy2))
          node2.isZbf = false;


      ++rank;
  }

  qWarning()<<(QString("%1 Nodes / %2 channels / %3BTC")
               .arg(network->nodes.size())
               .arg(network->edges.size())
               .arg(network->total_capacity/100000000.0));

  network->lns_noderank = network->node_index.value(config->lns_pubkey, -1);


  //TODO Add lock to avide deletig a result that is still used in an analyse
  NetworkSummary* old = _currentNetwork;
  _currentNetwork=network.take();
  delete old;
}

void PrefetcherThread::run()
{
    while(1)
        sleep(1);
}
