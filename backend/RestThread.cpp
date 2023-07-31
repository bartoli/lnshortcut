#include <RestThread.hpp>
#include <Analyse_capacityPerHop.hpp>


#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/version.h"
#include "cpprest/asyncrt_utils.h"
#include "cpprest/filestream.h"
#include "cpprest/containerstream.h"
#include "cpprest/producerconsumerstream.h"

using namespace std;
using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;

QScopedPointer<Logger> RestThread::logger;

class handler
{
    public:
        handler();
        handler(utility::string_t url);
        virtual ~handler();

        pplx::task<void>open(){return m_listener.open();}
        pplx::task<void>close(){return m_listener.close();}

    protected:

    private:
        void handle_get(http_request message);
        void handle_put(http_request message);
        void handle_post(http_request message);
        void handle_delete(http_request message);
        void handle_error(pplx::task<void>& t);
        http_listener m_listener;
};



#include <cpprest/http_listener.h>
#include <cpprest/json.h>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

#include <LndThread.hpp>
#include <NetworkSummary.hpp>
#include <PrefetcherThread.hpp>
#include <QString>
#include <QDebug>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <AnalysisThread.hpp>
#include <Hopness.hpp>
#include <Config.hpp>
#include <ResultPool.hpp>
#include <QJsonDocument>
#include <QJsonObject>
using namespace std;

#define TRACE(msg)            cout << msg
#define TRACE_ACTION(a, k, v) cout << a << L" (" << k << L", " << v << L")\n"

map<utility::string_t, utility::string_t> dictionary;

void display_json(
   json::value const & jvalue,
   utility::string_t const & prefix)
{
   qWarning() << prefix.c_str() << jvalue.serialize().c_str() << Qt::endl;
}

void answer_nodeinfo(const QJsonObject& request_json, json::value& response_body)
{
    NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
    if (network == nullptr)
    {
        response_body["error"] = json::value("Network graph informaton not available, please retry later.");
        return;
    }

    QString node_pubkey = request_json.value("pubkey").toString();
    int o_n_edges=0;
    uint64_t o_cap_min = ULONG_MAX, o_cap_max=0, o_cap_avg=0, o_cap_ttl=0;
    int o_peers=0;
    int node_rank = network->pubkey_index.value(node_pubkey, -1);
    if(node_rank<0)
    {
        response_body["error"] = json::value(QString("Unknown node public key %1.").arg(node_pubkey).toUtf8().constData());
        return;
    }
    bool lns_peer = node_rank==network->lns_noderank;

    const Node& node = network->nodes[node_rank];
    response_body["edges"] = node.edges.size();
    QSet<int> peer_ranks;
    for(int edge_rank : node.edges)
    {
        const Edge& edge = network->edges[edge_rank];
        int other_node_rank = edge.side[0].node_rank==node_rank? edge.side[1].node_rank : edge.side[0].node_rank;
        peer_ranks.insert(other_node_rank);
        uint64_t edge_cap = edge.capacity;
        o_cap_min =std::min(o_cap_min, edge_cap);
        o_cap_max = std::max(o_cap_max, edge_cap);
        o_cap_ttl += edge_cap;
        if(other_node_rank == network->lns_noderank)
            lns_peer = true;
        o_n_edges++;
    }
    o_cap_avg = o_cap_ttl/node.edges.size();
    o_peers = peer_ranks.size();

    //tor and/tor clearnet?
    response_body["on_tor"] = node.tor;
    response_body["on_clearnet"] = node.clearnet;

    response_body["edges"] = o_n_edges;
    response_body["peers"] = o_peers;
    response_body["cap_min"] = o_cap_min;
    response_body["cap_max"] = o_cap_max;
    response_body["cap_avg"] = o_cap_avg;
    response_body["cap_total"] = o_cap_ttl;
    response_body["lns_peer"] = lns_peer;

    return;
}
void GET_nodeinfo(const QString& resource, json::value& body)
{
    QString node_pubkey = resource.mid(11);
    qWarning() <<"node_info/"<< node_pubkey;
    NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
    if (network == nullptr)
    {
        body["error"] = json::value("Network graph informaton not available, please retry later.");
        return;
    }

    int o_n_edges=0;
    uint64_t o_cap_min = ULONG_MAX, o_cap_max=0, o_cap_avg=0, o_cap_ttl=0;
    int o_peers=0;
    int node_rank = network->pubkey_index.value(node_pubkey, -1);
    if(node_rank<0)
    {
        body["error"] = json::value(QString("Unknown node public key %1.").arg(node_pubkey).toUtf8().constData());
        return;
    }
    bool lns_peer = node_rank==network->lns_noderank;

    const Node& node = network->nodes[node_rank];
    body["edges"] = node.edges.size();
    QSet<int> peer_ranks;
    for(int edge_rank : node.edges)
    {
        const Edge& edge = network->edges[edge_rank];
        int other_node_rank = edge.side[0].node_rank==node_rank? edge.side[1].node_rank : edge.side[0].node_rank;
        peer_ranks.insert(other_node_rank);
        uint64_t edge_cap = edge.capacity;
        o_cap_min =std::min(o_cap_min, edge_cap);
        o_cap_max = std::max(o_cap_max, edge_cap);
        o_cap_ttl += edge_cap;
        if(other_node_rank == network->lns_noderank)
            lns_peer = true;
        o_n_edges++;
    }
    o_cap_avg = o_cap_ttl/node.edges.size();
    o_peers = peer_ranks.size();

    //tor and/tor clearnet?
    body["on_tor"] = node.tor;
    body["on_clearnet"] = node.clearnet;

    body["edges"] = o_n_edges;
    body["peers"] = o_peers;
    body["cap_min"] = o_cap_min;
    body["cap_max"] = o_cap_max;
    body["cap_avg"] = o_cap_avg;
    body["cap_total"] = o_cap_ttl;
    body["lns_peer"] = lns_peer;

    return;
}

