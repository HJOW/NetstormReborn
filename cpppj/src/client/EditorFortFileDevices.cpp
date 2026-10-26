// 기존파일/설정/원문부모를재사용하고미연결효과나부분실패를정상완료로숨기지않는다.
#include "client/EditorFortFileDevices.h"
#include <cstring>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 실제선택한현재경계만필요로한다.파일/장치유무를생성자에서선행거부하지않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Editor파일의현재장치미연결");return function; }
}
// 장기장치참조와scratch를보관하며아직파일/manager/State효과를발생시키지않는다.
EditorFortFileDevices::EditorFortFileDevices(EditorFiles& work,const o::BaseFileSystem& source,o::ConfigInterface& config,EditorFortPathEffects effects)
    :work_(work),source_(source),config_(config),effects_(std::move(effects)) {}
// 실제현재이름진단뒤같은고정키를선택하고기존Config함수로설정경로를얻는다.
std::string_view EditorFortFileDevices::CurrentPath() {
    const auto name=Need(effects_.currentName)();if (name.empty() || !name.front()) Need(effects_.assertion)(39);
    Need(effects_.selectFileSpec)();const auto key=Need(effects_.fileSpecKey)();const auto current=Need(effects_.currentName)();configuredPath_=config_.PathSpec(key,current);
    namespacePath_=std::filesystem::path(configuredPath_).is_absolute() ? configuredPath_ : o::NormalizeGamePath(configuredPath_);workingPath_=(effects_.resolve ? effects_.resolve(configuredPath_) : work_.ResolvePath(namespacePath_)).string();return workingPath_;
}
// 파일존재/읽기는설치부모가별도로실행한다.이준비는작업namespace에없는복사본만만든다.
std::string_view EditorFortFileDevices::PrepareCurrent() {
    static_cast<void>(CurrentPath());if (effects_.prepare) { effects_.prepare(configuredPath_);return workingPath_; }if (work_.Exists(workingPath_)) return workingPath_;
    const auto bytes=source_.TryRead(configuredPath_);if (bytes) static_cast<void>(work_.PreparePath(namespacePath_,*bytes));return workingPath_;
}
// 실제경로를재검사한뒤출처5/0을그대로반환한다.
std::int32_t EditorFortFileDevices::Exists(std::string_view path) const { return work_.Exists(path); }
// 파일권한값은Config저장억제DWORD와독립적으로그순간다시읽는다.
bool EditorFortFileDevices::ReadOnly(std::string_view path) const { return work_.ReadOnly(path,effects_.assertion); }
// 기존절대경로읽기는아카이브/원본기본경로로fallback하지않는다.
std::vector<std::uint8_t> EditorFortFileDevices::Read(std::string_view path) const {
    const auto actual=work_.ResolvePath(path);auto bytes=source_.TryRead(actual.string());return bytes ? std::move(*bytes) : std::vector<std::uint8_t>{};
}
// 실제CRT비교를빌리며새ASCII대소문자정책이나파일파서를만들지않는다.
TemplateFileLoadHooks EditorFortFileDevices::BindLoad(TemplateFileLoadHooks hooks) const {
    hooks.read=[this](auto path) { return Read(path); };hooks.readOnly=[this](auto path) { return ReadOnly(path); };
    hooks.sameName=[](auto first,auto second) { const std::string left(first),right(second);return _stricmp(left.c_str(),right.c_str())==0; }; //string_view를실제NUL종료CRT인자로만연결한다.
    return hooks;
}
//manager0도유효성false이며비영인알수없는토큰도원문처럼비영결과다.후속접근은별개다.
std::int32_t ExistingEditorTemplateValid(CampaignTemplateStorage& storage,std::uint32_t slot) {
    if (slot>=9) throw std::out_of_range("Editor valid의실제9슬롯범위");const auto header=storage.Headers().subspan(slot*20,4);return (header[0] || header[1] || header[2] || header[3]) ? 1 : 0;
}
}
