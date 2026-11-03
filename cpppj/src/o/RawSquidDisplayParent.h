// 004ad500의 현재 TYPE 두 조회·frameCheck 진단·표시 요청을 순수 부모로 보존한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // 진단/로그 효과 뒤에도 같은 현재 raw와 타입 필드를 읽는다.
struct RawSquidDisplayTypeView { // 선택된 TYPE의 주소 정체성을 유지하는 비소유 필드 뷰다.
    std::uint32_t token; // 현재 표에서 선택된 TYPE의 논리 식별값이며 실제 포인터를 역참조하지 않는다.
    const std::uint32_t& flags1; // TYPE+e8의 이중 프레임 비트다.
    const std::int32_t& frameCount; // TYPE+114의 현재 signed 프레임 개수다.
    const bool& loaded; // TYPE+dc의 비영/null 조건이다.
    const std::int32_t& header; // TYPE+0의 두 해제 표식과 비교하는 현재 DWORD다.
};
struct RawSquidDisplayParentHooks { // TYPE 조회와 이미 존재하는 로그/진단/표시 자식만 외부에 둔다.
    std::function<RawSquidDisplayTypeView(std::uint32_t)> type; // raw의 현재 BYTE 번호로 같은 표의 뷰를 반환한다.
    std::function<std::uint32_t(RawSquidDisplayTypeView,std::int32_t,std::uint32_t)> describe; // 원문49adc0의 선택 TYPE/고정 frame/현재 abstract 비트다.
    std::function<void(std::uint32_t)> log; // describe가 반환한 메시지 토큰의43d40이다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문846 진단이며 복귀할 수 있다.
    std::function<void(std::uint32_t,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> mark; //4990d0의 현재TYPE/고정frame/현재x·y비트/marker16이다.
};
// raw 주소/수명과 반환 TYPE 필드의 주소는 모든 callback 동안 유지되어야 한다. frame만 진입 시 고정한다.
void RunRawSquidDisplayParent(std::span<const std::uint8_t> raw,RawCheckingFlag checking,const RawSquidDisplayParentHooks& hooks);
}
