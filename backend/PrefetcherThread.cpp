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

  Config config;

  QJsonDocument d = QJsonDocument::fromJson(graphJson);

  QJsonObject graph_object = d.object();
  QJsonArray nodes_array = graph_object.value("nodes").toArray();
  //std::vector<Node> nodes;
  network->nodes.reserve(nodes_array.size());
  qint32 rank=0;
  //qint64 now = time(NULL);
  auto now = QDateTime::currentSecsSinceEpoch();
  auto oldest_update = now-(3600*60);
  //keep, for each node, the update date of the most reecntly updated channel. Node last_update might be older but it miht have updated channels since then
  QMap<QString, qint32> node_channels_last_update;
  QJsonArray edges_array = graph_object.value("edges").toArray();
  //std::vector<Edge> edges;
  network->edges.reserve(edges_array.size());
  for (const QJsonValue& edge_value : edges_array)
  {
      QJsonObject edge_object = edge_value.toObject();
      QString pubkey1 = edge_object.value("node1_pub").toString();
      QString pubkey2 = edge_object.value("node2_pub").toString();
      qint32 last_update = edge_object.value("last_update").toInt();
      //each policy object has a last_updae, value, but the edge value seems always the max of both, no need to read them
      auto& nclu = node_channels_last_update[pubkey1];
      nclu = std::max(nclu, last_update);
      auto& nclu2 = node_channels_last_update[pubkey2];
      nclu2 = std::max(nclu2, last_update);
  }
  for (const QJsonValue& node_value : nodes_array)
  {
      QJsonObject node_object = node_value.toObject();
      const QString pubkey = node_object.value("pub_key").toString();
      QJsonArray node_addresses = node_object.value("addresses").toArray();
      QJsonArray node_features = node_object.value("features").toArray();
      qint32 last_update = std::max(node_object.value("last_update").toInt(), node_channels_last_update[pubkey]);

      //filter nodes based on their info
      if(last_update<=oldest_update|| node_addresses.isEmpty()/* || node_features.isEmpty()*/)
      {
        //Nodes we know exist but could not reach? (only when tor is disabled?)
        config.ignored_routing_nodes.insert(pubkey);
        continue;
      }

      //check if there is a clearnet address
      bool clearnet = false;
      bool tor = false;
      for(const QJsonValue& address : node_addresses)
      {
          const QJsonObject addr_obj = address.toObject();
          const QString addr_str = addr_obj.value("addr").toString();
          if(addr_str.count('.')==3)
          {
              clearnet = true;
          }
          if(addr_str.contains("onion"))
          {
              tor = true;
          }
          if(tor && clearnet)
              break;
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
      node.tor = tor;
      node.alias = node_object.value("alias").toString();
      network->pubkey_index[node.pubKey] = rank;
      network->alias_index.insertMulti(node.alias, rank);
      ++rank;
  }

  //QJsonArray edges_array = graph_object.value("edges").toArray();
  //std::vector<Edge> edges;
  network->edges.reserve(edges_array.size());
  rank = 0;
  for (const QJsonValue& edge_value : edges_array)
  {
      QJsonObject edge_object = edge_value.toObject();
      QString pubkey1 = edge_object.value("node1_pub").toString();
      QString pubkey2 = edge_object.value("node2_pub").toString();

      if(config.ignored_routing_nodes.contains(pubkey1) || config.ignored_routing_nodes.contains(pubkey2))
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

      qint32 node1_rank = network->pubkey_index[pubkey1];
      qint32 node2_rank = network->pubkey_index[pubkey2];
      Node& node1 = network->nodes[node1_rank];
      Node& node2 = network->nodes[node2_rank];

      //analyze zbf for the channel
      auto is_zbf = [](const QJsonValue& policy) -> bool
      {
        auto fee = atoi(policy.toObject().value("fee_base_msat").toString().toUtf8().constData());
        if(fee == 0)
          return true;
        return false;
      };

      network->edges.push_back(Edge());
      Edge& edge = network->edges.back();
      edge.side[0].node_rank=node1_rank;
      edge.side[0].base_fee_msat = atoi(policy1.toObject().value("fee_base_msat").toString().toUtf8().constData());
      edge.side[0].feerate_msat = atoi(policy1.toObject().value("fee_rate_milli_msat").toString().toUtf8().constData());
      edge.side[0].inbound_base_fee_msat = atoi(policy1.toObject().value("inbound_fee_base_msat").toString().toUtf8().constData());
      edge.side[0].inbound_feerate_msat = atoi(policy1.toObject().value("inbound_fee_rate_milli_msat").toString().toUtf8().constData());
      edge.side[0].max_htlc_msat = atoll(policy1.toObject().value("max_htlc_msat").toString().toUtf8().constData());
      edge.side[0].disabled = policy1.toObject().value("disabled").toBool();
      edge.side[0].isZbf = is_zbf(policy1);
      edge.side[1].node_rank= node2_rank;
      edge.side[1].base_fee_msat = atoi(policy2.toObject().value("fee_base_msat").toString().toUtf8().constData());
      edge.side[1].feerate_msat = atoi(policy2.toObject().value("fee_rate_milli_msat").toString().toUtf8().constData());
      edge.side[1].inbound_base_fee_msat = atoi(policy2.toObject().value("inbound_fee_base_msat").toString().toUtf8().constData());
      edge.side[1].inbound_feerate_msat = atoi(policy2.toObject().value("inbound_fee_rate_milli_msat").toString().toUtf8().constData());
      edge.side[1].max_htlc_msat = atoll(policy2.toObject().value("max_htlc_msat").toString().toUtf8().constData());
      edge.side[1].disabled = policy2.toObject().value("disabled").toBool();
      edge.side[1].isZbf = is_zbf(policy2);
      QString cap_val = edge_object.value("capacity").toString();
      edge.capacity = atoi(cap_val.toUtf8().constData());

      network->total_capacity += edge.capacity;
      node1.edges.push_back(rank);
      node2.edges.push_back(rank);
      node1.minChanSize = std::min(node1.minChanSize, edge.capacity);
      node2.minChanSize = std::min(node2.minChanSize, edge.capacity);
      node1.totalCapacity += edge.capacity;
      node2.totalCapacity += edge.capacity;

      if(node1.isZbf && !is_zbf(policy1))
          node1.isZbf = false;
      if(node2.isZbf && !is_zbf(policy2))
          node2.isZbf = false;


      ++rank;
  }

  //For each node, compute median edge (proportional) fee on the node side
  for(size_t in=0, ncnt = network->nodes.size(); in < ncnt; ++in)
  {
      Node& node = network->nodes[in];
      size_t ecnt = node.edges.size();
      if(ecnt == 0)
          continue;
      std::vector<int> channels_feerate(ecnt);
      std::vector<int> channels_basefee(ecnt);
      for(size_t ie=0;ie<ecnt; ++ie){
          const Edge& edge = network->edges[node.edges[ie]];
          int node_side = edge.side[0].node_rank == in? 0 : 1;
          channels_feerate[ie] = edge.side[node_side].feerate_msat;
          channels_basefee[ie] = edge.side[node_side].base_fee_msat;
      }
      std::sort(channels_feerate.begin(), channels_feerate.end());
      //median rank : midle (rounded up if pair number)
      int middle_rank = qRound((ecnt+0.5)/2);
      node.median_feerate_ppm = channels_feerate[middle_rank];
      node.median_basefee_ppm = channels_basefee[middle_rank];
  }

  qWarning()<<(QString("%1 Nodes / %2 channels / %3BTC")
               .arg(network->nodes.size())
               .arg(network->edges.size())
               .arg(network->total_capacity/100000000.0));

  network->lns_noderank = network->pubkey_index.value(config.lns_pubkey, -1);

  for (const Node& node : network->nodes)
  {
      if(node.isZbf)
          ++network->zbfNodes;
  }
  for (const Edge& edge : network->edges)
  {
      if(edge.side[0].isZbf && edge.side[1].isZbf)
          ++network->zbfEdges;
  }


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
