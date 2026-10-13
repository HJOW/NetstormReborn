// 원본 표면 지도·spot·프레임 접합을 사용하는 BFS와 역추적 경로 축약을 복원한다.
#pragma once
#include "o/RawPathRuntime.h"
#include "o/Bridge.h"
#include "o/SquidHash.h"
#include <memory>

namespace netstorm::o {
// Path.cpp가 직접 읽고 바꾸는 전역이다. 명령을 만든 부모의 owner를 별도 이동 허가 정책으로 바꾸지 않는다.
struct RawPathSearchState {
    bool graphsEnabled{true}; // 00540414/CD005207f8: GraphAt 조회 허용 전역이며 번호254는 Graph 없음이다.
    bool searchEnabled{true}; // 00540be4/CD0050f280: 원본 kenGo 활성 전역이다. 같은 Graph·허용 지면에서 꺼졌으면 원본 assert 경로다.
    bool avoidFlagTwo{true}; // 00540be8/CD0050f284: 역추적 후보 raw+12 &6 ==2에 30 비용을 더한다.
    std::int32_t policySelector{6}; // 패치005318ec의 PE 초기값6. 5면 directionPrerequisite[3]을5로 바꾼다. CD에는 이 분기가 없다.
    std::array<std::uint32_t,8> directionPrerequisite{0,5,0,20,0,80,0,65}; // 대각선은 양옆 직선 방향의 허용 비트가 필요하다.
    std::array<std::int32_t,8> directionOrder{0,2,4,6,1,3,5,7}; // 원본 방문/동률 선택 우선순위다.
    std::uint32_t fenceMarkType{90},windArcherType{140}; // surface가 아닌 예외 Graph BYTE getter의 현재 타입 번호다.
};
// 원본 signed WORD 거리표와 두 BYTE 큐의 미사용 흔적까지 보존한다.
struct RawPathSearchWork {
    // distance: 256×256 칸의 원본 signed WORD BFS 거리표.
    std::array<std::int16_t,65536> distance{};
    // queue: BFS 방문 순서대로 X·Y BYTE 좌표를 담는 16,384항목 큐.
    std::array<std::array<std::uint8_t,2>,16384> queue{};
};
// 같은 실제 pool/hash0/spot/현재 타입·프레임에서만 경로를 계산한다.
class RawPathSearch {
public:
    // 모든 참조는 검색기보다 오래 살아야 한다. hash0의 실제 배열을 읽으며 표면 사본을 만들지 않는다.
    RawPathSearch(const SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,RawPathSearchState& state);
    // 0040eaf0: 좌표+0.9999를 절삭한 실제 표면 SID다. 보드 밖은0이다.
    Sid SurfaceAt(RawPathPoint position) const;
    // 00462c30/CD0045b750: 시작/목적 표면의 같은 Graph 번호를 비교할 실제 조회다.
    std::optional<std::uint8_t> GraphAt(RawPathPoint position) const;
    // 00441f20/CD004d2ef0: 같은 SID는 바로 연결되며 다른 SID는 현재 타입/물리 프레임으로 접합을 계산한다.
    bool Connects(Sid first,Sid second,std::int32_t direction) const;
    // 00488370/CD00402750: sx/sy에서 BFS한 뒤 dx/dy에서 역추적하고 실제 방향 변경점 두 개를 저장한다.
    // 출력은 역순이다. cardinalOnly가false이면 거리 한 단계 건너뛰는 대각선 축약도 허용한다.
    bool SearchCells(RawPathRuntime& output,int sx,int sy,int dx,int dy,bool cardinalOnly);
    // 00488af0/CD00402100: 현재 Graph/목적 지면을 검사하고 목적→시작 BFS를 불러 최종 start→dest 배열을 만든다.
    bool Search(RawPathRuntime& output,Sid parent,RawPathPoint start,RawPathPoint destination,std::uint32_t flags);
    // 원본 전체 거리표/큐를 독립 native 결과와 대조한다.
    const RawPathSearchWork& Work() const;
    // Root의 실제 RawPathProcess Init 표면 접두에 같은 풀인지 확인할 수 있게 한다.
    const SidPool& Pool() const;
private:
    // 현재 물리 프레임을 원본 signed side/variant 코드로 읽는다.
    FrameCode Frame(Sid sid) const;
    // 경로 배열에 raw append하고 최대 길이 assert를 진단한다.
    void Append(std::vector<RawPathPoint>& points,std::vector<std::int32_t>& directions,RawPathPoint point,std::int32_t direction,const RawPathRuntime& output) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // hash_: 좌표별 객체 후보를 찾는 공간 해시.
    const SquidHash& hash_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::span<const RiftTypeFrames> frames_;
    // state_: Path.cpp가 직접 읽고 바꾸는 전역이다. 명령을 만든 부모의 owner를 별도 이동 허가 정책으로 바꾸지 않는다 (RawPathSearchState).
    RawPathSearchState& state_;
    std::unique_ptr<RawPathSearchWork> work_; // 원본 lazy heap의 큰 배열을 호스트 stack에 두지 않는다.
};
}