void answer_nodeadvice(const QJsonObject& request_json, json::value& response_body)
{
  qWarning() << request_json;
  QString pubkey = request_json.value("pubkey").toString();
  uint32_t capacity = atoll(request_json.value("capacity").toString().toUtf8().constData());

  if(capacity<=0)
  {
    response_body["error"] = json::value("Capacity should be a positive number of satoshi.");
    return;
  }

  //Is there a prefetched network graph yet?
  NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
  if (network == nullptr)
  {
      response_body["error"] = json::value("Network graph informaton not available, please retry later.");
      return;
  }
  int node0_rank = network->pubkey_index.value(pubkey,-1);
  if(node0_rank < 0)
      node0_rank = network->alias_index.value(pubkey, -1);
  if(node0_rank < 0)
  {
      response_body["error"] = json::value("Unknown node public key");
      return;
  }
  //Is it a lns peer? Is capacity not more than capacity connected to lns?
  {
      //int lns_capacity = 0;
      /*const Node& node0 = network->nodes[node0_rank];
      uint32_t max_cap = 0;
      for(const int edge_rank : node0.edges)
      {
          const Edge& edge = network->edges[edge_rank];
          max_cap = std::max(max_cap, edge.capacity);
          int other_node_rank = edge.side[0].node_rank == node_rank? edge.side[1].node_rank : edge.side[0].node_rank;
          if(other_node_rank == network->lns_noderank)
              lns_capacity += edge.capacity;
      }*/
      /*if(node_rank != network->lns_noderank && lns_capacity<cap)
      {
          body["error"] = json::value("Can't advise for channels with more capacity than the sum of the channels connected to LNSHortcut");
          return status_codes::OK;
      }*/
      // Adjust analysed capacity to max existing capacity.
      // It makes no sense to analyse for more than the curent bigger channel,
      // since we need at least 2 channels at that capacity to actually route that size
      // For the general user, it could happen if they have enough cap connected to LNS, but in multiple channels.
      // For lns node, it can happen when we analyse for a bigger capacity than the existing channels.
      // There are no nodes to reach with at least that capacity, so we are looking at empty lists of reached nodes at any hop level
      // -> estimate next channel as if it is the masx size of current channels
      //capacity = std::min(capacity, max_cap);
  }

  Config config;
  config.minRoutingCap = capacity;

  json::value results;

  //Basic node stats
  json::value stats_output;
  answer_nodeinfo(request_json, stats_output);
  stats_output["type"] = json::value("node_info");
  stats_output["label"] = json::value("Node statistics");
  results[0] = stats_output;

  /*Result result;
  AnalysisThread::analyseHops(*network, node_rank, config, result);
  response_body["node0"] = json::value(pubkey.toUtf8().constData());
  response_body["node2"] = json::value(network->nodes[result.node2].pubKey.toUtf8().constData());
  response_body["node3"] = json::value(network->nodes[result.node3].pubKey.toUtf8().constData());
  response_body["cap0"] = capacity;
  response_body["cap2"] = result.cap2;
  response_body["cap3"] = result.cap3;*/

  int test_amt_sat = atoll(request_json.value("test_amt_sat").toString().toUtf8().constData());
  //by defualt, test transaction of half wanted channel capacity
  if(test_amt_sat <=0)
      test_amt_sat = capacity/2;

  int max_fee_sat = atoll(request_json.value("max_fee_sat").toString().toUtf8().constData());

  config.zbfPaths = request_json.value("zbfEdges").toBool();
  config.zbfEndpoints = request_json.value("zbfNodes").toBool();

  config.clearnetNodes = request_json.value("clearnetNodes").toBool();
  config.torNodes = request_json.value("torNodes").toBool();
  config.clearnetEdges = request_json.value("clearnetEdges").toBool();
  config.torEdges = request_json.value("torEdges").toBool();
  //Need at least one, connect to clearnet only by default
  if (!(config.torNodes || config.clearnetNodes))
      config.clearnetNodes = true;
  int max_hops=atoi(request_json.value("maxHops").toString().toUtf8().constData());

  config.minRoutingCap = test_amt_sat;
  config.minEndpointCap = capacity;
  //analyseHops(*network, node0_rank, config, result);

  const NetworkSummary filtered_network = network->filter(config, node0_rank);
  node0_rank = filtered_network.pubkey_index.value(network->nodes[node0_rank].pubKey);
  qWarning()<<"Filtered network has "<<filtered_network.nodes.size()<<" nodes and "<<filtered_network.edges.size()<<" edges";
  response_body["workNodes"] = filtered_network.nodes.size();
  response_body["workEdges"] = filtered_network.edges.size();


  /*capacity_for_fee(filtered_network, config, node0_rank, max_fee_sat, test_amt_sat, LiquidityDirection::OUTBOUND, outbound_results);
  capacity_for_fee(filtered_network, config, node0_rank, max_fee_sat, test_amt_sat, LiquidityDirection::INBOUND, inbound_results);*/

  CFF_Params params[] = {
      {filtered_network, config, node0_rank, (uint64_t)max_fee_sat, (uint64_t)test_amt_sat, max_hops, LiquidityDirection::INBOUND},
      {filtered_network, config, node0_rank, (uint64_t)max_fee_sat, (uint64_t)test_amt_sat, max_hops, LiquidityDirection::OUTBOUND}
  };
  CFF_Result cff_results[]= {
      CFF_Result(params[0]), CFF_Result(params[1])
  };
  std::thread t2([&](){capacity_for_fee(params[LiquidityDirection::OUTBOUND], cff_results[LiquidityDirection::OUTBOUND]);});  
  std::thread t3([&](){capacity_for_fee(params[LiquidityDirection::INBOUND], cff_results[LiquidityDirection::INBOUND]);});  
  t2.join();
  t3.join();

  //Choose the side to look for candidates : side with the higfhest median fee for current reach
  auto outbound_median_cost = cff_results[LiquidityDirection::OUTBOUND].median_cost_for_original_reach[CFF_Result::RankingCategory::REFERENCE];
  auto inbound_median_cost = cff_results[LiquidityDirection::INBOUND].median_cost_for_original_reach[CFF_Result::RankingCategory::REFERENCE];
  LiquidityDirection worst_direction = outbound_median_cost>inbound_median_cost? LiquidityDirection::OUTBOUND : LiquidityDirection::INBOUND;
  //CFF_Result new_result(cff_results[worst_direction]);
  //analyze_candidates(params[worst_direction], new_result);
  std::thread t4([&](){analyze_candidates(params[0], cff_results[0]);});
  std::thread t5([&](){analyze_candidates(params[1], cff_results[1]);});
  t4.join();
  t5.join();

  auto& outbound_results = cff_results[LiquidityDirection::OUTBOUND];
  auto& inbound_results = cff_results[LiquidityDirection::INBOUND];
  response_body["outboundReachedNodes"] = outbound_results.new_reached_nodes[CFF_Result::RankingCategory::REFERENCE];
  response_body["inboundReachedNodes"] = inbound_results.new_reached_nodes[CFF_Result::RankingCategory::REFERENCE];
  response_body["outboundReachedEdges"] = outbound_results.new_reached_edges[CFF_Result::RankingCategory::REFERENCE];
  response_body["inboundReachedEdges"] = inbound_results.new_reached_edges[CFF_Result::RankingCategory::REFERENCE];
  response_body["outboundMedianCost"] = outbound_results.median_cost_for_original_reach[CFF_Result::RankingCategory::REFERENCE];
  response_body["inboundMedianCost"] = inbound_results.median_cost_for_original_reach[CFF_Result::RankingCategory::REFERENCE];
  response_body["inBoundBestCandidateForEdges"] = json::value(filtered_network.nodes[inbound_results.best_candidate_rank[CFF_Result::RankingCategory::MOST_NEW_EDGES]].pubKey.toUtf8().constData());
  response_body["inBoundBestCandidateForNodes"] = json::value(filtered_network.nodes[inbound_results.best_candidate_rank[CFF_Result::RankingCategory::MOST_NEW_NODES]].pubKey.toUtf8().constData());
  response_body["outBoundBestCandidateForEdges"] = json::value(filtered_network.nodes[outbound_results.best_candidate_rank[CFF_Result::RankingCategory::MOST_NEW_EDGES]].pubKey.toUtf8().constData());
  response_body["outBoundBestCandidateForNodes"] = json::value(filtered_network.nodes[outbound_results.best_candidate_rank[CFF_Result::RankingCategory::MOST_NEW_NODES]].pubKey.toUtf8().constData());
  response_body["inBoundNewReachedEdgesForEdges"] = inbound_results.new_reached_edges[CFF_Result::RankingCategory::MOST_NEW_EDGES];
  response_body["inBoundNewReachedEdgesForNodes"] = inbound_results.new_reached_edges[CFF_Result::RankingCategory::MOST_NEW_NODES];
  response_body["outBoundNewReachedEdgesForEdges"] = outbound_results.new_reached_edges[CFF_Result::RankingCategory::MOST_NEW_EDGES];
  response_body["outBoundNewReachedEdgesForNodes"] = outbound_results.new_reached_edges[CFF_Result::RankingCategory::MOST_NEW_NODES];
  response_body["inBoundNewReachedNodesForEdges"] = inbound_results.new_reached_nodes[CFF_Result::RankingCategory::MOST_NEW_EDGES];
  response_body["inBoundNewReachedNodesForNodes"] = inbound_results.new_reached_nodes[CFF_Result::RankingCategory::MOST_NEW_NODES];
  response_body["outBoundNewReachedNodesForEdges"] = outbound_results.new_reached_nodes[CFF_Result::RankingCategory::MOST_NEW_EDGES];
  response_body["outBoundNewReachedNodesForNodes"] = outbound_results.new_reached_nodes[CFF_Result::RankingCategory::MOST_NEW_NODES];
  response_body["outBestCandidateForCost"] = json::value(filtered_network.nodes[outbound_results.best_candidate_rank[CFF_Result::RankingCategory::BEST_MEDIAN_COST]].pubKey.toUtf8().constData());
  response_body["outBestMedianCost"] = json::value(outbound_results.median_cost_for_original_reach[CFF_Result::RankingCategory::BEST_MEDIAN_COST]);
  response_body["inBestCandidateForCost"] = json::value(filtered_network.nodes[inbound_results.best_candidate_rank[CFF_Result::RankingCategory::BEST_MEDIAN_COST]].pubKey.toUtf8().constData());
  response_body["inBestMedianCost"] = json::value(inbound_results.median_cost_for_original_reach[CFF_Result::RankingCategory::BEST_MEDIAN_COST]);
}

