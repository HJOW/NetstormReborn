// 원본 주소를 호스트 포인터로 해석하지 않고 raw 슬롯의 판본별 필드만 읽는다.
#include "o/RawGraph.h"
#include "o/Squid.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 두 판본에서 같은 상태 비트다. contained 표면과 소수 좌표는 이번 정수 경로 밖이다.
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kContained=8,kBuried=8;
// 부호 있는 short SID를 사용하는 원본 표면 탐색의 한계다.
constexpr std::uint32_t kMaximumId=32767;
// raw little endian 필드를 정렬과 무관하게 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 필드 폭 안의 바이트만 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// 정수 스냅샷 밖 좌표를 반올림하여 원본 계산처럼 취급하지 않는다.
int Coordinate(float value) {
    if (!std::isfinite(value) || value<0 || value>=kWorldCells || std::trunc(value)!=value)
        throw std::out_of_range("raw 그래프는 지도 안 정수 좌표가 필요합니다");
    return static_cast<int>(value);
}
}
// 정상 표/스택을 소유하고 호출자가 준 타입/프레임 표의 수명을 분리한다.
RawGraph::RawGraph(SidPool& pool,SquidHash& hash,std::span<const std::uint8_t> spots,
    std::span<const RiftTypeRecord> types,std::span<const std::vector<FrameCode>> frames,
    std::span<const GraphRecord> records,std::span<const std::uint32_t> stack)
    :pool_(pool),hash_(hash),spots_(spots),types_(types.begin(),types.end()),frames_(frames.begin(),frames.end()) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || frames.size()!=count || spots.size()!=kWorldCells*kWorldCells)
        throw std::invalid_argument("raw 그래프 타입/프레임/지도 크기 오류");
    if (!records.empty() && records.size()!=records_.size()) throw std::invalid_argument("raw 그래프 표 크기 오류");
    if (!stack.empty() && stack.size()!=stack_.size()) throw std::invalid_argument("raw 그래프 스택 크기 오류");
    if (records.empty()) records_[Graph::kInvalid]={0x7dfd,0x7dfd,0};
    else std::copy(records.begin(),records.end(),records_.begin());
    if (!stack.empty()) std::copy(stack.begin(),stack.end(),stack_.begin());
}
// Pop과 postPop은 같은 원본 풀·공간 배열을 사용해야 한다.
const SidPool& RawGraph::Pool() const { return pool_; }
// 동일 크기의 별도 배열도 실제 공간 연결이 아니므로 거부한다.
void RawGraph::ValidateSpace(const SquidHash& hash,std::span<const std::uint8_t> spots) const {
    if (&hash!=&hash_ || spots.data()!=spots_.data() || spots.size()!=spots_.size())
        throw std::invalid_argument("Pop/raw 그래프 공간이 다릅니다");
}
// 그래프가 사용하는 필드만 비교한다. 비용·생산 목록은 postPop의 책임이다.
void RawGraph::ValidateTypes(std::span<const RiftTypeRecord> types) const {
    if (types.size()!=types_.size()) throw std::invalid_argument("postPop/raw 그래프 타입 크기가 다릅니다");
    // 프레임 연결과 표면 발자국의 의미가 모든 타입에서 같아야 한다.
    for (std::size_t i=0;i<types.size();++i)
        if (types[i].flags1!=types_[i].flags1 || types[i].flags2!=types_[i].flags2 ||
            types[i].footX!=types_[i].footX || types[i].footY!=types_[i].footY)
            throw std::invalid_argument("postPop/raw 그래프 타입이 다릅니다");
}
// 전체 계산을 복사본에서 수행하며 미지원 입력·소진·연결 오류를 쓰기 전에 보고한다.
RawGraph::Plan RawGraph::Calculate(Sid sid,bool add,std::uint8_t target,const RawGraphPop* pop) const {
    const auto root=pool_.Slot(sid); const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto graphOffset=patch ? 30U : 28U,extraOffset=patch ? 40U : 35U,frameOffset=patch ? 36U : 34U;
    if (sid.value<5 || sid.value>kMaximumId || (root[11]&(kFree|kContained)) ||
        root[10]<kFirstAssetTypeNumber || root[10]>=types_.size() || !(types_[root[10]].flags1&TypeFlag1::kSurface))
        throw std::logic_error("raw 그래프 표면 SID 오류");
    Plan plan{records_,stack_,{},0};
    if (pop) {
        const auto& type=types_[root[10]];
        if (!add || !pop->type || pop->level<0 || pop->level>3 || type.flags1!=pop->type->flags1 ||
            type.flags2!=pop->type->flags2 || type.footX!=pop->type->footX || type.footY!=pop->type->footY)
            throw std::invalid_argument("Pop/raw 그래프 타입/단계 오류");
    }
    // Add의 비활성 입력은 원본처럼 그래프 표/스택을 읽지 않고 돌아간다.
    const auto state=static_cast<std::uint8_t>(pop ? root[11]&~kVoid : root[11]);
    if (add && (state&7)) return plan;
    std::vector<std::uint16_t> map(hash_.Entries(0).begin(),hash_.Entries(0).end());
    std::vector<std::uint8_t> spots(spots_.begin(),spots_.end());
    if (pop) {
        const int x=Coordinate(pop->x),y=Coordinate(pop->y); const auto& type=types_[root[10]];
        if (pop->level==0) map[static_cast<std::size_t>(y*kWorldCells+x)]=sid.value;
        if (!(root[extraOffset]&kBuried)) {
            if (type.footX<1 || type.footY<1 || x-type.footX+1<1 || y-type.footY+1<1)
                throw std::out_of_range("raw 그래프 Pop 발자국 오류");
            // 실제 Pop의 y/x 순서와 genus low byte OR를 예측한다. overlap은 Pop이 처리한다.
            for (int cy=y-type.footY+1;cy<=y;++cy)
                // 각 발자국 행을 최종 spot으로 만든다.
                for (int cx=x-type.footX+1;cx<=x;++cx) {
                    auto& spot=spots[static_cast<std::size_t>(cy*kWorldCells+cx)];
                    spot=static_cast<std::uint8_t>(spot|Squid::EffectiveGenus(type.flags2,pop->x,pop->y,type.footX,type.footY,cx,cy));
                }
        }
    }
    std::vector<bool> required(pool_.Capacity()); required[sid.value]=true;
    // 0단계 머리만 읽는다. 전체 발자국을 가짜 객체 번호로 채우거나 다른 해시 단계를 섞지 않는다.
    for (auto id:map) if (id) {
        if (id<5 || id>kMaximumId || id>=pool_.Capacity()) throw std::out_of_range("raw 그래프 해시 SID 오류");
        required[id]=true;
    }
    std::vector<SurfaceObject> objects; std::vector<GraphMembership> members;
    // 현재 살아 있는 표면과 지도에 실제 등장하는 비표면 후보를 읽어 원본 필터에 전달한다.
    for (std::uint32_t id=5;id<pool_.Capacity();++id) {
        const Sid current{static_cast<std::uint16_t>(id)}; const auto bytes=pool_.Slot(current);
        const auto currentState=static_cast<std::uint8_t>(pop && current==sid ? state : bytes[11]);
        if (currentState&(kFree|kVoid)) {
            if (required[id] && current!=sid) throw std::logic_error("raw 그래프 해시의 free/void SID");
            if (current!=sid) continue;
        }
        if (bytes[10]<kFirstAssetTypeNumber || bytes[10]>=types_.size()) {
            if (required[id]) throw std::logic_error("raw 그래프 후보 타입 오류");
            continue;
        }
        const auto& type=types_[bytes[10]]; const bool surface=(type.flags1&TypeFlag1::kSurface)!=0;
        if (!surface && !required[id]) continue;
        if (id>kMaximumId || (currentState&kContained)) throw std::out_of_range("raw 그래프 후보 상태/SID 범위 오류");
        const float x=pop && current==sid ? pop->x : std::bit_cast<float>(Read(bytes,14));
        const float y=pop && current==sid ? pop->y : std::bit_cast<float>(Read(bytes,18));
        FrameCode code{};
        if (surface) {
            const auto frame=Read(bytes,frameOffset,patch ? 4U : 1U);
            if (frame>=frames_[bytes[10]].size()) throw std::out_of_range("raw 그래프 프레임 번호 오류");
            code=frames_[bytes[10]][frame];
            members.push_back({current.value,bytes[graphOffset],currentState});
        }
        objects.push_back({current.value,Coordinate(x),Coordinate(y),type.footX,type.footY,
            type.flags1,type.flags2,code,(currentState&kDead)!=0,(bytes[extraOffset]&kBuried)!=0});
    }
    SurfaceFinder finder(objects,map,spots); Graph graph(finder,members,records_,stack_);
    if (add) graph.Add(sid.value); else plan.changed=graph.Flood(sid.value,target);
    std::copy(graph.Records().begin(),graph.Records().end(),plan.records.begin());
    std::copy(graph.FloodStack().begin(),graph.FloodStack().end(),plan.stack.begin());
    // 変更対象の全スロットを先に検査し、確保済み graph byte のみを結果に含める。
    for (auto member:members) if (graph.Number(member.id)!=member.graph) {
        static_cast<void>(pool_.Slot(Sid{member.id})); plan.numbers.emplace_back(Sid{member.id},graph.Number(member.id));
    }
    return plan;
}
// 副作用なしの試算で Pop 前の保護にも使用する。
void RawGraph::ValidateAdd(Sid sid,const RawGraphPop* pop) const { static_cast<void>(Calculate(sid,true,0,pop)); }
// graph byte 以外の payload、座標、owner、state は書き換えない。
void RawGraph::Commit(const Plan& plan) {
    const auto offset=pool_.Edition()==OriginalEdition::Patch1078 ? 30U : 28U;
    // 成功した計算の変更分だけを raw プールへ戻す。
    for (const auto& [sid,number]:plan.numbers) pool_.AllocatedBytes(sid)[offset]=number;
    records_=plan.records; stack_=plan.stack;
}
// 現在のフレーム・状態・地図から毎回再計算する。
void RawGraph::Add(Sid sid) { Commit(Calculate(sid,true,0,nullptr)); }
// 既存スタックの未使用 DWORD を保ったまま flood の結果をコミットする。
std::uint32_t RawGraph::Flood(Sid sid,std::uint8_t graph) {
    const auto plan=Calculate(sid,false,graph,nullptr); Commit(plan); return plan.changed;
}
// sentinel と reserved WORD も含む全表を提供する。
std::span<const GraphRecord> RawGraph::Records() const { return records_; }
// 使用後の痕跡も次の演算に引き継ぐ。
std::span<const std::uint32_t> RawGraph::FloodStack() const { return stack_; }
}
