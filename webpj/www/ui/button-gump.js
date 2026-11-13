// 원문 ButtonGump의 생성·상태·입력·통지·idle를 같은 공통 Gump 장치에 연결한다(cpppj client/ButtonGump.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E416)
import { GumpCreateNode, GUMP_REGISTRY_CAPACITY } from './gump-create.js';
import { GumpBackgroundFlags } from '../render/gump-visual.js';

// 호스트의 가상 분류이며 원본 객체에 새 필드를 직렬화하지 않는다.
export const ButtonGumpKind = Object.freeze({ BASE: 0, STONE: 1 });
// 버튼 입력 플래그와 공통 Gump 통지/팝업/삭제·사건 비트다.
const DISABLED = 1; const TOGGLE = 2; const INSTANT = 4; const PULSE = 0x10; const KEEP_POPUPS = 0x20;
const NOTIFY = 0x100; const POPUP = 0x4000; const DEAD = 0x10000; const EVENT_MASK = 0x03ffffff; const LEFT = 0x10000; const RIGHT = 0x20000; const RELEASE = 0x80000000;
// 원문 double 벽 시계 초와10.78 VT 식별값이다. VT를 함수 포인터로 실행하지 않는다.
const FIRST_DELAY = 0.4; const REPEAT_DELAY = 0.05; const BLINK_DELAY = 1; const PATCH_VTABLE = 0x503848;
// 도달한 실제 가상/장치 경계만 요구한다.
function need(fn) { if (typeof fn !== 'function') throw new TypeError('버튼 선택 가상/장치 경계 필요'); return fn; }

// 기본160BYTE와 같은 주소의 +a0..b8/+100 버튼 필드다. 미작성 파생 필드는 기본 생성에서 보존한다.
export class ButtonGumpNode extends GumpCreateNode {
    // DWORD 값·마우스 누름·점멸과 독립 버튼 플래그다.
    value = 0; pressed = 0; blink = 0; buttonFlags = 0;
    // 첫 반복과 다음 반복을 판정할 벽 시계 double 초다.
    firstDeadline = 0; repeatDeadline = 0;
    // 같은 트리의 실제 배경 토큰과 호스트 가상 분류다.
    background = 0; kind = ButtonGumpKind.BASE;
}

// 문자열 초기화가 공급하는 현재 사건ID 네 개와 signed 기본 색 BYTE다.
export class ButtonGumpState {
    // 값 요청/왼쪽 클릭/우클릭/반복 부모 통지의 현재ID다.
    valueEvent = 0; clickEvent = 0; rightEvent = 0; repeatEvent = 0;
    // 기본 생성 뒤 command로 부호 확장하는 팔레트 색이다.
    baseColor = 0;
}

