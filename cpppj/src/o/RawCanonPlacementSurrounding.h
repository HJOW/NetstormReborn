// 배치 decoder의 현재 모양에서 flag 8 표면 지도를 동적으로 순회하고 주변 권한을 누적한다.
#pragma once
#include "o/RawCanonPlacementPermission.h"
#include "o/RawSquidNeighbors.h"

namespace netstorm::o {
// 일반 타입·패턴 표면 순회 중 현재 위치와 대상이다.
struct CanonSurfaceCursor {
    int x{},y{},left{},top{},right{},bottom{}; // 원본 +68..+7c의 정수 커서와 확장 모서리다.
    // current: 현재 처리 위치 또는 현재 선택 상태.
    Sid current{};
    std::vector<std::uint16_t> returned; // 원본 signed WORD 중복 캐시의 반환 순서다.
};
// 일반 타입·패턴 표면 순회 동작을 실행하는 처리기이다.
class RawCanonSurfaceWalk {
public:
    // 현재 모양의 float 발자국/중심/프레임 번호는 생성 때 고정하고 지도/raw/코드 표는 매 Next에서 읽는다.
    RawCanonSurfaceWalk(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,
        std::span<const std::uint8_t> spots,const CanonPlacementQuery& query,const CanonPlacementCell& cell);
    RawCanonSurfaceWalk(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,std::span<const std::uint16_t> islands,
        std::span<const std::uint8_t> spots,Sid source); //실제 Squid 원천도 같은 flag8 커서/필터를 사용하며 float 좌표를 정수화하지 않는다.
    // 생성 직후 첫 결과와 이후 결과를 원본처럼 노출한다. 0 반환 뒤에는 종료 상태를 유지한다.
    Sid Current() const;
    Sid Next();
    // 기계어 관찰과 지도 변경 검증에 사용할 실제 커서다.
    const CanonSurfaceCursor& State() const;
private:
    void Initialize(const CanonPlacementCell& cell); //두 원천의 같은 발자국/중심/최초 방문을 구성한다.
    std::span<const FrameCode> Codes(std::uint32_t type) const; //배치 표 또는 실제 TYPE의 현재 코드 span을 읽는다.
    // 표면 타입→접합→죽음/기준점 spot 순서로 flag 8 가상 필터를 적용한다.
    bool Accept(Sid candidate) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::span<const PriestPlainCanonType> frames_;
    std::span<const RiftTypeFrames> rawFrames_; //실제 Squid 원천만 사용하는 코드 표이며 배치/실제 표 중 하나만 빌린다.
    // islands_: 현재 섬의 배치·소유권 자료.
    std::span<const std::uint16_t> islands_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // sourceType_: 원천 타입.
    std::uint32_t sourceType_{};
    int sourceFrame_{}; // 원본 +44에 저장한 프레임 번호이며 코드 자체는 필터 때 읽는다.
    // left_: 사각형의 왼쪽 경계.
    // top_: 사각형의 위쪽 경계.
    // right_: 사각형의 오른쪽 경계.
    // bottom_: 사각형의 아래쪽 경계.
    // centerX_: 중심 X.
    // centerY_: 중심 Y.
    float left_{},top_{},right_{},bottom_{},centerX_{},centerY_{};
    // state_: 처리 중 조회·갱신하는 상태 (CanonSurfaceCursor).
    CanonSurfaceCursor state_;
    // ended_: 종료된 여부.
    bool ended_{};
};
// 배치 decoder의 현재 모양에서 flag 8 표면 지도를 동적으로 순회하고 주변 권한을 누적한다.
class RawCanonPlacementSurrounding {
public:
    // 실제 후보 권한 helper와 같은 풀을 요구하며 지도/타입/프레임 자료는 호출자 소유다.
    RawCanonPlacementSurrounding(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,
        std::span<const std::uint8_t> spots,const RawCanonPlacementPermission& permission);
    // 권한을 얻어도 남은 후보를 계속 읽으며 요청 owner의 하위 BYTE만 부호 확장한다.
    bool Inspect(const CanonPlacementQuery& query,const CanonPlacementCell& cell) const;
    // 같은 raw 풀을 사용하는 지형 어댑터만 허용한다.
    const SidPool& Pool() const;
private:
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::span<const PriestPlainCanonType> frames_;
    // islands_: 현재 섬의 배치·소유권 자료.
    std::span<const std::uint16_t> islands_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // permission_: 소유권·지역 관계에 따른 배치 허용 판정.
    const RawCanonPlacementPermission& permission_;
};
// 기존 후보/최종 정책을 유지하고 모양 종료의 주변 권한 경계만 실제 flag 8 순회로 바꾼다.
CanonPlacementTerrainHooks MakeCanonSurroundingTerrainHooks(const SidPool& pool,
    const RawCanonPlacementSurrounding& surrounding,CanonPlacementTerrainHooks hooks);
}
