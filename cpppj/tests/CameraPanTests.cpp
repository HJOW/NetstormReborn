// 원문259개 관측의 실제 관문/중심거리/현재 목표와 callback 뒤 재조회를 대조한다.
#include "TestSupport.h"
#include "client/RawCameraPan.h"
#include "client/RawEdgeScroll.h"
#include "client/ClientCameraGlobals.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace netstorm::client; //같은 원문 입력과 실제 카메라 목표 형식을 검사한다.
TEST_CASE(CameraPan_Original259AndCurrentCallbacks) {
    std::ifstream file(NETSTORM_CAMERA_PAN_FIXTURE);CHECK(file.good());std::string line;std::size_t count{};
    while (std::getline(file,line)) { //각 원문 관측은 별개 전역/Renderer/스택 입력이다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);std::array<std::int64_t,20> input{};std::array<std::uint32_t,6> expected{};std::string expectedEvents;
        for (auto& value:input) row>>value; //전체 DWORD와 signed 좌표를 넓은 입력에서 읽는다.
        for (auto& value:expected) row>>value; //원문 출력은 unsigned 하위 DWORD다.
        row>>expectedEvents;CHECK(!row.fail());if (row.fail()) continue;
        const auto value=[&](std::size_t i) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(input[i])); }; //원본 비트 그대로 signed 해석한다.
        CameraPanGlobals globals{value(5),value(6),value(7),value(8),value(9),value(10),value(11),value(12),value(13)};
        ClientCameraGlobals camera;camera.RendererView().pixels={value(14),value(15)};camera.Target().pixels={value(16),value(17)};camera.Snapshot().pixels={777,888};std::uint32_t revision=static_cast<std::uint32_t>(input[18]);std::string events;
        const auto append=[&](std::string entry) { if (!events.empty()) events+=';';events+=entry; }; //호출 순서/실제 signed 인수를 누적한다.
        CameraPanHooks hooks;hooks.globals=[&] { return globals; };
        hooks.poll=[&](std::uint32_t code) {
            append("P:"+std::to_string(code));if (code==0x100000U && input[19]==2) { camera.RendererView().pixels={900,901};revision=0xffffffffU; }
            return InputEvent{code|((code==InputCode::kMiddleButton ? input[3] : input[4]) ? InputCode::kRelease : 0U),value(1),value(2)};
        }; //폴링 callback이 변경한 현재 view는 뒤497e00에서 다시 조회한다.
        hooks.closePopups=[&] { append("C");if (input[19]==1) { globals.viewX=13;globals.viewY=17;globals.viewWidth=440;globals.viewHeight=330;globals.clientWidth=880;globals.clientHeight=660; } };
        hooks.request=[&](auto x,auto y) { append("D:"+std::to_string(x)+':'+std::to_string(y));RequestCameraDelta(camera.RendererView(),camera.Target(),768,x,y,revision); };
        const auto selected=ProcessCameraPan({static_cast<std::uint32_t>(input[0]),value(1),value(2)},hooks);
        const std::array<std::uint32_t,6> actual{selected ? 1U : 0U,static_cast<std::uint32_t>(camera.RendererView().pixels[0]),static_cast<std::uint32_t>(camera.RendererView().pixels[1]),static_cast<std::uint32_t>(camera.Target().pixels[0]),static_cast<std::uint32_t>(camera.Target().pixels[1]),revision};
        CHECK(actual==expected);CHECK(events==expectedEvents);CHECK((camera.Snapshot().pixels==std::array<std::int32_t,2>{777,888}));++count;
    }
    CHECK(count==259);
}
TEST_CASE(CameraPan_OnlySelectedDevicesAndInactiveQueueSelection) {
    CameraPanGlobals current{1,0,0,1024,768,128,26,896,680};std::string events;CameraPanHooks hooks;
    hooks.poll=[&](auto code) { events+='P';return InputEvent{code|InputCode::kRelease,0,0}; };hooks.globals=[&] { return current; };
    CHECK(!ProcessCameraPan({0x80120000U,600,400},hooks));CHECK(events=="P");
    bool missing{};try { ProcessCameraPan({0x120000U,600,400},hooks); }catch (const std::logic_error&) { missing=true; }CHECK(missing);
    hooks.closePopups=[&] { events+='C'; };CHECK(ProcessCameraPan({0x120000U,600,400},hooks));CHECK(events=="PPPC"); //비활성은 팝업 정리 뒤 전용 큐를 선택하되 Shift/목표를 요구하지 않는다.
}
