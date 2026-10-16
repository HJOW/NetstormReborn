//새 표식을 만드는 효과 뒤의 좌표/owner도 다시 읽어 포스 필드에 전달한다.
#include "o/RawFenceRebuild.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
//네 BYTE float의 비정렬 원본 비트를 읽는다.
float FenceRebuildCoordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{}; //현재 좌표 DWORD다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8); //정확한 네 바이트만 결합한다.
    return std::bit_cast<float>(value);
}
//미선택 경계는 요구하지 않으며 선택된 누락만 거부한다.
template<class Callback> const Callback& NeedRebuild(const Callback& callback) { if (!callback) throw std::logic_error("썬 바리케이드 재생성의 실제 하위 미연결");return callback; }
}
//CD의 다른 타입 배치를 PATCH로 취급하지 않는다.
RawFenceRebuild::RawFenceRebuild(const SidPool& pool,FenceRebuildHooks hooks):pool_(pool),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("썬 바리케이드 재생성 판본 오류");
}
//형식화한 사본을 오래 보관하지 않고 각 호출 직전에 현재 raw를 읽는다.
std::array<float,4> RawFenceRebuild::Endpoints(Sid parent,Sid other) const {
    const auto a=pool_.Slot(parent),b=pool_.Slot(other);return {FenceRebuildCoordinate(a,14),FenceRebuildCoordinate(a,18),FenceRebuildCoordinate(b,14),FenceRebuildCoordinate(b,18)};
}
//부모extra는 진입 때만 검사하고 하위 효과 뒤 대상extra/owner/좌표는 다시 읽는다.
void RawFenceRebuild::Rebuild(Sid parent) const {
    if (pool_.Slot(parent)[40]&9U) return;
    for (std::uint32_t direction=0;direction<8;direction+=2) { //원본 네 방향0/2/4/6의 순서다.
        const auto other=NeedRebuild(hooks_.neighbor)(parent,direction);if (!other.value) continue;
        const auto within=NeedRebuild(hooks_.withinRange)(parent,Endpoints(parent,other));if (!within || (pool_.Slot(other)[40]&0x21U)) continue;
        NeedRebuild(hooks_.markers)(parent,other);
        const auto owners=std::array<std::uint32_t,2>{pool_.Slot(parent)[34],pool_.Slot(other)[34]}; //표식 생성 뒤 현재 owner들이다.
        NeedRebuild(hooks_.shields)(Endpoints(parent,other),owners[0],owners[1],parent.value,other.value);
    }
}
}
