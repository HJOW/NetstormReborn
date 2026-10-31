// 원본004b51e0의 요청 작성과 다음 루프까지 보유하는 실제 Zone 이미지다.
#pragma once
#include "o/RawLocalMissionState.h"
#include "zacket/LocalMissionTransport.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace netstorm::client {
class State; // 호스트 지연 로드 큐를 요청 owner 해제 때만 함께 반납한다.
// 요청 작성 시점의 원본 전역을 빌려 큰 효과 뒤 현재 값을 다시 읽는다.
struct MissionFortRequestGlobals {
    o::LocalMissionStateBlock& next; // 005c8878의 전체144BYTE다. 지정 필드 밖은 보존한다.
    std::uint32_t& sequence; // 005c85c0이며 복제 효과 뒤 현재 값을1 증가한다.
    std::span<const std::uint8_t> localAddress; // 00513870의9BYTE만 복사한다. 뒤3BYTE는 쓰지 않는다.
    const std::uint32_t& port; // 005318e8의 현재 포트이며 복제 전에 캡처한다.
};
// 실제 Fort 선택과 관찰기/힙 복제의 외부 수명 경계다.
struct MissionFortRequestHooks {
    std::function<void(std::string_view)> selectFort; // 00460980의 큰 Fort 선택을 먼저 수행한다.
    std::function<std::uint32_t(zacket::LocalMissionPacket&)> duplicate; // 선언4인512BYTE를 관찰/복제하고 소유한 이미지 핸들을 반환한다.
};
// 전체 요청을 작성하되 State7의 전투 정리/설치/상태 실행을 앞당기지 않는다.
void WriteMissionFortRequest(std::string_view name,MissionFortRequestGlobals globals,const MissionFortRequestHooks& hooks);
// 실제 Client Init이 작성한 요청과 Zone 복사본의 수명을 Session 설치까지 보유한다.
struct MissionFortLoadRequest {
    o::LocalMissionStateBlock next{}; // Init이 만든 실제144BYTE의 요청 캡처다. 전체 State7 실행 상태와 구별한다.
    std::uint32_t sequence{}; // 요청 작성 뒤 관측한 순번 캡처이며 실제005c85c0 owner를 대신하지 않는다.
    std::string fort; // 같은 Fort 선택 효과가 소유 복사한 UTF-8 이름이다.
    std::vector<std::uint8_t> zone; // 관찰기 확정 뒤 실제86BYTE 복사본이며 원본512BYTE 스택의 참조가 아니다.
    bool consumed{}; // 실제 Zone 설치 후 반납 여부다. 재설치는 명시 거부한다.
    std::optional<o::LocalMissionStartingSnapshot> starting; // 실제 State7 앞단이 캡처한 정리 전 지역 값이다. 있으면 Login은 현재 Native 요청을 Init 캡처로 덮어쓰지 않는다.
};
// 원본0040a1e0(1)의 길이 확정과004dece0의 실제 소유 복제를 같은 버퍼에 수행한다.
std::uint32_t CaptureMissionFortZone(MissionFortLoadRequest& request,zacket::LocalMissionPacket& packet);
// Guide 교체/전체 종료에서 이전 미션의 지연 로드만 취소한다. State의 이전 World 반납에서는 호출하지 않는다.
void ReleaseMissionFortLoad(State& pending,std::shared_ptr<MissionFortLoadRequest>& request);
}
