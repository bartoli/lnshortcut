#include <RestThread.hpp>
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
using namespace std;

#define TRACE(msg)            cout << msg
#define TRACE_ACTION(a, k, v) cout << a << L" (" << k << L", " << v << L")\n"

map<utility::string_t, utility::string_t> dictionary;

void display_json(
   json::value const & jvalue,
   utility::string_t const & prefix)
{
   cout << prefix << jvalue.serialize() << endl;
}

web::http::status_code GET_nodeinfo(const QString& resource, json::value& body, QString& error_string)
{
    QString node_pubkey = resource.mid(11);
    qWarning() <<"node_info/"<< node_pubkey;
    NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
    if (network == nullptr)
    {
        body["error"] = json::value("Network graph informaton not available, please retry later.");
        return status_codes::OK;
    }

    int o_n_edges=0;
    uint64_t o_cap_min = ULONG_MAX, o_cap_max=0, o_cap_avg=0, o_cap_ttl=0;
    int o_peers=0;
    int node_rank = network->node_index.value(node_pubkey, -1);
    if(node_rank<0)
    {
        body["error"] = json::value(QString("Unknown node public key %1.").arg(node_pubkey).toUtf8().constData());
        return status_codes::OK;
    }
    bool lns_peer = node_rank==network->lns_noderank;

    const Node& node = network->nodes[node_rank];
    body["edges"] = node.edges.size();
    QSet<int> peer_ranks;
    for(int edge_rank : node.edges)
    {
        const Edge& edge = network->edges[edge_rank];
        int other_node_rank = edge.node1==node_rank? edge.node2 : edge.node1;
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

    return status_codes::OK;
}

web::http::status_code GET_nodeadvice(const QString& resource, json::value& body, QString& error_string)
{
    QStringList args = resource.mid(13).split("/");
    QString node_pubkey = args[0];
    int cap = atoi(args[1].toUtf8().constData());

    //Is there a prefetched network graph yet?
    NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
    if (network == nullptr)
    {
        body["error"] = json::value("Network graph informaton not available, please retry later.");
        return status_codes::OK;
    }
    int node_rank = network->node_index[node_pubkey];
    //Is it a lns peer? Is capacity not more than capacity connected to lns?
    {
        int lns_capacity = 0;
        const Node& node = network->nodes[node_rank];
        int max_cap = 0;
        for(const int edge_rank : node.edges)
        {
            const Edge& edge = network->edges[edge_rank];
            int other_node_rank = edge.node1 == node_rank? edge.node2 : edge.node1;
            max_cap = std::max(max_cap, edge.capacity);
            if(other_node_rank == network->lns_noderank)
                lns_capacity += edge.capacity;
        }
        if(node_rank != network->lns_noderank && lns_capacity<cap)
        {
            body["error"] = json::value("Can't advise for channels with more capacity than the sum of the channels connected to LNSHortcut");
            return status_codes::OK;
        }
        // Adjust analysed capacity to max existing capacity.
        // It makes no sense to analyse for more than the curent bigger channel,
        // since we need at least 2 channels at that capacity to actually route that size
        // For the general user, it could happen if they have enough cap connected to LNS, but in multiple channels.
        // For lns node, it can happen when we analyse for a bigger capacity than the existing channels.
        // There are no nodes to reach with at least that capacity, so we are looking at empty lists of reached nodes at any hop level
        // -> estimate next channel as if it is the masx size of current channels
        cap = std::min(cap, max_cap);
    }


    Result result;
    AnalysisThread::analyseHops(*network, node_pubkey, cap, result);

    body["node0"] = json::value(node_pubkey.toUtf8().constData());
    body["node2"] = json::value(network->nodes[result.node2].pubKey.toUtf8().constData());
    body["node3"] = json::value(network->nodes[result.node3].pubKey.toUtf8().constData());
    body["cap0"] = cap;
    body["cap2"] = result.cap2;
    body["cap3"] = result.cap3;

    return status_codes::OK;
}

void handle_get(http_request request)
{
   TRACE("\nhandle GET\n");
   /*cout <<"c"<<request.relative_uri().path()<<endl;
   cout <<"b"<<request.relative_uri().query()<<endl;
   cout <<"a"<<request.relative_uri().resource().to_string() <<endl;*/

   const QString resource(request.relative_uri().resource().to_string().c_str());
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
         QString error_string;
         auto status = GET_nodeinfo(resource, body, error_string);
         if(status != status_codes::OK)
         {
           request.reply(status);
           return;
         }
      }
   else if(resource.startsWith("/node_advice/"))
      {
         QString error_string;
         auto status = GET_nodeadvice(resource, body, error_string);
         if(status != status_codes::OK)
         {
           request.reply(status);
           return;
         }
      }
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
   answer.set_body(body);
   request.reply(answer);
}

void handle_request(
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
}

void handle_post(http_request request)
{
   TRACE("\nhandle POST\n");

   /*
    * Receive analyse request from the online form
    */

   handle_request(
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
   });
}

void handle_put(http_request request)
{
   TRACE("\nhandle PUT\n");

   handle_request(
      request,
      [](json::value const & jvalue, json::value & answer)
   {
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
   });
}

void handle_del(http_request request)
{
   TRACE("\nhandle DEL\n");

   handle_request(
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
   });
}

void RestThread::run()
{
    http_listener listener("http://192.168.1.54:4343");

    listener.support(methods::GET,  handle_get);
    listener.support(methods::POST, handle_post);
    listener.support(methods::PUT,  handle_put);
    listener.support(methods::DEL,  handle_del);

    try
    {
       listener
          .open()
          .then([&listener]() {TRACE("\nstarting to listen\n"); })
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

