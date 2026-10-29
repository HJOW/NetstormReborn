// 수신자 변경의 현재값 재조회와 원문 고정 패턴의 두 인접 캡처를 보존한다.
#include "client/RawChatSubmit.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 실제 manager 토큰과 현재 기록을 공유하는 제출 경계다.
namespace { // 원문 DWORD/바이트 문자열/고정 채널 목록만 처리한다.
constexpr std::string_view kSpace="\r\n\t "; // 대상과 본문 패턴에 명시된 네 공백이며 VT/FF는 제외한다.
constexpr std::array<std::string_view,8> kChannels{"","","","BATTLE","","EVERYONE","ALLIES","OBSERVER"}; //00531768의 첫NULL 앞 여덟 실제 키다.
// 실제로 도달한 장치만 요구하며 미선택 송신/표시를 가짜 성공으로 대체하지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Chat 제출의 실제 선택 장치 미연결");return function; }
// DWORD의 실제 물리 범위를 확인하고 원문 little endian 값을 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::uint32_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Chat 제출 DWORD의 실제 물리 범위");std::uint32_t value{};
    for (std::uint32_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8); // 인접 네 바이트만 읽는다.
    return value;
}
// 원문 DWORD store와 같은 네 바이트만 쓰며 인접 꼬리를 지우지 않는다.
void Write(std::span<std::uint8_t> bytes,std::uint32_t at,std::uint32_t value) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Chat 제출 store의 실제 물리 범위");
    for (std::uint32_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 하위 BYTE부터 같은 DWORD를 쓴다.
}
// 현재 manager/배열을 getter의 주소 사본 없이 조회한다.
std::span<std::uint8_t> Memory(const ChatSubmitHooks& hooks,std::uint32_t token) { return Need(hooks.memory)(token); }
// 원문 문자열은 첫NUL에서 끝나며 view 뒤의 호스트 메모리를 읽지 않는다.
std::string_view Z(std::string_view text) { return text.substr(0,text.find('\0')); }
// 같은 실제 span의 NUL 앞만 빌리며 인접 캡처까지 이어지는 문자열도 유지한다.
std::string_view Text(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("Chat 제출 문자열의 실제 NUL 없음");
    return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// Regexp capture의 최대2048BYTE 복사와 그 다음 NUL을 같은 인접 작업 공간에 쓴다.
void Capture(std::span<std::uint8_t> work,std::size_t at,std::string_view text) {
    const auto count=std::min<std::size_t>(text.size(),2048);if (at>work.size() || count+1>work.size()-at) throw std::out_of_range("Chat 캡처의 실제 물리 범위");
    std::copy_n(text.begin(),count,work.begin()+at);work[at+count]=0;
}
// C locale의 좁은 문자열 대소문자 동등 비교만 수행한다.
bool Equal(std::string_view first,std::string_view second) {
    first=Z(first);second=Z(second);if (first.size()!=second.size()) return false;
    const auto lower=[](unsigned char value) { return value>='A' && value<='Z' ? value+('a'-'A') : value; }; // 비ASCII는 변환하지 않는다.
    for (std::size_t index=0;index<first.size();++index) if (lower(static_cast<unsigned char>(first[index]))!=lower(static_cast<unsigned char>(second[index]))) return false; // 첫 다른 바이트에서 실패한다.
    return true;
}
// 같은 지역의 이름은 번역 장치 호출 뒤에도 다시 읽는다.
std::int32_t Channel(o::PlayerIdentityRecord& identity,const ChatSubmitHooks& hooks) {
    for (std::size_t index=0;index<kChannels.size();++index) { // 빈 키를 생략하지 않고 원문 순서로 조회한다.
        const auto value=Need(hooks.translate)(kChannels[index]);if (Equal(Text(std::span(identity).subspan(8)),value)) return static_cast<std::int32_t>(index);
    }
    return -1;
}
}

