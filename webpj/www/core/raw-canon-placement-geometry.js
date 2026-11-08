// 일반 타입과 특수 패턴의 배치 모양 순회와 실제 finder 사각형을 공유하는 모듈이다(cpppj `o/RawCanonPlacementGeometry.cpp/.h`, 10.78). 픽셀 getter 와 별개인 배치의 정수 범위/칸별 충돌 범위를 복원한다.
//  - bounds(query): 타입별 Canon 디코더(raw-canon-type.js)를 만들어 첫 현재 좌표와 전체 순회 개수×현재 발자국으로 미리보기 범위 {left, top, right, bottom} 을 구한다(빈 디코더에서도 계산한다).
//  - inspect(query, scan, hooks): 디코더 생성 뒤 beginRegions 를 한 번 부르고, 유효 칸마다 [칸 정보 cell = {frame, label, side(조회 가능한 프레임 코드의 방향 글자, 없으면 0), x, y, snappedX, snappedY(0 방향 절삭 float),
//    area(CollisionArea)}] → (선택) beginShape → scan(cell) → endShape(query, cell) 을 수행하고 하나라도 거짓이면 즉시 거짓을 돌려준다(최종 판정을 생략). 모든 칸이 끝나면(빈 디코더도) finishRegions(query) 의 결과를 돌려준다.
//    현재 발자국은 칸마다 다시 읽고 디코더 생성 때의 첫 프레임/패턴 선택은 유지한다. 요청은 값으로 복사해 훅이 바꿔도 처음 값을 유지한다.
//  - CollisionArea(x, y, footX, footY): 0049b8c0..0049b95a 의 실제 finder 인자 산술이다. 왼쪽 위는 절삭한 좌표에서 (발자국−1)을 뺀 정수, 오른쪽 아래는 원래 좌표에 0.99999f 를 더해 절삭한다. 발자국이 음수이거나 좌표가 비유한이면 거부한다.
// 10.72(CD)의 snap/CRT 저장 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E146: 일반 배치 모양 기하 이식으로 추가)
import { decodeCanonType } from './raw-canon-type.js';

// 단정도 올림 보정값(0.99999f)과 int32 범위, 타입 표 크기이다. (2026-11-09 WB04-E146)
const NEAR_CEIL = Math.fround(0.99999);
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;
const TYPE_COUNT = 188;

// 원본 CRT 의 범위 밖/비유한 변환을 정의되지 않은 동작으로 보내지 않고 거부한다. (2026-11-09 WB04-E146)
function truncate(value) {
    const whole = Math.trunc(value);
    if (!Number.isFinite(whole) || whole < INT_MIN || whole > INT_MAX) throw new RangeError('배치 모양 좌표 절삭 범위 오류');
    return whole | 0;
}

// 0049b8c0..0049b95a: 왼쪽 위는 snapped 좌표, 오른쪽 아래는 원래 소수 좌표에 넓은 정밀도로 보정을 더한다. 0 발자국(Dude)도 계산한다. (2026-11-09 WB04-E146)
export function canonCollisionArea(x, y, footX, footY) {
    if (footX < 0 || footY < 0) throw new RangeError('배치 모양 발자국 오류');
    x = Math.fround(x);
    y = Math.fround(y);
    const snappedX = Math.fround(truncate(x));
    const snappedY = Math.fround(truncate(y));
    return { left: truncate(snappedX - (footX - 1)), top: truncate(snappedY - (footY - 1)), right: truncate(x + NEAR_CEIL), bottom: truncate(y + NEAR_CEIL) };
}

// 일반 타입·패턴 배치의 모양 순회 계층이다. pool(10.78 SidPool)·types(188칸)·frames(타입별 { frames, defaultFrame })·state({ patternTypes[8], decoderDiagnostics })의 수명은 호출자가 보장한다.
// 사용: new RawCanonPlacementGeometry(pool, types, frames, state).inspect({ type, argument, direction, x, y, explicitFrame }, scan, hooks). (2026-11-09 WB04-E146)
export class RawCanonPlacementGeometry {
    #pool;
    #types;
    #frames;
    #state;

    // 모든 타입 번호와 실제 프레임 메타 자료를 같은 판본으로 받는다. (2026-11-09 WB04-E146)
    constructor(pool, types, frames, state) {
        if (pool?.edition !== 'patch1078') throw new TypeError('일반 배치 모양은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== types.length || !state) throw new TypeError('일반 배치 모양 타입/프레임 자료 누락');
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
    }

    // 동일 풀 연결 확인을 위해 원래 풀을 제공한다. (2026-11-09 WB04-E146)
    get pool() { return this.#pool; }

    // 일반 배치의 첫 인자는 타입 번호와 별개인 패턴 번호이며 비패턴 기본 모드에서는 무시한다. 현재 타입/메타 자료와 타입 전역의 우선순위로 디코더를 구성한다. (2026-11-09 WB04-E146)
    #decode(query) {
        if (query.type >= this.#types.length) throw new RangeError('일반 배치 모양 타입 번호 오류');
        const meta = this.#frames[query.type];
        return decodeCanonType(meta.frames, meta.defaultFrame | 0, this.#state.patternTypes, query, this.#state.decoderDiagnostics ?? null);
    }

    // 빈 디코더에서도 원본처럼 첫 현재 좌표와 순회 개수로 미리보기 범위를 계산한다. (2026-11-09 WB04-E146)
    bounds(query) {
        const decoder = this.#decode({ ...query });
        const type = this.#types[query.type];
        const [left, top, right, bottom] = decoder.bounds(type.footX, type.footY);
        return { left, top, right, bottom };
    }

    // 모든 칸에 준비→finder/후보→지형 종료→advance 를 수행한다. 거부 뒤 최종 판정을 생략한다. (2026-11-09 WB04-E146)
    inspect(query, scan, hooks) {
        if (typeof scan !== 'function' || typeof hooks?.beginRegions !== 'function' || typeof hooks.endShape !== 'function' || typeof hooks.finishRegions !== 'function') {
            throw new TypeError('일반 배치 모양 finder/지역 경계 누락');
        }
        // 요청은 값으로 캡처해 훅이 호출자의 객체를 바꿔도 처음 값을 유지한다.
        query = { ...query };
        const decoder = this.#decode(query);
        hooks.beginRegions(query);
        // 실제 디코더의 빈 칸 건너뛰기와 전체 회전 순서를 유지한다.
        while (decoder.valid) {
            const type = this.#types[query.type];
            const x = decoder.x;
            const y = decoder.y;
            const frame = decoder.frame;
            const codes = this.#frames[query.type].frames;
            // MayPlace 는 방향 코드를 요구하지 않는다. 비패턴 기본 프레임의 개수 검사 생략 계약을 유지한다.
            const side = frame >= 0 && frame < codes.length ? codes[frame].side : 0;
            const cell = { frame, label: decoder.label, side, x, y, snappedX: Math.fround(truncate(x)), snappedY: Math.fround(truncate(y)), area: canonCollisionArea(x, y, type.footX, type.footY) };
            if (typeof hooks.beginShape === 'function') hooks.beginShape(query, cell);
            if (!scan(cell) || !hooks.endShape(query, cell)) return false;
            decoder.advance();
        }
        return hooks.finishRegions(query);
    }
}
