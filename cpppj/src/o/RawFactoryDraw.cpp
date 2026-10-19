// 추가 그림 억제와 기본·현재 프레임의 서로 다른 역할을 원본 순서로 유지한다.
#include "o/RawFactoryDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 비정렬 원본 DWORD를 낮은 바이트부터 읽는다.
std::uint32_t Word(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // 이웃 바이트를 읽지 않고 지정한 네 바이트만 합친다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);return value;
}
}
// TYPE 표와 판본이 다른 저장소는 실제 슬롯 읽기 전에 거부한다.
RawFactoryDraw::RawFactoryDraw(const SidPool& pool,std::span<const RiftTypeFrames> frames):pool_(pool),frames_(frames) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188) throw std::invalid_argument("Factory 그림의10.78 실제 풀/프레임 표 필요");
}
// 이름/TYPE 번호가 같아도 미복원 파생 가상 표에는 기본 동작을 적용하지 않는다.
bool RawFactoryDraw::Handles(Sid sid) const { const auto table=Word(pool_.Slot(sid),0);return table==0x509148U || table==0x509210U; }
// 해제 여부를 새로 거부하지 않는다. 원본 추가 그림은 자신의 dead 조건을 별도로 검사한다.
std::span<const std::uint8_t> RawFactoryDraw::Raw(Sid sid) const {
    const auto raw=pool_.Slot(sid);if (!Handles(sid) || raw[10]>=frames_.size()) throw std::logic_error("Factory 그림의 실제 가상 표/TYPE 미복원 SID="+std::to_string(sid.value)+" TYPE="+std::to_string(raw[10])+" VT="+std::to_string(Word(raw,0)));return raw; // 실패 시 실제 대상을 남기며 가상 분배 조건은 보존한다.
}
// 두 VT의+80은004ad0f0다. 기존 hasLevel의 OR 조건 때문에 정상 TYPE에서는 항상 상위3비트를 읽는다.
std::int32_t RawFactoryDraw::MainFrame(Sid sid) const { const auto raw=Raw(sid);return frames_[raw[10]].Run(static_cast<std::uint8_t>('A'+(raw[11]>>5))).first; }
// getter 뒤에도 현재 슬롯의 실제 프레임을 읽는다. abstract의 추가 그림 생략은 원본 몸체의 분기다.
void RawFactoryDraw::Additional(Sid sid,std::int32_t x,std::int32_t y,
    const std::function<void(std::int32_t,std::int32_t,std::uint32_t,std::int32_t)>& draw) const {
    const auto raw=Raw(sid);if ((raw[40]&9) || (raw[11]&2)) return;
    const auto base=MainFrame(sid),current=std::bit_cast<std::int32_t>(Word(Raw(sid),36));if (current==base) return;
    if (!draw) throw std::logic_error("Factory 추가 그림0044c180 실제 소비자 미연결");draw(x,y,Raw(sid)[10],current);
}
}
