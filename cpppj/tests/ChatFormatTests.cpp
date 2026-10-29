// 원문 서식 명령372행의 전체 manager/공통 캡처·현재 전역·부분효과를 독립 기대값과 대조한다.
#include "TestSupport.h"
#include "client/RawChatFormat.h"
#include "o/RawChatManagerPrefix.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 입력 복원/원시 직렬화만 수행하며 제품 결과로 기대값을 계산하지 않는다.
using namespace netstorm::client;
constexpr std::uint32_t kFirst=0x12001000,kSecond=0x12004000; // 원문 관측의 두 실제 manager 토큰이다.
struct FormatStop { std::string reason; }; // 선택한 장치에서 앞선 부분 쓰기를 남기는 명시 중단 입력이다.
// 빈 마지막 열과 NUL 원시 문자열을 구별하는 TSV 분리다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 모든 열을 원문 순서대로 보존한다.
}
// 바이트 문자열을 소문자 hex로 기록해 제어문자/미작성 꼬리까지 비교한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 실제 관측과 같은 출력 문자표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // 전체 물리 바이트를 빠짐없이 직렬화한다.
    return result;
}
// 키/설정의 좁은 바이트도 같은 직렬화기에 전달한다.
std::string Hex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 원문 입력 hex를 UTF8 변환 없이 좁은 바이트 문자열로 되돌린다.
std::string Input(std::string_view hex) {
    std::string result;if (hex=="-") return result;
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)); // 두 hex문자를 한 BYTE로 복원한다.
    return result;
}
// 현재 실제 토큰을 관측 자료의0/1/2 별칭으로 표현한다.
std::uint32_t Alias(std::uint32_t token) { if (!token) return 0;if (token==kFirst) return 1;if (token==kSecond) return 2;throw std::logic_error("서식 검사 manager 토큰 오류"); }
}

// 실제 정규식·캡처·제거·두 setter의 원문 출력과 선택 중단의 전체 부분효과를 대조한다.
TEST_CASE(ChatFormat_IndependentNative372WholeMemoryAndPartialEffects) {
    std::ifstream file(NETSTORM_CHAT_FORMAT_FIXTURE);CHECK(file.good());std::string line;std::getline(file,line);std::size_t rows{};
    while (std::getline(file,line)) { // 보호 PE에서 제품보다 먼저 동결한 입력/출력 한 행씩 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto fields=Fields(line);CHECK(fields.size()==10);if (fields.size()!=10) continue;
        const bool front=fields[0]=="1";const auto source=Input(fields[1]);const auto& mode=fields[2];std::array<std::uint8_t,2712> first,second;
        for (std::size_t index=0;index<first.size();++index) { first[index]=static_cast<std::uint8_t>((0xa5+index*17)&255);second[index]=static_cast<std::uint8_t>((0xb0+index*17)&255); } // 같은 두 비영 원문 입력만 구성한다.
        std::array<std::uint8_t,kChatFormatCaptureBytes> capture;capture.fill(0xd3);std::uint32_t current=mode.starts_with("assert") ? 0U : kFirst,subscriber=100;std::vector<std::string> trace;ChatFormatHooks hooks;
        hooks.memory=[&](auto token) -> std::span<std::uint8_t> { if (token==kFirst) return first;if (token==kSecond) return second;throw std::out_of_range("서식 검사 실제 manager 없음"); };
        hooks.assertion=[&](auto number) { trace.push_back("assert:"+std::to_string(number));if (mode=="assertStop") throw FormatStop{"assertion"};CHECK(mode=="assertRestore");current=kFirst; };
        hooks.stripSlots=[&](auto text) { trace.push_back("strip:"+Hex(text));StripChatFormatSlots(text); };
        hooks.copyFormat=[&](auto token,auto offset,auto text,auto count) {
            trace.push_back("copy:"+std::to_string(Alias(token))+':'+std::to_string(offset)+':'+std::to_string(count));
            netstorm::o::CopyChatManagerFormat(hooks.memory(token).subspan(offset),text,count);
            if (mode=="copySwitch") { current=kSecond;subscriber=0xffffffffU;const std::string_view next="~gNEXT";std::copy(next.begin(),next.end(),capture.begin());capture[next.size()]=0; }
            if (mode=="stopCopy") throw FormatStop{"copy"};
        };
        hooks.writeConfig=[&](auto key,auto value) { trace.push_back("key:"+Hex(key));trace.push_back("config:"+Hex(key)+':'+Hex(value));if (mode=="stopConfig") throw FormatStop{"config"}; };
        hooks.illegalFormat=[&](auto text) { trace.push_back("error:"+Hex(text)); };
        std::string status="normal";try { CHECK(ApplyChatFormat(front,{current,subscriber},source,capture,hooks)==0); }catch (const FormatStop& stop) { status=stop.reason; }
        std::string events;for (const auto& event:trace) { if (!events.empty()) events+=';';events+=event; } // 실제 장치의 호출 순서를 그대로 직렬화한다.
        CHECK(status==fields[3] && Alias(current)==std::stoul(fields[4]) && subscriber==std::stoul(fields[5]));
        CHECK(Hex(first)==fields[6] && Hex(second)==fields[7] && Hex(capture)==fields[8] && events==fields[9]);++rows;
    }
    CHECK(rows==372);
}
