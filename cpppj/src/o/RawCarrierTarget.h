// 운반체의 실제 대상 접근/간헐천 이동/귀환 명령을 같은 경로 시작기에 연결한다.
#pragma once
#include "o/RawBoardTarget.h"
#include "o/RawPathStart.h"
namespace netstorm::o {
// 원본 EAX의 Path 주소를 보관한다. 호스트에서는 실제 Path 포인터 폭을 보존하며 직렬화하지 않는다.
using CarrierPathResult=std::uintptr_t;
// 순수 기하와 현재 raw 조회는 내부에서 수행하고 경로 시작/귀환 후보만 외부에 둔다.
struct CarrierTargetHooks {
    std::function<SquidFootprint(Sid)> footprint; // 현재4ade70(target,rectangle,0)의 실제 발자국이다.
    std::function<CarrierPathResult(RawPathStartRequest)> start; // 실제489080의 현재 요청/반환 주소다.
    std::function<Sid(Sid)> chooseReturn; //428ea0의 현재 귀환 후보이며 성공을 주입하는 대체가 아니다.
};
// 각 호출 시 현재 Player AI 포인터와 권한·부모 위치·대상 TYPE를 읽는다.
class RawCarrierTarget {
public:
    RawCarrierTarget(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> players,
        const bool& boss,const RawBoardTarget& board,CarrierTargetHooks hooks); // 참조 수명과 PATCH9 Player 폭을 검증한다.
    CarrierPathResult StartTarget(Sid parent,Sid target,std::uint32_t replaceStart,std::uint32_t helper) const; //4894e0: 첫 접근점 뒤0/2/4/6 순서로 최대 네 번 재시도한다.
    CarrierPathResult GoReturn(Sid parent,Sid target) const; //4276a0: AI가 없으면 처음 접근점 한 번만 시도한다.
    CarrierPathResult GoGeyser(Sid parent,Sid target) const; //427600: 같은 AI 검사 뒤 free/현재geyser genus를 검사한다.
    CarrierPathResult GoHome(Sid parent) const; //428fd0: 현재boss/귀환 후보 뒤 StartTarget(0,0)이다.
    const SidPool& Pool() const; // 같은 Session 경로/공간의 혼합을 검사한다.
private:
    RawPathPoint Position(Sid sid) const; // 최초 또는 발자국 효과 뒤 원본 시점의 두 float다.
    bool Controlled(Sid sid) const; //40f100: owner0은 즉시 거짓, 그 외 현재Player+b0을 읽는다.
    const SidPool& pool_; // 실제 부모/대상 raw의 동일 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 대상 TYPE의flags2를 조회한다.
    std::span<const std::uint8_t> players_; // 실제9개Player의현재AI 포인터를 직접 읽는다.
    const bool& boss_; // 귀환 wrapper 시작 때 현재 권한을 읽는다.
    const RawBoardTarget& board_; // 실제41d530/41d400이며 Graph 접근점과 구별한다.
    CarrierTargetHooks hooks_; // 실제 발자국/경로 시작/귀환 선택의 큰 하위다.
};
}
