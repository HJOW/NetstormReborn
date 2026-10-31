// 10.78의 실제 Objects 스트림을 읽으면서 같은 객체/내용물/월드에 즉시 적용하는 장치다.
#pragma once
#include "o/RawMemStream.h"
#include "o/SidPool.h"
#include <string_view>

namespace netstorm::o { // 원본 DWORD 토큰과 현재 MemStream을 사용하며 완성 레코드 목록을 만들지 않는다.
struct FortObjectStreamHooks { // 선택/파일 수명은 호출자가 유지하고 순회/읽기/현재 상태 판단은 로더가 수행한다.
    std::function<std::uint32_t(std::int32_t)> manager; // 고정 Template 슬롯의 현재 manager DWORD다.
    std::function<std::uint32_t(std::uint32_t,std::string_view,std::int32_t)> select; // 같은 manager의 Select(name,1)이 반환한 stream 토큰이다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; // 고정 stream의 현재16BYTE/버퍼를 한 읽기 동안 빌린다.
    std::function<void(std::int32_t)> prepareTypes; // 같은 현재255BYTE 변환표를 준비하는 실제004bd800이다.
    std::function<std::uint32_t()> conversion,checking; // 현재005c89bc/005e4794를 각각 필요한 위치에서 읽는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> conversionBuffer; // 현재 변환표 토큰의 실제 물리255BYTE다.
    std::function<void()> releaseConversion; // 정상 꼬리의 현재표 free 후 current0이며 예외 경로에서는 호출하지 않는다.
    std::function<void(Sid,std::uint32_t)> contents; // 같은 stream 위치에서 실제004bd130(parent,stream,9,0)을 실행한다.
    std::function<void(std::uint32_t)> assertion; // 원문 진단 행을 전달하며 복귀하면 중간 효과를 유지하고 계속한다.
};
}
