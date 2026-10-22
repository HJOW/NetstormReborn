// 패킷의 WORD 대상과 내부 DWORD 목록을 구별하며 원본의100칸 한도와 덧붙이기를 보존한다.
#include "o/RawBombTimerPacket.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 선택한 큰 경계가 없으면 빈 성공 대신 실제 누락을 보고한다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("상태 타이머 패킷의 실제 하위 미연결");return hook;
}
// 비정렬 필드를 현재 패킷에서 읽으며 실제 버퍼 밖 접근을 거부한다.
std::uint64_t Read(std::span<const std::uint8_t> packet,std::size_t offset,std::size_t width) {
    if (offset>packet.size() || width>packet.size()-offset) throw std::invalid_argument("상태 타이머 패킷 길이 부족");
    std::uint64_t result{};
    for (std::size_t i=0;i<width;++i) result|=std::uint64_t(packet[offset+i])<<(8*i); // 원본 little endian 폭만 조합한다.
    return result;
}
// 확인한 필드 폭만 쓰며 헤더 밖의 caller 바이트를 보존한다.
void Write(std::span<std::uint8_t> packet,std::size_t offset,std::uint64_t value,std::size_t width) {
    if (offset>packet.size() || width>packet.size()-offset) throw std::invalid_argument("상태 타이머 출력 길이 부족");
    for (std::size_t i=0;i<width;++i) packet[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); // 하위 바이트부터 원본 폭만 기록한다.
}
}
std::uint32_t WriteBombTimerPacket(const BombEffectTimerFields& fields,Sid form,Sid parent,std::span<std::uint8_t> packet,std::uint16_t declaredLength,std::uint8_t protocol,std::uint8_t message) {
    const auto count=static_cast<std::uint16_t>(fields.members.size()); // 원본은 count DWORD의 하위WORD만 전송한다.
    const auto length=22U+2U*count; // 실제 반환 길이와 현재 헤더의 선언 길이는 별도 값이다.
    if (packet.size()<length) throw std::invalid_argument("상태 타이머 출력 버퍼 부족");
    Write(packet,0,protocol&0x7fU,1);Write(packet,1,message,1);Write(packet,2,std::max<std::uint16_t>(4,declaredLength),2);
    Write(packet,4,form.value,2);Write(packet,6,parent.value,2);Write(packet,18,fields.flags,4);Write(packet,16,count,2);Write(packet,8,std::bit_cast<std::uint64_t>(fields.endTime),8);
    for (std::size_t i=0;i<count;++i) Write(packet,22+2*i,fields.members[i],2); // SID는 이 경계에서만 WORD로 잘라 쓰며 중복/순서를 유지한다.
    return length;
}
void ReadBombTimerPacket(BombEffectTimerFields& fields,std::span<const std::uint8_t> packet) {
    if (fields.capacity>0x7fffffffU || fields.members.size()>0x7fffffffU) throw std::invalid_argument("상태 타이머의 손상된 signed 목록 범위");
    fields.flags=static_cast<std::uint32_t>(Read(packet,18,4));
    for (std::size_t i=0;i<Read(packet,16,2);++i) { // 가득 찬 뒤에도 원본처럼 입력 WORD는 끝까지 읽는다.
        const auto member=Read(packet,22+2*i,2);if (fields.members.size()<fields.capacity) fields.members.push_back(static_cast<std::uint32_t>(member));
    }
    fields.endTime=std::bit_cast<double>(Read(packet,8,8));
}
Sid BombTimerParent(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint32_t> members) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("상태 타이머 부모의 PATCH 풀/188TYPE 필요");
    for (const auto member:members) { // 순서대로 현재 TYPE를 읽으며 free/dead/void를 추가로 제외하지 않는다.
        if (member>=pool.Capacity() || member>65535) throw std::out_of_range("상태 타이머 부모의 물리 SID 범위");
        const Sid sid{static_cast<std::uint16_t>(member)};const auto kind=pool.Slot(sid)[10];if (kind>=types.size()) throw std::out_of_range("상태 타이머 부모의 TYPE 범위");
        if (types[kind].flags2&0x40000000U) return sid;
    }
    return {};
}
void ReceiveBombTimerPacket(const SidPool& pool,const std::uint32_t& boss,const std::uint32_t& processType,std::span<const std::uint8_t> packet,const BombTimerReceiveHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("상태 타이머 수신의 PATCH 풀 필요");
    if (boss) Need(hooks.assertion)(0xcc);
    const Sid initial{static_cast<std::uint16_t>(Read(packet,4,2))};
    if (!(pool.Slot(initial)[11]&1U)) { Need(hooks.assertion)(0xe6);return; }
    const auto token=Need(hooks.create)(processType,0,0); // factory의 종류만 먼저 읽고 뒤 부착에서는 현재 종류를 다시 읽는다.
    Need(hooks.read)(token,packet);
    const Sid form{static_cast<std::uint16_t>(Read(packet,4,2))},parent{static_cast<std::uint16_t>(Read(packet,6,2))};
    Need(hooks.attach)(token,processType,parent,form,0);
}
}
