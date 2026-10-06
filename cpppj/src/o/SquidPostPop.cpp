// 실제 공통 후처리에서 선택한 효과만 구현하며 미복원 호출을 변경 전에 거부한다.
#include "o/SquidPostPop.h"
#include "o/SquidPop.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// raw 상태/extra 필드와 후처리 분기의 원본 마스크다.
constexpr std::uint8_t kFree=1,kContained=8,kAbstractBuried=9;
constexpr std::uint32_t kRegionMask=0x50444208,kFactoryMask=0x4200,kProvider=0x10000000;
constexpr std::uint32_t kNoGraph=0x2000000,kGraphChange=0x203,kDestroyGraph=0x800;
// 패치 비용 표가 타입 번호마다 더해 저장하는 정수다.
constexpr std::int64_t kCostBias=23;
// raw 가상 주소를 호스트 포인터로 해석하지 않고 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> bytes) {
    std::uint32_t value=0;
    // little endian DWORD를 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
// 원본은 가득 찬 목록에서 추가를 생략하며 기존 항목/메모리는 보존한다.
void Append(SquidPostPopList& list,Sid sid) {
    if (list.count<list.entries.size()) list.entries[list.count++]=sid.value;
}
// DWORD 집계를 signed 해석으로 다시 읽는다. 부호 있는 덧셈 넘침은 만들지 않는다.
std::int32_t Wrapped(std::int64_t value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)); }
}
// 원본 배열의 count 밖 stale 메모리는 유지하며 조회에서는 노출하지 않는다.
std::span<const std::uint32_t> SquidPostPopList::Items() const {
    if (count>entries.size()) throw std::logic_error("postPop 목록 count 범위 오류");
    return std::span(entries).first(count);
}
// 슬롯 byte type가 가리키는 타입 표를 검증한다.
SquidPostPop::SquidPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& state,RawGraph* graph)
    :pool_(pool),types_(types.begin(),types.end()),state_(state),graph_(graph) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count) throw std::invalid_argument("postPop 타입 판본/크기 오류");
    if (graph_ && &graph_->Pool()!=&pool_) throw std::invalid_argument("postPop/Graph SID 풀이 다릅니다");
    if (graph_) graph_->ValidateTypes(types);
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
    const auto bytes=pool_.Slot(sid); const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (sid.value<5 || (bytes[11]&(kFree|kContained)) || bytes[10]<kFirstAssetTypeNumber || bytes[10]>=types_.size())
        throw std::logic_error("postPop raw 자산 상태 오류");
    if (!SquidPop::Supports(pool_.Edition(),Vtable(bytes),flags)) throw std::logic_error("파생 postPop 미복원");
    if (state_.suppressed) return;
    const auto& type=types_[bytes[10]];
    if (state_.graphsEnabled) {
        if ((type.flags2&kRegionMask) && (flags&3) && !(flags&kNoGraph))
            throw std::logic_error("postPop 이웃 영역/그래프 갱신 미복원");
        if ((flags&kGraphChange) && !(flags&kNoGraph)) {
            if (flags&kDestroyGraph) throw std::logic_error("postPop destroyGraph 원본 assert 경로");
            if (type.flags1&TypeFlag1::kSurface) {
                if (!graph_) throw std::logic_error("postPop 표면 그래프가 연결되지 않았습니다");
                graph_->ValidateAdd(sid,pop);
            }
        }
    }
    if (!(flags&1) || (bytes[patch ? 40 : 35]&kAbstractBuried)) return;
    const auto owner=bytes[patch ? 34 : 32];
    if (owner>kPlayerCount) throw std::out_of_range("postPop 소유자 범위 오류");
    if (owner && state_.aiAttached[owner]) throw std::logic_error("postPop AI 생성 통지 미복원");
    if (state_.pendingPlacement) throw std::logic_error("postPop 배치 선택 해제 미복원");
    static_cast<void>(TotalCost(bytes[10]));
    if (type.flags1&kProvider) static_cast<void>(state_.providers.Items());
    if (type.flags2&kFactoryMask) {
        static_cast<void>(state_.factories.Items());
        if (owner) static_cast<void>(state_.ownerFactories[owner].Items());
    }
}
// 성공한 공간 Pop이 비전투 Activate에 전달한 flags를 그대로 사용한다.
void SquidPostPop::Activate(Sid sid,std::uint32_t flags) {
    Validate(sid,flags); ++state_.depth; PostPop(sid,flags);
}
// 공통 후처리의 순서: 그래프 생성/리셋→비용→공급/작업장 목록→소유자 조건 통계→깊이 감소다.
void SquidPostPop::PostPop(Sid sid,std::uint32_t flags) {
    Validate(sid,flags);
    if (state_.suppressed) { --state_.depth; return; }
    const auto bytes=pool_.Slot(sid); const auto number=bytes[10]; const auto& type=types_[number];
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (state_.graphsEnabled && (type.flags1&TypeFlag1::kSurface) && (flags&kGraphChange) && !(flags&kNoGraph))
        graph_->Add(sid);
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
        // AI는 Validate에서 없는 연결만 허용했다. 소유자 0도 로컬 번호 0과 비교한다.
        if (owner==state_.localOwner) { ++state_.localCounts[number]; ++state_.localSecondaryCounts[number]; }
        ++state_.globalCounts[number];
    }
    --state_.depth;
}
// 서로 다른 SID 풀의 연결을 막는 읽기 전용 소유자다.
const SidPool& SquidPostPop::Pool() const { return pool_; }
// 동일 SID라도 별도 해시/spot을 읽는 구성을 막는다.
void SquidPostPop::ValidateSpace(const SquidHash& hash,std::span<const std::uint8_t> spots) const {
    if (graph_) graph_->ValidateSpace(hash,spots);
}
}
