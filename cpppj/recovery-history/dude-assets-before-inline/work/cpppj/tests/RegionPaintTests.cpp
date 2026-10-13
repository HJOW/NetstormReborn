// 실제 두 PE의 도장 출력과 같은 raw 슬롯/지형 getter/게임 난수를 전수 재생한다.
#include "RawSceneSupport.h"
#include "o/RawRegionPaint.h"
#include "o/RawCanonPlacementTerrain.h"
#include <sstream>

namespace {
using namespace netstorm::test::rawscene;
// 원본 실행기에 제공한 실제 타입 번호와 좌표 입력이다.
constexpr std::uint32_t kTerrain=83,kEdge=84,kOther=85;
constexpr std::array<CellPoint,6> kPoints{{{10,10},{11,10},{10,11},{11,11},{12,10},{12,11}}};
// 지도 포함 항목만 지정하며 기대 결과는 독립 fixture에서 읽는다.
const std::array<std::vector<int>,6> kScenes{{{},{0},{0,1,4},{0,1,2,3},{0,1,2,3,4,5},{0,1,2,3,4,5}}};
// 원본 entry가 읽을 합성 live/free 슬롯을 만들며 생성이 완료됐다는 의미는 아니다.
std::span<std::uint8_t> InputRaw(SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
}
// 테스트 입력 프레임은 AA와144개 JA 구간 및 비연결 PP다.
std::vector<FrameCode> Codes() {
    std::vector<FrameCode> result{{'A','A',0,0}};
    // 실제 J 시작 번호1과 원소별36프레임 구간을 입력으로 제공한다.
    for (int i=0;i<144;++i) result.push_back({'J','A',static_cast<std::uint8_t>(i),0});
    // 비연결 PP도 유효한 네 quartile의12개 후보를 공급한다.
    for (int i=0;i<12;++i) result.push_back({'P','P',static_cast<std::uint8_t>(i),0});return result;
}
// 콜백과 필드 출력을 fixture와 같은 구분자로 연결한다.
void Append(std::string& text,std::string value,char separator=';') {
    if (!text.empty()) text+=separator;text+=std::move(value);
}
// 원본으로 기록한 모든 입력을 해당 판본의 실제 모듈에 재생한다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";const auto rows=LoadFixture(NETSTORM_REGIONPAINT_FIXTURE).rows;std::size_t count=0;
    // 각 행은 서로 다른 새 drawer/cache/map으로 실행한다.
    for (const auto& row:rows) {
        if (row[0]!=edition) continue;
        CHECK(row.size()==14);if (row.size()!=14) return;
        const auto scene=Number(row[1]),variant=Number(row[2]);SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
        // 실제 지역 getter는 판본의 전체188/171개 타입 표를 요구한다.
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<RiftTypeFrames> frames;std::vector<PriestPlainCanonType> plain(types.size());const auto codes=Codes();
        // 전용 타입 표도 같은 원본 프레임 입력을 소유한다.
        for (std::size_t i=0;i<types.size();++i) frames.emplace_back(codes);
        types[kTerrain].flags2=2;std::vector<std::uint16_t> islands(65536);const std::array<std::uint32_t,3> initialFrames{0,145,3};
        // 출력에서 읽을 미사용 슬롯도 원본 합성 pool처럼0으로 초기화한다.
        for (const auto number:{50,51,52,53,54,55,70,71}) { auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(number)});std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        // 선행 소유자·장벽·잘못된 시작 타입을 입력으로만 작성한다.
        for (const auto index:kScenes[scene]) {
            const auto point=kPoints[static_cast<std::size_t>(index)];const Sid sid{static_cast<std::uint16_t>(50+index)};auto raw=InputRaw(pool,sid);
            raw[10]=static_cast<std::uint8_t>(scene==5 && index==0 ? kOther : kTerrain);raw[patch ? 34 : 32]=static_cast<std::uint8_t>((index%3+variant)%3);
            Put(raw,8,point.x<12 ? 7U : 9U,2);Put(raw,14,std::bit_cast<std::uint32_t>(point.x+0.125F));Put(raw,18,std::bit_cast<std::uint32_t>(point.y+0.25F));Put(raw,patch ? 36 : 34,initialFrames[variant],patch ? 4 : 1);
            islands[static_cast<std::size_t>(point.y*256+point.x)]=sid.value;
        }
        const std::map<std::pair<int,int>,Sid> fallback{{{9,10},Sid{70}},{{12,12},Sid{71}}};
        // edgeFarm은 표면 지도 밖 일반 finder 후보로만 공급한다.
        for (const auto& [point,sid]:fallback) {
            auto raw=InputRaw(pool,sid);raw[10]=kEdge;raw[patch ? 34 : 32]=2;Put(raw,14,std::bit_cast<std::uint32_t>(point.first+0.125F));Put(raw,18,std::bit_cast<std::uint32_t>(point.second+0.25F));
        }
        CanonPlacementTerrainState terrainState;RawCanonPlacementTerrain terrain(pool,types,plain,islands,terrainState,{
            [](const CanonPlacementQuery&,Sid)->bool { throw std::logic_error("도장 재생에 배치 권한 호출"); },
            [](const CanonPlacementQuery&,const CanonPlacementCell&)->bool { throw std::logic_error("도장 재생에 주변 배치 호출"); },
            [](const CanonPlacementQuery&,CanonPlacementTerrainState&)->bool { throw std::logic_error("도장 재생에 최종 배치 호출"); }});
        GameRandom random(0x23456789);RegionPaintState state;state.owner=6;state.theme=3;state.revision=0xffffffff;
        RegionOwnershipState ownership;ownership.affected.push_back({1.0F,2.0F,7});std::string events;
        RegionPaintHooks hooks;
        hooks.setOwner=[&](Sid sid,std::uint32_t owner) { Append(events,"O:"+std::to_string(sid.value)+":"+std::to_string(owner));pool.AllocatedBytes(sid)[patch ? 34 : 32]=static_cast<std::uint8_t>(owner); };
        hooks.boundaryAt=[&](float x,float y,std::uint32_t type) {
            CHECK(type==kEdge);const auto found=fallback.find({static_cast<int>(x),static_cast<int>(y)});const auto sid=found==fallback.end() ? Sid{} : found->second;
            Append(events,"F:"+std::to_string(static_cast<int>(x))+":"+std::to_string(static_cast<int>(y))+":"+std::to_string(sid.value));return sid;
        };
        hooks.regionAt=[&](float x,float y) { return terrain.RegionAt(static_cast<int>(x),static_cast<int>(y)); };
        RawRegionPaint paint(pool,frames,islands,kTerrain,kEdge,random,state,ownership,std::move(hooks));
        paint.Paint({std::stoi(row[5]),std::stoi(row[6]),Number(row[3]),Number(row[4])});
        std::string objects,boundary,affected,drawer;
        // 슬롯 출력을 원본 실행기가 기록한 동일 폭으로 읽는다.
        for (const auto number:{50,51,52,53,54,55,70,71}) { const auto raw=pool.Slot(Sid{static_cast<std::uint16_t>(number)});Append(objects,std::to_string(number)+":"+std::to_string(raw[patch ? 34 : 32])+":"+std::to_string(Get(raw,patch ? 36 : 34,patch ? 4 : 1)),','); }
        // 후보 삽입 순서와 전체 영향 기록 비트를 그대로 비교한다.
        for (const auto point:state.boundary) Append(boundary,std::to_string(point.x)+":"+std::to_string(point.y),',');
        if (patch) for (const auto& value:ownership.affected) Append(affected,std::to_string(std::bit_cast<std::uint32_t>(value.x))+":"+std::to_string(std::bit_cast<std::uint32_t>(value.y))+":"+std::to_string(value.region),',');
        // 생성자/entry의 float 비트와 마지막 유효 요청도 모두 대조한다.
        for (const auto value:state.bounds) Append(drawer,std::to_string(std::bit_cast<std::uint32_t>(value)),':');
        Append(drawer,std::to_string(state.owner),':');Append(drawer,std::to_string(static_cast<std::uint32_t>(state.theme)),':');
        const bool same=(events.empty() ? "-" : events)==row[7] && objects==row[8] && (boundary.empty() ? "-" : boundary)==row[9] &&
            (affected.empty() ? "-" : affected)==row[10] && drawer==row[11] && state.revision==Number(row[12]) && (patch ? random.State() : 0U)==Number(row[13]);
        CHECK(same);if (!same) { std::printf("도장 %s 행%zu scene%u/variant%u 불일치\n",std::string(edition).c_str(),count,scene,variant);std::printf("events %s / %s\nslots %s / %s\nqueue %s / %s\naffected %s / %s\n",events.c_str(),row[7].c_str(),objects.c_str(),row[8].c_str(),boundary.c_str(),row[9].c_str(),affected.c_str(),row[10].c_str());break; }
        ++count;
    }
    CHECK(count==(patch ? 386U : 98U));
}
}
// 패치의 실제 frame/경계 확장/고유 지역 기록과 두 난수 경로를 대조한다.
TEST_CASE(RegionPaint_ReplaysOriginals) { Replay("originals"); }
// CD는 프레임이나 후보를 바꾸지 않고 소유자 flood와 revision만 수행한다.
TEST_CASE(RegionPaint_ReplaysCd) { Replay("originalCD"); }
