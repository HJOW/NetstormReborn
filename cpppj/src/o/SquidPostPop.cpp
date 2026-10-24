// 실제 공통 후처리에서 선택한 효과만 구현하며 미복원 호출을 변경 전에 거부한다.
#include "o/SquidPostPop.h"
#include "o/RawPriestPostPopTail.h"
#include "o/RawForcefieldLifecycle.h"
#include "o/RawCarrierPostPop.h"
#include "o/RawCarrierSteal.h"
#include "o/RawBalloonBody.h"
#include "o/RawAssetPostPop.h"
#include "o/RawFactoryPostPop.h"
#include "o/RawRunePostPop.h"
#include "o/RawDaisLifecycle.h"
#include "o/RawDudeSpawn.h"
#include "o/RawPlayerWorld.h"
#include "o/RawBuriedPostPop.h"
#include "o/RawFlyerProcess.h"
#include "o/SquidPop.h"
#include "o/RawIslandPostPop.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// raw 상태/extra 필드와 후처리 분기의 원본 마스크다.
constexpr std::uint8_t kFree=1,kContained=8,kAbstractBuried=9;
constexpr std::uint32_t kRegionMask=0x50444208,kFactoryMask=0x4200,kProvider=0x10000000;
constexpr std::uint32_t kNoGraph=0x2000000,kGraphChange=0x203,kDestroyGraph=0x800;
// 패치 비용 표가 타입 번호마다 더해 저장하는 정수다.
constexpr std::int64_t kCostBias=23;
// 실제 다리 가상 표의 postPop은 공통 함수 앞에 연결 접두 처리를 수행한다.
constexpr std::uint32_t kPatchBridgeVtable=0x005034c8,kCdBridgeVtable=0x00501ab0;
// 원본 가상 표 +0x20이 직접 Carrier 00427720/CD 004e6360인 주소다.
constexpr std::array<std::uint32_t,3> kPatchCarrierVtables{0x00503d80,0x00503e50,0x0050e568};
constexpr std::array<std::uint32_t,2> kCdCarrierVtables{0x00503ee8,0x00506ea0};
// 원본 가상 표 +0x20이 직접 Damageable 0044bf80/CD 004621c0인 주소다.
constexpr std::array<std::uint32_t,2> kPatchDamageableVtables{0x0050a820,0x0050a8e8};
constexpr std::array<std::uint32_t,2> kCdDamageableVtables{0x00500a58,0x00500b10};
// raw 가상 주소를 호스트 포인터로 해석하지 않고 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> bytes) {
    std::uint32_t value=0;
    // little endian DWORD를 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
