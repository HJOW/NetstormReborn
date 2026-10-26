// 같은 Client의 원문 State 다섯 블록과 로그인/기한 전역을 메뉴/Editor/로그인이 함께 빌린다.
#pragma once
#include "o/RawLocalMissionState.h"
#include "o/RawClockSyncReset.h"
#include "o/RawMetaZoneCleanup.h"
#include <array>

namespace netstorm::client {
// Session 교체에서도 같은 State 블록과 로그인/메뉴의 현재 전역 주소를 유지한다.
class ClientLocalStateGlobals {
public:
    ClientLocalStateGlobals()=default; // 새 Client에서만 원문 BSS0과 명령표의 실제 초기값을 만든다.
    ClientLocalStateGlobals(const ClientLocalStateGlobals&)=delete; // 같은 블록을 빌리는 State/패킷 소비자에게 사본을 만들지 않는다.
    ClientLocalStateGlobals& operator=(const ClientLocalStateGlobals&)=delete; // 현재 사용 중인 블록 주소를 대입으로 바꾸지 않는다.
    ClientLocalStateGlobals(ClientLocalStateGlobals&&)=delete; // 장기 참조가 이전 객체에 남는 이동을 금지한다.
    ClientLocalStateGlobals& operator=(ClientLocalStateGlobals&&)=delete; // 실제 Client 수명 동안 주소를 고정한다.
    std::array<o::LocalMissionStateBlock,5>& Blocks() { return blocks_; } // old/phase/immediate/current/next 순서의 물리 배열 한 개다.
    const std::array<o::LocalMissionStateBlock,5>& Blocks() const { return blocks_; } // 진단도 같은720BYTE를 읽는다.
    o::LocalMissionStateStorage State() { return {blocks_[0],blocks_[1],blocks_[2],blocks_[3],blocks_[4]}; } // 기존 원문 분배기에 같은 블록 참조를 제공한다.
    std::uint32_t& FirstMenuPending() { return firstMenuPending_; } //00542530의 초기1이며 WelcomeGiven 비영 분기의 안내 종료 위치에서만0을 쓴다.
    const std::uint32_t& FirstMenuPending() const { return firstMenuPending_; } //재진입에서도 같은 현재 DWORD를 읽는다.
    std::uint32_t& UserUpdated() { return userUpdated_; } //0054da8c의 BSS0이며 security.dat/명령줄 검증의 별도 writer를 기다리는 실제 저장소다.
    const std::uint32_t& UserUpdated() const { return userUpdated_; } //Config의 같은 이름 키로 값을 대체하지 않는다.
    std::uint32_t& TellTips() { return tellTips_; } //0053190c의 초기1과 실제 tellTips ReadInt writer가 같은 DWORD를 공유한다.
    const std::uint32_t& TellTips() const { return tellTips_; } //State3는 Tell callback 뒤의 현재 옵션값을 이 주소에서 다시 읽는다.
    std::uint32_t& TipNumber() { return tipNumber_; } //00531910의 초기1과 tipNumber ReadInt/실제 TellTip 순번 변경이 같은 DWORD를 공유한다.
    const std::uint32_t& TipNumber() const { return tipNumber_; } //옵션 변경과 안내 재진입 뒤에도 같은 현재 순번 주소를 읽는다.
    double& ActivityEnd() { return activityEnd_; } // 기존 Login과 Editor가00544d58의 같은 벽시각 기한을 직접 쓴다.
    const double& ActivityEnd() const { return activityEnd_; } // 기한 사본이나 별도 Editor 타이머를 만들지 않는다.
    std::uint32_t& ZonePending() { return zonePending_; } // 005c85c0이며 실제 Fort 요청 작성과 State 정리가 같은 DWORD를 쓴다.
    const std::uint32_t& ZonePending() const { return zonePending_; } // 요청 캡처의 sequence와 구별하는 현재값이다.
    std::uint32_t& SourceType() { return sourceType_; } // 0055a420을 State와 Reply 수신이 직접 공유한다.
    const std::uint32_t& SourceType() const { return sourceType_; } // Session 밖에서도 마지막 값을 보존한다.
    bool& SavedGame() { return savedGame_; } // 005c85d4의 현재 정상0/1 모델을 로그인/State가 함께 빌린다.
    const bool& SavedGame() const { return savedGame_; } // 임의 DWORD 상위 비트 보존을 bool 참조로 가장하지 않는다.
    std::uint32_t& Begin() { return begin_; } // 005c7a74의 Login/Ready/타이머가 같은 시작 DWORD를 쓴다.
    const std::uint32_t& Begin() const { return begin_; } // 세션 반납은 시작값을 암묵 초기화하지 않는다.
    std::uint32_t& CentralCreated() { return centralCreated_; } // 005c85a0의 실제 metadata writer와 cleanup이 공유할 DWORD다.
    const std::uint32_t& CentralCreated() const { return centralCreated_; } // 현재 중앙섬 상태를 복제하지 않는다.
    double& CaptureEnd() { return captureEnd_; } // 005c8618의 Reply/Zone/LoginBegin 벽시각 기한이다.
    const double& CaptureEnd() const { return captureEnd_; } // 연결/로그인/활동 기한과 별개다.
    double& ConnectEnd() { return connectEnd_; } // 005c85f8의 State8/9 연결 만료 시각이다.
    const double& ConnectEnd() const { return connectEnd_; } // 시계 getter나 프레임 시각으로 대체하지 않는다.
    double& LoginEnd() { return loginEnd_; } // 005c85f0의 로그인/Dais/충돌 효과가 같은 double을 빌린다.
    const double& LoginEnd() const { return loginEnd_; } // 효과 억제 관문도 현재 double의 실제 주소를 사용한다.
    std::uint32_t& ScriptBreak() { return scriptBreak_; } // 00542544의 DWORD이며 Login 완료0과 Abort1을 같은 주소에 쓴다.
    const std::uint32_t& ScriptBreak() const { return scriptBreak_; } // 최초 PE값1과 현재 임의 DWORD 비트를 그대로 읽는다.
    std::uint32_t& AbortPending() { return abortPending_; } // 005c85e0은004b6350이1을 쓰고004b8cb0이 검사하는 별도 DWORD다.
    const std::uint32_t& AbortPending() const { return abortPending_; } // requestPhase005c85ac와 합치지 않는 현재 값이다.
    std::uint32_t& StateAuxCC() { return stateAuxCC_; } // 005c85cc의 정확한4BYTE만 소유하며 인접005c85d0과 구별한다.
    const std::uint32_t& StateAuxCC() const { return stateAuxCC_; } // 확인된 Native 직접 writer는004b2df0의0 store다.
    std::uint32_t& MetadataMode() { return metadataMode_; } // 005c85c8은004b6dd0의1 store와0047d8c0/0047e630의 읽기를 보존한다.
    const std::uint32_t& MetadataMode() const { return metadataMode_; } // 이름은 편의 표기이며 미복원 부모의 전체 역할을 확정하지 않는다.
    std::uint32_t& ConnectionNotified() { return connectionNotified_; } // 005c85b4는004b6dd0의 별도0검사/1쓰기 DWORD다.
    const std::uint32_t& ConnectionNotified() const { return connectionNotified_; } // 명칭과 무관하게 Native State의 현재 비트 폭을 유지한다.
    std::uint32_t& CachedPeerIndex() { return cachedPeerIndex_; } // 00542534의 State 선택 인덱스 캐시이며004b2cc0도ffffffff로 무효화한다.
    const std::uint32_t& CachedPeerIndex() const { return cachedPeerIndex_; } // 다른 세 캐시와 별도로 현재 값을 읽는다.
    std::uint32_t& CachedPeerValue() { return cachedPeerValue_; } // 00542538은004b6dd0에서 선택 레코드 DWORD와 비교하는 캐시다.
    const std::uint32_t& CachedPeerValue() const { return cachedPeerValue_; } // 00542528/2c의 저장 복원 슬롯과 구별한다.
    std::uint32_t& CachedPeerFieldA4() { return cachedPeerFieldA4_; } // 0054253c는004b6dd0에서 선택 대상+a4 DWORD와 비교한다.
    const std::uint32_t& CachedPeerFieldA4() const { return cachedPeerFieldA4_; } // 원문 캐시의 모든32비트를 보존한다.
    std::uint32_t& CachedPeerFieldA8() { return cachedPeerFieldA8_; } // 00542540은004b6dd0에서 선택 대상+a8 DWORD와 비교한다.
    const std::uint32_t& CachedPeerFieldA8() const { return cachedPeerFieldA8_; } // 인접 캐시를 참조하는 주소 산술을 만들지 않는다.
    double& ReconnectSaveEnd() { return reconnectSaveEnd_; } // 005c8610의8BYTE이며004b6730이 별도로 검사하는 벽시각 기한이다.
    const double& ReconnectSaveEnd() const { return reconnectSaveEnd_; } // LoginEnd005c85f0와 ActivityEnd00544d58을 대신하지 않는다.
    std::uint32_t& HudModeCommand() { return hudModeCommand_; } // 005427ec의 현재 명령을 호출 직전에 읽고 변경도 보존한다.
    std::uint32_t& AddIslandCommand() { return addIslandCommand_; } // 00542780의 현재 명령을 빈섬 예약 직전에 읽는다.
    o::ClockSyncResetGlobals ClockSynchronization() { return {syncConnection_,syncReplyMode_,gameTicks_}; } //004c1f20이 정확히 지우는 실제 세 DWORD를 값 복사 없이 빌린다.
    const std::uint32_t& SynchronizationConnection() const { return syncConnection_; } //005c8bf4의 현재 동기화 token이며 전체22연결표와 구별한다.
    const std::uint32_t& SynchronizationReplyMode() const { return syncReplyMode_; } //005c8c00은 인접 동기화 송수신 카운터와 별도다.
    std::uint32_t& GameTicks() { return gameTicks_; } //0055b4ac의 현재 DWORD를 로그인 State와 정리가 직접 공유한다.
    const std::uint32_t& GameTicks() const { return gameTicks_; } //시계의 double·프레임 번호로 바꾸지 않고 원시32비트를 보존한다.
    std::uint32_t& RandomSeed() { return randomSeed_; } //005c85b8의 같은 현재 State 난수 기록을 로그인과 Editor가 빌린다.
    std::uint32_t& MetaRefresh() { return metaRefresh_; } //005c85d8의 현재 메타 갱신 예약 DWORD다.
    std::uint32_t& MetaRebuild() { return metaRebuild_; } //005c85dc의 별도 메타 재구성 예약 DWORD다.
    bool& Simulconnect() { return simulconnect_; } //0054db50의 정상0/1을 모든 State 소비자가 함께 읽는다.
    std::uint8_t& BattleByte() { return battleByte_; } //005c85d0의 첫 BYTE이며 인접 stateAuxCC와 다른 저장소다.
    std::array<std::uint32_t,2>& ZoneWords() { return zoneWords_; } //005c86c0의 DWORD와+4 WORD다. 마지막 상위WORD는 기존 호스트 보존 칸이다.
    const std::array<std::uint32_t,2>& ZoneWords() const { return zoneWords_; } //State 다섯144BYTE와 별개인 실제 Zone 입력을 조회한다.
    std::span<std::uint8_t> Zone() { return {reinterpret_cast<std::uint8_t*>(zoneWords_.data()),6}; } //로그인/metadata와 정리는 같은 앞6BYTE만 빌린다.
    std::span<const std::uint8_t> Zone() const { return {reinterpret_cast<const std::uint8_t*>(zoneWords_.data()),6}; } //인접 보존WORD를 원문 Zone 길이에 포함하지 않는다.
    o::MetaZoneCleanupGlobals MetaZones() { return {metaFirstReset_,metaZoneRecords_}; } //00531760과00546908의51×144BYTE를 같은 장기 주소에 제공한다.
    const std::uint32_t& MetaFirstReset() const { return metaFirstReset_; } //최초 PE값1은 Client 생성 한 번뿐이며 Session 생성/반납으로 되돌리지 않는다.
    std::span<const std::uint8_t> MetaZoneRecords() const { return metaZoneRecords_; } //내용은 같은 mutable MetaZones에서만 변경하며 사본을 만들지 않는다.
    std::array<std::uint32_t,255>& PlayerNoMakeWarnings() { return playerNoMakeWarnings_; } //0054d4e8의 타입별 경고 비트 표이며 SP 추적/TotalMade와 별개다.
    const std::array<std::uint32_t,255>& PlayerNoMakeWarnings() const { return playerNoMakeWarnings_; } //Session 대여 전후에도 같은 전체255DWORD를 조회한다.
private:
    std::array<o::LocalMissionStateBlock,5> blocks_{}; //005c8630/8758/86c8/87e8/8878의 독립144BYTE 다섯 개다.
    std::uint32_t firstMenuPending_{1},userUpdated_{},tellTips_{1}; //각 PE 초기값1/BSS0/1을 새 Client에서만 한 번 구성하며 State 교체로 되돌리지 않는다.
    std::uint32_t tipNumber_{1}; //00531910의 PE 초기1이며 TellTips 옵션과 별개인 실제32비트 안내 순번이다.
    double activityEnd_{}; //00544d58의 최초 BSS0이며 게임 정지 가능한 시계로 바꾸지 않는다.
    std::uint32_t zonePending_{},sourceType_{},begin_{},centralCreated_{}; // 순서가 다른 Native 주소들의 명시 소유자이며 주소 산술 배열로 쓰지 않는다.
    bool savedGame_{}; // 기존 실제 소비자의 정상0/1 표현을 유지하며 임의 Native DWORD 모델은 후속이다.
    double captureEnd_{},connectEnd_{},loginEnd_{}; // 각 Native 주소의 최초 BSS0과 서로 다른 수명을 유지한다.
    std::uint32_t scriptBreak_{1}; // 보호 PE00542544의 최초 DWORD는1이며 Session 생성으로0을 덮어쓰지 않는다.
    std::uint32_t abortPending_{}; // 005c85e0의 최초 BSS0이다. 미복원 Abort 부모의 요청 상태와 같은 수명이다.
    std::uint32_t stateAuxCC_{}; // 005c85cc의 최초 BSS0이며 알려지지 않은 역할을 다른 필드와 합치지 않는다.
    std::uint32_t metadataMode_{}; // 005c85c8의 최초 BSS0이며 Native 부모 접합 전에도 현재 DWORD를 유지한다.
    std::uint32_t connectionNotified_{}; // 005c85b4의 최초 BSS0이며 requestPhase와 별도로 보존한다.
    std::uint32_t cachedPeerIndex_{0xffffffffU}; // 보호 PE00542534의 최초 캐시 무효값이다.
    std::uint32_t cachedPeerValue_{0xffffffffU}; // 보호 PE00542538의 최초 DWORD를 독립적으로 보존한다.
    std::uint32_t cachedPeerFieldA4_{0xffffffffU}; // 보호 PE0054253c의 최초 무효값이다.
    std::uint32_t cachedPeerFieldA8_{0xffffffffU}; // 보호 PE00542540의 최초 무효값이다.
    double reconnectSaveEnd_{}; // 005c8610의 최초 BSS +0이며 cleanup의wall+10 store는 후속 실제 adapter가 맡는다.
    std::uint32_t hudModeCommand_{0x0200003dU}; //보호PE005427ec의 실제 명령 초기값이다.
    std::uint32_t addIslandCommand_{0x02000022U}; //보호PE00542780의 실제 Add Island 명령 초기값이다.
    std::uint32_t syncConnection_{},syncReplyMode_{},gameTicks_{}; //세 개의 실제 최초 BSS DWORD며 정리 세 store 이외의 재초기화를 추가하지 않는다.
    std::uint32_t randomSeed_{},metaRefresh_{},metaRebuild_{}; //세 State DWORD의 최초 BSS0을 Client 수명에 한 번 구성한다.
    bool simulconnect_{};std::uint8_t battleByte_{}; //각 최초 BSS0이며 Session 교체가 초기화하지 않는다.
    std::array<std::uint32_t,2> zoneWords_{}; //기존 로그인의8BYTE 물리를 장기화한다. 원문 Zone 정리는 앞6BYTE만 쓴다.
    std::uint32_t metaFirstReset_{1}; //보호PE00531760의 초기값이며 이후 정리의 실제 분기 값이다.
    std::array<std::uint8_t,o::kMetaZoneCleanupBytes> metaZoneRecords_{}; //51개 기록의 고정7344BYTE이며 다른 State·MetaMap 표와 합치지 않는다.
    std::array<std::uint32_t,255> playerNoMakeWarnings_{}; //PATCH 최초 BSS0이며 Session 생성/반납이나 SP 초기화로 지우지 않는다.
};
}
