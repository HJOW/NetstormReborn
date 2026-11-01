// HTML의 원소별 기술 목록과 전체 목록이 공유하는 원문 기술 필터/고정40칸 수집을 복원한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <span>
#include <string>

namespace netstorm::client { //시나리오 HTML과 기존 실제 기술 부모/controlled 메뉴의 같은 경계를 사용한다.
struct TechnologyListMenuHooks { //게임 계산은 실제 Session이 공급하고 UI는 같은 현재 controlled 메뉴에 작성한다.
    std::function<std::int32_t()> typeCount; //00541348의 현재 signed 개수를 각 반복 끝에서 다시 읽는다.
    std::function<const o::RiftTypeRecord&(std::uint32_t)> type; //0049a840의 현재 TYPE 기록이다.
    std::function<std::uint32_t()> priestType,templeProduct,currentTechnology,learnCommand; //005412d0/0054126c/0048fc60/00542a48의 현재 경계다.
    std::function<std::uint32_t(std::uint32_t)> level; //원문 대상의 실제 가상+80이며 caller의 DWORD를 WORD로 접지 않는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> owns; //004acac0(parent,4,type,0)의 종류WORD 반환이다. SID로 바꾸지 않는다.
    std::function<std::string(std::string_view)> translate; //004de9a0의 현재 "%s (Level %d)" 번역이다.
    std::function<std::string(std::uint32_t)> description; //0049af80의 현재 TYPE 설명이다.
    std::function<std::string(std::string_view,std::string_view,std::int32_t)> format; //실제 고정256BYTE printf의 원문 문자열/표시 레벨이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,bool)> item; //현재 AddText(label,command,type,1,enabled,0)다.
    std::function<void(std::uint32_t)> assertion; //Ui.cpp의 잘못된 level 진단 뒤 실제 순회를 계속한다.
    std::function<void(std::uint32_t)> reference; //00475c40→004644e0으로 현재 메뉴를 실제 대상에 부착한다.
    std::function<std::string(std::int32_t)> themeName; //004a8940의 현재 번역된 원소 이름이다.
    std::function<std::string(std::string_view,std::string_view)> formatString; //제목/최대 레벨의 실제 문자열 printf 인자다.
    std::function<std::string(std::string_view,std::int32_t)> formatNumber; //업그레이드/랭크의 실제 signed 표시값이다.
    std::function<void()> separator; //현재 controlled 메뉴에00478260 구분선을 추가한다.
    std::function<std::uint32_t(std::uint32_t)> surface; //대상 좌표+0.9999의 CRT 절삭과 표면 WORD 조회다. 범위 밖은0이다.
    std::function<std::uint32_t()> poolCapacity,doNothingCommand,upgradeCommand,nuggetType,noIslandType,rank; //호출 위치에서 현재 전역/Player/Core를 다시 읽는다.
    std::function<std::function<std::uint32_t()>()> homeIsland; //Player 주소를 먼저 선택하고 섬 getter 뒤 같은 기록의 본섬 값을 읽는다.
    std::function<std::uint32_t(std::uint32_t)> island; //물리 범위 안 표면의004acda0 섬 번호다. 그래프 번호와 구별한다.
    std::function<std::int32_t()> maximumRank; //004c6c20의 현재 noIsland TYPE의B/C 프레임 개수 곱-1이다.
};
class TechnologyListMenu { //004cb320/004cb3f0과004cb4b0을 실제 현재 장치에 연결한다.
public:
    explicit TechnologyListMenu(TechnologyListMenuHooks hooks); //선택한 분기의 장치는 해당 위치에서만 요구한다.
    std::int32_t CountKnown(std::span<std::uint32_t> output,std::int32_t& stored,std::int32_t theme,std::int32_t level) const; //전체 일치 수와 고정 용량에 저장한 개수는 다르다. 빈 output은 원문 NULL 목록이다.
    std::int32_t CountCandidates(std::span<std::uint32_t> output,std::int32_t& stored,std::int32_t theme,std::int32_t level) const; //소유 조회 없이 후보를 세지만 currentTechnology의 선행 조회는 유지한다.
    void AppendTheme(std::uint32_t target,std::int32_t theme) const; //실제 level-1→고정40칸 두 목록→level별 행과 현재 소유 비활성 판정을 보존한다.
    void AppendAll(std::uint32_t target) const; //004cb690의WORD 대상·네 원소·전체 보유 집계·업그레이드/보상 행을 같은 메뉴에 추가한다.
private:
    std::int32_t Count(std::span<std::uint32_t> output,std::int32_t& stored,std::int32_t theme,std::int32_t level,bool known) const; //두 실제 helper의 다른 조회/진단 위치만 공유한다.
    TechnologyListMenuHooks hooks_; //각 호출에서 현재 Session/Config/UI를 다시 조회하는 실제 경계다.
};
}
