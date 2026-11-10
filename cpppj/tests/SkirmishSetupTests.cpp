// 12차 스커미시: 설정 저장, Tell 대상 해석, 화면 본문, 시작 대본/맵 선택, Campaign 목록 항목을 게임 실행 없이 검사한다.
#include "TestSupport.h"
#include "client/ProductPresentation.h"
#include "client/SkirmishSetup.h"
#include <map>

using namespace netstorm::client;
using netstorm::o::AiDifficulty;

namespace { // 이 파일의 검사 도우미다.
// 문자열에 부분 문자열이 들어 있는지 본다.
bool Has(const std::string& text, std::string_view part) { return text.find(part) != std::string::npos; }
// 설정 저장소를 흉내 내는 맵이다(키 → 값).
struct FakeConfig {
    std::map<std::string, std::string> values; // 쓴 값이 여기에 쌓인다.
    SkirmishConfigGet Getter() { return [this](std::string_view key) { const auto found = values.find(std::string(key)); return found == values.end() ? std::string() : found->second; }; }
    SkirmishConfigSet Setter() { return [this](std::string_view key, std::string_view value) { values[std::string(key)] = std::string(value); }; }
};
}

// Tell 대상 해석: 종류와 인자를 소문자 기준으로 읽고 스커미시가 아니면 None이다.
TEST_CASE(Skirmish_ParseTargets) {
    CHECK(ParseSkirmishTarget("Patch1101.Skirmish").kind == SkirmishScreen::Main);
    CHECK(ParseSkirmishTarget("Patch1101.SkirmishCount").kind == SkirmishScreen::Count);
    const auto ai = ParseSkirmishTarget("Patch1101.SkirmishAi(3)");
    CHECK(ai.kind == SkirmishScreen::Ai && ai.args.size() == 1 && ai.args[0] == "3");
    const auto set = ParseSkirmishTarget("Patch1101.SkirmishSet(ban,4,1)");
    CHECK(set.kind == SkirmishScreen::Set && set.args.size() == 3 && set.args[0] == "ban" && set.args[1] == "4" && set.args[2] == "1");
    CHECK(ParseSkirmishTarget("Patch1101.SkirmishStart").kind == SkirmishScreen::Start);
    CHECK(ParseSkirmishTarget("Patch1101.Credits").kind == SkirmishScreen::None && ParseSkirmishTarget("UCampaign").kind == SkirmishScreen::None);
    CHECK(IsSkirmishMissionName("skirmish") && IsSkirmishMissionName("Skirmish") && !IsSkirmishMissionName("Skirmish2"));
}

// 동작 적용: AI 수, AI별 난이도/동맹, 금지 유닛 토글(최대 5개), 모두 해제와 돌아갈 화면.
TEST_CASE(Skirmish_ApplyActions) {
    SkirmishSettings settings;
    CHECK(ApplySkirmishAction(settings, {"count", "3"}) == "Patch1101.Skirmish" && settings.aiCount == 3);
    ApplySkirmishAction(settings, {"count", "99"});
    CHECK(settings.aiCount == kSkirmishMaxAi); // 범위 밖은 최대로 줄인다.
    CHECK(ApplySkirmishAction(settings, {"diff", "3", "1"}) == "Patch1101.SkirmishAi(3)" && settings.ai[1].difficulty == AiDifficulty::Easy);
    CHECK(ApplySkirmishAction(settings, {"ally", "3", "1"}) == "Patch1101.SkirmishAi(3)" && settings.ai[1].ally);
    // 금지 유닛은 5개까지만 받고 같은 항목을 다시 고르면 해제한다.
    for (int index = 0; index < 7; ++index) ApplySkirmishAction(settings, {"ban", std::to_string(index), "0"});
    CHECK(settings.bans.size() == kSkirmishMaxBans);
    CHECK(ApplySkirmishAction(settings, {"ban", "0", "0"}) == "Patch1101.SkirmishBans(0)" && settings.bans.size() == kSkirmishMaxBans - 1);
    ApplySkirmishAction(settings, {"ban", "26", "2"}); // 마지막 후보(outpost)도 고를 수 있다.
    CHECK(settings.bans.back() == "outpost");
    CHECK(ApplySkirmishAction(settings, {"clearbans"}) == "Patch1101.SkirmishBans(0)" && settings.bans.empty());
    CHECK(ApplySkirmishAction(settings, {"unknown"}) == "Patch1101.Skirmish");
}

