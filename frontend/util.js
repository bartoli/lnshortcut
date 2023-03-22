/*
Get Json result, and on success, apply the passed function to use it
*/
function getRestJson(path, applyFunc)
{
  var result;
  try {
    url = 'https://advisor.lnshortcut.ovh/' + path;
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

//Update block height in div vith id 'bh'
function update_block_height()
{
  function _apply(json)
  {
    const div = document.getElementById('bh');
    div.textContent = "The Lightning Network at block height "+json.height+":";
  }
  getRestJson("block_height", _apply);  
}

//Update block height in div vith id 'nh'
function update_network_info()
{
  function _apply(json)
  {
    const div = document.getElementById('nh');
    div.innerHTML = json.edges+" edges, "+json.nodes+" nodes, "+json.capacity/100000000+" BTC<br>"
    +"#ZeroBaseFee: "+json.zbf_nodes+" nodes, "+json.zbf_edges+" edges";
  }
  getRestJson("network_info", _apply); 
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
  getRestJson("node_info/"+pubkey, _apply); 
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
  var query = "node_advice/"+pubkey+"/"+capacity;
  var zbfEdges = document.getElementById("zbfEdges").checked;
  query += "/"+zbfEdges;
  var zbfNodes = document.getElementById("zbfNodes").checked;
  query += "/"+zbfNodes;
  getRestJson(query, _apply); 
}

function get_donate_invoice()
{
  function _apply(json)
  {
    const div = document.getElementById('donateInvoiceDiv');
    div.innerHTML = "Invoice (2H validity): "+ json.invoice;
  }
  var amt_sats = document.getElementById('donate_amt').value;
  var query = "donate_invoice/"+amt_sats;
  getRestJson(query, _apply); 
}