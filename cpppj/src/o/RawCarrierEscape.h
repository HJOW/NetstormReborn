//429010의 네 방향 회피와 주변 surface 경로 시간 비교를 실제 월드에 연결한다.
#pragma once
#include "o/RawPathRuntime.h"
#include "o/RawSquidFinder.h"
namespace netstorm::o {
//52f83c부터 연속한17DWORD다. x의9번째와 y의첫번째는 같은 물리 항목이다.
struct CarrierEscapeState {
    std::array<std::int32_t,17> directions{0,1,1,1,0,-1,-1,-1,-1,-1,0,1,1,1,0,-1,0}; // 첫난수방향4의중복서쪽도원본그대로다.
};
// 기존 실제 경로/난수/BFS/현재TYPE 속성만 공급한다. 결과 선택은 이 모듈이 수행한다.
struct CarrierEscapeHooks {
    std::function<RawPathRuntime*(Sid)> path; //427ce0의 현재 실제 경로다. 최초 깊이 조회에서만 빌린다.
    std::function<std::int32_t(std::int32_t,std::int32_t)> random; //4558f0(0,5000)의 같은 게임 난수다.
    std::function<RawPathRuntime*(Sid,RawPathPoint)> start; //489080(parent,x,y,0,0,0,0,0)의 실제 새 경로다. 기존 경로를 삭제할 수 있다.
    std::function<Sid(RawPathPoint)> blocker; //4b2070(x,y,10000h)의 현재 운반체 조회다.
    std::function<void(RawPathRuntime&,Sid,RawPathPoint,RawPathPoint,std::uint32_t)> search; //488af0의 같은 실제 임시 BFS다.
    std::function<std::array<float,2>(Sid)> movement; // 탐색 뒤 현재 TYPE의 speed/turningSpeed다.
    std::function<bool(RawPathPoint)> valid; //40e800의 엄격한0/현재float(mapLimit) 내부 검사다.
    std::function<void(std::uint32_t)> assertion; //427d30의 현재 깊이 진단 뒤 계속한다.
    std::function<void(const SquidSearchArea&)> scan; // 독립 관측의422230 정수 영역이며 게임 경로에서는 비워 둔다.
};
// 같은 Pool/Hash/spot/TYPE/현재시계/깊이와 실제 경로 시작을 사용한다.
class RawCarrierEscape {
public:
    RawCarrierEscape(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
        const double& now,const bool& checking,std::span<const std::uint32_t> cardinalTypes,CarrierEscapeState& state,CarrierEscapeHooks hooks); // 참조 자료와 현재 전역은 이 객체보다 오래 살아야 한다.
    std::uint32_t Run(Sid parent,float distance,std::uint32_t event) const; // 깊이관문은1000, 그외실패를포함한회피분기는1001이다.
    const SidPool& Pool() const; // 다른 실제 월드의 하위 연결을 확인한다.
private:
    std::int32_t Depth(Sid parent) const; // 검사 전역과 진단 뒤의 현재 Runtime 깊이를 구분한다.
    RawPathPoint Position(Sid sid) const; // 원본의 조회 시점마다 현재 두 float를 읽는다.
    const RiftTypeRecord& Type(Sid sid) const; // 현재 BYTE 타입을 같은 실제 표의 물리 범위에서 읽는다.
    bool Blocked(RawPathPoint point) const; //0.9999 편향/ftol하위DWORD/감긴선형spot색인의bit10h다.
    bool Start(Sid parent,RawPathPoint destination,std::int32_t depth) const; // 반환한 새 경로에만 깊이를 쓴다.
    const SidPool& pool_; // 같은 실제 부모/후보 raw 저장소다.
    const SquidHash& hash_; // 실제5×5 일반 탐색기의 네 단계 해시다.
    std::span<const RiftTypeRecord> types_; // 현재 flags1 surface/flags2 비행부류다.
    std::span<const std::uint8_t> spots_; //65536칸의 현재 통행/점유 비트다.
    const double& now_; // 후보 탐색 뒤 현재55b4d0 시각이다.
    const bool& checking_; // 현재5e4794 깊이 진단 전역이다.
    std::span<const std::uint32_t> cardinalTypes_; // 기존 선택기와 같은 mutable488fb0 타입4종이다.
    CarrierEscapeState& state_; // 현재 물리 방향표다.
    CarrierEscapeHooks hooks_; // 실제 큰 하위만 도달할 때 요구한다.
};
}
