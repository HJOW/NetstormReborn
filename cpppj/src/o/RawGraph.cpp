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
// 원본 일반 SquidFinder는 단계 0..3의 y/x 버킷과 next 체인을 순회하고 매몰 후보만 건너뛴다.
void RawGraph::Region(Sid sid,const RawGraphPop* pop,std::span<const std::uint8_t> spots,Plan& plan) const {
    const auto root=pool_.Slot(sid); const auto& source=types_[root[10]];
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto extraOffset=patch ? 40U : 35U,graphOffset=patch ? 30U : 28U;
    const int x=Coordinate(pop ? pop->x : std::bit_cast<float>(Read(root,14)));
    const int y=Coordinate(pop ? pop->y : std::bit_cast<float>(Read(root,18)));
    if (source.footX<1 || source.footY<1 || source.footX>kWorldCells || source.footY>kWorldCells)
        throw std::out_of_range("그래프 영역 발자국 오류");
    const int left=std::max(0,x-source.footX+1),top=std::max(0,y-source.footY+1);
    const int right=x,bottom=y;
    // 단계별 끝 버킷을 한 칸 늘리는 원본 탐색을 보존한다. 이는 표면 이웃 탐색과 다른 경로다.
    for (int level=0;level<4;++level) {
        const int scale=SquidHash::kScales[static_cast<std::size_t>(level)],side=SquidHash::Side(level);
        const int startX=left/scale,startY=top/scale,endX=std::min(side-1,right/scale+1),endY=std::min(side-1,bottom/scale+1);
        const auto heads=hash_.Entries(level);
        // 일반 탐색의 y 바깥/x 안쪽 순서다.
        for (int cy=startY;cy<=endY;++cy)
            // 최대 발자국이 더 큰 객체의 기준점도 끝 버킷에서 잡는다.
            for (int cx=startX;cx<=endX;++cx) {
                const auto index=static_cast<std::size_t>(cy*side+cx);
                const bool projected=pop && pop->level==level && index==SquidHash::BucketIndex(level,pop->x,pop->y);
                auto current=projected ? sid.value : heads[index]; std::vector<std::uint16_t> chain;
                // 각 버킷의 실제 머리/next 순서로 조회하며 손상된 순환은 쓰기 전에 거부한다.
                while (current) {
                    if (current<5 || current>kMaximumId || current>=pool_.Capacity() ||
                        std::find(chain.begin(),chain.end(),current)!=chain.end())
                        throw std::logic_error("그래프 영역 해시 범위/순환 오류");
                    chain.push_back(current); const Sid candidate{current}; const auto bytes=pool_.Slot(candidate);
                    const auto next=pop && candidate==sid ? hash_.Entries(pop->level)[SquidHash::BucketIndex(pop->level,pop->x,pop->y)] : Read(bytes,4,2);
                    current=static_cast<std::uint16_t>(next);
                    if (bytes[11]&(kFree|kContained)) throw std::logic_error("그래프 영역 후보 상태 오류");
                    if (bytes[extraOffset]&kBuried) continue;
                    if (bytes[10]<kFirstAssetTypeNumber || bytes[10]>=types_.size()) throw std::logic_error("그래프 영역 후보 타입 오류");
                    const int ax=Coordinate(pop && candidate==sid ? pop->x : std::bit_cast<float>(Read(bytes,14)));
                    const int ay=Coordinate(pop && candidate==sid ? pop->y : std::bit_cast<float>(Read(bytes,18)));
                    // 원본 패치 일반 탐색은 유효 좌표 밖을 건너뛰고 CD는 assert한다. 이 경로는 정상 입력만 받는다.
                    if (ax<=0 || ay<=0) throw std::out_of_range("그래프 영역 후보의 유효 좌표가 필요합니다");
                    const auto& type=types_[bytes[10]];
                    if (type.footX<1 || type.footY<1 || type.footX>kWorldCells || type.footY>kWorldCells)
                        throw std::out_of_range("그래프 영역 후보 발자국 오류");
                    const int aLeft=ax-type.footX+1,aTop=ay-type.footY+1;
                    if (ax<left || aLeft>right || ay<top || aTop>bottom || !(type.flags1&TypeFlag1::kSurface) ||
                        !(spots[static_cast<std::size_t>(ay*kWorldCells+ax)]&8)) continue;
                    const auto staged=std::find_if(plan.numbers.begin(),plan.numbers.end(),[&](const auto& item){return item.first==candidate;});
                    const auto graph=staged==plan.numbers.end() ? bytes[graphOffset] : staged->second;
                    if (graph!=Graph::kInvalid) {
                        if (graph>=Graph::kCount) throw std::out_of_range("그래프 영역 후보 번호 오류");
                        auto& record=plan.records[graph];
                        // Remove의 활성/양수 조건을 유지하며 0이 되면 사용 여부도 지운다. reserved는 보존한다.
                        if (record.inUse && record.surfaces>0) { --record.surfaces; if (!record.surfaces) record.inUse=0; }
                        if (staged==plan.numbers.end()) plan.numbers.emplace_back(candidate,Graph::kInvalid);
                        else staged->second=Graph::kInvalid;
                    }
                }
            }
    }
}
// 전체 계산을 복사본에서 수행하며 미지원 입력·소진·연결 오류를 쓰기 전에 보고한다.
RawGraph::Plan RawGraph::Calculate(Sid sid,Operation operation,std::uint8_t target,const RawGraphPop* pop) const {
    const bool region=operation==Operation::Region || operation==Operation::RegionAdd;
    const bool add=operation==Operation::Add || operation==Operation::RegionAdd;
    const auto root=pool_.Slot(sid); const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto graphOffset=patch ? 30U : 28U,extraOffset=patch ? 40U : 35U,frameOffset=patch ? 36U : 34U;
    if (sid.value<5 || sid.value>kMaximumId || (root[11]&(kFree|kContained)) ||
        root[10]<kFirstAssetTypeNumber || root[10]>=types_.size() ||
        (operation!=Operation::Region && !(types_[root[10]].flags1&TypeFlag1::kSurface)))
        throw std::logic_error("raw 그래프 표면 SID 오류");
    Plan plan{records_,stack_,{},0};
    if (pop) {
        const auto& type=types_[root[10]];
        if (operation==Operation::Flood || !pop->type || pop->level<0 || pop->level>3 || type.flags1!=pop->type->flags1 ||
            type.flags2!=pop->type->flags2 || type.footX!=pop->type->footX || type.footY!=pop->type->footY)
            throw std::invalid_argument("Pop/raw 그래프 타입/단계 오류");
    }
    // Add의 비활성 입력은 원본처럼 그래프 표/스택을 읽지 않고 돌아간다.
    const auto state=static_cast<std::uint8_t>(pop ? root[11]&~kVoid : root[11]);
    if (add && (state&7) && !region) return plan;
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
    if (region) Region(sid,pop,spots,plan);
    if (operation==Operation::Region || (add && (state&7))) return plan;
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
            const auto staged=std::find_if(plan.numbers.begin(),plan.numbers.end(),[&](const auto& item){return item.first==current;});
            members.push_back({current.value,staged==plan.numbers.end() ? bytes[graphOffset] : staged->second,currentState});
        }
        objects.push_back({current.value,Coordinate(x),Coordinate(y),type.footX,type.footY,
            type.flags1,type.flags2,code,(currentState&kDead)!=0,(bytes[extraOffset]&kBuried)!=0});
    }
    SurfaceFinder finder(objects,map,spots); Graph graph(finder,members,plan.records,plan.stack);
    if (add) graph.Add(sid.value); else plan.changed=graph.Flood(sid.value,target);
    std::copy(graph.Records().begin(),graph.Records().end(),plan.records.begin());
    std::copy(graph.FloodStack().begin(),graph.FloodStack().end(),plan.stack.begin());
    // 영역에서 변경한 번호도 최종 Add 결과로 덮고 다른 raw 필드에는 쓰지 않는다.
    for (auto member:members) {
        const Sid current{member.id}; const auto number=graph.Number(member.id);
        const auto staged=std::find_if(plan.numbers.begin(),plan.numbers.end(),[&](const auto& item){return item.first==current;});
        if (staged!=plan.numbers.end()) staged->second=number;
        else if (number!=member.graph) plan.numbers.emplace_back(current,number);
    }
    return plan;
}
// 부작용 없는 계산으로 Pop 전 검사에도 사용한다.
void RawGraph::ValidateAdd(Sid sid,const RawGraphPop* pop) const { static_cast<void>(Calculate(sid,Operation::Add,0,pop)); }
// 원본 영역 통지가 먼저 만든 무효 번호/표를 같은 복사본의 Add가 읽는다.
void RawGraph::ValidatePostPop(Sid sid,bool invalidate,bool add,const RawGraphPop* pop) const {
    if (invalidate || add) static_cast<void>(Calculate(sid,invalidate ? (add ? Operation::RegionAdd : Operation::Region) : Operation::Add,0,pop));
}
// graph byte 외의 payload·좌표·owner·state는 보존한다.
void RawGraph::Commit(const Plan& plan) {
    const auto offset=pool_.Edition()==OriginalEdition::Patch1078 ? 30U : 28U;
    // 성공한 계산의 변경분만 raw 풀에 반영한다.
    for (const auto& [sid,number]:plan.numbers) pool_.AllocatedBytes(sid)[offset]=number;
    records_=plan.records; stack_=plan.stack;
}
// 현재 프레임·상태·지도로 매번 다시 계산한다.
void RawGraph::Add(Sid sid) { Commit(Calculate(sid,Operation::Add,0,nullptr)); }
// 기존 스택의 미사용 DWORD를 보존하여 flood 결과를 반영한다.
std::uint32_t RawGraph::Flood(Sid sid,std::uint8_t graph) {
    const auto plan=Calculate(sid,Operation::Flood,graph,nullptr); Commit(plan); return plan.changed;
}
// 영역 단독 helper도 동일한 원본 탐색·표 감소를 사용한다.
void RawGraph::InvalidateRegion(Sid sid) { Commit(Calculate(sid,Operation::Region,0,nullptr)); }
// 성공한 전체 후처리를 한 번에 raw 번호와 그래프 표에 반영한다.
void RawGraph::PostPop(Sid sid,bool invalidate,bool add) {
    if (invalidate || add) Commit(Calculate(sid,invalidate ? (add ? Operation::RegionAdd : Operation::Region) : Operation::Add,0,nullptr));
}
// sentinel과 reserved WORD를 포함한 전체 표를 제공한다.
std::span<const GraphRecord> RawGraph::Records() const { return records_; }
// 사용 후 스택 흔적을 다음 연산에도 이어 준다.
std::span<const std::uint32_t> RawGraph::FloodStack() const { return stack_; }
}
