// 실제 Client 공급기가 현재 폴링/원문 바이트와 caller의 미복원 경계를 보존하는지 검사한다.
#include "TestSupport.h"
#include "client/CampaignCursorClientEffects.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"

using namespace netstorm::client;
// Run 없이 현재 장치 경계를 조립하고 현재 폴링 교체/원본 바이트 인코딩을 검증한다.
TEST_CASE(CampaignCursorClientEffects_CurrentPollAndOriginalTextPreserveOtherEffects) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
    Client client(std::move(options));RawButtonControls controls(netstorm::o::OriginalEdition::Patch1078);
    CampaignCursorEffects supplied;std::uint32_t selected{},closed{},sent{}; // 선택·팝업·전송은 caller의 실제 소비자를 그대로 빌린다.
    supplied.lifecycle.highlight=[&](std::uint32_t mode) { selected=mode; };
    supplied.lifecycle.closePopups=[&] { ++closed; };
    supplied.send=[&](std::uint32_t destination,std::span<const std::uint8_t> bytes) { sent=destination+static_cast<std::uint32_t>(bytes.size()); };
    auto effects=MakeCampaignCursorClientEffects(client,controls,std::move(supplied));
    client.inspectPoll=[](std::uint32_t code)->std::optional<InputEvent> { CHECK(code==InputCode::kLeftButton);return InputEvent{code,15,-7}; };
    auto point=effects.lifecycle.mouse();CHECK(point.x==15 && point.y==-7);
    client.inspectPoll=[](std::uint32_t)->std::optional<InputEvent> { return InputEvent{InputCode::kOutside,63,91}; };
    point=effects.lifecycle.mouse();CHECK(point.x==63 && point.y==91);
    CHECK(effects.lifecycle.translate("caf\xc3\xa9")=="caf\xe9");
    effects.lifecycle.highlight(4);effects.lifecycle.closePopups();effects.send(9,std::array<std::uint8_t,25>{});
    CHECK(selected==4 && closed==1 && sent==34);
    CHECK(!effects.drop.refresh && effects.drop.showSp && !effects.drop.record && !effects.drop.battleRefresh);
    bool missingTree{}; // 실제 비용 표시는 공통 Gump 계층이 없으면 빈 성공 대신 오류로 거부해야 한다.
    try { effects.drop.showSp(800,10,20,0); } catch (const std::logic_error&) { missingTree=true; }
    CHECK(missingTree);
    CHECK(effects.lifecycle.measure && effects.lifecycle.selectCursor && effects.placement.invalidate);
}
