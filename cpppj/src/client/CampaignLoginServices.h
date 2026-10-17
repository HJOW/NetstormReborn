// 실제 미션 로그인에 필요한 현재 설정/시계/표시 장치만 빌린다.
#pragma once
#include "o/RawMissionSettingsPacket.h"
#include <functional>
#include <string_view>
namespace netstorm::client {
struct CampaignLoginServices {
    std::function<std::int32_t(std::string_view,std::int32_t)> configInt; //현재 원본 설정값과 누락 기본값이다.
    std::function<double()> wallSeconds; //정지와 무관한 원본 현재 벽시각이다.
    std::function<std::uint32_t()> clock; //원본 CRT clock의 현재32비트다.
    std::function<void()> refresh; //같은 실제 UI/SP 표시의 현재 원문 재수집을 요청한다.
    std::function<bool()> playerUiBusy; //현재 Player UI 작업 중인지 조회하는 실제 장치 경계다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> playerNotice; //현재 Player 표시 사건을 실제 UI에 전달한다.
    std::function<void(std::uint32_t,bool,std::uint32_t,std::span<const std::uint8_t>)> observePacket; //선택검사전용현재sender/boss/local과수신바이트의읽기전용관측이다.
    std::function<void(std::string_view)> log; //로그인/전송의 원본 진단과 상태를 실제 로그에 남긴다.
};
}
