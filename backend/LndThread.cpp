#include <LndThread.hpp>
#include <RestThread.hpp>
#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <ResultPool.hpp>
#include <QJsonArray>

int LndThread::_currentHeight = 0;
QString LndThread::_nodePubkey;
QMap<QString, LndThread::ChannelInfo> LndThread::_currentChannels;

void LndThread::run()
{
    connect(this, &LndThread::_getinfo_newresult, &LndThread::_getinfo_fetchresult);
    connect(this, &LndThread::_newBlockHeight, this, &LndThread::_describeGraph);
    //connect(&RestThread::needDonateInvoice, this, &LndThread::_generateDonateInvoice);

    ResultPool* result_pool = ResultPool::getInstance();

    // main loop, just to detect new block heights
    // the rest is done through signals
    int chan_policy_loop=0;
    while(1)
    {
        uint64_t work_id, amt_sat;
        if(result_pool->hasInvoiceRequest(work_id, amt_sat))
        {
            QString invoice;
            _generateDonateInvoice(work_id, amt_sat, invoice);
            result_pool->addInvoice(work_id, invoice);
        }

                            sleep(1);

        if(!_configOK())
            continue;
        _getInfo();
        //optimize channels policy only once per hour
            chan_policy_loop++;
                                                                  if(chan_policy_loop > 3600)
        {
          chan_policy_loop = 0;
          _getChanInfo();
        }

    }
}

void LndThread::_getInfo()
{
    //static variable, to be able to emit the output only by reference (and used thae latest output in case some signals were slow to be handled?)
    static QByteArray getinfo_result;
    if( _runLnCli({"getinfo"}, getinfo_result))
    {
      emit _getinfo_newresult(getinfo_result);
    }
}


void LndThread::_getChanInfo()
{
    static QByteArray listchannels_result;
    if(! _runLnCli({"listchannels"}, listchannels_result))
      return;

    QStringList channel_ids;

    QJsonDocument responseDoc = QJsonDocument::fromJson(listchannels_result);
    QJsonObject responseObj = responseDoc.object();
    QJsonArray channels_array = responseObj.value("channels").toArray();

    QString chosen_chan_point;
    QString chosen_base_fee_msat;
    QString chosen_feerate_msat;
    int chosen_tld = 80;
    uint64_t fixed_max_htlc=0, max_delta=0;

    for(const QJsonValueRef chan : channels_array)
    {
        QString channel_id = chan.toObject().value("chan_id").toString();
        uint64_t local_balance = atoll(chan.toObject().value("local_balance").toString().toUtf8().constData());
        //qWarning()<<"Found channel id "<<channel_id<<" with local balance "<<local_balance;
        QByteArray chaninfo_result;
        if(!_runLnCli({"getchaninfo", channel_id}, chaninfo_result))
          continue;

        //read both sides'pubkey to find which side is ours
        QJsonDocument responseDoc2 = QJsonDocument::fromJson(chaninfo_result);
        QJsonObject responseObj2 = responseDoc2.object();
        QString node1_pubkey = responseObj2.value("node1_pub").toString();
        QString node2_pubkey = responseObj2.value("node2_pub").toString();
        QString chan_point = responseObj2.value("chan_point").toString();
        uint64_t capacity = atoll(responseObj2.value("capacity").toString().toUtf8().constData());
        bool node_is_first_side = (node1_pubkey == _nodePubkey);
        QString policy_key = node_is_first_side? "node1_policy" : "node2_policy";
        QJsonObject nodePolicyObject = responseObj2.value(policy_key).toObject();

        //uint64_t min_htlc_msat = atoll(nodePolicyObject.value("min_htlc").toString().toUtf8().constData());
        uint64_t max_htlc_msat = atoll(nodePolicyObject.value("max_htlc_msat").toString().toUtf8().constData());

        //Before having an optimal value, dues it hafe a valid value (priority to update it compared to others)?
        //Apparently, does not happen, RTL/lnd did not alow setting max=0 when min=1000?
        /*bool valid = true;
        if(min_htlc_msat> max_htlc_msat)
        {
            valid = false;
            qWarning()<<"Channel "<<channel_id<<" has invalid max_htlc<min_htlc";
        }*/
        uint64_t reserve = 25000; //min to avoid getting the channel 'stuck' because of too low balance
        uint64_t optimal_max_htlc = local_balance;
        if(optimal_max_htlc<=0)
            optimal_max_htlc = reserve;//min_htlc
        if(optimal_max_htlc>capacity-reserve)
            optimal_max_htlc = capacity-reserve;
        uint64_t htlc_delta = llabs((int64_t)max_htlc_msat/1000 - (int64_t)optimal_max_htlc);
        if(htlc_delta > 0/*reserve*/)
        {
          qWarning()<<"Channel "<<channel_id<<"/"<<chan_point<<" has wrong max htlc ("<<max_htlc_msat/1000<<", should be "<<optimal_max_htlc<<", local balance is "<<local_balance<<")";
          if(max_delta < htlc_delta)
          {
              max_delta = htlc_delta;
              chosen_chan_point = chan_point;
              chosen_base_fee_msat = nodePolicyObject.value("fee_base_msat").toString();
              chosen_feerate_msat = nodePolicyObject.value("fee_rate_milli_msat").toString();
              chosen_tld = nodePolicyObject.value("time_lock_delta").toInt();
              fixed_max_htlc = optimal_max_htlc;
          }
        }
    }
    if(max_delta>0)
    {
        qWarning()<<"Fixing "<<chosen_chan_point<<" has wrong max htlc ("<<fixed_max_htlc<<")";
        QByteArray output;
        _runLnCli({"updatechanpolicy",
                   "--max_htlc_msat",QString::number(fixed_max_htlc*1000ULL),
                   "--chan_point",chosen_chan_point,
                   "--base_fee_msat", chosen_base_fee_msat,
                   "--fee_rate_ppm", chosen_feerate_msat,
                   "--time_lock_delta", QString::number(chosen_tld)}
                  ,output);

        qWarning()<<output;
    }
}


