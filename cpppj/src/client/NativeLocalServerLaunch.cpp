//원문004b8026..004b80ee의여섯후보와선택효과순서를보존한다.
#include "client/NativeLocalServerLaunch.h"
#include <array>
#include <stdexcept>

namespace netstorm::client { //State37의큰부모를실행하고접속성공이나Challenge참가자를합성하지않는다.
namespace { //선택한실제장치가없을때해당위치에서중단한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("로컬서버실행의선택장치미연결");return function; } //앞선Native효과는그대로보존한다.
}
o::LocalMissionParentResult InstallNativeLocalServerLaunch(o::LocalMissionStateStorage state,const NativeLocalServerLaunchHooks& hooks) {
    if (state.current[0]!=37) throw std::logic_error("로컬서버실행의현재37계약");Need(hooks.progress)();Need(hooks.closeDiscovery)();const auto executable=Need(hooks.executable)(); //원문ReadString뒤같은이름을모든후보에서유지한다.
    constexpr std::array<std::string_view,6> prefixes{{".\\debug\\","..\\r\\debug\\","..\\..\\r\\debug\\",".\\r\\debug\\",".\\","..\\"}}; //00542624의실제여섯포인터순서다.
    for (const auto prefix:prefixes) { //첫비영출처에서검색을끝내며그이전실패마다다음34를쓴다.
        const auto candidate=Need(hooks.format)(prefix,executable);if (!Need(hooks.exists)(candidate)) { state.next[0]=34;continue; }
        Need(hooks.launch)(candidate,"lan",0);state.next[0]=38;return o::LocalMissionParentResult::Commit;
    }
    return o::LocalMissionParentResult::Commit;
}
}
