// 초기 Intro 표시값과 클릭 뒤의 Native 예약은 실제 장기 상태를 직접 사용한다.
#include "client/IntroMenu.h"
#include <stdexcept>

namespace netstorm::client { // 영상 파일이나 창은 열지 않고 존재/상태 장치를 빌린다.
// 현재 값은 Config ReadString/존재 조회가 실패해도 선행0을 보존한다. 빈 경로는 파일 장치를 요구하지 않는다.
std::uint32_t DetectIntroMovie(std::uint32_t& available,const std::function<std::string()>& path,const std::function<std::uint32_t(std::string_view)>& probe) {
    available=0;if (!path) throw std::invalid_argument("Intro 현재 경로 장치 없음");auto name=path();name.resize(name.find('\0')==name.npos ? name.size() : name.find('\0'));
    if (name.empty()) return available;if (!probe) throw std::invalid_argument("Intro 파일 존재 장치 없음");
    if (probe(name)) available=1;return available;
}
// 일반 입력의 현재 두 인자는 원문처럼 사용하지 않는다. State39의 실제 부모가 준비/영상/복귀를 맡는다.
bool DispatchPlayIntroInput(const InputEvent& event,o::LocalMissionStateStorage state) {
    if ((event.code&0x03ffffffU)!=kPlayIntroCommand) return false;if (!(event.code&InputCode::kRelease)) { state.next[0]=39;state.next[35]=1; }return true;
}
}
