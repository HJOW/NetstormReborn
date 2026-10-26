// 원본004bd400의 Subscriber 생성과 캡처한 ID/현재 원문 이름 쓰기 부모다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace netstorm::o { //현재 manager/등록/stream 장치를 기존 저장 본체에 연결한다.
struct FortSubscriberWriteHooks { //선택한 외부 경계만 요구하고 실제 이름/일반 writer는 기존 본체를 재사용한다.
    std::function<std::uint32_t(std::uint32_t)> manager; //고정 this 슬롯의 현재 manager를 진단 전과 등록 helper 복귀 뒤 다시 읽는다.
    std::function<void(std::uint32_t)> assertion; //Template.cpp463(manager0)/155(null 이름)의 실제 줄 진단이며 복귀하면 원문을 계속한다.
    std::function<void()> registerSubscriber; //004b38d0의 실제 FinishFortSubscriberRegistration을 같은 Client 전역/Config에 연결한다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> create; //현재 manager에서 CreateDataManagerSection("Subscriber")가 반환한 고정 stream 토큰이다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> write; //같은 명시 stream에 기존 WriteMemStream을 연결한다. 입력 span을 선행 복사/절단하지 않는다.
};
void SaveFortSubscriber(std::uint32_t slot,std::uint32_t capturedId,std::optional<std::span<const std::uint8_t>> capturedHandle,const FortSubscriberWriteHooks& hooks); //ID는 부모의 인자값을 유지하고 이름은 캡처된 같은 물리 포인터를 ID쓰기 뒤 문자열 writer에서 처음 읽는다.
}