void GET_nodeadvice(const QString& resource, json::value& body)
{
    Config config;

    QStringList args = resource.mid(13).split("/");
    if(args.size()<2)
    {
        body["error"] = json::value("2 arguments needed.");
        return;
    }
    QString node_pubkey = args[0];
    uint32_t cap = atoi(args[1].toUtf8().constData());
    if(cap<=0)
        {
            body["error"] = json::value("Capacity should be a positive number of satoshi.");
            return;
        }

    if(args.size()>=3)
        config.zbfPaths = args[2] == "true";
    if(args.size()>=4)
        config.zbfEndpoints = args[3] == "true";

    //Is there a prefetched network graph yet?
    NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
    if (network == nullptr)
    {
        body["error"] = json::value("Network graph informaton not available, please retry later.");
        return;
    }
    int node_rank = network->pubkey_index.value(node_pubkey,-1);
    if(node_rank < 0)
    {
        body["error"] = json::value("Unknown node public key");
        return;
    }
    //Is it a lns peer? Is capacity not more than capacity connected to lns?
    {
        //int lns_capacity = 0;
        const Node& node = network->nodes[node_rank];
        uint32_t max_cap = 0;
        for(const int edge_rank : node.edges)
        {
            const Edge& edge = network->edges[edge_rank];
            max_cap = std::max(max_cap, edge.capacity);            
            /*int other_node_rank = edge.side[0].node_rank == node_rank? edge.side[1].node_rank : edge.side[0].node_rank;
            if(other_node_rank == network->lns_noderank)
                lns_capacity += edge.capacity;*/
        }
        /*if(node_rank != network->lns_noderank && lns_capacity<cap)
        {
            body["error"] = json::value("Can't advise for channels with more capacity than the sum of the channels connected to LNSHortcut");
            return status_codes::OK;
        }*/
        // Adjust analysed capacity to max existing capacity.
        // It makes no sense to analyse for more than the curent bigger channel,
        // since we need at least 2 channels at that capacity to actually route that size
        // For the general user, it could happen if they have enough cap connected to LNS, but in multiple channels.
        // For lns node, it can happen when we analyse for a bigger capacity than the existing channels.
        // There are no nodes to reach with at least that capacity, so we are looking at empty lists of reached nodes at any hop level
        // -> estimate next channel as if it is the masx size of current channels
        cap = std::min(cap, max_cap);
    }
    config.minRoutingCap = cap;


    Result result;

    AnalysisThread::analyseHops(*network, node_rank, config, result);

    body["node0"] = json::value(node_pubkey.toUtf8().constData());
    body["node2"] = json::value(network->nodes[result.node2].pubKey.toUtf8().constData());
    body["node3"] = json::value(network->nodes[result.node3].pubKey.toUtf8().constData());
    body["cap0"] = cap;
    body["cap2"] = result.cap2;
    body["cap3"] = result.cap3;

    return;
}

