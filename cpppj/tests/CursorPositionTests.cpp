// 원본 정수 위치/역투영/절삭 및 실제 연속 이동 사슬의 독립 기대값을 비교한다.
#include "CursorPreviewSupport.h"
#include "client/RawCursorPosition.h"

using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace preview=netstorm::test::cursorpreview;
namespace {
// 독립 단독 입력18개의 열 순서다. 원문 DWORD를 산술 없이 비트로 읽는다.
enum Column : std::size_t { Operation,MouseX,MouseY,HotX,HotY,Width,Height,CameraX,CameraY,Snap,OldX,OldY,Lock,Legal,ActivateMutation,CreateMutation,InputX,InputY };
// 쉼표 DWORD를 원문 순서대로 읽는다.
std::vector<std::uint32_t> Numbers(std::string_view text) { return preview::Numbers(text); }
// 단독 위치 갱신의 명시 외부 효과와 공유 참조를 가진 장면이다.
struct PositionScene {
    CursorDropState drop; // 위치/입력 화면점/잠금/수신 Gump의 기존 상태다.
    CursorPlacementState placement; // 현재 그림의 크기/기준점이다.
    std::array<std::uint32_t,2> camera{}; // 현재59a900/904의 입력 비트다.
    std::uint32_t snap{}; // 현재55a2d8의 전체 DWORD다.
    std::vector<std::uint32_t> input; // 원문 단독 입력18개다.
    std::string trace; // 원본 외부 효과 순서다.
    // 실제 DWORD 인자를 부호 손실 없이 표시한다.
    void Event(std::string_view letter,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=letter;
        // 모든 호출 인자를 원문 unsigned 십진수로 기록한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 이전 결과를 재사용하지 않고 원문 입력만 초기화한다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        input=values;drop={};placement={};trace.clear();camera={values[CameraX],values[CameraY]};snap=values[Snap];
        drop.screenX=-3;drop.screenY=47;drop.x=std::bit_cast<float>(values[OldX]);drop.y=std::bit_cast<float>(values[OldY]);drop.lockDepth=values[Lock];drop.legalGump=values[Legal];
        placement.hotspotX=std::bit_cast<std::int32_t>(values[HotX]);placement.hotspotY=std::bit_cast<std::int32_t>(values[HotY]);placement.imageWidth=std::bit_cast<std::int32_t>(values[Width]);placement.imageHeight=std::bit_cast<std::int32_t>(values[Height]);
    }
    // 순서 관찰과 원문에 명시한 전역 변경만 수행한다. 좌표 계산은 제품 몸체가 한다.
    CursorPositionHooks Hooks() {
        CursorPositionHooks hooks;
        hooks.activate=[this](std::uint32_t force) {
            Event("A",{force,std::bit_cast<std::uint32_t>(drop.x),std::bit_cast<std::uint32_t>(drop.y),static_cast<std::uint32_t>(drop.screenX),static_cast<std::uint32_t>(drop.screenY)});
            if (input[ActivateMutation]==1) drop.lockDepth=3;if (input[ActivateMutation]==2) drop.lockDepth=0;
            if (input[ActivateMutation]==3) { drop.x=99;drop.y=-77; }
            if (input[ActivateMutation]==4) { snap=0;camera={std::bit_cast<std::uint32_t>(1.25f),std::bit_cast<std::uint32_t>(-2.5f)};placement.hotspotX=-5;placement.hotspotY=9;placement.imageWidth=13;placement.imageHeight=17; }
        };
        hooks.createPreviews=[this] {
            Event("B",{std::bit_cast<std::uint32_t>(drop.x),std::bit_cast<std::uint32_t>(drop.y),static_cast<std::uint32_t>(drop.screenX),static_cast<std::uint32_t>(drop.screenY),drop.lockDepth});
            const auto mode=input[CreateMutation];drop.legalGump=mode==1 || mode==2 ? 77U : 0U;if (mode==2) drop.lockDepth=5;if (mode==3) { drop.x=-11.25f;drop.y=7.5f; }
        };
        hooks.selectCursor=[this](std::uint32_t cursor) { Event("C",{cursor}); };return hooks;
    }
    // 원문의13전역과 두 출력 float의 비트를 같은 순서로 보존한다.
    std::string Snapshot(const std::array<float,2>& result) const {
        const std::array<std::uint32_t,15> values{static_cast<std::uint32_t>(drop.screenX),static_cast<std::uint32_t>(drop.screenY),std::bit_cast<std::uint32_t>(drop.x),std::bit_cast<std::uint32_t>(drop.y),drop.lockDepth,drop.legalGump,snap,camera[0],camera[1],static_cast<std::uint32_t>(placement.hotspotX),static_cast<std::uint32_t>(placement.hotspotY),static_cast<std::uint32_t>(placement.imageWidth),static_cast<std::uint32_t>(placement.imageHeight),std::bit_cast<std::uint32_t>(result[0]),std::bit_cast<std::uint32_t>(result[1])};std::string text;
        // 부호0/NaN payload도 숫자로 재변환하지 않는다.
        for (const auto value:values) text+=(text.empty() ? "" : ",")+std::to_string(value);return text;
    }
};
// 사슬의 각 이동 직후63DWORD를 같은 장면의 실제 상태에서 직렬화한다.
std::string ChainSnapshot(const preview::PreviewScene& scene,std::uint32_t snap,const std::array<std::uint32_t,2>& camera) {
    std::string text=scene.Snapshot();const std::array<std::uint32_t,11> values{static_cast<std::uint32_t>(scene.drop.screenX),static_cast<std::uint32_t>(scene.drop.screenY),scene.drop.legalGump,scene.life.currentGump,scene.drop.lockDepth,snap,camera[0],camera[1],scene.drop.inputFlags,scene.drop.costFlags,scene.placementState.placementFlags};
    // 기존52DWORD 뒤에 실제 화면점/Gump/잠금/snap/카메라를 붙인다.
    for (const auto value:values) text+=','+std::to_string(value);return text;
}
}
// 단독 세 몸체의 모든 상태·효과를 독립 x86 관찰과 비교한다.
TEST_CASE(CursorPosition_NativeUpdateInverseAndTruncate) {
    PositionScene scene;std::size_t count{};
    // 각 원문 입력은 새 상태에서만 실행한다.
    for (const auto& row:LoadFixture(NETSTORM_CURSOR_POSITION_FIXTURE).rows) {
        if (row[0]!="Point") continue;CHECK(row.size()==5);const auto input=Numbers(row[1]);scene.Prepare(input);RawCursorPosition position(scene.drop,scene.placement,scene.snap,scene.camera,scene.Hooks());
        std::array<float,2> result{std::bit_cast<float>(input[InputX]),std::bit_cast<float>(input[InputY])};
        if (!input[Operation]) position.Update(std::bit_cast<std::int32_t>(input[MouseX]),std::bit_cast<std::int32_t>(input[MouseY]));
        else if (input[Operation]==1) result=RawCursorPosition::ScreenToWorld(std::bit_cast<std::int32_t>(input[MouseX]),std::bit_cast<std::int32_t>(input[MouseY]),scene.camera);
        else result=RawCursorPosition::TruncatePoint(result);
        const bool same=(scene.trace.empty() ? "-" : scene.trace)==row[2] && scene.Snapshot(result)==row[3];CHECK(same);
        if (!same) { std::printf("위치 입력=%s\n기대=%s / %s\n실제=%s / %s\n",row[1].c_str(),row[3].c_str(),row[2].c_str(),scene.Snapshot(result).c_str(),scene.trace.c_str());break; }++count;
    }
    CHECK(count==623);
}
// 실제 위치→정리→생성→판정→화면 준비를 두 번 이어 각 단계의 전체 바이트까지 대조한다.
TEST_CASE(CursorPosition_RealLifecyclePreviewsAndPlacementChain) {
    preview::PreviewScene scene;std::size_t count{};
    // 기존 미리보기 장면과 원문 공급자를 공유한다. 원문 fixture는 바꾸지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_CURSOR_POSITION_FIXTURE).rows) {
        if (row[0]!="Chain") continue;const auto input=Numbers(row[1]);scene.Prepare({input[0],1,input[1],input[2],1,2,1,0,std::bit_cast<std::uint32_t>(20.25f),std::bit_cast<std::uint32_t>(21.75f),0,1,0,1});
        std::uint32_t snap=input[3];std::array<std::uint32_t,2> camera{std::bit_cast<std::uint32_t>(3.25f),std::bit_cast<std::uint32_t>(-5.75f)};scene.drop.lockDepth=input[4];scene.drop.costFlags|=input[2] ? 0x200U : 0U;
        auto previewHooks=scene.Hooks();auto placementHooks=scene.PlacementHooks(previewHooks);
        if (input[5]) {
            placementHooks.gumpAt=[&scene](std::int32_t x,std::int32_t y) { scene.Event("G",{static_cast<std::uint32_t>(x),static_cast<std::uint32_t>(y)});return 1U; };
            placementHooks.accept=[&scene](std::uint32_t gump,std::uint32_t event,std::uint32_t first,std::uint32_t second,std::uint32_t third) { scene.Event("A",{gump,event,first,second,third});return 2U; };
            placementHooks.parent=[](std::uint32_t gump) { CHECK(gump==2);return 77U; };
        }
        RawCursorPlacement placement(scene.pool,scene.types,{},scene.drop,scene.life,scene.placementState,placementHooks);BindCursorPreviewPlacement(previewHooks,scene.pool,placement,placementHooks);
        RawCursorPreviews previews(scene.pool,scene.types,scene.drop,scene.life,scene.surfaces,std::move(previewHooks));CursorLifecycleHooks lifecycleHooks;
        lifecycleHooks.prepareCursor=[&placement] { placement.Prepare(); };lifecycleHooks.destroy=[&scene](Sid sid,std::uint32_t flags) { scene.Event("D",{sid.value,flags}); };
        RawCursorLifecycle lifecycle(scene.pool,scene.types,scene.drop,scene.life,std::move(lifecycleHooks));
        RawCursorPosition position(scene.drop,scene.placementState,snap,camera,MakeCursorPositionHooks(lifecycle,previews,[&scene](std::uint32_t cursor) { scene.Event("K",{cursor}); }));std::string states,raws;
        // 각 실제 이동의 즉시 상태/바이트를 따로 보존해 두 번째 갱신이 첫 오류를 가리지 못하게 한다.
        for (const auto mouse:std::array<ScreenPoint,2>{{{320,220},{337,243}}}) {
            position.Update(mouse.x,mouse.y);if (!states.empty()) { states+='|';raws+='|'; }states+=ChainSnapshot(scene,snap,camera);raws+=scene.Raws();
        }
        const bool same=(scene.trace.empty() ? "-" : scene.trace)==row[2] && states==row[3] && raws==row[4];CHECK(same);
        if (!same) { std::printf("이동 사슬=%s\n기대=%s / %s\n실제=%s / %s\n",row[1].c_str(),row[3].c_str(),row[2].c_str(),states.c_str(),scene.trace.c_str());break; }++count;
    }
    CHECK(count==64);
}
// 현재 카메라/snap을 복사하지 않고 Take 꼬리 연결과 필수 의존·단일 풀을 확인한다.
TEST_CASE(CursorPosition_SharedReferencesBindingsAndGuards) {
    PositionScene scene;scene.Prepare({0,0,0,0,0,0,0,std::bit_cast<std::uint32_t>(1.0f),std::bit_cast<std::uint32_t>(2.0f),0,0,0,1,0,0,0,0,0});
    RawCursorPosition position(scene.drop,scene.placement,scene.snap,scene.camera,{});CursorLifecycleHooks tail;BindCursorPosition(tail,position);tail.updatePosition(0,0);CHECK(scene.drop.x==1 && scene.drop.y==2);
    scene.camera={std::bit_cast<std::uint32_t>(3.75f),std::bit_cast<std::uint32_t>(-5.5f)};scene.snap=1;tail.updatePosition(0,0);CHECK(scene.drop.x==3 && scene.drop.y==-5);
    scene.drop.lockDepth=0;CHECK(Throws([&] { position.Update(99,77); }));
    CHECK(Throws([&] { RawCursorPosition invalid(scene.drop,scene.placement,scene.snap,scene.camera,{},OriginalEdition::Cd1072); }));
    preview::PreviewScene first,second;RawCursorLifecycle lifecycle(first.pool,first.types,first.drop,first.life,{});RawCursorPreviews previews(first.pool,first.types,first.drop,first.life,first.surfaces,{}),other(second.pool,second.types,second.drop,second.life,second.surfaces,{});
    CHECK(Throws([&] { MakeCursorPositionHooks(lifecycle,previews,{}); }));CHECK(Throws([&] { MakeCursorPositionHooks(lifecycle,other,[](std::uint32_t) {}); }));
}
