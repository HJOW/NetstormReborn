// 복원된 원본 모듈을 연결하는 새 자산 계층. 원본 클래스 이름으로 주장하지 않는다.
#pragma once
#include "client/Screen.h"
#include "client/VFXDraw.h"
#include "o/BaseFile.h"
#include "o/RiftType.h"
#include <optional>
#include <string>
#include <vector>

namespace netstorm::client {
// 타입 정의와 그 타입이 사용하는 셰이프 블록이다.
struct TypeAsset {
    std::string assetName; // .type 파일 이름. typename과 다를 수 있다(dude → Man).
    // definition: RiftType의 자산 정의 부분. 모든 게임 플래그·생성자 함수 포인터를 복원한 구조체는 아니다 (o::RiftTypeDefinition).
    o::RiftTypeDefinition definition;
    // block: 셰이프 DB에서 그림을 찾는 블록 번호.
    std::size_t block{};
};
// 게임 실행에 필요한 타입·셰이프·팔레트 자산 저장소이다.
class GameAssets {
public:
    // setup.cfg의 `battlePal = "gifcloud"`와 `GamePalSpec`이 가리키는 기본 팔레트 경로.
    static constexpr std::string_view kDefaultPalettePath = "d/gifcloud.col";
    // 선택한 판본의 타입·SHP·팔레트를 동일한 VFS에서 읽어 연결한다.
    // 팔레트 경로는 설정(`GamePalSpec`에 `battlePal` 등을 넣은 값)으로 계산해 넘길 수 있다.
    GameAssets(const o::BaseFileSystem& files, o::OriginalEdition edition,
        std::string_view palettePath = kDefaultPalettePath);
    // 원본 로딩 순서를 보존한 자산 목록.
    std::span<const TypeAsset> Types() const;
    // 파일 이름 또는 typename으로 타입을 찾는다. ASCII 대소문자는 구분하지 않는다.
    const TypeAsset& Find(std::string_view name) const;
    // 클러스터·레이어를 SHP의 원래 프레임 인덱스로 바꾼다.
    std::size_t FrameIndex(const TypeAsset& type, std::string_view cluster, std::size_t layer) const;
    // 그래픽과 팔레트를 후속 화면 복원·검사 도구에 제공한다.
    const ShapeDatabase& Shapes() const;
    // 원본 팔레트 번호에 대응하는 색 목록을 제공한다.
    const GamePalette& Palette() const;
    // 전체 타입 번호 체계(내장 타입 포함)와 플래그. `.fort`의 타입 번호를 풀 때 쓴다.
    const o::RiftTypeTable& TypeTable() const;
    // raw 풀/프레임 필드가 자산을 읽은 판본과 같은 배치를 사용하도록 제공한다.
    o::OriginalEdition Edition() const;
private:
    // edition_: 판본별 필드 배치와 동작을 선택하는 원본 버전.
    o::OriginalEdition edition_;
    // shapes_: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    ShapeDatabase shapes_;
    // palette_: 그림의 색 인덱스를 실제 출력 색으로 바꿀 팔레트.
    GamePalette palette_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<TypeAsset> types_;
    std::optional<o::RiftTypeTable> typeTable_; // 타입을 모두 읽은 뒤에 만든다.
};
}
