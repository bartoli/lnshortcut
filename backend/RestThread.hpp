#ifndef RESTTHREAD_HPP
#define RESTTHREAD_HPP

#include <Logger.hpp>
#include <QThread>

namespace web { namespace json { class value; } }

class RestThread : public QThread
{
public:
    RestThread();
    void run() override;

    static QScopedPointer<Logger> logger;

    signals:
    void needDonateInvoice(int amt_sats, uint64_t workId);

    //static void GET_donateInvoice(const QString& resource, web::json::value& body);
};

#endif // RESTTHREAD_HPP
