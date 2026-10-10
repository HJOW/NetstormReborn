// 원본 contained 생성 보조 함수의 DWORD 판정과 좁은 raw 필드 쓰기를 유지한다.
#include "o/RawContainedCreate.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 타입·패턴·플래그 필드의 원본 바이트 위치다.
constexpr std::size_t kType=10,kArgument=12,kStoredType=18,kFlags=20;
// raw WORD 쓰기는 호스트 정렬과 엔디언에 의존하지 않는다.
void PutWord(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8);
}
}
RawContainedCreate::RawContainedCreate(SidPool& pool,const ContainedCreateState& state,ContainedCreateHooks hooks)
    :pool_(pool),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.create || !hooks_.notifySurface || !hooks_.attach) throw std::invalid_argument("contained 생성 경계 누락");
}
Sid RawContainedCreate::Create(Sid parent,std::uint32_t type,std::uint32_t argument,std::uint32_t flags,
    std::uint32_t attachFlags,std::uint32_t quality) const {
    if (parent.value==0 || parent.value>=pool_.Capacity()) throw std::out_of_range("contained 부모 SID 범위 오류");
    if (flags==0) flags=0x20;
    const std::uint32_t factoryFlags=(attachFlags&0x20) ? 1U : (attachFlags&0x10) ? 2U : 0U;
    const auto sid=hooks_.create(state_.containedType,factoryFlags);
    if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("contained 생성 SID 범위 오류");
    auto raw=pool_.AllocatedBytes(sid);PutWord(raw,kStoredType,type);PutWord(raw,kArgument,argument);
    if (raw[kType]==state_.bridgeType) hooks_.notifySurface(sid);
    raw=pool_.AllocatedBytes(sid);
    const auto qualityOffset=pool_.Edition()==OriginalEdition::Patch1078 ? 30U : 26U;
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        raw[qualityOffset]=static_cast<std::uint8_t>(quality);PutWord(raw,kFlags,flags);
    } else {
        PutWord(raw,kFlags,flags);raw[qualityOffset]=static_cast<std::uint8_t>(quality);
    }
    hooks_.attach(sid,parent,attachFlags);return sid;
}
const SidPool& RawContainedCreate::Pool() const { return pool_; }
ContainedCreateHooks MakeContainedCreateHooks(const SidPool& pool,SquidFactory& factory,ContainedCreateHooks hooks) {
    if (&factory.Pool()!=&pool) throw std::invalid_argument("contained 생성/factory SID 풀이 다릅니다");
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) {
        if (type!=6) throw std::invalid_argument("contained 전용 생성 타입은 6이어야 합니다");
        return factory.CreateContained(flags);
    };
    return hooks;
}
}
