// 실제 Client의 같은 Gump 저장소를 사용하는 커서 위치/수신/부모 조회 경계를 검사한다.
#include "TestSupport.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "DudeSpawnSupport.h"
#include <array>
#include <stdexcept>

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
