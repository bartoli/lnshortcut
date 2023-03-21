#ifndef RESULTPOOL_HPP
#define RESULTPOOL_HPP

#include <QMap>
#include <QMutex>
#include <stdint.h>

//Class for storing async results


class ResultPool
{

public:
    static ResultPool* getInstance();
private:
    ResultPool() = default;
    static ResultPool* _instance;

    //Store invoice string for a given uint64 work ID
public:
    void donateInvoiceRequest(uint64_t workId , uint64_t amt_sat);
    bool hasInvoiceRequest(uint64_t& workId, uint64_t& amt_sat) const;
    void addInvoice(uint64_t workId, const QString& invoice);
    int waitForResult_invoice(uint64_t workId, QString& result, int timeout_sec=-1, bool clear=true);
private:
    typedef struct {uint64_t workId, amtSat;} InvoiceRequest;
    mutable QMutex _invoiceRequestMutex;
    mutable QMutex _invoiceResultMutex;
    QList<InvoiceRequest> _invoiceRequests;
    QMap<uint64_t, QString> _invoiceResults;
};

#endif // RESULTPOOL_HPP
