// 보호 PE의 초기 등록/실제 토큰·CRT 결과와 같은 제품 콜백의 효과 순서를 대조한다.
#include "TestSupport.h"
#include "client/RawIssueCommands.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

namespace { // 원문 hex 입출력과 효과 관측만 제공한다.
// 좁은 원문 문자열의 바이트를 locale 변환 없이 고정 hex로 만든다.
std::string IssueHex(std::string_view value) {
    constexpr char digits[]="0123456789abcdef"; //원시 fixture의 바이트 순서다.
    std::string result;for (const auto byte:value) { result+=digits[static_cast<unsigned char>(byte)>>4];result+=digits[static_cast<unsigned char>(byte)&15]; } //각 바이트를 두 글자로 보존한다.
    return result;
}
// 고정 입력만 복호화하며 결과 숫자/치트 상태를 계산하지 않는다.
std::string IssueBytes(std::string_view hex) {
    std::string result;if (hex=="-" || hex=="~") return result;
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)); //두 글자가 한 원문 BYTE다.
    return result;
}
}
// 원문122행의 NULL/부호/32비트 감김·인용·78BYTE 토큰과 중복 등록을 현재 본체에 대조한다.
TEST_CASE(IssueCommands_NativeRegistrationFlashAndCheat) {
    using namespace netstorm::client;std::ifstream file(NETSTORM_ISSUE_COMMANDS_FIXTURE);CHECK(file.good());std::string line;std::size_t rows{};
    while (std::getline(file,line)) { // 기대값은 실제 원문 CPU 출력에서만 읽는다.
        if (line.empty() || line.front()=='#') continue;
        std::istringstream input(line);std::string operation,hex,expected;std::uint32_t before{},returned{},after{};input>>operation>>hex>>before>>returned>>after>>expected;CHECK(!input.fail());++rows;
        const auto bytes=IssueBytes(hex);std::string events;auto cheat=before;
        const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; //실제 선택된 효과만 순서대로 기록한다.
        if (operation=="Init") RegisterIssueCommands([&](auto name,auto callback,auto flag) { event("R,"+IssueHex(name)+","+std::to_string(callback)+","+std::to_string(flag)); });
        else {
            IssueCommandHooks hooks;hooks.flash=[&](auto value) { event("F,"+std::to_string(std::bit_cast<std::uint32_t>(value))); };
            hooks.sound=[&](auto name) { event("S,"+IssueHex(name)+",0,0,0,0,0"); };
            const auto result=InvokeIssueCommand(operation=="Flash" ? 0x4d5300U:0x4d5360U,hex=="-" ? std::nullopt:std::optional<std::string_view>{bytes},cheat,hooks);
            CHECK(result && *result==returned);
        }
        CHECK(cheat==after);CHECK((events.empty() ? "-":events)==expected);
    }
    CHECK(rows==122);
}
// 알 수 없는 콜백과 선택된 장치 부재를 빈 성공으로 바꾸지 않는다. MP desc는 실행하지 않는다.
TEST_CASE(IssueCommands_SelectedDevicesAndUnknownCallback) {
    using namespace netstorm::client;std::uint32_t cheat=17;CHECK(!InvokeIssueCommand(0,std::nullopt,cheat,{}));CHECK(cheat==17);
    bool failed{};try { static_cast<void>(InvokeIssueCommand(0x4d5300U,"7",cheat,{})); }catch (const std::logic_error&) { failed=true; }CHECK(failed);
    unsigned flashes{};IssueCommandHooks hooks;hooks.flash=[&](auto value) { CHECK(value==7);++flashes; };failed=false;
    try { static_cast<void>(InvokeIssueCommand(0x4d5300U,"7",cheat,hooks)); }catch (const std::logic_error&) { failed=true; }CHECK(failed && flashes==1 && cheat==17);
}
