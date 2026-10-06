// 원본 RiftType.cpp의 타입 표: 0049ebb0 ↔ CD 004434d0(배열 초기화·내장 이름), 0049c3b0 ↔ CD 004460f0(플래그 단어·속성),
//       0049b0d0 ↔ CD 00444e10(후처리), 0049a860 ↔ CD 00443410(이름 검색).
// 범위: 타입 번호 체계·이름·플래그 1/2·종류 단어·그룹·목록 플래그·발자국·초기 HP/깊이·생성자 주소 기록.
//       파생 생성자 실행, 사거리·비용 등 전투 수치·요구 에너지 문자열(+0xa0)·최대 사거리 집계는 후속이다.
// 검증: 이름 해시 표가 실제 .fort의 `TypeNames` 섹션과 같음을 tools/cpp_fort_smoke.py가 두 판본에서 확인한다.
#include "o/RiftType.h"
#include "o/OriginalText.h"
#include <array>
#include <cmath>
#include <bit>
#include <regex>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 패치판 타입 수(DAT_00541348의 초기값 188)와 CD판 타입 수(DAT_0051cbf0 = 171).
constexpr std::size_t kPatchTypeCount = 188;
constexpr std::size_t kCdTypeCount = 171;
// 내장 프로세스 타입의 첫 번호(원본 루프의 시작값 10).
constexpr std::size_t kFirstProcessTypeNumber = 10;
// 타입 구조체의 이름 필드 길이(+4~+0x17). 프로세스 타입 이름은 strncpy로 이 길이까지만 들어간다.
constexpr std::size_t kTypeNameBytes = 20;
// 설명 필드(+0x18~+0x3f)에 strncpy로 복사하는 최대 글자 수.
constexpr std::size_t kDescriptionBytes = 40;
// 종류 단어 필드(+0xf0)에 strncpy로 복사하는 최대 글자 수.
constexpr std::size_t kKindBytes = 8;

// 내장 프로세스 타입 이름(패치 00540fa0의 53개). CD판(0051c890)은 마지막 하나가 없는 앞 52개다.
constexpr std::array<std::string_view, 53> kProcessTypeNames{
    "evilRaySystemProcessType", "fieryTailSystemProcessType", "streakSystemProcessType", "ropeSystemProcessType",
    "burningSystemProcessType", "kaboomSystemProcessType", "flyingShrapnelSystemProcessType", "buildSystemProcessType",
    "dissolveSystemProcessType", "lightningSystemProcessType", "beamOverSystemProcessType", "smokeSystemProcessType",
    "fountainSystemProcessType", "teleportEffectProcessType", "dudeProcessType", "canonRealizeProcessType",
    "rookAttackProcessType", "targetProcessType", "missileProcessType", "pingPongProcessType",
    "constructionProcessType", "baseFlyerProcessType", "sunAviaryProcessType", "islandDropperProcessType",
    "gunProcessType", "fireAnimProcessType", "rotateAnimProcessType", "scrollTestProcessType",
    "simpleWalkerProcessType", "simpleFlyProcessType", "wormProcessType", "bulfProcessType",
    "balloonProcessType", "ambleProcessType", "driftProcessType", "rainVortexAnimProcessType",
    "regularProcessType", "soundProcessType", "windBatteryProcessType", "buildPlaybackProcessType",
    "manaDisplayProcessType", "rangeDisplayProcessType", "islandAnimatorProcessType", "cameraAnimatorProcessType",
    "priestProcessType", "paletteFlasherProcessType", "thunderArcherSystemProcessType", "bombEffectTimerProcessType",
    "daisProcessType", "windAviaryProcessType", "rainAviaryProcessType", "sharedRegularProcessType",
    "thunderAviaryProcessType",
};
// CD판 프로세스 타입 이름 수.
constexpr std::size_t kCdProcessTypeCount = 52;

// 후처리(0049b0d0)가 이름을 채우는 내장 타입: 번호와 이름(패치 DAT_00541074~88, CD DAT_0051c964~78). 두 판본이 같다.
struct BuiltinName { std::size_t number; std::string_view name; };
constexpr std::array<BuiltinName, 5> kBuiltinNames{{
    {1, "DependForm"}, {2, "ProcessForm"}, {3, "GumpForm"}, {4, "PlayerSquid"}, {6, "ContentForm"}}};

// `group` 속성의 이름 표(패치 00542468). 번호 = 배열 위치. 없는 이름은 원본처럼 -1이다.
constexpr std::array<std::string_view, 10> kGroupNames{
    "archer", "cannon", "blocker", "aviary", "flyer", "battery", "fence", "walker", "balloon", "misc"};

