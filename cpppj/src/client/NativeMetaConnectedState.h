// State28~33의 현재 CS/로그인 결과·표시 준비와 게임 서버 감시 본체다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client { // 현재 Client 물리를 빌리며 미연결 표시/전송 장치를 만들지 않는다.
class ClientChallengeGlobals; //실제 CS21328BYTE와 현재 토큰을 같은 장기 owner에서 조회한다.
struct MetaPeerCache { //원문00542534/538/53c/540의 서로 다른 현재 DWORD 참조다.
    std::uint32_t& index;std::uint32_t& value; //선택 master 인덱스와 해당 참가자의+a8 캐시다.
    std::uint32_t& battle;std::uint32_t& slot; //자기 참가자의+a4/+a8 캐시다.
};
struct NativeMetaConnectedGlobals { //상태 처리 중 같은 Client/CS owner가 반납되면 안 된다.
    o::LocalMissionStateStorage state; //28의 응답+38/32의 창serial+3c와 같은 다섯144BYTE다.
    ClientChallengeGlobals& challenge; //실제 CS 현재 물리의 조회/Reset을 수행한다.
    o::PlayerWorldState& player; //같은 현재 battleOwner/PlayerKnown을 호출 위치에서 다시 읽는다.
    std::uint32_t& zone; //005467b8의 현재 메타 존이다.
    std::uint32_t& metadataMode;std::uint32_t& connected; //005c85c8/005c85c4는 별개의 현재 DWORD다.
    std::uint32_t& notified;MetaPeerCache cache; //005c85b4의 현재 통지 여부와 네 캐시를 직접 쓴다.
    const std::uint32_t& gamePort; //005318e8의 같은 현재 DWORD이며 각 실제 원문 위치에서 읽는다.
};
struct NativeMetaConnectedHooks { //실제 장치 접합 전에는 선택된 필수 하위를 빈 함수로 대체하지 않는다.
    std::function<void(std::uint32_t)> request; //004b2bc0의 같은 즉시 상태 요청이다.
    std::function<std::string(std::string_view)> translate; //004de9a0의 현재 표시 언어 번역이다.
    std::function<void(std::string_view)> progress; //완성한 문구의004cf570이며 다시 번역하지 않는다.
    std::function<std::uint32_t(std::string_view)> tell; //004cf960의 실제 현재 HTML serial을 반환한다.
    std::function<std::optional<std::uint32_t>()> modalSerial; //00565dd4 NULL 또는 현재창+2c serial이다.
    std::function<void()> closeProgress,resetCamera,display; //004c8dc0/004340d0/0047dd60의 실제 전체 장치다.
    std::function<void(std::uint32_t)> rootLogin; //004b40c0(connection)의16h 요청이며29에서는11로 보낸다.
    std::function<void(std::uint32_t)> listen,closeConnection; //004e2430(port)/004e2940(id)이다.
    std::function<void()> closePending,notifyMaster,sendLogin; //004e2a00/004b2c00/004b62b0의 실제 본체다.
    std::function<bool(std::uint32_t)> open;std::function<std::int32_t(std::uint32_t)> status; //004e2b10/004e2b70의 현재 노드를 조회한다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t)> connect; //004e2620(id,address,port,0)의 실제 접속이다.
};
std::int32_t FindMetaBattleMaster(ClientChallengeGlobals& challenge,std::uint32_t owner); //004b2c60의 자기+a4 전투/8참가자와 bit4 첫 master 조회다.
std::string MetaLoginReason(std::int32_t code,const std::function<std::string(std::string_view)>& translate); //0042b090의 Unknown 선번역/여섯 이유/선택값 재번역 순서다.
o::LocalMissionParentResult InstallNativeMetaConnectedState(std::uint32_t selected,NativeMetaConnectedGlobals globals,const NativeMetaConnectedHooks& hooks); //28~33 실제 본체이며 공통phase 확정은 기존 분배기에 남긴다.
}
