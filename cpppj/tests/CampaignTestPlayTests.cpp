// 테스트 전용 캠페인 자동 플레이(--test-*)의 명령줄 해석·난이도 감싸기·로컬 AI 설정과, 실제 보호 미션 Start의 로컬 AI 부착을 게임 창 없이 검사한다.
// 동맹 AI가 생성 시점에 적으로 분류되던 11차 판정 수정도 같은 실제 Start로 확인한다.
// (2026-11-14 사용자 요청: 아군 AI 자동 플레이 테스트 기능 추가와 함께 신규 작성)
#include "DudeSpawnSupport.h"
#include "client/CampaignTestPlay.h"
#include "client/CampaignTestSpectator.h"
#include "client/RawCampaignMissionStartBindings.h"
#include "o/AiAllianceSide.h"
#include <bit>
#include <cmath>
#include <map>
#include <unordered_set>

using namespace netstorm::client;
namespace { // 이 파일의 검사 도우미다.
// 600BYTE 설정/본체의 little endian DWORD를 읽는다.
std::uint32_t Dword(std::span<const std::uint8_t> bytes, std::size_t offset) {
    std::uint32_t value{};
    // 네 BYTE를 낮은 비트부터 합친다.
    for (std::size_t index = 0; index < 4; ++index) value |= static_cast<std::uint32_t>(bytes[offset + index]) << (8 * index);
    return value;
}
// 같은 위치의 float 값이다.
float Float(std::span<const std::uint8_t> bytes, std::size_t offset) { return std::bit_cast<float>(Dword(bytes, offset)); }
// NUL 앞까지의 문자열이다.
std::string Text(std::span<const std::uint8_t> bytes, std::size_t offset, std::size_t limit) {
    std::string text;
    // 제한 안에서 첫 NUL까지 복사한다.
    for (std::size_t index = offset; index < offset + limit && bytes[index]; ++index) text.push_back(static_cast<char>(bytes[index]));
    return text;
}
// 인자 목록을 실제 main 반복문처럼 차례로 해석한다. 해석하지 못한 인자가 있으면 그 인자를 돌려준다.
std::string ParseAll(std::vector<std::string> arguments, CampaignTestPlayOptions& options) {
    // 값 옵션은 다음 인자를 소비하는 main의 value() 규약을 따른다.
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const auto value = [&]() -> std::string { if (index + 1 >= arguments.size()) throw std::invalid_argument("값 누락"); return arguments[++index]; };
        if (!ParseCampaignTestPlayOption(arguments[index], value, options)) return arguments[index];
    }
    FinalizeCampaignTestPlay(options); return {};
}
// 예외 종류를 확인하는 도우미다.
template<class F> bool ThrowsInvalid(F&& function) { try { function(); } catch (const std::invalid_argument&) { return true; } return false; }

