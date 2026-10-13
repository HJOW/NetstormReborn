// 원본 flood의 소유자 방문 표시·직접 프레임 쓰기와 패치 경계 누적 순서를 보존한다.
#include "o/RawRegionPaint.h"
#include "o/RawFrameChoose.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 지형 좌표와 두 판본의 프레임/소유자 raw 오프셋이다.
constexpr std::size_t kX=14,kY=18,kPatchFrame=36,kCdFrame=34,kPatchOwner=34,kCdOwner=32;
// 원본 네 방향은 북·동·남·서이며 비트는1·2·4·8이다.
constexpr std::array<CellPoint,4> kDirections{{{0,-1},{1,0},{0,1},{-1,0}}};
// 비정렬 little endian 필드를 지정 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 인접 CD필드를 섞지 않고 실제 폭만 조립한다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
// raw float 좌표에 원본 _ftol의 0 방향 절삭을 적용한다.
int Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    const auto value=std::trunc(static_cast<double>(std::bit_cast<float>(Read(raw,offset))));
    if (!std::isfinite(value) || value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max())
        throw std::out_of_range("지형 도장 좌표 절삭 범위 오류");
    return static_cast<int>(value);
}
}
// 지면과 경계는 원본처럼 하나의 지도와 같은 슬롯을 조회한다.
RawRegionPaint::RawRegionPaint(SidPool& pool,std::span<const RiftTypeFrames> frames,
    std::span<const std::uint16_t> islands,std::uint32_t terrainType,std::uint32_t edgeFarmType,
    GameRandom& random,RegionPaintState& state,RegionOwnershipState& ownership,RegionPaintHooks hooks)
    :pool_(pool),frames_(frames),islands_(islands),terrainType_(terrainType),edgeFarmType_(edgeFarmType),
     random_(random),state_(state),ownership_(ownership),hooks_(std::move(hooks)) {
    if (islands.size()!=65536 || terrainType>=frames.size() || !hooks_.setOwner ||
        (pool.Edition()==OriginalEdition::Patch1078 && (edgeFarmType>=frames.size() || !hooks_.boundaryAt || !hooks_.regionAt)))
        throw std::invalid_argument("지역 도장 지도/프레임/필수 효과 연결 오류");
}
// 외부 연결의 SID 저장소 일치 검사에 사용한다.
const SidPool& RawRegionPaint::Pool() const { return pool_; }
// 원본의 범위 검사 뒤 WORD0 처리와 같다.
Sid RawRegionPaint::Surface(int x,int y) const {
    return x>=0 && y>=0 && x<256 && y<256 ? Sid{islands_[static_cast<std::size_t>(y*256+x)]} : Sid{};
}
// 소유자 가상 호출 뒤 현재 프레임을 지면 코드 배열에서 읽는다.
FrameCode RawRegionPaint::Code(Sid sid) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto index=Read(pool_.Slot(sid),patch ? kPatchFrame : kCdFrame,patch ? 4 : 1);
    const auto codes=frames_[terrainType_].Codes();
    if (index>=codes.size()) throw std::out_of_range("지역 도장 현재 프레임 코드 범위 오류");
    return codes[index];
}
// 캐시 생성은 게임 전역 난수를 소비하지 않는 원본 로컬 seed0 동작이다.
void RawRegionPaint::Initialize(std::optional<std::array<std::int32_t,99>>& values) {
    if (values) return;
    values.emplace();
    // 매 항목에서 seed0를 만들고 이전 항목과 mod4 동률만 교정한다.
    for (std::size_t i=0;i<values->size();++i) {
        std::uint32_t seed=0;auto value=TerrainBuilder::Next(seed,65536);
        if (i!=0 && (*values)[i-1]%4==value%4) ++value;
        (*values)[i]=value;
    }
}
// 기계어에서 두 cache 좌표 항은 요청 x/y이며 픽셀 residue는 현재 raw 좌표를 절삭한다.
std::int32_t RawRegionPaint::Select(Sid sid,int x,int y,bool boundary) {
    const auto code=Code(sid);auto& cache=boundary ? state_.boundaryVariants : state_.coreVariants;Initialize(cache);
    const auto type=pool_.Slot(sid)[10];if (type>=frames_.size()) throw std::out_of_range("지역 도장 타입 번호 오류");
    if (code.side!='A' || code.variant!='A') return ChooseTerrainFrame(frames_[type],code,state_.theme,random_);
    const auto raw=pool_.Slot(sid);const int px=Coordinate(raw,kX),py=Coordinate(raw,kY);
    const auto bx=(x/3)%99,by=(y/3)%99;
    if (bx<0 || by<0) throw std::out_of_range("지역 도장 변형 캐시 좌표 오류");
    const auto sum=((*cache)[static_cast<std::size_t>(bx)]+(*cache)[static_cast<std::size_t>(by)])%99;
    const auto variant=(py/3+(*cache)[static_cast<std::size_t>(sum)])%4;
    const auto theme=state_.theme==0 || state_.theme==-1 ? 0 : state_.theme==1 ? 1 : state_.theme==2 ? 2 : 3;
    return frames_[type].Run('J').first+1+theme*36+(variant*3+py%3)*3+px%3;
}
// 좌표별 중복 제거는 값 전체가 같을 때만 적용한다.
void RawRegionPaint::AddBoundary(int x,int y) {
    const CellPoint point{x,y};
    if (std::find(state_.boundary.begin(),state_.boundary.end(),point)==state_.boundary.end()) state_.boundary.push_back(point);
}
// 표면0/범위 밖 후보도 원본은 목록에 넣고 다음 stamp에서 일반 finder를 수행한다.
void RawRegionPaint::AddNeighbors(int x,int y,int connections) {
    // 원본 코드의 북·동·남·서 호출 순서를 보존한다.
    for (std::size_t i=0;i<kDirections.size();++i) if ((connections&(1<<i))!=0) AddBoundary(x+kDirections[i].x,y+kDirections[i].y);
}
// 재귀의 가상 호출 순서를 유지하며 큰 실제 섬에서도 호스트 C++ 스택을 넘지 않게 한다.
void RawRegionPaint::Flood(int x,int y) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;std::vector<CellPoint> work{{x,y}};
    // 원본의 북·동·남 재귀와 서쪽 tail loop를 역순 push로 재현한다.
    while (!work.empty()) {
        const auto point=work.back();work.pop_back();const auto sid=Surface(point.x,point.y);
        if (sid.value==0) { if (patch) AddBoundary(point.x,point.y);continue; }
        const auto raw=pool_.Slot(sid);
        if (raw[10]!=terrainType_ || static_cast<std::uint32_t>(raw[patch ? kPatchOwner : kCdOwner])==state_.owner) continue;
        hooks_.setOwner(sid,state_.owner);
        const auto code=Code(sid);const auto connections=TerrainBuilder::Connection(static_cast<char>(code.side));
        if (patch) {
            const auto selected=static_cast<std::uint32_t>(Select(sid,point.x,point.y,false));auto target=pool_.AllocatedBytes(sid);
            // 원본은 SetFrame 가상 함수를 부르지 않고 DWORD만 직접 저장한다.
            for (std::size_t i=0;i<4;++i) target[kPatchFrame+i]=static_cast<std::uint8_t>(selected>>(i*8));
        }
        // 먼저 호출할 이웃이 마지막 push가 되도록 서·남·동·북 순서로 준비한다.
        for (std::size_t i=kDirections.size();i>0;--i) if ((connections&(1<<(i-1)))!=0)
            work.push_back({point.x+kDirections[i-1].x,point.y+kDirections[i-1].y});
    }
}
// patch만 빈 표면의 edgeFarm fallback·영향 지역·추가 후보를 수행한다.
void RawRegionPaint::Boundary(int x,int y) {
    auto sid=Surface(x,y);
    if (sid.value==0) { sid=hooks_.boundaryAt(static_cast<float>(x),static_cast<float>(y),edgeFarmType_);if (sid.value==0) return; }
    const auto region=hooks_.regionAt(static_cast<float>(x),static_cast<float>(y));
    if (std::none_of(ownership_.affected.begin(),ownership_.affected.end(),[&](const OwnershipAffectedRegion& value) { return value.region==region; }))
        ownership_.affected.push_back({std::bit_cast<float>(static_cast<std::uint32_t>(x)),std::bit_cast<float>(static_cast<std::uint32_t>(y)),region});
    const auto raw=pool_.Slot(sid);const auto type=raw[10];
    if (type==terrainType_ && static_cast<std::uint32_t>(raw[kPatchOwner])==state_.owner) return;
    if (type!=terrainType_ && type!=edgeFarmType_) return;
    hooks_.setOwner(sid,state_.owner);const auto code=Code(sid);const auto connections=TerrainBuilder::Connection(static_cast<char>(code.side));
    const auto selected=static_cast<std::uint32_t>(Select(sid,x,y,true));auto target=pool_.AllocatedBytes(sid);
    // 경계도 직접 프레임 쓰기 뒤에 이웃 후보를 추가한다.
    for (std::size_t i=0;i<4;++i) target[kPatchFrame+i]=static_cast<std::uint8_t>(selected>>(i*8));
    AddNeighbors(x,y,connections);
}
// 실패한 시작 좌표에서도 원본 전역 revision 증가를 생략하지 않는다.
void RawRegionPaint::Paint(const RegionOwnershipPaint& request) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;if (patch) state_.boundary.clear();
    state_.bounds={257.0F,257.0F,-1.0F,-1.0F};const auto sid=Surface(request.x,request.y);
    if (sid.value!=0 && pool_.Slot(sid)[10]==terrainType_) { state_.owner=request.owner;if (patch) state_.theme=static_cast<std::int32_t>(request.theme);Flood(request.x,request.y); }
    if (patch) {
        // stamp 중 추가된 후보도 같은 호출에서 뒤이어 처리한다.
        for (std::size_t i=0;i<state_.boundary.size();++i) { const auto point=state_.boundary[i];Boundary(point.x,point.y); }
    }
    ++state_.revision;
}
// 기존 투표 wrapper를 다시 구현하지 않고 실제 도장 몸체만 공급한다.
RegionOwnershipHooks MakeRegionPaintOwnershipHooks(const SidPool& pool,RawRegionPaint& paint,RegionOwnershipHooks hooks) {
    if (&pool!=&paint.Pool()) throw std::invalid_argument("지역 투표/도장 SID 풀이 다릅니다");
    hooks.paint=[&paint](const RegionOwnershipPaint& request) { paint.Paint(request); };return hooks;
}
}
