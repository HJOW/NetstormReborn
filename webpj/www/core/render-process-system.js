// 서로 다른 입자/표시 효과가 하나의 원본 표시 목록·토큰·수집/분배 경계를 공유하게 하는 모듈이다(cpppj `o/RenderProcessSystem.cpp/.h`, 10.78 전용: 00487960 Form 삭제 통지 영역).
//  - renderProcessRemovalBounds(fields, cameraX, cameraY): PATCH 00487960 의 Form 삭제 dirty 영역 [left, top, right, bottom] 이다. 각 변을 먼저 정수화한 뒤 카메라/1픽셀 여유를 적용하며,
//    top 만 카메라 감산 뒤 float 에 저장하지 않는 실제 x87 명령 순서를 따른다(일반 프레임 dirty 와 다름). 정상 signed32 범위 밖은 거부한다.
//  - new RenderProcessSystem(pool, host, state, hooks): pool/host(SquidProcessHost, 같은 풀)/state(`createConstructionVisualState()` 의 현재 16/11 배율)는 이 객체보다 오래 살아야 한다.
//    hooks = { reserve(바이트)→성공 여부, releaseArray() } 는 원본 표시 목록의 확보/반납 경계이며, 부착(attach)은 이 객체가 토큰별 실제 Host 부착 callback 으로 바꿔 넣는다.
//    Kernel 소유권은 각 효과 Host 가 유지하고 이 저장소는 등록/출력 경계만 공유한다. 효과 시스템들은 이 저장소보다 먼저 정리한다.
//  - track(fields, attach, draw)→token: 원본 등록 전에 비영 토큰과 생성 중 조회 항목을 확보한다(attach 는 실제 Host 부착, draw 는 현재 카메라/부모를 읽는 파생 출력).
//    ready(token): 파생 초기화가 끝난 뒤에만 수집/출력을 허용한다. forget(token): 효과 소멸자가 버퍼/등록을 정리한 뒤 조회 항목을 지운다(미등록 토큰은 무시).
//  - draw(): 같은 원본 목록의 슬롯 순서로 초기화 완료 효과만 출력한다. drawToken(token): 공통 kind3 분배의 현재 토큰 출력(수집 뒤 삭제된 항목은 생략).
//    collect(view, output, limit): 기존 원본 수집기(`RawRenderCollection`)로 모든 효과를 한 번에 수집한다(생성 중 항목은 건너뜀).
//  - registry/slots/size/lookup(token)/pool/host/state: 생성/삭제 몸체의 단일 registry, 현재 슬롯 사본, 생성 중 포함 조회 수, 초기화 완료 토큰의 현재 등록 필드, 같은 월드 확인용 참조이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E192: 공통 표시 저장소 이식으로 추가)
import { RawRenderRegistry } from './raw-construction-lifetime.js';
import { RawRenderCollection } from './raw-render-collection.js';

// 정상 signed32 범위 경계와 토큰 감김 경계이다. (2026-11-09 WB04-E192)
const TWO_31 = 2147483648;

// 원본 CRT 의 유한/정상 정수 범위만 0 방향으로 절삭한다. (2026-11-09 WB04-E192)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_31 || value >= TWO_31) throw new RangeError('표시 삭제 영역 정수화 범위 오류');
    return Math.trunc(value) | 0;
}

// top 만 카메라 감산 뒤 float 에 저장하지 않는 실제 x87 명령 순서를 따른다. 원본: 00487960. (2026-11-09 WB04-E192)
export function renderProcessRemovalBounds(fields, cameraX, cameraY) {
    const left = ftol(fields.left);
    const top = ftol(fields.top);
    const right = ftol(fields.right);
    const bottom = ftol(fields.bottom);
    const storedLeft = Math.fround(left - cameraX);
    const storedRight = Math.fround(Math.fround(right) - cameraX);
    const storedBottom = Math.fround(Math.fround(bottom) - cameraY);
    return [ftol(storedLeft - 1), ftol((top - cameraY) - 1), ftol(storedRight + 1), ftol(storedBottom + 1)];
}

// 효과별 콜백의 조회 수명을 원본 단일 표시 목록과 수집기에 연결하는 공통 표시 저장소이다. (2026-11-09 WB04-E192)
export class RenderProcessSystem {
    #pool;
    #host;
    #state;
    #registry;
    // 비영 토큰 → { fields, attach, draw, ready } 이다(객체 수명은 호출자가 유지).
    #bindings = new Map();
    // 모든 효과가 공유하는 다음 식별자이며 32비트 감김은 거부한다.
    #nextToken = 1;

