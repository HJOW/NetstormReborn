// 원본 Htmlgump.cpp·Ubergump.cpp의 조건/메뉴 명령과 본문을 구성한다.
#pragma once
#include "o/ConfigInterface.h"

namespace netstorm::client {
// 다이얼로그 항목이 실행할 명령과 인자이다.
struct DialogAction { /* command: 선택·클릭할 때 실행할 명령 이름 또는 명령 ID. */ /* argument: 타입·패턴·명령에 별도로 전달하는 첫 인자. */ std::string command, argument; };
// 다이얼로그의 표시 문구·동작·버튼 상태이다.
struct DialogItem {
    // label: 화면에 표시할 항목 문구 또는 패턴 번호.
    std::string label;
    // action: 항목의 활성화 때 실행할 명령·인자 자료.
    DialogAction action;
    // menu: 메뉴 여부.
    // checked: 선택·체크 표식을 표시할지.
    // enabled: 해당 동작을 입력·실행할 수 있는지.
    // instant: 버튼 해제 대기 없이 누름 즉시 동작할지.
    bool menu{}, checked{}, enabled{true}, instant{};
};
// 다이얼로그 본문 한 문단과 글꼴 효과이다.
struct DialogParagraph { /* text: 표시하거나 해석할 본문 문자열. */ std::string text; /* heading: 이동 방향 번호. */ int heading{}; /* italic: 본문에 기울임 글꼴 효과를 적용할지. */ /* bold: 본문에 굵은 글씨 효과를 적용할지. */ bool italic{}, bold{}; };
// 다이얼로그 한 페이지의 문단·항목·배경·시간 제한 동작이다.
struct DialogPage {
    // paragraphs: 표시 순서대로 해석한 다이얼로그 문단 목록.
    std::vector<DialogParagraph> paragraphs;
    // items: 읽거나 생성한 항목의 순서 있는 목록.
    std::vector<DialogItem> items;
    // background: 배경 출력 자료 또는 배경 Gump 토큰.
    std::string background;
    // timeoutSeconds: 다이얼로그 시간 제한의 길이(초).
    int timeoutSeconds{};
    // timeout: 다이얼로그 시간 제한에 도달하면 실행할 동작.
    // onExit: 페이지에서 나갈 때 실행할 동작.
    DialogAction timeout, onExit;
};
// 0046c090·0046c810: 조건을 숨기면 중첩 여는 태그는 무시하고 첫 닫힘에서 다시 표시한다.
std::string FilterDialogConditions(std::string_view source);
// 004c83b0: 설정 치환/조건 처리 뒤의 HTML 본문과 $Button/$Menu/$Checked 등을 분리한다.
DialogPage ParseDialogPage(std::string_view source);
}