// 별도 캡처/idle/월드를 만들지 않고 같은 생성·위치·이벤트·입력·수명을 빌린다.
export class ButtonGump {
    // 장기 현재 상태/공통 처리기와 실제 가상 장치다.
    #create; #state; #factory; #collection; #events; #input; #life; #hooks;
    // backgroundFlags(token)은 현재 DWORD의 {value} 읽기/쓰기 별칭을 돌려준다.
    constructor(create, state, factory, collection, events, input, lifetime, hooks = {}) { need(hooks.node); need(hooks.baseNode); this.#create = create; this.#state = state; this.#factory = factory; this.#collection = collection; this.#events = events; this.#input = input; this.#life = lifetime; this.#hooks = hooks; }
    // 안정된 실제 파생/기본 노드 주소가 같아야 한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node || this.#hooks.baseNode(token) !== node) throw new Error('버튼 Gump 토큰/공유 저장소 없음'); return node; }
    // 팝업 순회에는 버튼 외 기본 노드도 허용한다.
    #baseNode(token) { const node = token ? this.#hooks.baseNode(token) : null; if (!node) throw new Error('버튼 기본 Gump 토큰 없음'); return node; }
    // 원문은 매 호출마다 벽 시계를 다시 읽는다.
    #now() { return need(this.#hooks.now)(); }
    // 실제 픽셀 Draw 대신 가상 redraw(false)를 요청한다.
    #redraw(token) { need(this.#hooks.redraw)(token, false); }
    // 기본 생성/등록을 공유하고 int→float 저장과 double 배율 계산을 구별한다.
    constructBase(token, width, height, flags) { width |= 0; height |= 0; const n = this.#node(token); this.#factory.construct(token, 0, 0, 0, 1); n.vtable = PATCH_VTABLE; n.firstDeadline = 0; n.repeatDeadline = 0; n.command = ((this.#state.baseColor << 24) >> 24) >>> 0; n.flags = (n.flags | 0xa00) >>> 0; n.buttonFlags = flags >>> 0; n.value = n.pressed = n.blink = 0; n.kind = ButtonGumpKind.BASE; n.baseWidth = Math.fround(width); n.width = Math.fround(width * n.scale); n.baseHeight = Math.fround(height); n.height = Math.fround(height * n.scale); return token; }
    // 부모 redraw 뒤 현재 두 상태 OR가 바뀌면 현재 배경 플래그도 갱신한다.
    #writeState(token, value, pressed) { const n = this.#node(token); const old = !!(n.value || n.pressed); const kind = n.kind; n[pressed ? 'pressed' : 'value'] = value >>> 0; this.#redraw(token); const current = !!(n.value || n.pressed); if (kind === ButtonGumpKind.STONE && old !== current) { const flags = need(this.#hooks.backgroundFlags)(n.background); if (!flags) throw new Error('돌 버튼 배경 플래그 없음'); flags.value = (current ? flags.value | GumpBackgroundFlags.PRESSED : flags.value & ~GumpBackgroundFlags.PRESSED) >>> 0; this.#redraw(n.background); } }
    // 동일한 값도 원문처럼 부모 redraw를 호출한다.
    setValue(token, value) { this.#writeState(token, value, false); }
    // 전체DWORD 누름 값을 bool로 좁히지 않는다.
    setPressed(token, value) { this.#writeState(token, value, true); }
    // 비활성 비트만 변경하며 캡처/idle를 해제하지 않는다.
    setEnabled(token, enabled) { const n = this.#node(token); n.buttonFlags = (enabled ? n.buttonFlags & ~DISABLED : n.buttonFlags | DISABLED) >>> 0; this.#redraw(token); }
    // 점멸 지정은 redraw를 추가하지 않는다.
    setBlink(token, value) { this.#node(token).blink = value >>> 0; }
    // pulse bit를 먼저 쓰고 같은 실제 idle 표를 갱신한다.
    setPulse(token, enabled) { const n = this.#node(token); n.buttonFlags = (enabled ? n.buttonFlags | PULSE : n.buttonFlags & ~PULSE) >>> 0; this.#life.setIdle(token, enabled); }
    // ordered 비교로 NaN/음수는 제외하고 -0은 허용한다.
    contains(token, local) { const n = this.#node(token); return local[0] >= 0 && local[0] < n.width && local[1] >= 0 && local[1] < n.height; }
    // 파생 가상 setter가 있으면 전체DWORD 인자를 그대로 전달한다.
    #value(token, value) { if (this.#hooks.value) this.#hooks.value(token, value); else this.setValue(token, value); }
    // 누름 가상 setter의 기본 동작은 같은 실제 상태 쓰기다.
    #pressed(token, value) { if (this.#hooks.pressed) this.#hooks.pressed(token, value); else this.setPressed(token, value); }
    // 기본 pulse만 bool로 해석하고 파생에는 전체DWORD를 준다.
    #pulse(token, value) { if (this.#hooks.pulse) this.#hooks.pulse(token, value); else this.setPulse(token, value !== 0); }
    // 점멸 가상 setter에도 redraw를 삽입하지 않는다.
    #blink(token, value) { if (this.#hooks.blink) this.#hooks.blink(token, value); else this.setBlink(token, value); }
    // command 대신 현재 serial로 실제 부모 체인에 통지한다.
    #notify(token, event, value) { this.#events.bubble(token, event, this.#node(token).serial, value); }
    // RHS 시계 조회 뒤 현재 Node를 다시 읽고 서로 다른 두 시각을 저장한다.
    #resetDeadlines(token) { const first = this.#now() + FIRST_DELAY; this.#node(token).firstDeadline = first; const repeat = this.#now() + REPEAT_DELAY; this.#node(token).repeatDeadline = repeat; }
    // 루트 직계의 호출 전 next를 따라 현재 팝업에 실제 Kill만 요청한다.
    #closePopups() { const seen = new Set();
        // visible/void는 검사하지 않고 이미 dead인 팝업만 제외한다.
        for (let token = this.#baseNode(this.#create.entries[0]).child; token;) { if (seen.size >= GUMP_REGISTRY_CAPACITY || seen.has(token)) throw new Error('버튼 팝업 형제 순환/한도 초과'); seen.add(token); const n = this.#baseNode(token); const next = n.next; if ((n.flags & POPUP) && !(n.flags & DEAD)) this.#life.kill(token); token = next; }
    }
    // 값 명령 뒤에도 마우스/기본 도움말 분기를 계속하며 콜백 뒤 현재 필드를 읽는다.
    handle(token, event, first, second) {
        event >>>= 0; first >>>= 0; second >>>= 0; const n = this.#node(token); if (n.buttonFlags & DISABLED) return 0; const code = event & EVENT_MASK; const release = !!(event & RELEASE);
        if (code === this.#state.valueEvent && first === n.serial) { this.#value(token, second); this.#redraw(token); this.#notify(token, this.#state.clickEvent, n.value); }
        if (!n.alternateTextPointer && code === RIGHT && !release) { if (!(n.buttonFlags & KEEP_POPUPS)) this.#closePopups(); this.#notify(token, this.#state.rightEvent, n.value); return 1; }
        if (code === LEFT && !release) {
            if (!(n.buttonFlags & KEEP_POPUPS)) this.#closePopups();
            if (!(n.buttonFlags & INSTANT)) { this.#resetDeadlines(token); this.#input.capture(token); n.flags = (n.flags | NOTIFY) >>> 0; this.#life.setIdle(token, true); this.#pressed(token, 1); }
            else if (n.buttonFlags & TOGGLE) { this.#value(token, n.value === 0 ? 1 : 0); this.#notify(token, this.#state.clickEvent, n.value); }
            else if (!n.value) { this.#value(token, 1); this.#notify(token, this.#state.clickEvent, n.value); }
            need(this.#hooks.sound)(); this.#redraw(token); return 1;
        }
        if (code === LEFT && release && !(n.buttonFlags & INSTANT)) {
            const local = this.#collection.screenToLocal(token, { x: first | 0, y: second | 0 });
            if (!this.contains(token, local)) { n.flags = (n.flags & ~NOTIFY) >>> 0; this.#life.setIdle(token, false); this.#pressed(token, 0); return 1; }
            if (n.buttonFlags & TOGGLE) { this.#value(token, n.value === 0 ? 1 : 0); this.#notify(token, this.#state.clickEvent, n.value); } else if (n.pressed) this.#notify(token, this.#state.clickEvent, n.value);
            n.flags = (n.flags & ~NOTIFY) >>> 0; this.#life.setIdle(token, false); this.#pressed(token, 0); this.#redraw(token); return 1;
        }
        return need(this.#hooks.baseEvent)(token, event, first, second) >>> 0;
    }
    // 폴링 release와 전달 좌표를 구별하며 비활성은 추가 필터가 아니다.
    broadcast(token, event, first, second) { const n = this.#node(token); if (n.buttonFlags & INSTANT) throw new Error('즉시 버튼은 원본 통지 대상이 아님'); if (need(this.#hooks.poll)(LEFT).code & RELEASE) n.flags = (n.flags & ~NOTIFY) >>> 0; void event; const local = this.#collection.screenToLocal(token, { x: first | 0, y: second | 0 }); const inside = this.contains(token, local); const value = n.buttonFlags & TOGGLE ? (inside ? Number(n.value === 0) : n.value) : Number(inside); if (value !== n.pressed) { this.#pressed(token, value); this.#resetDeadlines(token); this.#redraw(token); } }
    // 첫 시각과 반복 시각이 각각 만료했을 때 한 번만 통지한다. 지연 프레임을 보충하지 않는다.
    idle(token) { const n = this.#node(token); if (n.buttonFlags & PULSE) { if (n.value || n.pressed) this.#pulse(token, 0); else { const now = this.#now(); if (n.firstDeadline <= now) { this.#blink(token, n.blink === 0 ? 1 : 0); n.firstDeadline = this.#now() + BLINK_DELAY; } } } if (!(n.buttonFlags & INSTANT) && n.pressed) { const first = this.#now(); if (n.firstDeadline <= first) { const repeat = this.#now(); if (n.repeatDeadline <= repeat) { this.#notify(token, this.#state.repeatEvent, 1); n.repeatDeadline = this.#now() + REPEAT_DELAY; } } } }
    // Bounds 콜백 뒤 현재 값/누름을 signedDWORD 보기로 기존 출력 계획에 전달한다.
    visualState(token, textWidth, textHeight) { const rect = this.#collection.bounds(token); const n = this.#node(token); return { rect, textWidth: textWidth | 0, textHeight: textHeight | 0, value: n.value | 0, pressed: n.pressed | 0, flags: n.buttonFlags >>> 0 }; }
}

// 선택/캡처·idle/통지의 기존 fallback을 각각 보존하고 호출 때 현재 버튼 처리기를 고른다.
export function bindButtonGump(events, input, frame, resolve) {
    need(resolve);
    // 이벤트와 캡처는 서로 다른 기존 처리기를 유지한다.
    const bindHandle = (previous) => (token, event, first, second) => { const button = resolve(token); return button ? button.handle(token, event, first, second) : need(previous)(token, event, first, second); };
    events.handle = bindHandle(events.handle); input.handle = bindHandle(input.handle); const idle = frame.idle; const broadcast = frame.broadcast;
    // 표 순회/현재값 필터는 공통 프레임이 담당한다.
    frame.idle = (token) => { const button = resolve(token); if (button) button.idle(token); else need(idle)(token); };
    // 원문 전체 사건 인자를 실제 버튼에 전달한다.
    frame.broadcast = (token, event, first, second) => { const button = resolve(token); if (button) button.broadcast(token, event, first, second); else need(broadcast)(token, event, first, second); };
}
