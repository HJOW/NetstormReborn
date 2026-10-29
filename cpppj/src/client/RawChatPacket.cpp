// 패킷 종류0x41/버전3과 현재 전역의 재조회·전투/비전투의 서로 다른 복사 폭을 보존한다.
#include "client/RawChatPacket.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string>
#include <vector>

namespace netstorm::client { // 같은 manager/배열을 caller 수명 동안 빌리는 원문 송신 부품이다.
namespace { // 범위 확인과 원문 비정렬 little endian 접근만 처리한다.
constexpr std::uint32_t kSubscriber=0x540c74,kBattle=0x594fbc,kChannelOne=0x594fb8,kMetamap=0x594fc8; // 원문이 각 호출 위치에서 읽는 현재 네 DWORD다.
// 미선택 장치를 미리 요구하지 않으며 실제 도달한 하위의 누락은 그 자리에서 중단한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Chat 패킷의 선택된 실제 장치 미연결");return function; }
// 명시 물리 안의 실제 BYTE/WORD/DWORD만 낮은 BYTE부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Chat 패킷 읽기의 실제 물리 부족");std::uint32_t value{};
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(bytes[at+index])<<(index*8); // 현재 폭 밖의 인접 BYTE를 읽지 않는다.
    return value;
}
// 주변 꼬리를 유지하며 지정 원문 폭만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Chat 패킷 쓰기의 실제 물리 부족");
    for (std::size_t index=0;index<width;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // WORD/BYTE 절단도 이 실제 store에서 발생한다.
}
// 첫NUL까지의 현재 바이트를 빌리며 caller의 실제 물리 밖 읽기는 명시적으로 거부한다.
std::string_view Text(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("Chat 패킷 문자열의 실제 NUL 없음");
    return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
}

// 이름 포인터는 추가 전 고정하고 내용·subscriber·모드는 원문의 후속 위치에서 다시 읽는다.
std::uint32_t SendChatPacket(std::uint32_t manager,std::uint32_t recipients,std::span<const std::uint8_t> body,const ChatPacketHooks& hooks) {
    const auto memory=[&](auto token) { return Need(hooks.memory)(token); }; // 토큰의 같은 실제 물리를 순수 조회한다.
    const auto global=[&](auto address) { return Need(hooks.read)(address); }; // 원문 현재 DWORD/헤더를 해당 위치에서만 읽는다.
    auto name=Need(hooks.currentName)();constexpr std::string_view nameless="++Nameless++"; // 빈 현재 이름의 원문 정적 fallback이다.
    const bool empty=Text(name).empty();bool found{};auto object=memory(recipients);
    for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(Read(object,0));++index) { // 현재 count와 실제 배열에서 자기 subscriber 중복을 탐색한다.
        if (Read(memory(Read(object,8)),std::size_t(index)*4)==global(kSubscriber)) { found=true;break; }
    }
    if (!found) Need(hooks.addRecipient)(recipients,global(kSubscriber));
    std::string battleText;bool transformed{};
    if (global(kBattle)) {
        auto flags=Read(memory(manager),16);if (Read(memory(manager),12)) flags|=0x100U;
        battleText=std::to_string(std::bit_cast<std::int32_t>(flags));while (battleText.size()<3) battleText.insert(battleText.begin(),'0'); // 원문은 세자리가 될 때까지 앞에0을 삽입한다.
        battleText+=Text(body);if (global(kBattle)) transformed=true;
    }
    const auto actualName=empty ? nameless : Text(name);const auto nameSize=actualName.size()+1;
    const auto message=transformed ? std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(battleText.data()),battleText.size()) : body;
    const auto count=Read(object,0);const auto recipientsSize=std::size_t(count)*4;
    if (nameSize>0x7fff || message.size()>0xffff || recipientsSize>memory(Read(object,8)).size()) throw std::out_of_range("Chat 패킷의 실제 이름/본문/수신자 복사 범위 부족");
    std::vector<std::uint8_t> packet(15+nameSize+message.size()+recipientsSize,0xff); // 원문 fill의 미작성+4는ff이며 뒤쪽 실제 복사까지 안전하게 소유한다.
    packet[1]=static_cast<std::uint8_t>(global(0x52edf6));Put(packet,2,std::max(global(0x544c5c)&0xffffU,4U),2);packet[0]=3;packet[5]=1;
    Put(packet,6,global(kSubscriber));Put(packet,10,static_cast<std::uint32_t>(nameSize),2);Put(packet,12,static_cast<std::uint32_t>(message.size()),2);Put(packet,14,count,1);
    std::copy(message.begin(),message.end(),packet.begin()+15+nameSize);std::copy(actualName.begin(),actualName.end(),packet.begin()+15);packet[15+actualName.size()]=0;
    const auto array=memory(Read(object,8));std::copy_n(array.begin(),recipientsSize,packet.begin()+15+nameSize+message.size());
    if (global(kChannelOne)) Need(hooks.send)(1,packet);
    else if (global(kMetamap)) Need(hooks.send)(11,packet);
    else if (global(kBattle)) Need(hooks.send)(9,packet);
    else { Need(hooks.log)(9,"ERROR: No transceiver on which to chat!\n");Need(hooks.zeroAddress)();Need(hooks.localReceive)(packet); }
    return 1;
}

// 실제 원문은 bufferLen을 zero-extend한 다음-1과 비교하므로 해당 assertion은 도달하지 않는다.
void ObserveChatPacket(std::span<std::uint8_t> packet,std::uint32_t sending,const std::function<void(std::uint32_t)>& assertion) {
    if (!sending) return;
    if (Read(packet,10,2)==0xffff) Need(assertion)(0x81d);
    if (Read(packet,14,1)==0xff) Need(assertion)(0x81f);
    const auto length=Read(packet,14,1)*4U+Read(packet,12,2)+Read(packet,10,2)+15U;Put(packet,2,length,2);
}
}
