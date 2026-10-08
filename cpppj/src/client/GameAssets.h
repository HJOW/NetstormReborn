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
struct TypeAsset {
    std::string assetName; // .type 파일 이름. typename과 다를 수 있다(dude → Man).
    o::RiftTypeDefinition definition;
    std::size_t block{};
};
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
    o::OriginalEdition edition_;
    ShapeDatabase shapes_;
    GamePalette palette_;
    std::vector<TypeAsset> types_;
    std::optional<o::RiftTypeTable> typeTable_; // 타입을 모두 읽은 뒤에 만든다.
};
}
