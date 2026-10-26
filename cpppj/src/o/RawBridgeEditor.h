// 원본 10.78 편집기의 실제 전체 다리 품질 변경을 같은 SID 풀/프레임/난수에서 실행한다.
#pragma once
#include "o/SidPool.h"
#include "o/SpStore.h"
#include <functional>

namespace netstorm::o { //편집 명령의 원문 객체/난수 규칙을 화면 장치와 분리한다.
// 품질 변경의 진단과 실제 가상 +88 갱신만 상위 월드에 맡긴다.
struct BridgeEditorHooks {
    std::function<const RiftTypeFrames&(std::uint32_t)> frames; //단어 진단 뒤 현재 TYPE의 실제 코드 표를 조회한다.
    std::function<void(Sid)> notifySurface; //004214a0의 현재 수명 진단이다. 모드/저장을 대신하지 않는다.
    std::function<void(Sid)> refresh; //직접 프레임 DWORD를 쓴 뒤 같은 객체의 가상 +88을 실행한다.
};
// 00422a70은 free 비트만 건너뛰며 기존 다리 TYPE과 같은 전역 crackLife를 현재값으로 읽는다.
class RawBridgeEditor {
public:
    RawBridgeEditor(SidPool& pool,const std::uint32_t& bridgeType,const std::int32_t& crackLife,GameRandom& random,BridgeEditorHooks hooks); //모든 참조는 처리기보다 오래 살아야 한다.
    void SetAll(std::uint32_t quality) const; //0=Cracked,1=Normal,2=Hard다. 그 밖의 정수는 원문처럼 변경하지 않는다.
private:
    std::span<std::uint8_t> Bytes(Sid sid) const; //예약 슬롯을 포함한 원문 물리 범위만 검사하며 원문에 없는 할당/상태 관문을 추가하지 않는다.
    void Change(Sid sid,std::uint32_t quality) const; //각 방문의 현재 WORD를 먼저 쓰고 진단→프레임 선택→직접 쓰기→표시 순서를 지킨다.
    SidPool& pool_; //iterator/하위 변경이 함께 읽는 실제 풀이다.
    const std::uint32_t& bridgeType_; //005411a0의 같은 현재 TYPE 번호다.
    const std::int32_t& crackLife_; //0052f960의 현재 수명 경계다.
    GameRandom& random_; //Hard의 후보 선택도 다른 플레이 요소와 같은 게임 난수를 소비한다.
    BridgeEditorHooks hooks_; //코드 표/진단/가상 표시 장치를 빌린다.
};
}
