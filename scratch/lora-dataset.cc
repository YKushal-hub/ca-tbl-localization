#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lorawan-module.h"

#include "ns3/lora-helper.h"
#include "ns3/lora-phy-helper.h"
#include "ns3/lorawan-mac-helper.h"

#include "ns3/packet-id-tag.h"

#include <fstream>
#include <cstdlib>
#include <ctime>

using namespace ns3;
using namespace ns3::lorawan;

std::ofstream datasetFile;

// -------- PARAMETERS --------
double TX_INTERVAL = 0.1;
double JITTER_MAX = 0.02;

uint32_t globalPacketId = 0;
uint32_t REPETITIONS = 3;
uint32_t packetsGenerated = 0;

/* ========================= TX FUNCTION ========================= */

void GeneratePacketData(
    NodeContainer endDevices,
    NetDeviceContainer endDevicesNetDevices,
    uint32_t maxPackets)
{
    double baseTime = Simulator::Now().GetSeconds();

    for (uint32_t i = 0; i < endDevices.GetN(); i++)
    {
        Ptr<MobilityModel> devMob = endDevices.Get(i)->GetObject<MobilityModel>();
        Vector devPos = devMob->GetPosition();

        uint32_t packetId = globalPacketId++;

        for (uint32_t rep = 0; rep < REPETITIONS; rep++)
        {
            double jitter = ((rand() / (double)RAND_MAX) * 2 - 1) * JITTER_MAX;
            double txTime = baseTime + jitter + rep * 0.02;

            Simulator::Schedule(Seconds(txTime), [=]() {

                Ptr<Packet> packet = Create<Packet>(20);

                // 🔥 Packet ID tag (required for combining)
                PacketIdTag tag;
                tag.SetPacketId(packetId);
                packet->AddPacketTag(tag);

                Ptr<NetDevice> dev = endDevicesNetDevices.Get(i);

                if (dev == nullptr)
                {
                    NS_LOG_UNCOND("ERROR: Device is NULL");
                    return;
                }

                // ✔ Correct send method
                dev->Send(packet, dev->GetBroadcast(), 0);

                datasetFile << packetId << ","
                            << i << ","
                            << txTime << ","
                            << devPos.x << ","
                            << devPos.y
                            << std::endl;
            });
        }
    }

    packetsGenerated++;

    if (packetsGenerated < maxPackets)
    {
        Simulator::Schedule(Seconds(TX_INTERVAL),
                            &GeneratePacketData,
                            endDevices,
                            endDevicesNetDevices,
                            maxPackets);
    }
}

/* ========================= MAIN ========================= */

int main(int argc, char *argv[])
{
    srand(time(NULL));

    uint32_t nDevices = 100;
    uint32_t nGateways = 4;
    uint32_t packetsPerNode = 200;

    CommandLine cmd;
    cmd.AddValue("nDevices", "Number of end devices", nDevices);
    cmd.Parse(argc, argv);

    datasetFile.open("lora_tx_dataset.csv");
    datasetFile << "PacketID,NodeID,TxTime,X,Y\n";

    /* ========================= NODES ========================= */

    NodeContainer endDevices;
    endDevices.Create(nDevices);

    NodeContainer gateways;
    gateways.Create(nGateways);

    /* ========================= MOBILITY ========================= */

    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();

    for (uint32_t i = 0; i < nDevices; i++)
    {
        pos->Add(Vector(rand() % 1000, rand() % 1000, 0));
    }

    MobilityHelper mobility;
    mobility.SetPositionAllocator(pos);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(endDevices);

    MobilityHelper gwMobility;
    gwMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gwMobility.Install(gateways);

    /* ========================= CHANNEL (CRITICAL FIX) ========================= */

    Ptr<LogDistancePropagationLossModel> loss =
        CreateObject<LogDistancePropagationLossModel>();

    Ptr<ConstantSpeedPropagationDelayModel> delay =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    Ptr<LoraChannel> channel =
        CreateObject<LoraChannel>(loss, delay);

    /* ========================= LORA SETUP ========================= */

    LoraHelper loraHelper;
    LoraPhyHelper phyHelper = LoraPhyHelper();
    LorawanMacHelper macHelper = LorawanMacHelper();

    // 🔥 Attach channel (THIS FIXES CRASH)
    phyHelper.SetChannel(channel);

    // -------- End Devices --------
    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);

    NetDeviceContainer endDevicesNetDevices =
        loraHelper.Install(phyHelper, macHelper, endDevices);

    // -------- Gateways --------
    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);

    NetDeviceContainer gatewayDevices =
        loraHelper.Install(phyHelper, macHelper, gateways);

    /* ========================= SCHEDULE ========================= */

    Simulator::Schedule(Seconds(TX_INTERVAL),
                        &GeneratePacketData,
                        endDevices,
                        endDevicesNetDevices,
                        packetsPerNode);

    Simulator::Stop(Seconds(packetsPerNode * TX_INTERVAL + 2));

    Simulator::Run();
    Simulator::Destroy();

    datasetFile.close();

    return 0;
}
