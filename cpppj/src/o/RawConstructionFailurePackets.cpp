// 좌표float비트·비정렬금액과현재삭제뒤다음후보/타입재조회를보존한다.
#include "o/RawConstructionFailurePackets.h"
#include <bit>
#include <stdexcept>
namespace netstorm::o { // 호스트 구조체 패딩을 원문 packet으로 전송하지 않는다.
namespace { // 실제 네BYTE little endian을 명시한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(index*8);return value; } // caller가검사한실제필드만읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); } // 원문store의네BYTE만쓴다.
}
std::array<std::uint8_t,11> WriteConstructionMoneyPacket(const ConstructionMoneyNotice& notice) { std::array<std::uint8_t,11> packet{1,0x6c,11,0,notice.player,notice.source,notice.kind};Put(packet,7,std::bit_cast<std::uint32_t>(notice.amount));return packet; } // 현재signed금액을그대로보존한다.
std::array<std::uint8_t,15> WriteConstructionRejectPacket(const ConstructionRejectNotice& notice) { std::array<std::uint8_t,15> packet{1,0x2f,15,0};Put(packet,4,std::bit_cast<std::uint32_t>(notice.x));Put(packet,8,std::bit_cast<std::uint32_t>(notice.y));packet[12]=notice.type;packet[13]=notice.argument;packet[14]=notice.direction;return packet; } // 원문하위BYTE 세개를전달한다.
void ReceiveConstructionReject(std::span<const std::uint8_t> packet,ConstructionFailureWorld world,const std::function<void(Sid,std::uint32_t)>& destroy) {
    if (packet.size()<15 || world.pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("배치 취소의 실제 PATCH15BYTE 계약");const auto type=std::uint32_t(packet[12]);if (type>=world.types.size() || type>=world.frames.size()) throw std::out_of_range("배치 취소 현재 TYPE 범위");const auto& meta=world.frames[type];
    auto decoder=DecodeCanonType(meta.frames,meta.defaultFrame,world.patternTypes,{type,packet[13],packet[14],std::bit_cast<float>(Read(packet,4)),std::bit_cast<float>(Read(packet,8)),false},world.pool.Edition(),world.diagnostics);
    for (;decoder.Valid();decoder.Advance()) { // 각 현재칸을일반flags0 finder로훑는다.
        const auto& record=world.types[type];const auto area=RawConstructionClear::CellArea(decoder.X(),decoder.Y(),record.footX,record.footY);RawSquidFinder finder(world.pool,world.hash,world.types);
        for (auto sid=finder.Begin(area);sid.value;sid=finder.Next()) { const auto raw=world.pool.Slot(sid);if ((raw[40]&1U) && (raw[10]==type || raw[10]==world.noIslandType)) { if (!destroy) throw std::logic_error("배치 취소의 실제 삭제 가상 미연결");destroy(sid,8); } } // 현재abstract/type만검사하며삭제뒤에도이칸을계속한다.
    }
}
}
