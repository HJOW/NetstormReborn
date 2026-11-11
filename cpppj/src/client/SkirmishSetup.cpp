// 12차 스커미시: 설정 저장, Tell 대상 해석, 화면 본문, 시작 대본/맵 선택의 구현이다.
#include "client/SkirmishSetup.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <numeric>
#include <random>
#include "o/CanonDecoder.h"

namespace netstorm::client { // 스커미시 로직의 구현이다.
namespace { // 이 파일에서만 쓰는 표와 작은 도우미다.
// 금지 유닛 후보 표다. 내부 타입 이름은 techAllowed가 쓰는 원문 이름이고 영어 이름은 게임 안 표기를 따른다(일부 짝은 이름 규칙에서 추정).
constexpr std::array<SkirmishBannable, 27> kBannable{{
    {"windBattery", "Wind Generator"}, {"rainBattery", "Rain Generator"}, {"thunderBattery", "Thunder Generator"},
    {"sunArcher", "Sun Disc Thrower"}, {"windArcher", "Crossbow"}, {"thunderArcher", "Vander Tower"},
    {"sunCannon", "Sun Cannon"}, {"rainCannon", "Ice Cannon"}, {"thunderCannon", "Thunder Cannon"},
    {"sunBlocker", "Stone Tower"}, {"windBlocker", "Wind Tower"}, {"rainBlocker", "Ice Tower"}, {"thunderBlocker", "Bulwark"},
    {"sunFence", "Sun Barricade"}, {"rainFence", "Acid Barricade"}, {"thunderFence", "Arc Spire"},
    {"sunBalloon", "Whirligig"}, {"windBalloon", "Dust Devil"}, {"rainBalloon", "Man o'War"},
    {"sunAviary", "Whirlibase"}, {"windAviary", "Devil Maker"}, {"rainAviary", "Man o'War Pool"},
    {"sunWalker", "Fire Ant"}, {"windWalker", "Sail Skater"}, {"rainWalker", "Crystal Crab"},
    {"bulf", "Bulf"}, {"outpost", "Outpost"}}};
// 시작 섬 후보 표를 한 번만 만든다(SkirmishStartIslands.inc, 자동 생성).
const std::vector<SkirmishStartIsland>& IslandPool() {
    static const std::vector<SkirmishStartIsland> pool{
#include "client/SkirmishStartIslands.inc"
    };
    return pool;
}
// 화면 문구의 언어 구분이다.
enum class Lang { English, Korean, Japanese };
// 원문 언어 번호를 화면 문구 언어로 바꾼다(5 일본어, 7 한국어, 그 밖은 영어).
Lang LangOf(std::uint32_t language) { return language == 7U ? Lang::Korean : language == 5U ? Lang::Japanese : Lang::English; }
// 언어별 고정 문구 묶음이다. {n}/{a}/{b}/{c}는 호출자가 값을 채운다.
struct Words {
    std::string_view title, intro, countLine, aiLine, bansLine, start, cancel, back, countTitle, countIntro, aiTitle, aiIntro,
        bansTitle, bansIntro, easy, normal, hard, enemy, ally, next, previous, clear, maxNote;
};
// 영어 문구다.
constexpr Words kEnglish{"Skirmish", "Choose your opponents and press Start. Islands and geysers are random.", "AI players: {n}",
    "AI {a}: {b} / {c}", "Banned units: {n}/5", "Start", "Cancel", "Back", "Number of AI players", "Pick how many AI players join this game.",
    "AI {a}", "Choose this AI's difficulty and side.", "Banned units", "Select up to 5 units or buildings that nobody may build. Selected items are marked [X].",
    "Easy", "Normal", "Hard", "Enemy", "Ally", "Next page", "Previous page", "Clear all", "Five units are banned. Unmark one to choose another."};
// 한국어 문구다(건물·유닛 이름은 번역 hook이 맡는다).
constexpr Words kKorean{"스커미시", "상대를 고른 뒤 시작을 누르세요. 섬과 간헐천은 무작위입니다.", "인공지능 수: {n}",
    "AI {a}: {b} / {c}", "금지 유닛: {n}/5", "시작", "취소", "뒤로", "인공지능 수", "이 게임에 참가할 인공지능 수를 고르세요.",
    "AI {a}", "이 인공지능의 난이도와 진영을 고르세요.", "금지 유닛", "아무도 지을 수 없는 건물·유닛을 최대 5개까지 고르세요. 고른 항목은 [X]로 표시됩니다.",
    "쉬움", "보통", "어려움", "적", "동맹", "다음 쪽", "이전 쪽", "모두 해제", "금지 유닛이 5개입니다. 다른 것을 고르려면 하나를 해제하세요."};
// 일본어 문구다.
constexpr Words kJapanese{"スカーミッシュ", "対戦相手を選んで<br>開始を押してください。<br>島と間欠泉は<br>ランダムです。", "AI人数: {n}",
    "AI {a}: {b} / {c}", "禁止ユニット: {n}/5", "開始", "キャンセル", "戻る", "AIの人数", "このゲームに参加する<br>AIの人数を選んでください。",
    "AI {a}", "このAIの難易度と<br>陣営を選んでください。", "禁止ユニット", "誰も建てられない建物や<br>ユニットを最大5つまで<br>選びます。選んだ項目は<br>[X]で表示されます。",
    "易しい", "通常", "硬い", "敵", "同盟", "次のページ", "前のページ", "すべて解除", "禁止ユニットが5つです。<br>別のものを選ぶには<br>1つ解除してください。"};
// 언어에 맞는 문구 묶음을 돌려준다.
const Words& WordsFor(std::uint32_t language) { const auto lang = LangOf(language); return lang == Lang::Korean ? kKorean : lang == Lang::Japanese ? kJapanese : kEnglish; }
// 문구의 {a}/{b}/{c}/{n} 자리를 값으로 바꾼다.
std::string Fill(std::string_view text, std::string_view a = {}, std::string_view b = {}, std::string_view c = {}, std::string_view n = {}) {
    std::string result(text);
    const std::array<std::pair<std::string_view, std::string_view>, 4> slots{{{"{a}", a}, {"{b}", b}, {"{c}", c}, {"{n}", n}}};
    // 네 자리표시자를 차례로 치환한다.
    for (const auto& [key, value] : slots) {
        for (std::size_t at = result.find(key); at != std::string::npos; at = result.find(key, at + value.size())) result.replace(at, key.size(), value); // 같은 자리가 여러 번 나와도 모두 바꾼다.
    }
    return result;
}
// 난이도의 화면 이름이다.
std::string_view DifficultyName(const Words& words, o::AiDifficulty level) { return level == o::AiDifficulty::Easy ? words.easy : level == o::AiDifficulty::Hard ? words.hard : words.normal; }
// 정수를 문자열로 만든다.
std::string Str(int value) { return std::to_string(value); }
// 소유자 번호(2..8)에 대응하는 설정 칸 번호(0..6)이다.
std::size_t SlotOf(int owner) { return static_cast<std::size_t>(std::clamp(owner, 2, 8) - 2); }
// 쉼표로 구분한 문자열을 나눈다(빈 칸은 버린다).
std::vector<std::string> SplitCommas(std::string_view text) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    // 각 쉼표 사이 조각을 모은다.
    while (start <= text.size()) {
        const auto comma = text.find(',', start);
        const auto piece = text.substr(start, comma == std::string_view::npos ? std::string_view::npos : comma - start);
        if (!piece.empty()) parts.emplace_back(piece);
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    return parts;
}
// 정수로 해석하며 실패하면 fallback이다.
int ToInt(std::string_view text, int fallback) { int value{}; const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value); return error == std::errc{} && end == text.data() + text.size() ? value : fallback; }
// 내부 타입 이름이 금지 후보인지 판정한다.
bool IsBannable(std::string_view type) { return std::any_of(kBannable.begin(), kBannable.end(), [&](const auto& item) { return item.type == type; }); }
// 소문자로 바꾼다.
std::string Lower(std::string_view text) { std::string result(text); for (auto& c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return result; }
// 창 공통 머리(배경·제목·본문 시작)이다.
std::string Head(std::string_view title, std::string_view intro) { return "<background=\"nsstart\">\n<h2>{H2}" + std::string(title) + "</h2>\n<p>{Text}\n" + std::string(intro) + "<br>\n"; }
}
// 후보 표를 span으로 돌려준다.
std::span<const SkirmishBannable> SkirmishBannableTypes() { return kBannable; }
// 시작 섬 후보 표를 span으로 돌려준다.
std::span<const SkirmishStartIsland> SkirmishStartIslandPool() { return IslandPool(); }
// 설정을 읽는다.
SkirmishSettings LoadSkirmishSettings(const SkirmishConfigGet& get) {
    SkirmishSettings settings;
    settings.aiCount = std::clamp(ToInt(get("skirmishAiCount"), 1), 1, kSkirmishMaxAi);
    // 소유자 2..8의 난이도/동맹을 읽는다.
    for (int owner = 2; owner <= 8; ++owner) {
        auto& ai = settings.ai[SlotOf(owner)];
        ai.difficulty = o::AiDifficultyFromValue(static_cast<std::uint32_t>(ToInt(get("skirmishAi" + Str(owner) + "Diff"), 2)));
        ai.ally = ToInt(get("skirmishAi" + Str(owner) + "Ally"), 0) != 0;
    }
    // 저장된 금지 목록에서 후보에 있는 항목만 최대 5개 받는다.
    for (const auto& type : SplitCommas(get("skirmishBans")))
        if (settings.bans.size() < kSkirmishMaxBans && IsBannable(type) && std::find(settings.bans.begin(), settings.bans.end(), type) == settings.bans.end()) settings.bans.push_back(type);
    return settings;
}
// 설정을 쓴다.
void SaveSkirmishSettings(const SkirmishConfigSet& set, const SkirmishSettings& settings) {
    set("skirmishAiCount", Str(settings.aiCount));
    // 소유자 2..8의 난이도/동맹을 쓴다.
    for (int owner = 2; owner <= 8; ++owner) {
        const auto& ai = settings.ai[SlotOf(owner)];
        set("skirmishAi" + Str(owner) + "Diff", Str(static_cast<int>(o::AiDifficultyToValue(ai.difficulty))));
        set("skirmishAi" + Str(owner) + "Ally", ai.ally ? "1" : "0");
    }
    std::string bans;
    // 금지 목록을 쉼표로 잇는다.
    for (const auto& type : settings.bans) { if (!bans.empty()) bans += ','; bans += type; }
    set("skirmishBans", bans);
}
// Tell 대상 문자열을 해석한다.
SkirmishTarget ParseSkirmishTarget(std::string_view target) {
    SkirmishTarget result;
    const auto lower = Lower(target);
    if (lower.rfind(kSkirmishTargetPrefix, 0) != 0) return result;
    auto rest = std::string_view(lower).substr(kSkirmishTargetPrefix.size()); // "", "count", "ai(2)", "set(ban,1,0)" 등이다.
    std::string args;
    if (const auto open = rest.find('('); open != std::string_view::npos) {
        const auto close = rest.rfind(')');
        if (close != std::string_view::npos && close > open) args = std::string(rest.substr(open + 1, close - open - 1));
        rest = rest.substr(0, open);
    }
    result.args = SplitCommas(args);
    if (rest.empty()) result.kind = SkirmishScreen::Main;
    else if (rest == "count") result.kind = SkirmishScreen::Count;
    else if (rest == "ai") result.kind = SkirmishScreen::Ai;
    else if (rest == "bans") result.kind = SkirmishScreen::Bans;
    else if (rest == "set") result.kind = SkirmishScreen::Set;
    else if (rest == "start") result.kind = SkirmishScreen::Start;
    return result;
}
// Set 동작을 적용하고 다음 화면 대상을 돌려준다.
std::string ApplySkirmishAction(SkirmishSettings& settings, const std::vector<std::string>& args) {
    const std::string main = "Patch1101.Skirmish";
    if (args.empty()) return main;
    const auto& verb = args[0];
    if (verb == "count" && args.size() >= 2) { settings.aiCount = std::clamp(ToInt(args[1], settings.aiCount), 1, kSkirmishMaxAi); return main; }
    if ((verb == "diff" || verb == "ally") && args.size() >= 3) {
        const int owner = std::clamp(ToInt(args[1], 2), 2, 8);
        auto& ai = settings.ai[SlotOf(owner)];
        if (verb == "diff") ai.difficulty = o::AiDifficultyFromValue(static_cast<std::uint32_t>(ToInt(args[2], 2)));
        else ai.ally = ToInt(args[2], 0) != 0;
        return main + "Ai(" + Str(owner) + ")"; // 같은 AI 화면에서 이어서 고른다.
    }
    if (verb == "ban" && args.size() >= 3) {
        const auto index = static_cast<std::size_t>(std::max(0, ToInt(args[1], 0)));
        if (index < kBannable.size()) {
            const std::string type(kBannable[index].type);
            const auto found = std::find(settings.bans.begin(), settings.bans.end(), type);
            if (found != settings.bans.end()) settings.bans.erase(found);                  // 이미 금지면 해제한다.
            else if (settings.bans.size() < kSkirmishMaxBans) settings.bans.push_back(type); // 6번째 선택은 무시한다(최대 5개).
        }
        return main + "Bans(" + Str(std::max(0, ToInt(args[2], 0))) + ")";
    }
    if (verb == "clearbans") { settings.bans.clear(); return main + "Bans(0)"; }
    return main;
}
// 화면 본문을 만든다.
std::string BuildSkirmishScreen(const SkirmishTarget& target, const SkirmishSettings& settings, std::uint32_t language,
    const std::function<std::string(std::string_view)>& translate) {
    const auto& words = WordsFor(language);
    const auto side = [&](bool ally) { return ally ? words.ally : words.enemy; };
    std::string body;
    switch (target.kind) {
    case SkirmishScreen::Count: { // AI 수 고르기
        body = Head(words.countTitle, words.countIntro);
        // 1..7 항목을 만들고 현재 값에 표시한다.
        for (int count = 1; count <= kSkirmishMaxAi; ++count) body += std::string("$Menu=") + (count == settings.aiCount ? "* " : "") + Str(count) + ",Tell,Patch1101.SkirmishSet(count," + Str(count) + ")\n";
        body += "$Button=" + std::string(words.back) + ",Tell,Patch1101.Skirmish\n";
        break;
    }
    case SkirmishScreen::Ai: { // 한 AI의 난이도/진영 고르기
        const int owner = std::clamp(target.args.empty() ? 2 : ToInt(target.args[0], 2), 2, 8);
        const auto& ai = settings.ai[SlotOf(owner)];
        body = Head(Fill(words.aiTitle, Str(owner - 1)), words.aiIntro);
        // 세 난이도 항목이다.
        for (const auto level : {o::AiDifficulty::Easy, o::AiDifficulty::Normal, o::AiDifficulty::Hard})
            body += std::string("$Menu=") + (level == ai.difficulty ? "* " : "") + std::string(DifficultyName(words, level)) + ",Tell,Patch1101.SkirmishSet(diff," + Str(owner) + "," + Str(static_cast<int>(o::AiDifficultyToValue(level))) + ")\n";
        // 두 진영 항목이다.
        for (const bool ally : {false, true})
            body += std::string("$Menu=") + (ally == ai.ally ? "* " : "") + std::string(side(ally)) + ",Tell,Patch1101.SkirmishSet(ally," + Str(owner) + "," + (ally ? "1" : "0") + ")\n";
        body += "$Button=" + std::string(words.back) + ",Tell,Patch1101.Skirmish\n";
        break;
    }
    case SkirmishScreen::Bans: { // 금지 유닛 고르기
        const auto pageCount = (kBannable.size() + kSkirmishBansPerPage - 1) / kSkirmishBansPerPage;
        const auto page = std::min<std::size_t>(target.args.empty() ? 0 : static_cast<std::size_t>(std::max(0, ToInt(target.args[0], 0))), pageCount - 1);
        body = Head(words.bansTitle, settings.bans.size() >= kSkirmishMaxBans ? words.maxNote : words.bansIntro);
        // 이 쪽의 항목을 만든다. 금지 상태는 [X]/[ ]로 표시하고 내부 이름을 함께 보인다.
        for (std::size_t index = page * kSkirmishBansPerPage; index < std::min(kBannable.size(), (page + 1) * kSkirmishBansPerPage); ++index) {
            const bool banned = std::find(settings.bans.begin(), settings.bans.end(), std::string(kBannable[index].type)) != settings.bans.end();
            const auto name = translate ? translate(kBannable[index].label) : std::string(kBannable[index].label);
            body += std::string("$Menu=") + (banned ? "[X] " : "[ ] ") + name + " - " + std::string(kBannable[index].type) + ",Tell,Patch1101.SkirmishSet(ban," + Str(static_cast<int>(index)) + "," + Str(static_cast<int>(page)) + ")\n";
        }
        if (page + 1 < pageCount) body += "$Menu=" + std::string(words.next) + ",Tell,Patch1101.SkirmishBans(" + Str(static_cast<int>(page + 1)) + ")\n";
        if (page > 0) body += "$Menu=" + std::string(words.previous) + ",Tell,Patch1101.SkirmishBans(" + Str(static_cast<int>(page - 1)) + ")\n";
        body += "$Menu=" + std::string(words.clear) + ",Tell,Patch1101.SkirmishSet(clearbans)\n$Button=" + std::string(words.back) + ",Tell,Patch1101.Skirmish\n";
        break;
    }
    default: { // 첫 설정 화면
        body = Head(words.title, words.intro);
        body += "$Menu=" + Fill(words.countLine, {}, {}, {}, Str(settings.aiCount)) + ",Tell,Patch1101.SkirmishCount\n";
        // 선택한 수만큼 AI 줄을 만든다(소유자 2부터).
        for (int owner = 2; owner < 2 + settings.aiCount; ++owner) {
            const auto& ai = settings.ai[SlotOf(owner)];
            body += "$Menu=" + Fill(words.aiLine, Str(owner - 1), DifficultyName(words, ai.difficulty), side(ai.ally)) + ",Tell,Patch1101.SkirmishAi(" + Str(owner) + ")\n";
        }
        body += "$Menu=" + Fill(words.bansLine, {}, {}, {}, Str(static_cast<int>(settings.bans.size()))) + ",Tell,Patch1101.SkirmishBans(0)\n";
        body += "$Button=" + std::string(words.start) + ",Tell,Patch1101.SkirmishStart\n$Button=" + std::string(words.cancel) + ",Tell,UCampaign\n";
        break;
    }
    }
    return body;
}
// 시작 자료(맵과 대본)를 만든다.
SkirmishLaunch BuildSkirmishLaunch(const SkirmishSettings& settings, std::uint32_t randomValue) {
    SkirmishLaunch launch;
    const auto& pool = IslandPool();
    const auto players = static_cast<std::size_t>(std::clamp(settings.aiCount, 1, kSkirmishMaxAi) + 1);
    if (pool.size() < players) return launch;
    std::mt19937 random(randomValue);
    // 후보 표에서 참가자 수만큼 서로 다른 시작 섬을 뽑는다(소유자 1..N+1 순서, MP에서 참가자마다 자기 fort 영역을 쓰는 것과 같다).
    std::vector<std::size_t> order(pool.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::shuffle(order.begin(), order.end(), random);
    for (std::size_t slot = 0; slot < players; ++slot) launch.islands.push_back(pool[order[slot]]);
    // 참가자가 많을수록 링 간격이 넓은 맵 크기를 쓴다(2명 간격4, 3~4명 간격5, 5명 이상 간격6).
    launch.mapSize = players <= 2 ? 1 : players <= 4 ? 2 : 3;
    // 중앙섬은 3x3 패턴 가운데서 무작위로 고르고 회전/씨앗도 무작위다(MP의 $Random 중앙섬 기록과 같은 형식).
    std::vector<std::uint8_t> bigPatterns;
    const auto patterns = o::TerritoryPatterns();
    for (std::size_t index = 0; index < patterns.size(); ++index) if (patterns[index].width == 3 && patterns[index].height == 3) bigPatterns.push_back(static_cast<std::uint8_t>(index));
    const auto pattern = bigPatterns.empty() ? std::uint8_t{0} : bigPatterns[random() % bigPatterns.size()];
    launch.central = {static_cast<std::uint8_t>(pattern | ((random() & 3U) << 6)), 0x09, 0x00, static_cast<std::uint8_t>(random()), 0x00, 0x00};
    std::string script = "[Header]\nmissionType=\"Tutorial\"\n\nmissionNumber=1\nmoreGeysers=1\nrandGeysers=1\n\nmyStartMoney=5000\nmyTech=\"all\"\n";
    if (!settings.bans.empty()) {
        script += "techAllowed=\"deny";
        // 금지 유닛을 techAllowed 목록에 잇는다.
        for (const auto& type : settings.bans) script += ";" + type;
        script += "\"\n";
    }
    std::string allies;
    // 플레이어와 같은 편인 AI 소유자를 모은다.
    for (int owner = 2; owner < 2 + settings.aiCount; ++owner)
        if (settings.ai[SlotOf(owner)].ally) { if (!allies.empty()) allies += ';'; allies += Str(owner); }
    script += "myAllyList=\"" + allies + "\"\ntitle=\"Skirmish\"\n";
    // 각 AI의 기본 설정과 같은 편/적 편 동맹 목록을 만든다(같은 편끼리 동맹이다).
    for (int owner = 2; owner < 2 + settings.aiCount; ++owner) {
        const bool ally = settings.ai[SlotOf(owner)].ally;
        std::string list = "\"";
        bool first = true;
        // 이 AI와 같은 편인 모든 소유자(자신 포함, 플레이어 편이면 1도 포함)를 나열한다.
        for (int other = 1; other < 2 + settings.aiCount; ++other) {
            const bool sameSide = other == owner || (other == 1 ? ally : (other >= 2 && settings.ai[SlotOf(other)].ally == ally));
            if (!sameSide) continue;
            if (!first) list += ';';
            list += Str(other);
            first = false;
        }
        list += "\"";
        script += "\nai" + Str(owner) + "Name=\"Computer #" + Str(owner - 1) + "\"\nai" + Str(owner) + "Tech=\"all\"\nai" + Str(owner) + "StartMoney=5000\n";
        script += "ai" + Str(owner) + "GeyserAttachments=3\nai" + Str(owner) + "Collectors=6\nai" + Str(owner) + "StuffRefreshRate=1\nai" + Str(owner) + "TimeBetweenMoves=2\n";
        script += "ai" + Str(owner) + "BridgeDrawRate=1\nai" + Str(owner) + "AllyList=" + list + "\n";
    }
    script += "\n[Options]\n\n[A.]\n<h2>Skirmish</h2>\n<i>" + Str(settings.aiCount) + " AI</i>\n\n$Button=Go!,DoNothing,0\n\n"
        "[Succeeded][BadTeamDead]\n<h2>Victory!</h2>\n<p>\nEvery enemy has been defeated.\n$Button=Leave,MissionAbort,0\n\n"
        "[Failed][GoodTeamDead]\n<h2>Defeat</h2>\n<p>\nYour team has fallen.\n$Button=Leave,MissionAbort,0\n\n[END]\n";
    launch.script = std::move(script);
    return launch;
}
// 스커미시용 빈 월드 fort를 만든다: 섹션 35개 중 Chaff(4)/Territory(6)/Terr00..19(15..34)만 비우고 나머지는 기준 fort 그대로 둔다.
// 12차에서 월드 배치를 기존 맵 선택에서 MP 시작 규칙으로 바꾸면서 추가했다(섬은 시작 때 참가자별로 만든다).
std::vector<std::uint8_t> MakeSkirmishEmptyWorldFort(std::span<const std::uint8_t> baseFort) {
    constexpr std::size_t kSections = 35, kChaff = 4, kTerritory = 6, kFirstTerr = 15, kTerritoryBytes = 120, kChunkRecords = 256; // 섹션 수와 각 섹션 위치/크기(원본 저장 순서)다.
    constexpr std::uint8_t kChunkMark = 99;                                                                                         // 청크 레코드 표지(0x63)다.
    if (baseFort.size() < 2 || baseFort[0] != 'F') return {};
    std::vector<std::vector<std::uint8_t>> sections;
    std::size_t position = 2;
    // 각 섹션의 [길이(자기 2바이트 포함)][본문]을 읽는다.
    while (sections.size() < kSections) {
        if (position + 2 > baseFort.size()) return {};
        const std::size_t length = static_cast<std::size_t>(baseFort[position]) | (static_cast<std::size_t>(baseFort[position + 1]) << 8);
        if (length < 2 || position + length > baseFort.size()) return {};
        sections.emplace_back(baseFort.begin() + static_cast<std::ptrdiff_t>(position + 2), baseFort.begin() + static_cast<std::ptrdiff_t>(position + length));
        position += length;
    }
    sections[kChaff].assign(1, 2);                                                  // 버전 2 뒤에 빈 청크 레코드 256개(표지, 개수0)를 둔다.
    for (std::size_t chunk = 0; chunk < kChunkRecords; ++chunk) sections[kChaff].insert(sections[kChaff].end(), {kChunkMark, 0, 0});
    sections[kTerritory].assign(kTerritoryBytes, 0);                                // 영역 표를 모두 비운다(섬 없음).
    for (std::size_t index = kFirstTerr; index < kSections; ++index) sections[index].assign(1, 2); // Terr 섹션은 버전만 남긴다.
    std::vector<std::uint8_t> result{baseFort[0], baseFort[1]};
    // 각 섹션을 [길이][본문]으로 다시 쓴다.
    for (const auto& body : sections) {
        const auto length = body.size() + 2;
        result.push_back(static_cast<std::uint8_t>(length & 0xff));
        result.push_back(static_cast<std::uint8_t>(length >> 8));
        result.insert(result.end(), body.begin(), body.end());
    }
    return result;
}
// 이름 비교는 대소문자를 무시한다.
bool IsSkirmishMissionName(std::string_view name) { return Lower(name) == Lower(kSkirmishMissionName); }
}
