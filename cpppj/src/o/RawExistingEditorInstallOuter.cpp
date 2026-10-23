// callback뒤현재owner/이름/Editor를다시읽으며하위부재와원문다른분기를명시적으로구별한다.
#include "o/RawExistingEditorInstallOuter.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 실제선택한경계만필수로요구하며미복원본문의빈성공을추가하지않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("기존Editor외곽실제경계미연결");return function;
}
// 다른원문경로는인계후에도이한정함수의정상성공으로돌아오지않는다.
[[noreturn]] void Outside(const std::function<void(ExistingEditorInstallOutside)>& function,ExistingEditorInstallOutside branch) {
    Need(function)(branch);throw std::logic_error("기존Editor외곽한정범위밖경로");
}
// 실제첫BYTE접근만검사하고State의나머지필드나수명을복제하지않는다.
std::span<std::uint8_t> State(const std::function<std::span<std::uint8_t>(std::uint32_t)>& getter,std::uint32_t slot) {
    auto bytes=Need(getter)(slot);if (bytes.empty()) throw std::out_of_range("기존Editor외곽State첫BYTE물리범위");return bytes;
}
}
// 004bd340은현재manager진단뒤항상0이므로기존유효파일에서도State|1/Notify가실행된다.
void InstallExistingEditorFile(const ExistingEditorFileHooks& hooks) {
    if (!Need(hooks.exists)(Need(hooks.path)())) Outside(hooks.outside,ExistingEditorInstallOutside::NewFort);
    const auto path=Need(hooks.path)();Need(hooks.load)(Need(hooks.owner)(),path); //경로getter복귀뒤현재owner를읽는다.
    if (!Need(hooks.valid)(Need(hooks.owner)())) Outside(hooks.outside,ExistingEditorInstallOutside::InvalidFile);
    const auto fixed=Need(hooks.owner)();if (!Need(hooks.manager)(fixed)) Need(hooks.assertion)(410); //원문IsDemo의정상결과0을다른headerflag로대체하지않는다.
    State(hooks.state,Need(hooks.owner)())[0]|=1U;Need(hooks.notify)(Need(hooks.owner)());
    Need(hooks.subscriber)(Need(hooks.owner)());Need(hooks.setConfig)("fort.mySubHandle",Need(hooks.handle)());Need(hooks.title)();
}
// world단계는실제Session의같은부모를한번호출하고그뒤외곽순서만이본문이소유한다.
void InstallExistingEditorFort(const ExistingEditorFortHooks& hooks) {
    const auto pending=Need(hooks.pendingName)(),current=Need(hooks.currentName)(); //원문비교시점의두문자열주소를캡처한다.
    if (Need(hooks.compare)(current,pending)) {
        if (Need(hooks.inMission)() && !Need(hooks.missionName)(Need(hooks.pendingName)())) Need(hooks.closeGuide)();
        Need(hooks.setName)(Need(hooks.pendingName)());
    }
    const auto name=Need(hooks.currentName)();if (name.empty() || name.front()=='.') Outside(hooks.outside,ExistingEditorInstallOutside::DefaultName);
    Need(hooks.file)();if (Need(hooks.inMission)()) Outside(hooks.outside,ExistingEditorInstallOutside::Mission);
    Need(hooks.world)();Need(hooks.deck)(Need(hooks.owner)());Need(hooks.priests)();Need(hooks.camera)();Need(hooks.production)();
    Need(hooks.coreWrite)(Need(hooks.owner)());Need(hooks.snapshot)(Need(hooks.owner)());
    State(hooks.state,Need(hooks.owner)())[0]&=0xfeU;Need(hooks.notify)(Need(hooks.owner)());Need(hooks.startupStateCheck)();
    if (Need(hooks.editor)()) { Need(hooks.setConfig)("lastEditFort",Need(hooks.currentName)());Need(hooks.editorPlayers)(); }
}
}
