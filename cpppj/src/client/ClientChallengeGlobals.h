// Client가 실제 CS 포인터 슬롯·할당·공통 배치 좌표를 장기 소유한다.
#pragma once
#include "o/RawChallengeState.h"
#include <functional>
#include <map>
#include <memory>

namespace netstorm::client { // 메타 서비스 객체와 지형 TYPE Outpost를 구별하는 원문 소유자다.
class ClientChallengeGlobals {
public:
    ClientChallengeGlobals()=default; // 새 Client의 CS/좌표 BSS0만 만들고 실제 객체는 Native 진입 때 확보한다.
    ClientChallengeGlobals(const ClientChallengeGlobals&)=delete;ClientChallengeGlobals& operator=(const ClientChallengeGlobals&)=delete; // 현재 슬롯/할당/빌린 물리를 복제하지 않는다.
    ClientChallengeGlobals(ClientChallengeGlobals&&)=delete;ClientChallengeGlobals& operator=(ClientChallengeGlobals&&)=delete; // 이미 빌린 DWORD/좌표 주소를 이동하지 않는다.
    std::uint32_t& Current();const std::uint32_t& Current() const; //005467d8의 실제 같은 DWORD를 Session/메타 입력이 빌린다.
    o::ChallengeGeometry& Geometry();const o::ChallengeGeometry& Geometry() const; // 같은 최초 초기화 표와 현재 표시 크기를 반환한다.
    bool Prepare(const std::function<void()>& uninitialized); // State19/30의 새 생성 또는 listsInitialized 진단→기존Reset을 실행한다.
    std::span<std::uint8_t> Bytes();std::span<const std::uint8_t> Bytes() const; // 현재 할당21328BYTE를 직접 빌린다.0/다른 포인터는 물리 경계에서 거부한다.
    std::span<std::uint8_t> Battle(std::uint32_t index); // 실제0..9의292BYTE 전투 기록이다.
    std::span<std::uint8_t> Participant(std::uint32_t index); // 실제0..39의460BYTE 참가자 기록이다.
private:
    std::uint32_t current_{},nextToken_{1}; // Native 현재0과 host 포인터를 자르지 않는 별도 비영 할당 토큰이다.
    o::ChallengeGeometry geometry_; // CS 객체 재사용/교체에서도 유지하는 같은 Client BSS다.
    std::map<std::uint32_t,std::unique_ptr<std::uint8_t[]>> allocations_; // 교체된 실제 할당도 최종 Client 수명까지 소유한다. 명시 파생 해제 부모는 별도다.
};
}
