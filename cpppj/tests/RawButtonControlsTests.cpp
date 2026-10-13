// 실제 Client 버튼 호스트의 캡처/부모 명령·표시 상태·반복 페이지 수명을 검사한다.
#include "TestSupport.h"
#include "client/RawButtonControls.h"
#include <array>
#include <cstring>
#include <fstream>
#include <sstream>

using namespace netstorm::client;
namespace {
// 실제 호스트가 같은 원본 헤더·팔레트를 입력/자식 배경/출력에 함께 공급하는 작은 검사 자산이다.
constexpr RawButtonAssets kAssets{90,0,20,16,{7,255,13}};
// 돌 버튼 두 개와 별도 메뉴 행을 섞는다. 화면 ID는 raw serial과 다른 숫자를 쓴다.
const std::array<GumpControl,3> kControls{{{7,{10,20,85,40},false,true,false},
    {2,{90,20,165,40},false,false,false},{5,{10,50,85,68},true,true,false}}};
// 화면 명령 번호와 대응하는 UTF-8 아닌 원본 바이트 라벨이다.
const std::array<std::string,8> kLabels{{"","","Disabled","","","Menu","","Campaign"}};
// 보고의 실제 카운터를 읽어 stale 노드/캡처가 다음 페이지에 남지 않는지 확인한다.
std::vector<std::uint32_t> Fields(const RawButtonControls& controls) {
    std::istringstream stream(controls.Report());std::string name;stream>>name;std::vector<std::uint32_t> result;
    // 모든 공개 카운터를 순서대로 읽는다.
    for (std::uint32_t value;stream>>value;) result.push_back(value);
    return result;
}
}
// 두 판본에서 실제 캡처/부모 serial/이탈 복귀·release·비활성 경계를 검사한다.
TEST_CASE(RawButtonControls_CaptureUsesRawTreeAndParentSerial) {
    // 같은 공유 트리 호스트를 두 판본의 실제 처리기로 실행한다.
    for (const auto edition:{netstorm::o::OriginalEdition::Patch1078,netstorm::o::OriginalEdition::Cd1072}) {
        int sounds=0;double now=1;RawButtonControls input(edition,{[&] { return now; },{},[&] { ++sounds; }});
        input.SetControls(kControls,kLabels,640,480,kAssets);
        CHECK(Fields(input)[0]==2 && Fields(input)[1]==5 && Fields(input)[5]==5);
        CHECK(!input.Event({InputCode::kLeftButton|InputCode::kRelease,20,25}));
        CHECK(!input.Event({InputCode::kRightButton,20,25}) && !input.Pressed() && sounds==0);
        CHECK(!input.Event({InputCode::kLeftButton,20,25}) && input.Pressed()==7 && sounds==1);
        CHECK(Fields(input)[3]!=0 && Fields(input)[4]==1);
        input.Poll({InputCode::kLeftButton,85,25});CHECK(!input.Pressed());
        now=2;input.Poll({InputCode::kLeftButton,10,39});CHECK(input.Pressed()==7);
        CHECK(input.Event({InputCode::kLeftButton|InputCode::kRelease,10,39})==7 && !input.Pressed());
        CHECK(Fields(input)[3]==0 && Fields(input)[4]==0 && sounds==1);
        CHECK(!input.Event({InputCode::kLeftButton,90,20}) && !input.Pressed() && sounds==1);
        CHECK(!input.Event({InputCode::kLeftButton,10,50})); // 메뉴 행은 돌 버튼으로 생성하지 않는다.
        input.Event({InputCode::kLeftButton,20,25});input.Cancel();
        CHECK(!input.Event({InputCode::kLeftButton|InputCode::kRelease,20,25}) && !input.Pressed());
    }
}
// raw 배경 눌림과 생성 문맥/현재 팔레트 전역의 서로 다른 수명을 검사한다.
TEST_CASE(RawButtonControls_DrawReadsCurrentChildAndButtonState) {
    RawButtonControls input(netstorm::o::OriginalEdition::Patch1078);input.SetControls(kControls,kLabels,640,480,kAssets);
    const auto normal=input.DrawPlan(7,40,12),disabled=input.DrawPlan(2,40,12);
    const auto oldBackground=input.BackgroundPlan(7,{0,0,640,480});CHECK(!oldBackground.empty());
    CHECK(normal.lines[0].first.x==11 && normal.lines[0].first.y==20 && normal.lines[0].last.x==83);
    input.Event({InputCode::kLeftButton,20,25});const auto pressed=input.DrawPlan(7,40,12);
    const auto background=input.BackgroundPlan(7,{0,0,640,480});
    CHECK(pressed.textOffset.x==normal.textOffset.x+1 && pressed.textOffset.y==normal.textOffset.y+1);
    CHECK(pressed.text.color==kAssets.palette.white && disabled.text.color==kAssets.palette.black);
    CHECK(background.size()==oldBackground.size() && background[1].shade!=oldBackground[1].shade);
    input.Poll({InputCode::kLeftButton|InputCode::kOutside,20,25});CHECK(!input.Pressed() && Fields(input)[3]==0);
    input.SetPalette({23,219,71});const auto recolored=input.DrawPlan(7,40,12);
    CHECK(recolored.lines[0].color==71 && recolored.text.color==normal.text.color);
}
// 실제 원본이 저장한 생성/Draw 6개 문맥·좌표·선 결과를 새 호스트에서 직접 대조한다.
TEST_CASE(RawButtonControls_DrawMatchesNativeConstructorFixture) {
    std::ifstream input(NETSTORM_RAWBUTTON_VISUAL_FIXTURE);CHECK(input.good());std::string line;std::size_t count=0;
    // 이미 저장한 독립 원본 생성/Draw 입력을 실제 호스트의 같은 파생 노드로 대조한다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::string edition,hex,expectedLines;unsigned number{},context{};ButtonVisualState state;ButtonPalette palette;ScreenPoint expected;
        row>>edition>>number>>state.rect.left>>state.rect.top>>state.rect.right>>state.rect.bottom>>state.textWidth>>state.textHeight
            >>state.value>>state.pressed>>state.flags>>palette.black>>palette.white>>palette.edge>>hex>>expected.x>>expected.y>>context>>expectedLines;
        if (edition=="original1037" || state.rect.left!=10 || state.rect.top!=20 || state.rect.right!=85 || state.rect.bottom!=39 ||
            state.value || (state.pressed!=0 && state.pressed!=1) || state.flags>1 || (state.flags && state.pressed)) continue;
        RawButtonControls controls(edition=="originals" ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072);
        const std::array<GumpControl,1> seed{{{7,state.rect,false,state.flags==0,false}}};auto assets=kAssets;assets.palette=palette;
        controls.SetControls(seed,kLabels,640,480,assets);
        if (state.pressed) controls.Event({InputCode::kLeftButton,20,25});
        const auto plan=controls.DrawPlan(7,state.textWidth,state.textHeight);CHECK(plan.textOffset.x==expected.x && plan.textOffset.y==expected.y);
        std::array<std::uint8_t,32> text{};
        // 실제 원본 생성자가 남긴 선택 문맥의 32바이트를 그대로 읽는다.
        for (std::size_t i=0;i<text.size();++i) text[i]=static_cast<std::uint8_t>(std::stoul(hex.substr(context*64+i*2,2),nullptr,16));
        CHECK(std::memcmp(&plan.text,text.data(),text.size())==0);std::ostringstream lines;
        // 원본 선 호출 순서/좌표/전체 DWORD 색 번호를 손실 없이 직렬화한다.
        for (std::size_t i=0;i<plan.lines.size();++i) {
            if (i) lines<<'|';const auto& current=plan.lines[i];lines<<current.first.x<<','<<current.first.y<<','<<current.last.x<<','<<current.last.y<<','<<current.color;
        }
        CHECK(lines.str()==expectedLines);++count;
    }
    CHECK(count==6);
}
// 1,000회 화면 교체에서 stale 노드·포커스·idle 없이 등록표 재사용/serial 증가를 검사한다.
TEST_CASE(RawButtonControls_PageReplacementReusesRegistryAndKeepsSerial) {
    RawButtonControls input(netstorm::o::OriginalEdition::Patch1078);input.SetControls(kControls,kLabels,640,480,kAssets);
    input.Event({InputCode::kLeftButton,20,25});const auto before=Fields(input);
    // 원본 등록 한도보다 많은 페이지 전환에서도 살아 있는 표/노드만 유지해야 한다.
    for (int i=0;i<1000;++i) input.SetControls(kControls,kLabels,640,480,kAssets);
    const auto after=Fields(input);CHECK(after[0]==2 && after[1]==5 && after[5]==5 && after[2]==before[2]+4000);
    CHECK(after[3]==0 && after[4]==0 && !input.Pressed());
    CHECK(!input.Event({InputCode::kLeftButton|InputCode::kRelease,20,25}));
    input.SetControls({},kLabels,800,600,kAssets);CHECK(Fields(input)[1]==1 && Fields(input)[5]==1);
}
