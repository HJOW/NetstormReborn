// 원본 주소를 호스트 포인터로 해석하지 않고 raw 슬롯의 판본별 필드만 읽는다.
#include "o/RawGraph.h"
#include "o/Squid.h"
#include "o/RawSquidFinder.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 두 판본에서 같은 상태 비트다. 전역 순회는 void/contained도 읽지만 그런 해시 머리는 거부한다.
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
    std::span<const GraphRecord> records,std::span<const std::uint32_t> stack,GraphRecovery recovery)
    :pool_(pool),hash_(hash),spots_(spots),types_(types.begin(),types.end()),frames_(frames.begin(),frames.end()),recovery_(recovery) {
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
// 삭제 연결의 일반 탐색은 flag 8 표면 탐색의 순서/교차 필터로 대체하지 않는다.
std::vector<std::uint16_t> RawGraph::DetachConnections(const SurfaceObject& source,const SurfaceFinder& finder) const {
    std::vector<std::uint16_t> result;
    // 발자국 helper는 두 번째 점을 1..255로 보정한다. 초기 점이 무효면 양 끝을 그 점으로 재설정한다.
    const auto bounds=[](const SurfaceObject& object) {
        const int x=std::clamp(object.x-object.width+1,1,kWorldCells-1);
        const int y=std::clamp(object.y-object.height+1,1,kWorldCells-1);
        return object.x>0 && object.y>0 ? std::array<int,4>{x,y,object.x,object.y} : std::array<int,4>{x,y,x,y};
    };
    const auto rect=bounds(source); const int left=rect[0],top=rect[1],right=rect[2],bottom=rect[3];
    std::uint8_t interior=0xff;
    // 원본은 원천 발자국 spot 전체의 AND로 내부 상태를 확인한다.
    for (int y=top;y<=bottom;++y)
        // 원천의 정수 발자국 각 칸을 확인한다.
        for (int x=left;x<=right;++x) interior&=spots_[static_cast<std::size_t>(y*kWorldCells+x)];
    if (interior&8) return result;
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    // 패치의 (작음 != 같음)도 정수 좌표에서는 <=다. 세 판본 모두 경계 접촉을 센다.
    const auto intersects=[&](int aLeft,int aTop,int aRight,int aBottom) {
        const int x1=std::max(left,aLeft),y1=std::max(top,aTop),x2=std::min(right,aRight),y2=std::min(bottom,aBottom);
        return x1>0 && y1>0 && x2<kWorldCells && y2<kWorldCells &&
            x1<=x2 && y1<=y2;
    };
    // 일반 finder는 원천을 한 칸 넓히고 단계별 마지막 버킷도 하나 늘린다.
    for (int level=0;level<4;++level) {
        const int scale=SquidHash::kScales[static_cast<std::size_t>(level)],side=SquidHash::Side(level);
        const int startX=std::max(0,left-1)/scale,startY=std::max(0,top-1)/scale;
        const int endX=std::min(side-1,std::min(kWorldCells-1,right+1)/scale+1);
        const int endY=std::min(side-1,std::min(kWorldCells-1,bottom+1)/scale+1);
        const auto heads=hash_.Entries(level);
        // 같은 단계 안에서는 y/x, 같은 버킷 안에서는 next 순서를 보존한다.
        for (int cy=startY;cy<=endY;++cy)
            // 끝 버킷의 큰 발자국 후보도 원본과 같은 순서로 판정한다.
            for (int cx=startX;cx<=endX;++cx) {
                auto current=heads[static_cast<std::size_t>(cy*side+cx)]; std::vector<std::uint16_t> chain;
                // 손상된 SID/자기 next/순환은 계획 계산 중에 거부한다.
                while (current) {
                    if (current<5 || current>kMaximumId || current>=pool_.Capacity() ||
                        std::find(chain.begin(),chain.end(),current)!=chain.end())
                        throw std::logic_error("그래프 삭제 해시 범위/순환 오류");
                    chain.push_back(current); const auto bytes=pool_.Slot(Sid{current}); const auto candidate=current;
                    current=static_cast<std::uint16_t>(Read(bytes,4,2));
                    if (bytes[11]&(kFree|kContained|kVoid)) throw std::logic_error("그래프 삭제 후보 상태 오류");
                    if (bytes[10]<kFirstAssetTypeNumber || bytes[10]>=types_.size()) throw std::logic_error("그래프 삭제 후보 타입 오류");
                    // CD 일반 finder는 매몰 제외 뒤 모든 후보의 양수 위치를 assert한다. UI 대신 변경 전 거부한다.
                    if (!patch && !(bytes[35]&kBuried) &&
                        (Coordinate(std::bit_cast<float>(Read(bytes,14)))<=0 || Coordinate(std::bit_cast<float>(Read(bytes,18)))<=0))
                        throw std::logic_error("CD 그래프 삭제 후보의 무효 위치");
                    if ((bytes[11]&kDead) || (bytes[patch ? 40 : 35]&kBuried) || !(types_[bytes[10]].flags1&TypeFlag1::kSurface)) continue;
                    const auto& object=finder.Object(candidate); const auto a=bounds(object);
                    if ((spots_[static_cast<std::size_t>(object.y*kWorldCells+object.x)]&8) ||
                        intersects(a[0]-1,a[1],a[2]+1,a[3])==intersects(a[0],a[1]-1,a[2],a[3]+1) ||
                        !SurfaceFinder::Connects(object,source)) continue;
                    if (std::find(result.begin(),result.end(),candidate)!=result.end()) throw std::logic_error("그래프 삭제 중복 해시 후보");
                    result.push_back(candidate);
                }
            }
    }
    return result;
}
// 전체 계산을 복사본에서 수행하며 미지원 입력·소진·연결 오류를 쓰기 전에 보고한다.
RawGraph::Plan RawGraph::Calculate(Sid sid,Operation operation,std::uint8_t target,const RawGraphPop* pop,std::optional<std::uint8_t> sourceType) const {
    const bool global=operation==Operation::Rebuild || operation==Operation::Allocate;
    Plan plan{records_,stack_,{},0};
    if (operation==Operation::Allocate) {
        // 원본의 여유 번호 경로는 풀/프레임/지도를 읽지 않는다.
        for (std::size_t i=0;i<Graph::kCount;++i) if (!plan.records[i].inUse) {
            plan.records[i].surfaces=0; plan.records[i].inUse=1; plan.changed=static_cast<std::uint32_t>(i); return plan;
        }
    }
    const bool region=operation==Operation::Region || operation==Operation::RegionAdd;
    const bool add=operation==Operation::Add || operation==Operation::RegionAdd;
    // 자동 재구성 가능 경로는 void/contained 표면까지 전체 풀에서 읽는다. 부분 스냅샷 정책은 유지한다.
    const bool fullPool=global || (recovery_==GraphRecovery::FullPool && (add || operation==Operation::Detach));
    const auto root=pool_.Slot(sid); const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto graphOffset=patch ? 30U : 28U,extraOffset=patch ? 40U : 35U,frameOffset=patch ? 36U : 34U;
    if (!global && (sid.value<5 || sid.value>kMaximumId || (root[11]&(kFree|kContained)) ||
        root[10]<kFirstAssetTypeNumber || root[10]>=types_.size() ||
        (operation!=Operation::Region && !(types_[root[10]].flags1&TypeFlag1::kSurface))))
        throw std::logic_error("raw 그래프 표면 SID 오류");
    if (operation==Operation::Detach && (!(root[11]&kDead) || (target&~3U)))
        throw std::invalid_argument("raw 그래프 삭제 준비의 dead/정책 오류");
    auto detachGraph=root[graphOffset];
    if (operation==Operation::Detach) {
        // GetGridSid는 해시 객체 +12의 0단계 머리를 읽는다. 다른 단계의 원천은 자연 반환한다.
        const int x=Coordinate(std::bit_cast<float>(Read(root,14))),y=Coordinate(std::bit_cast<float>(Read(root,18)));
        const auto found=hash_.Entries(0)[static_cast<std::size_t>(y*kWorldCells+x)];
        if (!found) return plan;
        // 원본 GetGraph(position)은 삭제 원천과 독립적으로 0단계 머리 SID의 graph byte를 읽는다.
        if (found<5 || found>kMaximumId || found>=pool_.Capacity()) throw std::out_of_range("raw 그래프 삭제 조회 SID 오류");
        const auto lookup=pool_.Slot(Sid{found});
        if ((lookup[11]&(kFree|kContained|kVoid)) || lookup[10]<kFirstAssetTypeNumber || lookup[10]>=types_.size() ||
            !(types_[lookup[10]].flags1&TypeFlag1::kSurface) ||
            Coordinate(std::bit_cast<float>(Read(lookup,14)))!=x || Coordinate(std::bit_cast<float>(Read(lookup,18)))!=y)
            throw std::logic_error("raw 그래프 삭제 조회 표면/위치 오류");
        // 원본은 무효/미사용 그래프에서 finder를 만들지 않는다. 손상 이웃도 아직 읽지 않는다.
        const auto number=lookup[graphOffset]; if (number==Graph::kInvalid) return plan;
        if (number>=Graph::kCount) throw std::out_of_range("raw 그래프 삭제 번호 오류");
        if (!records_[number].inUse) return plan;
        detachGraph=number;
    }
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
    std::vector<bool> required(pool_.Capacity()); if (!global) required[sid.value]=true;
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
        if (currentState&(fullPool ? kFree : (kFree|kVoid))) {
            if (required[id] && current!=sid) throw std::logic_error("raw 그래프 해시의 free/void SID");
            if (current!=sid) continue;
        }
        if (bytes[10]<kFirstAssetTypeNumber || bytes[10]>=types_.size()) {
            if (required[id] || fullPool) throw std::logic_error("raw 그래프 후보 타입 오류");
            continue;
        }
        const auto& type=types_[bytes[10]]; const bool surface=(type.flags1&TypeFlag1::kSurface)!=0;
        if (!surface && !required[id]) continue;
        if (id>kMaximumId || ((currentState&kContained) && (!fullPool || required[id])) || (fullPool && (currentState&kVoid) && required[id]))
            throw std::out_of_range("raw 그래프 후보 상태/SID 범위 오류");
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
    if (operation==Operation::Rebuild) graph.Rebuild(target!=0);
    else if (operation==Operation::Allocate) plan.changed=graph.AllocateWithRecovery();
    else if (add) graph.Add(sid.value,recovery_);
    else if (operation==Operation::Detach) {
        auto source=finder.Object(sid.value);
        if (sourceType) {
            if (*sourceType>=types_.size()) throw std::out_of_range("raw 그래프 삭제 치환 타입 오류");
            const auto& replacement=types_[*sourceType];const auto frame=Read(root,frameOffset,patch ? 4U : 1U);
            if (!(replacement.flags1&TypeFlag1::kSurface) || frame>=frames_[*sourceType].size())
                throw std::out_of_range("raw 그래프 삭제 치환 프레임/표면 오류");
            if (replacement.footX<1 || replacement.footY<1 || replacement.footX>kWorldCells || replacement.footY>kWorldCells)
                throw std::out_of_range("raw 그래프 삭제 치환 발자국 오류");
            source.width=replacement.footX;source.height=replacement.footY;source.flags1=replacement.flags1;
            source.flags2=replacement.flags2;source.frame=frames_[*sourceType][frame];
        }
        // 연결 판단은 삭제 정보의 타입을 쓰되 전체 풀 재구성은 원본 raw 타입을 읽는다.
        graph.DetachAt(sid.value,detachGraph,DetachConnections(source,finder),(target&1)!=0,(target&2)!=0 ? 9 : 1,recovery_,source.flags2);
    }
    else plan.changed=graph.Flood(sid.value,target);
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
// 분할 정책/특수 감소를 인코딩하고 선택한 전체 풀 소진 복구도 같은 사전 계획에서 검사한다.
void RawGraph::ValidateDetach(Sid sid,bool rebuild,std::uint8_t removedSurfaces,std::optional<std::uint8_t> sourceType) const {
    if (removedSurfaces!=1 && removedSurfaces!=9) throw std::invalid_argument("raw 그래프 감소 수 오류");
    static_cast<void>(Calculate(sid,Operation::Detach,static_cast<std::uint8_t>((rebuild ? 1 : 0)|(removedSurfaces==9 ? 2 : 0)),nullptr,sourceType));
}
// 같은 성공 계획의 번호·표·스택만 적용하고 원천 state/좌표/next는 Unpop에 남긴다.
void RawGraph::Detach(Sid sid,bool rebuild,std::uint8_t removedSurfaces,std::optional<std::uint8_t> sourceType) {
    if (removedSurfaces!=1 && removedSurfaces!=9) throw std::invalid_argument("raw 그래프 감소 수 오류");
    Commit(Calculate(sid,Operation::Detach,static_cast<std::uint8_t>((rebuild ? 1 : 0)|(removedSurfaces==9 ? 2 : 0)),nullptr,sourceType));
}
// 254는 다른 입력을 읽지 않는 자연 반환이며 251..253/255는 정상 번호가 아니다.
void RawGraph::ValidateFree(std::uint8_t graph) const {
    if (graph>=Graph::kCount && graph!=Graph::kInvalid) throw std::out_of_range("raw 그래프 Free 번호 오류");
}
// 레코드의 reserved WORD·풀 번호·전체 스택을 보존한다.
void RawGraph::Free(std::uint8_t graph) { ValidateFree(graph);if (graph!=Graph::kInvalid) records_[graph].surfaces=records_[graph].inUse=0; }
// 판본별 frame 폭을 구별하며 특수 타입의 현재 방향 문자를 조회한다.
FrameCode RawGraph::Frame(Sid sid) const {
    const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto number=raw[10];const auto index=Read(raw,patch ? 36 : 34,patch ? 4U : 1U);
    if (number<kFirstAssetTypeNumber || number>=types_.size() || index>=frames_[number].size())
        throw std::out_of_range("raw 그래프 프레임 조회 오류");
    return frames_[number][index];
}
// post의 일반 탐색 순서에 따라 이전 Add의 번호/표/스택을 다음 Add가 읽는다.
RawGraph::Plan RawGraph::CalculatePostDestroy(Sid sid) const {
    const auto root=pool_.Slot(sid);
    if (sid.value<5 || (root[11]&(kFree|kContained)) || root[10]<kFirstAssetTypeNumber || root[10]>=types_.size())
        throw std::logic_error("raw 그래프 postDestroy 자산 오류");
    const auto& type=types_[root[10]];const int x=Coordinate(std::bit_cast<float>(Read(root,14))),y=Coordinate(std::bit_cast<float>(Read(root,18)));
    if (type.footX<1 || type.footY<1 || type.footX>kWorldCells || type.footY>kWorldCells)
        throw std::out_of_range("raw 그래프 postDestroy 발자국 오류");
    const int left=std::clamp(x-type.footX+1,1,kWorldCells-1),top=std::clamp(y-type.footY+1,1,kWorldCells-1);
    const SquidSearchArea area{left,top,x>0 && y>0 ? x : left,x>0 && y>0 ? y : top};
    auto nextPool=pool_;RawGraph next(nextPool,hash_,spots_,types_,frames_,records_,stack_,recovery_);
    RawSquidFinder finder(nextPool,hash_,types_);
    // Begin는 첫 Next까지 수행한다. 반환 직전 next 저장과 단계/y/x 순서를 그대로 사용한다.
    for (auto candidate=finder.Begin(area);candidate.value;candidate=finder.Next()) {
        const auto raw=nextPool.Slot(candidate);
        if (raw[10]>=types_.size()) throw std::out_of_range("raw 그래프 postDestroy 후보 타입 오류");
        if (types_[raw[10]].flags1&TypeFlag1::kSurface) next.Add(candidate);
    }
    Plan plan{next.records_,next.stack_,{},0};const auto offset=pool_.Edition()==OriginalEdition::Patch1078 ? 30U : 28U;
    // 실제 raw 필드 중 graph byte의 변경분만 원본 풀에 반영할 계획에 넣는다.
    for (std::uint32_t id=5;id<pool_.Capacity();++id) {
        const Sid current{static_cast<std::uint16_t>(id)};const auto number=nextPool.Slot(current)[offset];
        if (number!=pool_.Slot(current)[offset]) plan.numbers.emplace_back(current,number);
    }
    return plan;
}
// 앞선 Add 성공 뒤 뒤 후보가 실패해도 실제 풀/표/스택은 보존한다.
void RawGraph::ValidatePostDestroy(Sid sid) const { static_cast<void>(CalculatePostDestroy(sid)); }
// 원본 순차 Add의 최종 결과를 한 번에 반영한다. 이 함수는 Unpop/반납을 수행하지 않는다.
void RawGraph::PostDestroy(Sid sid) { Commit(CalculatePostDestroy(sid)); }
// 기존 스택의 미사용 DWORD를 보존하여 flood 결과를 반영한다.
std::uint32_t RawGraph::Flood(Sid sid,std::uint8_t graph) {
    const auto plan=Calculate(sid,Operation::Flood,graph,nullptr); Commit(plan); return plan.changed;
}
// 지역 root를 지정하지 않고 전체 할당 풀에서 계산한다.
void RawGraph::Rebuild(bool resetAll) { Commit(Calculate(Sid{},Operation::Rebuild,resetAll ? 1 : 0,nullptr)); }
// 재구성 결과의 번호/표/스택을 함께 적용한 뒤 확보한 번호를 돌려준다.
std::uint8_t RawGraph::Allocate() {
    const auto plan=Calculate(Sid{},Operation::Allocate,0,nullptr); Commit(plan); return static_cast<std::uint8_t>(plan.changed);
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
