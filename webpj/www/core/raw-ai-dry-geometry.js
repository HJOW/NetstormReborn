// AI 가짜 부족(dry) 좌표의 원본 선분/수직 양방향 탐색과 디버그 표식 수명을 옮긴 모듈이다(cpppj `o/RawAiDryGeometry.cpp/.h`, 10.78: 패치 00415bb0·00414d30·00410540·0041cfc0).
//  - aiPointDistance(first, second): 0041cfc0 — 두 float 좌표 차이의 제곱합/제곱근을 중간 float 저장 없이(배정도) 반환한다.
//  - new RawAiDryGeometry(pool, globals, hooks): globals = { bounds{left, top, right, bottom}(00544d14..20 의 현재 네 float, 양끝 포함), debug(0052f3dc, 29 일 때만 실제 표식을 만든다), markerType(005411f8 의 현재 TYPE, 초기 104) } 이며
//    매 위치에서 현재 값을 다시 읽도록 같은 객체를 빌린다(복사하지 않는다). hooks = { surface(x, y)→WORD(00542514 의 256폭 지도, 범위 밖은 호출하지 않음), create(type, flags)→SID(004af530),
//    word(sid, word)(004ac220 kind 93 쓰기·다리 통지), pop(sid, point, flags)(가상 +90), destroy(sid, flags)(가상 +10)} 이며 도달한 경계가 없으면 앞서 수행한 효과를 유지한 채 그 위치에서 거부한다.
//  - candidate(source, target): 00415bb0 — (디버그면 표식 정리 후 두 끝점 표시) 선분을 float 누적으로 방문(디버그면 각 점 표시)한 뒤, 중점에서 수직 양방향(왼쪽 먼저)으로 이동하며 각 방향의 연속 빈칸이 4 가 될 때까지 반복해
//    왼쪽이 4 이면 왼쪽(동률도), 오른쪽만 4 이면 오른쪽 좌표를 돌려준다. 원본의 반복 예산 부재도 유지한다(경계 안에서만 이동하므로 경계 밖에서는 멈춘다).
//  - clearMarkers(): 00414d30 — SID 1 을 건너뛰어 free 비트만 건너뛰는(dead 는 방문) iterator 로, 다음을 먼저 고른 뒤 객체 TYPE BYTE 가 현재 markerType 전체 DWORD 와 같고 kind WORD 가 93 인 것만 가상 삭제한다.
//    mark(point): 00410540 — 현재 TYPE/flags 2 생성 → kind 93 쓰기 → 실제 Pop(flags 0).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E163: AI 가짜 부족 좌표 탐색 이식으로 추가)

// 0.9999f 편향(00500edc, float 비트 3f7ff972)과 중점 상수, 활성 디버그 종류, 표식 구분 WORD, 지도 한 변, 방향별 필요한 연속 빈칸 수이다. (2026-11-09 WB04-E163)
const CELL_BIAS = Math.fround(0.9999);
const HALF = 0.5;
const DEBUG = 29;
const MARKER_WORD = 93;
const SIDE = 256;
const RUN = 4;
const TWO_63 = 9223372036854775808;

// CRT signed64 절삭의 하위 DWORD 이며 잘못된 FISTP 의 하위값은 0 이다. (2026-11-09 WB04-E163)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 도달한 실제 효과가 없으면 앞서 수행한 효과를 유지하고 그 위치에서 거부한다. (2026-11-09 WB04-E163)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 가짜좌표 탐색의 실제 하위 미연결');
    return hook;
}

// 두 축 차이를 중간 float 로 줄이지 않고 제곱근까지 계산한다. (2026-11-09 WB04-E163)
export function aiPointDistance(first, second) {
    const dy = first.y - second.y;
    const dx = first.x - second.x;
    return Math.sqrt(dy * dy + dx * dx);
}

// AI 가짜 부족 좌표 탐색기이다. 별도 지도나 후보 사본을 만들지 않고 같은 실제 풀/경계/현재 효과를 사용한다. (2026-11-09 WB04-E163)
export class RawAiDryGeometry {
    #pool;
    #globals;
    #hooks;

