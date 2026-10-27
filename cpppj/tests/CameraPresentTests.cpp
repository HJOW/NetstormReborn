// 보호 원문 후처리의 전체 목표/전역·선택된 호출과 적용 전 현재 view 보존을 검사한다.
#include "TestSupport.h"
#include "client/RawCameraPresent.h"
#include "client/ClientGameplayGlobals.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace netstorm::client; // 실제 Client 카메라 자료형과 같은 본체를 직접 실행한다.
// 입력17열과 실제 원문 출력8열·호출열을 대조하며 C++ 결과로 기대값을 만들지 않는다.
TEST_CASE(CameraPresent_MatchesNativeLimitsAndCalls) {
    std::ifstream file(NETSTORM_CAMERA_PRESENT_FIXTURE);CHECK(file.good());std::string line;std::size_t count{};
    while (std::getline(file,line)) { // 원문 관찰마다 독립 현재 전역과 목표/현재 view를 준비한다.
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::array<std::int64_t,17> input{};std::array<std::uint32_t,8> expected{};std::string expectedEvents;
        for (auto& value:input) row>>value; // signed 입력과 전체 DWORD 입력을 넓은 정수로 손실 없이 읽는다.
        for (auto& value:expected) row>>value; // 원문 출력은 하위32비트 unsigned 열이다.
        row>>expectedEvents;CHECK(!row.fail());if (row.fail()) continue;
        const auto integer=[&](std::size_t index) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(input[index])); }; // 원본 하위 DWORD의 signed 해석만 바꾼다.
        std::uint32_t diagnostic=static_cast<std::uint32_t>(input[0]);auto width=integer(1),height=integer(2),cells=integer(3);ClientCameraGlobals camera;
        auto& target=camera.Target();target.limits={integer(4),integer(5)};target.pixels={integer(6),integer(7)};target.zoom=0xabcdef01U;camera.RendererView().pixels={123,456};camera.Snapshot().pixels={789,987};
        std::string events;const auto record=[&](char code) { if (!events.empty()) events+=';';events+=std::string(1,code)+':'+std::to_string(static_cast<std::uint32_t>(target.pixels[0]))+':'+std::to_string(static_cast<std::uint32_t>(target.pixels[1])); }; // 선택된 실제 호출 위치의 현재 목표를 기록한다.
        RawCameraPresentHooks hooks;hooks.flip=[&] { record('F');if (input[8]) { diagnostic=static_cast<std::uint32_t>(input[9]);width=integer(10);height=integer(11);cells=integer(12);target.limits={integer(13),integer(14)};target.pixels={integer(15),integer(16)}; } };
        hooks.commit=[&] { record('C'); };PresentCameraTarget(target,{diagnostic,width,height,cells},hooks);
        const std::array<std::uint32_t,8> actual{diagnostic,static_cast<std::uint32_t>(width),static_cast<std::uint32_t>(height),static_cast<std::uint32_t>(cells),static_cast<std::uint32_t>(target.limits[0]),static_cast<std::uint32_t>(target.limits[1]),static_cast<std::uint32_t>(target.pixels[0]),static_cast<std::uint32_t>(target.pixels[1])};
        CHECK(actual==expected && (events.empty() ? "-" : events)==expectedEvents);CHECK(target.zoom==0xabcdef01U);
        CHECK((camera.RendererView().pixels==std::array<std::int32_t,2>{123,456}));CHECK((camera.Snapshot().pixels==std::array<std::int32_t,2>{789,987}));++count;
    }
    CHECK(count==178);
}
// 진단1은 장치 연결 없이도 반환하며 선택된 경계 누락은 각 원문 호출 위치에서 거부한다.
TEST_CASE(CameraPresent_DiagnosticAndSelectedDeviceGuards) {
    std::uint32_t diagnostic=1;std::int32_t width=1024,height=768,cells=256;RawCameraTargetState target;target.limits={3072,2048};target.pixels={4000,-1};
    PresentCameraTarget(target,{diagnostic,width,height,cells},{});CHECK((target.pixels==std::array<std::int32_t,2>{4000,-1}));
    diagnostic=6;bool flipMissing{},commitMissing{};try { PresentCameraTarget(target,{diagnostic,width,height,cells},{}); }catch (const std::logic_error&) { flipMissing=true; }
    CHECK(flipMissing && target.pixels[0]==4000 && target.pixels[1]==-1);
    try { PresentCameraTarget(target,{diagnostic,width,height,cells},{[] {},{}}); }catch (const std::logic_error&) { commitMissing=true; }
    CHECK(commitMissing && target.pixels[0]==3072 && target.pixels[1]==0);
}
// 같은 실제 Client 진단값의 변경을 읽고 제한 완료 전에는 현재 view/snapshot을 바꾸지 않는다.
TEST_CASE(CameraPresent_SharedDiagnosticAndDeferredCurrentView) {
    ClientGameplayGlobals client;auto& camera=client.Camera();auto& target=camera.Target();target.limits={3072,2048};target.pixels={4000,4000};target.zoom=65536;InitializeCameraView(camera.RendererView());InitializeCameraView(camera.Snapshot());
    std::int32_t width=1024,height=768,cells=256;std::string order;
    RawCameraPresentHooks hooks;hooks.flip=[&] { order+='F';CHECK(camera.RendererView().pixels[0]==0 && target.pixels[0]==4000); };hooks.commit=[&] { order+='C';CHECK(target.pixels[0]==3072 && target.pixels[1]==2048 && camera.Snapshot().pixels[0]==0);camera.ApplyPixelsToRendererAndSnapshot(target.pixels[0],target.pixels[1]); };
    client.PatchDiagnostic()=1;PresentCameraTarget(target,{client.PatchDiagnostic(),width,height,cells},hooks);CHECK(order.empty() && camera.RendererView().pixels[0]==0);
    client.PatchDiagnostic()=6;PresentCameraTarget(target,{client.PatchDiagnostic(),width,height,cells},hooks);CHECK(order=="FC" && camera.RendererView().pixels==target.pixels && camera.Snapshot().pixels==target.pixels);
}
