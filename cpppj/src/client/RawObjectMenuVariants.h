// 원본10.78의 여덟 파생+a8 메뉴를 공통 객체/회수 계층 앞에 작성한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 현재 세션과 실제 controlled 메뉴가 제공하는 조회/작성 경계다. 호출 시점의 값을 사용한다.
struct ObjectMenuVariantHooks {
    std::function<std::uint32_t(std::uint32_t)> global; // 원본 모드·로컬 소유자·명령 전역을 주소 식별자로 읽는다.
    std::function<std::uint32_t()> localOwner; //0040da40의 실제 로컬 번호다. 직접 전역을 읽는 분기와 구별한다.
    std::function<std::int32_t(o::Sid)> maximumHp; //004adcb0의 현재 최대 HP다.
    std::function<bool(o::Sid)> eventZero,hasBrain; //004afb40(this,0)과0040f100(this)의 서로 다른 현재 존재 검사다.
    std::function<std::uint32_t(o::Sid)> count; // 현재 가상+80의 전체DWORD다.
    std::function<void(std::uint32_t)> aiMenu; //00413dd0(owner)의 큰 AI 디버그 작성 경계다.
    std::function<std::string(std::string_view)> translate; // 현재 원문 번역을 먼저 완료한다.
    std::function<std::string(std::string_view,std::int32_t)> format; // 번역이 끝난 고정%d 서식만 출력한다.
    std::function<std::string()> helpWord; //0054db38의 현재 문자열이며 번역을 한 번 더 하지 않는다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> item; // label/command/parameter/include/enabled의 원문DWORD이며 force0이다.
    std::function<void(std::string_view)> header; // 실제 메뉴 제목 뒤 설명 줄이다.
    std::function<void()> separator; //00478260의 실제 조건부 구분선이다.
    std::function<void(o::Sid)> object,damageable; // 기존004b1270/0044cbd0의 전체 공통 부모다.
    std::function<void(o::Sid,std::uint32_t,std::uint32_t)> salvage; //0044ca00의 명시 타입/현재 개수 인자다.
};
// 명령 ID의 원본 초기값이다. 게임 상태·통신 성공을 공급하지 않는다.
std::optional<std::uint32_t> ObjectMenuVariantCommandValue(std::uint32_t address);
// UI 명령 검증과 미연결 실행 진단에서 같은 원본 ID 표를 사용한다.
bool IsObjectMenuVariantCommand(std::uint32_t code);
// 파생 가상 주소별 원문 분기를 보존하며 없는 파생을 기본 메뉴로 바꾸지 않는다.
class RawObjectMenuVariants {
public:
    RawObjectMenuVariants(const o::SidPool& pool,ObjectMenuVariantHooks hooks); // 같은10.78 풀을 빌리며 생성 중 메뉴 효과를 실행하지 않는다.
    void Open(o::Sid sid,std::uint32_t originalBody) const; // 현재+a8 본체 주소를 입력받아 해당 전체 파생을 작성한다.
private:
    std::span<const std::uint8_t> Raw(o::Sid sid) const; // 원본 물리 슬롯을 효과 뒤 다시 읽는다.
    std::uint32_t Read(o::Sid sid,std::size_t offset,std::size_t width=4) const; // 비정렬 WORD/DWORD를 낮은 바이트부터 읽는다.
    std::uint32_t Global(std::uint32_t address) const; // 선택한 분기에 필요한 현재 전역만 조회한다.
    const o::SidPool& pool_; // 메뉴가 빌리는 같은 실제 자산 풀이다.
    ObjectMenuVariantHooks hooks_; // 실제 게임 조회와 controlled 작성의 현재 수명이다.
};
}
