// 원본0x5c/41e860의 원격 비용 처리와 주문 Regular 예약이다.
#pragma once
#include "o/SidPool.h"
#include <functional>
namespace netstorm::o {
// 실제 경제/예약 효과는 같은 Session에 연결하며 도달하지 않은 경계는 요구하지 않는다.
struct BombActivationHooks {
    std::function<std::int32_t(std::uint32_t)> cost; //현재 TYPE의 복호화 비용 단위다.
    std::function<void(std::uint32_t,float)> subtract; //41df80의 원격 소유자 SP 증감이다.
    std::function<void(std::uint32_t)> assertion; //Bomb.cpp206의 복귀 가능한 genus 진단이다.
    std::function<bool(std::uint32_t)> allocate; //new40의 실제 확보 성공 여부다.
    std::function<void(std::uint32_t,Sid,float)> construct; //496f00(event0,parent,0.1)의 같은 확보 메모리 생성이다.
};
// 생성/피해가 아니라 명령 수신과 예약만 복원하며 호출자의 현재 소유자 전역을 빌린다.
class RawBombActivation {
public:
    RawBombActivation(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& localOwner,BombActivationHooks hooks); //10.78의 같은 풀/TYPE/경제를 사용한다.
    RawBombActivation(const SidPool&,std::span<const RiftTypeRecord>,std::uint32_t&&,BombActivationHooks)=delete; //사라지는 임시 소유자 참조를 금지한다.
    RawBombActivation(const SidPool&,std::span<const RiftTypeRecord>,const std::uint32_t&&,BombActivationHooks)=delete; //const 임시 소유자도 참조 수명보다 먼저 사라지므로 금지한다.
    void Receive(std::span<const std::uint8_t> packet) const; //6바이트 패킷의 부모가 살아 있으면 원격 비용과 Regular를 처리한다.
    const SidPool& Pool() const; //다른 Session의 모듈을 혼합하지 않도록 확인한다.
private:
    const SidPool& pool_; //현재 주문 객체의 물리50BYTE 저장소다.
    std::span<const RiftTypeRecord> types_; //효과 뒤 바뀐 현재 TYPE의 genus를 다시 읽는다.
    const std::uint32_t& localOwner_; //원본540c70의 현재 전체DWORD다.
    BombActivationHooks hooks_; //실제 경제/예약의 선언 경계다.
};
}