// 실제 게임 계산을 대신하지 않는 장치/시간/설정 외부 입력이다. CampaignMissionStartTests의 StartExternal과 같은 공급 규약을 따른다.
struct TestPlayExternal {
    SoundList sounds{OriginalEdition::Patch1078}; // 실제 원본 음향 이름표이며 장치는 열지 않는다.
    std::unordered_set<SoundHandle> playing; // 재생 계층 외부 입력의 현재 핸들이다.
    std::array<std::uint8_t, 4> you{'Y', 'o', 'u', 0}; // 원본 번역 키의 명시 영어 입력이다.
    std::size_t ui{}; // 화면 요청 횟수만 관찰한다.
    const MissionFrameGateState* gate{}; // 실제 Start에서 빌린 현재 데모 상태다.
    std::map<std::uint32_t, bool> classified; // 난이도 조회가 받은 owner별 적 판정이다(11차 판정 수정 확인용).
    // 실제 월드 설치에 필요한 장치/표시 경계를 공급하고 그 외 게임 규칙은 Session 몸체에 맡긴다.
    RawCampaignSessionServices Services() {
        sounds.Initialize(); RawCampaignSessionServices s; s.soundNames = &sounds; s.unixSeconds = [] { return 946684800; };
        s.sound.wallSeconds = [] { return 100.0; };
        s.sound.playLoopAt = [this](float, float, SoundHandle current, SoundHandle sound) { const auto token = current ? current : sound; playing.insert(token); return token; };
        s.sound.playOnceAt = [](float, float, SoundHandle sound, std::uint32_t, std::uint32_t) { return sound; };
        s.sound.play = [this](SoundHandle sound, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) { playing.insert(sound); return sound; };
        s.sound.stop = [this](SoundHandle sound) { playing.erase(sound); }; s.sound.isPlaying = [this](SoundHandle sound) { return playing.contains(sound) ? 1U : 0U; };
        s.priest.ownerNotice = [this](Sid) { ++ui; }; s.priest.shield.noticeSound = [this](const PriestShieldNotice&) { ++ui; };
        s.priest.shield.translate = [](std::string_view key) { return std::string(key); }; s.priest.shield.tell = [this](std::string_view) { ++ui; };
        s.ownership.refreshProduction = [this] { ++ui; }; s.ownership.themeMode = [] { return 0U; };
        s.install.translateYou = [this] { return std::span<const std::uint8_t>(you); }; s.install.setConfig = [this](std::string_view, std::span<const std::uint8_t>) { ++ui; };
        s.install.refreshWindowTitle = [this] { ++ui; }; s.install.resetCamera = [this] { ++ui; };
        s.assets.postPop.playImpactSound = [this](std::uint32_t) { ++ui; }; s.assets.queueCommand = [this](std::uint32_t, std::int32_t, std::int32_t) { ++ui; };
        s.hit.general.warningEvent = [this] { ++ui; };
        s.player.log = [this](std::uint32_t, std::string_view) { ++ui; }; s.deletion.selected = [] { return Sid{}; };
        s.dependencyTransmit = [this](Sid, std::uint32_t) { ++ui; }; s.playerContent.gump = [](std::uint32_t) { return 0U; };
        s.ai.allocationBytes = [](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(), bytes.end(), std::uint8_t{0xb3}); };
        s.ai.bridgeScratchBytes = s.ai.allocationBytes; s.production.allocationBytes = s.ai.allocationBytes;
        s.ai.names.language = [] { return 1U; }; s.ai.names.translate = s.ai.names.substitute = [](std::string_view text) { return std::string(text); };
        s.ai.assertion = [](std::string_view text) { throw std::logic_error(std::string(text)); }; // 실제 AI 불변식 실패를 빈 성공으로 숨기지 않는다.
        s.ai.blockingGumpOpen = [] { return false; };
        s.contentLifetime.localNotice = [this](std::uint32_t) { ++ui; }; s.contentLifetime.sound = [this](float, float) { ++ui; }; s.motion.path.stepSound = [this](Sid, float, float, std::uint32_t) { ++ui; };
        s.construction.wallSeconds = [] { return 100.0; }; s.construction.configInt = [](std::string_view, std::int32_t fallback) { return fallback; }; s.construction.refresh = [this] { ++ui; };
        s.production.effects.configInt = [](std::string_view) { return 0; }; s.production.effects.modeAssertion = [] { throw std::logic_error("테스트 자동 플레이 Start의 생산 모드 진단"); };
        s.missionOutcome.demo = [this] { if (!gate) throw std::logic_error("테스트 자동 플레이 Start의 현재 데모 게이트 미연결"); return gate->demo; };
        s.ai.defaultName = [] { return std::string("Default AI"); }; s.ai.localTransport = [] { return false; };
        s.ai.participation.clearChat = [this](std::uint32_t) { ++ui; }; s.ai.participation.addChat = [this](std::uint32_t, std::string_view, std::uint32_t) { ++ui; };
        s.ai.participation.sendAlliance = [this](std::uint32_t, std::uint32_t, std::uint32_t) { ++ui; };
        s.ai.participation.assertion = [](PlayerParticipationAssertion) { throw std::logic_error("테스트 자동 플레이 Start의 예상하지 않은 원본 참여 진단"); };
        s.ai.difficulty = [this](std::uint32_t owner, bool enemy) -> std::uint32_t { classified[owner] = enemy; return 0U; }; // 기본(옵션 없음)은 보통이며 판정만 기록한다.
        return s;
    }
    // 미초기화 600BYTE 입력을 명시 패턴으로 공급한다.
    CampaignMissionStartEffects Effects(const MissionFrameGateState& current) {
        gate = &current;
        CampaignMissionStartEffects e; e.settingsBytes = [](std::uint32_t, std::span<std::uint8_t> bytes) { std::fill(bytes.begin(), bytes.end(), std::uint8_t{0xa7}); };
        e.gameSeconds = [] { return 123.5; }; e.refreshSp = [this] { ++ui; }; e.refreshCommandGump = [this] { ++ui; }; e.resetCamera = [this] { ++ui; };
        e.randomGeysers = [this] { ++ui; }; return e;
    }
};
// 실제 읽기 전용 VFS를 같은 Config/MissionScript에 연결한다.
void ConfigureStart(ConfigInterface& config) {
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
}
// 한 미션을 실제 Session으로 설치하고 Start까지 실행한 결과다.
struct StartedMission {
    std::uint32_t local{}; // 로컬 owner다.
    std::array<std::uint32_t, 9> brains{}; // owner별 AI 본체 토큰이며 0은 AI 없음이다.
    std::array<std::vector<std::uint8_t>, 9> bodies; // owner별 AI 본체 사본(0xb08)이다.
    std::array<std::uint32_t, 9> allies{}; // owner별 Player+54 동맹 마스크다.
    std::string localName; // 로컬 Player+0c 이름이다.
    float localSp{}; // Start 뒤 로컬 SP다.
    std::map<std::uint32_t, bool> classified; // 난이도 조회의 owner별 적 판정이다.
    std::vector<std::string> technology; // 확인 대상 기술 이름 중 로컬이 가진 것이다.
};
// 지정 미션을 일반 캠페인(비데모)으로 시작한다. test가 있으면 UberGump와 같은 방식으로 난이도 조회와 Start 효과에 연결한다.
StartedMission StartMission(std::string_view name, const std::optional<CampaignTestPlayOptions>& test, const std::vector<std::string>& probeTechnology) {
    const auto& assets = netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); }); ConfigureStart(config); MissionScript script(config, name); CHECK(script.Loaded());
    auto fort = FortTemplate::Parse(assets.files.Read(script.FortPath()), assets.game.TypeTable()); TestPlayExternal external;
    auto services = external.Services();
    if (test) services.ai.difficulty = WrapCampaignTestPlayDifficulty(*test, std::move(services.ai.difficulty)); // UberGump의 적격 미션 연결과 같다.
    RawCampaignSession session(assets.game, assets.files, std::move(fort), script.FortName(), std::move(services)); session.Load();
    MissionFrameGateState gate; MissionStartState state; MissionStartGlobals globals; MissionAiSettingsState ai;
    auto effects = external.Effects(gate); if (test) effects.testPlay = *test;
    RawCampaignMissionStartBindings start(session, script, gate, state, globals, ai, std::move(effects));
    start.Start(); CHECK(gate.started == 1);
    StartedMission result; result.local = session.LocalOwner(); result.localSp = session.Sp(result.local); result.classified = external.classified;
    const auto records = session.PlayerRecords(); result.localName = Text(records, result.local * 0xb4 + 12, 32);
    // owner1..8의 AI 토큰·본체·동맹을 Start 직후 그대로 기록한다.
    for (std::uint32_t owner = 1; owner < 9; ++owner) {
        result.allies[owner] = Dword(records, owner * 0xb4 + 0x54); result.brains[owner] = session.Participation().Brain(owner);
        if (result.brains[owner]) { const auto body = session.AiMemory().Bytes(result.brains[owner]); result.bodies[owner].assign(body.begin(), body.end()); }
    }
    // 로컬이 가진 기술만 이름으로 남긴다.
    for (const auto& technology : probeTechnology) { const auto type = assets.game.TypeTable().Find(technology); CHECK(type > 0); if (session.FindTechnology(static_cast<std::int32_t>(result.local), type).value) result.technology.push_back(technology); }
    return result;
}
}

