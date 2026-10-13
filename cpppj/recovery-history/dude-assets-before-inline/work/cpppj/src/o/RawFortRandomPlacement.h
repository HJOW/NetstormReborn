// Fort 후처리의 단일 랜덤 배치를 현재 raw 공간·지역·생성/Pop에 연결한다.
#pragma once
#include "o/RawFortFinishOwner.h"
#include "o/RawCanonPlacementPipeline.h"
#include "o/SquidFactory.h"

namespace netstorm::o {
// 매 실제 분기에서 읽는 원본 타입/편집 소유자 전역이다.
struct FortRandomPlacementState {
    std::uint32_t islandType{97},edgeFarmType{121}; // 두 PE의 isle/edgeFarm 타입 초기값이다.
    std::uint32_t ownerOverride{}; // 00544ce0/CD005187b8: 0이면 지역 owner/force0 규칙을 유지한다.
};
// 임의 성공/기본값을 제공하지 않는 실제 월드 효과 경계다.
struct FortRandomPlacementHooks {
    std::function<bool(const CanonPlacementQuery&)> mayPlace; // 완전한0049b510/CD00445200 공간·Foot·Graph 검사다.
    std::function<std::int8_t(std::int32_t,std::int32_t)> chunkRegion; // 현재 chunk grid의 signed BYTE다.
    std::function<std::uint32_t(std::int32_t)> regionOwner; // 현재 Islandlist owner0046f320/CD004bda70다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 실제 factory(type,0)와 월드 등록이다.
    std::function<void(Sid,std::uint32_t)> construct; // Retype 후 현재 타입의 생성자만 실행한다. postCreate는 부르지 않는다.
    std::function<bool(Sid)> hasHitPoints; // 현재 dais/abstract/HP 플래그 검사다.
    std::function<std::int32_t(std::uint32_t)> typeHitPoints; // MaxHitPoints 가상이 아닌 현재 타입 HP getter다.
    std::function<void(Sid,std::uint32_t)> setOwner,unpop,repop; // 실제 가상+74/+48/+4c이며 공간/postPop까지 이어진다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 새 객체의 실제 가상+90(flags0)다.
};
// callback 밖의 spot/finder/프레임/모서리/Retype 순서는 실제 명령대로 실행한다.
class RawFortRandomPlacement {
public:
    // 모든 자료와 경계는 같은 월드이며 이 객체보다 오래 살아야 한다. islands는 실제 hash0 배열이다.
    RawFortRandomPlacement(SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,GameRandom& globalRandom,
        FortRandomPlacementState& state,FortRandomPlacementHooks hooks);
    // 0040db90/CD0042c3d0: localRandom 인자는 원본 단일 body에서 사용하지 않는다. 프레임은 globalRandom을 소비한다.
    // 모서리 거부 반환1은 실제 새 SID가 아니므로 호출자가 새 객체로 해석해서는 안 된다.
    std::uint32_t Place(GameRandom& localRandom,const FortRandomPlaceRequest& request) const;
    // 실제 isle WORD 지도/type 검사와 네 모서리의 단락 평가다.
    bool MayReplace(std::int32_t x,std::int32_t y) const;
    // void/same foot assert→type BYTE→현재 생성자만 재실행하는 실제004b1620/CD004ab6b0다.
    void Retype(Sid sid,std::uint32_t type) const;
    // 실제 월드 어댑터 연결의 pool 동일성 검사에 사용한다.
    const SidPool& Pool() const;
private:
    // 실제 일반 finder 순회에서 처음 통과하는 exact type 또는 genus 대상이다.
    Sid Find(float x,float y,std::uint32_t value,bool genus) const;
    // 현재 표면 WORD가 isle 타입인지 읽는다. 추가 dead/free/void 필터를 넣지 않는다.
    bool Island(std::int32_t x,std::int32_t y) const;
    // 고정 P/P·wildcard 요청의 실제 global PRNG/최대256 후보다.
    std::int32_t RandomFrame(std::uint32_t type) const;
    // raw 타입과 타입 표의 범위를 확인하며 현재 값만 조회한다.
    const RiftTypeRecord& Type(std::uint32_t number) const;
    // 직접 초기 프레임/HP 쓰기의 판본 폭과 실제 타입 HP 조회 순서다.
    void Frame(Sid sid,std::uint32_t value) const;
    void HitPoints(Sid sid) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;/* hash_: 좌표별 객체 후보를 찾는 공간 해시. */ const SquidHash& hash_;std::span<const std::uint8_t> spots_; // 현재 공간 배열을 사본 없이 읽는다.
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;std::span<const RiftTypeFrames> frames_; // 실제 로딩 자료다.
    // random_: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom).
    GameRandom& random_;/* state_: 매 실제 분기에서 읽는 원본 타입/편집 소유자 전역이다 (FortRandomPlacementState). */ FortRandomPlacementState& state_;FortRandomPlacementHooks hooks_; // 현재 전역/실제 가상 경계다.
};
// 같은 pool의 완전한 배치 pipeline/factory/타입 HP를 연결한다. 가상 공간·지역·owner는 필수로 남긴다.
FortRandomPlacementHooks MakeFortRandomPlacementHooks(const SidPool& pool,RawCanonPlacementPipeline& placement,
    SquidFactory& factory,const SquidReward& hp,FortRandomPlacementHooks hooks);
// FinishOwner의 필수 단일 placement를 실제 몸체에 연결한다.
FortFinishOwnerHooks MakeFortSinglePlacementHook(const SidPool& pool,const RawFortRandomPlacement& placement,
    FortFinishOwnerHooks hooks);
}
