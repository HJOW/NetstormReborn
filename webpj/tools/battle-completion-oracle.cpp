// cpppj RawBattleCompletion의 결정적 합성 입력별 반환과 조회 순서를 출력한다.
// 사용: C++20으로 이 파일과 cpppj/src/o/RawBattleCompletion.cpp를 함께 빌드한 뒤 표준 출력을 UTF-8 파일로 저장한다.
#include "o/RawBattleCompletion.h"
#include <array>
#include <cstdint>
#include <iostream>

// 정해진 seed의 상위 16비트를 반환하며 JS 검사와 같은 DWORD 감김을 사용한다.
static std::uint32_t Next(std::uint32_t& state) { state=state*1664525U+1013904223U;return state>>16; }

// 실제 C++ 판정기의 반환과 장치 조회 순서를 각 입력마다 출력한다.
int main() {
    // 독립 상태 4096개를 같은 순서로 생성한다.
    for (std::uint32_t sample=0;sample<4096;++sample) {
        std::uint32_t random=sample+0x14b37U;
        std::array<std::uint8_t,9*180> players{};std::array<std::uint8_t,87> options{};std::array<std::uint32_t,81> alliances{};std::array<bool,9> dead{};
        // 각 Player의 현재 subscriber, 로그인·기권, 관전자 및 사제 상태다.
        for (std::size_t owner=1;owner<9;++owner) {
            players[owner*180+0x2c]=(Next(random)%4U)!=0 ? static_cast<std::uint8_t>(owner):0;
            const auto logged=Next(random)%2U,flagged=Next(random)%2U; // 두 난수 호출 순서를 명시해 컴파일러별 평가 순서에 의존하지 않는다.
            players[owner*180+0x50]=static_cast<std::uint8_t>((logged ? 1U:0U)|(flagged ? 0x80U:0U));
            options[0x3e+owner]=static_cast<std::uint8_t>((Next(random)%5U)==0);dead[owner]=(Next(random)%2U)!=0;
        }
        // 두 방향 동맹을 각각 독립적으로 선택한다.
        for (auto& alliance:alliances) alliance=(Next(random)%4U)==0 ? 1U:0U;
        const bool battle=(sample%10U)!=0,mission=(sample%11U)==0,boss=(sample&1U)!=0,connected=(sample%9U)!=0;
        const std::uint32_t scriptBreak=(sample%13U)==0 ? 1U:0U,owner=sample%8U+1U;const double login=(sample%7U)==0 ? 1.0:0.0;
        std::uint32_t trace=0;netstorm::o::BattleCompletionHooks hooks;
        hooks.serverConnection=[&] { trace=trace*33U+10U;return connected; };
        hooks.priestDead=[&](auto current) { trace=trace*33U+current;return dead.at(current); };
        netstorm::o::RawBattleCompletion gate({players,options,alliances,battle,mission,boss,scriptBreak,login},hooks);
        // 각 판정의 결과와 누적 조회 순서를 같은 행에 저장한다.
        const bool allied=gate.AllAllied(),team=gate.TeamFinished(),flag0=gate.FlagFinished(0),flag1=gate.FlagFinished(1),finished=gate.Finished(),unlogged=gate.UnloggedFinished(owner);
        std::cout<<sample<<' '<<allied<<' '<<team<<' '<<flag0<<' '<<flag1<<' '<<finished<<' '<<unlogged<<' '<<trace<<'\n';
    }
}
