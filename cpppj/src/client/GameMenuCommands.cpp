// 보호PE의초기명령DWORD를이름/GUI변환과구별한다.
#include "client/GameMenuCommands.h"
#include "o/OriginalText.h"
#include <array>
#include <utility>
namespace netstorm::client {
namespace {
// 00542xxx는원문주소식별자다. 런타임호스트포인터로역참조하지않는다.
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,29> kCommands{{
    {0x00542780U,0x02000022U},
    {0x005427b8U,0x02000030U},
    {0x005428acU,0x0200006dU},
    {0x005428dcU,0x02000079U},
    {0x00542914U,0x02000088U},
    {0x00542924U,0x0200008cU},
    {0x00542928U,0x0200008dU},
    {0x00542938U,0x02000091U},
    {0x00542944U,0x02000094U},
    {0x00542948U,0x02000095U},
    {0x00542974U,0x020000a0U},
    {0x005429acU,0x020000aeU},
    {0x005429d4U,0x020000b8U},
    {0x005429fcU,0x020000c2U},
    {0x00542a08U,0x020000c5U},
    {0x00542a0cU,0x020000c6U},
    {0x00542a20U,0x020000cbU}, //SaveGoMain의 두 저장 선택은 같은 실제 Native State3 예약 명령을 사용한다.
    {0x00542a30U,0x020000cfU},
    {0x00542a28U,0x020000cdU}, //GetFullDeck의 원본 입력 ID다.
    {0x00542a4cU,0x020000d6U},
    {0x00542a60U,0x020000dbU},
    {0x00542a8cU,0x020000e6U},
    {0x00542a90U,0x020000e7U},
    {0x00542aa0U,0x020000ebU}, //Tips의 이전·다음·확인은 같은 TellTip 명령의 두 번째 인자를 쓴다.
    {0x00542ab4U,0x020000f0U},
    {0x00542ab8U,0x020000f1U},
    {0x00542700U,0x02000001U},
    {0x005429ecU,0x020000beU},
    {0x00542b54U,0x02000123U},
}};
}
// 등록한원문전역과동일명령ID의첫쌍만반환한다.
std::optional<std::uint32_t> GameMenuCommandValue(std::uint32_t address) {
    for (const auto& [source,value]:kCommands) if (source==address) return value; // 주소와명령숫자를뒤바꾸지않는다.
    return {};
}
// 원본name→ID표에서확인한메뉴확인창의필수이름이다.
std::uint32_t GameMenuScriptCode(std::string_view name) {
    const auto key=o::AsciiLower(name); // 이름비교만원문ASCII대소문자를무시한다.
    if (key=="quitapp") return 0x02000001U;
    if (key=="requestquitapp") return 0x02000094U;
    if (key=="menujustclosed") return 0x020000beU;
    if (key=="reloadtheams") return 0x02000123U;
    if (key=="pause") return 0x020000cfU;
    if (key=="getfulldeck") return 0x020000cdU;
    if (key=="savegomain") return 0x020000cbU;
    if (key=="telltip") return 0x020000ebU;
    if (key=="newfort") return 0x02000079U; // LOADLOADFORT가 예약하는 새 이름 창의 원본 명령이다. y=0/1은 실제 큐에 그대로 보존한다.
    return 0xffffffffU;
}
// 실제큐연결허용과선택후미연결명령진단에같은표를쓴다.
bool IsGameMenuCommand(std::uint32_t code) {
    for (const auto& [address,value]:kCommands) if (value==code) return true; // 같은값의주소항목이여럿이어도허용결과는같다.
    return code==0x02000049U; // About의TechnicalHelp는별도부모기본전역0054281c다.
}
}
