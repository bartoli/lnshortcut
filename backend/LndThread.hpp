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


private slots:
    void _getinfo_fetchresult(const QByteArray&);
    void _fetchNewBlock(const int);

private:
    //wrappers for lncli acctions
    void _describeGraph(int height);
    void _getInfo();

    bool _configOK();
    bool _runLnCli(const QStringList& options, QByteArray& output);

    static const int _getinfo_period_sec = 10;

    QString _lncliCmd;
    static int _currentHeight;
};

#endif // LNDTHREAD_H
