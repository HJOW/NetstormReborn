// 원본7ah 압축 이미지를 현재 TYPE/HP·같은 owner 캐시로 공통35BYTE에 복원한다.
#pragma once
#include "o/RawSquidTransmission.h"

namespace netstorm::o { // 송수신의0054d8e4 owner 캐시는 장기 전역 한 개다.
struct SquidCompressedReceiveHooks {
    std::function<std::uint32_t()> defaultType; // 생성자 위치에서 읽는 현재005411a0이다.
    std::function<const RiftTypeRecord&(std::uint32_t)> type; //4번 필드 뒤 현재 TYPE를 선택한다.
    std::function<std::int32_t(const RiftTypeRecord&)> maximumHp; // 선택한 TYPE의 실제0049fce0이다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 상충 마스크/캐시/읽기 진단이다.
};
std::array<std::uint8_t,35> DecodeCompressedSquid(std::span<const std::uint8_t> packet,RawSquidTransmissionState& state,const SquidCompressedReceiveHooks& hooks); //00433fe0/00432a10/00434080의16필드 decoder다.
void ObserveCompressedSquid(std::span<std::uint8_t> packet,std::uint32_t sending,const std::function<void(std::string_view,std::string_view,std::uint32_t)>& assertion); //0040b2a0의 송신 시점 실제 길이 관찰기다.
}
