// 표시 영역 환산·가시 영역 교차·16바이트 그리기 항목 수집을 옮긴 모듈이다(cpppj `o/RawRenderCollection.cpp/.h`, 10.78 전용. 원본 Particle.cpp 의 collector).
//  - RawRenderCollection.bounds(fields): 00487a40. 부모 head 가 비어 있지 않은지 확인한 뒤 등록된 화면 영역을 `역수(1/배율, double) × 좌표` 로 월드 좌표 사각형(float 4개)으로 환산한다(역수를 float 로 먼저 반올림하지 않음). 배율은 유한 비영이어야 한다.
//  - RawRenderCollection.intersects(view, bounds): 00487740. 네 변의 엄격한 교차(경계만 닿은 영역은 제외)이며 역전 영역은 교차하지 않는다. 점/선 영역은 내부에서 허용한다. NaN 은 비교가 거짓이라 교차하지 않는다.
//  - collect(view, output, limit): 00487f10. 표시 목록 슬롯 0 번부터 limit 개까지 가시 항목을 16바이트 항목(x·y·token·depth·kind·tail)으로 쓴다. limit<=0 이면 0 이며 나머지 출력 바이트는 보존한다.
//    limit 가 항목 수보다 크면 변경 전에 거부한다. 별도의 부모 free/dead/void/abstract 필터는 추가하지 않는다(원본 동일). 출력을 다 채운 뒤 다음 슬롯의 부모를 조회하지 않는다.
// 출력은 항목 n 개의 Uint8Array(16바이트×n)이다. x 는 부모 raw 의 float 바이트를 그대로 복사하고 y 는 `float(부모 y) + 0.001` 을 float 로 저장한다(같은 부모의 자산보다 뒤에 오도록).
// 정렬(draw-sort.js)·실제 출력은 별도 단계이다. 풀/전역/목록/조회 함수는 collector 보다 오래 살아야 한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E131: RawRenderCollection 이식으로 추가)

// 부모 raw 의 head(WORD)·x·y 위치와 표시 항목 종류(kind 3)이다. (2026-11-08 WB04-E131)
const HEAD = 6;
const X = 14;
const Y = 18;
const RENDER_KIND = 3;

// 항목 하나의 바이트 폭이다: x(4)·y(4)·token(4)·depth(2)·kind(1)·tail(1). (2026-11-08 WB04-E131)
export const RENDER_ENTRY_BYTES = 16;

// 표시가 같은 부모의 자산보다 뒤에 오도록 y 에 더하는 원본 float32(0x3a83126f)이다. (2026-11-08 WB04-E131)
const ORDER_BIAS = (() => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, 0x3a83126f, true);
    return view.getFloat32(0, true);
})();

// raw 의 단정도 float 를 읽는다. (2026-11-08 WB04-E131)
function readFloat(raw, offset) {
    const view = new DataView(raw.buffer, raw.byteOffset + offset, 4);
    return view.getFloat32(0, true);
}

// 표시 수집기이다. 사용: const collector = new RawRenderCollection(pool, visualState, registry, (token) => fieldsOf(token)); const count = collector.collect(view, output, limit).
// lookup(token)→등록 필드(RenderProcessFields) 또는 null(생성 중인 호스트 객체를 건너뜀)이며 자료를 바꾸지 않아야 한다. 같은 풀이 아니거나 lookup 이 없으면 거부한다. (2026-11-08 WB04-E131)
export class RawRenderCollection {
    // 판본별 raw 부모 자료, 현재 좌표 배율, 구멍을 포함한 표시 목록, 토큰→필드 조회이다.
    #pool;
    #state;
    #registry;
    #lookup;

    // 등록 목록과 부모 raw 가 같은 풀인지 확인하며 실행/삭제 경계는 만들지 않는다. (2026-11-08 WB04-E131)
    constructor(pool, state, registry, lookup) {
        if (pool !== registry?.pool || typeof lookup !== 'function') throw new TypeError('표시 수집 연결 오류');
        this.#pool = pool;
        this.#state = state;
        this.#registry = registry;
        this.#lookup = lookup;
    }

    // 원본은 부모가 비어 있거나 dead/void 인지를 검사하지 않고 head 만 확인한다. (2026-11-08 WB04-E131)
    #parent(fields) {
        if (fields.parent < 0 || fields.parent >= this.#pool.capacity) throw new RangeError('표시 수집 부모 범위 오류');
        const raw = this.#pool.slot(fields.parent);
        if (!(raw[HEAD] | raw[HEAD + 1])) throw new Error('표시 수집 부모 head 가 비었습니다');
        return raw;
    }

    // 역수를 float 로 먼저 반올림하지 않는다. 결과의 NaN/무한대는 교차 판정에 그대로 전달한다. (2026-11-08 WB04-E131)
    bounds(fields) {
        this.#parent(fields);
        const { scaleX, scaleY } = this.#state;
        if (!Number.isFinite(scaleX) || !Number.isFinite(scaleY) || scaleX === 0 || scaleY === 0) throw new RangeError('표시 수집 배율 오류');
        const x = 1 / scaleX;
        const y = 1 / scaleY;
        return { left: Math.fround(x * fields.left), top: Math.fround(y * fields.top), right: Math.fround(x * fields.right), bottom: Math.fround(y * fields.bottom) };
    }

    // 유효한 점/선은 내부 교차를 허용하지만 네 변 중 하나라도 닿기만 하면 제외한다. (2026-11-08 WB04-E131)
    static intersects(view, bounds) {
        return view.left <= view.right && view.top <= view.bottom && bounds.left <= bounds.right && bounds.top <= bounds.bottom
            && view.left < bounds.right && bounds.left < view.right && view.top < bounds.bottom && bounds.top < view.bottom;
    }

    // 첫 슬롯부터 limit 개까지 가시 항목을 쓴다. 기존 출력의 tail/미사용 항목도 보존한다. (2026-11-08 WB04-E131)
    collect(view, output, limit) {
        limit |= 0;
        const capacity = Math.floor(output.length / RENDER_ENTRY_BYTES);
        if (limit > 0 && limit > capacity) throw new RangeError('표시 수집 출력 용량 오류');
        const target = new DataView(output.buffer, output.byteOffset, output.length);
        let count = 0;
        // 등록 배열의 0번부터 검사하며 빈 칸은 출력 항목을 소비하지 않는다.
        for (const token of this.#registry.slots) {
            if (count >= limit) return count;
            if (!token) continue;
            const fields = this.#lookup(token);
            if (!fields) continue;
            if (!RawRenderCollection.intersects(view, this.bounds(fields))) continue;
            const parent = this.#parent(fields);
            const base = count * RENDER_ENTRY_BYTES;
            count += 1;
            // x 는 부모 raw 의 바이트를 그대로 복사하고 y 는 float 저장 순서를 유지한다.
            output.set(parent.subarray(X, X + 4), base);
            target.setFloat32(base + 4, readFloat(parent, Y) + ORDER_BIAS, true);
            target.setUint32(base + 8, token >>> 0, true);
            target.setInt16(base + 12, fields.zOrder, true);
            target.setUint8(base + 14, RENDER_KIND);
        }
        return count;
    }
}
