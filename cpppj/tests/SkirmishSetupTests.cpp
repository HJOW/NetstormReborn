// 12차 스커미시: 설정 저장, Tell 대상 해석, 화면 본문, 시작 대본/맵 선택, Campaign 목록 항목을 게임 실행 없이 검사한다.
#include "TestSupport.h"
#include "client/ProductPresentation.h"
#include "client/FrameRateOption.h"
#include "client/SkirmishSetup.h"
#include "client/HtmlWindowMetadata.h"
#include <algorithm>
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
    // 시작 섬 후보는 최대 참가자(사람 1 + AI 7 = 8명)보다 많아야 한다.
    CHECK(SkirmishStartIslandPool().size() >= static_cast<std::size_t>(kSkirmishMaxAi + 1));
    SkirmishSettings settings;
    settings.aiCount = 3;
    settings.ai[0].ally = true;                    // 소유자 2는 같은 편이다.
    settings.ai[1].ally = false;                   // 소유자 3, 4는 적이다.
    settings.bans = {"outpost", "sunCannon"};
    const auto launch = BuildSkirmishLaunch(settings, 7);
    CHECK(launch.islands.size() == 4); // 사람 1 + AI 3 = 시작 섬 4개이며 서로 다른 후보다.
    for (std::size_t first = 0; first < launch.islands.size(); ++first)
        for (std::size_t second = first + 1; second < launch.islands.size(); ++second)
            CHECK(!(launch.islands[first].fort == launch.islands[second].fort && launch.islands[first].territory == launch.islands[second].territory));
    CHECK(launch.mapSize == 2); // 4명은 링 간격 5칸(크기 2)이다.
    CHECK(launch.central[2] == 0 && (launch.central[0] & 63) < 68); // 중앙섬 기록은 원점 배치(위치 0)이고 패턴 번호가 영역 패턴 표 안이다.
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
    for (std::uint32_t value = 1; value < 10; ++value) {
        const auto other = BuildSkirmishLaunch(settings, value);
        different = different || other.islands[0].fort != launch.islands[0].fort || other.islands[0].territory != launch.islands[0].territory || other.central != launch.central;
    }
    CHECK(different);
    // 참가자 수에 따라 맵 크기가 커진다(2명=1, 3~4명=2, 5명 이상=3).
    SkirmishSettings size;
    size.aiCount = 1; CHECK(BuildSkirmishLaunch(size, 1).mapSize == 1 && BuildSkirmishLaunch(size, 1).islands.size() == 2);
    size.aiCount = 7; CHECK(BuildSkirmishLaunch(size, 1).mapSize == 3 && BuildSkirmishLaunch(size, 1).islands.size() == 8);
}

