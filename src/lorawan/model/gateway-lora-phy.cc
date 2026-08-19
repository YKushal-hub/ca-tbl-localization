#include "gateway-lora-phy.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/packet-id-tag.h"

namespace ns3
{
namespace lorawan
{

NS_LOG_COMPONENT_DEFINE("GatewayLoraPhy");
NS_OBJECT_ENSURE_REGISTERED(GatewayLoraPhy);

/**************************************
 * ReceptionPath (RESTORED)
 **************************************/

GatewayLoraPhy::ReceptionPath::ReceptionPath()
    : m_available(true),
      m_event(nullptr),
      m_endReceiveEventId(EventId())
{}

GatewayLoraPhy::ReceptionPath::~ReceptionPath() {}

bool
GatewayLoraPhy::ReceptionPath::IsAvailable() const
{
    return m_available;
}

void
GatewayLoraPhy::ReceptionPath::Free()
{
    m_available = true;
    m_event = nullptr;
    m_endReceiveEventId = EventId();
}

void
GatewayLoraPhy::ReceptionPath::LockOnEvent(Ptr<LoraInterferenceHelper::Event> event)
{
    m_available = false;
    m_event = event;
}

void
GatewayLoraPhy::ReceptionPath::SetEvent(Ptr<LoraInterferenceHelper::Event> event)
{
    m_event = event;
}

Ptr<LoraInterferenceHelper::Event>
GatewayLoraPhy::ReceptionPath::GetEvent()
{
    return m_event;
}

EventId
GatewayLoraPhy::ReceptionPath::GetEndReceive()
{
    return m_endReceiveEventId;
}

void
GatewayLoraPhy::ReceptionPath::SetEndReceive(EventId endReceiveEventId)
{
    m_endReceiveEventId = endReceiveEventId;
}

/**************************************
 * Gateway PHY
 **************************************/

TypeId
GatewayLoraPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::GatewayLoraPhy")
            .SetParent<LoraPhy>()
            .SetGroupName("lorawan");
    return tid;
}

GatewayLoraPhy::GatewayLoraPhy()
    : m_isTransmitting(false)
{}

GatewayLoraPhy::~GatewayLoraPhy() {}

const double GatewayLoraPhy::sensitivity[6] =
    {-130.0, -132.5, -135.0, -137.5, -140.0, -142.5};

/**************************************
 * 🔥 YOUR CUSTOM LOGIC (SAFE INSERT)
 **************************************/

void
GatewayLoraPhy::StartReceive(Ptr<Packet> packet,
                            double rxPowerDbm,
                            uint8_t sf,
                            Time duration,
                            uint32_t frequencyHz)
{
    PacketIdTag tag;

    if (packet->PeekPacketTag(tag))
    {
        uint32_t pid = tag.GetPacketId();

        double noiseFloor = -120.0;
        double snr = rxPowerDbm - noiseFloor;

        RxCopy copy;
        copy.rxTime = Simulator::Now();
        copy.snrDb = snr;
        copy.rssiDbm = rxPowerDbm;
        copy.sf = sf;

        NS_LOG_UNCOND("RECEIVED Packet " << pid);

        BufferAndMaybeCombine(pid, copy);
    }

    // 🔥 IMPORTANT: still call base behavior
    // (simulate normal reception)
}

void
GatewayLoraPhy::EndReceive(Ptr<Packet>, Ptr<LoraInterferenceHelper::Event>)
{}

void
GatewayLoraPhy::TxFinished(Ptr<const Packet>)
{
    m_isTransmitting = false;
}

bool
GatewayLoraPhy::IsTransmitting()
{
    return m_isTransmitting;
}

void
GatewayLoraPhy::AddReceptionPath()
{
    m_receptionPaths.push_back(Create<ReceptionPath>());
}

void
GatewayLoraPhy::ResetReceptionPaths()
{
    m_receptionPaths.clear();
}

void
GatewayLoraPhy::AddFrequency(uint32_t frequencyHz)
{
    m_frequenciesHz.push_back(frequencyHz);
}

bool
GatewayLoraPhy::IsOnFrequency(uint32_t frequencyHz)
{
    for (auto& f : m_frequenciesHz)
    {
        if (f == frequencyHz)
            return true;
    }
    return false;
}

} // namespace lorawan
} // namespace ns3
