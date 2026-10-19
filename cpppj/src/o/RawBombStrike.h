// 원본 네 직접 피해 주문의 가상bc 자격과c0 효과·공유 난수 순서를 처리한다.
#pragma once
#include "o/SidPool.h"
#include "o/SpStore.h"
#include <functional>

namespace netstorm::o {
// 큰 효과 뒤 원본 위치에서 다시 읽는 같은 세션의 현재 전역이다.
struct BombStrikeState {
    std::uint32_t boss{},specialType{138},firstEffectType{174},hailEffectType{177}; // 540bc4 권한과541280/541310/54131c의 현재 종류다.
};
// 실제 월드/음향/빔/투영은 같은 세션의 기존 몸체로 연결한다.
struct BombStrikeHooks {
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 4af530(kind,2)의 실제 생성이다.
    std::function<void(Sid,std::uint32_t,float,float,std::uint32_t)> pop; // 생성 뒤 현재 가상90의(x,y,0) 호출이다.
    std::function<void(Sid,std::uint32_t,std::int32_t,std::uint8_t,Sid)> damage; // 현재 가상98에 피해·현재 부모owner BYTE·부모SID를 전달한다.
    std::function<void(Sid,std::uint32_t,std::uint32_t)> destroy; // 특별 대상의 현재 가상10 삭제flags다.
    std::function<void(float,float)> sound; // 현재 부모 중심의thunderCrack.wav/flags0 음향이다.
    std::function<void(Sid,Sid)> beam; // 472700의 현재QA와 대상 사이 실제 빔이다.
    std::function<std::array<std::int32_t,2>(float,float)> project; // 497220의 실제 화면 투영이며 같은 캡처 좌표로 두 번 호출한다.
    std::function<void(Sid,std::array<std::int32_t,2>,std::array<std::int32_t,2>,float)> column; // 472770의 첫 점y=0·둘째 투영점·수명5다.
};
// 정확한 네 가상 표만 분배한다. 다른 주문의 자격 부수효과를 임의로 대체하지 않는다.
class RawBombStrike {
public:
    RawBombStrike(const SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,BombStrikeState& state,BombStrikeHooks hooks); // 같은10.78 풀/188TYPE/공유 게임 난수를 빌린다.
    bool Handles(Sid parent) const; // 현재 가상 표가 복원한 네 종류인지 확인한다.
    std::uint32_t MayTarget(Sid parent,Sid target) const; // 첫 계열의 원래 마스크 반환과 나머지0/1 반환을 구별한다.
    void Apply(Sid parent,Sid target) const; // QA 제외·현재 효과·난수 반복·피해 전 HP 조기 종료를 실행한다.
    const SidPool& Pool() const; // 다른 월드와의 하위 혼용을 검사한다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; // 현재 비정렬 물리 필드를 원본 폭으로 읽는다.
    const RiftTypeRecord& Type(Sid sid) const; // 현재 TYPE를 범위 검사하여 읽는다.
    std::array<float,2> Center(Sid sid) const; // 4adea0의 현재 발자국 중심을float로 저장한다.
    void Hit(Sid parent,Sid target,std::int32_t amount) const; // 호출 시점의 가상 표/owner를 다시 읽어 실제 피해를 요청한다.
    void Spawn(Sid target,std::uint32_t kind) const; // 생성 뒤 현재 대상 중심을 읽고 새 객체의 현재 가상90을 호출한다.
    void Sound(Sid parent) const; // 요청 시점의 부모 중심을 읽어 실제 음향을 요청한다.
    const SidPool& pool_; // 같은 물리 부모/QA/대상의 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 genus/HP 보유/발자국 표다.
    GameRandom& random_; // 다른 효과/월드와 공유하는 원본 게임 난수다.
    BombStrikeState& state_; // 큰 효과가 변경할 수 있는 현재 종류/권한이다.
    BombStrikeHooks hooks_; // 실제로 도달한 큰 효과만 필수로 요구한다.
};
}