// 빈 월드 fort: 길이 표 35개 섹션을 유지하고 Chaff/Territory/Terr만 비운다. 입력이 틀리면 빈 결과다.
TEST_CASE(Skirmish_EmptyWorldFort) {
    std::vector<std::uint8_t> base{'F', 0};
    // 길이(자기 2바이트 포함)가 서로 다른 섹션 35개를 만든다(Chaff=4, Territory=6, Terr=15..34).
    for (std::size_t index = 0; index < 35; ++index) {
        std::vector<std::uint8_t> body(index == 6 ? 120 : (index % 5) + 3, static_cast<std::uint8_t>(index + 1));
        const auto length = body.size() + 2;
        base.push_back(static_cast<std::uint8_t>(length & 0xff)); base.push_back(static_cast<std::uint8_t>(length >> 8));
        base.insert(base.end(), body.begin(), body.end());
    }
    const auto world = MakeSkirmishEmptyWorldFort(base);
    CHECK(!world.empty() && world[0] == 'F');
    // 결과를 다시 섹션으로 읽는다.
    std::vector<std::vector<std::uint8_t>> sections;
    std::size_t position = 2;
    while (sections.size() < 35 && position + 2 <= world.size()) {
        const std::size_t length = world[position] | (static_cast<std::size_t>(world[position + 1]) << 8);
        sections.emplace_back(world.begin() + static_cast<std::ptrdiff_t>(position + 2), world.begin() + static_cast<std::ptrdiff_t>(position + length));
        position += length;
    }
    CHECK(sections.size() == 35 && position == world.size());
    CHECK(sections[0].size() == 3 && sections[0][0] == 1); // 나머지 섹션(예: Subscriber)은 그대로다.
    CHECK(sections[4].size() == 1 + 256 * 3 && sections[4][0] == 2 && sections[4][1] == 99 && sections[4][2] == 0 && sections[4][3] == 0); // Chaff는 빈 청크 256개다.
    CHECK(sections[6].size() == 120 && std::all_of(sections[6].begin(), sections[6].end(), [](auto byte) { return byte == 0; })); // 영역 표는 모두 0이다.
    for (std::size_t index = 15; index < 35; ++index) CHECK(sections[index].size() == 1 && sections[index][0] == 2); // Terr 섹션은 버전만 남는다.
    CHECK(MakeSkirmishEmptyWorldFort({}).empty());
    CHECK(MakeSkirmishEmptyWorldFort(std::span<const std::uint8_t>(base).first(base.size() - 1)).empty()); // 마지막 섹션이 잘렸다.
    base[0] = 'X';
    CHECK(MakeSkirmishEmptyWorldFort(base).empty()); // 서명이 틀렸다.
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
    CHECK(Has(count, "$Checked=2,Tell,Patch1101.SkirmishSet(count;2),1,1") && Has(count, "$Checked=7,Tell,Patch1101.SkirmishSet(count;7),0,1"));
    const auto ai = BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishAi(2)"), settings, 0, {});
    CHECK(Has(ai, "$Checked=Hard,Tell,Patch1101.SkirmishSet(diff;2;3),1,1") && Has(ai, "$Checked=Ally,Tell,Patch1101.SkirmishSet(ally;2;1),1,1"));
    const auto translate = [](std::string_view text) { return std::string(text) + "!"; };
    const auto bans = BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishBans(0)"), settings, 0, translate);
    CHECK(Has(bans, "$Checked=Wind Generator! - windBattery,Tell,Patch1101.SkirmishSet(ban;0;0),1,1") && Has(bans, "$Checked=Rain Generator! - rainBattery,Tell,Patch1101.SkirmishSet(ban;1;0),0,1") && Has(bans, "Patch1101.SkirmishBans(1)") && !Has(bans, "Previous"));
    CHECK(Has(BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishBans(2)"), settings, 0, translate), "Outpost!") && Has(BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishBans(2)"), settings, 0, translate), "$Checked=Previous page,"));
    settings.bans = {"windBattery", "rainBattery", "thunderBattery", "sunArcher", "windArcher"};
    CHECK(Has(BuildSkirmishScreen(ParseSkirmishTarget("Patch1101.SkirmishBans(0)"), settings, 0, translate), "Five units are banned"));
}

