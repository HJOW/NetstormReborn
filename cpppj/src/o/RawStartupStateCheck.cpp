// 디컴파일의 두 피연산자 float 선절삭 대신 실제 x87의 double 계산 뒤 float32 저장을 보존한다.
#include "o/RawStartupStateCheck.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr double kSecondsToMinutes=1.0/60.0; //0050c130의실제double상수이며곱한뒤에만float32로저장한다.
constexpr std::string_view kButtons="$Button=%s,StateReconnect,0 $Button=%s,DelStateFile,0 "; //실제앞버튼과뒤삭제명령을같은문자열로서식화한다.
constexpr std::string_view kQuestion="A battle was started %2.1f minutes ago, but was not cleanly exited.  Attempt to reconnect?%s"; //번역에전달하는원문키이며버튼서식뒤에조회한다.
// 옵션과 시간 관문을 통과해 실제 도달한 하위만 요구한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("startupStateCheck 실제 선택 장치 미연결");return function;
}
}
// 실제 저장상태 reader가0을 반환해도 벽시각 조회와 경과분 계산을 앞당겨 생략하지 않는다.
std::uint32_t CheckStartupState(const StartupStateCheckHooks& hooks) {
    if (!Need(hooks.configInt)("startupStateCheck")) return 0;
    const auto saved=Need(hooks.readSavedState)(); //같은signedDWORD이며임의Unix시각정책을추가하지않는다.
    const double elapsed=Need(hooks.wallSeconds)()-static_cast<double>(saved); //원문fild 뒤double현재시각에서빼는순서다.
    const float minutes=static_cast<float>(elapsed*kSecondsToMinutes); //비교와서식에는실제로저장한float32만쓴다.
    if (!saved || !(minutes>0.0F && minutes<45.0F)) return 0;
    const auto yes=Need(hooks.translate)("Yes");const auto no=Need(hooks.translate)("No"); //각반환의주소는첫서식이끝날때까지유효해야한다.
    const auto buttons=Need(hooks.formatButtons)(kButtons,yes,no); //질문번역보다앞선원문256BYTE지역버퍼의장치경계다.
    const auto question=Need(hooks.translate)(kQuestion); //앞선서식callback뒤의현재번역을조회한다.
    const auto text=Need(hooks.formatWarning)(question,static_cast<double>(minutes),buttons); //원문float32분을double로승격한정확한인자다.
    Need(hooks.tell)(text,0,0);return 1;
}
}