// 명령줄 옵션 해석: 프리셋은 명시 값을 덮지 않고, 잘못된 값은 거부하며, 다른 인자는 처리하지 않는다.
TEST_CASE(CampaignTestPlay_ParseOptions) {
    CampaignTestPlayOptions empty; CHECK(!empty.Active() && !ConfiguredCampaignTestPlay().Active()); // 지정 전 프로세스 설정은 비활성이다(일반 실행과 같다).
    CHECK(!ConfiguredCampaignTestPlay().followBattle && CurrentTestSpectator(&empty) == nullptr); // 일반 실행에서는 관전 카메라 판단기도 없다.
    CampaignTestPlayOptions preset; CHECK(ParseAll({"--test-autoplay"}, preset).empty());
    CHECK(preset.Active() && preset.allyAi == AiDifficulty::Hard && preset.enemyAi == AiDifficulty::Easy && !preset.startSp && !preset.enemyStartSp && preset.followBattle); // 프리셋은 관전 카메라도 켠다.
    CampaignTestPlayOptions follow; CHECK(ParseAll({"--test-follow-battle"}, follow).empty() && follow.Active() && follow.followBattle && !follow.allyAi); // 관전 카메라만 켤 수도 있다.
    CampaignTestPlayOptions mixed; CHECK(ParseAll({"--test-ally-ai", "Normal", "--test-autoplay", "--test-start-sp", "30000", "--test-enemy-start-sp", "0"}, mixed).empty());
    CHECK(mixed.allyAi == AiDifficulty::Normal && mixed.enemyAi == AiDifficulty::Easy && mixed.startSp == 30000 && mixed.enemyStartSp == 0); // 명시한 아군 난이도는 프리셋이 덮지 않는다.
    CampaignTestPlayOptions numeric; CHECK(ParseAll({"--test-enemy-ai", "3", "--test-ally-ai", "1"}, numeric).empty());
    CHECK(numeric.enemyAi == AiDifficulty::Hard && numeric.allyAi == AiDifficulty::Easy && numeric.Active());
    CampaignTestPlayOptions spOnly; CHECK(ParseAll({"--test-start-sp", "10000000"}, spOnly).empty() && spOnly.Active() && !spOnly.allyAi && !spOnly.enemyAi); // 시작 SP만 바꾸는 사람 플레이 검증도 가능하다.
    CampaignTestPlayOptions other; CHECK(ParseAll({"--window"}, other) == "--window" && !other.Active()); // 다른 인자는 기존 오류 처리로 넘긴다.
    CampaignTestPlayOptions bad;
    CHECK(ThrowsInvalid([&] { ParseAll({"--test-ally-ai", "insane"}, bad); }));
    CHECK(ThrowsInvalid([&] { ParseAll({"--test-start-sp", "-1"}, bad); }));
    CHECK(ThrowsInvalid([&] { ParseAll({"--test-start-sp", "10000001"}, bad); }));
    CHECK(ThrowsInvalid([&] { ParseAll({"--test-enemy-start-sp", "12abc"}, bad); }));
    CHECK(ThrowsInvalid([&] { ParseAll({"--test-ally-ai"}, bad); })); // 값 누락이다.
    CHECK(DescribeCampaignTestPlay(mixed) == "아군AI=보통 적AI=쉬움 시작SP=30000 적시작SP=0 관전카메라=켬");
}
// 난이도 감싸기: 지정한 쪽만 바꾸고 지정하지 않은 쪽은 기존 조회(옵션/스커미시)를 그대로 부른다.
TEST_CASE(CampaignTestPlay_DifficultyWrap) {
    std::vector<std::pair<std::uint32_t, bool>> calls; // 기존 조회가 받은 인자다.
    const auto base = [&](std::uint32_t owner, bool enemy) -> std::uint32_t { calls.emplace_back(owner, enemy); return 7U; };
    CampaignTestPlayOptions both; both.allyAi = AiDifficulty::Hard; both.enemyAi = AiDifficulty::Easy;
    const auto wrapped = WrapCampaignTestPlayDifficulty(both, base);
    CHECK(wrapped(1, false) == 3U && wrapped(2, true) == 1U && calls.empty());
    CampaignTestPlayOptions enemyOnly; enemyOnly.enemyAi = AiDifficulty::Hard;
    const auto partial = WrapCampaignTestPlayDifficulty(enemyOnly, base);
    CHECK(partial(3, true) == 3U && partial(4, false) == 7U && calls.size() == 1 && calls[0] == std::make_pair(4U, false));
    CHECK(WrapCampaignTestPlayDifficulty(enemyOnly, {})(5, false) == 0U); // 기존 조회가 없으면 보통(0)이다.
}
// 로컬 AI 설정: 원문 MakeAi 규칙(기술 추가 없음·현재 SP)과 이름/동맹 보존, 위험 필드 0을 확인한다.
TEST_CASE(CampaignTestPlay_LocalAiSettings) {
    std::vector<std::uint8_t> settings(600, 0xa7);
    const std::string name = "Commander"; std::vector<std::uint8_t> record(32, 0); std::copy(name.begin(), name.end(), record.begin());
    PrepareLocalTestAiSettings(settings, record, 12345.75F, 0x0aU);
    CHECK(Text(settings, 0, 30) == name && settings[name.size()] == 0 && settings[0x1d] == 0 && settings[0x1e] == 0);
    CHECK(Dword(settings, 0x22c) == 12345 && Dword(settings, 0x23c) == 0x0a && Dword(settings, 0x240) == 0 && Dword(settings, 0x244) == 0 && Dword(settings, 0x250) == 0 && Dword(settings, 0x254) == 0);
    CHECK(settings[0x1f] == 0xa7 && settings[0x220] == 0xa7 && settings[0x248] == 0xa7); // 기본값이 쓴 다른 필드(호출자 소유)는 건드리지 않는다.
    std::vector<std::uint8_t> longName(32, 'N'); PrepareLocalTestAiSettings(settings, longName, -5.0F, 0);
    CHECK(Text(settings, 0, 30) == std::string(29, 'N') && settings[29] == 0 && Dword(settings, 0x22c) == 0); // 29BYTE로 자르고 음수 SP는 0이다.
    SetAiSettingsStartMoney(settings, 1500); CHECK(Dword(settings, 0x22c) == 1500);
    CHECK(ThrowsInvalid([&] { std::vector<std::uint8_t> small(100); PrepareLocalTestAiSettings(small, record, 0.0F, 0); }));
}
// 생성 직전 아군/적 판정: 로컬 자신·로컬 동맹 마스크·설정 동맹 목록 중 하나라도 있으면 적이 아니다.
TEST_CASE(AiAllianceSide_EnemyClassification) {
    std::vector<std::uint8_t> settings(600, 0);
    CHECK(!IsAiEnemyOfLocal(1, 1, 0x02, settings)); // 로컬 자신이다.
    CHECK(IsAiEnemyOfLocal(2, 1, 0x02, settings)); // 동맹 표시가 없다.
    CHECK(!IsAiEnemyOfLocal(2, 1, 0x06, settings)); // 로컬 마스크에 2가 있다.
    settings[0x23c] = 0x02; CHECK(!IsAiEnemyOfLocal(2, 1, 0x02, settings)); // ai2AllyList="1"이다.
    settings[0x23c] = 0x18; CHECK(IsAiEnemyOfLocal(2, 1, 0x02, settings)); // 다른 AI끼리의 동맹("3;4")은 로컬 아군이 아니다.
    CHECK(ThrowsInvalid([&] { IsAiEnemyOfLocal(2, 1, 0, std::span<const std::uint8_t>(settings).first(16)); }));
}
// 실제 보호 미션(캠페인 1-1 TheWarBegins): 테스트 설정이 없으면 원본처럼 로컬 AI가 없고, 있으면 로컬 AI·난이도·시작 SP가 반영된다.
TEST_CASE(CampaignTestPlay_ActualStartAttachesLocalAi) {
    const std::vector<std::string> probe{"sunWalker", "rainBattery", "sunAviary", "sunCannon", "windArcher", "windBattery", "sunBalloon", "sunBlocker", "sunArcher"}; // 앞 넷은 myTech, 뒤 다섯은 기본 AI 기술 목록에만 있다.
    const auto normal = StartMission("TheWarBegins", std::nullopt, probe);
    CHECK(normal.local >= 1 && normal.local <= 8 && normal.brains[normal.local] == 0 && normal.brains[2] != 0 && normal.localSp == 3000.0F); // 원본 일반 캠페인은 로컬 AI를 만들지 않는다.
    CHECK((normal.technology == std::vector<std::string>{"sunWalker", "rainBattery", "sunAviary", "sunCannon"}));
    CHECK(normal.classified.contains(2) && normal.classified.at(2)); // 적 AI는 적으로 판정된다.
    CampaignTestPlayOptions test; test.allyAi = AiDifficulty::Hard; test.enemyAi = AiDifficulty::Easy; test.startSp = 30000; test.enemyStartSp = 1500;
    const auto played = StartMission("TheWarBegins", test, probe);
    const auto local = played.local; CHECK(local == normal.local && played.brains[local] != 0 && played.brains[2] != 0);
    CHECK(played.localSp == 30000.0F && played.localName == normal.localName && played.allies[local] == normal.allies[local]); // 시작 SP만 바뀌고 이름/동맹은 보존된다.
    CHECK(played.technology == normal.technology); // 로컬 AI가 기본 AI 기술을 추가하지 않는다(myTech 유지).
    const auto& mine = played.bodies[local]; const auto& enemy = played.bodies[2]; const auto& baseline = normal.bodies[2];
    CHECK(mine.size() == 0xb08 && Dword(mine, 0x260) == local && Dword(mine, 0x2d8) == 30000 && mine[0x1e] == 0 && Dword(mine, 0x254) == 0 && Dword(mine, 0x244) == 0);
    CHECK(Float(mine, 0x220) == 3.0F && Float(mine, 0x228) == 1.5F); // 기본 6.0/3.0에 어려움 배율 0.5다.
    CHECK(Dword(enemy, 0x2d8) == 1500 && Dword(baseline, 0x2d8) != 1500); // 적 시작 자금만 지정값이다.
    if (Float(baseline, 0x228) > 0.0F) CHECK(Float(enemy, 0x228) == Float(baseline, 0x228) * 4.0F); // 적 이동 간격은 쉬움 배율 4다.
}
// 실제 보호 미션(캠페인 2-4 GuardMyBack, 비데모): 미션 동맹 AI(ai2AllyList="1")는 적이 아니며 아군 난이도·미션 시작 자금을 받는다.
TEST_CASE(CampaignTestPlay_ActualAlliedMissionAi) {
    const auto normal = StartMission("GuardMyBack", std::nullopt, {});
    const auto local = normal.local; CHECK(local == 1 && normal.brains[local] == 0 && normal.brains[2] != 0 && normal.brains[3] != 0);
    CHECK(normal.classified.contains(2) && !normal.classified.at(2) && normal.classified.contains(3) && normal.classified.at(3)); // 11차 수정: 동맹 AI2는 적이 아니다.
    CHECK(((normal.allies[local] >> 2) & 1U) && ((normal.allies[2] >> local) & 1U));
    CampaignTestPlayOptions test; test.autoplay = true; test.enemyStartSp = 777; FinalizeCampaignTestPlay(test);
    const auto played = StartMission("GuardMyBack", test, {});
    CHECK(played.brains[local] != 0 && ((played.allies[local] >> 2) & 1U)); // 로컬 AI를 붙여도 기존 동맹이 지워지지 않는다.
    CHECK(Dword(played.bodies[2], 0x2d8) == Dword(normal.bodies[2], 0x2d8) && Dword(played.bodies[3], 0x2d8) == 777); // 동맹은 미션 자금, 적만 지정 자금이다.
    if (Float(normal.bodies[2], 0x228) > 0.0F) CHECK(Float(played.bodies[2], 0x228) == Float(normal.bodies[2], 0x228) * 0.5F); // 동맹 AI도 아군 난이도(어려움)다.
    if (Float(normal.bodies[3], 0x228) > 0.0F) CHECK(Float(played.bodies[3], 0x228) == Float(normal.bodies[3], 0x228) * 4.0F); // 적 AI는 쉬움이다.
}
// 관전 카메라: 첫 표본은 기준만 기록하고, 피해가 모인 곳의 가중 중심을 고르며, 작은 소모는 무시한다.
TEST_CASE(CampaignTestSpectator_PicksHeaviestBattle) {
    CampaignTestSpectator spectator;
    std::vector<TestSpectatorSample> samples{{10, 80, 1, 20.0F, 20.0F, 100}, {11, 80, 2, 22.0F, 20.0F, 100}, {12, 80, 2, 60.0F, 60.0F, 100}};
    CHECK(spectator.Due(0.0) && !spectator.Observe(samples, 0.0) && !spectator.Due(0.4) && spectator.Due(0.5)); // 기준 기록과 0.5초 표본 간격이다.
    samples[2].hitPoints = 95; CHECK(!spectator.Observe(samples, 0.5)); // 5 피해는 전투 최소치(20) 미만이다.
    samples[0].hitPoints = 70; samples[1].hitPoints = 90; // 20,20 근처에서 30+10 피해다.
    const auto focus = spectator.Observe(samples, 1.0);
    CHECK(focus && !focus->ritual && std::abs(focus->x - 20.5F) < 0.01F && std::abs(focus->y - 20.0F) < 0.01F && focus->weight == 40.0); // (20*30+22*10)/40 = 20.5다.
    CHECK(spectator.Current() && spectator.Current()->x == focus->x);
}
// 관전 카메라: 유지 시간 안에는 다른 전투로 옮기지 않고, 가까운 같은 전투로도 옮기지 않으며, 파괴는 상한 60으로 센다.
TEST_CASE(CampaignTestSpectator_DwellAndDestroyed) {
    TestSpectatorSettings settings; settings.windowSeconds = 1.0; // 앞 전투가 빨리 잊히게 시간 폭을 줄인다.
    CampaignTestSpectator spectator(settings);
    std::vector<TestSpectatorSample> samples{{1, 80, 1, 10.0F, 10.0F, 100}, {2, 80, 2, 50.0F, 50.0F, 500}, {3, 80, 2, 12.0F, 10.0F, 100}};
    spectator.Observe(samples, 0.0);
    samples[0].hitPoints = 50; samples[1].hitPoints = 499; CHECK(spectator.Observe(samples, 0.5)); // 첫 전투(10,10)로 옮긴다. 50,50도 1 피해를 입었다.
    samples.erase(samples.begin() + 1); // 피해를 입은 50,50의 HP499 객체가 사라졌다(파괴).
    CHECK(!spectator.Observe(samples, 1.0)); // 파괴(상한 60)가 더 크지만 유지 시간(4초) 전이다.
    samples[1].hitPoints = 99; CHECK(!spectator.Observe(samples, 5.0)); // 시간 폭 밖이 된 사건은 버려지고 1 피해는 최소치 미만이다.
    std::vector<TestSpectatorSample> far{{1, 80, 1, 10.0F, 10.0F, 50}, {3, 80, 2, 12.0F, 10.0F, 99}, {4, 80, 2, 40.0F, 40.0F, 100}};
    spectator.Observe(far, 5.5); far[2].hitPoints = 40; // 4번이 새로 나타난 뒤 60 피해다.
    const auto moved = spectator.Observe(far, 6.0); CHECK(moved && moved->x == 40.0F && moved->y == 40.0F); // 유지 시간이 지나 먼 전투로 옮긴다.
    far[1].hitPoints = 60; spectator.Observe(far, 6.5); // 12,10의 39 피해는 유지 시간 안이라 옮기지 않는다.
    CHECK(spectator.Current()->x == 40.0F);
    std::vector<TestSpectatorSample> gone{{1, 80, 1, 10.0F, 10.0F, 50}, {3, 80, 2, 12.0F, 10.0F, 60}};
    const auto destroyed = spectator.Observe(gone, 11.0); // 40,40의 4번(HP40)이 사라졌고 유지 시간이 지났다.
    CHECK(!destroyed || (destroyed->x == 40.0F && destroyed->weight == 40.0) || std::abs(destroyed->x - 40.0F) < 6.0F); // 같은 곳이면 옮기지 않거나 같은 곳을 가리킨다.
}
// 관전 카메라: 피해 없이 사라진 객체(건설 완료 교체·내용물 진입 등)는 전투로 세지 않는다(실제 1-1 실행에서 찾은 오탐 보정).
TEST_CASE(CampaignTestSpectator_UndamagedVanishIgnored) {
    TestSpectatorSettings settings; settings.minimumWeight = 1.0; // 사건이 하나라도 생기면 목표가 나오게 한다.
    CampaignTestSpectator spectator(settings);
    std::vector<TestSpectatorSample> samples{{1, 80, 1, 10.0F, 10.0F, 100}, {2, 80, 1, 30.0F, 30.0F, 100}};
    spectator.Observe(samples, 0.0);
    samples.pop_back(); CHECK(!spectator.Observe(samples, 0.5)); // 피해 없는 2번의 사라짐은 무시한다.
    samples[0].hitPoints = 90; CHECK(spectator.Observe(samples, 1.0)); // 피해는 그대로 잡힌다.
    samples.clear(); const auto destroyed = spectator.Observe(samples, 6.0); // 피해 입은 1번이 사라짐(파괴 90→상한 60), 앞 사건(1.0초)도 시간 폭 안이다.
    CHECK(!destroyed); // 같은 위치(10,10)라 다시 옮기지 않는다.
}
// 관전 카메라: 희생 의식 소리는 유지 시간과 무관하게 제단을 우선하고, 끝난 뒤에는 전투로 돌아간다.
TEST_CASE(CampaignTestSpectator_RitualHasPriority) {
    CampaignTestSpectator spectator;
    std::vector<TestSpectatorSample> samples{{1, 80, 1, 10.0F, 10.0F, 100}, {2, 80, 2, 70.0F, 70.0F, 100}};
    spectator.Observe(samples, 0.0); samples[0].hitPoints = 40; CHECK(spectator.Observe(samples, 0.5)); // 전투(10,10)를 본다.
    spectator.NoteRitual(33.0F, 44.0F, 0.8);
    const auto altar = spectator.Observe(samples, 1.0); CHECK(altar && altar->ritual && altar->x == 33.0F && altar->y == 44.0F); // 유지 시간 안이어도 제단으로 간다.
    spectator.NoteRitual(33.0F, 44.0F, 2.0); CHECK(!spectator.Observe(samples, 2.5)); // 같은 제단을 이미 보고 있다.
    samples[1].hitPoints = 30; CHECK(!spectator.Observe(samples, 6.0)); // 의식 우선 시간(마지막 소리+5초) 안이다.
    samples[1].hitPoints = 0; const auto back = spectator.Observe(samples, 7.5); // 의식이 끝났고 70,70에서 큰 피해가 이어진다.
    CHECK(back && !back->ritual && back->x == 70.0F);
}
// 관전 판단기는 지정한 월드에서만 돌려준다(LAN·편집 등 다른 월드에서는 쓰지 않는다).
TEST_CASE(CampaignTestSpectator_BoundToWorld) {
    int worldA{}, worldB{};
    SetCurrentTestSpectator(std::make_shared<CampaignTestSpectator>(), &worldA);
    CHECK(CurrentTestSpectator(&worldA) != nullptr && CurrentTestSpectator(&worldB) == nullptr && CurrentTestSpectator(nullptr) == nullptr);
    SetCurrentTestSpectator(nullptr, nullptr); CHECK(CurrentTestSpectator(&worldA) == nullptr);
}