/*void RestThread::GET_donateInvoice(const QString& resource, json::value& body)
{
    QStringList args = resource.mid(16).split('/');
    if(args.isEmpty())
    {
        body["error"] = json::value("Missing ammount argument");
    }
    long long amt_sat = atoll(args[0].toUtf8().constData());
    if(amt_sat<=0)
    {
        body["error"] = json::value("Need positive donation ammount");
        return;
    }

    ResultPool::getInstance()->donateInvoiceRequest((intptr_t)QThread::currentThreadId(), amt_sat);
    QString result;
    ResultPool::getInstance()->waitForResult_invoice((intptr_t)QThread::currentThreadId(), result, 10, true);
    body["invoice"] = json::value(result.toLocal8Bit().constData());
}*/
void answer_donateinvoice(const QJsonObject& request_json, json::value& response_body)
{
    long long amt_sat = atoll(request_json.value("amt_sat").toString().toUtf8().constData());
    if(amt_sat<=0)
    {
        response_body["error"] = json::value("Need positive donation ammount");
        return;
    }

    ResultPool::getInstance()->donateInvoiceRequest((intptr_t)QThread::currentThreadId(), amt_sat);
    QString result;
    ResultPool::getInstance()->waitForResult_invoice((intptr_t)QThread::currentThreadId(), result, 10, true);
    response_body["invoice"] = json::value(result.toLocal8Bit().constData());
}

