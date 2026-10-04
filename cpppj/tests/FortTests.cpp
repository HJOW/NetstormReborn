// 타입 표(번호 체계·플래그·이름 해시)와 `.fort` 읽기의 단위 검사. 원본 파일 없이 작은 합성 입력을 쓴다.
// 실제 두 판본의 `.fort` 전체 대조는 tools/cpp_fort_smoke.py가 한다.
#include "TestSupport.h"
#include "o/RiftType.h"
#include "o/Template.h"
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace {
// 형식 오류가 예외로 보고되는지 검사한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}

// 검사에 쓰는 타입 정의. 여기 없는 타입은 플래그·속성이 없는 빈 정의다.
const std::map<std::string, std::string> kDefinitions = {
    {"bridge", "typeflags bridge;"},
    {"island", "typeflags island islandThreeByThree;"},
    {"sunCannon", "typeflags emplacement saveFrame; { group=\"cannon\"; foot_x=3; foot_y=3; maxHitPoints=40; }"},
    {"priest", "typeflags priest walker;"},
    {"bombHeal", "typeflags bomb saveQA;"},
    {"nugget", "typeflags nugget saveQB;"},
    {"sunFactory", "typeflags factory emplacement; { foot_y=6; }"},
    {"windVortex", "typeflags vortex emplacement; { minUsage=2; }"},
    {"residence", "typeflags residence guy;"},
    {"sunBlocker", "{ group=\"blocker\"; }"},
    {"windBattery", "{ group=\"battery\"; level=3; }"},
    {"fakeThreeByThreeSurface", "typeflags surface; { description=\"fake3x3Surface\"; }"},
};

// 판본의 로딩 순서대로 정의를 만들어 타입 표를 구성한다.
struct TestTypes {
    std::vector<RiftTypeDefinition> definitions;
    std::vector<RiftTypeSource> sources;
    // 정의가 옮겨지지 않도록 먼저 모두 만든 뒤 주소를 넘긴다.
    explicit TestTypes(OriginalEdition edition) {
        const auto order = TypeLoadOrder(edition);
        // 타입마다 최소 문법(typename, 본문, 클러스터 하나)의 글을 읽는다.
        for (const auto name : order) {
            const auto found = kDefinitions.find(std::string(name));
            std::string body = found == kDefinitions.end() ? "{}" : found->second;
            if (body.find('{') == std::string::npos) body += " {}";
            definitions.push_back(RiftTypeDefinition::Parse("typename " + std::string(name) + " " + body + " A00:default:\"a.gif\"#0;"));
        }
        // 로딩 순서의 이름과 정의를 짝짓는다.
        for (std::size_t i = 0; i < order.size(); ++i) sources.push_back({order[i], &definitions[i]});
    }
    // 타입 표를 만든다.
    RiftTypeTable Table(OriginalEdition edition) const { return RiftTypeTable(edition, sources); }
};

// 리틀 엔디언 정수를 바이트 배열 끝에 붙인다.
void Put(std::vector<std::uint8_t>& bytes, std::uint32_t value, std::size_t size) {
    // 낮은 바이트부터 쓴다.
    for (std::size_t i = 0; i < size; ++i) bytes.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
}

// 섹션 35개(비어 있는 것은 길이 2)로 `.fort` 바이트를 만든다. 번호는 저장 순서다.
std::vector<std::uint8_t> MakeFort(const std::map<std::size_t, std::vector<std::uint8_t>>& bodies, std::size_t sectionCount = FortTemplate::kSectionCount) {
    std::vector<std::uint8_t> file{0x46, 0x00};
    // 길이 필드에는 자기 2바이트가 들어 있다.
    for (std::size_t i = 0; i < sectionCount; ++i) {
        const auto found = bodies.find(i);
        const auto size = found == bodies.end() ? 0 : found->second.size();
        Put(file, static_cast<std::uint32_t>(size + 2), 2);
        if (found != bodies.end()) file.insert(file.end(), found->second.begin(), found->second.end());
    }
    return file;
}

// 섹션 번호(실행 파일의 이름 목록 순서).
constexpr std::size_t kSubscriber = 0, kChaff = 4, kTerritory = 6, kTypeNames = 7, kTechnology = 9, kMoney = 10, kDeck = 11, kTerr00 = 15, kTerr01 = 16, kTerr02 = 17;
// 검사용 파일의 타입 번호. 실제 파일처럼 0번은 이름 없는 타입이다.
enum StoredType : std::uint8_t { kNone, kCannon, kPriest, kBomb, kIslandType, kFactory, kNugget, kUnknown };
}