// 플래그 단어 하나가 켜는 비트. kind가 참이면 단어를 종류 필드(+0xf0)에 복사한다.
struct FlagWord { std::string_view word; std::uint32_t flags1; std::uint32_t flags2; bool kind; };
// 원본 0049c3b0의 단어 비교 순서 그대로다. 앞 20개는 서로 독립이고 뒤는 if-else 사슬이지만 단어가 겹치지 않는다.
constexpr std::array<FlagWord, 48> kFlagWords{{
    {"carribleInVehicle", 0x10000, 0, false}, {"container", 0x20000, 0, false},
    {"default_hotspot", 0x1, 0, false}, {"defaulthotspot", 0x1, 0, false},
    {"shadow", 0x40000, 0, false}, {"flyershadow", 0x400000, 0, false},
    {"not_selectable", 0x8000000, 0, false}, {"notselectable", 0x8000000, 0, false},
    {"not_real", 0x28000000, 0x2000000, false}, {"notreal", 0x28000000, 0x2000000, false},
    {"createsisland", 0x400, 0, false}, {"mayDropOnRim", 0x4, 0, false}, {"mayDropOnIsle", 0x2, 0, false},
    {"surface", 0x800, 0, false}, {"randframe", 0x100000, 0, false}, {"matchframe", 0x200000, 0, false},
    {"opaqueCollide", 0x800000, 0, false}, {"dontSave", 0x20000000, 0, false},
    {"saveFrame", 0x20, 0, false}, {"saveQA", 0x1000, 0, false}, {"saveQB", 0x2000, 0, false},
    {"dropblocking", 0, 0x10, false}, {"blocking", 0, 0x18, false}, {"shotblocking", 0, 0x20, false},
    {"fencePost", 0, 0x1, false}, {"fence", 0, 0x20000000, false},
    {"buried", 0, 0x2000, true}, {"nugget", 0, 0x8000, true}, {"bomb", 0, 0x100, true},
    {"emplacement", 0, 0x40000, false}, {"bridge", 0, 0x4, true}, {"walker", 0, 0x10000, true},
    {"balloon", 0, 0x20000, true}, {"island", 0, 0x2, true}, {"islandThreeByThree", 0, 0x1000000, false},
    {"factory", 0, 0x4000, true}, {"vortex", 0, 0x200, true}, {"edgefarm", 0, 0x80000, true},
    {"focus", 0, 0, true}, {"guy", 0, 0x400, true}, {"geyser", 0, 0x10000000, true},
    {"flyer", 0, 0x100000, false}, {"residence", 0, 0x40000000, true}, {"fringe", 0, 0x1000, false},
    {"priest", 0, 0x200000, false}, {"dais", 0, 0x400000, false}, {"altar", 0, 0x80000000, false},
    // `predictable`은 +0x1e8에 99를 OR 한다. 그 필드는 아직 옮기지 않아 비트 없이 둔다.
    {"predictable", 0, 0, false},
}};

// ASCII 대소문자를 무시한 비교(원본 stricmp).
bool Equal(std::string_view left, std::string_view right) { return AsciiLower(left) == AsciiLower(right); }

// 원본 FUN_004e49c0(float → int, 0쪽으로 버림)에 해당하는 변환. 범위를 벗어나면 0으로 둔다.
std::int32_t ToInt(double value) {
    if (!(value > -2147483649.0 && value < 2147483648.0)) return 0;
    return static_cast<std::int32_t>(value);
}

// 패치 00540d10 / CD 0051c630의 23개 깊이 이름/값은 두 PE에서 같다.
struct ZOrderName { std::string_view name; std::int32_t value; };
constexpr std::array<ZOrderName,23> kZOrders{{
    {"zoNONE",-127},{"zoFALLING",30},{"zoSTALAG",20},{"zoCHALRING",20},{"zoBATTLE",10},
    {"zoEDGEFARM",0},{"zoISLAND",0},{"zoBRIDGE",0},{"zoBRIDGE_CONNECTOR",-10},{"zoARTIFACTS",-15},
    {"zoEMPLACEMENTS",-20},{"zoILLEGAL_DITHER",-21},{"zoFLARES",-22},{"zoMISSILES",-25},
    {"zoFLYER_SHADOWS",-26},{"zoFLYERS",-30},{"zoFENCE",-35},{"zoMANAICON",-36},
    {"zoUBERGUMP",-40},{"zoMENUGUMP",-60},{"zoDIALOGGUMP",-80},{"zoLOOKGUMP",-100},{"zoRISING",-31}}};
