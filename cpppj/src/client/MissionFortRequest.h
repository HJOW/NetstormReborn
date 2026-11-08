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
//004b50b0은 이름 선택 대신 현재 Fort의 Subscriber를 Champion으로 바꾼 뒤 현재 handle을 읽는다.
struct GuideFortRequestHooks {
    std::function<void(std::uint32_t,std::string_view)> subscriber; //004b32f0(1,"Champion")의 실제 같은 Template/Config/My 변경이다.
    std::function<std::string_view()> handle; // 큰 Subscriber 부모 복귀 뒤 현재00540ca8을 읽는다.
    std::function<std::uint32_t(zacket::LocalMissionPacket&)> duplicate; // 길이4의512BYTE를 관찰/복제하는 동일 수명 경계다.
};
//현재 Guide가 살아 있는 State1의 요청이다. Fort 이름 재선택/편집 World/HUD 설치를 앞당기지 않는다.
void WriteCurrentGuideFortRequest(MissionFortRequestGlobals globals,const GuideFortRequestHooks& hooks);
// 실제 Client Init이 작성한 요청과 Zone 복사본의 수명을 Session 설치까지 보유한다.
struct MissionFortLoadRequest {
    o::LocalMissionStateBlock next{}; // Init이 만든 실제144BYTE의 요청 캡처다. 전체 State7 실행 상태와 구별한다.
    std::uint32_t sequence{}; // 요청 작성 뒤 관측한 순번 캡처이며 실제005c85c0 owner를 대신하지 않는다.
    std::string fort; // 같은 Fort 선택 효과가 소유 복사한 UTF-8 이름이다.
    std::vector<std::uint8_t> zone; // 관찰기 확정 뒤 실제86BYTE 복사본이며 원본512BYTE 스택의 참조가 아니다.
    bool consumed{}; // 실제 Zone 설치 후 반납 여부다. 재설치는 명시 거부한다.
    std::optional<o::LocalMissionStartingSnapshot> starting; // 실제 State7 앞단이 캡처한 정리 전 지역 값이다. 있으면 Login은 현재 Native 요청을 Init 캡처로 덮어쓰지 않는다.
};
// 일반 LAN State7은 미션용86BYTE 생성기를 사용하지 않고 실제 호스트 명단 또는 client의 Zone 부재를 인계한다.
struct GeneralBattleStartRequest {
    bool reconnect{}; // 실제 next+84의 저장 복구 요청이며 새 LAN과 달리 빈 현재 이름도 원문대로 보존한다.
    std::vector<std::uint8_t> zone; // 호스트가 작성한 가변 길이 Zone이며 참가자 기록은 원문71BYTE stride다. client는 비운다.
    bool storedZone{};std::uint32_t zoneToken{}; //장기 ZonePackets를 사용하는 실제 요청인지와 취소 시 반납할 최초 소유 token이다. 설치/해제는 현재 State+80을 각각 다시 읽는다.
    std::string handle; // 정리 전 실제 Fort의 표시 부호화 이름이며 새 Session의 원문64BYTE 이름에 인계한다.
    std::optional<o::LocalMissionStartingSnapshot> starting; // 이전 World 정리 전에 캡처한 실제 State7 지역값이다.
    bool consumed{}; // 같은 요청의 정상 후반 소비만 표시하며 Player/Ready/연결 성공을 대신하지 않는다.
    bool textAddress{}; //메타 Native 요청의 current+54는 원문 C문자열이다. 기존 일반 LAN의20BYTE binary 주소와 구별한다.
};
// 원본0040a1e0(1)의 길이 확정과004dece0의 실제 소유 복제를 같은 버퍼에 수행한다.
std::uint32_t CaptureMissionFortZone(MissionFortLoadRequest& request,zacket::LocalMissionPacket& packet);
// Guide 교체/전체 종료에서 이전 미션의 지연 로드만 취소한다. State의 이전 World 반납에서는 호출하지 않는다.
void ReleaseMissionFortLoad(State& pending,std::shared_ptr<MissionFortLoadRequest>& request);
}
