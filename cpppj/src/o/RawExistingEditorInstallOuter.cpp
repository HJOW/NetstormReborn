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
// 유효성이 확정된 파일만 같은 State/Subscriber/config/title 꼬리를 공유한다.
void CompletePresentEditorFile(const ExistingEditorFileHooks& hooks) {
    const auto fixed=Need(hooks.owner)();if (!Need(hooks.manager)(fixed)) Need(hooks.assertion)(410); // 원문 IsDemo는 정상 결과0이며 다른 헤더 flag로 대체하지 않는다.
    State(hooks.state,Need(hooks.owner)())[0]|=1U;Need(hooks.notify)(Need(hooks.owner)());
    Need(hooks.subscriber)(Need(hooks.owner)());Need(hooks.setConfig)("fort.mySubHandle",Need(hooks.handle)());Need(hooks.title)();
}
// 원문의 DWORD 쓰기만 수행하며 주변 헤더나 반납 중 owner 변경을 복제하지 않는다.
void SetEditorReadOnly(const InvalidEditorFileHooks& hooks,std::uint32_t slot,std::uint32_t value) {
    auto header=Need(hooks.header)(slot);if (header.size()!=20) throw std::out_of_range("잘못된 Editor 파일의20BYTE 헤더 범위");
    for (std::size_t byte=0;byte<4;++byte) header[8+byte]=static_cast<std::uint8_t>(value>>(8*byte)); // 같은+8 DWORD의 네 BYTE만 갱신한다.
}
}
// 004bd340은현재manager진단뒤항상0이므로기존유효파일에서도State|1/Notify가실행된다.
void InstallExistingEditorFile(const ExistingEditorFileHooks& hooks) {
    if (!Need(hooks.exists)(Need(hooks.path)())) Outside(hooks.outside,ExistingEditorInstallOutside::NewFort);
    InstallPresentEditorFile(hooks);
}
// 신규/기존 공통 분배가 이미 수행한 존재 검사는 다시 하지 않고 그 뒤 원문 위치부터 이어간다.
void InstallPresentEditorFile(const ExistingEditorFileHooks& hooks) {
    const auto path=Need(hooks.path)();Need(hooks.load)(Need(hooks.owner)(),path); //경로getter복귀뒤현재owner를읽는다.
    if (!Need(hooks.valid)(Need(hooks.owner)())) Outside(hooks.outside,ExistingEditorInstallOutside::InvalidFile);
    CompletePresentEditorFile(hooks);
}
// 실제 파일 부모는 손상 파일 뒤 기본 이름을 재선택하고 존재 검사부터 다시 시작한다.
void InstallEditorFortFile(std::uint32_t& alternate,const NewFortMissingFileHooks& missing,const ExistingEditorFileHooks& present,const InvalidEditorFileHooks& invalid) {
    for (;;) { // 원문은 재시도 상한이나 임의 성공 없이 부재/유효 파일에 도달할 때까지 반복한다.
        if (InstallMissingNewFort(alternate,missing)) return;
        const auto path=Need(present.path)();Need(present.load)(Need(present.owner)(),path);
        if (Need(present.valid)(Need(present.owner)())) { CompletePresentEditorFile(present);return; }
        const auto fixed=Need(present.owner)();SetEditorReadOnly(invalid,fixed,1);Need(invalid.clear)(fixed);SetEditorReadOnly(invalid,Need(present.owner)(),0);
        Need(invalid.log)(29,"Game file was invalid.\n");
        if (Need(invalid.isDefaultName)()) { Need(invalid.log)(29,"Killing game file.\n");Need(invalid.erase)(Need(present.path)()); }
        Need(invalid.defaultName)(); // 전체 이름 선택의 저장/반납/설정 효과를 실제 하위가 수행한 뒤 반복한다.
    }
}
// world단계는실제Session의같은부모를한번호출하고그뒤외곽순서만이본문이소유한다.
void InstallExistingEditorFort(const ExistingEditorFortHooks& hooks) {
    const auto pending=Need(hooks.pendingName)(),current=Need(hooks.currentName)(); //원문비교시점의두문자열주소를캡처한다.
    if (Need(hooks.compare)(current,pending)) {
        if (Need(hooks.inMission)() && !Need(hooks.missionName)(Need(hooks.pendingName)())) Need(hooks.closeGuide)();
        Need(hooks.setName)(Need(hooks.pendingName)());
    }
    const auto name=Need(hooks.currentName)();if (name.empty() || name.front()=='.') Need(hooks.defaultName)();
    Need(hooks.file)();if (Need(hooks.inMission)()) Outside(hooks.outside,ExistingEditorInstallOutside::Mission);
    Need(hooks.world)();Need(hooks.deck)(Need(hooks.owner)());Need(hooks.priests)();Need(hooks.camera)();Need(hooks.production)();
    Need(hooks.coreWrite)(Need(hooks.owner)());Need(hooks.snapshot)(Need(hooks.owner)());
    State(hooks.state,Need(hooks.owner)())[0]&=0xfeU;Need(hooks.notify)(Need(hooks.owner)());Need(hooks.startupStateCheck)();
    if (Need(hooks.editor)()) { Need(hooks.setConfig)("lastEditFort",Need(hooks.currentName)());Need(hooks.editorPlayers)(); }
}
}