// 원본 문자열 zorder의 첫 식별자 검색과 이름 표의 부분 문자열 비교, 선택적 +/- 숫자를 처리한다.
std::int32_t ZOrder(std::string_view text) {
    // 원본 0049c3b0의 두 정규식이다. 깊이 이름은 stricmp와 달리 대소문자를 구분한다.
    static const std::regex identifier("([A-Za-z0-9_]+)[ \\t]*");
    static const std::regex expression("[ \\t]*([A-Za-z0-9_]+)[ \\t]*(\\+|-)[ \\t]*([0-9]+)");
    const std::string input(text); std::smatch word,offset;
    if (!std::regex_search(input,word,identifier)) throw std::invalid_argument("zorder 이름이 없습니다");
    // 원본은 표 순서대로 strstr(표 이름, 입력 식별자)의 첫 일치를 선택한다.
    for (const auto& item:kZOrders) {
        if (item.name.find(word[1].str())==std::string_view::npos) continue;
        auto value=static_cast<std::uint32_t>(item.value);
        if (std::regex_search(input,offset,expression)) {
            const auto delta=std::stoull(offset[3].str());
            if (delta>2147483647) throw std::out_of_range("zorder 오프셋 범위 오류");
            value=offset[2].str()=="+" ? value+static_cast<std::uint32_t>(delta) : value-static_cast<std::uint32_t>(delta);
        }
        return std::bit_cast<std::int32_t>(value);
    }
    throw std::invalid_argument("알 수 없는 zorder 이름");
}

// 종류 필드에 strncpy(dst, word, min(strlen, 8))로 덮어쓴다. 짧은 단어는 이전 내용의 뒤가 남는다(원본 그대로).
void CopyKind(std::array<char, kKindBytes>& kind, std::string_view word) {
    // 최대 8글자만 덮어쓰고 종료 문자는 쓰지 않는다.
    for (std::size_t i = 0; i < kKindBytes && i < word.size(); ++i) kind[i] = word[i];
}

// 0049c3b0의 플래그·속성 부분: 정의에서 타입 구조체의 필드를 채운다.
void ApplyDefinition(RiftTypeRecord& type, const RiftTypeDefinition& definition) {
    type.footX = 1;
    type.footY = 1;
    type.flags1 = 0;
    type.flags2 = 0;
    std::array<char, kKindBytes> kind{};
    // 플래그 단어마다 표의 비트를 켠다. 표에 없는 단어는 원본처럼 조용히 지나간다.
    for (const auto& word : definition.flags) {
        if (Equal(word, "dontdrawdefault")) throw std::runtime_error("Type flag dontdrawdefault is rejected by the original");
        // 단어 하나는 표의 한 항목에만 맞는다.
        for (const auto& entry : kFlagWords) {
            if (!Equal(word, entry.word)) continue;
            type.flags1 |= entry.flags1;
            type.flags2 |= entry.flags2;
            if (entry.kind) CopyKind(kind, word);
        }
    }
    // 원본은 shadow와 flyershadow가 함께 있으면 assert 한다.
    if ((type.flags1 & 0x40000) != 0 && (type.flags1 & 0x400000) != 0)
        throw std::runtime_error("!(ts->typeFlags & TYPE_FLAG_FLYER_SHADOW)");
    bool hasDescription = false;
    // 속성은 파일에 나온 순서대로 적용한다. 같은 이름이 여러 번이면 뒤의 값이 남는다.
    for (const auto& property : definition.properties) {
        const auto* number = std::get_if<double>(&property.value);
        const auto* text = std::get_if<std::string>(&property.value);
        const auto is = [&property](std::string_view name) { return Equal(property.name, name); };
        if (is("maxHitPoints")) {
            if (!number) throw std::invalid_argument("maxHitPoints는 숫자여야 합니다");
            if (type.maxHitPoints!=0) throw std::logic_error("maxHitPoints == 0");
            type.flags1 |= TypeFlag1::kHasHitPoints;
            type.maxHitPoints=ToInt(*number);
        }
        else if (is("zorder")) {
            if (number) type.zOrder=ToInt(*number);
            else if (text) type.zOrder=ZOrder(*text);
        }
        else if (is("minUsage") && number) type.minUsage[0] = static_cast<float>(*number);
        else if ((is("maxUsage") || is("maxRainBattleUsage")) && number) type.maxUsage[0] = static_cast<float>(*number);
        else if (is("maxThunderBattleUsage") && number) type.maxUsage[1] = static_cast<float>(*number);
        else if (is("maxWindBattleUsage") && number) type.maxUsage[2] = static_cast<float>(*number);
        else if ((is("foot_x") || is("footx")) && number) type.footX = ToInt(*number);
        else if ((is("foot_y") || is("footy")) && number) type.footY = ToInt(*number);
        else if (is("level") && number) type.level = ToInt(*number) - 1;
        else if (is("description") && text) { type.description = text->substr(0, kDescriptionBytes); hasDescription = true; }
        else if (is("group") && text) {
            type.group = -1;
            // 이름 표에서 첫 일치의 위치가 그룹 번호다.
            for (std::size_t i = 0; i < kGroupNames.size(); ++i)
                if (Equal(*text, kGroupNames[i])) { type.group = static_cast<std::int32_t>(i); break; }
        }
    }
    // 원본 0049ad00/0049ad60: 사용량을 0이 아닌 값으로 정하면 그 자리에서 0x4000이 켜진다.
    for (std::size_t i = 0; i < 3; ++i)
        if (type.minUsage[i] != 0.0f || type.maxUsage[i] != 0.0f) type.flags1 |= TypeFlag1::kUsesPower;
    // 이름은 길이 제한 없이 복사되어 20글자 이상이면 설명 필드로 넘친다. 그 뒤에 `description`이 설명 필드를
    // 덮어쓰면, C 문자열로 읽은 이름은 "이름 앞 20글자 + 설명"이 된다. 실제 파일의 `TypeNames` 해시로 확인했다.
    if (type.name.size() >= kTypeNameBytes && hasDescription) type.name = type.name.substr(0, kTypeNameBytes) + type.description;
    // 종류 필드는 12바이트이고 0으로 초기화되어 있어, 8글자를 다 채워도 그 뒤에서 끝난다.
    type.kind.assign(kind.data(), kind.size());
    type.kind.resize(type.kind.find('\0') == std::string::npos ? kind.size() : type.kind.find('\0'));
}

