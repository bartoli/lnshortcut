#include "ResultPool.hpp"

#include <QElapsedTimer>
#include <QThread>
#include <QTimer>

ResultPool* ResultPool::_instance = nullptr;

ResultPool* ResultPool::getInstance()
{
    if(_instance == nullptr)
    {
        _instance = new ResultPool();
    }
    return _instance;
}

void ResultPool::donateInvoiceRequest(uint64_t workId , uint64_t amt_sat)
{
    QMutexLocker locker(&_invoiceRequestMutex);
    _invoiceRequests.append(InvoiceRequest{workId, amt_sat});
}

bool ResultPool::hasInvoiceRequest(uint64_t& workId, uint64_t& amt_sat) const
{
    QMutexLocker locker(&_invoiceRequestMutex);
    if(!_invoiceRequests.empty())
    {
        const auto& req  =_invoiceRequests.first();
        workId = req.workId;
        amt_sat = req.amtSat;
        return true;
    }
    return false;
}

void ResultPool::addInvoice(uint64_t workId, const QString& invoice)
{
    {
      QMutexLocker locker(&_invoiceResultMutex);
      _invoiceResults[workId] = invoice;
    }
    {
      QMutexLocker locker(&_invoiceRequestMutex);
      QList<InvoiceRequest>::iterator it(_invoiceRequests.begin());
      while(it!= _invoiceRequests.end())
      {
          const InvoiceRequest& req = *it;
          if(req.workId == workId)
          {
              _invoiceRequests.erase(it);
              return;
          }
          ++it;
      }
    }
}

int ResultPool::waitForResult_invoice(uint64_t workId, QString& result, int timeout_sec, bool clear)
{
  QElapsedTimer timer;
  timer.start();
  while(timer.elapsed() < timeout_sec*1000)
  {
    {
      QMutexLocker locker(&_invoiceResultMutex);
      if(_invoiceResults.contains(workId))
      {
          result = _invoiceResults[workId];
          if(clear)
              _invoiceResults.remove(workId);
          return 0;
      }
    }
    QThread::msleep(100);
  }
  return -1;
}
