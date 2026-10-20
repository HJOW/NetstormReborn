// 큰 효과 뒤의 현재 이름/flags/구독자를 다시 읽고 원본 C locale의 첫 글자 정규화를 유지한다.
#include "client/TutorialInit.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 도달한 실제 효과를 빈 성공으로 대체하지 않는다.
template<class F> const F& Effect(const F& effect) { if (!effect) throw std::logic_error("Tutorial Init 실제 효과 누락");return effect; }
// 원본 C문자열의 첫NUL 뒤는 이름/설정/로드로 해석하지 않는다.
std::string_view CString(std::string_view value) { return value.substr(0,value.find('\0')); }
}
// 이름 지역80BYTE·로드256BYTE 범위 밖의 원본 오버런만 안전한 호스트에서 명시 거부한다.
void InitializeTutorialMission(TutorialInitState& state,std::uint32_t& flags,std::uint32_t& demo,TutorialInitGlobals globals,const TutorialInitHooks& hooks) {
    Effect(hooks.clearTemplates)();globals.tutorialNumber=std::bit_cast<std::uint32_t>(o::ConfigParseLong(Effect(hooks.get)("tutorialNumber","0")));if (globals.tutorialNumber) globals.tutorialSeen=1;
    globals.missionNumber=std::bit_cast<std::uint32_t>(o::ConfigParseLong(Effect(hooks.get)("missionNumber","0")));
    const auto text=Effect(hooks.get)("demo",std::nullopt);if (!CString(text).empty()) demo=std::bit_cast<std::uint32_t>(o::ConfigParseLong(text));Effect(hooks.resetOptions)();
    const auto name=CString(state.name);if (name.size()>=state.scratch.size()) throw std::length_error("Tutorial Init 이름80BYTE 초과");std::copy(name.begin(),name.end(),state.scratch.begin());state.scratch[name.size()]=0;
    const auto lower=o::AsciiLower(name);std::copy(lower.begin(),lower.end(),state.scratch.begin());if (!lower.empty() && state.scratch[0]>='a' && state.scratch[0]<='z') state.scratch[0]=static_cast<char>(state.scratch[0]-'a'+'A');
    state.namePrefix=0;
    // 정규화 지역의 실제 첫4바이트를 x86 little endian DWORD로 읽는다. 짧은 이름의 꼬리를 지우지 않는다.
    for (std::size_t index=0;index<4;++index) state.namePrefix|=std::uint32_t(static_cast<std::uint8_t>(state.scratch[index]))<<(index*8);
    Effect(hooks.set)("fileName",{state.scratch.data(),name.size()});globals.loginSubscriber=globals.initialSubscriber;
    const auto fort=std::string(CString(Effect(hooks.get)("loadFort",CString(state.name))));if (fort.size()>=256) throw std::length_error("Tutorial Init loadFort256BYTE 초과");Effect(hooks.requestFort)(fort);Effect(hooks.header)();flags|=1U;
}
}
