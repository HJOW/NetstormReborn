// HTML 부모의 인스턴스 수/현재serial/종료 시각을 같은 Client 수명에 유지한다.
#pragma once
#include "client/HtmlWindowGump.h"
#include "client/TellSection.h"
namespace netstorm::client { // 실제 장기 HTML 전역의 소유 계층이다.
struct ClientHtmlWindowGlobals { HtmlWindowGumpState window;TellSectionState tell; }; // 표시 부모를 교체해도 창 전역과5cab40의 현재 Tell 이름은 같은 Client 주소다.
}