// 원본은 가득 찬 목록에서 추가를 생략하며 기존 항목/메모리는 보존한다.
void Append(SquidPostPopList& list,Sid sid) {
    list.Append(sid.value);
}
// DWORD 집계를 signed 해석으로 다시 읽는다. 부호 있는 덧셈 넘침은 만들지 않는다.
std::int32_t Wrapped(std::int64_t value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)); }
}
// 원본 배열의 count 밖 stale 메모리는 유지하며 조회에서는 노출하지 않는다.
std::span<const std::uint32_t> SquidPostPopList::Items() const {
    const auto used=Count();if (used>Capacity()) throw std::logic_error("postPop 목록 count 범위 오류");
    return Storage().first(used);
}
// 기존 독립 자료가 있으면 별도 숨은 사본을 남기지 않고 연결을 거부한다.
void SquidPostPopList::BindCurrent(CurrentBinding binding) {
    if (current.storage || !entries.empty() || count || !binding.storage || !binding.capacity || !binding.readCount || !binding.writeCount)
        throw std::logic_error("postPop 현재 목록 연결의 소유/장치 오류");
    current=std::move(binding);
}
std::uint32_t SquidPostPopList::Count() const { return current.readCount ? current.readCount() : count; } // 실제 개수 또는 기존 독립 개수다.
std::size_t SquidPostPopList::Capacity() const { return current.capacity ? current.capacity() : entries.size(); } // 실제 용량 또는 독립 vector 크기다.
std::span<std::uint32_t> SquidPostPopList::Storage() { return current.storage ? current.storage() : std::span(entries); } // 현재 물리만 빌린다.
std::span<const std::uint32_t> SquidPostPopList::Storage() const { return current.storage ? current.storage() : std::span<const std::uint32_t>(entries); } // 복사하지 않는다.
void SquidPostPopList::SetCount(std::uint32_t value) { if (current.writeCount) current.writeCount(value);else count=value; } // 같은 현재 DWORD만 바꾼다.
// 가득 찬 목록에서는 배열 포인터를 해석하지 않는다. 실제 원문도 store 뒤 개수를 증가시킨다.
void SquidPostPopList::Append(std::uint32_t value) {
    const auto used=Count();if (used>=Capacity()) return;
    Storage()[used]=value;SetCount(used+1);
}
// 현재 활성 항목만 방문하여 모든 중복을 빼고 상대 순서와 stale 꼬리를 보존한다.
void SquidPostPopList::RemoveAll(std::uint32_t value) {
    static_cast<void>(Items());std::uint32_t removed{}; // 압축 중 제거한 개수다.
    for (std::uint32_t index=0;index<Count();++index) { // 원문의 현재 count/배열을 같은 목록에서 읽는다.
        const auto entry=Storage()[index];if (entry==value) ++removed;else Storage()[index-removed]=entry;
    }
    SetCount(Count()-removed);
}
// 슬롯 byte type가 가리키는 타입 표를 검증한다.
SquidPostPop::SquidPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& state,RawGraph* graph,
    std::function<void(Sid)> bridgeConnector)
    :pool_(pool),types_(types.begin(),types.end()),state_(state),graph_(graph),bridgeConnector_(std::move(bridgeConnector)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count) throw std::invalid_argument("postPop 타입 판본/크기 오류");
    if (graph_ && &graph_->Pool()!=&pool_) throw std::invalid_argument("postPop/Graph SID 풀이 다릅니다");
    if (graph_) graph_->ValidateTypes(types);
}
// 실제 vtable과 명시적인 연결 효과 둘 다 있어야 다리의 파생 후처리를 사용할 수 있다.
bool SquidPostPop::HandlesBridge(Sid sid) const {
    const auto expected=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchBridgeVtable : kCdBridgeVtable;
    return bridgeConnector_ && Vtable(pool_.Slot(sid))==expected;
}
// 접두 효과는 생성 뒤에 연결한다(연결 계층이 Pop을 거쳐 이 객체를 다시 부르기 때문이다).
void SquidPostPop::SetIslandPrefix(std::function<void(Sid,std::uint32_t)> prefix) { islandPrefix_=std::move(prefix); }
// 실제 vtable과 명시적인 접두 효과 둘 다 있어야 섬 받침의 파생 후처리를 사용할 수 있다.
bool SquidPostPop::HandlesIsland(Sid sid) const {
    const auto expected=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchIslandVtable : kCdIslandVtable;
    return islandPrefix_ && Vtable(pool_.Slot(sid))==expected;
}
// 파생 표는 서로 다른 주소이므로 한 종류만 참이 된다.
bool SquidPostPop::HandlesDerived(Sid sid) const { return HandlesCarrierSteal(sid) || HandlesBridge(sid) || HandlesIsland(sid) || HandlesSurface(sid) || HandlesPriest(sid) || HandlesForcefield(sid) || HandlesCarrier(sid) || HandlesDamageable(sid) || HandlesAsset(sid) || HandlesFactory(sid) || HandlesRune(sid) || HandlesDais(sid) || HandlesDude(sid) || HandlesPlayer(sid) || HandlesBuried(sid) || HandlesFlyer(sid) || HandlesBalloon(sid); }
// 정확한5040c0과 같은 풀의 장기 몸체를 확인한다.
void SquidPostPop::SetBalloonPostPop(const RawBalloonBody* postPop) { if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("벌룬 PostPop/공통 풀 불일치");balloonPostPop_=postPop; }
bool SquidPostPop::HandlesBalloon(Sid sid) const { return balloonPostPop_ && balloonPostPop_->Handles(sid); } //동일 생성자 표 이외는 처리하지 않는다.
// 단일 풀과 실제 전체 몸체 둘 다 확인한다.
void SquidPostPop::SetFlyerPostPop(const RawFlyerPostPop* postPop) { if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("비행 PostPop/공통 풀 불일치");flyerPostPop_=postPop; }
bool SquidPostPop::HandlesFlyer(Sid sid) const { return flyerPostPop_ && flyerPostPop_->Handles(sid); } // 다른 비행 파생 표를 임의로 허용하지 않는다.
// 다른 월드의 QA와 장부를 조합하지 않도록 실제 풀을 확인한다.
void SquidPostPop::SetBuriedPostPop(const RawBuriedPostPop* postPop) { if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("buried Pop/후처리 풀 불일치");buriedPostPop_=postPop; }
// 주소 표를 넓히지 않고 완성한 실제 몸체에 파생 지원 여부를 위임한다.
bool SquidPostPop::HandlesBuried(Sid sid) const { return buriedPostPop_ && buriedPostPop_->Handles(sid); }
// 지원 주소만 늘리지 않고 실제 전체 몸체와 동일 풀을 확인한다.
void SquidPostPop::SetFactoryPostPop(const RawFactoryPostPop* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("Factory Pop/후처리 풀 불일치");
    factoryPostPop_=postPop;
}
// 다른 타입의 기본 함수와 겹치지 않는 실제 가상 표로 분배한다.
bool SquidPostPop::HandlesFactory(Sid sid) const { return factoryPostPop_ && factoryPostPop_->Handles(sid); }
// 실제Rune후처리의같은풀을확인한경우만파생분배에등록한다.
void SquidPostPop::SetRunePostPop(const RawRunePostPop* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("Rune Pop/후처리 풀 불일치");
    runePostPop_=postPop;
}
// 타입/가상표판단은등록된실제몸체에위임한다.
bool SquidPostPop::HandlesRune(Sid sid) const { return runePostPop_ && runePostPop_->Handles(sid); }
// 실제 다이스/제단 수명 전체몸체를 같은 풀에만 등록한다.
void SquidPostPop::SetDaisPostPop(const RawDaisLifecycle* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("다이스/제단 Pop 후처리 풀 불일치");
    daisPostPop_=postPop;
}
// 실제 다이스/제단 수명 타입과 가상표의 판단은 등록된 몸체를 재사용한다.
bool SquidPostPop::HandlesDais(Sid sid) const { return daisPostPop_ && daisPostPop_->Handles(sid); }
// 실제 Dude 후처리의 전체 몸체를 같은 풀에만 명시 등록한다.
void SquidPostPop::SetDudePostPop(const RawDudePostPop* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("Dude Pop/후처리 풀 불일치");
    dudePostPop_=postPop;
}
// 현재 타입과 생성자 가상 표의 판단은 등록된 실제 몸체를 재사용한다.
bool SquidPostPop::HandlesDude(Sid sid) const { return dudePostPop_ && dudePostPop_->Handles(sid); }
// 실제 Player 최초 기록/예약 몸체를 같은 풀에만 명시 등록한다.
void SquidPostPop::SetPlayerPostPop(const RawPlayerWorld* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("Player Pop/후처리 풀 불일치");
    playerPostPop_=postPop;
}
// 실제 생성자의 가상 표 판단은 등록된 Player 몸체를 재사용한다.
bool SquidPostPop::HandlesPlayer(Sid sid) const { return playerPostPop_ && playerPostPop_->Handles(sid); }
// 복원한 전용 자산 몸체를 같은 월드에 명시 등록한다. 일반 지원 주소 표는 변경하지 않는다.
void SquidPostPop::SetAssetPostPop(const RawAssetPostPop* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("자산 Pop/후처리 풀 불일치");
    assetPostPop_=postPop;
}
// 전용 body의 PE 가상 표 조회를 사용한다. 등록된 객체의 Validate가 필요한 외부 효과도 검사한다.
bool SquidPostPop::HandlesAsset(Sid sid) const { return assetPostPop_ && assetPostPop_->Handles(sid); }
// 크랩의 별도492예약을 생략하지 않도록 직접Carrier 표를 확장하지 않는다.
void SquidPostPop::SetCarrierStealPostPop(const RawCarrierSteal* postPop) { if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("크랩 PostPop/공통 풀 불일치");carrierStealPostPop_=postPop; }
// 실제 등록한 같은풀/정확한가상표만 지원한다.
bool SquidPostPop::HandlesCarrierSteal(Sid sid) const { return carrierStealPostPop_ && carrierStealPostPop_->Handles(sid); }
// 기존 직접 호출 몸체를 같은 월드의 가상 분배에 연결한다. 주소 표만으로 미연결 효과를 허용하지 않는다.
void SquidPostPop::SetCarrierPostPop(const RawCarrierPostPop* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("Carrier Pop/후처리 풀 불일치");
    carrierPostPop_=postPop;
}
// 파생 Carrier 재정의는 포함하지 않으며 PE +0x20이 실제 직접 몸체인 표만 조회한다.
bool SquidPostPop::HandlesCarrier(Sid sid) const {
    const auto values=pool_.Edition()==OriginalEdition::Patch1078 ? std::span<const std::uint32_t>(kPatchCarrierVtables) : std::span<const std::uint32_t>(kCdCarrierVtables);
    return carrierPostPop_ && std::ranges::find(values,Vtable(pool_.Slot(sid)))!=values.end();
}
// 타입 번호에 따른 추측 대신 두 PE의 직접 Damageable 가상 주소 표를 사용한다.
bool SquidPostPop::HandlesDamageable(Sid sid) const {
    const auto& values=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchDamageableVtables : kCdDamageableVtables;
    return carrierPostPop_ && std::ranges::find(values,Vtable(pool_.Slot(sid)))!=values.end();
}
// 공간이 바뀌기 전에 전체 예약 접두의 풀을 확인한다. 참조 대상은 연결보다 오래 살아야 한다.
void SquidPostPop::SetForcefieldPostPop(const RawForcefieldPostPop* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("보호막 Pop/접두 풀 불일치");
    forcefieldPostPop_=postPop;
}
// 보호막도 공통 firstPop/Pop을 공유하지만 postPop 재정의는 반드시 연결해야 한다.
bool SquidPostPop::HandlesForcefield(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    return forcefieldPostPop_ && raw[10]==kForcefieldType && Vtable(raw)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchForcefieldVtable : kCdForcefieldVtable);
}
// 완성한 후처리와 같은 풀인지 구성 시 확인한다. 전역 지원 vtable 표는 확장하지 않는다.
void SquidPostPop::SetPriestPostPop(const RawPriestPostPopTail* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("사제 Pop/후처리 풀 불일치");
    priestPostPop_=postPop;
}
// 원본 사제 postPop 재정의는 사제 자산 타입과 실제 가상 표 둘 다 필요하다.
bool SquidPostPop::HandlesPriest(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    return priestPostPop_ && raw[10]==kPriestType && Vtable(raw)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchPriestVtable : kCdPriestVtable);
}
// 순환하는 생성/Pop 연결을 인스턴스 구성 뒤에 공급한다.
void SquidPostPop::SetSurfacePrefix(std::function<void(Sid,std::uint32_t)> prefix) { surfacePrefix_=std::move(prefix); }
// 가상 표와 명시적 접두가 둘 다 있어야 noIsland 후처리를 허용한다.
bool SquidPostPop::HandlesSurface(Sid sid) const {
    const auto expected=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchNoIslandVtable : kCdNoIslandVtable;
    return surfacePrefix_ && Vtable(pool_.Slot(sid))==expected;
}
// 00422150 ↔ CD 00449890. 공통 postPop을 호출하기 전에 수행하는 부분만 복원한다.
void SquidPostPop::BridgePrefix(SidPool& pool,Sid sid,std::uint32_t flags,const std::function<void(Sid)>& connector) {
    const auto extraOffset=pool.Edition()==OriginalEdition::Patch1078 ? 40U : 35U;
    const auto extra=pool.Slot(sid)[extraOffset];
    const bool connect=(flags&1)!=0 || ((flags&4)!=0 && (extra&1)==0);
    if (connect && !connector) throw std::invalid_argument("다리 postPop 연결 효과 누락");
    if (flags&1) pool.AllocatedBytes(sid)[extraOffset]|=2;
    if (connect) connector(sid);
}
// 패치는 trunc(cost+type*23)-type*23, CD는 원본 float cost를 누적하고 절삭한다.
std::int32_t SquidPostPop::TotalCost(std::size_t type) const {
    const double cost=types_[type].cost; double value=cost;
    if (!std::isfinite(cost)) throw std::invalid_argument("postPop 유한 비용이 필요합니다");
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        const auto bias=static_cast<std::int64_t>(type)*kCostBias;
        const auto encoded=cost+static_cast<double>(bias);
        if (encoded<std::numeric_limits<std::int32_t>::min() || encoded>std::numeric_limits<std::int32_t>::max())
            throw std::out_of_range("postPop 패치 비용 표 범위 오류");
        value=Wrapped(static_cast<std::int64_t>(encoded)-bias);
    }
    value+=state_.totalCost;
    // 정상 비용 누적의 signed DWORD 경계 통과는 원본 ftol 반환의 low DWORD로 보존한다.
    if (value<-4294967296.0 || value>4294967295.0) throw std::out_of_range("postPop 비용 누적 범위 오류");
    return Wrapped(static_cast<std::int64_t>(value));
}
// 실제 원본 분기가 미복원 함수를 요구할 때만 거부한다. 표시/통계 억제면 타입 효과를 읽지 않는다.
void SquidPostPop::Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop) const {
    const auto bytes=pool_.Slot(sid);
    if (sid.value<5 || (bytes[11]&(kFree|kContained)) || bytes[10]<kFirstAssetTypeNumber || bytes[10]>=types_.size())
        throw std::logic_error("postPop raw 자산 상태 오류");
    if (!SquidPop::Supports(pool_.Edition(),Vtable(bytes),flags) && !HandlesDerived(sid)) throw std::logic_error("파생 postPop 미복원");
    if (HandlesPriest(sid)) priestPostPop_->Validate(sid);
    if (HandlesRune(sid)) runePostPop_->Validate(sid,flags,pop);
    if (HandlesDais(sid)) daisPostPop_->Validate(sid,flags);
    if (HandlesDude(sid)) dudePostPop_->Validate(sid,flags);
    if (HandlesPlayer(sid)) playerPostPop_->Validate(sid,flags,pop);
    if (HandlesBuried(sid)) buriedPostPop_->Validate(sid,flags,pop);
    if (HandlesBalloon(sid)) balloonPostPop_->Validate(sid,flags);
    if (HandlesForcefield(sid)) forcefieldPostPop_->Validate(sid);
    if (HandlesAsset(sid)) assetPostPop_->Validate(sid,flags,pop);
    if (HandlesFactory(sid)) factoryPostPop_->Validate(sid,flags,pop);
    ValidateBase(sid,flags,pop);
}
// 파생 몸체의 명시적 공통 호출용 검사다. 자동 가상 분배의 지원 검사는 Validate에 남긴다.
void SquidPostPop::ValidateBase(Sid sid,std::uint32_t flags,const RawGraphPop* pop) const {
    const auto bytes=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (sid.value<5 || (bytes[11]&(kFree|kContained)) || bytes[10]<kFirstAssetTypeNumber || bytes[10]>=types_.size())
        throw std::logic_error("postPop raw 자산 상태 오류");
    if (state_.suppressed) return;
    const auto& type=types_[bytes[10]];
    if (state_.graphsEnabled) {
        const bool region=(type.flags2&kRegionMask) && (flags&3) && !(flags&kNoGraph);
        const bool add=(type.flags1&TypeFlag1::kSurface) && (flags&kGraphChange) && !(flags&kNoGraph);
        if ((flags&kGraphChange) && !(flags&kNoGraph)) {
            if (flags&kDestroyGraph) throw std::logic_error("postPop destroyGraph 원본 assert 경로");
        }
        if ((region || add) && !graph_) throw std::logic_error("postPop 표면/영역 그래프가 연결되지 않았습니다");
        if (graph_) graph_->ValidatePostPop(sid,region,add,pop);
    }
    if (!(flags&1) || (bytes[patch ? 40 : 35]&kAbstractBuried)) return;
    const auto owner=bytes[patch ? 34 : 32];
    if (owner>kPlayerCount) throw std::out_of_range("postPop 소유자 범위 오류");
    if (!aiNotice_ && owner && state_.aiAttached[owner]) throw std::logic_error("postPop AI 생성 통지 미복원");
    if (state_.pendingPlacement) throw std::logic_error("postPop 배치 선택 해제 미복원");
    static_cast<void>(TotalCost(bytes[10]));
    if (type.flags1&kProvider) static_cast<void>(state_.providers.Items());
    if (type.flags2&kFactoryMask) {
        static_cast<void>(state_.factories.Items());
        if (owner) static_cast<void>(state_.ownerFactories[owner].Items());
    }
}
// 성공한 공간 Pop의 flags로 가상 후처리를 분배한다. 전체 사제 wrapper는 자기 공통 깊이를 감소시킨다.
void SquidPostPop::Activate(Sid sid,std::uint32_t flags) {
    Validate(sid,flags);const auto before=state_.depth; ++state_.depth;
    // 벌룬의 재귀 그림자 Pop은 내부 Damageable을 통해 각 공통 깊이를 끝낸다.
    if (HandlesBalloon(sid)) { balloonPostPop_->PostPop(sid,flags);if (state_.depth!=before) throw std::logic_error("벌룬 Activate 공통 깊이 불일치");return; }
    // 맨 오 워는 자신의 그림자 재귀 Pop과 Damageable 뒤 공통 깊이를 한 번 줄인다.
    if (HandlesFlyer(sid)) {
        flyerPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("비행 Activate 공통 깊이 불일치");return;
    }
    // Player는 현재9기록/SID와 조건부예약 뒤 자기 몸체에서 공통 장부를 한 번 호출한다.
    if (HandlesPlayer(sid)) {
        playerPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("Player Activate 공통 깊이 불일치");
        return;
    }
    // buried도 QA/예약 뒤 같은 공통 장부를 내부에서 한 번 호출한다.
    if (HandlesBuried(sid)) {
        buriedPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("buried Activate 공통 깊이 불일치");
        return;
    }
    // Dude는 idle/population/I 프레임 뒤 자기 전체 몸체에서 공통 장부를 한 번 호출한다.
    if (HandlesDude(sid)) {
        dudePostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("Dude Activate 공통 깊이 불일치");
        return;
    }
    // 다이스/제단은 자기 전체 몸체 안에서 Damageable와 공통 장부를 한 번 호출한다.
    if (HandlesDais(sid)) {
        daisPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("다이스/제단 Activate 공통 깊이 불일치");
        return;
    }
    // Rune은304예약/건설후공통몸체를자신의내부에서한번호출한다.
    if (HandlesRune(sid)) {
        runePostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("Rune Activate 공통 깊이 불일치");
        return;
    }
    if (HandlesPriest(sid)) {
        priestPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("사제 Activate 공통 깊이 불일치");
        return;
    }
    // 크랩은492예약/현재검사뒤자체내부에서공통장부를한번실행한다.
    if (HandlesCarrierSteal(sid)) {
        carrierStealPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("크랩 Activate 공통 깊이 불일치");return;
    }
    // 두 직접 몸체도 같은 flags의 공통 장부를 내부에서 한 번 호출한다.
    if (HandlesCarrier(sid) || HandlesDamageable(sid)) {
        if (HandlesCarrier(sid)) carrierPostPop_->PostPop(sid,flags);
        else carrierPostPop_->DamageablePostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("Carrier/Damageable Activate 공통 깊이 불일치");
        return;
    }
    // 자산 전용 재정의는 Damageable/base를 내부에서 실행하므로 접두처럼 다시 장부를 호출하지 않는다.
    if (HandlesAsset(sid)) {
        assetPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("자산 Activate 공통 깊이 불일치");
        return;
    }
    // Factory도 Damageable와 공통 장부를 내부에서 실행하므로 중복 호출하지 않는다.
    if (HandlesFactory(sid)) {
        factoryPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("Factory Activate 공통 깊이 불일치");
        return;
    }
    if (HandlesBridge(sid)) BridgePrefix(pool_,sid,flags,bridgeConnector_);
    // 섬 받침은 접두(종유석·연결 순회) 뒤에 같은 flags로 공통 후처리를 부른다.
    else if (HandlesIsland(sid)) islandPrefix_(sid,flags);
    // 표면 칸 접두가 만든 받침/종유석의 중첩 postPop을 마친 뒤 자기 공통 장부를 갱신한다.
    else if (HandlesSurface(sid)) surfacePrefix_(sid,flags);
    // 보호막의 두 Regular 예약을 마친 뒤 자기 공통 장부/깊이를 한 번 처리한다.
    else if (HandlesForcefield(sid)) forcefieldPostPop_->Prefix(sid,flags);
    PostPop(sid,flags);
}
// 공통 후처리의 순서: 영역 무효화→그래프 생성/리셋→비용→공급/작업장 목록→소유자 조건 통계→깊이 감소다.
void SquidPostPop::PostPop(Sid sid,std::uint32_t flags) {
    Validate(sid,flags);
    PostPopBase(sid,flags);
}
// 직접 base 호출도 공통 상태·효과의 검사와 깊이 감소를 그대로 수행한다.
void SquidPostPop::PostPopBase(Sid sid,std::uint32_t flags) {
    ValidateBase(sid,flags);
    if (state_.suppressed) { --state_.depth; return; }
    const auto bytes=pool_.Slot(sid); const auto number=bytes[10]; const auto& type=types_[number];
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (state_.graphsEnabled && graph_)
        graph_->PostPop(sid,(type.flags2&kRegionMask) && (flags&3) && !(flags&kNoGraph),
            (type.flags1&TypeFlag1::kSurface) && (flags&kGraphChange) && !(flags&kNoGraph));
    if (state_.graphsEnabled && (type.flags1&TypeFlag1::kSurface) && (flags&1) && (flags&kNoGraph))
        pool_.AllocatedBytes(sid)[patch ? 30 : 28]=state_.invalidGraph;
    if ((flags&1) && !(bytes[patch ? 40 : 35]&kAbstractBuried)) {
        state_.totalCost=TotalCost(number);
        if (type.flags1&kProvider) {
            const auto items=state_.providers.Items();
            if (std::find(items.begin(),items.end(),sid.value)==items.end()) Append(state_.providers,sid);
            ++state_.productionDirty;
        }
        const auto owner=bytes[patch ? 34 : 32];
        if (type.flags2&kFactoryMask) {
            if (owner) Append(state_.ownerFactories[owner],sid);
            Append(state_.factories,sid); ++state_.productionDirty;
        }
        if (aiNotice_) aiNotice_(pool_.Slot(sid)[34],sid,true); // 원본의 생산 목록 뒤·통계 앞에 현재 owner로004166e0을 호출한다.
        const auto current=pool_.Slot(sid);const auto currentType=current[10];if (currentType>=types_.size()) throw std::out_of_range("AI 생성 통지 뒤 현재 TYPE 범위");
        // 통지의 실제 하위가 바꾼 현재 owner/type을 다시 읽는다. 소유자0도 로컬 번호0과 비교한다.
        if (current[patch ? 34 : 32]==state_.localOwner) { ++state_.localCounts[currentType];++state_.localSecondaryCounts[currentType]; }
        ++state_.globalCounts[currentType];
    }
    --state_.depth;
}
// 서로 다른 SID 풀의 연결을 막는 읽기 전용 소유자다.
const SidPool& SquidPostPop::Pool() const { return pool_; }
// 실제 PATCH 통지와 해제만 허용한다. CD의 기존 가상 표/값형 입력은 별도로 유지한다.
void SquidPostPop::BindPatchAiNotice(std::function<void(std::uint32_t,Sid,bool)> notice) { if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("PATCH AI 생성 통지 판본 불일치");aiNotice_=std::move(notice); }
// 동일 SID라도 별도 해시/spot을 읽는 구성을 막는다.
void SquidPostPop::ValidateSpace(const SquidHash& hash,std::span<const std::uint8_t> spots) const {
    if (graph_) graph_->ValidateSpace(hash,spots);
}
}
