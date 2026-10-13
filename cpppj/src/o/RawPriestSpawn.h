// 사제 해방의 자리 탐색·생성 호출과 Carrier 검사 분기를 복원한다.
#pragma once
#include "o/RawDamageableRelease.h"

namespace netstorm::o {
// 사제 배치 검사 검사에 전달하는 입력 값이다.
struct PriestPlacementQuery {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    std::uint32_t type{};
    // x: X 좌표.
    // y: Y 좌표.
    float x{},y{};
    std::uint32_t flags{},owner{},mode{}; // 원본 검사 인자는 (type,x,y,0,WORD&0x7f,0)이다.
};
// 사제 생성 처리 중 조회·갱신하는 상태 묶음이다.
struct PriestSpawnState {
    std::uint32_t bridgeType{82}; // 005411a0 / CD 0051ca8c: WORD 쓰기 뒤 표면 알림 대상이다.
    bool checkingPlacement{true}; // 005e4794: 패치에서 모든 자리 탐색 실패 시 debug assert를 보고하는 조건이다.
};
// 사제 생성 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct PriestSpawnHooks {
    // 0049b510 / CD 00445200: 타입별 배치 가능 검사. 아직 복원하지 않은 공간/관계 조회 경계다.
    std::function<bool(const PriestPlacementQuery&)> mayPlace;
    // 004af530 / CD 004ab390: 생성(type,0), 초기화된 할당 자산 SID를 반환한다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create;
    // 가상 +0x74 소유자와 +0x90 Pop은 아직 하위 경계다.
    std::function<void(Sid,std::uint32_t)> setOwner;
    // pop: 대상을 지정 위치의 공간에 등록하는 Pop 콜백.
    std::function<void(Sid,float,float,std::uint32_t)> pop;
    // WORD 쓰기 뒤 현재 다리 타입이면 부르는 표면 검사다.
    std::function<void(Sid)> notifySurface;
    // 00426230 / CD 004e38f0의 실제 genus 분기 뒤 가상 +0xcc Carrier 검사 요청이다.
    std::function<void(Sid)> checkCarrier;
};
// 사제 해방의 자리 탐색·생성 호출과 Carrier 검사 분기를 복원한다.
class RawPriestSpawn {
public:
    // 풀·타입 표·공간·현재 전역은 이 객체보다 오래 살아야 한다. 모든 하위 효과를 명시적으로 받는다.
    RawPriestSpawn(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
        const PriestSpawnState& state,PriestSpawnHooks hooks);
    // 0044b2e0 / CD 00461430: 거의 올림→132개 나선 후보→생성/WORD/소유자/Pop/Carrier 검사다.
    // WORD 전체를 복사하며 소유자와 배치 검사에만 하위 7비트를 전달한다.
    void Spawn(float x,float y,std::uint32_t type,std::uint16_t word) const;
    // 같은 풀의 종속 해방 연결을 확인한다.
    const SidPool& Pool() const;
private:
    // Pop 뒤의 현재 새 자산 타입으로 Carrier genus를 판정한다.
    void CheckCarrier(Sid sid) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // state_: 처리 중 조회·갱신하는 상태 (const PriestSpawnState).
    const PriestSpawnState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (PriestSpawnHooks).
    PriestSpawnHooks hooks_;
};
// 같은 풀의 종속 해방에서 사제 생성/자리 탐색 경계만 실제 몸체로 연결한다.
DamageableReleaseHooks MakePriestSpawnHooks(const SidPool& pool,const RawPriestSpawn& spawn,DamageableReleaseHooks hooks);
}
