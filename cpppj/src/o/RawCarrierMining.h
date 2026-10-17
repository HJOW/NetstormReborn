// CarrierArrival의 채취 통지 작성→송신→같은 지면 로컬 수신을 연결한다.
#pragma once
#include "o/RawFormPacket.h"
#include <functional>
namespace netstorm::o {
// 원본429800 안의 두 실제 큰 효과이며 순서를 뒤집지 않는다.
struct CarrierMiningHooks {
    std::function<void(std::span<const std::uint8_t>)> broadcast; //4e2e30의 현재 연결 표 송신이다.
    std::function<void(std::span<const std::uint8_t>)> receive; //4266c0(0,0,packet)의 실제 로컬 수량/고갈 몸체다.
};
// 전체 도착의 채취 조건/너겟 생성은 기존 RawCarrierArrival이 맡는다.
class RawCarrierMining {
public:
    RawCarrierMining(const SidPool& pool,const FormPacketHeader& header,CarrierMiningHooks hooks); // 현재 풀/헤더 참조는 이 객체보다 오래 살아야 한다.
    void Notify(Sid geyser) const; //40ac20의6바이트 원문을 작성하고 송신 뒤 로컬 수신한다.
    const SidPool& Pool() const; // 실제 수신/Session의 같은 저장소를 확인한다.
private:
    const SidPool& pool_; // 다른 판본이나 다른 수신 풀의 혼합을 확인할 현재 저장소다.
    const FormPacketHeader& header_; // 현재52ee32/33/34의 메시지/프로토콜/선언 길이다. 길이를4 이상으로 보정하지 않는다.
    CarrierMiningHooks hooks_; // 송신 효과가 바꾼 현재 지면을 로컬 수신이 다시 읽는다.
};
}
