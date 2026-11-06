// Client 수명의 두 Subscriber DWORD와 현재 사용자 handle을 소유한다. 모드·Player·State 정리와 분리한다.
#pragma once
#include <cstdint>
#include <array>

namespace netstorm::client {
// 같은 Client의 Init/Session/Chat/재접속이 동일 주소를 빌리며 새 Client에서만 초기값을 구성한다.
class ClientSubscriberGlobals {
public:
    ClientSubscriberGlobals()=default; //00540c74=-1/0054265c=1을 최초 한 번만 구성한다.
    ClientSubscriberGlobals(const ClientSubscriberGlobals&)=delete; // 실제 전역 두 주소를 값 복제로 나누지 않는다.
    ClientSubscriberGlobals& operator=(const ClientSubscriberGlobals&)=delete; // 기존 소비자의 alias를 대입으로 바꾸지 않는다.
    ClientSubscriberGlobals(ClientSubscriberGlobals&&)=delete; // 실제 owner 주소는 수명 동안 고정한다.
    ClientSubscriberGlobals& operator=(ClientSubscriberGlobals&&)=delete; // 소비자가 빌린 참조를 이전 주소에 남기지 않는다.
    std::uint32_t& MySubscriber() { return my_; } //00540c74의 같은 현재 DWORD다.
    const std::uint32_t& MySubscriber() const { return my_; } //Chat prefix가 빌리는 실제 const 참조다.
    std::uint32_t& LoginSubscriber() { return login_; } //0054265c의 같은 현재 DWORD다. 생성 뒤 임의1쓰기를 추가하지 않는다.
    const std::uint32_t& LoginSubscriber() const { return login_; } //Init/World의 현재값 읽기도 같은 주소다.
    std::array<std::uint8_t,64>& Handle() { return handle_; } //PTR00540ca8의 현재 NUL 문자열 물리를 Client 수명으로 빌린다. Session 반납/재생성에서 지우지 않는다.
    const std::array<std::uint8_t,64>& Handle() const { return handle_; } // 같은 현재 이름을 읽으며 설정/Player 이름 사본으로 대체하지 않는다.
private:
    std::uint32_t my_{0xffffffffU}; // 실제 PE 초기 -1이며 메뉴/월드 부재를0으로 표현하지 않는다.
    std::uint32_t login_{1}; // 실제 PE 초기1이며 확인된 직접읽기는 Init/World의 두 위치다.
    std::array<std::uint8_t,64> handle_{}; // 기존 호스트64BYTE/NUL 계약을 유지하는 현재 이름이며 원본32비트 할당 주소/ABI 자체 일치는 주장하지 않는다.
};
}
