// 원본 P/R 키의 사제 조회·첫 선택·반복 가운데 명령과 같은 장기 DWORD를 연결한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::client { //화면 좌표 재클릭 대신 실제 참여 조회와 입력 큐 경계를 정의한다.
struct PriestSelectionShortcutHooks { //참여 결과와 장치는 같은 실제 Session/Client를 조회한다.
    std::function<std::uint32_t()> localOwner,viewCommand; //00540c70과005429d4를 원문 소비 시점에서 읽는다.
    std::function<std::uint32_t(std::uint32_t)> find; //00494f10의 실제 살아 있는 사제/운반체/제단 참여 조회다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> select,queue; //004d5c90의 세 인자와00452fe0의 일반 명령을 구별한다.
};
void SelectPriestByKey(std::uint32_t& selected,std::uint32_t& repeated,const PriestSelectionShortcutHooks& hooks); //004d9884..004d98e1: 조회 전 selected 캡처→변경 시 count0→Select 후 현재count 증가, 같고 비0이면 View 큐다.
}
