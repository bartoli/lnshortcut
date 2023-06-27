base_url = 'https://advisor.lnshortcut.ovh/';

/*
Get Json result from REST request on sub-url, and on success, apply the passed function to use it
function getRestJson(path, applyFunc)
{
  var result;
  try {
    url = base_url + path;
    console.log('1', url);
    fetch(url)
    .then((response) => response.json())
    .then((json) => 
    {  
      applyFunc(json);
    });  
  }
  catch (err) {
    console.log('3', err);
  }
  return result;
}*/

/* TOOLS for POST requests sendig params as json*/
function getPostJson(json, applyFunc)
{
  fetch('https://advisor.lnshortcut.ovh/', {
    method: 'POST',
    headers: {
      'Accept': 'application/json',
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*'
    },
    body: JSON.stringify(json),
   })
   .then((response) => response.json())
   .then((json) => 
   {  
     applyFunc(json);
   });  
}

//Update block height in div vith id 'nh'
function update_network_info()
{
  function _apply(json)
  {
    const div_bh = document.getElementById('bh');
    div_bh.textContent = "The Lightning Network at block height "+json.height+":";
    const div_ni = document.getElementById('nh');
    div_ni.innerHTML = json.nodes+" nodes, "+json.edges+" edges, "+json.capacity/100000000+" BTC<br>"
    +"#ZeroBaseFee: "+json.zbf_nodes+" nodes, "+json.zbf_edges+" edges";
  }
  var json_query = {};
  json_query['op'] = "network_info";
  getPostJson(json_query, _apply); 
}

function node_stats()
{
  function _apply(json)
  {
    const div = document.getElementById('analysisResult');
    if( json.hasOwnProperty('error'))
    {
      div.innerHTML = json.error;
    }
    else
    {
      var clearnet_val;
      if(json.on_tor)
      {
        clearnet_val = json.on_clearnet? "Both" : "Tor Only";
      }
      else
        clearnet_val = "Clearnet only";
      div.innerHTML = json.edges+" edges with "+json.peers+" peers<br>"
      + "Min/Max/Avg/Total capacity (sats): "+json.cap_min+"/"+json.cap_max+"/"+json.cap_avg+"/"+json.cap_total+"<br>"
      + "Connected to LNShortcut node: "+(json.lns_peer? "Yes" : "No")+"<br>"
      + "Clearnet / Tor : "+clearnet_val+"<br>";      
    }    
  }
  var pubkey = document.getElementById('pubkey').value;
  var json_query = {};
  json_query['op'] = "node_info";
  json_query['pubkey'] = pubkey;
  getPostJson(json_query, _apply);
}

function node_advice()
{
  function _apply(json)
  {
    const div = document.getElementById('analysisResult');
    if( json.hasOwnProperty('error'))
    {
      div.innerHTML = json.error;
    }
    else
    {
      var result_html = "";
      //Filtered network info
      result_html += "With curent filtering parameters, the 'useful' network contains:<br>"
      result_html += json.workNodes+ " nodes and "+json.workEdges+ " channels<br>";
      result_html += "<br>";

      //Current reach
      result_html += "The specified transaction could currently reach:<br>"
      result_html += " - In Outbound direction : "+json.outboundReachedNodes+" nodes and "+json.outboundReachedEdges+" channels<br>";
      result_html += "   for a median cost of "+json.outboundMedianCost/1000.0+"sats<br>";
      result_html += " - In Inbound direction : "+json.inboundReachedNodes+" nodes and "+json.inboundReachedEdges+" channels<br>";
      result_html += "   for a median cost of "+json.inboundMedianCost/1000.0+"sats<br>";
      result_html += "<br>";

      //Results
      result_html += "For the most new reached edges in outbound direction:<br>"
      result_html += json.outBoundBestCandidateForEdges+ "<br>" +
        "reaches +"+(json.outBoundNewReachedNodesForEdges-json.outboundReachedNodes)+" nodes and +"+(json.outBoundNewReachedEdgesForEdges-json.outboundReachedEdges)+" channels<br>";
      result_html += "<br>For the most new reached nodes in outbound direction:<br>"
      result_html += json.outBoundBestCandidateForNodes+ "<br>" +
        "reaches +"+(json.outBoundNewReachedNodesForNodes-json.outboundReachedNodes)+" nodes and +"+(json.outBoundNewReachedEdgesForNodes-json.outboundReachedEdges)+" channels<br>";
      result_html += "<br>For the most new reached edges in inbound direction:<br>"
      result_html += json.inBoundBestCandidateForEdges+ "<br>" +
        "reaches +"+(json.inBoundNewReachedNodesForEdges-json.inboundReachedNodes)+" nodes and +"+(json.inBoundNewReachedEdgesForEdges-json.inboundReachedEdges)+" channels<br>";
      result_html += "<br>For the most new reached nodes in inbound direction:<br>"
      result_html += json.inBoundBestCandidateForNodes+ "<br>" +
        "reaches +"+(json.inBoundNewReachedNodesForNodes-json.inboundReachedNodes)+" nodes and +"+(json.inBoundNewReachedEdgesForNodes-json.inboundReachedEdges)+" channels<br>";


      div.innerHTML = result_html;
    }
  }

  var pubkey = document.getElementById('pubkey').value;
  var capacity = document.getElementById('capacity').value;
  var test_amt_sat = document.getElementById('test_amt_sat').value;
  var max_fee_sat = document.getElementById('max_fee_sat').value;
  var zbfEdges = document.getElementById("zbfEdges").checked;  
  var zbfNodes = document.getElementById("zbfNodes").checked;

  var query_json = {};
  query_json['op'] = "node_advice";
  query_json['pubkey'] = pubkey;
  query_json['capacity'] = capacity;
  query_json['test_amt_sat'] = test_amt_sat;
  query_json['max_fee_sat'] = max_fee_sat;
  query_json['zbfEdges'] = zbfEdges;
  query_json['zbfNodes'] = zbfNodes;
  query_json['clearnetEdges'] = document.getElementById("clearnetEdges").checked;
  query_json['torEdges'] = document.getElementById("torEdges").checked;
  query_json['clearnetNodes'] = document.getElementById("clearnetNodes").checked;
  query_json['torNodes'] = document.getElementById("torNodes").checked;
  query_json['useMinChanSizeDb'] = document.getElementById("useMinChanSizeDb").checked;
  query_json['guessMinChanSize'] = document.getElementById("guessMinChanSize").checked;
  useMinChanSizeDb
  getPostJson(query_json, _apply);
}

function get_donate_invoice()
{
  function _apply(json)
  {
    const div = document.getElementById('donateInvoiceDiv');
    div.innerHTML = "Invoice (2H validity):<br><form><textarea rows=10 cols=80>"+ json.invoice+"</textarea></form>";
  }
  var amt_sats = document.getElementById('donate_amt').value;
  var query_json = {};
  query_json['op'] = 'donate_invoice';
  query_json['amt_sat'] = amt_sats;
  var query = "donate_invoice/"+amt_sats;
  getPostJson(query_json, _apply); 
}
