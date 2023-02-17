#include <LndThread.hpp>
#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

int LndThread::_currentHeight = 0;

void LndThread::run()
{
    connect(this, &LndThread::_getinfo_newresult, &LndThread::_getinfo_fetchresult);
    connect(this, &LndThread::_newBlockHeight, this, &LndThread::_describeGraph);

    // main loop, just to detect new block heights
    // the rest is done through signals
    while(1)
    {
        sleep(_getinfo_period_sec);

        if(!_configOK())
            continue;
        _getInfo();

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
    int height = doc.object().value("block_height").toInt();
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
