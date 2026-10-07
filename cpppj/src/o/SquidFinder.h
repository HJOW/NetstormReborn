// SquidFinder.cpp의 flag 8 표면 이웃 탐색 경로. 일반 공간 해시/이동 중 소수 좌표는 후속이다.
#pragma once
#include "o/RiftType.h"
#include "o/TerrainBuilder.h"
#include <unordered_map>

namespace netstorm::o {
// 원본 Squid/Type에서 읽는 표면 필드만 이름 있는 정수 발자국 스냅샷으로 받는다.
struct SurfaceObject {
    std::uint16_t id{};
    int x{},y{},width{1},height{1}; // 기준점은 발자국의 오른쪽 아래 칸이다.
    std::uint32_t flags1{},flags2{};
    FrameCode frame{};
    bool dead{},buried{};
};
class SurfaceFinder {
public:
    // 원본 표면 오브젝트 번호 지도와 spot 바이트를 복사한다. 모두 256×256이며 원본 SID 형식은 별도다.
    SurfaceFinder(std::span<const SurfaceObject> objects,std::span<const std::uint16_t> map,
        std::span<const std::uint8_t> spots);
    // 원본 004b23e0/004b1d70/004b1c20/004b1e80 ↔ CD 004ebad0/004ebf30/004ebdd0/004ec040.
    // flag 8: 한 칸 확장·네 모서리 제외·y/x 순서·자기 사각형 제외·연결/죽음/내부 필터·중복 제거.
    std::vector<std::uint16_t> Neighbors(std::uint16_t id) const;
    // 원본 배열 범위 확인을 이름 있는 조회로 옮긴다. 없는 번호는 예외다.
    const SurfaceObject& Object(std::uint16_t id) const;
    // 전역 Graph 재구성은 발자국 AND와 달리 기준점 spot의 내부 비트만 읽는다.
    bool OriginInterior(std::uint16_t id) const;
    // 일반 삭제 탐색과 flag 8 이웃 탐색이 공유하는 프레임/방향 접합 판정이다.
    static bool Connects(const SurfaceObject& from,const SurfaceObject& to);
private:
    // 원본 004ab880 ↔ CD 00487810: 각 좌표를 1..255로 자르고 사각형의 모든 spot 비트를 AND한다.
    bool Interior(const SurfaceObject& object) const;
    std::unordered_map<std::uint16_t,SurfaceObject> objects_;
    std::vector<std::uint16_t> map_;
    std::vector<std::uint8_t> spots_;
};
}
