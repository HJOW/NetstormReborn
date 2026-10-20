// Dude의 이동 정지·지면 판정·도착과 population 삭제를 같은 현재 월드에 연결한다.
#pragma once
#include "o/SquidPostPop.h"
#include <functional>

namespace netstorm::o {
// 원본의 현재 진단 모드와 생성/최초 Pop이 공유하는 population이다.
struct DudeBehaviorState {
    const bool& checking; // 005e4794의 현재 진단 모드이며 도착에서만 조건부로 읽는다.
    SquidPostPopList& population; // 0055a4f0/4/8의 실제 목록·capacity·count이며 stale 꼬리도 보존한다.
};
// 이미 복원한 실제 task/타입/내용물/공통 삭제 몸체를 공급한다.
struct DudeBehaviorHooks {
    std::function<void(Sid,std::uint32_t,float)> task; // 450670의 현재 event/float 지연 설정이다.
    std::function<std::uint32_t(std::uint32_t)> movementType; // 488fe0이 읽는 현재 부모 타입의 프로세스 종류다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,bool)> find; // 4b1a90(parent,type,0,true)의 실제 현재 이동 삭제다.
    std::function<void(Sid,std::uint32_t)> destroy; // 도착의 부모 가상+10 flags0이다.
    std::function<void(Sid,std::uint32_t)> basePostDestroy; // 목록 정리 뒤 전체 공통4b0840이다.
    std::function<void(std::uint32_t)> assertion; // 원본의 진단 행 번호이며 반환하면 이후 분기를 계속한다.
};
// 10.78의450e60/450e80/450f50/450960/450920을 현재 슬롯으로 실행한다.
class RawDudeBehavior {
public:
    RawDudeBehavior(const SidPool& pool,std::span<const std::uint8_t> spots,DudeBehaviorState state,DudeBehaviorHooks hooks); // 같은 256×256 지도와 실제 생성 목록을 빌린다.
    bool Handles(Sid sid) const; // 실제70/508cc8 쌍만 분배한다.
    std::uint32_t Halt(Sid sid) const; // dead 또는 공간 전환이면12, 계속 보행이면10을 반환한다.
    bool CheckSpace(Sid sid) const; // 거의 올림 칸의 bit6/8을 읽어 FALL/5106과 기존 이동 삭제를 수행한다.
    bool Fall(Sid sid) const; // 현재 상태에 추가 관문 없이 task5105와 기존 이동 삭제를 수행한다.
    std::uint32_t Arrival(Sid sid) const; // task별 가상 삭제와1000/1001 원본 반환을 보존한다.
    void PostDestroy(Sid sid,std::uint32_t flags) const; // 일반 객체의 중복 population 항목을 모두 압축한 뒤 공통 후처리한다.
    const SidPool& Pool() const; // 실제 Session/경로 어댑터의 풀 혼용을 검사한다.
    const SquidPostPopList& Population() const; //현재 생성/최초 Pop/삭제와 같은30칸 목록을 읽기 전용으로 제공한다.
private:
    std::span<const std::uint8_t> Raw(Sid sid) const; // free/dead 필터 없이 타입/가상 표만 확인한다.
    void StopMovement(Sid sid) const; // task 효과 뒤 현재 타입을 다시 읽어 같은 lookup/remove를 실행한다.
    const SidPool& pool_; // 현재 raw 부모와 종속 체인이 공유하는 저장소다.
    std::span<const std::uint8_t> spots_; // 현재 동일 월드의 점유/표면 비트다.
    DudeBehaviorState state_; // 현재 진단 모드/목록의 장기 참조다.
    DudeBehaviorHooks hooks_; // 실제 도달한 효과만 요구하며 누락을 정상 처리하지 않는다.
};
}
