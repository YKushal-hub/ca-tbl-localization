#ifndef LORA_PHY_H
#define LORA_PHY_H

#include "lora-interference-helper.h"

#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"

#include <map>
#include <vector>

namespace ns3
{
namespace lorawan
{

class LoraChannel;

/* ===================== RX COPY ===================== */

struct RxCopy
{
    Time rxTime;
    double snrDb;
    double rssiDbm;
    uint8_t sf;
};

/* ===================== TX PARAMS ===================== */

enum class CodingRate : uint8_t
{
    CR_4_5 = 1,
    CR_4_6,
    CR_4_7,
    CR_4_8,
};

struct LoraTxParameters
{
    uint8_t sf = 7;
    bool headerDisabled = false;
    CodingRate codingRate = CodingRate::CR_4_5;
    uint32_t bandwidthHz = 125000;
    uint32_t nPreamble = 8;
    bool crcEnabled = true;
    bool lowDataRateOptimizationEnabled = false;
};

/* ✅ REQUIRED FOR NS-3 LOGGING */
std::ostream& operator<<(std::ostream& os, const LoraTxParameters& params);

/* ===================== MAIN CLASS ===================== */

class LoraPhy : public Object
{
  public:
    static TypeId GetTypeId();

    LoraPhy();
    ~LoraPhy() override;

    typedef Callback<void, Ptr<const Packet>> RxOkCallback;
    typedef Callback<void, Ptr<const Packet>> RxFailedCallback;
    typedef Callback<void, Ptr<const Packet>> TxFinishedCallback;

    virtual void StartReceive(Ptr<Packet> packet,
                             double rxPowerDbm,
                             uint8_t sf,
                             Time duration,
                             uint32_t frequencyHz) = 0;

    virtual void EndReceive(Ptr<Packet> packet,
                            Ptr<LoraInterferenceHelper::Event> event) = 0;

    virtual void Send(Ptr<Packet> packet,
                      LoraTxParameters txParams,
                      uint32_t frequencyHz,
                      double txPowerDbm) = 0;

    virtual bool IsTransmitting() = 0;
    virtual bool IsOnFrequency(uint32_t frequencyHz) = 0;

    void SetReceiveOkCallback(RxOkCallback callback);
    void SetReceiveFailedCallback(RxFailedCallback callback);
    void SetTxFinishedCallback(TxFinishedCallback callback);

    Ptr<MobilityModel> GetMobility();
    void SetMobility(Ptr<MobilityModel> mobility);

    void SetChannel(Ptr<LoraChannel> channel);
    Ptr<LoraChannel> GetChannel() const;

    Ptr<NetDevice> GetDevice() const;
    void SetDevice(Ptr<NetDevice> device);

    static Time GetTSym(LoraTxParameters txParams);
    static Time GetOnAirTime(Ptr<Packet> packet, LoraTxParameters txParams);

  protected:

    /* ===================== 🔥 RESTORE NS-3 TRACE MEMBERS ===================== */

    TracedCallback<Ptr<const Packet>, uint32_t> m_startSending;
    TracedCallback<Ptr<const Packet>> m_phyRxBeginTrace;
    TracedCallback<Ptr<const Packet>> m_phyRxEndTrace;
    TracedCallback<Ptr<const Packet>, uint32_t> m_successfullyReceivedPacket;
    TracedCallback<Ptr<const Packet>, uint32_t> m_underSensitivity;
    TracedCallback<Ptr<const Packet>, uint32_t> m_interferedPacket;

    /* ======================================================================= */

    /* ===================== COMBINING ===================== */

    std::map<uint32_t, std::vector<RxCopy>> m_rxBuffer;

    uint32_t m_expectedCopies = 3;
    bool m_enableCombining = true;
    Time m_combineWindow = MilliSeconds(200);

    void BufferAndMaybeCombine(uint32_t pid, const RxCopy& copy);
    void CombineAndDecide(uint32_t pid);

    double CombineSnrCoherentApprox(const std::vector<RxCopy>& v);
    Time AlignTime(const std::vector<RxCopy>& v);
    double EstimatePhaseOffset(const std::vector<RxCopy>& v);

    /* ==================================================== */

    Ptr<NetDevice> m_device;
    Ptr<LoraChannel> m_channel;

    LoraInterferenceHelper m_interference;

    RxOkCallback m_rxOkCallback;
    RxFailedCallback m_rxFailedCallback;
    TxFinishedCallback m_txFinishedCallback;

  private:
    virtual void TxFinished(Ptr<const Packet> packet) = 0;

    Ptr<MobilityModel> m_mobility;
};

} // namespace lorawan
} // namespace ns3

#endif
