// Client 수명 HTML 부모/Tell/파서 공유 전역의 실제 소유자다(cpppj client/ClientHtmlWindowGlobals.h, 기준선1f00a25a).
import { HtmlWindowGumpState } from '../ui/html-window-gump.js';
import { createHtmlTextParserGlobals } from '../ui/html-text-parser.js';
import { createTellSectionState } from '../ui/tell-section.js';

// 창 교체/닫힘 때 재초기화하지 않는 같은 세 저장소를 빌린다. Tell의 current는 원문256BYTE이고 text는 565e70 실제 80BYTE 배경과 565ec0 P 억제를 가진 HtmlTextParser 전역이다(2026-11-14 WB04-E460 교체).
// 사용: const globals=new ClientHtmlWindowGlobals(); UI context.globals에 같은 객체를 전달한다. (2026-11-14 WB04-E456)
export class ClientHtmlWindowGlobals {
    // 5cab2c 인스턴스/serial/시각·원문 사건의 같은 장기 객체다.
    #window = new HtmlWindowGumpState();
    // 5cab40의 첫NUL 현재Tell 이름 256BYTE다. TellSection(E461)이 같은 실제 BYTE를 공유한다.
    #tell = createTellSectionState();
    // 565e70 실제 80BYTE 배경과 565ec0 P억제를 공유하는 HtmlTextParser 전역이다.
    #text = createHtmlTextParserGlobals();
    // 같은 창 상태를 사본 없이 빌린다.
    get window() { return this.#window; }
    // 같은 현재Tell 원문256BYTE를 빌린다.
    get tell() { return this.#tell; }
    // 같은 파서 전역(80BYTE 배경 포함)을 빌린다.
    get text() { return this.#text; }
}