    // 메모리 확보/반납은 기존 경계를 보존하고 실제 부착만 토큰별로 분배한다. Host 와 풀이 다르면 거부한다. (2026-11-09 WB04-E192)
    constructor(pool, host, state, hooks) {
        if (!host || host.pool !== pool) throw new TypeError('공통 표시 Host/풀 불일치');
        if (!state) throw new TypeError('공통 표시 배율 상태 누락');
        this.#pool = pool;
        this.#host = host;
        this.#state = state;
        // 원본 registry 의 attach 만 현재 토큰에 등록한 실제 Host 콜백으로 바꾼다.
        this.#registry = new RawRenderRegistry(pool, {
            reserve: hooks?.reserve,
            releaseArray: hooks?.releaseArray,
            attach: (fields, type, parent, flags) => {
                const found = this.#bindings.get(fields.token);
                if (!found || found.fields !== fields) throw new Error('공통 표시 부착 토큰 불일치');
                // 부착이 자기 항목을 지워도 이번 호출의 callable 은 유지한다.
                const attach = found.attach;
                attach(fields, type, parent, flags);
            }
        });
    }

    // 조회 항목 확보가 실패하면 필드/번호를 바꾸지 않는다. 실제 슬롯 등록은 각 Raw 생성자가 수행한다. (2026-11-09 WB04-E192)
    track(fields, attach, draw) {
        if (typeof attach !== 'function' || typeof draw !== 'function' || fields.slot >= 0 || this.#bindings.has(fields.token)) throw new TypeError('공통 표시 조회/출력 경계 오류');
        if (!this.#nextToken) throw new RangeError('공통 표시 토큰 소진');
        const token = this.#nextToken;
        this.#bindings.set(token, { fields, attach, draw, ready: false });
        fields.token = token;
        this.#nextToken = (this.#nextToken + 1) >>> 0;
        return token;
    }

    // 원본 등록이 아직 끝나지 않은 항목을 활성화하는 호출 실수는 거부한다. (2026-11-09 WB04-E192)
    ready(token) {
        const found = this.#bindings.get(token);
        if (!found || found.fields.slot < 0) throw new Error('공통 표시 초기화 완료 토큰 오류');
        found.ready = true;
    }

    // 실제 버퍼/등록 반납은 효과 소멸자가 수행하므로 여기서는 조회만 없앤다. (2026-11-09 WB04-E192)
    forget(token) {
        this.#bindings.delete(token);
    }

    // 출력 중 신규 등록으로 목록이 확장될 수 있으므로 매 차례 현재 슬롯을 다시 읽는다(종류별 분리 없는 하나의 원본 순서). (2026-11-09 WB04-E192)
    draw() {
        for (let slot = 0; slot < this.#registry.slots.length; slot += 1) this.drawToken(this.#registry.slots[slot]);
    }

    // 출력 콜백을 먼저 꺼내 자기 삭제로 조회 항목이 없어져도 이번 호출은 유지한다. (2026-11-09 WB04-E192)
    drawToken(token) {
        const found = this.#bindings.get(token);
        if (!found || !found.ready) return;
        const draw = found.draw;
        draw();
    }

    // 생성 중 파생 필드를 읽지 않는 조회를 기존 수집기의 확장 계약에 공급한다. view = { left, top, right, bottom }, output 은 16바이트 항목 배열(Uint8Array)이다. (2026-11-09 WB04-E192)
    collect(view, output, limit) {
        const collection = new RawRenderCollection(this.#pool, this.#state, this.#registry, (token) => this.lookup(token));
        return collection.collect(view, output, limit);
    }

    // 생성/삭제는 이 registry 만 사용한다. (2026-11-09 WB04-E192)
    get registry() { return this.#registry; }

    // 현재 원본 등록 배열의 사본이다(구멍·40칸 확장을 그대로 관찰). (2026-11-09 WB04-E192)
    get slots() { return this.#registry.slots; }

    // 생성 중인 항목도 포함하는 현재 조회 수이다. (2026-11-09 WB04-E192)
    get size() { return this.#bindings.size; }

    // 초기화 완료 토큰의 현재 등록 필드이다. 생성 중이거나 삭제된 토큰은 null 이며 삭제 뒤 보관하지 않는다. (2026-11-09 WB04-E192)
    lookup(token) {
        const found = this.#bindings.get(token);
        return found && found.ready ? found.fields : null;
    }

    // 효과가 같은 월드·Host·좌표 배율을 쓰는지 검증할 참조이다. (2026-11-09 WB04-E192)
    get pool() { return this.#pool; }
    get host() { return this.#host; }
    get state() { return this.#state; }
}
