// 원본 Template.cpp(요새 = "Template")·Datamanager.cpp의 `.fort` 읽기. 파일 내용을 구조화하는 데까지다.
// 월드 배치는 후속 GameWorld/Squid/IslandBuilder의 부분 구현에 연결하며 이 모듈은 파일 해석만 담당한다.
// 근거·검증 범위: docs/exe/cpp-fort-reconstruction.md
#pragma once
#include "o/RiftType.h"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::o {
// 내용물 목록의 한 항목(원본 FUN_004bd130 ↔ CD 00426dd0).
struct FortContent {
    std::uint8_t storedType{};   // 파일에 적힌 타입 번호.
    int type{};                  // `TypeNames`로 바꾼 현재 타입 번호.
    std::int32_t quantity{};     // saveQA면 1바이트(부호 없음), saveQB면 부호 있는 16비트. 둘 다 아니면 0.
    std::uint32_t listFlags{};   // 타입 +0x100 → 그릇 타입 +0xfc → 파일의 1바이트 순서로 정한 목록 플래그.
    bool listFlagsStored{};      // 목록 플래그를 파일에서 읽었는가.
    std::vector<FortContent> contents; // container 타입이면 중첩 내용물.
    bool contentsComplete{true}; // 중첩 목록이 타입 0에서 끊기지 않았는가.
};
// 내용물 목록 전체. complete가 거짓이면 변환한 타입이 0인 항목에서 읽기가 끊긴 것이다(원본 반환값 0).
struct FortContentList {
    std::vector<FortContent> items;
    bool complete{true};
};
// 청크 안의 오브젝트 레코드(원본 FUN_004bdc60 ↔ CD 00428320).
struct FortObject {
    std::uint8_t position{};     // 청크 안 위치: 상위 4비트 x, 하위 4비트 y.
    std::uint8_t storedType{};   // 파일에 적힌 타입 번호. 0xf6 이상이면 다리 약식 표기다.
    int type{};                  // 현재 타입 번호. 0이면 빈 레코드다.
    std::optional<int> shorthandOwner;        // 다리 약식 표기의 소유자(0xff - 타입 바이트).
    std::optional<std::uint8_t> frame;        // saveFrame: 프레임.
    std::optional<std::uint8_t> quantityByte; // saveQA: 수량의 낮은 1바이트.
    std::optional<std::int16_t> quantityWord; // saveQB: 수량 16비트.
    std::optional<std::uint8_t> bridgeFrame;  // bridge: 다리 클러스터 번호(프레임 자리에 쓴다).
    std::optional<std::uint8_t> legacyState;  // 버전 0에서만 있는 상태 1바이트.
    std::optional<std::uint8_t> factoryState; // 버전 2 이상, factory: 작업장 상태.
    std::optional<std::uint8_t> owner;        // 버전 1 이상, 소유자를 저장하는 타입: 파일의 값 그대로.
    std::optional<std::uint8_t> islandByte;   // island 타입 뒤의 건너뛰는 1바이트.
    std::optional<FortContentList> contents;  // container: 내용물 목록.

    // 청크 안의 칸 좌표.
    int CellX() const { return position >> 4; }
    int CellY() const { return position & 0xf; }
    // 원본이 소유자 바이트를 쓰는 값: 0이거나 8보다 크면 1.
    std::optional<int> NormalizedOwner() const;
};
// 청크 레코드: 'c' + 오브젝트 수 + 오브젝트(원본 FUN_004be160 ↔ CD 004288a0).
struct FortChunk {
    std::vector<FortObject> objects;
};
// `Chaff`·`TerrNN` 섹션: 버전 1바이트 + 청크 레코드들.
struct FortChunkSection {
    bool present{};              // 섹션에 버전 바이트 말고도 내용이 있는가.
    std::uint8_t version{};      // 원본 파일은 2(구버전 1도 있다).
    std::vector<FortChunk> chunks;
};
// `Deck` 항목(원본 FUN_004bf190 ↔ CD 00429670).
struct FortDeckEntry {
    std::uint8_t storedType{};
    int type{};                  // 0이면 원본은 덱에 넣지 않는다.
    std::uint8_t chance{};
    std::int8_t power{};
    std::uint8_t remaining{};
};
// `Subscriber` 섹션(원본 FUN_004bd450): 소유자 ID와 요새 이름.
struct FortSubscriber {
    std::uint32_t id{};
    std::string name;            // 원본 바이트 그대로(Windows-1252). 화면에 쓸 때 변환한다.
};

class FortTemplate {
public:
    // 실행 파일의 섹션 이름 목록(패치 005146b0 + Terr00~Terr19) 길이.
    static constexpr std::size_t kSectionCount = 35;
    // 영역 섹션 수.
    static constexpr std::size_t kTerritoryCount = 20;
    // 월드의 청크 수는 16 × 16이고 `Chaff`는 y 바깥·x 안쪽 순서로 모두 담는다.
    static constexpr std::size_t kWorldChunksPerSide = 16;
    // 청크 한 변의 칸 수.
    static constexpr int kChunkCells = 16;

    // 섹션 이름을 저장 순서대로 돌려준다.
    static std::span<const std::string_view> SectionNames();
    // 파일 전체를 읽는다. 타입 표는 그 파일을 읽을 판본의 것이어야 한다. 형식 오류는 예외다.
    static FortTemplate Parse(std::span<const std::uint8_t> bytes, const RiftTypeTable& types);
    // 이름으로 원시 섹션을 찾는다. 파일에 그 섹션이 없으면 빈 범위다.
    std::span<const std::uint8_t> Section(std::string_view name) const;
    // `TypeNames`로 파일의 타입 번호를 현재 번호로 바꾼다. 섹션이 비어 있으면 그대로다.
    int ConvertType(std::uint8_t storedType) const;

    std::uint8_t flag{};                          // 파일 둘째 바이트(0 또는 0xfe).
    std::vector<std::vector<std::uint8_t>> sections; // 읽은 원시 섹션(최대 35개, 이름 순서).
    std::size_t trailingBytes{};                  // 마지막으로 읽은 섹션 뒤에 남은 바이트 수.
    std::optional<FortSubscriber> subscriber;
    std::optional<float> money;                   // `Money`: Storm Power.
    std::vector<std::uint32_t> typeHashes;        // `TypeNames`의 해시(파일의 타입 번호 순서).
    bool hasTypeConversion{};                     // `TypeNames`가 비어 있지 않은가.
    std::array<std::uint8_t, 255> typeConversion{}; // 파일 타입 번호 → 현재 타입 번호(없으면 0).
    FortContentList technology;                   // `Technology`: 타입별 저장 상태.
    std::size_t technologyTrailingBytes{};        // 목록 뒤에 남은 바이트(원본은 읽지 않는다).
    std::vector<FortDeckEntry> deck;
    std::vector<std::uint8_t> territory;          // `Territory` 120바이트(6바이트 × 20) 원시 값.
    FortChunkSection chaff;
    std::array<FortChunkSection, kTerritoryCount> territories;
};
}
