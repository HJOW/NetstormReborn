// Client 수명의 실제 Chatmanager 현재 토큰과2712/128BYTE 미초기화 할당들을 소유한다.
#pragma once
#include "o/RawChatManagerPrefix.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <span>

namespace netstorm::client {
// 원문42e050 재초기화는 이전 manager를free하지 않으므로 current와 할당 수명을 구별한다.
class ClientChatManagerGlobals {
public:
    ClientChatManagerGlobals()=default; // 새 Client의 현재0만 구성하고 manager 할당은 공통 startup에서 실행한다.
    ClientChatManagerGlobals(const ClientChatManagerGlobals&)=delete; // 살아 있는 실제 할당/현재 DWORD의 주소를 복제하지 않는다.
    ClientChatManagerGlobals& operator=(const ClientChatManagerGlobals&)=delete; // 토큰과 그 물리 소유권의 중복을 막는다.
    ClientChatManagerGlobals(ClientChatManagerGlobals&&)=delete; // 이미 빌린 현재 슬롯과 BYTE뷰의 주소를 유지한다.
    ClientChatManagerGlobals& operator=(ClientChatManagerGlobals&&)=delete; // 소비자의 alias를 대입으로 끊지 않는다.
    std::uint32_t& Current(); //00548714의 실제 한 DWORD를 직접 빌린다.
    const std::uint32_t& Current() const; // 현재 토큰을 값 사본 없이 읽는다.
    std::uint32_t Allocate(std::uint32_t size); // 실제2712/128BYTE를 초기화 없이 확보하고 실패0/비영opaque토큰을 반환한다.
    std::span<std::uint8_t> Bytes(std::uint32_t token); // 같은 살아 있는 할당을 빌리며0/미등록 토큰은 도달 시 거부한다.
    std::span<const std::uint8_t> Bytes(std::uint32_t token) const; // 쓰기 사본 없이 실제 물리를 읽는 const뷰다.
private:
    // 배열 길이와 실제 new[] 소유권을 같은 토큰에 보관한다.
    struct Allocation {
        std::unique_ptr<std::uint8_t[]> bytes; //{} 없는 new[]의 실제 미초기화 입력이며 임의0채움을 하지 않는다.
        std::size_t size; // 실제 요청한2712 또는128BYTE다.
    };
    std::uint32_t current_{}; // 원시00548714의 최초0이며 Allocate 자체는 이 슬롯을 변경하지 않는다.
    std::uint32_t nextToken_{1}; // host 포인터를32비트로 자르지 않는 비영 수명 토큰이다.
    std::map<std::uint32_t,Allocation> allocations_; // current가 바뀌어도 이전 실제 할당을 계속 보유한다.
};
// 실제 같은 owner를 장기 캡처해 allocate/memory/순수CRTcopy만 연결한다. 나머지 하위는 필수 외부 경계다.
o::ChatManagerPrefixHooks MakeClientChatManagerHooks(std::shared_ptr<ClientChatManagerGlobals> owner);
}