// 0049b0d0의 플래그 관련 후처리. 모든 타입(내장·빈 타입 포함)에 적용한다.
void PostProcess(RiftTypeRecord& type) {
    if (type.group == 5) type.flags2 |= TypeFlag2::kGroupBattery;
    if (type.group == 1 || type.group == 0) type.flags2 |= TypeFlag2::kGroupArcherOrCannon;
    if (type.group == 2) type.flags2 |= TypeFlag2::kGroupBlocker;
    const auto flags2 = type.flags2;
    if ((flags2 & 0x34200) != 0) { // vortex·factory·walker·balloon은 내용물을 가진다.
        type.flags1 |= TypeFlag1::kContainer;
        type.containerListFlags |= (flags2 & 0x30000) == 0 ? 0x10u : 0x20u;
    }
    if ((flags2 & TypeFlag2::kBomb) != 0) type.contentListFlags |= 1;
    if ((flags2 & TypeFlag2::kBuried) != 0) type.flags1 |= TypeFlag1::kSaveQA;
    if ((flags2 & 0xa00) != 0) type.flags1 |= 0x10000000;
    if ((flags2 & TypeFlag2::kFactory) != 0) type.flags1 |= 0x4000000;
    if ((flags2 & TypeFlag2::kVortex) != 0) type.flags1 |= 0x84000;
    // 원본은 여기서 minUsage·maxUsage가 0이 아니면 0x4000을 다시 켠다(ApplyDefinition에서 이미 켰다).
    if ((type.flags1 & TypeFlag1::kSaveQA) != 0 && (type.flags1 & TypeFlag1::kSaveQB) != 0)
        throw std::runtime_error("(typeFlags & TYPE_FLAG_SAVE_Q16) == 0"); // 원본 assert.
    if ((type.flags1 & TypeFlag1::kMayDropOnRim) == 0) type.flags1 |= TypeFlag1::kMayDropOnIsle;
    if ((type.flags2 & TypeFlag2::kEmplacement) != 0) type.flags2 |= TypeFlag2::kDropBlocking;
    const auto final2 = type.flags2;
    if ((final2 & TypeFlag2::kEmplacement) != 0 && (final2 & 0x4200) == 0 && type.footX == 3 && type.footY == 3)
        type.flags1 |= 6;
    if ((final2 & 0x408100) != 0) type.flags1 |= TypeFlag1::kMayDropOnIsle;
    if ((final2 & 0x70000) != 0 && (final2 & TypeFlag2::kFactory) == 0) type.flags1 |= 0x8000;
    if (type.footY == 6) type.footY = 8; // 원본: foot_y 6은 8로 바꾼다.
}
}

