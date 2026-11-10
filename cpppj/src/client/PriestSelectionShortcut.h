// 원본 P/R 키의 사제 조회·첫 선택·반복 가운데 명령과 같은 장기 DWORD를 연결한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::client { //화면 좌표 재클릭 대신 실제 참여 조회와 입력 큐 경계를 정의한다.
struct PriestSelectionShortcutHooks { //참여 결과와 장치는 같은 실제 Session/Client를 조회한다.
    std::function<std::uint32_t()> localOwner,viewCommand; //00540c70과005429d4를 원문 소비 시점에서 읽는다.
    std::function<std::uint32_t(std::uint32_t)> find; //00494f10의 실제 살아 있는 사제/운반체/제단 참여 조회다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> select,queue; //004d5c90의 세 인자와00452fe0의 일반 명령을 구별한다.
    // pieceHeld: 건설 준비 조각(생산 칸/메뉴에서 집은 건물·유닛)이 커서에 붙어 있는지 조회한다. 실제 커서 잠금 깊이 0과 미리보기 SID 비0이다.
    // cancelPiece: 붙어 있는 조각을 원문 취소 00446e70(우클릭과 같은 반환 경로)으로 돌려놓는다.
    // 두 장치는 선택 사항이며 둘 다 있을 때만 쓴다. 없으면 원문 10.78의 호출 순서가 그대로 유지된다.
    // 10차 NB20(알려진 버그): 사제를 선택해도 짓던 건물이 커서에 남던 원문 결함을 고치려고 추가했다.
    std::function<bool()> pieceHeld;
    std::function<void()> cancelPiece;
};
void SelectPriestByKey(std::uint32_t& selected,std::uint32_t& repeated,const PriestSelectionShortcutHooks& hooks); //004d9884..004d98e1: 조회 전 selected 캡처→변경 시 count0→Select 후 현재count 증가, 같고 비0이면 View 큐다.
}
