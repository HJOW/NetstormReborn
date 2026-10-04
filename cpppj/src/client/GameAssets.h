// 복원된 원본 모듈을 연결하는 새 자산 계층. 원본 클래스 이름으로 주장하지 않는다.
#pragma once
#include "client/Screen.h"
#include "client/VFXDraw.h"
#include "o/BaseFile.h"
#include "o/RiftType.h"
#include <string>

namespace netstorm::client {
struct TypeAsset {
    std::string assetName; // .type 파일 이름. typename과 다를 수 있다(dude → Man).
    o::RiftTypeDefinition definition;
    std::size_t block{};
};
class GameAssets {
public:
    // 선택한 판본의 타입·SHP·팔레트를 동일한 VFS에서 읽어 연결한다.
    GameAssets(const o::BaseFileSystem& files, o::OriginalEdition edition);
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
private:
    ShapeDatabase shapes_;
    GamePalette palette_;
    std::vector<TypeAsset> types_;
};
}
