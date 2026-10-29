// 실제 미초기화 할당과 opaque 토큰을 연결하며 Chat 본체/설정/명령등록의 성공을 대신하지 않는다.
#include "client/ClientChatManagerGlobals.h"
#include "client/RawChatPacket.h"
#include "client/RawChatSubmit.h"
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 같은 current DWORD를 초기화 없이 직접 전달한다.
std::uint32_t& ClientChatManagerGlobals::Current() { return current_; }
// 읽기 소비자도 같은 주소를 유지한다.
const std::uint32_t& ClientChatManagerGlobals::Current() const { return current_; }
// 동일 공통 캡처의 뒤쪽 미작성 바이트와 재제출 흔적을 유지한다.
std::span<std::uint8_t> ClientChatManagerGlobals::FormatCapture() { return formatCapture_; }
// 원문 두 크기만 할당하며 새 current 대입/이전 할당 해제를 임의로 수행하지 않는다.
std::uint32_t ClientChatManagerGlobals::Allocate(std::uint32_t size) {
    if (size!=o::kChatManagerBytes && size!=o::kChatManagerBufferBytes) throw std::invalid_argument("실제 Chatmanager 할당 크기 오류");
    return AllocateStorage(size,size==o::kChatManagerBufferBytes);
}
// 수신자 저장소는 원문 DWORD 배열 요청의 양수 BYTE 폭을 그대로 할당한다.
std::uint32_t ClientChatManagerGlobals::AllocateRecipientBuffer(std::uint32_t size) {
    if (!size || (size&3U)) throw std::invalid_argument("Chat 수신자 배열의 실제 DWORD 폭 오류");
    return AllocateStorage(size,true);
}
// 새 current 쓰기/0채움 없이 실제 배열과 할당 종류를 같은 토큰에 보관한다.
std::uint32_t ClientChatManagerGlobals::AllocateStorage(std::uint32_t size,bool recipientBuffer) {
    if (nextToken_==0) throw std::overflow_error("Chatmanager host 토큰 소진");
    std::unique_ptr<std::uint8_t[]> memory(new(std::nothrow) std::uint8_t[size]); //값 초기화 없는 실제 할당이며 검사도 미작성 BYTE를 읽지 않는다.
    if (!memory) return 0;
    const auto token=nextToken_; //map 삽입 성공 뒤에만 다음 토큰으로 진행한다.
    allocations_.emplace(token,Allocation{std::move(memory),size,recipientBuffer});++nextToken_;return token;
}
// free(NULL)은 원문처럼 허용하며 이미 반납된 토큰/manager 자체는 실제 접근 오류다.
void ClientChatManagerGlobals::ReleaseRecipientBuffer(std::uint32_t token) {
    if (!token) return;const auto found=allocations_.find(token); // 토큰의 종류는 크기 추측으로 대체하지 않는다.
    if (found==allocations_.end() || !found->second.recipientBuffer) throw std::out_of_range("Chat 수신자 반납의 실제 할당 없음/종류 오류");
    allocations_.erase(found);
}
//0/미등록 토큰은 실제 선택된 메모리 접근에서 명시 물리 오류로 드러낸다.
std::span<std::uint8_t> ClientChatManagerGlobals::Bytes(std::uint32_t token) {
    const auto found=allocations_.find(token); //current와 독립인 실제 할당 표다.
    if (token==0 || found==allocations_.end()) throw std::out_of_range("Chatmanager 실제 할당 토큰 없음");
    return {found->second.bytes.get(),found->second.size};
}
//const 조회도 같은 실제 배열을 반환한다.
std::span<const std::uint8_t> ClientChatManagerGlobals::Bytes(std::uint32_t token) const {
    const auto found=allocations_.find(token); //읽기에서 별도 캐시나 snapshot을 만들지 않는다.
    if (token==0 || found==allocations_.end()) throw std::out_of_range("Chatmanager 실제 할당 토큰 없음");
    return {found->second.bytes.get(),found->second.size};
}
// 반환한 장치가 마지막 사용 때까지 owner를 보유하므로 외부 shared_ptr 반납으로 실제 할당이 사라지지 않는다.
o::ChatManagerPrefixHooks MakeClientChatManagerHooks(std::shared_ptr<ClientChatManagerGlobals> owner) {
    if (!owner) throw std::invalid_argument("Chatmanager 실제 소유자 없음");
    o::ChatManagerPrefixHooks hooks; //registerCommand/readConfig/max/minor/core는 실제 도달 시 계속 필수다.
    hooks.allocate=[owner](auto size) { return owner->Allocate(size); };
    hooks.memory=[owner](auto token) { return owner->Bytes(token); };
    return o::BindChatManagerMemoryCopy(std::move(hooks));
}
// 조회/확장/반납은 같은 실제 할당을 사용하며 나머지 조회/분배는 계속 필수 외부 장치다.
ChatSubmitHooks MakeClientChatSubmitHooks(std::shared_ptr<ClientChatManagerGlobals> owner) {
    if (!owner) throw std::invalid_argument("Chat 제출의 실제 소유자 없음");ChatSubmitHooks hooks;
    hooks.memory=[owner](auto token) { return owner->Bytes(token); };
    hooks.allocate=[owner](auto size) { return owner->AllocateRecipientBuffer(size); };
    hooks.release=[owner](auto token) { owner->ReleaseRecipientBuffer(token); };
    return hooks;
}
// 송신에 의한 자기 추가도 기존32슬롯 확장 몸체를 사용하고 이전 배열을 실제 반납한다.
ChatPacketHooks MakeClientChatPacketHooks(std::shared_ptr<ClientChatManagerGlobals> owner) {
    if (!owner) throw std::invalid_argument("Chat 패킷의 실제 소유자 없음");ChatPacketHooks hooks;
    hooks.memory=[owner](auto token) { return owner->Bytes(token); };
    hooks.addRecipient=[owner](auto manager,auto subscriber) { AddChatRecipient(manager,subscriber,MakeClientChatSubmitHooks(owner)); };
    return hooks;
}
}
