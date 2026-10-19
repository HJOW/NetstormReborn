// 원본420bf0의 0x5f 주문 효과 수신과 대상/지연 처리 순서를 복원한다.
#pragma once
#include "o/SidPool.h"
#include <array>
#include <functional>

namespace netstorm::o {
// 기존 TYPE 표에서 이 수신부가 읽는 비용·소리·지연 시간이다.
struct BombEffectType {
    float cost{}; // TYPE+c4의 현재 비용이며 signed64 절삭값의 하위DWORD를 누적한다.
    std::uint32_t sound{}; // TYPE+70의 현재 효과음 토큰이다.
    std::int32_t delay{}; // TYPE+1cc의 지연 효과 시간이다.
};
// 같은 세션의 현재 전역이며 외부 효과 뒤 원본 위치에서 다시 읽는다.
struct BombEffectState {
    std::uint32_t boss{},firstType{117},secondType{118}; // 540bc4 권한과54122c/30의 지연 효과 종류다.
    std::uint32_t processType{57},spentCounter{}; // 541154의 예약 프로세스 종류와595088의 누적 카운터다. SP 잔액과 구별한다.
};
// 큰 하위 몸체는 현재 월드의 실제 구현으로 연결한다.
struct BombEffectHooks {
    std::function<void(Sid)> stopCast; // 425f50의 운반체493 시전 프로세스 종료다.
    std::function<bool(std::uint32_t)> allocate; // 대상DWORD 배열과 지연 객체40바이트의 실제 확보 성공이다.
    std::function<void(Sid,std::uint32_t,Sid)> apply; // 현재 부모 가상 표+c0로 대상에 효과를 준다. 가운데 값은 현재VT 토큰이다.
    std::function<std::uint32_t(std::int32_t,std::uint32_t,std::span<const std::uint32_t>,std::uint32_t)> construct; // 420a40(delay,최초capacity/현재목록,효과1/2)의 같은 확보 메모리 생성이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> argument; // 41e0a0(0,0)의 실제 예약 인자다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> attach; // 41bd20(새객체,현재종류,예약인자,0,0)다. 새객체0 경로도 원본 순서를 유지한다.
    std::function<void(std::array<std::uint32_t,6>)> sound; // 4a9550(sound,0,0,0,1,0)의 실제 장치 경계다.
    std::function<void(Sid,std::uint32_t,std::uint32_t)> destroy; // 현재 가상 표+10의 flags100048 삭제다.
    std::function<void()> release; // 최초 확보한 대상 배열만 해제한다. 지연 객체 수명은 생성/부착 몸체가 담당한다.
};
// 10.78 수신 순서만 처리하며 실제 피해/내용물 삭제를 경계의 성공으로 대신하지 않는다.
class RawBombEffect {
public:
    RawBombEffect(const SidPool& pool,std::span<const BombEffectType> types,BombEffectState& state,BombEffectHooks hooks); // 같은 물리 풀/188TYPE/현재 전역의 수명을 빌린다.
    void Receive(std::span<const std::uint8_t> packet) const; // 부모WORD·현재 대상수/목록을 읽고 효과·지연·소리·삭제를 순서대로 호출한다.
    const SidPool& Pool() const; // 같은 세션의 하위인지 확인한다.
private:
    const BombEffectType& Type(std::uint32_t kind) const; // 현재 종류를 범위 검사하여 TYPE를 조회한다.
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width) const; // 원본 물리 슬롯을 현재 시점에 조회한다.
    const SidPool& pool_; // 효과가 바꾸는 현재50바이트 슬롯들이다.
    std::span<const BombEffectType> types_; // 같은188TYPE의 비용/소리/지연 필드다.
    BombEffectState& state_; // 권한·효과종류·프로세스종류·누적값의 현재 전역이다.
    BombEffectHooks hooks_; // 실제 도달한 하위만 필수로 요구한다.
};
}
