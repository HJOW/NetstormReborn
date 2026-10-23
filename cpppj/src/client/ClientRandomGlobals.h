// 같은 Client의 CRT holdrand와 Subscriber ID 최초시드 전역을 Session 교체와 무관하게 소유한다.
#pragma once
#include "o/CrtRandom.h"
#include "o/SpStore.h"
#include <cstdint>

namespace netstorm::client {
// 원본 한스레드 CRT의 실제DWORD 하나를 장기Borrow wrapper와 기존DWORD 소비자가 함께 사용한다.
class ClientRandomGlobals {
public:
    ClientRandomGlobals()=default; // 새 Client에서만 CRT1/Subscriber 초기화0을 구성한다. 별도 srand는 실행하지 않는다.
    ClientRandomGlobals(const ClientRandomGlobals&)=delete; // Borrow가 빌린 실제DWORD와 초기화 플래그를 복제하지 않는다.
    ClientRandomGlobals& operator=(const ClientRandomGlobals&)=delete; // 살아 있는 소비자의 주소를 대입으로 바꾸지 않는다.
    ClientRandomGlobals(ClientRandomGlobals&&)=delete; // 내부 Borrow의 포인터가 이전 소유자에 남는 이동을 막는다.
    ClientRandomGlobals& operator=(ClientRandomGlobals&&)=delete; // 이미 빌린 난수 객체/상태의 수명을 유지한다.
    o::CrtRandom& Crt() { return crt_; } // Subscriber/파편이 같은 장기 CRT 객체에 seed/next를 실행한다.
    const o::CrtRandom& Crt() const { return crt_; } // 진단도 같은 객체의 현재 상태를 읽는다.
    std::uint32_t& CrtState() { return crtState_; } // Seed/지형의 기존uint32_t& API에 같은 실제holdrand를 제공한다.
    const std::uint32_t& CrtState() const { return crtState_; } // 상태 사본을 만들지 않는 읽기 소비자용 실제DWORD다.
    o::GameRandom& Game() { return game_; } //00532710의단일실제객체를메뉴구름/현재Session이직접빌린다.
    const o::GameRandom& Game() const { return game_; } //현재같은게임난수객체의읽기참조다.
    std::uint32_t& GameMode() { return gameMode_; } //005c89a8현재DWORD를같은Seed/Reseed가참조한다.
    const std::uint32_t& GameMode() const { return gameMode_; } //현재모드의사본을새로만들지않는다.
    std::uint32_t& SubscriberIdInitialized() { return subscriberIdInitialized_; } // 005c89a4의현재DWORD를 생성기와 함께 읽고 쓴다.
    const std::uint32_t& SubscriberIdInitialized() const { return subscriberIdInitialized_; } // 같은 최초시드 상태를 복제 없이 조회한다.
private:
    std::uint32_t crtState_{1}; // 원본 새 CRT TLS holdrand의 최초값1이며 활성 난수 상태는 이한개다.
    o::CrtRandom crt_{o::CrtRandom::Borrow(crtState_)}; // 동일한 실제DWORD를 빌리는 장기wrapper 한개이며 미션마다 다시 만들지 않는다.
    o::GameRandom game_{55}; //보호PE00532710의실제data초기값0x37이다.Session생성/반납으로재초기화하지않는다.
    std::uint32_t gameMode_{}; //005c89a8의실제BSS0이며CRT/IDinit과별개상태다.
    std::uint32_t subscriberIdInitialized_{}; // 원본005c89a4의BSS0이며 Session 생성/반납에서 초기화하지 않는다.
};
}
