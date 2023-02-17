/*
Get Json result, and on success, apply the passed function to use it
*/
function getRestJson(path, applyFunc)
{
  var result;
  try {
    url = 'http://79.87.88.235:4343/' + path;
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
    div.textContent = json.height;
  }
  getRestJson("block_height", _apply);  
}

//Update block height in div vith id 'nh'
function update_network_info()
{
  function _apply(json)
  {
    const div = document.getElementById('nh');
    div.textContent = json.edges+" edges, "+json.nodes+" nodes, "+json.capacity/100000000+" BTC";
  }
  getRestJson("network_info", _apply); 
}

//update_block_height();