void LndThread::_generateDonateInvoice(uint64_t /*workId*/, uint64_t amt_sats, QString& invoice)
{
    //static variable, to be able to emit the output only by reference (and used the latest output in case some signals were slow to be handled?)
    //beware to not use the output anymore while the new one is being written in place!
    static QByteArray output_json;
    if( _runLnCli({"addinvoice",
                  "--memo", "\"LNShortcut donation\"",
                  "--amt", QString::number(amt_sats),
                  "--expiry", "120"}, output_json))
    {

      QJsonDocument responseDoc = QJsonDocument::fromJson(output_json);
      QJsonObject responseObj = responseDoc.object();
      invoice = responseObj.value("payment_request").toString();
    }
}

void LndThread::_describeGraph(int height)
{
    //static variable, to be able to emit the output only by reference (and used the latest output in case some signals were slow to be handled?)
    //beware to not use the output anymore while the new one is being written in place!
    static QByteArray latest_graph_json;
    if( _runLnCli({"describegraph", "--include_unannounced"}, latest_graph_json))
    {
      emit newGraphJson(height, latest_graph_json);
    }
}

void LndThread::setLncliCmd(const QString& cmd)
{
    _lncliCmd = cmd;
}

bool LndThread::_configOK()
{
    return (!_lncliCmd.isEmpty());
}

void LndThread::_getinfo_fetchresult(const QByteArray& result)
{
    QJsonDocument doc(QJsonDocument::fromJson(result));
    QJsonObject doc_obj = doc.object();
    int height = doc_obj.value("block_height").toInt();
    _nodePubkey = doc_obj.value("identity_pubkey").toString();
    if(height != _currentHeight)
    {
      _currentHeight = height;
      emit _newBlockHeight(_currentHeight);
    }
}

bool LndThread::_runLnCli(const QStringList& options, QByteArray& output)
{
    QProcess process;
    process.start(_lncliCmd, options, QIODevice::ReadOnly);
    process.waitForFinished(60000);
    output = process.readAllStandardOutput();
    return (process.exitCode()==0);
}

//gets from lncli all the network/node info when a new block height is reached
void LndThread::_fetchNewBlock(const int height)
{
  _describeGraph(height);
}
