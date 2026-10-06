// 원본 RiftType.cpp의 4바이트 프레임 코드와 검색 함수 복원.
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace netstorm::o {
struct FrameCode {
    std::uint8_t side{}; // 원본 코드 +0, 방향 문자.
    std::uint8_t variant{}; // 원본 코드 +1, 변형 문자.
    std::uint8_t number{}; // 원본 코드 +2, 8비트 프레임 번호.
    std::uint8_t flags{}; // 원본 코드 +3. 검색에서 MSVC signed char로 부호 확장한다.
};
// 원본 배열 원소는 정확히 4바이트다. 게임 객체 자체의 32비트 포인터 배치는 복제하지 않는다.
static_assert(sizeof(FrameCode) == 4);

class RiftTypeFrames {
public:
    // 원본 type +0x114(개수), +0x124(코드 포인터)를 소유권 있는 배열로 바꾼다.
    explicit RiftTypeFrames(std::vector<FrameCode> frames);
    // 번호까지 같은 첫 프레임. 실패는 -1. 패치 0049a9a0 ↔ CD 00444350.
    int FindNumber(std::uint8_t side, std::uint8_t variant, std::uint8_t number) const;
    // 번호와 (부호 확장 플래그 & 마스크)가 맞는 첫 프레임. 패치 0049a9e0 ↔ CD 00444410.
    int FindMasked(std::uint8_t side, std::uint8_t variant, std::uint8_t number, std::uint32_t mask) const;
    // 방향·변형·플래그가 정확히 같은 첫 프레임. 패치 0049aa30 ↔ CD 00444460.
    int FindFlags(std::uint8_t side, std::uint8_t variant, std::int32_t flags) const;
    // 코드 배열을 검증·직렬화 도구에 공개한다.
    std::span<const FrameCode> Codes() const;
private:
    std::vector<FrameCode> frames_;
};

// 실제 자산을 연결할 때 CD판과 패치판의 로딩 목록을 구분한다.
enum class OriginalEdition { Patch1078, Cd1072 };
// 패치판 00540dd0의 116개, CD판 0051c6f8의 101개 자산 이름을 반환한다.
std::span<const std::string_view> TypeLoadOrder(OriginalEdition edition);
// 타입 초기화의 실제 대입 구간에서 복원한 생성자 주소다. 호스트 함수 포인터로 사용하지 않는다.
std::uint32_t TypeConstructorAddress(OriginalEdition edition,std::size_t number);

struct TypeProperty {
    std::string name;
    std::variant<double, std::string> value; // 숫자와 따옴표 문자열을 구분하여 보존한다.
};
struct TypeImageReference {
    std::string file;
    double number{}; // GIF 번호는 빌드용 참조이며 런타임 SHP 선택에는 쓰지 않는다.
};
struct TypeCluster {
    std::string name;
    FrameCode code{};
    std::vector<TypeImageReference> images;
};
struct TypeSpecialFrames {
    int defaultFrame{}, gumpFrame{}; // 원본의 0 초기값.
    int helpFrame{-1}, baseFrame{-1}; // 원본의 -1 초기값.
};

// RiftType의 자산 정의 부분. 모든 게임 플래그·생성자 함수 포인터를 복원한 구조체는 아니다.
class RiftTypeDefinition {
public:
    // .type 문법을 읽고 0049c3b0 ↔ CD 004460f0의 프레임 코드·특수 인덱스를 만든다.
    static RiftTypeDefinition Parse(std::string_view text);
    // 원본 속성 이름은 ASCII 대소문자를 구분하지 않는다. 마지막 지정값이 우선한다.
    const TypeProperty* Property(std::string_view name) const;
    // 숫자형 속성을 조회한다. 누락 또는 문자열형이면 빈 값을 반환한다.
    std::optional<double> Number(std::string_view name) const;
    // 문자열형 속성을 조회한다. 누락 또는 숫자형이면 빈 값을 반환한다.
    std::optional<std::string_view> String(std::string_view name) const;
    // 클러스터 순서를 이전 단계에서 복원한 프레임 검색 API에 연결한다.
    RiftTypeFrames FrameTable() const;
    // 원본 후처리 0049b0d0의 foot_y=6 → 8을 반영한 배치 폭·높이.
    std::array<int, 2> Footprint() const;

    std::string name, constructor;
    std::vector<std::string> flags;
    std::vector<TypeProperty> properties;
    std::vector<TypeCluster> clusters;
    TypeSpecialFrames specialFrames;
};

// 타입 플래그 1(원본 타입 구조체 +0xe8, "typeFlags")의 비트 가운데 저장 형식과 분류에 쓰이는 것.
// 이름은 .type의 플래그 단어와 원본 assert 문자열(TYPE_FLAG_HAS_HIT_POINTS, TYPE_FLAG_SAVE_Q16 등)을 따른다.
namespace TypeFlag1 {
inline constexpr std::uint32_t kDefaultHotspot = 0x1;   // default_hotspot
inline constexpr std::uint32_t kMayDropOnIsle = 0x2;    // mayDropOnIsle. 후처리가 rim이 아니면 켠다.
inline constexpr std::uint32_t kMayDropOnRim = 0x4;     // mayDropOnRim
inline constexpr std::uint32_t kHasHitPoints = 0x10;    // maxHitPoints 속성이 있으면 켜진다.
inline constexpr std::uint32_t kSaveFrame = 0x20;       // saveFrame: .fort에 프레임 1바이트.
inline constexpr std::uint32_t kSurface = 0x800;        // surface: 표면 오브젝트 지도/이웃 탐색 대상.
inline constexpr std::uint32_t kSaveQA = 0x1000;        // saveQA: .fort에 수량 1바이트.
inline constexpr std::uint32_t kSaveQB = 0x2000;        // saveQB: .fort에 수량 2바이트(TYPE_FLAG_SAVE_Q16).
inline constexpr std::uint32_t kUsesPower = 0x4000;     // minUsage·maxUsage가 0이 아니거나 vortex.
inline constexpr std::uint32_t kContainer = 0x20000;    // container: .fort에 내용물 목록.
}
// 타입 플래그 2(원본 타입 구조체 +0xec)의 비트 가운데 저장 형식과 분류에 쓰이는 것.
namespace TypeFlag2 {
inline constexpr std::uint32_t kIsland = 0x2;
inline constexpr std::uint32_t kBridge = 0x4;           // .fort에 다리 클러스터 번호 1바이트.
inline constexpr std::uint32_t kDropBlocking = 0x10;
inline constexpr std::uint32_t kBomb = 0x100;
inline constexpr std::uint32_t kVortex = 0x200;
inline constexpr std::uint32_t kGroupBattery = 0x800;   // group = battery(5).
inline constexpr std::uint32_t kBuried = 0x2000;
inline constexpr std::uint32_t kFactory = 0x4000;       // .fort 버전 2부터 작업장 상태 1바이트.
inline constexpr std::uint32_t kWalker = 0x10000;
inline constexpr std::uint32_t kBalloon = 0x20000;
inline constexpr std::uint32_t kEmplacement = 0x40000;
inline constexpr std::uint32_t kGroupBlocker = 0x4000000;      // group = blocker(2).
inline constexpr std::uint32_t kGroupArcherOrCannon = 0x8000000; // group = archer(0)·cannon(1).
// 소유 플레이어 1바이트를 저장하는 타입(패치 DAT_00542644).
inline constexpr std::uint32_t kOwnerSavedMask = 0x5d77cf00;
// 읽은 뒤 소유자를 0으로 만드는 타입: geyser·buried(패치 DAT_00542648).
inline constexpr std::uint32_t kOwnerClearedMask = 0x10002000;
}
// `group` 속성의 기본값(원본 NO_GROUP). 이름 표(패치 00542468): archer cannon blocker aviary flyer battery fence walker balloon misc.
inline constexpr int kTypeNoGroup = 10;
// .type 타입의 번호 = 이 값 + 로딩 순서(원본 0049c3b0의 `(읽은 파일 수 + 0x45) * 500`). 두 판본이 같다.
inline constexpr int kFirstAssetTypeNumber = 70;

// 원본 타입 구조체(패치 500바이트, CD 468바이트) 가운데 복원한 필드.
struct RiftTypeRecord {
    float cost{};                      // +0xc4: cost. postPop 비용 집계/후속 건설 비용 입력.
    std::int32_t maxHitPoints{};         // +0: maxHitPoints. 서버 base postCreate의 초기 HP 입력.
    std::int32_t zOrder{};               // +0x104: base Squid에는 하위 8비트만 복사한다.
    std::uint32_t constructorAddress{};  // 패치 +0x1d0 / CD +0x1b0. 0이면 base, 지원 주소는 자산 파생 생성자.
    // +4(20바이트)를 C 문자열로 읽은 값. 내장 프로세스 타입은 20글자로 잘린다. .type 타입은 파일 이름이지만,
    // 20글자 이상이면 종료 문자가 없어 바로 뒤의 설명 필드까지 이어 읽힌다(fakeThreeByThreeSurface).
    std::string name;
    std::string description;            // +0x18(40바이트): `description` 속성.
    std::string assetName;              // 새 필드: .type 파일 이름(로딩 순서의 이름). 표시·도구용이다.
    std::array<float, 3> minUsage{};    // +0x54: minUsage
    std::array<float, 3> maxUsage{};    // +0x60: maxUsage·maxRainBattleUsage, maxThunderBattleUsage, maxWindBattleUsage
    std::int32_t level{};               // +0x94: `level` 속성 - 1
    std::int32_t group{kTypeNoGroup};   // +0x9c: `group` 속성의 이름 표 번호
    std::uint32_t flags1{};             // +0xe8
    std::uint32_t flags2{};             // +0xec
    std::string kind;                   // +0xf0: 마지막으로 나온 종류 단어(최대 8글자, 원본 strncpy 그대로)
    std::uint32_t containerListFlags{}; // +0xfc: 이 타입이 그릇일 때 내용물의 목록 플래그(0이면 파일에서 읽는다)
    std::uint32_t contentListFlags{};   // +0x100: 이 타입이 내용물일 때의 목록 플래그(0이면 그릇 것을 쓴다)
    std::int32_t footX{}, footY{};      // 패치 +0x1d4/+0x1d8, CD +0x1b4/+0x1b8. .type 타입의 기본값은 1.
    bool fromAsset{};                   // .type 파일에서 읽은 타입인가.
};

// .type 한 개를 타입 표에 넣을 때 쓰는 입력.
struct RiftTypeSource {
    std::string_view assetName;           // 로딩 순서의 파일 이름. 타입 구조체의 이름이 된다.
    const RiftTypeDefinition* definition{}; // 읽은 정의.
};

// 원본 전역 타입 배열(패치 DAT_0059ab20, 개수 DAT_00541348 = 188 / CD 171)을 객체로 둔다.
// 번호 체계: 0 없음, 1~9 내장, 10~ 내장 프로세스 타입, 70~ .type 타입, 패치판 186·187은 이름 없는 빈 타입.
class RiftTypeTable {
public:
    // 0049ebb0 ↔ CD 004434d0의 초기화, 0049c3b0 ↔ CD 004460f0의 플래그·속성 읽기,
    // 0049b0d0 ↔ CD 00444e10의 후처리를 차례로 적용한다. sources는 로딩 순서여야 한다.
    RiftTypeTable(OriginalEdition edition, std::span<const RiftTypeSource> sources);
    // 번호순 타입 목록.
    std::span<const RiftTypeRecord> Types() const;
    // 원본 FUN_0049a860 ↔ CD 00443410: 대소문자 무시 첫 일치의 번호. 없으면 -1.
    int Find(std::string_view name) const;
    // .fort `TypeNames` 섹션의 이름 해시(원본 004bd6f0·004bd800): 글자를 부호 있는 8비트로 읽어
    // (위치 % 4) × 8비트 왼쪽으로 밀어 더한다. 타입 표의 name(C 문자열로 읽은 값)을 넣는다.
    static std::uint32_t NameHash(std::string_view name);
    // 판본의 내장 프로세스 타입 이름(번호 10부터). 패치 53개(00540fa0), CD 52개(0051c890).
    static std::span<const std::string_view> ProcessTypeNames(OriginalEdition edition);
    // 다리 약식 표기와 섬 건너뛰기에 쓰는 타입 번호(패치 DAT_005411a0, DAT_005411d0).
    int BridgeType() const;
    int IslandType() const;
private:
    std::vector<RiftTypeRecord> types_;
    int bridge_{-1}, island_{-1};
};
}
