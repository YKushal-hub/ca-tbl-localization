#ifndef PACKET_ID_TAG_H
#define PACKET_ID_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class PacketIdTag : public Tag {
public:
  static TypeId GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::PacketIdTag")
      .SetParent<Tag> ()
      .AddConstructor<PacketIdTag> ();
    return tid;
  }
  TypeId GetInstanceTypeId () const override { return GetTypeId (); }

  void SetPacketId (uint32_t id) { m_id = id; }
  uint32_t GetPacketId () const { return m_id; }

  uint32_t GetSerializedSize () const override { return 4; }
  void Serialize (TagBuffer i) const override { i.WriteU32 (m_id); }
  void Deserialize (TagBuffer i) override { m_id = i.ReadU32 (); }
  void Print (std::ostream &os) const override { os << "pid=" << m_id; }

private:
  uint32_t m_id = 0;
};

} // namespace ns3
#endif
