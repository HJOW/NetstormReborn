// 번개 팔레트의 진행을 실제 ProcessForm/부모 종속 삭제와 같은 Kernel에 연결한다.
#pragma once
#include "client/PaletteFlash.h"
#include "o/SquidProcess.h"

namespace netstorm::client { // 화면 장치는 빌리고 객체 수명은 실제 공통 프로세스 호스트가 소유한다.
// 0054114c의 10.78 초기 TYPE이며 원문 생성자는 flags0x10으로 기술 부모에 부착한다.
inline constexpr std::uint32_t kThunderProcessType=55;
// 00470d40/00470e30/00471650/00471820의 등록·초기화·종료·중단 경계를 잇는다.
class ThunderProcess final:public o::SquidProcess {
public:
    ThunderProcess(bool fullScreen,PaletteFlashHooks hooks,std::function<void(o::ProcessId)> destroyed,std::int32_t brightness=0); //생성 모드/요청 밝기를 보관하며 부착 전 팔레트는 읽지 않는다.0은 기존 기본64다.
    ~ThunderProcess() override; // 화면이 살아 있는 동안 복구하고 등록 목록에서 같은 PID를 지운다.
    void OnAttached() override; // 실제 Kernel/Form/부모 연결 뒤 원본 가상+14에서 팔레트를 초기화한다.
    void RunFrame() override; // 실시간 진행이 끝나면 원문0041bf10처럼 Form 삭제 경로를 사용한다.
private:
    PaletteFlashState state_; // 두 부분 팔레트·64단계·실시간 기한의 실제 효과 상태다.
    PaletteFlash flash_; // 기존 원문 대조를 마친 색 계산과 복구를 같은 상태에 실행한다.
    std::function<void(o::ProcessId)> destroyed_; // Client의 현재 PID 목록만 정리하며 Form을 중복 삭제하지 않는다.
};
}
