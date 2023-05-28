base_url = 'https://advisor.lnshortcut.ovh/';

/*
Get Json result from REST request on sub-url, and on success, apply the passed function to use it
*/
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
}

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
    div_ni.innerHTML = json.edges+" edges, "+json.nodes+" nodes, "+json.capacity/100000000+" BTC<br>"
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
      div.innerHTML = "<table border=1><tr><td>"+json.edges+" edges with "+json.peers+" peers<br>"
      + "Min/Max/Avg/Total capacity (sats): "+json.cap_min+"/"+json.cap_max+"/"+json.cap_avg+"/"+json.cap_total+"<br>"
      + "Connected to LNShortcut node: "+(json.lns_peer? "Yes" : "No")+"<br>"
      + "Clearnet / Tor : "+clearnet_val+"<br>"
      +"</td></tr></table><br>";
      
    }    
  }
  var pubkey = document.getElementById('pubkey').value;
  var json_query = {};
  json_query['op'] = "node_info";
  json_query['pubkey'] = pubkey;
  getPostJson(json_query, _apply);
  //getRestJson("node_info/"+pubkey, _apply); 
}

function node_advice()
{
  function _apply(json)
  {
    const div = document.getElementById('channelAdvice');
    if( json.hasOwnProperty('error'))
    {
      div.innerHTML = json.error;
    }
    else
    {
      div.innerHTML = "Hop2: "+json.node2+" brings "+json.cap2+" sats nearer.<br>"
                    + "Hop3: "+json.node3+" brings "+json.cap3+" sats nearer.<br>";
    }
  }

  var pubkey = document.getElementById('pubkey').value;
  var capacity = document.getElementById('capacity').value;
  var zbfEdges = document.getElementById("zbfEdges").checked;  
  var zbfNodes = document.getElementById("zbfNodes").checked;

  var query_json = {};
  query_json['op'] = "node_advice";
  query_json['pubkey'] = pubkey;
  query_json['capacity'] = capacity;
  query_json['zbfEdges'] = zbfEdges;
  query_json['zbfNodes'] = zbfNodes;
  getPostJson(query_json, _apply);
}

function get_donate_invoice()
{
  function _apply(json)
  {
    const div = document.getElementById('donateInvoiceDiv');
    div.innerHTML = "Invoice (2H validity): "+ json.invoice;
  }
  var amt_sats = document.getElementById('donate_amt').value;
  var query_json = {};
  query_json['op'] = 'donate_invoice';
  query_json['amt_sat'] = amt_sats;
  var query = "donate_invoice/"+amt_sats;
  getPostJson(query_json, _apply); 
}
