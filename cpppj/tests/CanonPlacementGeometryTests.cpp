// 세 실제 PE의 모든 패턴별 finder 인자/순회와 사제 공통 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPlacementGeometry.h"
#include "o/RawSquidFinder.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 실제 타입 전역의 패턴 번호와 마지막 비패턴 사제 번호다.
constexpr std::array<std::uint32_t,9> kTypes{107,82,94,157,131,129,140,142,158};
// 기대 좌표를 계산하지 않고 기계어 입력의 코드 순서만 구성한다.
RiftTypeFrames InputFrames(int profile) {
    std::vector<FrameCode> codes;
    // 정상 방향 A..P와 원본 셀의 번호 영역을 모두 공급한다.
    for (int side='A';side<='P';++side)
        // 번호 10도 독립 메타 입력에 포함한다.
        for (int number=0;number<=10;++number) if (profile!=2) codes.push_back({static_cast<std::uint8_t>(side),'P',static_cast<std::uint8_t>(number),0});
    if (profile==1) std::reverse(codes.begin(),codes.end());
    if (profile!=2) codes.push_back(codes.front());
    return RiftTypeFrames(std::move(codes));
}
// 기본 지역 경계는 순회 검사에서만 명시적으로 허용한다. 실제 지형 정책을 대신하지 않는다.
CanonPlacementGeometryHooks AllowRegions() {
    return {[](const CanonTypeQuery&) {},{},[](const CanonTypeQuery&,const CanonPlacementCell&) { return true; },[](const CanonTypeQuery&) { return true; }};
}
// 실제 명령 fixture의 모든 칸과 초기 범위를 공통 순회에 대조한다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,true);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());PriestPlacementGeometryState state;
    RawCanonPlacementGeometry geometry(pool,types,frames,state);const auto rows=LoadFixture(NETSTORM_CANONGEOMETRY_FIXTURE).rows;std::size_t count=0;
    // 입력의 패턴 번호/별칭/현재 발자국과 프레임 메타를 독립적으로 구성한다.
    for (const auto& row:rows) {
        if (row[0]!=edition) continue;CHECK(row.size()==15);const auto type=kTypes[Number(row[1])];
        std::copy_n(kTypes.begin(),8,state.patternTypes.begin());if (row[10]=="1") state.patternTypes[0]=state.patternTypes[1];if (row[10]=="2") state.patternTypes[2]=state.patternTypes[4];
        frames[type].frames=InputFrames(std::stoi(row[4]));frames[type].defaultFrame=std::stoi(row[11]);
        const int wx=std::stoi(row[7]),wy=std::stoi(row[8]);types[type].footX=wx;types[type].footY=wy;
        const CanonTypeQuery query{type,std::stoi(row[2]),std::stoi(row[3]),std::bit_cast<float>(Number(row[5])),std::bit_cast<float>(Number(row[6])),row[9]!="0"};
        const auto bounds=geometry.Bounds(query);const auto expectedBounds=Split(row[13],',');
        CHECK(bounds.left==std::stoi(expectedBounds[0]) && bounds.top==std::stoi(expectedBounds[1]) && bounds.right==std::stoi(expectedBounds[2]) && bounds.bottom==std::stoi(expectedBounds[3]));
        const auto expected=row[14]=="-" ? std::vector<std::string>{} : Split(row[14],';');std::size_t ordinal=0;int begun=0,ended=0,finished=0;
        auto hooks=AllowRegions();hooks.beginRegions=[&](const CanonTypeQuery&) { ++begun; };hooks.endShape=[&](const CanonTypeQuery&,const CanonPlacementCell&) { ++ended;return true; };hooks.finishRegions=[&](const CanonTypeQuery&) { ++finished;return true; };
        CHECK(geometry.Inspect(query,[&](const CanonPlacementCell& cell) {
            CHECK(ordinal<expected.size());if (ordinal>=expected.size()) return false;const auto fields=Split(expected[ordinal],',');CHECK(fields.size()==11);
            CHECK(cell.frame==std::stoi(fields[0]) && std::bit_cast<std::uint32_t>(cell.x)==Number(fields[1]) && std::bit_cast<std::uint32_t>(cell.y)==Number(fields[2]));
            CHECK(cell.label==std::stoi(fields[3]) && cell.side==Number(fields[4]));
            CHECK(std::bit_cast<std::uint32_t>(cell.snappedX)==Number(fields[5]) && std::bit_cast<std::uint32_t>(cell.snappedY)==Number(fields[6]));
            CHECK(cell.area.left==std::stoi(fields[7]) && cell.area.top==std::stoi(fields[8]) && cell.area.right==std::stoi(fields[9]) && cell.area.bottom==std::stoi(fields[10]));
            ++ordinal;
            // 다음 칸의 발자국 변경은 기대 사각형이 아닌 원본 분석 입력과 동일한 메타 변경이다.
            types[type].footX=wx+(row[12]!="0" ? static_cast<int>(ordinal%3) : 0);
            types[type].footY=wy+(row[12]!="0" ? static_cast<int>((ordinal+1)%3) : 0);
            return true;
        },hooks));
        CHECK(ordinal==expected.size() && begun==1 && ended==static_cast<int>(expected.size()) && finished==1);++count;
    }
    CHECK(count==(patch ? 3159U : 1404U));
}
}
// 10.78의 전체 패턴/홀수·signed 회전/각 칸의 실제 finder 인자를 대조한다.
TEST_CASE(canon_geometry_patch_x86) { Replay("originals"); }
// CD의 실제 snap/CRT 저장/사각형 계산을 별도 원본 관찰과 대조한다.
TEST_CASE(canon_geometry_cd_x86) { Replay("originalCD"); }
// 별도 10.37 PE의 모든 모양 관찰도 독립적으로 대조한다.
TEST_CASE(canon_geometry_1037_x86) { Replay("original1037"); }
// 다중 칸의 준비/후처리/조기 거부 순서와 빈 모양의 최종 호출을 검사한다.
TEST_CASE(canon_geometry_region_order_rejection_and_empty) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> frames(188);PriestPlacementGeometryState state;
    std::copy_n(kTypes.begin(),8,state.patternTypes.begin());types[157].footX=types[157].footY=1;frames[157].frames=InputFrames(0);
    RawCanonPlacementGeometry geometry(pool,types,frames,state);CanonTypeQuery query{157,0,0,20.5f,21.5f,false};std::vector<std::string> events;int scanned=0;
    auto hooks=AllowRegions();hooks.beginRegions=[&](const CanonTypeQuery& q) { CHECK(q.x==20.5f);events.push_back("begin");query.x=99; };
    hooks.beginShape=[&](const CanonTypeQuery& q,const CanonPlacementCell&) { CHECK(q.x==20.5f);events.push_back("prepare"); };
    hooks.endShape=[&](const CanonTypeQuery&,const CanonPlacementCell&) { events.push_back("end");return true; };
    hooks.finishRegions=[&](const CanonTypeQuery&) { events.push_back("finish");return true; };
    CHECK(!geometry.Inspect(query,[&](const CanonPlacementCell&) { events.push_back("scan");return ++scanned!=2; },hooks));
    CHECK(events==std::vector<std::string>({"begin","prepare","scan","end","prepare","scan"}));
    events.clear();scanned=0;query.x=20.5f;hooks.endShape=[&](const CanonTypeQuery&,const CanonPlacementCell&) { events.push_back("end");return false; };
    CHECK(!geometry.Inspect(query,[&](const CanonPlacementCell&) { ++scanned;events.push_back("scan");return true; },hooks));CHECK(scanned==1 && events.back()=="end");
    query={70,70,0,20.5f,21.5f,false};types[70].footX=types[70].footY=1;frames[70].defaultFrame=-1;events.clear();
    CHECK(geometry.Inspect(query,[](const CanonPlacementCell&) { CHECK(false);return true; },hooks));CHECK(events==std::vector<std::string>({"begin","finish"}));
    frames[70].frames=InputFrames(0);frames[70].defaultFrame=2;hooks=AllowRegions();hooks.beginRegions=[&](const CanonTypeQuery&) { frames[70].defaultFrame=-1; };
    CHECK(geometry.Inspect(query,[](const CanonPlacementCell& cell) { CHECK(cell.frame==2);return true; },hooks));
}
// 실제 해시/finder가 패턴 모양의 차례대로 후보를 찾고 거부 뒤의 지형 판정을 생략한다.
TEST_CASE(canon_geometry_pattern_real_finder_rejection) {
    // 두 판본의 실제 SID 슬롯/해시 구조를 사용하며 충돌 정책만 명시적으로 거부한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());PriestPlacementGeometryState state;
        std::copy_n(kTypes.begin(),8,state.patternTypes.begin());types[157].footX=types[157].footY=1;types[83].footX=types[83].footY=1;frames[157].frames=InputFrames(0);
        const auto blocker=pool.Allocate();auto raw=pool.AllocatedBytes(blocker);raw[10]=83;Put(raw,14,std::bit_cast<std::uint32_t>(21.0f));Put(raw,18,std::bit_cast<std::uint32_t>(22.0f));hash.Bucket(1,21,22)=blocker.value;
        RawCanonPlacementGeometry geometry(pool,types,frames,state);RawSquidFinder finder(pool,hash,types);int scanned=0,ended=0,finished=0;auto hooks=AllowRegions();
        hooks.endShape=[&](const CanonTypeQuery&,const CanonPlacementCell&) { ++ended;return true; };hooks.finishRegions=[&](const CanonTypeQuery&) { ++finished;return true; };
        CHECK(!geometry.Inspect({157,0,0,20.5f,21.5f,false},[&](const CanonPlacementCell& cell) { ++scanned;return !finder.Begin(cell.area).value; },hooks));
        CHECK(scanned>0 && scanned<=9 && ended==scanned-1 && finished==0 && finder.State().current==blocker);
    }
}
// 원본 assert/배열 밖 입력과 누락된 지형 판정은 C++ 계약 오류로 구별한다.
TEST_CASE(canon_geometry_guards) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> frames(188);PriestPlacementGeometryState state;
    RawCanonPlacementGeometry geometry(pool,types,frames,state);auto hooks=AllowRegions();const CanonTypeQuery query{70,70,0,20,21,false};
    CHECK(Throws([&] { RawCanonPlacementGeometry bad(pool,types,{},state); }));CHECK(Throws([&] { geometry.Bounds({188,0,0,0,0,false}); }));
    CHECK(Throws([&] { geometry.Inspect(query,{},hooks); }));CHECK(Throws([&] { geometry.Inspect(query,[](const CanonPlacementCell&) { return true; },{}); }));
    // 모양 계산에는 방향 코드가 필요 없다. 원본 기본 프레임의 개수 검사 생략을 보존한다.
    frames[70].defaultFrame=4;types[70].footX=types[70].footY=1;
    CHECK(geometry.Inspect(query,[](const CanonPlacementCell& cell) { CHECK(cell.frame==4 && cell.side==0);return true; },hooks));
    CHECK(Throws([] { RawCanonPlacementGeometry::CollisionArea(20,21,-1,1); }));CHECK(Throws([] { RawCanonPlacementGeometry::CollisionArea(std::numeric_limits<float>::infinity(),21,1,1); }));
}
