#include "tcp-lr-newreno.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpLrNewReno");
NS_OBJECT_ENSURE_REGISTERED (TcpLrNewReno);

TypeId
TcpLrNewReno::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpLrNewReno")
    .SetParent<TcpCongestionOps> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpLrNewReno> ()
  ;
  return tid;
}

TcpLrNewReno::TcpLrNewReno (void) : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
}

TcpLrNewReno::TcpLrNewReno (const TcpLrNewReno& sock)
  : TcpCongestionOps (sock)
{
  NS_LOG_FUNCTION (this);
}

TcpLrNewReno::~TcpLrNewReno (void)
{
}

uint32_t
TcpLrNewReno::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked >= 1)
    {
      uint32_t sndCwnd = tcb->m_cWnd;
      tcb->m_cWnd = std::min ((sndCwnd + tcb->m_segmentSize), (uint32_t)tcb->m_ssThresh);
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - ((tcb->m_cWnd - sndCwnd) / tcb->m_segmentSize);
    }

  return 0;
}

void TcpLrNewReno::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) {
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  if (segmentsAcked > 0){
    if( tcb->m_cWnd < CW_MAX ) {
      double num = static_cast<double> ( CW_MAX-tcb->m_cWnd.Get() );
      double den = static_cast<double> ( alpha*tcb->m_cWnd.Get() );
      double f = static_cast<double> (tcb->m_segmentSize); 
      double adder = f*(num/den);
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);
    }
    else {
      double adder = static_cast<double> (tcb->m_segmentSize*tcb->m_segmentSize)/(alpha*tcb->m_cWnd.Get());
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);

      if( (tcb->m_cWnd - CW_MAX) > (beta*CW_MAX-tcb->m_ssThresh) ) {
        Ideal_CW = CW_MAX;
        Ideal_CW_SET = 1;
      }
    }
  }
  NS_LOG_DEBUG ("At end of CongestionAvoidance(), m_cWnd: " << tcb->m_cWnd);
}

void
TcpLrNewReno::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      NS_LOG_DEBUG ("In slow start, m_cWnd " << tcb->m_cWnd << " m_ssThresh " << tcb->m_ssThresh);
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }
  else
    {
      NS_LOG_DEBUG ("In cong. avoidance, m_cWnd " << tcb->m_cWnd << " m_ssThresh " << tcb->m_ssThresh);
      CongestionAvoidance (tcb, segmentsAcked);
    }
}

std::string
TcpLrNewReno::GetName () const
{
  return "TcpLrNewReno";
}

uint32_t
TcpLrNewReno::GetSsThresh (Ptr<const TcpSocketState> state,
                           uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  CW_MAX = state->m_cWnd;
  uint32_t ssThresh = std::max<uint32_t> (2 * state->m_segmentSize, state->m_cWnd / 2);

  if( Ideal_CW_SET ) {
    ssThresh = std::max<uint32_t> (ssThresh, Ideal_CW); 
    Ideal_CW_SET = 0;
  }

  return ssThresh;
}

Ptr<TcpCongestionOps>
TcpLrNewReno::Fork ()
{
  return CopyObject<TcpLrNewReno> (this);
}

} // namespace ns3