// 설정 저장/읽기 왕복과 잘못된 저장 값의 정리.
TEST_CASE(Skirmish_SaveLoadRoundTrip) {
    FakeConfig config;
    SkirmishSettings settings;
    settings.aiCount = 4;
    settings.ai[0] = {AiDifficulty::Hard, true};
    settings.ai[2] = {AiDifficulty::Easy, false};
    settings.bans = {"outpost", "sunCannon"};
    SaveSkirmishSettings(config.Setter(), settings);
    const auto loaded = LoadSkirmishSettings(config.Getter());
    CHECK(loaded.aiCount == 4 && loaded.ai[0].difficulty == AiDifficulty::Hard && loaded.ai[0].ally && loaded.ai[2].difficulty == AiDifficulty::Easy && !loaded.ai[2].ally);
    CHECK(loaded.bans == settings.bans);
    FakeConfig bad;
    bad.values["skirmishAiCount"] = "99"; bad.values["skirmishBans"] = "outpost,notAType,sunCannon,sunCannon,windBattery,rainBattery,thunderBattery,bulf";
    const auto cleaned = LoadSkirmishSettings(bad.Getter());
    CHECK(cleaned.aiCount == kSkirmishMaxAi && cleaned.bans.size() == kSkirmishMaxBans); // 모르는 이름/중복은 버리고 5개까지만 받는다.
    CHECK(LoadSkirmishSettings(FakeConfig{}.Getter()).aiCount == 1); // 값이 없으면 기본값이다.
}

// 시작 대본: AI 수만큼 소유자 2..N+1이 있고, 동맹/금지/AI 이름이 설정을 따른다.
TEST_CASE(Skirmish_LaunchScript) {
    // 모든 AI 수에 후보 맵이 있다.
    for (int count = 1; count <= kSkirmishMaxAi; ++count) CHECK(!SkirmishMapsFor(count).empty());
    SkirmishSettings settings;
    settings.aiCount = 3;
    settings.ai[0].ally = true;                    // 소유자 2는 같은 편이다.
    settings.ai[1].ally = false;                   // 소유자 3, 4는 적이다.
    settings.bans = {"outpost", "sunCannon"};
    const auto launch = BuildSkirmishLaunch(settings, 7);
    CHECK(!launch.mapName.empty());
    bool inPool = false;
    for (const auto name : SkirmishMapsFor(3)) inPool = inPool || name == launch.mapName; // 고른 맵은 AI 3명용 후보에 있다.
    CHECK(inPool);
    const auto& script = launch.script;
    CHECK(Has(script, "ai2Name=") && Has(script, "ai3Name=") && Has(script, "ai4Name=") && !Has(script, "ai5Name="));
    CHECK(Has(script, "myAllyList=\"2\"")); // 같은 편은 AI 2뿐이다.
    CHECK(Has(script, "ai2AllyList=\"1;2\"") && Has(script, "ai3AllyList=\"3;4\"") && Has(script, "ai4AllyList=\"3;4\"")); // 적끼리도 같은 편이다.
    CHECK(Has(script, "techAllowed=\"deny;outpost;sunCannon\"") && Has(script, "randGeysers=1") && Has(script, "[Succeeded][BadTeamDead]") && Has(script, "[Failed][GoodTeamDead]"));
    CHECK(launch.script == BuildSkirmishLaunch(settings, 7).script); // 같은 난수는 같은 시작 자료다.
    SkirmishSettings plain;
    CHECK(!Has(BuildSkirmishLaunch(plain, 0).script, "techAllowed")); // 금지가 없으면 줄 자체가 없다.
    // 난수에 따라 후보가 달라질 수 있다.
    bool different = false;
    for (std::uint32_t value = 1; value < 10; ++value) different = different || BuildSkirmishLaunch(settings, value).mapName != launch.mapName;
    CHECK(different);
}

