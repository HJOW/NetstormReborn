// HTML 부모의 인스턴스 수/현재serial/종료 시각을 같은 Client 수명에 유지한다.
#pragma once
#include "client/HtmlWindowGump.h"
namespace netstorm::client { // 실제 장기 HTML 전역의 소유 계층이다.
struct ClientHtmlWindowGlobals { HtmlWindowGumpState window; }; // 표시 부모를 교체해도5cab2c/34/38와5cad30은 같은 주소다.
}
