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
#include <iostream>
#include <map>
#include <set>
#include <string>
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

void handle_get(http_request request)
{
   TRACE("\nhandle GET\n");
   /*cout <<"c"<<request.relative_uri().path()<<endl;
   cout <<"b"<<request.relative_uri().query()<<endl;
   cout <<"a"<<request.relative_uri().resource().to_string() <<endl;*/

   const std::string resource = request.relative_uri().resource().to_string();
   http_response answer = http_response(status_codes::OK);
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
   }
   /*
    * TODO:
    * GET node_info(pubkey)
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