    // 생성할 때 지도를 복사하거나 표식을 만들지 않는다. PATCH WORD 풀(용량 65536 이하)만 받는다. (2026-11-09 WB04-E163)
    constructor(pool, globals, hooks) {
        if (pool?.edition !== 'patch1078' || pool.capacity > 65536) throw new TypeError('AI 가짜좌표 탐색의 PATCH WORD 풀 누락');
        if (!globals?.bounds) throw new TypeError('AI 가짜좌표 탐색의 전역 경계 누락');
        this.#pool = pool;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E163)
    get pool() { return this.#pool; }

    // 현재 경계의 less/equal 만 허용하는 원본 parity 분기를 일반 ordered 비교로 표현한다(매 위치의 현재 네 경계, NaN 제외). (2026-11-09 WB04-E163)
    #inside(point) {
        const bounds = this.#globals.bounds;
        return bounds.left <= point.x && point.x <= bounds.right && bounds.top <= point.y && point.y <= bounds.bottom;
    }

    // 각 축의 편향 합은 ftol 전에 float 로 저장하지 않는다. 지도 밖은 빈칸으로 계산하고 그 경우 지도 효과를 요청하지 않는다. (2026-11-09 WB04-E163)
    #empty(point) {
        const x = ftol(point.x + CELL_BIAS);
        const y = ftol(point.y + CELL_BIAS);
        return x < 0 || y < 0 || x >= SIDE || y >= SIDE || !need(this.#hooks.surface)(x, y);
    }

    // 00414d30: 원본은 SID 1 에서 Next 를 먼저 호출하며 삭제 전에 다음 SID 를 미리 잡는다. 원본 iterator 는 free 비트만 건너뛰고 dead 객체는 방문한다. (2026-11-09 WB04-E163)
    clearMarkers() {
        const pool = this.#pool;
        const end = pool.capacity;
        let cursor = 1;
        const next = () => {
            cursor += 1;
            while (cursor < end && (pool.slot(cursor)[11] & 1) !== 0) cursor += 1;
        };
        next();
        // 가상 삭제 효과 뒤 현재 TYPE 전역/슬롯을 읽으며 논리 순서를 바꾸지 않는다.
        while (cursor >= 1 && cursor < end) {
            const sid = cursor;
            next();
            const raw = pool.slot(sid);
            const word = raw[12] | (raw[13] << 8);
            if (raw[10] === (this.#globals.markerType >>> 0) && word === MARKER_WORD) need(this.#hooks.destroy)(sid, 0);
        }
    }

    // 00410540: 생성/WORD/Pop 경계가 실패해도 원본처럼 앞선 생성이나 쓰기를 취소하지 않는다. (2026-11-09 WB04-E163)
    mark(point) {
        const sid = need(this.#hooks.create)(this.#globals.markerType, 2);
        need(this.#hooks.word)(sid, MARKER_WORD);
        need(this.#hooks.pop)(sid, point, 0);
    }

    // 00415bb0: 진행이 경계에서 정지하는 원본 경로에 임의 성공/예산/대체 좌표를 넣지 않는다. (2026-11-09 WB04-E163)
    candidate(source, target) {
        const globals = this.#globals;
        // 입력 좌표는 원본 float 이므로 단정도로 맞춘다.
        source = { x: Math.fround(source.x), y: Math.fround(source.y) };
        target = { x: Math.fround(target.x), y: Math.fround(target.y) };
        if (globals.debug === DEBUG) {
            this.clearMarkers();
            this.mark(source);
            this.mark(target);
        }
        const distance = Math.fround(aiPointDistance(target, source));
        const inverse = 1 / distance;
        const dx = Math.fround((target.x - source.x) * inverse);
        const dy = Math.fround((target.y - source.y) * inverse);
        const middle = { x: Math.fround((target.x + source.x) * HALF), y: Math.fround((target.y + source.y) * HALF) };
        const along = { x: source.x, y: source.y };
        let progress = 0;
        // 디버그가 꺼져도 선분의 float 누적/횟수 진행은 원래 순서대로 수행한다.
        while (progress < distance) {
            along.x = Math.fround(along.x + dx);
            along.y = Math.fround(along.y + dy);
            if (globals.debug === DEBUG) this.mark({ x: along.x, y: along.y });
            const advanced = progress + 1;
            progress = Math.fround(advanced);
            if (!(advanced < distance)) break;
        }
        const left = { x: middle.x, y: middle.y };
        const right = { x: middle.x, y: middle.y };
        let leftRun = 0;
        let rightRun = 0;
        // 왼쪽을 먼저 움직이고 오른쪽도 같은 반복에서 검사한 뒤 왼쪽 성공을 우선 반환한다.
        for (;;) {
            if (rightRun >= RUN) return leftRun >= RUN ? left : right;
            if (this.#inside(left)) {
                left.x = Math.fround(left.x - dy);
                left.y = Math.fround(left.y + dx);
                leftRun = this.#empty(left) ? leftRun + 1 : 0;
            }
            if (this.#inside(right)) {
                right.x = Math.fround(right.x + dy);
                right.y = Math.fround(right.y - dx);
                rightRun = this.#empty(right) ? rightRun + 1 : 0;
            }
            if (globals.debug === DEBUG) {
                this.mark({ x: left.x, y: left.y });
                this.mark({ x: right.x, y: right.y });
            }
            if (leftRun >= RUN) return left;
        }
    }
}
