// 원문 입력 부모의 독립fixture를 재생하는 집중 검사다.
#include "RawSceneSupport.h"
#include "client/EditorIslandDrag.h"
#include "client/EditorIslandDialog.h"
#include <sstream>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { //새 섬 창 검사 전용 장치이며 실제 GUI 검증을 대신하지 않는다.
//현재 관측 장치의 float32비트를 그대로 문자열에 쓴다.
std::string IslandDragBits(float value) { return std::to_string(std::bit_cast<std::uint32_t>(value)); }
}
//원문 전체40관측의 상태·좌표·부분효과·수명 호출순서를 그대로 비교한다.
TEST_CASE(EditorIslandDrag_ReplaysActualNativeParent) {
    const auto fixture=LoadFixture(NETSTORM_EDITOR_ISLAND_DRAG_FIXTURE);std::size_t checked=0;
    for (const auto& row:fixture.rows) { //표제와 같은입력의 두 x87 관측을 구분한다.
        if (row[0]=="name") continue;
        EditorIslandDragState state;state.flags=0xa50300;state.dirty=0;state.drag=Number(row[4]) ? 1U : 0U;state.sourceIndex=88;state.record={9,3,0,37,41,43};
        for (std::size_t byte=0;byte<126;++byte) state.choices[byte]=static_cast<std::uint8_t>(byte*7+3); //Native의 입력126BYTE이며 출력기대는fixture다.
        std::vector<std::string> trace;std::int32_t selection=5;std::uint32_t revision=77;
        const auto log=[&](std::string value) { trace.push_back(std::move(value)); }; //장치의 실제 호출순서만 기록한다.
        EditorIslandDragHooks hooks;
        hooks.closeSerial=[] { return 777U; };hooks.killParent=[&] { log("killParent"); };hooks.pushParent=[&] { log("push"); };
        hooks.assertion=[&](const char* text) { CHECK(std::string_view(text)=="dragIsland == NULL");log("assert:5087"); };
        hooks.image=[&](auto index) { log("image:"+std::to_string(index));return Number(row[6]) ? 17U : 0U; };
        hooks.selection=[&] { return selection; };hooks.recordIndex=[&](auto index) { log("index:"+std::to_string(index));return 3; };
        hooks.createDrag=[&](auto image) { CHECK(image==17);log("new");log("construct");return 1U; };
        hooks.adjustDrag=[&](auto) { log("adjust"); };hooks.killDrag=[&](auto) { log("killDrag"); };
        hooks.size=[&](auto) { return std::array<float,2>{std::stof(row[7]),std::stof(row[8])}; };
        hooks.attachDrag=[&](auto,float x,float y) { log("attach:"+IslandDragBits(x)+':'+IslandDragBits(y)); };
        hooks.parentBounds=[&] { log("bounds:parent");return std::array<std::int32_t,4>{0,0,432,412}; };
        hooks.listBounds=[&] { log("bounds:list");return std::array<std::int32_t,4>{0,0,20,300}; };
        hooks.mapBounds=[&] { log("bounds:map");return std::array<std::int32_t,4>{100,100,324,324}; };
        hooks.mapSize=[] { return std::array<std::int32_t,2>{14,14}; };
        hooks.drop=[&](auto record) {
            log("can:6:"+std::to_string(static_cast<std::uint32_t>(record[0])|(static_cast<std::uint32_t>(record[1])<<8U)|(static_cast<std::uint32_t>(record[2])<<16U)));
            if (!Number(row[5])) return false;
            record[1]&=0xfdU;log("add:"+Hex(record)+":4:1:1:15:15:1");return true;
        }; //월드 큰 하위의 명시 장치이며 실제World 검사는 별도다.
        hooks.camera=[&](float x,float y) { log("camera:"+IslandDragBits(x)+':'+IslandDragBits(y)); };
        hooks.releaseImage=[&](auto image) { CHECK(image==17);log("releaseImage");log("freeImage"); };
        hooks.remove=[&](auto index) { log("remove:"+std::to_string(index)); };
        hooks.clearSelectionAndUpdate=[&] { selection=-1;log("update:0"); };hooks.incrementRevision=[&] { ++revision; };
        const auto result=HandleEditorIslandDrag(state,{Number(row[1]),std::stoi(row[2]),std::stoi(row[3])},hooks);
        CHECK(result==static_cast<bool>(Number(row[10])) && state.flags==Number(row[11]) && state.dirty==Number(row[12]) && static_cast<bool>(state.drag)==(row[13]=="True"));
        CHECK(state.sourceIndex==static_cast<std::int32_t>(Number(row[14])) && Hex(state.record)==row[15] && revision==Number(row[16]));
        std::string actual;
        for (const auto& event:trace) { if (!actual.empty()) actual+=';';actual+=event; } //fixture의 원문 호출순서를 정렬하지 않는다.
        CHECK((actual.empty() ? "-" : actual)==row[17]);++checked;
    }
    CHECK(checked==40);
}
//메뉴 release와 선택된 실제 전용창 미복원 경계는 섬/난수 효과를 만들지 않는다.
TEST_CASE(EditorIslandDialog_ActualCommandReleaseAndRequiredFactory) {
    std::uint32_t calls=0;
    CHECK(DispatchEditorIslandDialog({kAddIslandCommand|InputCode::kRelease,42,99},[&] { ++calls; }) && calls==0);
    CHECK(!DispatchEditorIslandDialog({0x02000023U,42,99},[&] { ++calls; }) && calls==0);
    CHECK(DispatchEditorIslandDialog({kAddIslandCommand,42,99},[&] { ++calls; }) && calls==1);
    CHECK(Throws([&] { static_cast<void>(DispatchEditorIslandDialog({kAddIslandCommand,42,99},{})); }));
}
