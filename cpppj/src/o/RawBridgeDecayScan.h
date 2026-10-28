// 원본00422bc0의 실제 슬롯 방문과 각 칸의 예외 경계를 현재 풀에 연결한다.
#pragma once
#include "o/Bridge.h"
#include "o/GameClock.h"
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o { //프레임 스캔은 수명/파괴의 가상 몸체와 같은 현재 슬롯을 사용한다.
// 초기화 함수와 분리하여 현재 범위·커서·주기를 생성/반납에서 다시 쓰지 않는다.
class RawBridgeDecayScan {
public:
    RawBridgeDecayScan(const SidPool& pool,std::span<const RiftTypeRecord> types,
        BridgeDecayScanState& state,const float& period,const BridgeDecayMode& mode,
        std::function<void(Sid)> cell); //모든 참조와 칸 처리기의 대상은 이 실행기보다 오래 살아야 한다.
    void Frame(const FrameTime& time,bool paused) const; //정지→편집기→권한 관문 뒤 원문의 부분 방문/기한 재설정을 실행한다.
private:
    const SidPool& pool_; //할당 상태와 TYPE/extra를 매 방문 다시 읽는 실제50바이트 풀이다.
    std::span<const RiftTypeRecord> types_; //004ac200이 읽는 현재 TYPE+ec genus 표다.
    BridgeDecayScanState& state_; //0052f96c/970·005453a8/b8의 같은 현재 네 전역이다.
    const float& period_; //0052f968의 현재float이며 기본10으로 고정하지 않는다.
    const BridgeDecayMode& mode_; //005c85a4/00540bc4를 입구에서만 검사한다.
    std::function<void(Sid)> cell_; //004227e0의 실제 한 칸 처리이며 실패 뒤에도 다음 번호로 진행한다.
};
}
