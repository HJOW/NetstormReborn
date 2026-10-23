// 원본 PE의 이름/콜백/표시/count 인접 표를 Client 수명에 같은 바이트로 소유한다.
#pragma once
#include "client/RawDotCommandRegister.h"
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::client {
// 최초 say 슬롯과 count1은 PE 정적 데이터이며 Chat 초기화 때 빈 표로 다시 만들지 않는다.
class ClientDotCommandGlobals {
public:
    ClientDotCommandGlobals(); // 최초00531b30~00532537의 실제 데이터만 한 번 구성한다.
    ClientDotCommandGlobals(const ClientDotCommandGlobals&)=delete; // 빌린 인접 표의 주소를 복제하지 않는다.
    ClientDotCommandGlobals& operator=(const ClientDotCommandGlobals&)=delete; // 사용 중인 같은 표를 다른 소유자 값으로 바꾸지 않는다.
    ClientDotCommandGlobals(ClientDotCommandGlobals&&)=delete; // 등록 장치가 캡처한 실제 물리 주소를 유지한다.
    ClientDotCommandGlobals& operator=(ClientDotCommandGlobals&&)=delete; // 살아 있는 참조를 이동 대입으로 끊지 않는다.
    std::span<std::uint8_t> Bytes(); // count를 따로 복사하지 않고 전체2568BYTE의 같은 가변 물리를 빌린다.
    std::span<const std::uint8_t> Bytes() const; // 같은 표를 읽는 뷰이며 별도 캐시를 생성하지 않는다.
    std::uint32_t Register(std::span<const std::uint8_t> name,std::uint32_t callback,std::uint32_t flag,const DotCommandRegisterHooks& hooks={}); // 실제0043f660 본체에 같은 표를 전달한다.
private:
    std::array<std::uint8_t,kDotCommandStateBytes> bytes_{}; // PE의 영 초기화 영역과 비영 정적 슬롯을 함께 보관한다.
};
}
