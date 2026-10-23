// 실제 Client의 같은 Gump 저장소를 사용하는 커서 위치/수신/부모 조회 경계를 검사한다.
#include "TestSupport.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/FloatingSpGump.h"
#include "o/GameClock.h"
#include "DudeSpawnSupport.h"
#include <array>
#include <stdexcept>
#include <vector>

using namespace netstorm::client;
namespace {
// 실제 돌 버튼/배경 생성자가 함께 읽는 원본 헤더와 팔레트 입력이다.
constexpr RawButtonAssets kCursorAssets{90,0,20,16,{7,255,13}};
// 동일 깊이의 겹친 두 버튼과 비활성 버튼이다. 마지막 부착한 형제가 선택 순서에서 앞선다.
const std::array<GumpControl,3> kCursorControls{{{0,{10,20,85,40},false,true,false},
    {1,{10,20,85,40},false,true,false},{2,{90,20,165,40},false,false,false}}};
// 화면 명령 번호에 대응하는 원본 바이트 라벨이다.
const std::array<std::string,3> kCursorLabels{{"First","Second","Disabled"}};
// 잘못된 토큰은0부모/0깊이의 성공한 조회로 바꾸지 않고 명시적으로 거부해야 한다.
template<class Callback> bool CursorBoundaryRejects(Callback callback) {
    try { callback(); } catch (const std::out_of_range&) { return true; }
    return false;
}
// 실제 메뉴·StyleText·ScrollingText 생성에 필요한 외부 효과만 연결한다. 노드/표/입력은 공통 처리기를 사용한다.
RawMenuHooks CursorFrameMenuHooks(std::function<void(std::uint32_t)> cursor,std::function<double()> seconds={}) {
    RawMenuHooks hooks; // 실제 자산은 호출자가 EnableMenus에 별도로 공급한다.
    hooks.command=[](auto,auto,auto) {};hooks.readInt=[](std::string_view,std::int32_t&) {};hooks.sound=[](std::string_view) {};
    hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=std::move(cursor);hooks.gameSeconds=std::move(seconds);return hooks;
}
// Fancy 생성자가 실제 overlay 표에 등록한 제목의 기존 부모를 유지하고 현재 화면 입력점을 읽는다.
ScrollingTextGumpNode* CursorFrameOverlayTitle(RawButtonControls& controls,RawMenuControls& menus,std::uint32_t& titleToken,ScreenPoint& point) {
    std::uint32_t menuToken{}; // 생성된 실제 메뉴의 title 필드로 대상 토큰을 조회한다.
    menus.Open({20,100},[&](const auto& widgets,const auto& items,const auto&) {
        menuToken=widgets.BeginFancy();items.AddTitle("Pointer title\n");items.AddText("Body",0x02000060,1); // 원본004b9a50이 표시하는 완료 줄을 실제 버퍼에 넣는다. ~.은 개행이 아니다.
    });
    auto* menu=static_cast<MenuGumpNode*>(menus.Node(menuToken));CHECK(menu!=nullptr);if (!menu) return nullptr;
    titleToken=menu->title;auto* title=static_cast<ScrollingTextGumpNode*>(menus.Node(titleToken));CHECK(title!=nullptr);if (!title) return nullptr;
    auto* decoration=menus.Node(menu->fancy);CHECK(decoration!=nullptr);if (!decoration) return nullptr;
    CHECK(title->parent==menu->fancy && decoration->parent==menuToken && menu->parent==controls.RootToken()); // Push 없이 이미 연결된 노드를 다시 Attach하지 않는다.
    CHECK(menu->expanding==0 && title->width>1 && title->height>1 && decoration->width>title->width && decoration->height>title->height); // Fancy는 자체 Idle 등록 없이 완료 제목의 실제 높이에 맞춰 생성된다.
    point={static_cast<int>((title->x+decoration->x)+menu->x)+1,static_cast<int>((title->y+decoration->y)+menu->y)+1}; // 실제 부모 세 개의 현재 좌표 안쪽을 입력으로 사용한다.
    CHECK(point.y<menu->y && point.x>=decoration->x+menu->x && point.y>=decoration->y+menu->y); // 본문 밖이지만 장식 부모 안쪽인 원래 제목 영역이다.
    title->textFlags=4;title->flags|=0x100;title->scroll=100;title->dragY=point.y-1; // 실제 scroll Broadcast만 켜며 태그 hit/Slider 표시는 켜지 않는다.
    CHECK(controls.PickGump(point)==0); //465ed0은 루트 직계 본문 Bounds만 보므로 그 위의 제목 자식을 반환하지 않는다.
    CHECK(controls.DispatchGump(controls.RootToken(),InputCode::kLeftButton|InputCode::kRelease,static_cast<std::uint32_t>(point.x),static_cast<std::uint32_t>(point.y))==titleToken); // 반면 실제 자식 우선 Route 경계는 같은 좌표에서 제목을 소비한다.
    title->flags|=0x100; // 위의 실제 release가 지운 통지 비트만 다음 포인터 관측의 초기 상태로 되돌린다.
    controls.CleanupGumps();return title;
}
}
// 루트 전체 경계나 자식 배경을 커서 Gump로 잘못 고르지 않고465ed0의직계 선택을 사용한다.
TEST_CASE(RawButtonCursorBoundary_PickUsesTopLevelCurrentTree) {
    // 두 판본은 같은 공개 호스트와 각 원본 생성/부착 처리기를 사용한다.
    for (const auto edition:{netstorm::o::OriginalEdition::Patch1078,netstorm::o::OriginalEdition::Cd1072}) {
        RawButtonControls controls(edition);controls.SetControls(kCursorControls,kCursorLabels,640,480,kCursorAssets);
        const auto first=controls.RegisteredGump(1),second=controls.RegisteredGump(3),disabled=controls.RegisteredGump(5);
        CHECK(first && second && disabled && first!=second);
        CHECK(controls.PickGump({10,20})==second && controls.PickGump({84,39})==second);
        CHECK(controls.PickGump({85,39})==0 && controls.PickGump({84,40})==0);
        CHECK(controls.PickGump({11,21})==second); // 손자 배경의 깊이가 달라도 직계 버튼을 반환한다.
        CHECK(controls.PickGump({90,20})==disabled); // 비활성은 visible/dead 선택 필터와 별개다.
        CHECK(controls.PickGump({300,300})==0 && controls.PickGump({-1,-1})==0);
        CHECK(controls.GumpParentSid(second)==0 && controls.GumpDepth(second)==0xffffffd8U);
        const auto root=controls.RegisteredGump(0);CHECK(root && root!=controls.GumpParentSid(second));
        controls.SetControls({},kCursorLabels,640,480,kCursorAssets);
        CHECK(controls.PickGump({10,20})==0 && controls.RegisteredGump(0)==root);
        CHECK(CursorBoundaryRejects([&] { static_cast<void>(controls.GumpParentSid(second)); }));
        CHECK(CursorBoundaryRejects([&] { static_cast<void>(controls.GumpDepth(second)); }));
    }
}
// 4651b0의 성공 토큰과 직접+28의성공 DWORD를 구별하고 기본 필터를 함께 실행한다.
TEST_CASE(RawButtonCursorBoundary_DispatchPreservesRecipientToken) {
    RawButtonControls controls(netstorm::o::OriginalEdition::Patch1078);
    controls.SetControls(kCursorControls,kCursorLabels,640,480,kCursorAssets);
    const auto token=controls.PickGump({20,25}),root=controls.RegisteredGump(0);
    CHECK(token!=0 && token!=1);
    CHECK(controls.DispatchGump(token,InputCode::kRightButton,20,25)==token);
    CHECK(controls.HandleGump(token,InputCode::kRightButton,20,25)==1);
    CHECK(controls.DispatchGump(root,InputCode::kRightButton,20,25)==token);
    CHECK(controls.DispatchGump(token,0x02000007,0,0)==0);
    CHECK(controls.DispatchGump(token,InputCode::kRightButton,640,480)==0); // 도움말 마우스의기본 경계 필터다.
    CHECK(controls.DispatchGump(controls.PickGump({100,25}),InputCode::kRightButton,100,25)==0);
    CHECK(CursorBoundaryRejects([&] { static_cast<void>(controls.DispatchGump(0,InputCode::kRightButton,0,0)); }));
    CHECK(CursorBoundaryRejects([&] { static_cast<void>(controls.GumpParentSid(0)); }));
    CHECK(CursorBoundaryRejects([&] { static_cast<void>(controls.GumpDepth(0)); }));
    controls.SetControls({},kCursorLabels,640,480,kCursorAssets);
    CHECK(CursorBoundaryRejects([&] { static_cast<void>(controls.DispatchGump(token,InputCode::kRightButton,20,25)); }));
}
// 같은 파생 노드의 세 서로 다른 식별자와 현재 DWORD 깊이를 실제 조회하며 캐시/잘못된 필드를 잡는다.
TEST_CASE(RawButtonCursorBoundary_ParentSidAndDepthReadCurrentDerivedNode) {
    const auto& assets=netstorm::test::dudespawn::Assets(netstorm::o::OriginalEdition::Patch1078); // 보호 자산은 읽기만 한다.
    FontStore fonts(assets.files,"Arial");RawButtonControls controls(netstorm::o::OriginalEdition::Patch1078);
    controls.SetControls({},kCursorLabels,640,480,kCursorAssets);RawMenuHooks hooks;
    hooks.command=[](auto,auto,auto) {};hooks.readInt=[](std::string_view,std::int32_t&) {};hooks.sound=[](std::string_view) {};
    hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
    auto& menus=controls.EnableMenus(assets.game,fonts,std::move(hooks));std::uint32_t token=0;
    menus.Open({100,100},[&](const auto& widgets,const auto& items,const auto&) {
        token=widgets.BeginVertical();items.AddText("Current",0x02000060,1);
    });
    auto* node=menus.Node(token);CHECK(node!=nullptr);if (!node) return;
    node->object=0x1234abcdU;node->form=0x87654321U;
    CHECK(node->parent==controls.RegisteredGump(0) && node->parent!=node->object && node->form!=node->object);
    CHECK(controls.GumpParentSid(token)==0x1234abcdU);
    node->object=77;node->depth=0xf1234567U;
    CHECK(controls.GumpParentSid(token)==77 && controls.GumpDepth(token)==0xf1234567U);
    node->object=0;node->form=0;node->depth=0xffffffd8U;
    const auto point=menus.Point("Current");CHECK(point.has_value());if (!point) return; // 원본 현재 항목 경계에서 고른다.
    CHECK(controls.PickGump(*point)==token);
    node->flags|=8;CHECK(controls.PickGump(*point)==token); //465ed0은void를 제외하지 않는다.
    node->flags|=0x10000;CHECK(controls.PickGump(*point)==0);
    node->flags&=~0x10008U;node->depth=1000;CHECK(controls.PickGump(*point)==0);
    node->depth=999;CHECK(controls.PickGump(*point)==token);
    node->flags&=~1U;CHECK(controls.PickGump(*point)==0);
    node->flags|=1; // 원본 팝업의 정상 삭제 계약으로 되돌린 뒤 해제한다.
    const auto registeredId=node->id; // Kill 요청 뒤에도 같은 등록 칸과 파생 노드가 남아야 한다.
    controls.RequestClosePopups();
    CHECK(menus.Node(token)==node && (node->flags&0x10000) && controls.RegisteredGump(registeredId)==token);
    CHECK(controls.GumpParentSid(token)==0 && controls.PickGump(*point)==0);
    controls.RequestClosePopups();CHECK(menus.Node(token)==node); // 재요청은 이미 죽은 대상을 다시 큐잉하지 않는다.
    controls.SetControls({},kCursorLabels,640,480,kCursorAssets);
    CHECK(menus.Node(token)==nullptr && controls.RegisteredGump(registeredId)==0);
    CHECK(CursorBoundaryRejects([&] { static_cast<void>(controls.GumpParentSid(token)); }));
}
// 실제 메뉴 수명 처리기가 없는 상태를 빈 성공으로 숨기지 않는다.
TEST_CASE(RawButtonCursorBoundary_ClosePopupsRequiresCurrentLifetime) {
    RawButtonControls controls(netstorm::o::OriginalEdition::Patch1078);bool rejected=false;
    try { controls.RequestClosePopups(); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);
}
// 실제 overlay의 raw 반환값과 cursor 콜백 전/후 dragY로 Overlay→Broadcast→일반 Route의 순서를 구별한다.
TEST_CASE(RawButtonCursorBoundary_NotifyPointerOverlayPrecedesBroadcastAndQueue) {
    const auto& assets=netstorm::test::dudespawn::Assets(netstorm::o::OriginalEdition::Patch1078);FontStore fonts(assets.files,"Arial");
    RawButtonControls controls(netstorm::o::OriginalEdition::Patch1078);controls.SetControls({},kCursorLabels,640,480,kCursorAssets);
    ScrollingTextGumpNode* title{};std::uint32_t titleToken{};ScreenPoint point{};std::vector<std::int32_t> cursorDragY; // 가상 cursor 효과 때의 실제 현재 노드를 관측한다.
    auto& menus=controls.EnableMenus(assets.game,fonts,CursorFrameMenuHooks([&](auto) { CHECK(title!=nullptr);if (title) cursorDragY.push_back(title->dragY); }));
    title=CursorFrameOverlayTitle(controls,menus,titleToken,point);if (!title) return;
    const auto pointer=controls.NotifyPointer({InputCode::kLeftButton,point.x,point.y},0,false);
    CHECK(pointer.overlay.has_value() && *pointer.overlay==1 && *pointer.overlay!=titleToken);CHECK(!pointer.activated && pointer.changed);
    CHECK(cursorDragY==std::vector<std::int32_t>{point.y-1});CHECK(title->dragY==point.y && title->scrollRequest==99);CHECK(!controls.HasCapture());
    const auto expected="raw_menu_input\t"+std::to_string(titleToken)+"\t"+std::to_string(0x0200003bU)+'\t'+std::to_string(point.x)+'\t'+std::to_string(point.y)+'\n'; // caller의 왼쪽 버튼이 아니라 별도 Overlay 사건 전역을 전달한다.
    CHECK(menus.Report().find(expected)!=std::string::npos);
    const auto modeBlocked=controls.NotifyPointer({InputCode::kLeftButton,point.x,point.y+1},2,false);CHECK(!modeBlocked.overlay && !modeBlocked.activated && modeBlocked.changed);
    const auto cursorBlocked=controls.NotifyPointer({InputCode::kLeftButton,point.x,point.y+2},0,true);CHECK(!cursorBlocked.overlay && !cursorBlocked.activated && cursorBlocked.changed);
    const auto hostBlocked=controls.NotifyPointer({InputCode::kLeftButton,point.x,point.y+3},0,false,false);CHECK(!hostBlocked.overlay && !hostBlocked.activated && hostBlocked.changed);
    CHECK(cursorDragY.size()==1 && title->dragY==point.y+3); // Overlay를 막은 세 경우에도 실제 현재 좌표의 Broadcast는 계속된다.
    const auto noHit=controls.NotifyPointer({0,500,400},0,false);CHECK(noHit.overlay.has_value() && *noHit.overlay==0 && !noHit.activated); // 실제0과 미호출을 구별한다.
}
// 밖 포인터는 두 통지를 모두 생략하며 현재 capture는 이후 일반 큐의 실제 release까지 보존한다.
TEST_CASE(RawButtonCursorBoundary_NotifyPointerOutsidePreservesActualCapture) {
    const auto& assets=netstorm::test::dudespawn::Assets(netstorm::o::OriginalEdition::Patch1078);FontStore fonts(assets.files,"Arial");
    RawButtonControls controls(netstorm::o::OriginalEdition::Patch1078);controls.SetControls({},kCursorLabels,640,480,kCursorAssets);std::size_t cursors{}; // 실제 cursor 장치 호출 수다.
    auto& menus=controls.EnableMenus(assets.game,fonts,CursorFrameMenuHooks([&](auto) { ++cursors; }));std::uint32_t titleToken{};ScreenPoint point{}; // 생성된 실제 제목의 현재 화면 입력점이다.
    auto* title=CursorFrameOverlayTitle(controls,menus,titleToken,point);if (!title) return;
    const auto down=controls.RouteInput({InputCode::kLeftButton,point.x,point.y});CHECK(down.consumed==titleToken && !down.activated && controls.HasCapture());CHECK(title->dragY==point.y && (title->flags&0x100));
    const auto inside=controls.NotifyPointer({InputCode::kLeftButton,point.x,point.y+1},0,false);CHECK(!inside.overlay && !inside.activated && inside.changed);CHECK(title->dragY==point.y+1 && controls.HasCapture());
    const auto outside=controls.NotifyPointer({InputCode::kOutside|InputCode::kRelease,point.x,-1},0,false);
    CHECK(!outside.overlay && !outside.activated && !outside.changed);CHECK(cursors==0 && title->dragY==point.y+1 && (title->flags&0x100) && controls.HasCapture());
    controls.CleanupGumps();controls.IdleGumps();CHECK(controls.HasCapture()); // 포인터 통지·별도 정리·Idle은 합성 Cancel이 아니다.
    const auto released=controls.RouteInput({InputCode::kLeftButton|InputCode::kRelease|InputCode::kOutside,point.x,-1});
    CHECK(released.consumed==titleToken && !released.activated && !controls.HasCapture() && !(title->flags&0x100));
}
// 실제 FloatingSp Idle의 Kill은 저장소/등록을 남기며 GameClock Pause 중에도 다음 Cleanup에서만 반납한다.
TEST_CASE(RawButtonCursorBoundary_SeparateCleanupAndIdleRetainDeadNodeUntilNextCleanup) {
    const auto& assets=netstorm::test::dudespawn::Assets(netstorm::o::OriginalEdition::Patch1078);FontStore fonts(assets.files,"Arial");
    netstorm::o::GameClock clock;std::uint32_t milliseconds=100;std::size_t reads{}; // 실제 게임 정지와 독립된 UI 프레임 호출을 대조한다.
    RawButtonControls controls(netstorm::o::OriginalEdition::Patch1078);controls.SetControls({},kCursorLabels,640,480,kCursorAssets);
    auto& menus=controls.EnableMenus(assets.game,fonts,CursorFrameMenuHooks([](auto) {},[&] { ++reads;return clock.GameSeconds(milliseconds); }));
    const auto token=menus.ShowSp(5,20,0);auto* node=menus.Node(token);CHECK(node!=nullptr);if (!node) return;const auto registeredId=node->id; // 생성 Attach의 화면 clamp를 유지한다.
    static_cast<FloatingSpGumpNode*>(node)->velocity=-1; // 앞선 프레임에서 상승한 실제 상태를 입력하여 다음 원문 y+=velocity가 화면을 이탈하게 한다.
    CHECK(controls.RegisteredGump(registeredId)==token && (node->flags&0x80) && !(node->flags&0x10000));const auto initialReads=reads;
    clock.Pause(milliseconds);milliseconds=1100;CHECK(clock.IsPaused() && clock.GameSeconds(milliseconds)==0.1);
    controls.CleanupGumps();CHECK(reads==initialReads && menus.Node(token)==node && !(node->flags&0x10000)); // Cleanup은 Idle/시계 조회를 실행하지 않는다.
    CHECK(controls.IdleGumps());CHECK(reads==initialReads+1 && clock.IsPaused());
    CHECK(menus.Node(token)==node && controls.RegisteredGump(registeredId)==token && (node->flags&0x10000) && !(node->flags&0x80)); // Kill 직후 실제 주소와 등록 칸이 남는다.
    CHECK(!controls.IdleGumps() && reads==initialReads+1 && menus.Node(token)==node); // 죽은 대상은 현재 idle 표에서 제거됐지만 아직 반납되지 않았다.
    controls.CleanupGumps();CHECK(menus.Node(token)==nullptr && controls.RegisteredGump(registeredId)==0);CHECK(reads==initialReads+1 && !controls.IdleGumps());
}