// 패치판 타입 표: 번호 체계, 내장 이름, 이름 해시. 해시 값은 실제 `.fort`의 `TypeNames`에서 확인한 값이다.
TEST_CASE(TypeTable_PatchNumberingAndNameHashes_MatchOriginalFiles) {
    const TestTypes source(OriginalEdition::Patch1078);
    const auto table = source.Table(OriginalEdition::Patch1078);
    const auto types = table.Types();
    CHECK(types.size() == 188);
    CHECK(types[0].name.empty() && types[5].name.empty() && types[69].name.empty());
    CHECK(types[1].name == "DependForm" && types[4].name == "PlayerSquid" && types[6].name == "ContentForm");
    CHECK(types[10].name == "evilRaySystemProcess"); // 20글자에서 잘린다.
    CHECK(types[62].name == "thunderAviaryProcess" && types[63].name.empty());
    CHECK(types[70].name == "dude" && types[185].name == "Monument");
    CHECK(types[186].name.empty() && types[187].name.empty());
    CHECK(table.BridgeType() == 82 && table.IslandType() == 94);
    CHECK(table.Find("BRIDGE") == 82 && table.Find("nothing") == -1);
    CHECK(table.Find("") == 0); // 이름 없는 타입 가운데 첫 번호.
    CHECK(RiftTypeTable::NameHash("") == 0);
    CHECK(RiftTypeTable::NameHash(types[1].name) == 0xd4b73724u);
    CHECK(RiftTypeTable::NameHash(types[10].name) == 0x083d0100u);
    CHECK(RiftTypeTable::NameHash("bridge") == 0x6469d7c9u);
    // 20글자 이상인 이름은 설명 필드와 이어져 읽힌다.
    const auto& overflowing = types[static_cast<std::size_t>(kFirstAssetTypeNumber) + 92];
    CHECK(overflowing.assetName == "fakeThreeByThreeSurface");
    CHECK(overflowing.name == "fakeThreeByThreeSurffake3x3Surface");
    CHECK(RiftTypeTable::NameHash(overflowing.name) == 0x0534a54bu);
    CHECK(table.Find("fakeThreeByThreeSurface") == -1);
    // 부호 있는 글자는 부호 확장한 뒤 민다.
    CHECK(RiftTypeTable::NameHash("\xff") == 0xffffffffu);
    CHECK(RiftTypeTable::NameHash("a\xff") == 0x61u + 0xffffff00u);
}

// CD판은 타입 171개, 프로세스 타입 52개다.
TEST_CASE(TypeTable_CdEdition_HasItsOwnCounts) {
    const TestTypes source(OriginalEdition::Cd1072);
    const auto table = source.Table(OriginalEdition::Cd1072);
    CHECK(table.Types().size() == 171);
    CHECK(table.Types()[61].name == "sharedRegularProcess" && table.Types()[62].name.empty());
    CHECK(table.Types()[169].name == "daisExtraFrames" && table.Types()[170].name == "fenceShield");
    CHECK(RiftTypeTable::ProcessTypeNames(OriginalEdition::Cd1072).size() == 52);
    CHECK(RiftTypeTable::ProcessTypeNames(OriginalEdition::Patch1078).size() == 53);
    // 다른 판본의 로딩 순서를 넣으면 거부한다.
    const TestTypes patch(OriginalEdition::Patch1078);
    CHECK(Throws([&] { patch.Table(OriginalEdition::Cd1072); }));
}

