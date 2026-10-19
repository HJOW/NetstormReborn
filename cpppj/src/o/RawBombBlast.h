// 원본502370 폭발 계열의 가상bc 자격과 가상c0 효과를 처리한다.
#pragma once
#include "o/RawBombTargets.h"

namespace netstorm::o {
// 외부 효과 뒤 읽는 값은 같은 세션의 현재 상태로 유지한다.
struct BombBlastState {
    std::uint32_t boss{},growingRainBlockerType{150},priestType{158}; // 540bc4 권한과5412b0/5412d0의 현재 특별 대상 종류다.
    std::array<std::uint32_t,81> alliances{}; // 595200의 owner*9+owner 동맹DWORD이며0만 비동맹이다.
};
// 피해/삭제/지면 효과/끝 칸 변환은 기존 실제 월드의 몸체에 연결한다.
struct BombBlastHooks {
    std::function<std::int32_t(Sid,std::uint32_t)> radius; // 현재 부모 가상b4의 signed 정수 반경이다.
    std::function<void(Sid,std::uint32_t,std::int32_t,std::uint8_t,Sid)> damage; // 현재 대상 가상98에1000·부모owner BYTE·부모SID를 넘긴다.
    std::function<void(Sid,std::uint32_t,std::uint32_t)> destroy; // 현재 대상 가상10의 실제 flags 삭제다.
    std::function<void(float,float)> ground; // 470ac0의 피해 전 대상 중심에 대한 실제 Dude/난수/시각 효과다.
    std::function<void(Sid,float,float)> edge; // 421f70→4215d0의 부모 중심에 따른 실제 다리 끝 칸 변환이다.
};
// 실제 폭발 계열의 공용 몸체다. 다른 Bomb 가상 표를 임의로 포함하지 않는다.
class RawBombBlast {
public:
    RawBombBlast(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,const RawBombTargets& coordinates,BombBlastState& state,BombBlastHooks hooks); // 같은10.78 풀/188TYPE와 기존 좌표 계산기를 빌린다.
    bool Handles(Sid parent) const; // 현재 부모의 실제 가상 표502370을 확인한다.
    bool MayTarget(Sid target) const; // 41e9c0의 보호 프레임/특별 종류/HP 보유/genus 자격이다. 현재HP값과 구별한다.
    void Apply(Sid parent,Sid target) const; // 41ea30의 QA 제외·동맹 사제·가상 피해/삭제·경계 다리 변환 순서다.
    const SidPool& Pool() const; // 다른 세션의 하위 연결을 거부할 때 사용한다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; // 현재 물리 필드를 원본 폭으로 읽는다.
    const RiftTypeRecord& Type(Sid sid) const; // 현재 TYPE를 범위 검사해 읽는다.
    bool HasHitPoints(Sid sid) const; // 실제4adc60의 TYPE_HAS_HP/dais abstract 판단이다.
    std::array<float,2> TargetCenter(Sid target) const; // 실제4adea0의 현재 발자국 중심이며 보드 제한을 추가하지 않는다.
    bool Allied(std::uint32_t first,std::uint32_t second) const; // 현재9x9 동맹값을 읽는다.
    const SidPool& pool_; // 현재 부모/대상의 물리 슬롯이다.
    std::span<const RiftTypeRecord> types_; // 현재 HP 보유/genus/발자국 속성이다.
    std::span<const RiftTypeFrames> frames_; // 현재 프레임의 원본 코드 보호 비트다.
    const RawBombTargets& coordinates_; // 부모 스냅/반경 계산을 기존 복원과 공유한다.
    BombBlastState& state_; // 권한·종류·동맹의 현재 전역이다.
    BombBlastHooks hooks_; // 실제 도달하는 큰 효과만 필수로 요구한다.
};
}