// 화면 본문: 시작/취소와 현재 값, 금지 표시와 쪽 이동, 최대 도달 안내, 언어별 제목.
TEST_CASE(Skirmish_ScreenBodies) {
    SkirmishSettings settings;
    settings.aiCount = 2;
    settings.ai[0] = {AiDifficulty::Hard, true};
    settings.bans = {"windBattery"};
    const auto mainScreen = BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.Skirmish"), settings, 0, {});
    CHECK(Has(mainScreen, "AI players: 2") && Has(mainScreen, "AI 1: Hard / Ally") && Has(mainScreen, "AI 2: Normal / Enemy") && !Has(mainScreen, "AI 3:"));
    CHECK(Has(mainScreen, "Banned units: 1/5") && Has(mainScreen, "$Button=Start,Tell,Patch1101.SkirmishStart") && Has(mainScreen, "$Button=Cancel,Tell,UCampaign"));
    const auto korean = BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.Skirmish"), settings, 7, {});
    CHECK(Has(korean, "스커미시") && Has(korean, "인공지능 수: 2") && Has(korean, "어려움 / 동맹"));
    CHECK(Has(BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.Skirmish"), settings, 5, {}), "スカーミッシュ"));
    const auto count = BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishCount"), settings, 0, {});
    CHECK(Has(count, "$Menu=* 2,Tell,Patch1101.SkirmishSet(count,2)") && Has(count, "SkirmishSet(count,7)"));
    const auto ai = BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishAi(2)"), settings, 0, {});
    CHECK(Has(ai, "$Menu=* Hard,Tell,Patch1101.SkirmishSet(diff,2,3)") && Has(ai, "$Menu=* Ally,Tell,Patch1101.SkirmishSet(ally,2,1)"));
    const auto translate = [](std::string_view text) { return std::string(text) + "!"; };
    const auto bans = BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishBans(0)"), settings, 0, translate);
    CHECK(Has(bans, "[X] Wind Generator! - windBattery") && Has(bans, "[ ] Rain Generator! - rainBattery") && Has(bans, "Patch1101.SkirmishBans(1)") && !Has(bans, "Previous"));
    CHECK(Has(BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishBans(2)"), settings, 0, translate), "Outpost!") && Has(BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishBans(2)"), settings, 0, translate), "$Menu=Previous page,"));
    settings.bans = {"windBattery", "rainBattery", "thunderBattery", "sunArcher", "windArcher"};
    CHECK(Has(BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishBans(0)"), settings, 0, translate), "Five units are banned"));
}

// Campaign 목록(UCampaign)에는 Skirmish 항목이 Back 버튼 앞에 한 번만 들어가고 다른 대상은 건드리지 않는다.
TEST_CASE(Skirmish_CampaignEntry) {
    const std::string original = "<h2>Campaign</h2>\n$Menu=Early Missions,Tell,x\n$Button=Back,Tell,Blank\n";
    const auto english = ProductTellBody("UCampaign", original, 0);
    CHECK(Has(english, "$Menu=Skirmish,Tell,Patch1101.Skirmish\n$Button=Back") && english.find("$Menu=Skirmish") == english.rfind("$Menu=Skirmish")); // 한 번만 들어간다.
    CHECK(Has(ProductTellBody("UCampaign", original, 7), "$Menu=스커미시,Tell,Patch1101.Skirmish") && Has(ProductTellBody("ucampaign", original, 5), "$Menu=スカーミッシュ"));
    CHECK(ProductTellBody("Blank", original, 0) == original);
}