// 플래그 단어·속성·후처리(원본 0049c3b0, 0049b0d0).
TEST_CASE(TypeTable_FlagsFollowWordsPropertiesAndPostProcessing) {
    const TestTypes source(OriginalEdition::Patch1078);
    const auto table = source.Table(OriginalEdition::Patch1078);
    const auto type = [&table](std::string_view name) -> const RiftTypeRecord& { return table.Types()[static_cast<std::size_t>(table.Find(name))]; };
    // 내장·빈 타입도 후처리를 거쳐 mayDropOnIsle이 켜진다.
    CHECK(table.Types()[0].flags1 == TypeFlag1::kMayDropOnIsle && table.Types()[0].flags2 == 0);
    CHECK(table.Types()[0].group == kTypeNoGroup && table.Types()[0].footX == 0);
    // 3×3 emplacement + group cannon.
    const auto& cannon = type("sunCannon");
    CHECK(cannon.flags1 == (TypeFlag1::kSaveFrame | TypeFlag1::kHasHitPoints | 6u | 0x8000u)); // 0x8000: emplacement이고 factory가 아니다.
    CHECK(cannon.flags2 == (TypeFlag2::kEmplacement | TypeFlag2::kDropBlocking | TypeFlag2::kGroupArcherOrCannon));
    CHECK(cannon.group == 1 && cannon.footX == 3 && cannon.footY == 3);
    // walker는 내용물을 갖고 목록 플래그는 0x20이다.
    const auto& priest = type("priest");
    CHECK((priest.flags1 & TypeFlag1::kContainer) != 0 && (priest.flags1 & 0x8000) != 0);
    CHECK(priest.containerListFlags == 0x20 && priest.kind == "walker");
    // factory: 내용물 목록 플래그 0x10, foot_y 6은 8로 바뀐다.
    const auto& factory = type("sunFactory");
    CHECK((factory.flags1 & (TypeFlag1::kContainer | 0x4000000u)) == (TypeFlag1::kContainer | 0x4000000u));
    CHECK(factory.containerListFlags == 0x10 && factory.footY == 8 && factory.kind == "factory");
    // vortex: 0x84000과 0x10000000, minUsage로 전력 사용.
    const auto& vortex = type("windVortex");
    CHECK((vortex.flags1 & (0x84000u | 0x10000000u)) == (0x84000u | 0x10000000u));
    CHECK(vortex.minUsage[0] == 2.0f);
    // bomb: 내용물일 때의 목록 플래그 1.
    CHECK(type("bombHeal").contentListFlags == 1 && (type("bombHeal").flags1 & TypeFlag1::kSaveQA) != 0);
    CHECK((type("nugget").flags1 & TypeFlag1::kSaveQB) != 0);
    // 종류 단어는 8글자까지만 복사하고, 짧은 단어는 앞 글자만 덮어쓴다(원본 strncpy).
    CHECK(type("residence").kind == "guyidenc");
    CHECK(type("sunBlocker").group == 2 && (type("sunBlocker").flags2 & TypeFlag2::kGroupBlocker) != 0);
    CHECK(type("windBattery").group == 5 && (type("windBattery").flags2 & TypeFlag2::kGroupBattery) != 0);
    CHECK(type("windBattery").level == 2);
    // rim에만 놓는 타입이 아니면 isle 비트가 켜진다.
    CHECK((type("bridge").flags1 & TypeFlag1::kMayDropOnIsle) != 0 && type("bridge").flags2 == TypeFlag2::kBridge);
}