void answer_networkinfo(const QJsonObject& request_json, json::value& response_body)
{
    NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
    if (network == nullptr)
    {
        response_body["error"] = json::value("Network info not ready");
        return;
    }

    response_body["height"] = network->block_height;
    response_body["nodes"] = network->nodes.size();
    response_body["edges"] = network->edges.size();
    response_body["capacity"] = network->total_capacity;
    response_body["zbf_nodes"] = network->zbfNodes;
    response_body["zbf_edges"] = network->zbfEdges;
}

void handle_get(const http_request& request)
{
   TRACE("\nhandle GET\n");
   /*cout <<"c"<<request.relative_uri().path()<<endl;
   cout <<"b"<<request.relative_uri().query()<<endl;
   cout <<"a"<<request.relative_uri().resource().to_string() <<endl;*/
   QString origin=request.remote_address().c_str();
   const http_headers& headers = request.headers();
   origin = headers.find("X-Forwarded-For")->second.c_str();
   const QString resource(request.relative_uri().resource().to_string().c_str());
   RestThread::logger->log(QString("GET url=%1 thread=%2 source=%3")
                           .arg(resource)
                           .arg((intptr_t)QThread::currentThreadId())
                           .arg(origin).toUtf8());

   http_response answer = http_response(status_codes::OK);
   //Allow requests from a different IP address
   answer.headers().add("Access-Control-Allow-Origin", "*");
   auto body = json::value();
   if(resource == "/block_height")
   {
     // Get current block_height
     body["height"] = json::value::number(LndThread::getOnChainHeight());
   }else if(resource =="/network_info")
   {
       NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
       if (network == nullptr)
       {
           request.reply(status_codes::PreconditionFailed);
           return;
       }

       body["nodes"] = network->nodes.size();
       body["edges"] = network->edges.size();
       body["capacity"] = network->total_capacity;
       body["zbf_nodes"] = network->zbfNodes;
       body["zbf_edges"] = network->zbfEdges;
   }
   else if(resource.startsWith("/node_info/"))
      {
         GET_nodeinfo(resource, body);
         //request.reply(status_codes::OK);
      }
   else if(resource.startsWith("/node_advice/"))
      {
         GET_nodeadvice(resource, body);
         //request.reply(status_codes::OK);
      }
   else if(resource.startsWith("/https_test"))
      {
         body["passed"]=true;
      }
   /*else if(resource.startsWith("/donate_invoice/"))
      {
       RestThread::GET_donateInvoice(resource, body);
       //request.reply(status_codes::OK);
      }*/
   /*
    * TODO:
    * GET node_info(pubkey)
    * is_peer(pubkey)
    * node_advice(pubkey, capacity)
   */
   else
   {
       request.reply(status_codes::NotFound);
       return;
   }
   //body["test" ] = 1;
   //display_json(body, "S: ");
   answer.set_body(body);
   request.reply(answer);
}

