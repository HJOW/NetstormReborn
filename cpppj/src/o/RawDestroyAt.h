// Damageable.cpp의 실제 문자열 위치 해석/점 검색/가상 삭제를 복원한다.
#pragma once
#include "o/RawSquidFinder.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // 현재 슬롯과 원문 finder의 순서를 그대로 사용한다.
//0044c540의 다섯 출력이며 삭제 부모는 radius를 사용하지 않는다.
struct DestroyAtLocation {
    std::int32_t x{},y{}; // 문자열에서 읽은 좌표이며 실제 부모는 float 저장/정수화를 두 번 적용한다.
    std::uint32_t type{}; //0은 전체 타입이며 다른 DWORD는 현재 타입 BYTE와 그대로 비교한다.
    std::int32_t radius{}; // '/' 뒤에서 해석하지만0044c790은 검색 범위에 반영하지 않는 원문 출력이다.
    std::uint32_t owner{}; // '#' 뒤의0은 전체 소유자이며 다른 값은 현재 owner BYTE와 비교한다.
};
// 실제 토큰 구분자 x/)/#//·개행과 기존 atol/타입 이름 조회를 조립한다.
DestroyAtLocation ParseDestroyAtLocation(std::string_view source,const std::function<std::int32_t(std::string_view)>& lookup);
// 파생 삭제를 직접 대체하지 않고 현재 권한/슬롯과 실제 finder/가상 호출만 빌린다.
struct DestroyAtHooks {
    std::function<bool()> boss; //00540bc4의 현재 권한이며 서버00540bc0과 구별한다.
    std::function<void(std::uint32_t)> assertion; // 권한0의Damageable.cpp839진단이며 복귀 뒤 권한을 다시 읽는다.
    std::function<DestroyAtLocation(std::string_view)> parse; // 실제0044c540의 다섯 출력이다.
    std::function<std::uint32_t(SquidSearchArea)> begin; //004b16d0의 같은 x/y 네 인자·flag0 점 검색이다.
    std::function<std::uint32_t()> next,capacity; // 삭제 뒤004b1810과 루프마다 현재005c847c를 읽는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> slot; // 현재 물리50BYTE를 복사 없이 빌린다.
    std::function<std::uint32_t(std::uint32_t)> genus; //004ac200의 현재 분류 비트다.
    std::function<void(std::uint32_t,std::uint32_t)> destroy; // 현재 대상의 실제 가상+10에 원문400000플래그를 전달한다.
};
// 원문 필터를 통과한 현재 대상만 삭제하고 삭제 뒤 다음 항목을 조회한다.
void DestroyObjectsAt(std::string_view source,const DestroyAtHooks& hooks);
}
