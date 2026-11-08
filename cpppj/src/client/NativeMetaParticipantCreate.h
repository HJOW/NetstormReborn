// 실제20h 참가자 생성과 같은 기록/World/Image/Gump/Rig/후행 표시 순서를 복원한다.
#pragma once
#include "client/NativeMetaDisplay.h"
#include "client/NativeMetaParticipantName.h"
#include "client/NativeMetaSail.h"

namespace netstorm::client { //원문에서 선택한 실제 장치만 요구하고 참가자 사본/접속 성공을 만들지 않는다.
struct NativeMetaParticipantCreateHooks { //42b580의 실제 생성 하위와 현재 전역 조회 장치다.
    NativeMetaDisplayHooks display;NativeMetaParticipantNameHooks name; //같은 World/Gump와1142의 번역/기술 합계/이름표 서식이다.
    std::function<bool(std::uint32_t)> guildChanged; //42ca20의 미초기화80BYTE와 첫 ReadString 변경 여부다. 단순 키 존재와 구별한다.
    std::function<void(std::uint32_t,std::span<std::uint8_t>)> guild; //42ca60의 별도80BYTE 확인/현재60BYTE 목적 ReadString이며 없는 길드는 쓰지 않는다.
    std::function<void()> refreshCore; //48f190의 현재 전투8자리 subscriber/Core 장부 갱신이다.
    std::function<void(std::int32_t,std::uint32_t,bool)> status; //5제거/3표시를 같은20칸 MessageBoard에 수행한다.
    std::function<void(std::span<std::uint8_t>,std::array<std::uint8_t,6>)> image; //현재identity6로 실제14×14 bitmap을 만든 뒤 같은 CS20BYTE Image를 변환한다.
    std::function<std::uint32_t()> participantType,createName,root; //현재108 TYPE/실제d8 이름표 공장/현재루트 토큰을 각각 원문 위치에서 읽는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> create; //같은 World의 실제 객체(type,flags2) 공장이다.
    std::function<void(std::uint32_t,std::uint32_t)> selectable,scrollable,reference; //같은 이름표의1/0 위젯 설정과464540 객체 부착이다.
    std::function<void(std::uint32_t,bool)> enter; //원본packet bit40이 없을 때만 실제 참가자 Rig를 생성/검색해 입장시킨다.
    std::function<void()> intro;std::function<std::int32_t()> noIntro; //실제419840 카메라 뒤 현재 noShowChalIntro를 읽는다.
    std::function<void(std::string_view)> tell; //선택된 실제 ChalIntro(1) HTML이다.
    std::function<void(std::span<const std::uint8_t>)> monitor; //현재 이름 주소로418200의 alert/monitor/실제 Chat 부모를 호출한다.
};
struct NativeMetaParticipantAddHooks { //42c180은 진입CS/참가자를 보유하고 하위 효과 뒤 필요한 값만 다시 읽는다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Ch.cpp의 복귀형 active/좌석 진단이다.
    std::function<void(std::span<std::uint8_t>,std::span<const std::uint8_t>)> construct; //active 진단 뒤 현재 실제 생성 장치를 결합하는 전체42b580 부모다.
    std::function<void(std::uint32_t)> battle; //캡처한 Battle+4 실제ID의 특정 전투 표시다.
    std::function<std::array<float,2>(std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> seat; //캡처된 참가자와 현재 전투 포구/좌석 계산이다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>,std::uint32_t)> chat; //원문 packet 이름 주소를 같은 Chat 채널20h에 보낸다. 자기 subscriber면 이름을 먼저 읽지 않는다.
    std::function<void(std::uint32_t,std::uint8_t)> update;std::function<void()> refreshInfo; //지정 참가자ff 갱신 뒤 같은 NSInfo Push/Attach다.
};
void ObserveNativeMetaParticipantAdd(std::span<std::uint8_t> packet,bool sending,const std::function<void(std::string_view,std::uint32_t)>& assertion); //40a340의 송신 nameLen 진단→현재BYTE+35h WORD 길이다.
void ConstructNativeMetaParticipant(std::span<std::uint8_t> record,std::span<const std::uint8_t> packet,std::uint32_t& marker,const std::uint32_t& subscriber,const NativeMetaParticipantCreateHooks& hooks); //42b580 전체의 필드/이름/장부/상태/실제 이미지·객체·이름표/입장/카메라/모니터다.
void ReceiveNativeMetaParticipantAdd(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaParticipantAddHooks& hooks); //42c180 전체:캡처 기록 생성→같은 CS 좌석/버전→좌표→Chat→지정 표시→NSInfo다.
}
