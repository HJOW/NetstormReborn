// 사제 배치 검사의 전역 초기화·강제 허용·모양 여백·지도 경계 접두를 복원한다.
#pragma once
#include "o/RawPriestSpawn.h"

namespace netstorm::o {
// 사제 배치 검사 검사 범위를 지정하는 사각형이다.
struct PriestPlacementRect {
    float left{},top{},right{},bottom{}; // 모양 조회가 반환하는 원본 float 사각형이다.
};
// 사제 배치 검사 처리 중 조회·갱신하는 상태 묶음이다.
struct PriestPlacementState {
    std::uint32_t forcePlacement{}; // 0055a488 / CD 00537e98: 모든 배치를 즉시 허용한다.
    std::int32_t localPlayer{}; // 00540c70 / CD 0050f6c8: 부호 있는 owner BYTE와 비교하는 DWORD다.
    std::uint32_t blockedRelation{}; // 0059ab2c / CD 0051cbfc: 검사 진입마다 0으로 지운다.
};
// 사제 배치 검사 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct PriestPlacementHooks {
    // 0043cbd0 / CD 004ed7c0: (원본 타입 번호, 첫 인자, flags)의 모양 조회 경계다.
    std::function<PriestPlacementRect(std::uint32_t,std::uint32_t,std::uint32_t)> shape;
    // 0049b661~0049bfbb / CD 0044537c~00445ce2: 미복원된 미리보기·충돌·지역/관계 처리다.
    // localOwner는 모양 조회 전에 캡처한 비교 결과이며 이 경계가 필요한 전역 효과를 수행한다.
    std::function<bool(const PriestPlacementQuery&,bool)> inspectGeometry;
};
// 사제 배치 검사의 전역 초기화·강제 허용·모양 여백·지도 경계 접두를 복원한다.
class RawPriestPlacement {
public:
    // 사제 genus 전용 접두다. 풀 판본·타입 표·전역·하위 경계의 참조 수명은 호출자가 보장한다.
    RawPriestPlacement(const SidPool& pool,std::span<const RiftTypeRecord> types,
        PriestPlacementState& state,PriestPlacementHooks hooks);
    // 원본 값 인자를 보존하며 별도 소유자 마스크 없이 owner의 low BYTE를 부호 확장해 비교한다.
    bool MayPlace(PriestPlacementQuery query) const;
    // 사제 생성과 같은 풀/판본에 연결하는지 확인한다.
    const SidPool& Pool() const;
private:
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // state_: 처리 중 조회·갱신하는 상태 (PriestPlacementState).
    PriestPlacementState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (PriestPlacementHooks).
    PriestPlacementHooks hooks_;
};
// 사제 생성의 mayPlace 경계만 위 접두로 연결하고 다른 생성 효과는 유지한다.
PriestSpawnHooks MakePriestPlacementHooks(const SidPool& pool,const RawPriestPlacement& placement,PriestSpawnHooks hooks);
}
