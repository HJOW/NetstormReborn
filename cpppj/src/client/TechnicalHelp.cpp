// 원문435e40/435ef0/435f10과41ad10의 선택/출력/비대기 실행 순서다.
#include "client/TechnicalHelp.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::client { //순수 부모는 OS·Client 장치를 선택 위치에서만 요구한다.
namespace { //실제 C문자열과 필요한 콜백만 처리한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Technical Help의 선택된 실제 장치 미연결");return function; } //미선택 실행 장치는 요구하지 않는다.
std::string_view Text(std::string_view value) { return value.substr(0,value.find('\0')); } //첫NUL까지만 읽는다.
void Patch(o::OriginalEdition edition) { if (edition!=o::OriginalEdition::Patch1078) throw std::invalid_argument("Technical Help는 실제10.78 경계 필요"); } //CD는 별도 근거로 연결한다.
std::string Slashes(std::string_view value) { std::string result(Text(value));std::replace(result.begin(),result.end(),'/','\\');return result; } //원문 Filespec 구분자로 변환한다.
}
// 두 반환은 short circuit 없이 모두 읽으며 첫 경로 성공은 두 번째 실패에도 남긴다.
bool ResolveTechnicalHelp(o::OriginalEdition edition,std::string& exe,std::string& file,const TechnicalHelpHooks& hooks) {
    Patch(edition);exe.clear();file.clear();const auto first=Need(hooks.readString)("techHelpExe",exe);const auto second=Need(hooks.readString)("techHelpFile",file);exe=Text(exe);file=Text(file);
    if (!(first&second)) return false;const auto program=Need(hooks.resolve)(exe);if (!program) return false;exe=Text(*program);const auto argument=Need(hooks.resolve)(file);if (!argument) return false;file=Text(*argument);return true;
}
// 실제435ef0은 두 로컬 출력만 가지며 실행 효과가 없다.
bool TechnicalHelpAvailable(o::OriginalEdition edition,const TechnicalHelpHooks& hooks) { std::string exe,file;return ResolveTechnicalHelp(edition,exe,file,hooks); }
// 실제435f10의 반환은 실행 장치의 성공이 아니라 앞선 파일 가용성이다.
bool LaunchTechnicalHelp(o::OriginalEdition edition,const TechnicalHelpHooks& hooks) { std::string exe,file;const bool available=ResolveTechnicalHelp(edition,exe,file,hooks);if (available) Need(hooks.launch)(exe,file);return available; }
// FileSpec 자체의 장치 판정은 존재 확인 없이 원래 이름을 반환한다.
std::optional<std::string> ResolveExternalFilePath(std::string_view name,std::uint32_t* source,const ExternalFileHooks& hooks) {
    name=Text(name);const auto found=[&](std::uint32_t kind,std::string value)->std::optional<std::string> { if (source) *source=kind;return value; }; //실패에서는 호출자의 기존 출처를 보존한다.
    if (Need(hooks.device)(name)) return found(5,std::string(name));auto path=Need(hooks.path)(false,name);if (Need(hooks.readable)(path)) return found(1,std::move(path));
    if (Need(hooks.archive)(false,name)) return found(2,std::string(name));path=hooks.path(true,name);if (hooks.readable(path)) return found(3,std::move(path));if (hooks.archive(true,name)) return found(4,std::string(name));return {};
}
// 원문은 전체 정규화 없이 끝 구분자 한 개만 지우고 루트 뒤 구분자를 보장한다.
std::string JoinExternalFileRoot(std::string_view root,std::string_view name) {
    auto result=Slashes(root);if (result.find_first_of("\\:")==result.npos) result=".\\"+result;if (result.size()>1 && result.back()=='\\') result.pop_back();if (result.empty() || result.back()!='\\') result+='\\';
    auto part=Slashes(name);part.erase(0,part.find_first_not_of('\\'));if (part.size()>1 && part.back()=='\\') part.pop_back();result+=part;return result;
}
// 현재 원본 명령만 분배하며 일반 문자/다른 메뉴는 소비하지 않는다.
bool DispatchTechnicalHelpInput(std::uint32_t event,const std::function<void()>& launch) { if ((event&0x03ffffffU)!=0x02000049U) return false;if (!(event&0x80000000U)) Need(launch)();return true; }
}
