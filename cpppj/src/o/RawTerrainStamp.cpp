// 초기 terrain frame 선택과 실제 stamp를 두 난수/현재 지도/같은 SID 풀에서 원본 순서로 실행한다.
#include "o/RawTerrainStamp.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 factory 결과의 Pop flags이며 위치0입력/프로세스 생성 의미를 보존한다.
constexpr std::uint32_t kPopFlags=0x02000061U;
// 실제 _rand의 holdrand 곱셈/증가 값과15비트 반환 mask다.
constexpr std::uint32_t kCrtMultiplier=0x343fdU,kCrtIncrement=0x269ec3U,kCrtMask=0x7fffU;
// 실제 양판본의 북부터 시계방향 bit표다. 대각선은 북서1/북동2/남동4/남서8이다.
constexpr std::array<int,8> kNeighborBits{1,2,2,4,4,8,8,1};
// 프레임flags BYTE를 MSVC signed char→DWORD로 부호 확장한다.
std::uint32_t SignedFlags(std::uint8_t flags) {
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(flags)));
}
// 실제 문자의 하위BYTE를 부호 있는 값으로 읽는다. 지도에 ASCII 밖 문자가 있으면 별도 raw 범위가 필요하다.
std::size_t Character(std::uint8_t code) {
    if (std::bit_cast<std::int8_t>(code)<0) throw std::out_of_range("초기stamp signed 문자mapping 범위 오류");
    return code;
}
}
// 지도를 복사하지 않고 실제 저장소와 필수 하위 효과를 보관한다.
RawTerrainStamp::RawTerrainStamp(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    TerrainStampMap& map,GameRandom& random,std::uint32_t& crtState,TerrainStampState& state,TerrainStampHooks hooks)
    :pool_(pool),types_(types),frames_(frames),map_(map),random_(random),crtState_(crtState),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || frames.size()!=count || map.codes.size()!=65536 || map.regions.size()!=65536 ||
        map.typeMapping.size()!=256 || map.aliases.size()!=256 || !hooks_.create || !hooks_.setOwner || !hooks_.pop ||
        !hooks_.setRegion || !hooks_.chunkRegion || !hooks_.assertion)
        throw std::invalid_argument("초기stamp 지도/타입/필수 효과 누락");
}
// 실제 지도 접근은 X열 우선이고 유효 좌표를 밖에서 clamp하지 않는다.
std::size_t RawTerrainStamp::Index(std::int32_t x,std::int32_t y) {
    if (x<0 || x>=256 || y<0 || y>=256) throw std::out_of_range("초기stamp 지도 범위 오류");
    return static_cast<std::size_t>(x)*256+static_cast<std::size_t>(y);
}
// 이웃은 bounds 양 끝을 포함하고 같은 aliasDWORD/지역BYTE일 때만 mask에 들어간다.
bool RawTerrainStamp::Same(std::int32_t x,std::int32_t y,std::int32_t alias,std::uint8_t region) const {
    const auto& bounds=map_.bounds;
    if (x<bounds[0] || y<bounds[1] || x>bounds[2] || y>bounds[3]) return false;
    const auto index=Index(x,y);return map_.aliases[Character(map_.codes[index])]==alias && map_.regions[index]==region;
}
// 단일 전역cache는 첫 프레임 요청에서만 현재 CRTstate를 전진시킨다.
void RawTerrainStamp::Initialize() {
    if (state_.variants) return;state_.variants.emplace();
    // 원본99개 연속 _rand와 앞 원소의 signedmod4 비교 순서를 그대로 수행한다.
    for (std::size_t index=0;index<state_.variants->size();++index) {
        crtState_=crtState_*kCrtMultiplier+kCrtIncrement;
        auto value=static_cast<std::int32_t>((crtState_>>16)&kCrtMask);
        if (index && (*state_.variants)[index-1]%4==value%4) ++value;
        (*state_.variants)[index]=value;
    }
}
// 직선/대각선 이웃은 실제 표의 북부터 시계방향 네 비트이며 cache는 비AA에서도 초기화한다.
std::int32_t RawTerrainStamp::FrameAt(std::int32_t x,std::int32_t y) {
    const auto index=Index(x,y);const auto code=map_.codes[index];const auto character=Character(code);
    const auto alias=static_cast<std::int32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(map_.aliases[character])));
    const auto region=map_.regions[index];int straight=0,diagonal=0;
    // 짝수는 직선/홀수는 대각선이며 실제 이웃별 bit표로 두 mask를 각각 누적한다.
    for (std::size_t neighbor=0;neighbor<kNeighborCells.size();++neighbor) {
        const auto [dx,dy]=kNeighborCells[neighbor];
        if (Same(x+dx,y+dy,alias,region)) (neighbor%2 ? diagonal : straight)|=kNeighborBits[neighbor];
    }
    const auto type=map_.typeMapping[character];
    if (type<0 || static_cast<std::size_t>(type)>=frames_.size()) throw std::out_of_range("초기stamp frame 타입 범위 오류");
    const auto side=TerrainBuilder::Orientation(straight),variant=TerrainBuilder::Orientation(diagonal);Initialize();
    std::int32_t frame;
    if (code=='x' && side=='A' && variant=='A') {
        const auto& values=*state_.variants;const auto sum=(values[static_cast<std::size_t>((x/3)%99)]+values[static_cast<std::size_t>((y/3)%99)])%99;
        if (sum<0) throw std::out_of_range("초기stamp cache 합계 범위 오류");
        const auto chosen=(values[static_cast<std::size_t>(sum)]+y/3)%4;
        frame=frames_[static_cast<std::size_t>(type)].Run('J').first+1+(y%3+chosen*3)*3+x%3;
    } else frame=ChooseTerrainFrame(frames_[static_cast<std::size_t>(type)],{static_cast<std::uint8_t>(side),static_cast<std::uint8_t>(variant),0,0},
        pool_.Edition()==OriginalEdition::Patch1078 ? 0 : -1,random_);
    return frame==-1 ? 0 : frame;
}
// 원본 직접 대입 폭은 패치DWORD/CD BYTE이며 SetFrame 부수효과를 만들지 않는다.
void RawTerrainStamp::Frame(Sid sid,std::int32_t frame) const {
    auto raw=pool_.AllocatedBytes(sid);const auto bits=static_cast<std::uint32_t>(frame);
    const std::size_t offset=pool_.Edition()==OriginalEdition::Patch1078 ? 36 : 34,width=pool_.Edition()==OriginalEdition::Patch1078 ? 4 : 1;
    // 실제 필드 폭의 바이트만 little endian으로 쓴다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(bits>>(index*8));
}
// factory 뒤 현재 타입을 캡처하고 Pop 이후 현재 지역/frameflag를 조회해 fringe를 원본 순서로 만든다.
void RawTerrainStamp::Create(std::int32_t x,std::int32_t y,std::uint32_t type,std::int32_t frame,std::int32_t region) {
    const auto sid=hooks_.create(type,map_.makePredictable);map_.lastSid=sid;hooks_.setOwner(sid,0);Frame(sid,frame);
    const auto actualType=pool_.Slot(sid)[10];
    if (actualType>=types_.size()) throw std::out_of_range("초기stamp 생성 타입 범위 오류");
    hooks_.pop(sid,static_cast<float>(x),static_cast<float>(y),kPopFlags);
    if (!(types_[actualType].flags2&2U)) return;
    if (region==state_.noRegion) {
        if (pool_.Edition()==OriginalEdition::Patch1078) hooks_.assertion(TerrainStampAssertion::MissingRegion);
        region=hooks_.chunkRegion(x/16,y/16);
    }
    hooks_.setRegion(sid,region);const auto codes=frames_[actualType].Codes();
    if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("초기stamp 생성 프레임 범위 오류");
    const auto code=codes[static_cast<std::size_t>(frame)];if (!(code.flags&1U)) return;
    if (state_.fringeType>=frames_.size()) throw std::out_of_range("초기stamp fringe 타입 범위 오류");
    std::int32_t selected;
    // 원본은 후보flags를 −1 판정 전에 읽으며 제외flags가 겹치면 난수를 다시 소비한다.
    do {
        selected=ChooseTerrainFrame(frames_[state_.fringeType],code,-1,random_);
        const auto flags=selected==-1 ? state_.fringePrefixFlags : frames_[state_.fringeType].Codes()[static_cast<std::size_t>(selected)].flags;
        if (!(map_.excludedFlags&SignedFlags(flags))) break;
    } while (true);
    if (selected==-1) return;
    if (!map_.makePredictable) hooks_.assertion(TerrainStampAssertion::UnpredictableFringe);
    const auto fringe=hooks_.create(state_.fringeType,map_.makePredictable);Frame(fringe,selected);
    hooks_.pop(fringe,static_cast<float>(x),static_cast<float>(y)+4.0F,kPopFlags);
}
// 입력 문자별 alias를 현재 읽고 실제 mapping 세 항목을 생성 직전에 다시 지정한다.
void RawTerrainStamp::Stamp(std::int8_t code) {
    // Y외부/X내부 반열린 순서는 난수와 factory/Pop 호출의 순서를 결정한다.
    for (auto y=map_.bounds[1];y<map_.bounds[3];++y) {
        // 각 행의 left/right도 현재 context에서 다시 읽는다.
        for (auto x=map_.bounds[0];x<map_.bounds[2];++x) {
            const auto index=Index(x,y);const auto character=Character(map_.codes[index]);
            if (x<map_.bounds[0] || y<map_.bounds[1] || x>map_.bounds[2] || y>map_.bounds[3] || map_.aliases[character]!=code) continue;
            map_.typeMapping['x']=static_cast<std::int32_t>(state_.islandType);map_.typeMapping['X']=static_cast<std::int32_t>(state_.edgeType);
            map_.typeMapping['=']=static_cast<std::int32_t>(state_.islandType);
            const auto type=map_.typeMapping[character];if (!type) continue;
            const auto region=static_cast<std::int32_t>(std::bit_cast<std::int8_t>(map_.regions[index]));
            const auto frame=FrameAt(x,y);Create(x,y,static_cast<std::uint32_t>(type),frame,region);
        }
    }
}
// 연결하려는 월드의 같은 pool인지 확인할 때 현재 저장소를 반환한다.
const SidPool& RawTerrainStamp::Pool() const { return pool_; }
}
