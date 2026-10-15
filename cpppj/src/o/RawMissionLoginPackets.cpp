// sendFort의image+20예약과image+17선언을구분하고현재선택을원문위치마다다시읽는다.
#include "o/RawMissionLoginPackets.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kStride=0xb4,kOwners=9,kImageOffset=0x11,kReservationOverhead=0x14; //실제Player폭/개수와0x2a의두다른크기다.
// 비정렬DWORD/WORD를원본little endian필드에쓴다.
void Write(std::span<std::uint8_t> packet,std::size_t offset,std::uint32_t value,std::size_t size=4) {
    // 지정폭의하위BYTE만원문순서로기록한다.
    for (std::size_t byte=0;byte<size;++byte) packet[offset+byte]=static_cast<std::uint8_t>(value>>(8*byte));
}
// 실제packet필드의원본폭을현재배열에서읽는다.
std::uint32_t Read(std::span<const std::uint8_t> packet,std::size_t offset,std::size_t size) {
    std::uint32_t value{};
    // 정렬가정없이낮은BYTE부터조립한다.
    for (std::size_t byte=0;byte<size;++byte) value|=std::uint32_t(packet[offset+byte])<<(8*byte);return value;
}
// 실제선택소비자누락은해당위치에서거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("sendFort 실제 선택 소비자 미연결");return function; }
}
// 길이observer뒤실제wire범위만반환하며예약tail을전송하지않는다.
std::span<const std::uint8_t> MissionFortPacket::Bytes() const {
    if (storage.size()<4) throw std::out_of_range("sendFort packet 헤더 물리 범위 오류");const auto length=Read(storage,2,2);
    if (length<4 || length>storage.size()) throw std::out_of_range("sendFort 선언 길이 물리 범위 오류");return std::span(storage).first(length);
}
// 실제helper는전체예약을ff로채운뒤헤더와현재필드만쓴다.끝3BYTE는손대지않는다.
MissionFortPacket MakeMissionFortPacket(std::uint32_t owner,std::uint32_t predictable,std::uint8_t watcher,std::span<const std::uint8_t> image,std::uint16_t initialLength) {
    if (image.size()>RawFortImage::kCapacity) throw std::out_of_range("sendFort 실제 이미지 할당 범위 초과");
    MissionFortPacket packet;packet.storage.resize(image.size()+kReservationOverhead,std::uint8_t{0xff});
    packet.storage[0]=1;packet.storage[1]=0x2a;Write(packet.storage,2,std::max<std::uint32_t>(initialLength,4),2);
    Write(packet.storage,4,owner);Write(packet.storage,8,predictable);Write(packet.storage,12,static_cast<std::uint32_t>(image.size()));packet.storage[16]=watcher;
    std::copy(image.begin(),image.end(),packet.storage.begin()+kImageOffset);return packet;
}
// observer의false는아무것도읽지않는다.정상true에서는원본하위WORD덧셈을보존한다.
void FinalizeMissionFortPacket(std::span<std::uint8_t> packet,bool writeLength,std::function<void()> assertion) {
    if (!writeLength) return;
    if (packet.size()<kImageOffset) throw std::out_of_range("sendFort 길이 observer 물리 범위 오류");const auto size=Read(packet,12,4);
    if (size==0xffffffffU) { if (!assertion) throw std::logic_error("sendFort 실제 크기 진단 미연결");assertion(); }
    Write(packet,2,size+static_cast<std::uint32_t>(kImageOffset),2);
}
// 실제현재조회없이값형기본Template를삽입하지않는다.
RawMissionLoginPackets::RawMissionLoginPackets(OriginalEdition edition,std::span<const std::uint8_t> players,MissionFortPacketReads reads,FortImageHooks image,MissionFortPacketHooks hooks)
    :players_(players),reads_(std::move(reads)),image_(std::move(image)),hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078 || players.size()!=kStride*kOwners || !reads_.current || !reads_.initialLength || !image_.typeCount)
        throw std::invalid_argument("sendFort의 PATCH 현재 Player/Template/길이/타입 조회 누락");
}
// 원본wrapper는manager존재검사와실제Save대상조회도합치지않는다.
bool RawMissionLoginPackets::SendFort(std::uint32_t owner,std::int32_t target,std::uint32_t predictable) const {
    if (owner>=kOwners) throw std::out_of_range("sendFort 현재 Template owner 범위 오류");
    if (!reads_.current(owner)) return false;
    Need(hooks_.log)(owner,target);
    if (!reads_.current(owner)) Need(hooks_.missingTemplate)();
    const auto* current=reads_.current(owner);if (!current) throw std::logic_error("sendFort Save의 현재 Template manager NULL");
    const auto image=RawFortImage(OriginalEdition::Patch1078,*current,image_).Save();
    const auto initialLength=reads_.initialLength();const auto watcher=players_[owner*kStride+0x58]; //원본은길이WORD조회/헤더작성뒤현재watcher를읽는다.
    auto packet=MakeMissionFortPacket(owner,predictable,watcher,image.Bytes(),initialLength);
    FinalizeMissionFortPacket(packet.storage,true);Need(hooks_.send)(target,packet.Bytes());return true;
}
}
