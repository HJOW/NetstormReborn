// 원본 비용 표시 Gump 의 생성·부착·프레임 이동을 복원한다. 원본 클래스 이름은 미확정이다.
// (cpppj client/FloatingSpGump.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 가상 표/몸체는 추정하지 않는다)
// 별도 타이머/문자 출력/등록표를 만들지 않고 기존 원본 모듈(한 줄 TextGump·StyleText·등록표·수명·시계)을 조립한다. 모든 토큰은 같은 기본 노드를 가리키며 콜백이 필드를 바꿀 수 있으므로 이후 사용은 현재 저장소를 다시 조회한다.
// 원본 x87 의 double 시간 차이 저장과 float32 위치/속도 저장 순서를 유지한다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E434: RawMenuControls 선행 FloatingSpGump 이식)
import { TextGumpNode } from './text-gump.js';

// 원본 10.78 의 비용 표시 가상 표 식별값이며 호스트 함수 포인터로 쓰지 않는다. 생성 때 0xb8 바이트(TextGump 0xa4 + 4필드)를 확보한다.
const VTABLE = 0x0050688c; const NODE_BYTES = 0xb8;

// 기본 한 줄 TextGump 와 같은 저장소에 원본 추가 필드를 둔다. velocity/acceleration 은 +a4/+a8 의 프레임 이동량과 초당 이동량 변화(float32)이고 padding(+ac)은 생성자가 쓰지 않으며 stamp(+b0)는 직전 게임 시각(double)이다.
export class FloatingSpGumpNode extends TextGumpNode {
    // 프레임 이동량/초당 이동량 변화(float32 저장), 쓰이지 않는 패딩, 직전 게임 시각이다.
    velocity = 0; acceleration = 0; padding = 0; stamp = 0;
}

// 같은 실제 TextGump·등록표·시계·수명에 연결하는 비용 표시기다. 사용: new FloatingSpGump(createState, { node, now, allocate, constructText, clearText, fitText, styledText, idle, redraw, kill, attach, reference?, deleteText }).show(50, 100, 200)
export class FloatingSpGump {
    // 같은 현재 화면 크기와 루트를 가진 생성 상태와 실제 처리기/가상 효과다.
    #create; #hooks;
    // 필수 연결(reference 만 선택)이 하나라도 없으면 거부한다. 생성 상태와 안정된 노드/훅 저장소는 처리기보다 오래 살아야 한다.
    constructor(create, hooks = {}) {
        for (const name of ['node', 'now', 'allocate', 'constructText', 'clearText', 'fitText', 'styledText', 'idle', 'redraw', 'kill', 'attach', 'deleteText']) if (typeof hooks[name] !== 'function') throw new TypeError('비용 표시의 실제 Gump/시계 연결 필요');
        this.#create = create; this.#hooks = hooks;
    }

    // 실제 사용 시 현재 노드를 조회한다. 토큰 0 은 조회하지 않는다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node) throw new Error('비용 표시 실제 노드 없음');
        return node;
    }

    // 0043db70: 한 줄 생성→가상 표/시각/속도/가속도→원본 서식(글꼴 3·색·그림 명령 포함)/측정→idle 등록→현재 시각/깊이 순서다. 비용 차감은 이 표시기의 역할이 아니다.
    construct(token, cost, acceleration) {
        this.#node(token); this.#hooks.constructText(token); this.#node(token).vtable = VTABLE; this.#node(token).stamp = 0;
        this.#node(token).acceleration = Math.fround(acceleration); this.#node(token).velocity = 0; this.#hooks.clearText(token);
        this.#hooks.styledText(token, `~3~E~y${cost | 0}~[IfortGump.3]`); this.#hooks.fitText(token); this.#hooks.idle(token, true);
        const now = this.#hooks.now(); this.#node(token).stamp = now; this.#node(token).depth = 0xffffffd8; return token;
    }

    // 0043dd20: new(0xb8)→가속도 -2 생성→(선택) 객체 참조→실제 루트 Attach(mode 0)다. 일반 커서 배치는 참조 0 이고 참조가 있으면 위치는 (0,0)이다.
    show(cost, x, y, reference = 0) {
        const token = this.#hooks.allocate(NODE_BYTES) >>> 0; if (!token) throw new Error('비용 표시 할당 실패');
        this.construct(token, cost, -2);
        let px = Math.fround(x | 0); let py = Math.fround(y | 0);
        if (reference >>> 0) { if (typeof this.#hooks.reference !== 'function') throw new Error('비용 표시 실제 객체 참조 미연결'); this.#hooks.reference(token, reference >>> 0); px = 0; py = 0; }
        this.#hooks.attach(token, px, py, this.#create.collection.root, 0); return token;
    }

    // 0043dc50: double 시간 차이를 보관하고 이동 뒤 범위 검사/속도 갱신 또는 idle 해제/Kill 한다. 이탈 조건은 x87 C0/C2 parity(jnp)라 unordered(NaN)는 범위 이탈이 아니다 — `>=` 포함 조건으로 바꾸면 NaN 동작이 달라진다.
    idle(token) {
        const now = this.#hooks.now(); const elapsed = now - this.#node(token).stamp; this.#node(token).stamp = now; this.#hooks.redraw(token, false);
        const y = Math.fround(this.#node(token).velocity + this.#node(token).y); this.#node(token).y = y;
        if (y < 0 || this.#node(token).x < 0 || this.#create.screenWidth < this.#node(token).x || this.#create.screenHeight < y) {
            this.#hooks.idle(token, false); this.#hooks.kill(token); return;
        }
        this.#hooks.redraw(token, false); const change = this.#node(token).acceleration * elapsed;
        this.#node(token).velocity = Math.fround(change + this.#node(token).velocity);
    }

    // 0043e620→004d3080→004bc790: 기존 TextGump 소멸(같은 head 를 두 번 Clear 한 뒤 기본 수명/반납)과 하위 BYTE bit1 반납이다.
    deleteScalar(token, flags) { return this.#hooks.deleteText(token, flags); }
}
