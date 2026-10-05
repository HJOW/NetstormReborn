// 원본 Htmlgump.cpp·Ubergump.cpp의 조건/메뉴 명령과 본문을 구성한다.
#pragma once
#include "o/ConfigInterface.h"

namespace netstorm::client {
struct DialogAction { std::string command, argument; };
struct DialogItem {
    std::string label;
    DialogAction action;
    bool menu{}, checked{}, enabled{true}, instant{};
};
struct DialogParagraph { std::string text; int heading{}; bool italic{}, bold{}; };
struct DialogPage {
    std::vector<DialogParagraph> paragraphs;
    std::vector<DialogItem> items;
    std::string background;
    int timeoutSeconds{};
    DialogAction timeout, onExit;
};
// 0046c090·0046c810: 조건을 숨기면 중첩 여는 태그는 무시하고 첫 닫힘에서 다시 표시한다.
std::string FilterDialogConditions(std::string_view source);
// 004c83b0: 설정 치환/조건 처리 뒤의 HTML 본문과 $Button/$Menu/$Checked 등을 분리한다.
DialogPage ParseDialogPage(std::string_view source);
}
