// 원본004b8cb0의 저장 복구/자동 서버 선택을 같은 현재 Player와 상태 블록으로 수행한다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <string_view>

namespace netstorm::o { // 실제 장치 수명은 Client에 두고 선택/현재 쓰기 순서만 복원한다.
struct BattleReconnectGlobals {
    LocalMissionStateStorage state; // current15의 동기 실행과 next7/3의 다음 Poll 요청을 구분한다.
    const std::uint32_t& abortPending;std::uint32_t& scriptBreak;std::uint32_t& sourceType; // 별도 중단/대본/출처 전역이다.
    std::uint32_t& mySubscriber;const std::uint32_t& restoreOwner;const std::uint32_t& restoreSubscriber; // reader 뒤 같은 복구 슬롯을 다시 읽는다.
    std::span<const std::uint8_t> previousHandle;std::span<std::uint8_t> handle; // NUL까지 복사하는 이전/현재 이름 물리다.
    std::uint32_t& noRestartSerial; //005c893c의 안내 생존 조회/실제 생성 serial이다.
    std::function<std::span<const std::uint8_t>()> players; // reader 효과 뒤 재할당된 현재9행을 조회한다.
};
struct BattleReconnectHooks {
    std::function<bool()> battleFinished,checking; // 현재 전투 종료/진단 관문이며 성공 표식을 쓰지 않는다.
    std::function<void()> closeAll,readSaved,takeServer,deleteSaved; // 원문의22칸 종료/파일/동기 State15 부모다.
    std::function<void(std::string_view)> setHandle,log; // 현재 이름 설정과1d 진단에 같은 바이트를 전달한다.
    std::function<std::uint32_t()> gamePort;std::function<float()> localSp; // 원격 후보가 선택된 시점의 현재 포트/SP다.
    std::function<void(std::string_view,std::uint32_t)> assertion; // 진단이 정상 복귀하면 서버 인수로 계속한다.
    std::function<bool(std::uint32_t)> serialAlive;std::function<std::uint32_t()> tellNotRestart; // 이미 살아 있는 안내에서는 파일을 다시 지우지 않는다.
};
bool ReconnectBattle(bool reloadFile,BattleReconnectGlobals globals,const BattleReconnectHooks& hooks); // 자동/복구 후보를 고르고 실제 원격 next7을 작성했을 때만 참이다. 입력 관문 복귀를 새 시작으로 오해하지 않는다.
}