// 확장 하위가 같은 count/capacity를 바꾸더라도 원문 위치에서 현재 값을 다시 읽는다.
void AddChatRecipient(std::uint32_t manager,std::uint32_t subscriber,const ChatSubmitHooks& hooks) {
    auto object=Memory(hooks,manager); // 객체 자체는 호출 동안 살아 있으며 메모리 getter는 순수하다.
    for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(Read(object,0));++index) { // 현재 count의 실제 DWORD를 순서대로 비교해 중복을 보존하지 않는다.
        if (Read(Memory(hooks,Read(object,8)),index*4U)==subscriber) return;
    }
    if (std::bit_cast<std::int32_t>(Read(object,4))<=std::bit_cast<std::int32_t>(Read(object,0))) {
        const auto buffer=Need(hooks.allocate)(Read(object,4)*4U+128U);
        if (!buffer) Need(hooks.assertion)(0x112);
        for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(Read(object,4));++index) { // 원문은 count가 아니라 capacity 전체를 복사한다.
            Write(Memory(hooks,buffer),index*4U,Read(Memory(hooks,Read(object,8)),index*4U));
        }
        Need(hooks.release)(Read(object,8));Write(object,8,buffer);Write(object,4,Read(object,4)+32U);
    }
    Write(Memory(hooks,Read(object,8)),Read(object,0)*4U,subscriber);Write(object,0,Read(object,0)+1U);
}

// 고정 ^[공백]*([^:]*)($|:[공백]*(.*))를 현재 인접 지역에 실행한 뒤 수신자를 갱신한다.
void SubmitChatInput(std::uint32_t manager,std::string_view input,o::PlayerIdentityRecord& identity,
    std::span<std::uint8_t> work,const ChatSubmitHooks& hooks) {
    auto object=Memory(hooks,manager);Write(object,0,0);Write(object,12,0);
    if (work.size()<kChatSubmitWorkBytes) throw std::out_of_range("Chat 제출의 두 인접 캡처 물리 부족");work=work.first(kChatSubmitWorkBytes);
    input=Z(input);auto source=input;const auto first=source.find_first_not_of(kSpace);source=first==source.npos ? std::string_view{} : source.substr(first);
    const auto colon=source.find(':');std::uint32_t mask{};work[0]=0; // 실제 첫 capture3 실패도 본문 첫BYTE에NUL을 쓴다.
    if (colon!=source.npos) {
        auto body=source.substr(colon+1);const auto begin=body.find_first_not_of(kSpace);body=begin==body.npos ? std::string_view{} : body.substr(begin);
        Capture(work,0,body);Capture(work,2048,source.substr(0,colon));o::RawPlayerIdentity::Initialize(identity,0);Write(object,12,1);
        std::size_t cursor{}; // 반복 토큰 Regexp의 현재 접두 버퍼 상대 위치다.
        for (;;) { // 전체 캡처 끝으로 이동하며40BYTE 이름 절단과 구별한다.
            const auto prefix=Text(work.subspan(2048));if (cursor>prefix.size()) throw std::out_of_range("Chat 접두 토큰의 실제 다음 위치");const auto remaining=prefix.substr(cursor);
            const auto start=remaining.find_first_not_of(kSpace);if (start==remaining.npos) break;const auto end=remaining.find_first_of(kSpace,start);const auto length=(end==remaining.npos ? remaining.size() : end)-start;
            Write(identity,0,0);Write(identity,4,0);identity[8]=0;Write(identity,48,0);
            const auto copied=std::min<std::size_t>(length,40);std::copy_n(remaining.begin()+start,copied,identity.begin()+8);identity[8+copied]=0;
            if (Need(hooks.resolve)(identity)) AddChatRecipient(manager,Read(identity,4),hooks);
            else {
                const auto channel=Channel(identity,hooks);Write(object,12,0);
                if (channel<0) {
                    Write(object,0,0);mask=0x20;const auto bytes=std::min<std::size_t>(input.size(),2048);
                    std::copy_n(input.begin(),bytes,work.begin());std::fill(work.begin()+bytes,work.begin()+2048,std::uint8_t{});work[2047]=0;break;
                }
                mask|=1U<<(static_cast<std::uint32_t>(channel)&31U);
            }
            cursor+=start+length;
        }
    } else { Capture(work,0,source);mask=0x20; }
    Write(object,16,mask);
    if (mask) {
        for (std::uint32_t index=0;index<100;++index) { // 같은100행을 고정 순서로 읽으며 선택된 추가 뒤 다음 행은 현재 기록을 재조회한다.
            const auto records=Need(hooks.records)();if (records.size()<4800) throw std::out_of_range("Chat 현재100행의 실제 물리 부족");const auto row=records.subspan(index*48U,48);
            const auto subscriber=Read(row,44);const auto flags=static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(row[40])));
            if (subscriber && (mask&flags)) AddChatRecipient(manager,subscriber,hooks);
        }
    }
    const auto body=Text(work);if (body.empty()) Need(hooks.inputVisible)(0);
    else Need(hooks.dispatch)(work.first(body.size()+1));
}
}
