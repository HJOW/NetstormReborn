// Client 수명의 실제 Chatmanager 현재 토큰과 미초기화 객체/수신자 배열을 소유한다.
#pragma once
#include "o/RawChatManagerPrefix.h"
#include "client/RawChatFormat.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <span>

namespace netstorm::client {
struct ChatSubmitHooks; // 실제 제출의 메모리 장치만 같은 소유자에 연결하며 파서 헤더 결합을 줄인다.
struct ChatPacketHooks; // 일반 송신 부모도 같은 manager/수신자 배열의 실제 소유자를 빌린다.
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
    std::span<std::uint8_t> FormatCapture(); //0059a830 공통 Regexp 캡처 중 서식 명령이 접근하는 앞81BYTE를 같은 Client 수명으로 빌린다.
    std::uint32_t Allocate(std::uint32_t size); // 실제2712/128BYTE를 초기화 없이 확보하고 실패0/비영opaque토큰을 반환한다.
    std::uint32_t AllocateRecipientBuffer(std::uint32_t size); // 원문32슬롯 확장의 요청 BYTE를 별도 수신자 할당으로 확보한다.
    void ReleaseRecipientBuffer(std::uint32_t token); // 이전 수신자 배열을 반납하며 manager 객체 자체의 해제는 거부한다.
    std::span<std::uint8_t> Bytes(std::uint32_t token); // 같은 살아 있는 할당을 빌리며0/미등록 토큰은 도달 시 거부한다.
    std::span<const std::uint8_t> Bytes(std::uint32_t token) const; // 쓰기 사본 없이 실제 물리를 읽는 const뷰다.
private:
    // 배열 길이와 실제 new[] 소유권을 같은 토큰에 보관한다.
    struct Allocation {
        std::unique_ptr<std::uint8_t[]> bytes; //{} 없는 new[]의 실제 미초기화 입력이며 임의0채움을 하지 않는다.
        std::size_t size; // manager2712 또는 초기/확장 수신자 배열의 실제 요청 BYTE다.
        bool recipientBuffer; // manager 객체와 반납 가능한 수신자 저장소를 물리 크기와 별도로 구별한다.
    };
    std::uint32_t AllocateStorage(std::uint32_t size,bool recipientBuffer); // 값 초기화 없는 공통 할당과 비영 토큰 등록을 수행한다.
    std::uint32_t current_{}; // 원시00548714의 최초0이며 Allocate 자체는 이 슬롯을 변경하지 않는다.
    std::array<std::uint8_t,kChatFormatCaptureBytes> formatCapture_{}; // BSS0인 공통 캡처의 현재 복원된 앞81BYTE다. 명령/manager 재초기화 때 지우지 않는다.
    std::uint32_t nextToken_{1}; // host 포인터를32비트로 자르지 않는 비영 수명 토큰이다.
    std::map<std::uint32_t,Allocation> allocations_; // current 교체 때 이전 manager를 유지하며 수신자 확장만 이전 배열을 명시 반납한다.
};
// 실제 같은 owner를 장기 캡처해 allocate/memory/순수CRTcopy만 연결한다. 나머지 하위는 필수 외부 경계다.
o::ChatManagerPrefixHooks MakeClientChatManagerHooks(std::shared_ptr<ClientChatManagerGlobals> owner);
ChatSubmitHooks MakeClientChatSubmitHooks(std::shared_ptr<ClientChatManagerGlobals> owner); // 같은 수명 소유자의 memory/allocate/release만 실제 제출에 연결한다.
ChatPacketHooks MakeClientChatPacketHooks(std::shared_ptr<ClientChatManagerGlobals> owner); // 송신의 현재 물리와0042d1b0 추가/확장/반납을 같은 실제 소유자에 연결한다. 전역/전송/수신은 caller의 장치다.
}
