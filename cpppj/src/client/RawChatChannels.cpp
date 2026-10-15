// 실제 Chatmanager 기록 선택과 하위 이름 정제를 복원한다. 게임/GUI/장치 수명은 소유하지 않는다.
#include "client/RawChatChannels.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::size_t kNameBytes=40,kFlags=0x28,kSubscriber=0x2c; // 실제 이름 물리 폭·BYTE 플래그·DWORD 식별자 위치다.
constexpr std::string_view kForbidden="\xa0%.?=()[]~\"`;: "; // 0042b0e0이 첫 두 BYTE를 고친 뒤 실제 strchr에 전달하는 표다.
}
// 현재 원문과 현재 subscriber 조회기를 최초 쓰기 전에 확인한다. CD의 비슷한 몸체를 추정하지 않는다.
RawChatChannels::RawChatChannels(o::OriginalEdition edition,std::span<std::uint8_t> records,
    std::function<std::uint32_t()> mySubscriber,std::function<void(ChatChannelCall,std::uint32_t)> observe)
    :records_(records),mySubscriber_(std::move(mySubscriber)),observe_(std::move(observe)) {
    if (edition!=o::OriginalEdition::Patch1078 || records.size()!=kBytes || !mySubscriber_)
        throw std::invalid_argument("Chat 채널의 실제 PATCH100기록/현재 subscriber 조회 누락");
}
// 높은 식별자 비트와 빈 기록0을 구분하고 정렬되지 않은 기록을 직접 읽는다.
std::uint32_t RawChatChannels::Subscriber(std::size_t index) const {
    const auto record=records_.subspan(index*kStride,kStride);std::uint32_t result{};
    // 원본 DWORD의 낮은 BYTE부터 네 바이트를 조립한다.
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(record[kSubscriber+byte])<<(byte*8);
    return result;
}
// 물리 이름40바이트를 모두 쓰되 원본처럼 입력 첫 NUL 뒤에는 입력을 더 읽지 않는다.
void RawChatChannels::CopyName(std::span<std::uint8_t> record,std::string_view name) const {
    bool ended=false;
    // strncpy의0보충과 정확히40글자인 입력을 같은 루프에서 보존한다.
    for (std::size_t index=0;index<kNameBytes;++index) {
        if (index>=name.size() || (!ended && name[index]=='\0')) ended=true;
        record[index]=ended ? std::uint8_t{} : static_cast<std::uint8_t>(name[index]);
    }
}
// 실제 이름 정제의 memmove는 NUL까지 옮기고 그 뒤의 stale BYTE를 지우지 않는다.
void RawChatChannels::NameChanged(std::uint32_t subscriber,std::span<std::uint8_t> record) const {
    if (observe_) observe_(ChatChannelCall::NameChanged,subscriber);
    std::size_t length=0;
    // Add가+27에NUL을 쓴 뒤의 실제 문자열만 읽는다.
    while (length<kNameBytes && record[length]) ++length;
    std::size_t index=0;
    // 금지 문자를 지우면 같은 index를 다시 검사한다. 입력의 선후행 공백도 원본처럼 전부 지운다.
    while (index<length) {
        if (kForbidden.find(static_cast<char>(record[index]))==std::string_view::npos) { ++index;continue; }
        // 현재 NUL까지 왼쪽으로 한 칸 옮기며 문자열 밖 stale 값은 보존한다.
        for (std::size_t next=index;next<length;++next) record[next]=record[next+1];
        --length;
    }
    record[length]=0;
    if (length==0) {
        const auto replacement=std::string("Cheat")+std::to_string(subscriber&0xffU); // 원본 format 직전 and eax,ff의 unsigned 인자다.
        std::copy(replacement.begin(),replacement.end(),record.begin());record[replacement.size()]=0;
    }
}
// 실제004437f0은 단일ret이다. 선택 관찰을 제외하면 추가 UI 갱신이나 이벤트를 만들어내지 않는다.
void RawChatChannels::Refresh() const { if (observe_) observe_(ChatChannelCall::Refresh,0); }
// 현재 자기 식별자·첫 빈칸·기존 식별자와 특수-1 분기를 원본 검색 순서로 처리한다.
std::int32_t RawChatChannels::Add(std::uint32_t subscriber,std::optional<std::string_view> name,std::uint32_t flags) const {
    if (subscriber==mySubscriber_()) return -1;
    std::int32_t empty=-1,existing=-1;auto incoming=static_cast<std::uint8_t>(flags);
    // -1은 기존-1을 갱신하지 않고 첫0기록을 예약하며 bit10을 항상 제거한다.
    if (subscriber==0xffffffffU) {
        incoming&=0xefU;
        // 첫 빈 subscriber만 선택한다. 모두 차 있으면 기존-1이 있어도 실패한다.
        for (std::size_t index=0;index<kCount;++index) if (Subscriber(index)==0) { empty=static_cast<std::int32_t>(index);break; }
    } else {
        // 빈칸을 만난 뒤에도 같은 subscriber를 끝까지 찾으며 첫 기존 기록에서 중단한다.
        for (std::size_t index=0;index<kCount;++index) {
            const auto current=Subscriber(index);
            if (current==subscriber) { existing=static_cast<std::int32_t>(index);break; }
            if (current==0 && empty==-1) empty=static_cast<std::int32_t>(index);
        }
    }
    const auto chosen=existing!=-1 ? existing : empty;if (chosen==-1) return -1;
    auto record=records_.subspan(static_cast<std::size_t>(chosen)*kStride,kStride);
    if (existing==-1) {
        // 새 기록에서만 실제 subscriber DWORD를 저장한다. 주변 stale 값은 초기화하지 않는다.
        for (std::size_t byte=0;byte<4;++byte) record[kSubscriber+byte]=static_cast<std::uint8_t>(subscriber>>(byte*8));
        if (name) CopyName(record,*name);
        else {
            std::array<char,16> unknown{}; // Unknown7글자+DWORD hex8글자+NUL의 원본 sprintf 결과다.
            std::snprintf(unknown.data(),unknown.size(),"Unknown%08x",static_cast<unsigned int>(subscriber));
            std::copy(unknown.begin(),unknown.end(),record.begin());
        }
    } else {
        record[0]=0;if (name) CopyName(record,*name);incoming|=record[kFlags];
    }
    record[0x27]=0;record[kFlags]=incoming;NameChanged(subscriber,record);Refresh();Refresh();return chosen;
}
// Player의 실제8인자 호출에서 읽히는1/2/6인자를 같은 BYTE 필드에 적용한다.
std::int32_t RawChatChannels::AddToChannel(std::uint32_t subscriber,std::string_view name,std::uint32_t channel) const { return Add(subscriber,name,channel); }
// 원본5기록 묶음의 순서와 같은 index순서이며0식별자 기록의 stale flags는 유지한다.
void RawChatChannels::Clear(std::uint32_t mask) const {
    const auto keep=static_cast<std::uint8_t>(~static_cast<std::uint8_t>(mask)); // 원본 not dl과 같은 하위BYTE 마스크다.
    // 모든100기록을 방문하고 실제 subscriber가비제로일 때만 한BYTE를 쓴다.
    for (std::size_t index=0;index<kCount;++index) if (Subscriber(index)!=0) records_[index*kStride+kFlags]&=keep;
}
}
