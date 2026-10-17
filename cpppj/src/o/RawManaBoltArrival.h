// 마나 볼트105의 현재 건설 대상 검색과 실제 도착 분기를 같은 월드에 적용한다.
#pragma once
#include "o/RawSquidFinder.h"
namespace netstorm::o {
// 원본 마나 볼트가 도착 때 제거/예약하는 현재 전역이다.
struct ManaBoltArrivalState {
    std::uint32_t regularType{46},sharedRegularType{61}; // 00541128/00541164의 실제 두 프로세스 타입이다.
    float delay{0.05f}; // 00532750의 event1 예약 payload다.
};
// 도착 계산 밖의 음향/같은 Host/가상 삭제 효과다. 선택된 경계만 필요하다.
struct ManaBoltArrivalHooks {
    std::function<void(float,float,std::string_view)> sound; // 4a9d70(name,loop0)의 현재 위치 요청이다.
    std::function<void(Sid,std::uint32_t)> removeProcesses; // 4b27a0(parent,type)의 모든 해당 프로세스 제거다.
    std::function<bool()> reserveRegular; // 실제 new28 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 확보한 객체로 event1을 부착한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 현재 부모의 가상+10(flags0)이다.
};
// 45aab0/45ab30은 사제 도착이나 teleport134 도착과 다른 몸체다.
class RawManaBoltArrival {
public:
    RawManaBoltArrival(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,ManaBoltArrivalState& state,ManaBoltArrivalHooks hooks); // 참조 대상은 이 모듈보다 오래 살아야 한다.
    Sid Target(Sid bolt) const; // 같은 절삭 칸에서 abstract·!preview·동일 owner인 첫 현재 후보를 반환한다.
    std::uint32_t Arrive(Sid bolt) const; // 음향→대상→두 프로세스 제거/예약 또는 자기 삭제 뒤1000/1001이다.
    const SidPool& Pool() const; // 다른 월드의 경로와 잘못 연결하지 않게 한다.
private:
    const SidPool& pool_;const SquidHash& hash_; // 현재 raw/일반 finder가 읽는 같은 저장소다.
    std::span<const RiftTypeRecord> types_; // finder의 현재 부류 표다.
    ManaBoltArrivalState& state_; // 제거/예약 직전 다시 읽는 실제 전역이다.
    ManaBoltArrivalHooks hooks_; // 계산을 대체하지 않는 실제 효과다.
};
}
