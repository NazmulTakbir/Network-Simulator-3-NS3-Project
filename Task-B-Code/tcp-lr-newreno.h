#ifndef TCPLRNEWRENO_H
#define TCPLRNEWRENO_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-state.h"

namespace ns3 {

class TcpLrNewReno : public TcpCongestionOps {
public:
  static TypeId GetTypeId (void);

  TcpLrNewReno ();
  TcpLrNewReno (const TcpLrNewReno& sock);
  ~TcpLrNewReno ();
  std::string GetName () const;
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
  virtual Ptr<TcpCongestionOps> Fork ();

protected:
  virtual uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

private:
  uint32_t CW_MAX {0};
  uint32_t Ideal_CW {0};
  uint32_t Ideal_CW_SET {0};
  uint32_t  alpha {4}; 
  double  beta {0.5}; 
};

} 

#endif 
