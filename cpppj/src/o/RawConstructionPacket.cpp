// 원본메시지작성과송신직전의길이확정을별도단계로유지한다.
#include "o/RawConstructionPacket.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::size_t kTemporarySize=0x200,kPayloadStart=0x1b,kMaxSids=19; // Confirm의실제스택메시지크기/SID목록위치/정상상한이다.
// 원본의비정렬WORD/DWORD를주변바이트변경없이기록한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 낮은BYTE부터실제지정폭만덮어쓴다.
    for (std::size_t index=0;index<width;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
}
// 정상호출의실제패킷전체와그뒤호출자버퍼의보존을보장한다.
std::size_t WriteConstructionPacket(const ConstructionNotice& notice,const ConstructionPacketHeader& header,std::span<std::uint8_t> packet) {
    if (packet.size()<kTemporarySize || notice.sids.size()>kMaxSids) throw std::out_of_range("건설패킷의512바이트/정상SID19개범위오류");
    std::fill_n(packet.begin(),kTemporarySize,std::uint8_t{0xff});packet[0]=header.protocol&0x7f;packet[1]=header.message;
    Put(packet,2,std::max<std::uint16_t>(4,header.declaredLength),2);
    // 원본Confirm은현재decoder순서의SID목록을먼저쓴다.
    for (std::size_t index=0;index<notice.sids.size();++index) Put(packet,kPayloadStart+2*index,notice.sids[index],2);
    packet[4]=notice.type;Put(packet,5,std::bit_cast<std::uint32_t>(notice.x),4);Put(packet,9,std::bit_cast<std::uint32_t>(notice.y),4);
    packet[14]=notice.direction;packet[13]=notice.argument;packet[15]=notice.player;Put(packet,18,notice.timeLow,4);
    packet[26]=static_cast<std::uint8_t>(notice.sids.size());Put(packet,22,notice.timeHigh,4);packet[16]=notice.flags;packet[17]=notice.quality;
    return kTemporarySize;
}
// 원본은sending0일때개수진단과길이쓰기를실행하지않는다.
void FinalizeConstructionPacket(std::span<std::uint8_t> packet,bool sending,const std::function<void()>& invalidCount) {
    if (!sending) return;
    if (packet.size()<kPayloadStart) throw std::out_of_range("건설패킷길이확정의현재버퍼부족");
    if (packet[26]==255) { if (!invalidCount) throw std::logic_error("건설패킷sidCount원본진단미연결");invalidCount(); }
    Put(packet,2,static_cast<std::uint32_t>(packet[26])*2U+static_cast<std::uint32_t>(kPayloadStart),2);
}
}
