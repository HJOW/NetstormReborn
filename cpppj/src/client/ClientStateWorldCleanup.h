// Native 전체 State 정리 부모를 실제 Client 장기 전역·기존 하위·Runtime 반납에 연결한다.
#pragma once
#include "client/ClientStateWorldGlobals.h"
#include <memory>

namespace netstorm::client { // 게임 자식은 내부 실제 owner에 연결하고 외부 UI/미복원 선택 경계만 명시 장치로 받는다.
struct ClientStateWorldDevices {
    std::function<void()> cancelInitialCursor; //Session 없는 첫 메뉴의 같은 Cursor Cancel/Prepare와 현재 Renderer를 연결한다.
    std::function<void()> killGumps,cleanupGumps,replaceUber; //같은 공통 등록표의 기존 Kill/Cleanup/전체 HUD 공장이다.
    std::function<void()> refreshSp; //0043dad0의 현재 예산 serial Push/Attach를 같은 공통 UI에 실행한다.
    std::function<void()> retireRuntime; //이미 정상 반환한 실제 Session과 이를 빌리는 UI/Guide/입력 소비자를 반납한다. 게임 전역을 다시 초기화하지 않는다.
    std::function<void()> leaveMetamap; //실제 metamap 비영에서만 필요한004795a0이며 누락을 빈 성공으로 처리하지 않는다.
    std::function<void(std::uint32_t)> saveBattle; //비mission 전투 phase14에서만 필요한004bf880의 실제 현재 snapshot이다.
    std::function<void(std::uint32_t)> assertion; //원문 중복 모드 진단을 같은 호출 위치에서 보고한다.
};
//004b2df0의 전체 직접 쓰기와34Effect 선택을 실행한다. UI·저장·metamap의 선택 장치 누락은 그 위치에서 실패한다.
void CleanupClientStateWorld(std::uint32_t reason,ClientStateWorldContext context,std::shared_ptr<RawSurfaceStorage> storage,const ClientStateWorldDevices& devices);
}
