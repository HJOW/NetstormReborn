// PATCH004ae560/004aeb40/004ac340 ↔ CD004af750/004aff30/004abc10.
#include "o/RawFormPacket.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 비정렬 원본 WORD/DWORD를 실제 폭대로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0; // little endian 조립 결과다.
    // 필드의 각 바이트만 읽고 인접 payload는 섞지 않는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);
    return value;
}
// 원본 header/WORD 필드의 지정 폭만 덮어쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 낮은 바이트부터 쓴다. 반환 크기 뒤의 옛 버퍼는 보존한다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 004df590/CD00418b60의 erase0 경로다. 반환 크기와 선언 길이를 같게 강제하지 않는다.
void Header(std::span<std::uint8_t> packet,FormPacketHeader header,std::size_t size) {
    if (packet.size()<size) throw std::out_of_range("Form 패킷 버퍼 부족");
    packet[0]=header.protocol&0x7f;packet[1]=header.message;Put(packet,2,std::max<std::uint16_t>(4,header.length),2);
}
}
// 게임 상태의 사본을 만들지 않고 현재 참조만 보관한다.
RawFormPacket::RawFormPacket(const SidPool& pool,const FormPacketHeaders& headers,FormDeletionStamp& stamp)
    :pool_(pool),headers_(headers),stamp_(stamp) {}
// 원본은0..4를 이 assert에서 client SID로 분류하지 않는다.
void RawFormPacket::ServerSid(std::uint16_t sid) const {
    if (sid>=5 && sid<pool_.Layout().serverFirst) throw std::logic_error("Form 패킷의 client SID 원본 assert");
}
// ProcessForm은 타입 BYTE가 프로세스 번호여도 같은 생성자 VT의+0 getter를 사용한다.
std::uint16_t RawFormPacket::Parent(Sid form) const {
    const auto raw=pool_.Slot(form);const auto table=Read(raw,0,4); // 호출 시점의 현재 raw VT다.
    constexpr std::array<std::uint32_t,4> kPatch{0x512110,0x512098,0x512188,0x512208}; // Depend/Process/Gump/Content의 실제 VT다.
    constexpr std::array<std::uint32_t,4> kCd{0x505a08,0x505990,0x505a80,0x505b00}; // 같은 CD 네 생성자 VT다.
    const auto& tables=pool_.Edition()==OriginalEdition::Patch1078 ? kPatch : kCd; // 판본에 맞는 실제 가상 getter만 허용한다.
    if (std::find(tables.begin(),tables.end(),table)==tables.end()) throw std::logic_error("Form 부모 가상 getter 미복원");
    return static_cast<std::uint16_t>(Read(raw,16,2));
}
// 실제 class VT로 두 몸체를 선택하며 free/dead/void 필터를 추가하지 않는다.
std::uint16_t RawFormPacket::WriteData(Sid form,std::span<std::uint8_t> packet) const {
    const auto raw=pool_.Slot(form);const auto table=Read(raw,0,4); // 원본의 현재 가상+54 선택 기준이다.
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 내용물의 추가 WORD/순번 BYTE 위치가 다르다.
    const bool dependency=table==(patch ? 0x512110U : 0x505a08U); // 감시 Form의13바이트 writer다.
    if (!dependency && table!=(patch ? 0x512208U : 0x505b00U)) throw std::logic_error("다른 Form 가상+54 writer 미복원");
    const auto parent=Parent(form);ServerSid(parent); // 원본도 parent의 server 번호를 먼저 검사한다.
    if (dependency) {
        if (!pool_.IsServer()) throw std::logic_error("감시 Form writer의 원본 server assert");
        // 상대 Form과 그 부모도 server 번호여야 한다. 현재0인 상대는 getter를 호출하지 않는다.
        for (const std::size_t offset:{18U,20U}) {
            const auto other=static_cast<std::uint16_t>(Read(raw,offset,2)); // 실제 watcher/watched Form 번호다.
            if (other) { ServerSid(other);ServerSid(Parent(Sid{other})); }
        }
        Header(packet,headers_.dependency,13);Put(packet,4,form.value,2);Put(packet,6,parent,2);packet[8]=raw[11];
        Put(packet,9,Read(raw,18,2),2);Put(packet,11,Read(raw,20,2),2);return 13;
    }
    Header(packet,headers_.content,18);Put(packet,4,form.value,2);Put(packet,6,Read(raw,12,2),2);
    Put(packet,8,Read(raw,18,2),2);Put(packet,10,Read(raw,20,2),2);Put(packet,12,Read(raw,patch ? 24U : 23U,2),2);
    Put(packet,14,Read(raw,16,2),2);packet[16]=raw[patch ? 23U : 22U];packet[17]=raw[11];return 18;
}
// 원본 wrapper의 세 번째 인자는 쓰지 않고 네 번째 flags의 상위 WORD만 소비한다.
std::uint16_t RawFormPacket::WriteDeletion(Sid sid,std::uint32_t flags,std::span<std::uint8_t> packet) const {
    ServerSid(sid.value);Header(packet,headers_.deletion,8);Put(packet,4,sid.value,2);
    std::uint16_t token=0; // 현재 SID와 일치하지 않으면 원본의 최근 표식 소비는0이다.
    if (stamp_.sid==sid.value) { token=static_cast<std::uint16_t>(stamp_.token);stamp_.sid=0; }
    Put(packet,6,((flags>>16)&0xfff0U)|token,2);return 8;
}
}
