// 원문 순서·빈 후보·중복·BYTE 잔류와 signed 인덱스 감김을 유지한다.
#include "client/ChatNameCompletion.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>
namespace netstorm::client { //물리 상태 밖의 손상은 성공값으로 보정하지 않고 명시 경계로 남긴다.
namespace { //원본 주소 배치와 C locale ASCII 비교만 공유한다.
constexpr std::size_t kIndex=0,kLength=4,kPrefix=8,kCount=0x88,kCandidates=0x90,kSlot=128; //00548810 기준의 DWORD/배열 오프셋이다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { //비정렬 원문 DWORD를 그대로 읽는다.
    if (offset+4>bytes.size()) throw std::out_of_range("자동완성 DWORD 읽기 범위");std::uint32_t value{};std::memcpy(&value,bytes.data()+offset,4);return value;
}
void Write(ChatNameCompletionState& state,std::size_t offset,std::uint32_t value) { std::memcpy(state.bytes.data()+offset,&value,4); } //원문 고정 DWORD 위치만 쓴다.
std::string_view String(std::span<const std::uint8_t> bytes,std::size_t offset) { //슬롯128을 넘어 이어진 원문 C문자열도 같은 물리에서 읽는다.
    if (offset>=bytes.size()) throw std::out_of_range("자동완성 문자열 주소 범위");const auto begin=bytes.begin()+static_cast<std::ptrdiff_t>(offset),end=std::find(begin,bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("자동완성 문자열 NUL 범위");return {reinterpret_cast<const char*>(bytes.data()+offset),static_cast<std::size_t>(end-begin)};
}
void Copy(ChatNameCompletionState& state,std::size_t offset,std::string_view text) { //strcpy의 종료BYTE까지 복사하며 번역을 슬롯 폭으로 잘라내지 않는다.
    text=text.substr(0,text.find('\0'));if (offset>=state.bytes.size() || text.size()>=state.bytes.size()-offset) throw std::out_of_range("자동완성 후보 복사 물리 범위");std::memcpy(state.bytes.data()+offset,text.data(),text.size());state.bytes[offset+text.size()]=0;
}
std::uint8_t Lower(std::uint8_t value) { return value>='A' && value<='Z' ? std::uint8_t(value+'a'-'A') : value; } //원문 C locale의 ASCII 대소문자 변환이다.
bool Equal(const ChatNameCompletionState& state,std::size_t candidate,std::uint32_t length) { //strnicmp처럼 첫NUL 또는 명시 길이에서 멈춘다.
    for (std::uint32_t index=0;index<length;++index) { //원문 인접 상태를 읽으며 NUL 이후에는 접근하지 않는다.
        if (candidate+index>=state.bytes.size() || kPrefix+index>=state.bytes.size()) throw std::out_of_range("자동완성 비교 물리 범위");const auto a=Lower(state.bytes[candidate+index]),b=Lower(state.bytes[kPrefix+index]);if (a!=b) return false;if (!a) return true;
    }
    return true;
}
}
//새 query에서만 후보를 수집하며 NULL 순환 중에는 변경된 현재 기록/번역을 다시 읽지 않는다.
std::optional<std::string> CompleteChatName(ChatNameCompletionState& state,std::optional<std::string_view> query,std::int32_t length,bool backward,const ChatNameCompletionHooks& hooks) {
    if (!query) Write(state,kIndex,Read(state.bytes,kIndex)+(backward ? 0xffffffffU : 1U));
    else {
        std::uint32_t count{}; //현재 수집 수이며 이전 후보의 뒤쪽 BYTE는 지우지 않는다.
        constexpr std::array<std::string_view,8> channels{"","","","BATTLE","","EVERYONE","ALLIES","OBSERVER"}; //원문00531768의NULL 전8개 원시 문자열이다.
        for (const auto channel:channels) { //번역 전의 빈값/길이를 검사하고 번역 후 문자열을 그대로 복사한다.
            if (count>=128) break;if (channel.empty() || channel.size()>=128) continue;if (!hooks.translate) throw std::logic_error("자동완성 실제 번역 필요");Copy(state,kCandidates+count*kSlot,hooks.translate(channel));++count;
        }
        if (!hooks.records) throw std::logic_error("자동완성 현재 Chat 기록 필요");const auto records=hooks.records();if (records.size()!=4800) throw std::out_of_range("자동완성 실제100행 필요");
        for (std::size_t index=0;index<100;++index) { //비영+2c Subscriber의 빈 이름도 원문처럼 포함한다.
            if (count>=128) break;if (!Read(records,index*48+44)) continue;const auto name=String(records,index*48);if (name.size()<128) { Copy(state,kCandidates+count*kSlot,name);++count; }
        }
        Write(state,kCount,count);if (length>126) { if (!hooks.assertion) throw std::logic_error("자동완성 원문 길이 진단 필요");hooks.assertion(0x354); }
        const auto width=std::bit_cast<std::uint32_t>(length);if (width>state.bytes.size()-kPrefix) throw std::out_of_range("자동완성 strncpy 물리 범위");const auto text=query->substr(0,query->find('\0'));
        for (std::uint32_t index=0;index<width;++index) state.bytes[kPrefix+index]=index<text.size() ? static_cast<std::uint8_t>(text[index]) : 0; //strncpy의NUL패딩과미작성접두꼬리를구별한다.
        Write(state,kLength,width);Write(state,kIndex,0);
    }
    bool wrapped=false; //원문은 범위를 두 번째 벗어났을 때만NULL을 반환한다.
    for (;;) { //정/역방향의 첫 검색과NULL후보 순환은 같은 signed 인덱스를 사용한다.
        const auto index=std::bit_cast<std::int32_t>(Read(state.bytes,kIndex)),count=std::bit_cast<std::int32_t>(Read(state.bytes,kCount));
        if (index<0 || index>=count) { if (wrapped) return std::nullopt;wrapped=true;Write(state,kIndex,static_cast<std::uint32_t>(count));if (!backward) { Write(state,kIndex,0);continue; } }
        else if (Equal(state,kCandidates+std::size_t(index)*kSlot,Read(state.bytes,kLength))) return std::string(String(state.bytes,kCandidates+std::size_t(index)*kSlot));
        Write(state,kIndex,Read(state.bytes,kIndex)+(backward ? 0xffffffffU : 1U));
    }
}
}
