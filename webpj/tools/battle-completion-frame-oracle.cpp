// cpppj RawBattleCompletion::Frame의 합성 입력별 결과·Player와 효과 순서를 출력한다.
// 사용: C++20으로 이 파일과 cpppj/src/o/RawBattleCompletion.cpp를 함께 빌드해 표준 출력을 저장한다.
#include "o/RawBattleCompletion.h"
#include <array>
#include <cstdint>
#include <iostream>

// 관측 입력의 DWORD 난수를 한 번 진행하고 상위 16비트를 돌려준다.
static std::uint32_t Next(std::uint32_t& state) { state=state*1664525U+1013904223U;return state>>16; }

// 각 독립 상태의 실제 C++ 프레임 처리와 결과를 기록한다.
int main() {
    // 연결·관문·결과 조합을 4096개 생성한다.
    for (std::uint32_t sample=0;sample<4096;++sample) {
        std::uint32_t random=sample+0x14b37U,trace=0;
        std::array<std::uint8_t,9*180> players{};std::array<std::uint8_t,87> options{};std::array<std::uint32_t,81> alliances{};std::array<bool,9> dead{};
        // 각 행의 subscriber·flags·관전자·사제를 독립적으로 선택한다.
        for (std::size_t owner=1;owner<9;++owner) {
            players[owner*180+0x2c]=(Next(random)%4U)!=0 ? static_cast<std::uint8_t>(owner):0;
            const auto logged=Next(random)%2U,flagged=Next(random)%2U;
            players[owner*180+0x50]=static_cast<std::uint8_t>((logged ? 1U:0U)|(flagged ? 0x80U:0U));
            options[0x3e+owner]=static_cast<std::uint8_t>((Next(random)%5U)==0);dead[owner]=(Next(random)%2U)!=0;
        }
        // 각 방향의 동맹과 선택된 진단 불일치를 설정한다.
        for (auto& alliance:alliances) alliance=(Next(random)%4U)==0 ? 1U:0U;
        if (sample%17U==0) players[180+0x7c]=1;
        const bool battle=sample%10U!=0,mission=sample%11U==0,boss=(sample&1U)!=0,connected=sample%9U!=0,server=sample%3U==0;
        const std::uint32_t scriptBreak=sample%13U==0 ? 1U:0U,phase=sample%8U==0 ? 4U:14U,owner=sample%8U+1U;const double login=sample%7U==0 ? 1.0:0.0,seconds=10.0;
        double capture=sample%5U==0 ? 8.0:0.0,draw=sample%7U==0 ? 5.0:0.0;std::uint8_t settle=static_cast<std::uint8_t>(sample%4U==0 ? 255U:sample%4U==1 ? 4U:5U);std::uint32_t result=99;
        netstorm::o::BattleCompletionHooks playerHooks;
        playerHooks.serverConnection=[&] { trace=trace*33U+10U;return connected; };
        playerHooks.priestDead=[&](auto current) { trace=trace*33U+current;return dead.at(current); };
        netstorm::o::BattleCompletionFrameHooks frameHooks;
        frameHooks.wallSeconds=[&] { trace=trace*33U+30U;return 10.0; };
        frameHooks.assertion=[&](auto,auto,auto line) { trace=trace*33U+line; };
        frameHooks.currentDialog=[&] { trace=trace*33U+31U;return sample%7U==1 ? 1U:0U; };
        frameHooks.deleteSavedState=[&] { trace=trace*33U+32U; };
        frameHooks.rejectDraw=[&] { trace=trace*33U+33U; };
        frameHooks.queueMainMenu=[&] { trace=trace*33U+34U; };
        netstorm::o::RawBattleCompletion gate({players,options,alliances,battle,mission,boss,scriptBreak,login},playerHooks);
        gate.Frame({players,server,phase,owner,capture,draw,settle,result,seconds},frameHooks);
        std::cout<<sample<<' '<<result<<' '<<unsigned(settle)<<' '<<capture<<' '<<draw<<' '<<trace;
        // 프레임이 바꾼 각 Player flags의 현재 BYTE를 기록한다.
        for (std::size_t current=1;current<9;++current) std::cout<<' '<<unsigned(players[current*180+0x50]);
        std::cout<<'\n';
    }
}
