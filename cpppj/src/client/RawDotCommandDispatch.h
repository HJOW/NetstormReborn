// 원문0043f6f0의 로컬 점 명령 분배를 고정 Regexp 문법과 같은 현재 표에 수행한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // Client가 소유하는 원문 명령표와 외부 명령 장치의 경계다.
// 재진입/콜백이 공유하는00557db4와00557df0의 관측 가능한 현재 값을 유지한다.
struct DotCommandDispatchState {
    std::uint32_t fallbackIndex{}; // PE의 기본 명령 슬롯이며 오류 출력 직전에도 현재 값을 다시 읽는다.
    std::int32_t separatorOffset{-1}; // 마지막 Regexp 검색의 그룹3 시작 상대 위치다. 실패는-1이며 빈 입력은 보존한다.
};
// 명령 본문과 오류 표시는 분배기 밖의 실제 장치이며 선택된 경우에만 요구한다.
struct DotCommandDispatchHooks {
    std::function<std::uint32_t(std::uint32_t,std::span<std::uint8_t>)> invoke; // 함수 식별자와 NUL 포함 가변 인자를 받아 원문 반환 코드를 돌려준다.
    std::function<void(std::string_view,std::uint32_t)> error; // 호출 뒤 현재 이름과 비영 코드를 원문0042f100의 서식 장치에 넘긴다.
};
// table은 원문 이름/함수/count 인접 물리, input은 NUL을 포함한 실제 가변 입력이다.
// 인자는127BYTE까지 복사하고 이름은80BYTE까지 비교한다. 물리 밖 원시 포인터 접근은 명시 예외다.
// 공유 separatorOffset은 콜백 뒤 재조회하므로 같은 상태의 중첩 분배 효과도 유지한다.
std::uint32_t DispatchDotCommand(std::span<std::uint8_t> table,DotCommandDispatchState& state,
    std::span<std::uint8_t> input,const DotCommandDispatchHooks& hooks);
}
