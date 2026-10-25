// 현재 HP와 캡처 마스크에서 고정 연소점 행을 선택하는 원본 Burning 몸체다.
#pragma once
#include "o/RawBurningVisual.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 실제 TYPE 최대HP helper가 읽는 현재 위크 마나 옵션과 타입 전역이다.
struct BurningSourcesState {
    bool weakenedMana{}; // 패치005ca8f0/CD00511c90이 켜지면 해당타입 최대HP를4로 나눈다.
    std::uint32_t manaType{158}; // 실제005412d0/CD0051cbbc의 마나/사제 자산 번호다.
    const bool* sharedWeakenedMana{}; // 실제005ca8f0의 주소이며 Frame 준비 사본보다 우선한다.
    bool WeakenedMana() const { return sharedWeakenedMana ? *sharedWeakenedMana : weakenedMana; } // 연소점 선택의 최대HP 조회 위치에서 현재값을 읽는다.
};
// 00424110/CD004a5d30의 연소점 선택이며 Step/캡처/팔레트/표시는 별도 연결한다.
class RawBurningSources {
public:
    // visual은 기존 실제 Append 몸체이며 pool/random/types/state는 같은 현재 월드여야 한다.
    RawBurningSources(const SidPool& pool,std::span<const RiftTypeRecord> types,
        const RawBurningVisual& visual,GameRandom& random,const BurningSourcesState& state);
    // 현재HP·I 이미지 크기로 목표 행을 구하고 원본 난수/300시도/7점 폴백과 제거를 수행한다.
    void Select(BurningVisualFields& fields) const;
    // 같은 실제 Step/Host와 피해 SID 저장소를 확인한다.
    const SidPool& Pool() const;
private:
    // 원본 min포함/max제외 정수 난수이며 max<=min의 0나눗셈은 거부한다.
    std::int32_t Random(std::int32_t minimum,std::int32_t maximum) const;
    // 원본 char 픽셀0xff 투명 검사와 정상 캡처 범위를 확인한다.
    bool Opaque(const BurningVisualFields& fields,std::int32_t x,std::int32_t y) const;
    // 원본 float 좌표를0 방향 정수화해 rowTaken의 대각선 행을 찾는다.
    std::int32_t Row(const BurningParticle& particle) const;
    const SidPool& pool_; // 현재 victim raw HP/타입을 읽는 같은 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 HasHP/genus/최대HP를 읽는 실제 타입 표다.
    const RawBurningVisual& visual_; // 고정 source 점의 실제150슬롯 Append 몸체다.
    GameRandom& random_; // 다른 게임 기능과 공유하는 실제32비트 난수 전역이다.
    const BurningSourcesState& state_; // 실제 최대HP 조회의 현재 옵션/타입을 매호출 다시 읽는다.
};
}
