#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lorawan-module.h"

#include <map>
#include <vector>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LoraCombining");

// Storage for combining
std::map<uint32_t, std::vector<double>> snrMap;
std::map<uint32_t, std::vector<double>> timeMap;

static uint32_t globalPacketId = 0;

// Simulated receive callback (hook)
void ReceivePacket(uint32_t packetId, double snr)
{
    double time = Simulator::Now().GetSeconds();

    snrMap[packetId].push_back(snr);
    timeMap[packetId].push_back(time);

    NS_LOG_UNCOND("RX PacketID=" << packetId
                  << " SNR=" << snr
                  << " Time=" << time);
}

// Simulate sending repeated packets
void SendRepeated(uint32_t packetId, int N, double gap)
{
    for (int i = 0; i < N; i++)
    {
        Simulator::Schedule(Seconds(i * gap), [packetId]() {

            // Simulate noisy SNR (example: -25 ± noise)
            double baseSnr = -25.0;
            double noise = (rand() % 10 - 5); // [-5, +5]
            double snr = baseSnr + noise;

            ReceivePacket(packetId, snr);
        });
    }
}

// Combine after simulation
void CombineAll()
{
    std::cout << "\n===== COMBINING RESULTS =====\n";

    for (auto &entry : snrMap)
    {
        uint32_t id = entry.first;
        auto &snrs = entry.second;

        double linearSum = 0;

        for (double snr : snrs)
        {
            linearSum += pow(10.0, snr / 10.0);
        }

        double combinedSnr = 10 * log10(linearSum);

        // Timing average
        auto &times = timeMap[id];
        double avgTime = 0;

        for (double t : times) avgTime += t;
        avgTime /= times.size();

        std::cout << "Packet " << id
                  << " | Copies=" << snrs.size()
                  << " | Combined SNR=" << combinedSnr
                  << " | Avg Time=" << avgTime
                  << std::endl;
    }
}

int main(int argc, char *argv[])
{
    int N = 3;          // repetitions
    double gap = 0.05; // seconds

    CommandLine cmd;
    cmd.AddValue("N", "Number of repetitions", N);
    cmd.Parse(argc, argv);

    // Simulate multiple packets
    for (int i = 0; i < 5; i++)
    {
        uint32_t id = globalPacketId++;
        Simulator::Schedule(Seconds(i * 1.0), &SendRepeated, id, N, gap);
    }

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    CombineAll();

    Simulator::Destroy();
    return 0;
}
