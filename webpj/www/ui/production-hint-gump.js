// 생산 실패 안내의 원본 TextGump 파생과 두 번역 wrapper 를 복원한다(10.78 의 4728d0/4729d0/472a00/472a50/472be0/472d20 처리기).
// (cpppj client/ProductionHintGump.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 주소/몸체는 추정하지 않는다)
// 기존 TextGump/StyleText/수집/수명/시계에만 위임하는 실제 경계다. x86 의 부호 있는 DWORD 감김과 float32 저장 경계를 보존한다. 콜백 뒤에도 현재 저장소를 재조회한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E434: RawMenuControls 선행 ProductionHintGump 이식)
import { TextGumpNode } from './text-gump.js';

// 원본 10.78 안내 파생 가상 표 식별값, PE 506588 의 벽시계 수명(초), 0xb8 바이트 노드 크기다.
const VTABLE = 0x50c2b4; const LIFETIME = 4; const NODE_BYTES = 0xb8;
// 안내 창에 표시하는 두 원문(번역 wrapper 입력)이다.
const EMPTY_TEXT = '~0~w~ENone in inventory'; const POOR_TEXT = '~0~w~ENot enough Storm Power to build this';
// signed DWORD 덧셈의 하위 32비트 결과다.
const add = (value, amount) => (value + amount) | 0;
// float32 의 비트 패턴(DWORD)이다.
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }

// +a4 는 미작성이고 +a8/+b0/+b4 는 벽시각 만료/소유 문자열/기준 노드다.
export class ProductionHintNode extends TextGumpNode {
    // 생성자가 덮어쓰지 않는 +a4, 게임 속도와 독립인 벽시계 만료 시각(double), 복제 문자열의 소유 토큰, 기준 Gump 토큰이다.
    reserved = 0; until = 0; owned = 0; reference = 0;
}

// 원본 현재 팔레트/종류 BYTE/마지막 생성 일련번호를 공유한다(540434 의 초기 종류 25, 5a4058/5bec2c/592988 의 현재 값).
export class ProductionHintState {
    // 종류 BYTE(signed)와 현재 흰색/검정 색 번호, 마지막으로 만든 안내의 serial 이다.
    kind = 25; white = 7; black = 0; lastSerial = 0;
}

// 생산 실패 안내 처리기다. 사용: new ProductionHintGump(createState, hintState, { node, now, allocate, constructText, kill, duplicate, string, free, translate, style, fit, idle, bounds, position, draw, attach, deleteText }).show(false, 10, 20, referenceToken)
export class ProductionHintGump {
    // 같은 등록/전역을 빌리고 실제 공통 경계를 빠짐없이 요구한다(CD판 주소를 추정하지 않는다).
    #create; #state; #hooks;
    constructor(create, state, hooks = {}) {
        for (const name of ['node', 'now', 'allocate', 'constructText', 'kill', 'duplicate', 'string', 'free', 'translate', 'style', 'fit', 'idle', 'bounds', 'position', 'draw', 'attach', 'deleteText']) if (typeof hooks[name] !== 'function') throw new TypeError('생산 안내 실제 경계 누락');
        this.#create = create; this.#state = state; this.#hooks = hooks;
    }

    // callback 뒤에도 현재 저장소를 재조회한다.
    #node(token) { const node = this.#hooks.node(token); if (!node) throw new Error('생산 안내 노드 없음'); return node; }

    // 원본 기본 생성/소유권/파싱/측정/idle/시계 순서다. 문자열을 복제해 소유하고 4초 뒤 만료를 설정한다(PE 506588).
    construct(token, text) {
        this.#hooks.constructText(token); this.#node(token).vtable = VTABLE; this.#node(token).until = 0; this.#node(token).command = (this.#state.kind << 24 >> 24) >>> 0;
        this.#node(token).textContext.set([0, 6, this.#state.white, 0, 0, this.#state.black, 1, 1]); this.#node(token).reference = 0;
        const owned = this.#hooks.duplicate(text) >>> 0; this.#node(token).owned = owned;
        this.#hooks.style(token, this.#hooks.string(this.#node(token).owned)); this.#hooks.fit(token, 6, 4); this.#hooks.idle(token, true);
        const until = this.#hooks.now() + LIFETIME; this.#node(token).until = until; return token;
    }

    // 두 원본 실패 안내 wrapper 다(472a00/472a50). 기준 영역은 입력 직사각형으로 저장하며 실제 표시 위치는 입력 y+20 이다. 번역은 wrapper 위치에서 한다.
    show(empty, x, y, reference) {
        const token = this.#hooks.allocate(NODE_BYTES) >>> 0; if (!token) throw new Error('생산 안내 할당 실패');
        this.construct(token, this.#hooks.translate(empty ? EMPTY_TEXT : POOR_TEXT)); this.#node(token).flags = (this.#node(token).flags | 0x4000) >>> 0;
        const rect = this.#hooks.bounds(reference); const node = this.#node(token);
        node.positioning[2] = floatBits(rect.left); node.positioning[3] = floatBits(rect.top); node.positioning[4] = floatBits(rect.right); node.positioning[5] = floatBits(rect.bottom);
        node.flags = (node.flags | 0x8100) >>> 0; node.depth = 0xffffff9c; node.reference = reference >>> 0;
        this.#hooks.attach(token, Math.fround(x | 0), Math.fround(add(y | 0, 20)), this.#create.collection.root, 0); this.#state.lastSerial = this.#node(token).serial; return token;
    }

    // 만료 시각이 지났거나 같으면 같은 지연 삭제를 예약한다. unordered(NaN) 비교는 만료시키지 않는다.
    idle(token) { if (this.#node(token).until <= this.#hooks.now()) this.#hooks.kill(token); }

    // 빈 head 에서도 StyleText 경계를 호출한다. 경계/위치를 읽고 (3,2) 여백으로 출력한다.
    draw(token) {
        this.#hooks.bounds(token); const point = this.#hooks.position(token);
        this.#hooks.draw(this.#node(token).head, { x: add(point.x, 3), y: add(point.y, 2) });
    }

    // 문자열 다음 TextGump/객체 소유권 순서다. free 뒤 소유 필드를 0 으로 덮어쓰지 않는 원본 소멸 순서를 지킨다.
    deleteScalar(token, flags) { this.#node(token).vtable = VTABLE; if (this.#node(token).owned) this.#hooks.free(this.#node(token).owned); return this.#hooks.deleteText(token, flags); }
}
