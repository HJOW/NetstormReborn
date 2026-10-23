// 실제 미초기화 할당과 opaque 토큰을 연결하며 Chat 본체/설정/명령등록의 성공을 대신하지 않는다.
#include "client/ClientChatManagerGlobals.h"
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 같은 current DWORD를 초기화 없이 직접 전달한다.
std::uint32_t& ClientChatManagerGlobals::Current() { return current_; }
// 읽기 소비자도 같은 주소를 유지한다.
const std::uint32_t& ClientChatManagerGlobals::Current() const { return current_; }
// 원문 두 크기만 할당하며 새 current 대입/이전 할당 해제를 임의로 수행하지 않는다.
std::uint32_t ClientChatManagerGlobals::Allocate(std::uint32_t size) {
    if (size!=o::kChatManagerBytes && size!=o::kChatManagerBufferBytes) throw std::invalid_argument("실제 Chatmanager 할당 크기 오류");
    if (nextToken_==0) throw std::overflow_error("Chatmanager host 토큰 소진");
    std::unique_ptr<std::uint8_t[]> memory(new(std::nothrow) std::uint8_t[size]); //값 초기화 없는 실제 할당이며 검사도 미작성 BYTE를 읽지 않는다.
    if (!memory) return 0;
    const auto token=nextToken_; //map 삽입 성공 뒤에만 다음 토큰으로 진행한다.
    allocations_.emplace(token,Allocation{std::move(memory),size});++nextToken_;return token;
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
}