// 생성한 선택 행을 실제 HTML 메타 처리기에 통과시켜 선택 비트와 명령 인자 전체를 확인한다.
TEST_CASE(Skirmish_CheckedMetadataKeepsSelectionAndActionArguments) {
    SkirmishSettings settings; // 화면과 각 선택 동작이 공유하는 현재 설정이다.
    settings.aiCount = 2;
    settings.ai[0] = {AiDifficulty::Hard, true};
    settings.bans = {"windBattery"};
    // 세 언어의 수·난이도/동맹·금지 목록 첫쪽과 마지막쪽을 같은 실제 처리기로 해석한다.
    for (const auto language : {0U, 7U, 5U}) {
        for (const auto target : {"Patch1101.SkirmishCount", "Patch1101.SkirmishAi(2)", "Patch1101.SkirmishBans(0)", "Patch1101.SkirmishBans(2)"}) {
            const auto screen = ParseSkirmishTarget(target); // 화면별 후보와 선택 행 수를 정한다.
            HtmlWindowGumpNode node; // 메타 처리기가 메뉴 열 수를 갱신하는 현재 부모다.
            std::vector<std::string> stored; // 실제 명령 해석 뒤 선택 행이 참조하는 문자열 고리다.
            std::size_t rows{}, selected{}; // 선택 행 전체 수와 선택 아이콘을 켠 행 수다.
            HtmlWindowMetadataHooks hooks; // 실제 버튼·일반 행은 관찰하고 선택 행은 아래에서 검증한다.
            hooks.node = [&](auto token) { return token == 1 ? &node : nullptr; };
            hooks.clearButtons = [](auto) {};
            hooks.addButton = [](auto, auto, auto, auto, auto) {};
            hooks.beginMenu = [](auto flags) { CHECK(flags == 0x40000); };
            hooks.menuText = [](auto, auto, auto) {};
            hooks.commands.lookup = [](auto name) { CHECK(name == "Tell"); return 1U; };
            hooks.storeText = [&](auto text) { stored.emplace_back(text); return static_cast<std::uint32_t>(stored.size()); };
            hooks.checked = [&](auto checked, auto label, auto command, auto parameter, bool enabled) { // 옵션과 같은 checked 경계에서 전체 인자를 검증한다.
                CHECK(command == 1 && enabled && (checked == 0 || checked == 1));
                CHECK(!label.empty() && label.front() != '*' && label.front() != '[');
                const auto parsed = ParseSkirmishTarget(stored.at(parameter - 1)); // 쉼표 없는 메타 열이 원래 인자들로 복원돼야 한다.
                const auto candidates = screen.kind == SkirmishScreen::Count ? 7U : screen.kind == SkirmishScreen::Ai ? 5U : screen.args[0] == "0" ? 12U : 3U; // 후보 뒤 이동·해제 순서도 보존한다.
                if (rows >= candidates) {
                    CHECK(checked == 0);
                    if (rows == candidates) CHECK(parsed.kind == SkirmishScreen::Bans);
                    else CHECK(parsed.kind == SkirmishScreen::Set && parsed.args == std::vector<std::string>{"clearbans"});
                    ++rows;
                    return;
                }
                CHECK(parsed.kind == SkirmishScreen::Set);
                CHECK(parsed.args.size() == (parsed.args[0] == "count" ? 2U : 3U));
                auto changed = settings; // 원래 화면은 보존하고 실제 클릭 적용 결과만 확인한다.
                ApplySkirmishAction(changed, parsed.args);
                if (parsed.args[0] == "count") CHECK((changed.aiCount == settings.aiCount) == (checked != 0));
                else if (parsed.args[0] == "diff") CHECK((changed.ai[0].difficulty == settings.ai[0].difficulty) == (checked != 0));
                else if (parsed.args[0] == "ally") CHECK((changed.ai[0].ally == settings.ai[0].ally) == (checked != 0));
                else { CHECK(parsed.args[0] == "ban"); CHECK(changed.bans.size() == (checked ? 0U : 2U)); }
                ++rows;
                selected += checked != 0;
            };
            const auto source = BuildSkirmishScreen(screen, settings, language, {});
            std::array<char, 8192> buffer{}; // 제품 메타 처리기와 같은 물리 본문 버퍼다.
            CHECK(source.size() < buffer.size());
            std::copy(source.begin(), source.end(), buffer.begin());
            HtmlWindowMetadata metadata(std::move(hooks)); // 생성된 화면을 실제 선택 메뉴 작성 경계까지 진행한다.
            CHECK(metadata.Process(1, buffer) == 1);
            CHECK(rows == (screen.kind == SkirmishScreen::Count ? 7U : screen.kind == SkirmishScreen::Ai ? 5U : screen.args[0] == "0" ? 14U : 5U));
            CHECK(selected == (screen.kind == SkirmishScreen::Ai ? 2U : screen.kind == SkirmishScreen::Bans && screen.args[0] == "2" ? 0U : 1U));
        }
    }
}

// 후순위 F1: Frame Rate 옵션은 60/75/120만 받고 설정이 없으면 원본 기본 75로 본다.
TEST_CASE(FrameRateOption_ParseAndNormalize) {
    CHECK(ParseFrameRateTarget("Patch1101.SetFrameRate(120)") == 120 && ParseFrameRateTarget("patch1101.setframerate(60)") == 60 && ParseFrameRateTarget("Patch1101.SetFrameRate(75)") == 75);
    CHECK(!ParseFrameRateTarget("Patch1101.SetFrameRate(90)") && !ParseFrameRateTarget("Patch1101.SetFrameRate(abc)") && !ParseFrameRateTarget("Patch1101.SetFrameRate(") && !ParseFrameRateTarget("Patch1101.Skirmish"));
    CHECK(NormalizeFrameRate(0) == 75 && NormalizeFrameRate(-3) == 75 && NormalizeFrameRate(5000) == 75 && NormalizeFrameRate(120) == 120 && NormalizeFrameRate(60) == 60);
}

// Campaign 목록(UCampaign)에는 Skirmish 항목이 Back 버튼 앞에 한 번만 들어가고 다른 대상은 건드리지 않는다.
TEST_CASE(Skirmish_CampaignEntry) {
    const std::string original = "<h2>Campaign</h2>\n$Menu=Early Missions,Tell,x\n$Button=Back,Tell,Blank\n";
    const auto english = ProductTellBody("UCampaign", original, 0);
    CHECK(Has(english, "$Menu=Skirmish,Tell,Patch1101.Skirmish\n$Button=Back") && english.find("$Menu=Skirmish") == english.rfind("$Menu=Skirmish")); // 한 번만 들어간다.
    CHECK(Has(ProductTellBody("UCampaign", original, 7), "$Menu=스커미시,Tell,Patch1101.Skirmish") && Has(ProductTellBody("ucampaign", original, 5), "$Menu=スカーミッシュ"));
    CHECK(ProductTellBody("Blank", original, 0) == original);
}
