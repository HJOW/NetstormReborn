// BFS의 원본 방문 순서/거리/동률 비용/방향 전환 중복 꼭짓점을 보존한다.
#include "o/RawPathSearch.h"
#include "o/RawPathProcess.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 북쪽부터 시계 방향인 원본 단위 좌표다. 큐에는 두 BYTE로 저장한다.
constexpr std::array<int,8> kDirectionX{0,1,1,1,0,-1,-1,-1},kDirectionY{-1,-1,0,1,1,1,0,-1};
// 표면 거의 올림/보드 변/원본 ring queue 마지막 번호다.
constexpr float kCellBias=0.9999F;
constexpr int kBoardSide=256;
constexpr std::uint32_t kQueueMask=0x3fff;
// 서로 다른 호스트 정렬에서도 같은 raw WORD/DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 낮은 byte부터 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8);
    return value;
}
// 보드 밖 진단을 원본 포인터 산술 밖에서 처리한다.
bool Inside(int x,int y) { return x>=0 && y>=0 && x<kBoardSide && y<kBoardSide; }
// signed WORD 거리의 원본 덧셈 감김을 보존한다.
std::int16_t Increment(std::int16_t value) { return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(value)+1)); }
// 좌표 보정은 float으로 한 번 저장하지 않고 x87 같은 double 합에서 절삭한다.
int Cell(float value) {
    const double adjusted=static_cast<double>(value)+kCellBias;
    if (!std::isfinite(adjusted) || adjusted<-2147483648.0 || adjusted>2147483647.0) throw std::out_of_range("BFS 표면 좌표");
    return static_cast<int>(adjusted);
}
}
// 실제 해시0/표면/타입 표가 공유되며 거리/큐만 검색기가 소유한다.
RawPathSearch::RawPathSearch(const SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
    std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,RawPathSearchState& state)
    :pool_(pool),hash_(hash),spots_(spots),types_(types),frames_(frames),state_(state),work_(std::make_unique<RawPathSearchWork>()) {
    if (spots.size()!=65536 || types.size()!=frames.size()) throw std::invalid_argument("BFS 실제 월드 연결");
}
// 표면 SID는 같은 실제 hash0 배열에서 조회한다.
Sid RawPathSearch::SurfaceAt(RawPathPoint position) const {
    return RawPathSurfaceAt(hash_,position);
}
// 거의 올림 좌표의 실제 지도 WORD만 읽어 Init의 자연 반환 앞에 힙 할당을 추가하지 않는다.
Sid RawPathSurfaceAt(const SquidHash& hash,RawPathPoint position) {
    const int x=Cell(position.x),y=Cell(position.y);if (!Inside(x,y)) return {};
    return Sid{hash.Cell(0,x,y)};
}
// Graph 포인터의 동일성은 같은 표면 Graph BYTE의 동일성과 같다. 실제 inUse/cost/owner는 여기서 읽지 않는다.
std::optional<std::uint8_t> RawPathSearch::GraphAt(RawPathPoint position) const {
    if (!state_.graphsEnabled) return {};
    const auto sid=SurfaceAt(position);if (sid.value==0) return {};
    const auto raw=pool_.Slot(sid);const auto type=raw[10];
    if (type>=types_.size() || (pool_.Edition()==OriginalEdition::Patch1078 && (types_[type].flags1&TypeFlag1::kSurface)==0 && type!=state_.fenceMarkType && type!=state_.windArcherType))
        throw std::logic_error("BFS Graph getter 표면 타입");
    const auto graph=raw[pool_.Edition()==OriginalEdition::Patch1078 ? 30 : 28];return graph==254 ? std::optional<std::uint8_t>{} : graph;
}
// 같은 표면의 여러 점유 칸을 다른 오브젝트로 해석하지 않는다.
bool RawPathSearch::Connects(Sid first,Sid second,std::int32_t direction) const {
    if (first.value==0 || second.value==0) return false;if (first==second) return true;
    return Bridge::Connects(Frame(first),types_[pool_.Slot(first)[10]].flags2,Frame(second),types_[pool_.Slot(second)[10]].flags2,direction);
}
// CD의 frame BYTE 다음 extra를 섞지 않는다.
FrameCode RawPathSearch::Frame(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto type=raw[10];const auto frame=pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)) : raw[34];
    if (type>=frames_.size() || frame<0 || static_cast<std::size_t>(frame)>=frames_[type].Codes().size()) throw std::out_of_range("BFS 표면 프레임");
    return frames_[type].Codes()[frame];
}
// 한 방향이 바뀔 때 같은 좌표의 이전/새 방향 꼭짓점 둘을 저장하는 원본 append다.
void RawPathSearch::Append(std::vector<RawPathPoint>& points,std::vector<std::int32_t>& directions,RawPathPoint point,std::int32_t direction,const RawPathRuntime& output) const {
    points.push_back(point);directions.push_back(direction);
    if (points.size()>=RawPathRuntime::Layout(pool_.Edition()).capacity || output.Bytes().size()!=RawPathRuntime::Layout(pool_.Edition()).size)
        throw std::out_of_range("BFS 최대 경로 길이");
}
// 미사용 queue byte는 다시 초기화하지 않는다. 원본은 거리표만 매 호출에서 전체 지운다.
bool RawPathSearch::SearchCells(RawPathRuntime& output,int sx,int sy,int dx,int dy,bool cardinalOnly) {
    if (!Inside(sx,sy) || !Inside(dx,dy)) throw std::out_of_range("BFS 시작/끝 칸");
    std::fill(work_->distance.begin(),work_->distance.end(),std::int16_t{});
    if (pool_.Edition()==OriginalEdition::Patch1078 && state_.policySelector==5) state_.directionPrerequisite[3]=5;
    work_->queue[0]={static_cast<std::uint8_t>(sx),static_cast<std::uint8_t>(sy)};work_->distance[sy*256+sx]=1;
    std::uint32_t tail=1,head=0,layerEnd=1;std::int16_t distance=2;
    // 원본은 cardinal 네 방향만 BFS 확장하며 대각선은 역추적 축약에서 선택한다.
    for (;;) {
        if (head>=work_->queue.size()) throw std::out_of_range("BFS 원본 큐 읽기 넘침");
        if (head==layerEnd) { distance=Increment(distance);layerEnd=tail; }
        const int x=work_->queue[head][0],y=work_->queue[head][1];const Sid current{hash_.Cell(0,x,y)};
        if (current.value==0) return false;if (x==dx && y==dy) break;
        std::uint32_t admitted=0;
        // 원본 우선순위의 앞 네 항목은 북/동/남/서다.
        for (std::size_t i=0;i<4;++i) {
            const auto direction=state_.directionOrder[i];if (direction<0 || direction>7) throw std::out_of_range("BFS 방향 전역");
            const auto required=state_.directionPrerequisite[direction];if ((admitted&required)!=required) continue;
            const int nx=x+kDirectionX[direction],ny=y+kDirectionY[direction];
            if (!Inside(nx,ny)) continue;
            const auto index=static_cast<std::size_t>(ny*256+nx);const auto spot=spots_[index];
            if (work_->distance[index]!=0 || (spot&6)==0 || (spot&8)!=0) continue;
            const Sid next{hash_.Cell(0,nx,ny)};if (next.value==0) throw std::logic_error("BFS 허용 지면의 표면0");
            if (!Connects(current,next,direction)) continue;
            work_->queue[tail]={static_cast<std::uint8_t>(nx),static_cast<std::uint8_t>(ny)};work_->distance[index]=distance;tail=(tail+1)&kQueueMask;admitted|=1U<<direction;
        }
        ++head;if (head==tail) return false;
    }
    std::vector<RawPathPoint> points;std::vector<std::int32_t> directions;std::vector<double> times;
    // append 전 기존 경로와 시각을 보존한다. 원본 search 본체는 nVert를 자동 0으로 만들지 않는다.
    for (std::int32_t i=0;i<output.Count();++i) { points.push_back(output.Point(i));directions.push_back(output.Direction(i));times.push_back(output.Time(i)); }
    int x=dx,y=dy,remaining=work_->distance[y*256+x]-1,previousDirection=-1;
    if (remaining==0) Append(points,directions,{static_cast<float>(x),static_cast<float>(y)},0,output);
    // 짧은 거리 후보/현재 방향/대각선/raw flag 비용을 실제 방문 순서로 비교한다.
    while (remaining>0) {
        const Sid current{hash_.Cell(0,x,y)};int alternate=remaining;if (!cardinalOnly && remaining>1) --alternate;
        int bestCost=9999,bestDirection=0;std::uint32_t admitted=0;
        // cardinalOnly는 앞 네 항목, 일반 사제는 여덟 방향으로 역추적한다.
        for (std::size_t i=0;i<(cardinalOnly ? 4U : 8U);++i) {
            const auto direction=state_.directionOrder[i];if (direction<0 || direction>7) throw std::out_of_range("BFS 역추적 방향");
            const int nx=x+kDirectionX[direction],ny=y+kDirectionY[direction];if (!Inside(nx,ny)) continue;
            const auto value=work_->distance[ny*256+nx];if (value!=remaining && value!=alternate) continue;
            const Sid next{hash_.Cell(0,nx,ny)};if (next.value==0) throw std::logic_error("BFS 역추적 표면0");
            const auto required=state_.directionPrerequisite[direction];if ((admitted&required)!=required || !Connects(current,next,direction)) continue;
            int cost=direction==previousDirection ? 10 : 15;if ((direction&1)!=0) cost-=10;
            if (state_.avoidFlagTwo && (pool_.Slot(next)[12]&6)==2) cost+=30;
            if (cost<bestCost) { bestCost=cost;bestDirection=direction; }admitted|=1U<<direction;
        }
        if (bestCost==9999) throw std::logic_error("BFS 역추적 후보 없음");
        if (bestDirection!=previousDirection) {
            if (previousDirection!=-1) Append(points,directions,{static_cast<float>(x),static_cast<float>(y)},previousDirection,output);
            Append(points,directions,{static_cast<float>(x),static_cast<float>(y)},bestDirection,output);previousDirection=bestDirection;
        }
        x+=kDirectionX[bestDirection];y+=kDirectionY[bestDirection];remaining=work_->distance[y*256+x]-1;
    }
    if (previousDirection!=-1) Append(points,directions,{static_cast<float>(x),static_cast<float>(y)},previousDirection,output);
    if (x!=sx || y!=sy) throw std::logic_error("BFS 역추적 시작 불일치");
    times.resize(points.size(),0.0);output.SetPath(points,directions,times);return true;
}
// actual wrapper는 목적 표면의 Graph가 존재하며 시작과 같아야 검색한다. 소유자 정책을 더하지 않는다.
bool RawPathSearch::Search(RawPathRuntime& output,Sid parent,RawPathPoint start,RawPathPoint destination,std::uint32_t flags) {
    const auto parentType=pool_.Slot(parent)[10];if (parentType>=types_.size()) throw std::out_of_range("BFS 부모 타입");
    const auto destinationGraph=GraphAt(destination);if (!destinationGraph || destinationGraph!=GraphAt(start)) return false;
    const int sx=Cell(destination.x),sy=Cell(destination.y),dx=Cell(start.x),dy=Cell(start.y);
    if (!Inside(sx,sy) || !Inside(dx,dy) || (spots_[dy*256+dx]&6)==0) return false;
    if (!state_.searchEnabled) throw std::logic_error("원본 kenGo 비활성 진단");
    return SearchCells(output,sx,sy,dx,dy,(flags&1)!=0);
}
// 전체 작업 배열을 읽기 전용으로 제공한다.
const RawPathSearchWork& RawPathSearch::Work() const { return *work_; }
// 실제 PathProcess/월드 연결의 풀 일치를 검사한다.
const SidPool& RawPathSearch::Pool() const { return pool_; }
// 명령의 현재 시작·목적·flags를 그대로 원본 검색기에 전달한다. 성공 여부로 대체 경로를 만들지 않는다.
RawPathProcessHooks MakePathSearchHooks(const SidPool& pool,RawPathSearch& search,RawPathProcessHooks hooks) {
    if (&pool!=&search.Pool()) throw std::invalid_argument("Path 검색/프로세스 SID 풀 불일치");
    hooks.search=[&search](RawPathRuntime& output,Sid parent,RawPathPoint start,RawPathPoint destination,std::uint32_t flags) {
        static_cast<void>(search.Search(output,parent,start,destination,flags));
    };
    return hooks;
}
}