/*void handle_request(
   http_request request,
   function<void(json::value const &, json::value &)> action)
{
   auto answer = json::value::object();

   request
      .extract_json()
      .then([&answer, &action](pplx::task<json::value> task) {
         try
         {
            auto const & jvalue = task.get();
            display_json(jvalue, "R: ");

            if (!jvalue.is_null())
            {
               action(jvalue, answer);
            }
         }
         catch (http_exception const & e)
         {
            cout << e.what() << endl;
         }
      })
      .wait();


   display_json(answer, "S: ");

   request.reply(status_codes::OK, answer);
}*/

void handle_options(const http_request& request)
{
   TRACE("\nhandle OPTIONS\n");
   http_response answer = http_response(status_codes::OK);
   //Allow requests from a different IP address
   answer.headers().add("Access-Control-Allow-Origin", "*");
   answer.headers().add("Access-Control-Allow-Headers", "*");
   auto body = json::value();
   //answer.set_body(body);
   request.reply(answer);

}

void unknown_op(const QJsonObject&, json::value& response_body)
{
    response_body["error"] = json::value("Unknown request");
};

void handle_post(const http_request& request)
{
   TRACE("\nhandle POST\n");

   /*
    * Receive analyse request from the online form
    */

   using OpMap = QMap<QString, std::function<void(const QJsonObject&, json::value&)>>;
   OpMap op_map;
   op_map["donate_invoice"] = &answer_donateinvoice;
   op_map["node_advice"] = &answer_nodeadvice;
   op_map["node_info"] = &answer_nodeinfo;
   op_map["network_info"] = &answer_networkinfo;

   request.extract_json().then([request, op_map](web::json::value body)
      {
         auto request_body(body.serialize());
         RestThread::logger->log(QString("POST json=%1 thread=%2")
                               .arg(request_body.c_str())
                               .arg((intptr_t)QThread::currentThreadId()).toUtf8());

         http_response answer = http_response(status_codes::OK);
         answer.headers().add("Access-Control-Allow-Origin", "*"); //Allow requests from a different IP address
         auto answer_body = json::value();

         QJsonDocument request_json_doc(QJsonDocument::fromJson(request_body.c_str()));
         QJsonObject request_json_obj = request_json_doc.object();
         QString op = request_json_obj.value("op").toString();

         //analyze json and route the request to the propoer code
         OpMap::const_iterator op_it = op_map.constFind(op);
         if(op_it != op_map.constEnd())
             (*op_it)(request_json_obj, answer_body);
         else
             unknown_op(request_json_obj, answer_body);

          //request.reply(status_codes::OK, body);
         RestThread::logger->log(QString("POST answer json=%1")
                               .arg(answer_body.serialize().c_str()).toUtf8()
                               /*.arg((intptr_t)QThread::currentThreadId()).toUtf8()*/);
         answer.set_body(answer_body);
         request.reply(answer);
      });




   /*handle_request(
      request,
      [](json::value const & jvalue, json::value & answer)
   {
      for (auto const & e : jvalue.as_array())
      {
         if (e.is_string())
         {
            auto key = e.as_string();
            auto pos = dictionary.find(key);

            if (pos == dictionary.end())
            {
               answer[key] = json::value::string("<nil>");
            }
            else
            {
               answer[pos->first] = json::value::string(pos->second);
            }
         }
      }
   });*/
}

