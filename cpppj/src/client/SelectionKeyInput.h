// 원본 일반 키 부모의 정지 관문과 Home/사제/다음 유닛 명령 경로를 실제 입력에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { //선택 몸체와 메뉴 큐의 서로 다른 실행 경계를 보존한다.
struct SelectionKeyInputHooks { //장치는 같은 Client/World의 현재 상태를 조회한다.
    std::function<bool()> paused; //004d96b6의 공통 정지 관문이며 실제 선택 몸체보다 먼저 읽는다.
    std::function<std::uint32_t(std::uint32_t)> command; //현재005429ac/005429d4/00542974의 원문 Home/사제/지식 명령 ID를 조회한다.
    std::function<std::uint32_t()> localOwner; //F5의 두 인자는 같은 현재00540c70 DWORD다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue; //View 명령의 같은 일반 FIFO 입력이다.
    std::function<void(std::uint32_t)> next; //N과Ctrl+F5는 원문 Shift 비트 전체를 기존004da880 몸체에 전달한다.
    std::function<void()> priest; //P/R는 기존 참여 조회/누적 카운터 몸체로 전달한다.
};
bool DispatchSelectionKeyInput(InputEvent event,const SelectionKeyInputHooks& hooks); //확인한 H/N/P/R/F4/F5/F6만 처리하며 정지/금지 수정키도 소비한다. 뗌·다른 명령은 false다.
}
