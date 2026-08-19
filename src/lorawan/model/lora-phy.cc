#include "lora-phy.h"
#include "lora-channel.h"
#include "ns3/simulator.h"
#include "ns3/node.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace ns3
{
namespace lorawan
{

NS_LOG_COMPONENT_DEFINE("LoraPhy");
NS_OBJECT_ENSURE_REGISTERED(LoraPhy);

/* ===================== HELPER ===================== */

static double GaussianNoise(double stddev)
{
    double u1 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
    double u2 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
    return stddev * sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);
}
Time
LoraPhy::GetOnAirTime(Ptr<Packet> packet, LoraTxParameters txParams)
{
    const double sf = static_cast<double>(txParams.sf);
    const double bw = static_cast<double>(txParams.bandwidthHz);
    const double payloadSize = static_cast<double>(packet->GetSize());
    const double cr = static_cast<double>(txParams.codingRate);
    const double de = (txParams.lowDataRateOptimizationEnabled && sf >= 11.0) ? 1.0 : 0.0;
    const double h = txParams.headerDisabled ? 1.0 : 0.0;

    const double tsym = std::pow(2.0, sf) / bw;
    const double nPayload = 8.0 + std::max(
        std::ceil((8.0 * payloadSize - 4.0 * sf + 28.0 + 16.0 - 20.0 * h) /
                  (4.0 * (sf - 2.0 * de))) * (cr + 4.0),
        0.0);

    const double tPayload = nPayload * tsym;
    const double tPreamble = (static_cast<double>(txParams.nPreamble) + 4.25) * tsym;

    return Seconds(tPayload + tPreamble);
}


/* ===================== TYPE ===================== */

TypeId
LoraPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LoraPhy")
            .SetParent<Object>()
            .SetGroupName("lorawan");
    return tid;
}

/* ===================== BASICS ===================== */

LoraPhy::LoraPhy() {}
LoraPhy::~LoraPhy() {}

Ptr<NetDevice> LoraPhy::GetDevice() const { return m_device; }
void LoraPhy::SetDevice(Ptr<NetDevice> device) { m_device = device; }

Ptr<LoraChannel> LoraPhy::GetChannel() const { return m_channel; }
void LoraPhy::SetChannel(Ptr<LoraChannel> channel) { m_channel = channel; }

Ptr<MobilityModel> LoraPhy::GetMobility()
{
    if (m_mobility) return m_mobility;
    return m_device->GetNode()->GetObject<MobilityModel>();
}

void LoraPhy::SetMobility(Ptr<MobilityModel> mobility)
{
    m_mobility = mobility;
}

/* ===================== CALLBACKS ===================== */

void LoraPhy::SetReceiveOkCallback(RxOkCallback cb) { m_rxOkCallback = cb; }
void LoraPhy::SetReceiveFailedCallback(RxFailedCallback cb) { m_rxFailedCallback = cb; }
void LoraPhy::SetTxFinishedCallback(TxFinishedCallback cb) { m_txFinishedCallback = cb; }

/* ===================== TIME ===================== */

Time
LoraPhy::GetTSym(LoraTxParameters txParams)
{
    return Seconds(pow(2, int(txParams.sf)) / (txParams.bandwidthHz));
}

/* ===================== ALIGNMENT ===================== */

Time
LoraPhy::AlignTime(const std::vector<RxCopy>& v)
{
    Time ref = v.front().rxTime;
    double sum = 0;

    for (auto& c : v)
    {
        double dt = (c.rxTime - ref).GetSeconds();
        double corrected = dt / sqrt((double)v.size());
        sum += ref.GetSeconds() + corrected;
    }

    return Seconds(sum / v.size());
}

/* ===================== PHASE ESTIMATION ===================== */

double
LoraPhy::EstimatePhaseOffset(const std::vector<RxCopy>& v)
{
    double phaseSum = 0;

    for (auto& c : v)
    {
        double phaseStd = 1.0 / std::max(0.1, pow(10.0, c.snrDb / 20.0));
        double phaseError = GaussianNoise(phaseStd);
        phaseSum += phaseError;
    }

    return phaseSum / v.size();
}

/* ===================== COHERENT COMBINING ===================== */

double
LoraPhy::CombineSnrCoherentApprox(const std::vector<RxCopy>& v)
{
    double linearSum = 0;

    for (auto& c : v)
        linearSum += pow(10.0, c.snrDb / 10.0);

    double phaseOffset = EstimatePhaseOffset(v);
    double coherence = std::max(0.0, std::min(1.0, cos(phaseOffset)));

    double N = v.size();
    double coherentLinear = linearSum * N;

    double effective =
        coherence * coherentLinear +
        (1.0 - coherence) * linearSum;

    return 10 * log10(effective);
}

/* ===================== BUFFER ===================== */

void
LoraPhy::BufferAndMaybeCombine(uint32_t pid, const RxCopy& copy)
{
    auto& vec = m_rxBuffer[pid];
    vec.push_back(copy);

    if (vec.size() >= m_expectedCopies)
    {
        CombineAndDecide(pid);
        m_rxBuffer.erase(pid);
    }
    else
    {
        Simulator::Schedule(m_combineWindow,
                            &LoraPhy::CombineAndDecide,
                            this,
                            pid);
    }
}

/* ===================== COMBINE ===================== */

void
LoraPhy::CombineAndDecide(uint32_t pid)
{
    auto it = m_rxBuffer.find(pid);
    if (it == m_rxBuffer.end()) return;

    auto& v = it->second;

    double combinedSnr = CombineSnrCoherentApprox(v);
    Time alignedTime = AlignTime(v);

    NS_LOG_UNCOND("COMBINED Packet " << pid
                    << " SNR=" << combinedSnr
                    << " Time=" << alignedTime.GetSeconds());

    if (!m_rxOkCallback.IsNull())
        m_rxOkCallback(nullptr);

    m_rxBuffer.erase(it);
}

std::ostream&
operator<<(std::ostream& os, const LoraTxParameters& p)
{
    os << "SF=" << (int)p.sf
       << " BW=" << p.bandwidthHz
       << " CR=" << (int)p.codingRate;
    return os;
}

} // namespace lorawan
} // namespace ns3