// 섹션·타입 번호 변환·오브젝트 선택 필드·내용물·덱을 한 파일로 검사한다.
TEST_CASE(FortTemplate_ReadsSectionsObjectsAndContents) {
    const TestTypes source(OriginalEdition::Patch1078);
    const auto table = source.Table(OriginalEdition::Patch1078);
    std::map<std::size_t, std::vector<std::uint8_t>> bodies;
    // Subscriber: ID + 길이 붙은 이름.
    auto& subscriber = bodies[kSubscriber];
    Put(subscriber, 0x12345678, 4); Put(subscriber, 3, 2);
    subscriber.insert(subscriber.end(), {'F', 'o', 0xe9});
    // Money: 539.0f.
    Put(bodies[kMoney], 0x4406c000, 4);
    bodies[kTerritory] = std::vector<std::uint8_t>(0x78, 7);
    // TypeNames: 파일 번호 0~7의 이름 해시. 7번은 지금 없는 타입이다.
    auto& names = bodies[kTypeNames];
    names.push_back(8);
    // 이름마다 해시 4바이트.
    for (const char* name : {"", "sunCannon", "priest", "bombHeal", "island", "sunFactory", "nugget", "goneType"})
        Put(names, RiftTypeTable::NameHash(name), 4);
    // Chaff(버전 2): 청크 둘. 첫 청크에 포대·다리 약식 표기·섬·사제(내용물)·작업장.
    bodies[kChaff] = {
        2,
        99, 5, 0,
        0x12, kCannon, 9, 3,                                  // 프레임 9, 소유자 3
        0xf0, 0xfe, 4,                                        // 다리 약식 표기: 소유자 1, 클러스터 4
        0x00, kIslandType, 0,                                 // 섬: 1바이트 건너뜀
        0x34, kPriest, 0, 2, kBomb, 5, kNugget, 0xfe, 0xff,   // 소유자 0, 내용물 2개(QA 5, QB -2)
        0x56, kFactory, 1, 9, 0,                              // 작업장 상태 1, 소유자 9, 빈 내용물
        99, 0, 0};
    // Terr00(버전 1): 작업장 상태 바이트가 없다.
    bodies[kTerr00] = {1, 99, 1, 0, 0x01, kFactory, 2, 0};
    // Terr01(버전 0): 모든 타입 뒤에 상태 1바이트가 있고 소유자는 없다.
    bodies[kTerr01] = {0, 99, 2, 0, 0x02, kCannon, 7, 0xaa, 0x03, kFactory, 0xbb, 0};
    // Terr02: 버전 바이트만 있으면 내용이 없는 섹션이다.
    bodies[kTerr02] = {2};
    // Technology: 포대(목록 플래그 4), 폭탄(QA 6, 플래그는 타입 것), 사제(플래그 4, 중첩 1개), 뒤에 0 하나.
    bodies[kTechnology] = {3, kCannon, 4, kBomb, 6, kPriest, 4, 1, kNugget, 1, 0, 0};
    // Deck: 항목 둘. 둘째는 변환할 수 없는 타입이다.
    bodies[kDeck] = {2, kCannon, 4, 1, 255, kUnknown, 3, 0xff, 2};
    auto bytes = MakeFort(bodies);
    bytes.insert(bytes.end(), {1, 2, 3}); // 35개 뒤의 남는 바이트.
    const auto fort = FortTemplate::Parse(bytes, table);

    CHECK(fort.sections.size() == 35 && fort.trailingBytes == 3 && fort.flag == 0);
    CHECK(fort.subscriber && fort.subscriber->id == 0x12345678 && fort.subscriber->name == "Fo\xe9");
    CHECK(fort.money && *fort.money == 539.0f);
    CHECK(fort.territory.size() == 0x78 && fort.territory.front() == 7);
    CHECK(fort.Section("typenames").size() == 33 && fort.Section("Reserved2").empty() && fort.Section("nope").empty());
    // 변환: 이름 없는 0번은 이름 없는 현재 타입 가운데 가장 큰 번호가 된다(원본 004bd800 그대로).
    CHECK(fort.hasTypeConversion && fort.typeHashes.size() == 8);
    CHECK(fort.ConvertType(kNone) == 187);
    CHECK(fort.ConvertType(kCannon) == table.Find("sunCannon") && fort.ConvertType(kUnknown) == 0);
    CHECK(fort.ConvertType(0xfe) == 0xfe); // 0xf6 이상은 바꾸지 않는다.

    CHECK(fort.chaff.present && fort.chaff.version == 2 && fort.chaff.chunks.size() == 2);
    const auto& objects = fort.chaff.chunks[0].objects;
    CHECK(objects.size() == 5 && fort.chaff.chunks[1].objects.empty());
    CHECK(objects[0].CellX() == 1 && objects[0].CellY() == 2 && objects[0].type == table.Find("sunCannon"));
    CHECK(objects[0].frame == 9 && objects[0].owner == 3 && !objects[0].factoryState && !objects[0].contents);
    CHECK(objects[1].type == 82 && objects[1].shorthandOwner == 1 && objects[1].bridgeFrame == 4 && !objects[1].owner);
    CHECK(objects[2].type == 94 && objects[2].islandByte == 0);
    CHECK(objects[3].owner == 0 && objects[3].NormalizedOwner() == 1);
    CHECK(objects[3].contents && objects[3].contents->complete && objects[3].contents->items.size() == 2);
    const auto& carried = objects[3].contents->items;
    CHECK(carried[0].type == table.Find("bombHeal") && carried[0].quantity == 5 && carried[0].listFlags == 1 && !carried[0].listFlagsStored);
    CHECK(carried[1].type == table.Find("nugget") && carried[1].quantity == -2 && carried[1].listFlags == 0x20 && !carried[1].listFlagsStored);
    CHECK(objects[4].factoryState == 1 && objects[4].owner == 9 && objects[4].NormalizedOwner() == 1);
    CHECK(objects[4].contents && objects[4].contents->items.empty());

    CHECK(fort.territories[0].version == 1 && fort.territories[0].chunks.size() == 1);
    CHECK(!fort.territories[0].chunks[0].objects[0].factoryState && fort.territories[0].chunks[0].objects[0].owner == 2);
    const auto& legacy = fort.territories[1].chunks[0].objects;
    CHECK(legacy[0].frame == 7 && legacy[0].legacyState == 0xaa && !legacy[0].owner);
    CHECK(legacy[1].legacyState == 0xbb && !legacy[1].factoryState && legacy[1].contents);
    CHECK(!fort.territories[2].present && fort.territories[2].chunks.empty() && !fort.territories[3].present);

    CHECK(fort.technology.complete && fort.technology.items.size() == 3 && fort.technologyTrailingBytes == 1);
    CHECK(fort.technology.items[0].listFlags == 4 && fort.technology.items[0].listFlagsStored);
    CHECK(fort.technology.items[1].quantity == 6 && fort.technology.items[1].listFlags == 1 && !fort.technology.items[1].listFlagsStored);
    CHECK(fort.technology.items[2].listFlagsStored && fort.technology.items[2].contents.size() == 1);
    CHECK(fort.technology.items[2].contents[0].quantity == 1 && fort.technology.items[2].contents[0].listFlags == 0x20);

    CHECK(fort.deck.size() == 2 && fort.deck[0].type == table.Find("sunCannon") && fort.deck[0].chance == 4);
    CHECK(fort.deck[0].power == 1 && fort.deck[0].remaining == 255);
    CHECK(fort.deck[1].type == 0 && fort.deck[1].power == -1 && fort.deck[1].remaining == 2);
}

