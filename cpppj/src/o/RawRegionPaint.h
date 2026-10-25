// 지역 투표 뒤의 실제 지면 flood와 패치판 경계 프레임 도장을 같은 SID 풀에 적용한다.
#pragma once
#include "o/RawRegionOwnership.h"
#include "o/SpStore.h"
#include "o/TerrainBuilder.h"

namespace netstorm::o {
// Region drawer의 네 float·현재 요청·두 독립 변형 캐시와 좌표 후보 목록이다.
struct RegionPaintState {
    // bounds: 생성자는 -10000을 저장하며 각 Paint 호출은 257,257,-1,-1로 다시 쓴다.
    std::array<float,4> bounds{-10000.0F,-10000.0F,-10000.0F,-10000.0F};
    // owner: 마지막 유효 지면에서 시작한 요청의 전체 DWORD 소유자 값이다.
    std::uint32_t owner{};
    // theme: 패치 요청의 원소 quartile이다. 0/-1은 기본, 1/2/그 밖은 다음 세 원소다.
    std::int32_t theme{};
    // revision: 독립 입력의 월드 다시 그리기 DWORD이며 패치0059a8b0은 flood 호출마다 한 번 증가한다.
    std::uint32_t revision{};
    // boundary: 패치의 중복 없는 8바이트 정수 좌표 후보를 삽입 순서로 보관한다.
    std::vector<CellPoint> boundary;
    // coreVariants: 본섬 내부 AA 프레임에 사용하는 독립 99개 캐시다.
    std::optional<std::array<std::int32_t,99>> coreVariants;
    // boundaryVariants: 경계 도장의 AA 프레임에 사용하는 별도 99개 캐시다.
    std::optional<std::array<std::int32_t,99>> boundaryVariants;
    std::uint32_t* sharedWorldRedraw{}; // 패치0059a8b0의 같은 현재 DWORD이며 미니맵005949d4와 합치지 않는다.
    std::uint32_t& WorldRedraw() { return sharedWorldRedraw ? *sharedWorldRedraw : revision; } // 각 Paint의 실제 증가가 동일 Client 월드 갱신에 도달한다.
    const std::uint32_t& WorldRedraw() const { return sharedWorldRedraw ? *sharedWorldRedraw : revision; } // 기존 독립 호출만 값형 revision을 사용한다.
};
// 소유자 가상 호출·빈 표면의 edgeFarm finder·현재 지역 getter를 연결한다.
struct RegionPaintHooks {
    // setOwner: 가상 +0x74다. 같은 raw 슬롯을 갱신하며 표시/장부 효과도 유지해야 한다.
    std::function<void(Sid,std::uint32_t)> setOwner;
    // boundaryAt: 표면0 또는 월드 밖 좌표에서 edgeFarm 타입을 조회하는 실제 일반 finder다.
    std::function<Sid(float,float,std::uint32_t)> boundaryAt;
    // regionAt: 패치의 영향을 받는 지역 기록에 사용할 0046f530 getter다.
    std::function<std::uint32_t(float,float)> regionAt;
};
// 00470900/0046fe40/004703b0 및 CD004bd2d0/004bd0b0의 도장 몸체다.
class RawRegionPaint {
public:
    // 섬 지도는 hash0의 실제 WORD 머리다. 프레임·난수·영향 목록은 월드와 같은 저장소를 사용한다.
    RawRegionPaint(SidPool& pool,std::span<const RiftTypeFrames> frames,
        std::span<const std::uint16_t> islands,std::uint32_t terrainType,std::uint32_t edgeFarmType,
        GameRandom& random,RegionPaintState& state,RegionOwnershipState& ownership,RegionPaintHooks hooks);
    // 좌표가 유효하지 않아도 bounds를 초기화하고 revision을 한 번 증가시킨다.
    void Paint(const RegionOwnershipPaint& request);
    // 투표와 실제 도장이 같은 슬롯을 사용하는지 검사한다.
    const SidPool& Pool() const;
private:
    // 월드 밖은 SID0이다. 별도의 snap이나 좌표 clamp를 넣지 않는다.
    Sid Surface(int x,int y) const;
    // 프레임 코드는 drawer가 캡처한 지면 타입의 코드 표에서 읽는다.
    FrameCode Code(Sid sid) const;
    // 현재 요청과 다른 소유자의 지면을 북·동·남·서 순으로 처리한다.
    void Flood(int x,int y);
    // 현재 후보 하나의 영향을 기록하고 필요한 edgeFarm/지면 프레임과 이웃 후보를 갱신한다.
    void Boundary(int x,int y);
    // 원본 좌표 목록은 같은 두 정수가 이미 있으면 추가하지 않는다.
    void AddBoundary(int x,int y);
    // 현재 코드의 네 직선 이웃을 원본 순서로 좌표 목록에 추가한다.
    void AddNeighbors(int x,int y,int connections);
    // 캐시는 각 항목마다 새 seed0의 첫 난수를 쓰며 직전 항목과 mod4가 같으면1을 더한다.
    static void Initialize(std::optional<std::array<std::int32_t,99>>& values);
    // AA는 원본 좌표/99캐시/36프레임 원소 구간을 쓰고 다른 코드는 실제 후보 검색을 사용한다.
    std::int32_t Select(Sid sid,int x,int y,bool boundary);
    // pool_: 소유자와 직접 프레임 대입의 실제 raw 슬롯 저장소다.
    SidPool& pool_;
    // frames_: 현재 타입별 코드·J 시작 번호의 조회 표다.
    std::span<const RiftTypeFrames> frames_;
    // islands_: 실제 hash0 표면 머리 WORD 지도다.
    std::span<const std::uint16_t> islands_;
    // terrainType_: flood가 처리하는 실제 isle 타입 번호다.
    std::uint32_t terrainType_;
    // edgeFarmType_: 빈 표면의 일반 finder에서 찾는 지형 경계 타입 번호다.
    std::uint32_t edgeFarmType_;
    // random_: 비AA 후보 선택이 소비하는 게임 전역 난수다.
    GameRandom& random_;
    // state_: drawer 요청과 원본 전역의 도장 캐시/후보/revision 상태다.
    RegionPaintState& state_;
    // ownership_: 투표 wrapper와 공유하는 중복 없는 영향 지역 기록이다.
    RegionOwnershipState& ownership_;
    // hooks_: 실제 소유자/일반 finder/지역 조회 경계다.
    RegionPaintHooks hooks_;
};
// 기존 투표의 필수 도장 경계를 실제 몸체에 연결하고 같은 풀 여부를 확인한다.
RegionOwnershipHooks MakeRegionPaintOwnershipHooks(const SidPool& pool,RawRegionPaint& paint,RegionOwnershipHooks hooks);
}
