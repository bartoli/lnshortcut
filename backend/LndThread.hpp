#ifndef LNDTHREAD_H
#define LNDTHREAD_H

#include <QString>
#include <QThread>

/*
 * Thread that manages lnd cli interface.
 */

class LndThread : public QThread
{
    Q_OBJECT
public:
    void setLncliCmd(const QString&);
    void run() override;

    static inline int getOnChainHeight() { return _currentHeight;}

signals:
    void newGraphJson(int height, const QByteArray&);
    void _getinfo_newresult(const QByteArray&);
    void _newBlockHeight(const int);
    //void newDonateInvoice(uint64_t workId, const QString& invoice_string);

private slots:
    void _getinfo_fetchresult(const QByteArray&);
    void _fetchNewBlock(const int);

private:
    //wrappers for lncli actions
    void _describeGraph(int height);
    void _getInfo();
    void _generateDonateInvoice(uint64_t workId, uint64_t amt_sats, QString& invoice);
    void _getChanInfo(); //listchannels + getchaninfo on each

    bool _configOK();
    bool _runLnCli(const QStringList& options, QByteArray& output);

    static const int _getinfo_period_sec = 10;

    QString _lncliCmd;
    static QString _nodePubkey;
    //Last valid result of block height query
    static int _currentHeight;


    //Info that we currently need on our channels (lncli getchaninfo)
    typedef struct
    {
        uint64_t min_htlc_msat; //min_htlc
        uint64_t max_htlc_msat; //max_htlc_msat
    }ChannelInfo;
    //Last valid info on the node's channels
    static QMap<QString, ChannelInfo> _currentChannels; //index is "channel_id" value
};

#endif // LNDTHREAD_H
