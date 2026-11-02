// 원문004d6e34..6e60의 사건 마스크·뗌·전체화면 검사와 즉시 호출 인자를 보존한다.
#include "client/ScreenMouseKey.h"
#include <stdexcept>

namespace netstorm::client { // 화면 장치의 호환 정책과 입력의 원문 조건을 같은 현재 값으로 연결한다.
bool DispatchScreenMouseKey(const InputEvent& event,const std::function<std::uint32_t()>& screenMode,const std::function<void(std::uint32_t)>& changeMode) { // Guide/모달/정지보다 먼저 조회하며 전환 후에도 사건을 소비하지 않는다.
    if ((event.code&InputCode::kRelease) || (event.code&0x03ffffffU)!=0x7b0000U) return false;
    if (!screenMode) throw std::logic_error("F12 현재 화면 모드 조회 미연결");const auto mode=screenMode(); // 생성 시점 값 대신 누르는 시점의 실제 DWORD를 읽는다.
    if (!(mode&1U)) return false;
    if (!changeMode) throw std::logic_error("F12 실제 화면 모드 전환 미연결");changeMode(mode^0x40U);return true;
}
}
