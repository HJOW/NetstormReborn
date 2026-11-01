// 두 PE의 실제 초기 시야 선택/좌표/억제 깊이·요청/변환 번호를 전수 대조한다.
#include "RawSceneSupport.h"
#include "client/CameraHome.h"
#include "client/ClientCameraGlobals.h"
#include "CameraTestDevices.h"
#include <bit>
#include <sstream>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 원본 실행기에 넣은 동일 TYPE 번호/genus/footX/footY의 물리 입력이다.
constexpr std::array<std::array<std::uint32_t,4>,5> kCameraTypes{{{70,0x200,3,6},{71,0x210000,1,1},{72,0x200000,3,2},{73,0x10000,6,4},{74,0,1,1}}};
// 실제 SID 풀의 읽기 자료만 준비하며 자유 목록 조작/객체 게임 동작은 실행하지 않는다.
void CheckCameraHome(OriginalEdition edition) {
    SidPool pool(edition,24000,false);std::vector<RiftTypeRecord> types(96); // 원본 입력에는5..127만 사용하며 나머지는 실제 free 슬롯이다.
    std::vector<std::span<std::uint8_t>> raw;std::size_t count{}; // 먼저 확보한 raw 입력 슬롯과 대조 개수다.
    // 물리 입력을 만들 때만 슬롯을 확보한다. 행마다 바꾸는 free 비트는 이 읽기 전용 검사 밖에서 사용하지 않는다.
    for (std::uint16_t sid=5;sid<128;++sid) raw.push_back(pool.AllocatedBytes(pool.Allocate(2)));
    // 실행기와 같은 타입 표의 현재 발자국/genus를 설정한다.
    for (const auto& input:kCameraTypes) { auto& type=types[input[0]];type.flags2=input[1];type.footX=static_cast<std::int32_t>(input[2]);type.footY=static_cast<std::int32_t>(input[3]); }
    // fixture의 raw 슬롯 입력을 그대로 읽으며 기대 선택이나 좌표를 계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_CAMERA_HOME_FIXTURE).rows) {
        if (row[0]!=(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")) continue;
        // 앞선 행의 입력 바이트를 없애고 실제 free 상태를 만든다.
        for (auto slot:raw) { std::fill(slot.begin(),slot.end(),std::uint8_t{});slot[11]=1; }
        if (row[2]!="-") {
            // 실제 PE에 공급한 번호/타입/상태/소유자/좌표 DWORD를 같은 위치에 쓴다.
            for (const auto& encoded:Split(row[2],';')) {
                const auto object=Split(encoded,',');auto slot=raw.at(Number(object[0])-5); // 이 입력은 원본과 같은5..127 번호다.
                slot[10]=static_cast<std::uint8_t>(Number(object[1]));slot[11]=static_cast<std::uint8_t>(Number(object[2]));
                slot[edition==OriginalEdition::Patch1078 ? 34 : 32]=static_cast<std::uint8_t>(Number(object[3]));Put(slot,14,Number(object[4]));Put(slot,18,Number(object[5]));
            }
        }
        CameraHomeState state; // 원본 함수에 넣은 현재 모드/전체 로컬 번호/장치 크기/카운터다.
        state.fort=Number(row[3])!=0;state.battle=Number(row[4])!=0;state.challenge=Number(row[5])!=0;state.localPlayer=Number(row[6]);
        state.width=std::stoi(row[7]);state.height=std::stoi(row[8]);state.suppressionDepth=Number(row[9]);state.requests=Number(row[10]);state.revision=Number(row[11]);
        state.challengeType=74;state.worldWidth=4097;state.worldHeight=2819;
        std::ostringstream applied; // 같은 commit 위치의 원점/현재 카운터 관찰이다.
        CameraHome logic(pool,types,state,netstorm::test::camera::MakePointDevice(state,[&](std::int32_t x,std::int32_t y) {
            if (!applied.str().empty()) applied<<'|';applied<<x<<','<<y<<','<<state.suppressionDepth<<','<<state.requests<<','<<state.revision;
        }));
        logic.Reset();CHECK((applied.str().empty() ? "-" : applied.str())==row[12]);
        CHECK(state.suppressionDepth==Number(row[13]) && state.requests==Number(row[14]) && state.revision==Number(row[15]));++count;
    }
    CHECK(count==1182);
}
}
// PATCH의 실제 번호순/전체 로컬 DWORD/NaN ftol/마지막 dead 및 void 후보를 대조한다.
TEST_CASE(CameraHome_PatchNative) { CheckCameraHome(OriginalEdition::Patch1078); }
// CD의 직접 반복자/별도 ftol/화면 중심 helper도 같은 독립 원본과 대조한다.
TEST_CASE(CameraHome_CdNative) { CheckCameraHome(OriginalEdition::Cd1072); }
// 원본433ad0의 실제 토큰 분리/x87 결과를 같은 입력으로 재생한다. 기대 좌표를 여기서 계산하지 않는다(단위676).
TEST_CASE(ViewSpot_PatchNative) {
    std::size_t count{}; // 고정된 입력 행이 빠지지 않았는지 끝에서 확인한다.
    // 입력은 탭/따옴표를 표 구분자와 분리한 16진수이며 '-'는 빈 문자열이다.
    for (const auto& row:LoadFixture(NETSTORM_VIEWSPOT_FIXTURE).rows) {
        if (row[0]=="input_hex") continue;
        std::string input; // 원본 입력 메모리에 쓴 것과 같은 바이트다.
        // 두 글자씩 한 바이트로 복원한다.
        for (std::size_t offset=0;row[0]!="-" && offset<row[0].size();offset+=2) input.push_back(static_cast<char>(std::stoul(row[0].substr(offset,2),nullptr,16)));
        const auto spot=ParseViewSpot(input);
        CHECK(std::bit_cast<std::uint32_t>(spot.x)==std::stoul(row[1],nullptr,16));
        CHECK(std::bit_cast<std::uint32_t>(spot.y)==std::stoul(row[2],nullptr,16));++count;
    }
    CHECK(count==34);
}
// 실제 RunForIt 인자가 즉시 이동 부모의 목표/두 카운터/적용 호출까지 이어지는지 확인한다(단위676).
TEST_CASE(ViewSpot_CentersThroughImmediateParent) {
    ClientCameraGlobals camera;std::uint32_t worldRedraw{},revision{};int commits{}; // 원문433aa0이 쓰는 두 전역과 497960 호출 수다.
    const auto spot=ParseViewSpot("(139x88)");
    camera.CenterImmediately(spot.x,spot.y,1024,768,worldRedraw,revision,[&] { ++commits; });
    CHECK(camera.Target().pixels[0]==1712 && camera.Target().pixels[1]==584);
    CHECK(worldRedraw==1 && revision==1 && commits==1);
}
// 화면 적용 예외가 이후 Carrier 표시를 계속 억제하지 않게 깊이를 복구한다.
TEST_CASE(CameraHome_DisplayFailureRestoresGuard) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);std::vector<RiftTypeRecord> types(96);CameraHomeState state;state.suppressionDepth=3;
    CameraHome logic(pool,types,state,netstorm::test::camera::MakePointDevice(state,[](std::int32_t,std::int32_t) { throw std::runtime_error("명시 장치 실패 입력"); }));bool failed=false; // 실제433aa0 뒤 외부 화면 실패만 입력한다.
    try { logic.Reset(); } catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && state.suppressionDepth==3 && state.requests==1 && state.revision==1);
}
