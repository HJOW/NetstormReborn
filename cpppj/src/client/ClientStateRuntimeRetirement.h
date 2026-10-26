// 원문 State의 AI→Player 링크→Kernel→Core0 뒤 반납을 같은 실제 Client/Session에 조정한다.
#pragma once
#include "client/RawProcessHostStorage.h"
#include <functional>
#include <memory>
#include <optional>

namespace netstorm::client { // 게임 전역의 쓰기와 C++ 대여 종료를 구별하며 전체 State 부모는 호출자가 실행한다.
class Client;class RawSurfaceStorage;class RawCampaignSession; // 현재 단일 Client와 실제 물리/실행기를 빌린다.
// 동일 정리 호출의 실제 자식 증빙을 모아 Core0 뒤 모든 고정뷰 소비자를 반납한다. 소멸만으로 정리를 실행하지 않는다.
class ClientStateRuntimeRetirement {
public:
    ClientStateRuntimeRetirement(Client& client,std::shared_ptr<RawSurfaceStorage> storage); // 같은 권한·보조 전역·Player·Kernel과 현재 Session만 허용한다.
    ClientStateRuntimeRetirement(const ClientStateRuntimeRetirement&)=delete; // 실제 정상 복귀 증빙의 반납 권한을 복제하지 않는다.
    ClientStateRuntimeRetirement& operator=(const ClientStateRuntimeRetirement&)=delete; // 진행 중 정리의 소유자를 교체하지 않는다.
    void ResetAi(std::uint32_t owner); // 원문 owner1..8 위치에서 실제 reset과 같은 모듈의 정상 복귀 증빙을 소비한다.
    void ClearPlayerLinks(); // 실제0048fcd0을 실행하고 같은 Deck의 반환 증빙을 소비한다.
    void ClearKernel(); // 같은 전역 Kernel의 단일 Clear만 실행하며 현재 Runtime 처리기는 Core0 뒤까지 유지한다.
    void Retire(const std::function<void()>& releaseOwner); // 실제 known0/Core0 뒤 전체 자식 증빙을 확인하고 현재 소유자를 동기 반납한다. 빈 callback 성공을 인정하지 않는다.
private:
    RawCampaignSession* Current() const; // 매 호출에서 현재 대여의 정체성과 같은 물리/Player 고정뷰를 다시 확인한다.
    Client& client_; // 모든 원문 전역과 Player/Kernel의 실제 장기 owner다.
    std::shared_ptr<RawSurfaceStorage> storage_; // Session의 모든 소비자가 사라진 뒤에도 Pool/TYPE/Host 물리를 유지한다.
    RawCampaignSession* session_{}; // 구성 시점의 실제 공개 Session이며 반납 뒤에는 역참조하지 않는다.
    std::uint64_t generation_{}; // 주소가 재사용돼도 이전 표면 대여와 다음 Session을 구별하는 기존 호스트 세대다.
    std::optional<RawProcessHostStorage::ClearPass> processes_; // 실제 단일 Clear 정상 반환에서만 생기는 이동 전용 증빙이다.
};
}
