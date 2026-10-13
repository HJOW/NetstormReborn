// 실제 목록 압축/통계 감소와 판본별 postDestroy 비용 차감을 공통 삭제 수명에 연결한다.
#include "o/SquidDestroyLifecycle.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 raw 공통 필드와 분류/삭제 flags다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18;
constexpr std::uint32_t kFactoryMask=0x4200,kProvider=0x10000000,kPlacement=0x30000;
constexpr std::uint32_t kNoRefund=0x200000,kNoGraph=0x2000000,kDestroyGraph=0x800,kSilentGenus=0x200000;
// 원본 타입 비용 표에 더하는 인코딩 계수다.
constexpr std::int64_t kCostBias=23;
// 정렬되지 않은 float의 비트는 원본 바이트 순서대로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 호스트 엔디언/정렬을 결과에 섞지 않는다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
// 원본 Array::remove는 모든 중복을 순서대로 압축하고 inactive 꼬리 메모리는 지우지 않는다.
void Remove(SquidPostPopList& list,Sid sid) {
    static_cast<void>(list.Items());std::uint32_t removed=0;
    // 생존 항목은 최초 상대 순서를 유지한다. 현재 count 밖 값은 읽지도 쓰지도 않는다.
    for (std::uint32_t i=0;i<list.count;++i) {
        const auto value=list.entries[i];
        if (value==sid.value) ++removed;
        else list.entries[i-removed]=value;
    }
    list.count-=removed;
}
}
// 판본별 타입 수/풀 연결을 확인하고 타입을 복사해 장부 처리 동안의 입력을 고정한다.
SquidDestroyLifecycle::SquidDestroyLifecycle(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& bookkeeping,
    SquidDeletionState& state,RawSquidDestroy& destroy,SquidDeletionHooks hooks,RawGraph* graph,SquidReward* reward)
    :pool_(pool),types_(types.begin(),types.end()),bookkeeping_(bookkeeping),state_(state),destroy_(destroy),hooks_(std::move(hooks)),graph_(graph),reward_(reward) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count) throw std::invalid_argument("삭제 훅 타입 판본/크기 오류");
    if (&destroy.Pool()!=&pool) throw std::invalid_argument("삭제 훅 SID 풀이 다릅니다");
    if (graph_) {
        if (&graph_->Pool()!=&pool_) throw std::invalid_argument("삭제 훅/Graph SID 풀이 다릅니다");
        graph_->ValidateTypes(types);destroy_.ValidateGraph(*graph_);
    }
    // 보상은 같은 풀과 같은 장부(로컬 소유자/AI 부착)를 읽어야 지급 대상이 어긋나지 않는다.
    if (reward_ && (&reward_->Pool()!=&pool_ || &reward_->Bookkeeping()!=&bookkeeping_)) throw std::invalid_argument("삭제 훅/보상 풀 또는 장부가 다릅니다");
}
// 루트 자산에만 공통 장부 처리를 제공한다. form/contained의 파생 메서드는 별도다.
std::span<const std::uint8_t> SquidDestroyLifecycle::Object(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    if (sid.value<5 || sid.value==pool_.Layout().predictableFirst || (raw[kState]&9) || raw[kType]<70 || raw[kType]>=types_.size())
        throw std::logic_error("삭제 훅 raw 자산 상태 오류");
    return raw;
}
// 앞선 외부 콜백이 raw 타입을 바꿨을 때 후속 타입 조회를 캐시하지 않는다.
const RiftTypeRecord& SquidDestroyLifecycle::Type(Sid sid) const { return types_[Object(sid)[kType]]; }
// 두 판본의 owner 바이트 위치를 구별한다.
std::uint8_t SquidDestroyLifecycle::Owner(Sid sid) const { return Object(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]; }
// abstract 또는 buried이면 글로벌 장부/비용 경로를 생략한다. owner 목록 제거와 구별한다.
bool SquidDestroyLifecycle::Ordinary(Sid sid) const { return !(Object(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&9); }
// 원본 unitLost 조건이며 패치 silentType의 소리 제외는 좌표 기록과 별도로 적용한다.
bool SquidDestroyLifecycle::LostNotice(Sid sid,std::uint32_t flags) const {
    return Type(sid).group!=10 && !(flags&(kNoRefund|kDestroyGraph)) && Owner(sid)==bookkeeping_.localOwner &&
        !(Type(sid).flags2&kSilentGenus) && !state_.unitLostSuppressed1 && !state_.unitLostSuppressed2;
}
// postDestroy의 x87 뺄셈/0방향 절삭을 low DWORD로 보존한다.
std::int32_t SquidDestroyLifecycle::RemainingCost(Sid sid) const {
    const auto number=Object(sid)[kType];const double cost=types_[number].cost;double decoded=cost;
    if (!std::isfinite(cost)) throw std::invalid_argument("삭제 훅 유한 비용이 필요합니다");
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        const auto bias=static_cast<std::int64_t>(number)*kCostBias;const double encoded=cost+static_cast<double>(bias);
        if (encoded<std::numeric_limits<std::int32_t>::min() || encoded>std::numeric_limits<std::int32_t>::max())
            throw std::out_of_range("삭제 훅 패치 비용 표 범위 오류");
        decoded=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(encoded)-bias));
    }
    const double result=static_cast<double>(bookkeeping_.totalCost)-decoded;
    if (result<-4294967296.0 || result>4294967295.0) throw std::out_of_range("삭제 훅 비용 누적 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(result)));
}
// 선택/Graph는 통계 억제보다 먼저 실행되므로 각각의 보호도 그 분기대로 확인한다.
void SquidDestroyLifecycle::ValidatePre(Sid sid,std::uint32_t flags) const {
    const auto raw=Object(sid);const auto& type=Type(sid);
    if (state_.selected==sid && !hooks_.emit) throw std::invalid_argument("삭제 훅 선택 해제 콜백 누락");
    PreGraph(sid,flags,true);
    if (bookkeeping_.suppressed) return;
    const auto owner=Owner(sid);
    if (owner>kPlayerCount) throw std::out_of_range("삭제 훅 소유자 범위 오류");
    if (type.flags2&kFactoryMask) { if (owner) static_cast<void>(bookkeeping_.ownerFactories[owner].Items()); }
    if (!Ordinary(sid)) return;
    if (owner && bookkeeping_.aiAttached[owner]) throw std::logic_error("삭제 훅 AI 통지 미복원");
    // 보상 객체가 있으면 계산 입력 오류를 쓰기 전에 거부하고, 없으면 외부 보상 콜백이 필요하다.
    if (!(flags&kNoRefund)) { if (reward_) static_cast<void>(reward_->Plan(sid,(flags>>16)&15,false)); else if (!hooks_.emit) throw std::invalid_argument("삭제 훅 보상 콜백 누락"); }
    if (LostNotice(sid,flags) && (pool_.Edition()==OriginalEdition::Cd1072 || raw[kType]!=state_.silentType) && !hooks_.emit)
        throw std::invalid_argument("삭제 훅 소리 콜백 누락");
    if (type.flags1&kProvider) static_cast<void>(bookkeeping_.providers.Items());
    if (type.flags2&kFactoryMask) static_cast<void>(bookkeeping_.factories.Items());
}
// post의 비용 효과는 suppressed와 무관하며 Graph 분기를 앞서 거부한다.
void SquidDestroyLifecycle::ValidatePost(Sid sid,std::uint32_t flags) const {
    static_cast<void>(Object(sid));
    if (NeedsPostGraph(sid,flags)) {
        if (!graph_) throw std::logic_error("공통 postDestroy Graph 효과 미연결");
        graph_->ValidatePostDestroy(sid);
    }
    if (Ordinary(sid)) static_cast<void>(RemainingCost(sid));
}
// 선택 복구 효과 뒤 논리 선택을 지우고 실제 UI 변경은 호출자에게 전달한다.
void SquidDestroyLifecycle::ClearSelection(Sid sid) {
    hooks_.emit({SquidDeletionEffect::ClearSelection,sid,0});state_.selected={};
}
// 원본은 0x800 Free를 noGraph보다 먼저 판단하며 buried만 Graph 효과를 생략한다.
void SquidDestroyLifecycle::PreGraph(Sid sid,std::uint32_t flags,bool validate) const {
    const auto raw=Object(sid);
    if (!bookkeeping_.graphsEnabled || (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&8) ||
        !(Type(sid).flags1&TypeFlag1::kSurface) || (!(flags&kDestroyGraph) && (flags&kNoGraph))) return;
    if (!graph_) throw std::logic_error("공통 preDestroy Graph 효과 미연결");
    if (flags&kDestroyGraph) {
        const auto number=raw[pool_.Edition()==OriginalEdition::Patch1078 ? 30 : 28];
        if (validate) graph_->ValidateFree(number);else graph_->Free(number);return;
    }
    std::optional<std::uint8_t> sourceType;
    if (raw[kType]==state_.specialSurfaceType) {
        if (graph_->Frame(sid).side!='H') return;
        sourceType=state_.specialSurfaceReplacementType;
    }
    const auto removed=static_cast<std::uint8_t>(sourceType.value_or(raw[kType])==state_.largeSurfaceType ? 9 : 1);
    if (validate) graph_->ValidateDetach(sid,state_.rebuildGraph,removed,sourceType);
    else graph_->Detach(sid,state_.rebuildGraph,removed,sourceType);
}
// noGraph는 pre 분할만 억제한다. post 주변 Add를 임의로 억제하지 않는다.
bool SquidDestroyLifecycle::NeedsPostGraph(Sid sid,std::uint32_t flags) const {
    return bookkeeping_.graphsEnabled && !(Object(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&8) &&
        !(flags&kDestroyGraph) && (Type(sid).flags2&(state_.regionMask|8));
}
// 원본 순서: 선택→owner 작업장→보상→현재 타입 수→소리/좌표→provider→AI(null)→global 작업장→깊이 감소.
void SquidDestroyLifecycle::PreDestroy(Sid sid,std::uint32_t flags) {
    ValidatePre(sid,flags);
    if (state_.selected==sid) {
        if (!Ordinary(sid) && (Type(sid).flags2&kPlacement)) {
            const auto raw=Object(sid);state_.placementX=Coordinate(raw,kX);state_.placementY=Coordinate(raw,kY);
            state_.placementType=raw[kType];bookkeeping_.pendingPlacement=true;
        }
        ClearSelection(sid);
    }
    PreGraph(sid,flags,false);
    if (!bookkeeping_.suppressed) {
        if (Type(sid).flags2&kFactoryMask) { const auto owner=Owner(sid);if (owner) Remove(bookkeeping_.ownerFactories.at(owner),sid); }
        if (Ordinary(sid)) {
            if (!(flags&kNoRefund)) {
                // 원본 인자: sid, 플래그 16..19비트의 수신자, 마지막 0(삭제 보상 비율).
                if (reward_) reward_->Refund(sid,(flags>>16)&15,false);
                else hooks_.emit({SquidDeletionEffect::Refund,sid,(flags>>16)&15});
            }
            const auto number=Object(sid)[kType];
            if (Owner(sid)==bookkeeping_.localOwner) --bookkeeping_.localCounts[number];
            --bookkeeping_.globalCounts[number]; // localSecondaryCounts는 누적 생산 수이므로 유지한다.
            if (LostNotice(sid,flags)) {
                if (pool_.Edition()==OriginalEdition::Cd1072 || Object(sid)[kType]!=state_.silentType)
                    hooks_.emit({SquidDeletionEffect::UnitLostSound,sid,0});
                const auto raw=Object(sid);state_.lostX=Coordinate(raw,kX);state_.lostY=Coordinate(raw,kY);
            }
            if (Type(sid).flags1&kProvider) { Remove(bookkeeping_.providers,sid);++bookkeeping_.productionDirty; }
            // AI 포인터가 있는 경우는 ValidatePre에서 거부했다. null 통지는 원본처럼 효과가 없다.
            if (Type(sid).flags2&kFactoryMask) { Remove(bookkeeping_.factories,sid);++bookkeeping_.productionDirty; }
        }
    }
    destroy_.CompletePreDestroy();
}
// 주변 표면을 원본 순서로 다시 Add하고 ordinary 비용을 마지막에 차감한다.
void SquidDestroyLifecycle::PostDestroy(Sid sid,std::uint32_t flags) {
    ValidatePost(sid,flags);if (NeedsPostGraph(sid,flags)) graph_->PostDestroy(sid);
    if (Ordinary(sid)) bookkeeping_.totalCost=RemainingCost(sid);
    destroy_.CompletePostDestroy();
}
// 원본 파생 가상 메서드는 forward로, 공통 Pre/Post는 위 구현으로 연결한다.
SquidDestroyHooks SquidDestroyLifecycle::Hooks() {
    return {
        // 미연결 외부 효과는 해당 경계에서 중단한다. 임의의 dead 대체를 하지 않는다.
        [this](const SquidDestroyEvent& event) {
            switch (event.effect) {
            case SquidDestroyEffect::PreDestroy:PreDestroy(event.sid,event.flags);break;
            case SquidDestroyEffect::PostDestroy:PostDestroy(event.sid,event.flags);break;
            case SquidDestroyEffect::ClearSelection:ClearSelection(event.sid);break;
            default:
                if (!hooks_.forward) throw std::logic_error("삭제 훅 종속/전파 콜백 미연결");
                hooks_.forward(event);break;
            }
        },[this] { return state_.selected; }
    };
}
}