// 판본의 타입 수만큼 0으로 초기화한 뒤 이름·정의·후처리를 적용한다.
RiftTypeTable::RiftTypeTable(OriginalEdition edition, std::span<const RiftTypeSource> sources) {
    const bool cd = edition == OriginalEdition::Cd1072;
    types_.resize(cd ? kCdTypeCount : kPatchTypeCount);
    const auto order = TypeLoadOrder(edition);
    if (sources.size() != order.size()) throw std::invalid_argument("Type source count does not match the edition load order");
    // .type 타입: 번호 = 70 + 로딩 순서. 이름은 정의의 typename이 아니라 파일 이름이다(원본 0049c3b0).
    for (std::size_t i = 0; i < sources.size(); ++i) {
        if (!sources[i].definition || !Equal(sources[i].assetName, order[i]))
            throw std::invalid_argument("Type sources must follow the edition load order");
        auto& type = types_.at(static_cast<std::size_t>(kFirstAssetTypeNumber) + i);
        type.name = std::string(order[i]); // 원본은 실행 파일의 이름 목록에서 복사한다.
        type.assetName = type.name;
        type.fromAsset = true;
        ApplyDefinition(type, *sources[i].definition);
    }
    // 내장 프로세스 타입 이름: 20바이트 필드에 strncpy 하므로 긴 이름은 잘린다.
    const auto processNames = ProcessTypeNames(edition);
    // 번호 10부터 차례로 이름을 넣는다.
    for (std::size_t i = 0; i < processNames.size(); ++i)
        types_.at(kFirstProcessTypeNumber + i).name = std::string(processNames[i].substr(0, kTypeNameBytes));
    // 후처리가 이름을 채우는 내장 타입.
    for (const auto& builtin : kBuiltinNames) types_.at(builtin.number).name = std::string(builtin.name);
    // 후처리는 번호순으로 모든 타입에 적용된다.
    for (auto& type : types_) PostProcess(type);
    // 원본은 로딩/후처리 뒤 생성자 표를 대입한다. 주소는 미복원 파생 dispatch를 감지하는 메타데이터다.
    for (std::size_t i=0;i<types_.size();++i) types_[i].constructorAddress=TypeConstructorAddress(edition,i);
    // 원본은 타입 번호를 전역 변수로 갖는다(패치 DAT_005411a0 = 82, DAT_005411d0 = 94). 로딩 순서에서 찾는다.
    for (std::size_t i = 0; i < order.size(); ++i) {
        if (order[i] == "bridge") bridge_ = kFirstAssetTypeNumber + static_cast<int>(i);
        if (order[i] == "island") island_ = kFirstAssetTypeNumber + static_cast<int>(i);
    }
}
// 번호순 목록을 돌려준다.
std::span<const RiftTypeRecord> RiftTypeTable::Types() const { return types_; }
// 원본은 0번부터 순서대로 비교해 첫 일치를 돌려준다.
int RiftTypeTable::Find(std::string_view name) const {
    // 이름 없는 타입은 빈 이름을 찾을 때만 맞는다.
    for (std::size_t i = 0; i < types_.size(); ++i) if (Equal(types_[i].name, name)) return static_cast<int>(i);
    return -1;
}
// 부호 있는 글자를 32비트로 넓혀 밀고, 덧셈은 32비트로 감긴다.
std::uint32_t RiftTypeTable::NameHash(std::string_view name) {
    std::uint32_t hash = 0;
    // 원본은 이름 필드를 NUL까지 읽는다. 20글자에서 자르지 않는다.
    for (std::size_t i = 0; i < name.size(); ++i) {
        const auto extended = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<signed char>(name[i])));
        hash += extended << ((i & 3) * 8);
    }
    return hash;
}
// CD판 목록은 패치판 목록의 앞부분이다.
std::span<const std::string_view> RiftTypeTable::ProcessTypeNames(OriginalEdition edition) {
    const std::span<const std::string_view> names(kProcessTypeNames);
    return edition == OriginalEdition::Cd1072 ? names.first(kCdProcessTypeCount) : names;
}
// 다리 타입 번호(패치 82).
int RiftTypeTable::BridgeType() const { return bridge_; }
// 섬 타입 번호(패치 94).
int RiftTypeTable::IslandType() const { return island_; }
}
