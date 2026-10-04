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
}
