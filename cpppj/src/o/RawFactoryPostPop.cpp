// 원본 Factory의 실제 공간 순회와 callback 뒤 raw 재조회를 보존한다.
#include "o/RawFactoryPostPop.h"
#include "o/SquidFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 정렬되지 않은 little endian DWORD를 현재 슬롯에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 현재 필드의 네 바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 원본 float 좌표는 전달할 때 비트와 소수 부분을 그대로 유지한다.
float Position(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(Read(raw,offset)); }
// CRT ftol의 signed64 범위와 low DWORD를 유지하며 손상 입력은 거부한다.
std::int32_t Truncate(float value) {
    const auto wide=static_cast<double>(value);
    if (!std::isfinite(wide) || wide<-9223372036854775808.0 || wide>=9223372036854775808.0)
        throw std::out_of_range("Factory 좌표 CRT 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(wide)));
}
// 두 판본 모두 목록 용량 안에서 중복을 건너뛰고 SID를 추가한다.
void AddUnique(SquidPostPopList& list,Sid sid) {
    const auto items=list.Items();
    if (list.count<list.entries.size() && std::ranges::find(items,sid.value)==items.end()) list.entries[list.count++]=sid.value;
}
}
// 같은 raw 저장소의 공통 후처리와 Damageable만 조합한다.
RawFactoryPostPop::RawFactoryPostPop(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,SquidPostPop& base,const RawCarrierPostPop& damageable,
    AssetPostPopState& state,FactoryPostPopHooks hooks,FactoryPostPopTypes numbers)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),base_(base),damageable_(damageable),state_(state),hooks_(std::move(hooks)),numbers_(numbers) {
    if (&pool!=&base.Pool() || &pool!=&damageable.Pool() || types.size()!=frames.size() ||
        types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U))
        throw std::invalid_argument("Factory 후처리 풀/판본/프레임 표 불일치");
}
// 실제 생성자가 기록하는 vtable 시작 주소이며 Ghidra의 표 조각 시작 주소와 구별한다.
std::uint32_t RawFactoryPostPop::Kind(Sid sid) const {
    const auto table=Read(pool_.Slot(sid),0);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (table==(patch ? 0x509148U : 0x503298U)) return 1;
    if (table==(patch ? 0x509210U : 0x503358U)) return 2;
    return 0;
}
// 해당 네 vtable의 가상 +0x80은 같은 base 상태 getter다. 원본의 항상 참 OR 조건도 반영한다.
std::int32_t RawFactoryPostPop::Count(Sid sid) const { return pool_.Slot(sid)[11]>>5; }
// 원본 타입 이름이 일치해도 다른 가상 표의 wrapper는 허용하지 않는다.
bool RawFactoryPostPop::Handles(Sid sid) const { return Kind(sid)!=0; }
// 실제 흐름에서 쓰는 효과만 요구하되 callback 뒤 분기 변화도 필수 경계로 유지한다.
void RawFactoryPostPop::Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop) const {
    const auto raw=pool_.Slot(sid);const auto kind=Kind(sid);
    if (!kind || raw[10]>=types_.size()) throw std::invalid_argument("Factory 후처리 가상 표/타입 미복원");
    base_.ValidateBase(sid,flags,pop);
    if ((flags&1) && (!hooks_.reserveRegular || !hooks_.addRegular)) throw std::invalid_argument("Factory Regular 경계 누락");
    if (!hooks_.setFrame) throw std::invalid_argument("Factory 실제 프레임 설정 경계 누락");
    if (state_.boss && (!hooks_.destroy || !hooks_.warnDuplicate)) throw std::invalid_argument("Factory 중복 삭제/경고 경계 누락");
    if (kind==2) {
        static_cast<void>(state_.aviaries.Items());static_cast<void>(state_.residences.Items());
        if (!hooks_.aviaryOwnership) throw std::invalid_argument("아비어리 실제 섬 소유권 경계 누락");
    }
}
// raw currentType/state/frame을 효과 후 다시 읽고 원본 signed DWORD 덧셈 감김을 유지한다.
void RawFactoryPostPop::AdjustFrame(Sid sid) const {
    if (pool_.Slot(sid)[10]==numbers_.fixedFrame) return;
    const auto frame=pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(pool_.Slot(sid),36)) : pool_.Slot(sid)[34];
    const auto run=frames_[pool_.Slot(sid)[10]].Run(static_cast<std::uint8_t>('A'+Count(sid)));
    if (run.first<=frame) {
        const auto current=frames_[pool_.Slot(sid)[10]].Run(static_cast<std::uint8_t>('A'+Count(sid)));
        const auto end=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(run.first)+static_cast<std::uint32_t>(current.count));
        if (frame<end) return;
    }
    hooks_.setFrame(sid,run.first,0);
}
// 각각의 실패는 해당 예약만 건너뛰며 다음 호출 순서는 유지한다.
void RawFactoryPostPop::Regular(Sid sid,std::uint32_t event,float delay) const { if (hooks_.reserveRegular()) hooks_.addRegular(sid,event,delay); }
// wrapper 목록·예약·공통 몸체·프레임·현재 공간 순서가 원본과 같다.
void RawFactoryPostPop::PostPop(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto extra=patch ? 40U : 35U;
    if (Kind(sid)==2 && (flags&1) && !(pool_.Slot(sid)[extra]&9)) {
        AddUnique(state_.aviaries,sid);AddUnique(state_.residences,sid);
        hooks_.aviaryOwnership(Position(pool_.Slot(sid),14),Position(pool_.Slot(sid),18));
    }
    if (flags&1) {
        if (pool_.Slot(sid)[10]==numbers_.aviary) Regular(sid,2,8.0f);
        if (pool_.Slot(sid)[10]==numbers_.secondTimer) Regular(sid,1,0.5f);
        if (patch) Regular(sid,5,0.5f);
    }
    damageable_.DamageablePostPop(sid,flags);AdjustFrame(sid);
    if (!state_.boss) return;
    const int x=Truncate(Position(pool_.Slot(sid),14)),y=Truncate(Position(pool_.Slot(sid),18));
    RawSquidFinder finder(pool_,hash_,types_);auto candidate=finder.Begin({x,y,x,y});
    // 실제 finder는 다음 raw 연결을 먼저 저장하므로 가상 삭제 뒤에도 원본 순서로 진행한다.
    while (candidate.value) {
        if (candidate!=sid && pool_.Slot(candidate)[10]==pool_.Slot(sid)[10] &&
            !((pool_.Slot(candidate)[extra]^pool_.Slot(sid)[extra])&8)) {
            hooks_.warnDuplicate(candidate,Truncate(Position(pool_.Slot(candidate),14)),Truncate(Position(pool_.Slot(candidate),18)));
            hooks_.destroy(candidate,0);
        }
        candidate=finder.Next();
    }
}
// 분배기의 동일 풀 계약에 사용한다.
const SidPool& RawFactoryPostPop::Pool() const { return pool_; }
// 다른 풀의 가상 프레임 효과를 거부하며 실제 dirty/공간 순서를 그대로 호출한다.
FactoryPostPopHooks MakeFactoryPostPopHooks(const SidPool& pool,SquidFrame& frame,FactoryPostPopHooks hooks) {
    if (&pool!=&frame.Pool()) throw std::invalid_argument("Factory 프레임 지정 풀 불일치");
    hooks.setFrame=[&frame](Sid sid,std::int32_t value,std::uint32_t flags) { frame.Set(sid,value,flags); };
    return hooks;
}
}
