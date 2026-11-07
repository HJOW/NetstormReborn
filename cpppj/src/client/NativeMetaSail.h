//메타 전투 좌석 좌표와 참가/항해 요청의 서로 다른 송신·현재 기록 쓰기 순서를 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include "client/RawCameraView.h"
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { //같은 CS/카메라/실제 전송을 빌리며 상태나 성공을 합성하지 않는다.
struct NativeMetaSeatHooks { //원문 좌표 계산에서 도달한 TYPE/SHP 장치만 선택한다.
    std::function<std::int32_t(std::uint32_t)> firstFrame; //현재 전투 TYPE의 C 첫 프레임 DWORD다.
    std::function<std::array<float,2>(std::uint32_t,std::int32_t)> muzzle; //419850의 frameCheck/진단 뒤 signed 포구 WORD를 읽는다.
};
std::array<float,2> NativeMetaSeatPosition(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> participant,std::uint32_t battle,std::uint32_t seat,const RawCameraViewState& camera,const NativeMetaSeatHooks& hooks); //42bd10:크기/현재칸→frame C 포구/기본칸→현재 전투 좌표의 각 float 저장을 유지한다.
struct NativeMetaSailHooks { //실제 현재 subscriber·좌석 계산·TCP11·Rig 수명을 선택한 위치에서 빌린다.
    std::function<std::uint32_t()> subscriber; //00540c74의 현재 DWORD를 검색 직전에 읽는다.
    std::function<std::array<float,2>(std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> seat; //같은 현재 참가자의 fort 폭/높이와 실제 SHP 좌석이다.
    std::function<void(std::span<std::uint8_t>)> send; //고정14BYTE/1eh를 같은 TCP11로 전송한다.
    std::function<void(std::span<const std::uint8_t>)> animate; //같은 참가자+b8 SID의 실제419520 Rig 목표를 변경한다.
};
void JoinNativeMetaBattle(ClientChallengeGlobals& challenge,std::uint32_t parameter,const NativeMetaSailHooks& hooks); //42be00:좌석/BYTE→자기 검색→좌표/기록 쓰기→송신→Rig다.
void SailNativeMetaParticipant(ClientChallengeGlobals& challenge,std::array<std::uint32_t,2> position,const NativeMetaSailHooks& hooks); //42beb0:위치/0/0→송신→현재 자기 검색→기록 쓰기→Rig다.
struct NativeMetaSailInputHooks { //일반 입력의6b/6c/e3/fe가 실제로 선택하는 장치만 요구한다.
    std::function<std::int32_t()> passedDiags,noShowWarn,serverSelection; //54db70·현재 noShowWarnFailedDiags 조회·54db4c의 별도 관문이다.
    std::function<std::uint32_t(std::uint32_t)> freeSlots; //현재 전투의8개 DWORD 중0 개수다.
    std::function<void(std::string_view)> tell; //WarnFailedDiags의 원문 signed 매개변수를 실제 Tell에 전달한다.
    std::function<void(std::uint32_t)> join; //실제42be00 부모다. 일반/확인/억제 확인이 공유한다.
    std::function<void(std::array<std::uint32_t,2>)> sail; //signed 상위WORD X/unsigned 하위WORD Y를 float32로 저장한 실제42beb0다.
    std::function<void()> suppress; //441520의 실제 noShowWarnFailedDiags=1 설정과 동기 재해석이다.
};
bool IsNativeMetaSailCommand(std::uint32_t code); //6b/6c/e3/fe의 정수 명령만 확인하여 실제 메뉴 허용 관문과 입력 분배기가 공유한다.
bool DispatchNativeMetaSailInput(InputEvent event,const NativeMetaSailInputHooks& hooks); //4d74ed~75f3의 공통뗌 뒤 분배다. 효과 뒤 일반 입력 꼬리는 caller가 계속 실행한다.
}
