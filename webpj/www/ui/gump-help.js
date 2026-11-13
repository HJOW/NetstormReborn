// 기본 Gump의 우클릭 도움말 메뉴 요청과 콜백 뒤 현재 문자열/전역/루트를 이식한다(cpppj client/GumpHelp.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E414)
import { InputCode } from '../core/input-queue.js';

// 원문 사건의 수정키와 분리된 코드 마스크다.
const HELP_EVENT_MASK = 0x03ffffff;
// 도움말 항목 본문/사건과 현재 %sHelp 주제 문자열이다.
export class GumpHelpState {
    // 현재 전체DWORD 본문/사건 토큰과 같은 장기 주제다.
    body = 0; event = 0; topic = '';
}

// 같은 기본/파생 노드와 실제 메뉴/소리 장치를 빌린다.
export class GumpHelp {
    // 장기 등록/도움말 상태와 현재 메뉴 경계다.
    #create; #state; #hooks;
    // 미선택 메뉴/소리는 선행 요구하지 않는다. 사용: new GumpHelp(create,state,{node,...}).
    constructor(create, state, hooks = {}) { if (typeof hooks.node !== 'function') throw new TypeError('Gump 도움말 노드 저장소 필요'); this.#create = create; this.#state = state; this.#hooks = hooks; }
    // 같은 현재 문자열 토큰 필드를 읽는다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node) throw new Error('Gump 도움말 토큰 없음'); return node; }
    // 선택된 메뉴/소리 경계만 그 호출 위치에서 요구한다.
    #need(name) { const device = this.#hooks[name]; if (typeof device !== 'function') throw new Error(`Gump 도움말 ${name} 경계 필요`); return device; }
    // 주제→메뉴 시작→현재 제목→현재 본문/사건→현재 루트 부착→소리 순서다.
    handle(token, event, first, second) {
        const node = this.#node(token); if ((event & HELP_EVENT_MASK) !== InputCode.rightButton || (event & InputCode.release) || !node.alternateTextPointer) return 0;
        const text = this.#need('text')(node.alternateTextPointer); const end = text.indexOf('\0'); this.#state.topic = text.substring(0, end === -1 ? text.length : end) + 'Help'; this.#need('beginMenu')(0); this.#need('title')(node.textPointer || node.alternateTextPointer);
        this.#need('entry')(this.#state.body, this.#state.event, this.#state.topic, 1, 1, 0); this.#need('attachMenu')(first | 0, second | 0, this.#create.entries[0]); this.#need('sound')('opengump.wav'); return 1;
    }
}

// 기본 ButtonGump가 거절한 사건만 실제 도움말로 보내는 fallback이다. help는 해당 훅보다 오래 살아야 한다.
export function bindGumpHelp(buttons, help) { buttons.baseEvent = (token, event, first, second) => help.handle(token, event, first, second); }