void handle_put(const http_request& request)
{
   TRACE("\nhandle PUT\n");

   /*handle_request(
      request,
      [](json::value const & jvalue, json::value & answer)
   {
      qWarning()<<jvalue.as_string().c_str();
      for (auto const & e : jvalue.as_object())
      {
         if (e.second.is_string())
         {
            auto key = e.first;
            auto value = e.second.as_string();

            if (dictionary.find(key) == dictionary.end())
            {
               TRACE_ACTION("added", key, value);
               answer[key] = json::value::string("<put>");
            }
            else
            {
               TRACE_ACTION("updated", key, value);
               answer[key] = json::value::string("<updated>");
            }

            dictionary[key] = value;
         }
      }
   });*/
}

void handle_del(http_request request)
{
   TRACE("\nhandle DEL\n");

   /*handle_request(
      request,
      [](json::value const & jvalue, json::value & answer)
   {
      set<utility::string_t> keys;
      for (auto const & e : jvalue.as_array())
      {
         if (e.is_string())
         {
            auto key = e.as_string();

            auto pos = dictionary.find(key);
            if (pos == dictionary.end())
            {
               answer[key] = json::value::string("<failed>");
            }
            else
            {
               TRACE_ACTION("deleted", pos->first, pos->second);
               answer[key] = json::value::string("<deleted>");
               keys.insert(key);
            }
         }
      }

      for (auto const & key : keys)
         dictionary.erase(key);
   });*/
}

RestThread::RestThread()
{
    logger.reset(new Logger("/tmp/ln4me.log"));
}

void RestThread::run()
{
    http_listener_config config;

    /*config.set_ssl_context_callback([](boost::asio::ssl::context &context){

        context.use_certificate_chain_file("/tmp/example.crt");
        context.use_private_key_file("/tmp/example.key", boost::asio::ssl::context::pem);

});*/
    http_listener listener("http://192.168.1.54:4343"/*, config*/);

    listener.support(methods::GET,  handle_get);
    listener.support(methods::POST, handle_post);
    listener.support(methods::OPTIONS, handle_options);
    //listener.support(methods::PUT,  handle_put);
    //listener.support(methods::DEL,  handle_del);

    try
    {
       listener
          .open()
          .then([/*&listener*/]() {TRACE("\nstarting to listen\n"); })
          .wait();

       while (true)
       {
           QThread::msleep(100);
       }
    }
    catch (exception const & e)
    {
       cout << e.what() << endl;
    }
}