// `TypeNames`가 비어 있으면 파일의 번호를 그대로 쓴다. 잘못된 입력은 예외다.
TEST_CASE(FortTemplate_HandlesMissingConversionAndRejectsBrokenInput) {
    const TestTypes source(OriginalEdition::Patch1078);
    const auto table = source.Table(OriginalEdition::Patch1078);
    const auto cannon = static_cast<std::uint8_t>(table.Find("sunCannon"));
    std::map<std::size_t, std::vector<std::uint8_t>> bodies;
    bodies[kChaff] = {2, 99, 2, 0, 0x11, cannon, 1, 2, 0x22, 0, 0};
    const auto fort = FortTemplate::Parse(MakeFort(bodies), table);
    CHECK(!fort.hasTypeConversion && fort.chaff.chunks[0].objects.size() == 2);
    CHECK(fort.chaff.chunks[0].objects[0].type == cannon && fort.chaff.chunks[0].objects[0].owner == 2);
    CHECK(fort.chaff.chunks[0].objects[1].type == 0); // 타입 0: 뒤의 0 한 바이트만 읽는 빈 레코드.
    CHECK(!fort.subscriber && !fort.money && fort.deck.empty() && fort.technology.items.empty());
    // 섹션이 35개보다 적은 파일은 있는 만큼만 읽는다.
    CHECK(FortTemplate::Parse(MakeFort(bodies, 5), table).sections.size() == 5);

    const std::vector<std::uint8_t> notFort{0x47, 0};
    CHECK(Throws([&] { FortTemplate::Parse(notFort, table); }));
    CHECK(Throws([&] { FortTemplate::Parse(std::vector<std::uint8_t>{0x46}, table); }));
    bodies[kChaff] = {2, 98, 0, 0}; // 청크 표지가 'c'가 아니다.
    CHECK(Throws([&] { FortTemplate::Parse(MakeFort(bodies), table); }));
    bodies[kChaff] = {2, 99, 1, 0, 0x11, cannon, 1}; // 소유자 바이트 앞에서 끝난다.
    CHECK(Throws([&] { FortTemplate::Parse(MakeFort(bodies), table); }));
    bodies[kChaff] = {2, 99, 1, 0, 0x11, 0, 5}; // 타입 0 뒤의 바이트가 0이 아니다(원본 assert "num == 0").
    CHECK(Throws([&] { FortTemplate::Parse(MakeFort(bodies), table); }));
    bodies[kChaff] = {2, 99, 1, 0, 0x11, 200, 0}; // 타입 표 밖의 번호.
    CHECK(Throws([&] { FortTemplate::Parse(MakeFort(bodies), table); }));
    // 내용물이 변환할 수 없는 타입에서 끊기면 그 뒤는 읽지 않고 표시만 남긴다.
    bodies.clear();
    bodies[kTypeNames] = {1};
    Put(bodies[kTypeNames], RiftTypeTable::NameHash("sunCannon"), 4);
    bodies[kTechnology] = {2, 0, 4, 9, 9};
    const auto partial = FortTemplate::Parse(MakeFort(bodies), table);
    CHECK(partial.technology.items.size() == 1 && !partial.technology.complete);
    CHECK(partial.technologyTrailingBytes == 1);
}
