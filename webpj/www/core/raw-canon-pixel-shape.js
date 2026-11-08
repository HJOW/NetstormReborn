// 일반 타입과 특수 패턴 전체의 픽셀 범위 getter(0043cbd0)를 옮긴 모듈이다(cpppj `o/RawCanonPixelShape.cpp/.h`, 10.78).
//  - measure(type, argument, direction, explicitFrame): 타입별 Canon 디코더(raw-canon-type.js)의 유효 칸을 원본 순서로 모두 돌며, 칸마다 SHP 프레임 머리(폭·높이·기준점)와 타입 핫스폿으로
//    칸의 화면 픽셀 사각형을 만들어 한 개의 합집합 범위 { bounds: {left, top, right, bottom}, anchorX, anchorY } 로 합친다. 시작값은 bounds (1000, 1000, 0, 0)이며 anchor 시작값은 현재 배율이다.
//    · 핫스폿 차이 dx/dy = (프레임 기준점 − 타입 핫스폿)의 int32 감싸기. 칸 좌표×배율은 넓은 정밀도(double)로 곱하고 Y 만 단정도로 저장한다.
//    · 왼쪽/위: 넓은 값이 현재 최솟값보다 작으면 기준점 보정(anchorX = float(dx), anchorY = float(dy))을 갱신하고, 단정도로 줄인 값이 최솟값 이하이면 최솟값을 저장한다.
//    · 오른쪽/아래: (단정도 왼쪽 + 폭), (넓은 위 + 높이)가 최댓값 이상이면(같은 값도) 단정도로 저장한다. 비유한 배율은 첫 유효 칸에서 거부한다(빈 모양은 거부하지 않음).
//  - 프레임 확인(00419850): SHP 가 없거나 물리 프레임이 범위 밖이면, 그리고 논리 프레임 수(코드 수×이중 프레임 플래그면 2)를 넘거나 타입이 해제된 표식(최대 체력이 두 해제 값)이면 원문 assert 대신 거부한다.
// 배율·SHP·기준점·프레임 표는 호출 때마다 현재 값을 읽는다. 10.72(CD)의 중간 단정도 저장·엄격 최댓값 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-09 WB04-E147: 일반 픽셀 모양 getter 이식으로 추가)
import { decodeCanonType } from './raw-canon-type.js';

// 타입 표 크기, 이중 프레임 플래그, 해제된 타입의 두 표식(0xdddddddd·0xcdcdcdcd 의 int32), 빈 범위의 시작값이다. (2026-11-09 WB04-E147)
const TYPE_COUNT = 188;
const DOUBLE_FRAMES = 0x440000;
const FREED_A = -0x22222223;
const FREED_B = -0x32323233;
const EMPTY_LEFT = 1000;
const EMPTY_TOP = 1000;

// 일반 타입·패턴의 픽셀 범위 계층이다. pool(10.78 SidPool)·types(188칸 { flags1, maxHitPoints })·frames(타입별 { frames, defaultFrame })·shapes(타입별 { loaded, frames[{ width, height, hotspotX, hotspotY }] })·
// hotspots(타입별 { x, y })·geometryState({ patternTypes[8], decoderDiagnostics })·state({ scaleX, scaleY })의 수명은 호출자가 보장하며 값은 호출 때마다 다시 읽는다.
// 사용: new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometryState, state).measure(type, argument, direction, explicitFrame). (2026-11-09 WB04-E147)
export class RawCanonPixelShape {
    #pool;
    #types;
    #frames;
    #shapes;
    #hotspots;
    #geometryState;
    #state;

    // 모든 표가 같은 타입 번호 체계(188칸)이고 10.78 풀과 연결될 때만 만든다. (2026-11-09 WB04-E147)
    constructor(pool, types, frames, shapes, hotspots, geometryState, state) {
        if (pool?.edition !== 'patch1078') throw new TypeError('일반 픽셀 모양은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== types.length
            || !Array.isArray(shapes) || shapes.length !== types.length || !Array.isArray(hotspots) || hotspots.length !== types.length || !geometryState || !state) {
            throw new TypeError('일반 픽셀 모양 타입/메타/SHP/기준점 자료 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#shapes = shapes;
        this.#hotspots = hotspots;
        this.#geometryState = geometryState;
        this.#state = state;
    }

    // 배치 접두와 같은 풀에 연결하는지 확인하도록 원래 풀을 제공한다. (2026-11-09 WB04-E147)
    get pool() { return this.#pool; }

    // 00419850 의 프레임 확인: 로드된 SHP 의 물리 프레임 범위, 논리 프레임 수, 해제된 타입 표식을 검사하고 프레임 머리를 돌려준다. (2026-11-09 WB04-E147)
    #header(type, frame) {
        const shape = this.#shapes[type];
        const record = this.#types[type];
        if (!shape.loaded || frame < 0 || frame >= shape.frames.length) throw new RangeError('일반 픽셀 모양 SHP 물리 프레임 조회 실패');
        const count = this.#frames[type].frames.length * ((record.flags1 & DOUBLE_FRAMES) ? 2 : 1);
        const maximum = record.maxHitPoints | 0;
        if (frame >= count || maximum === FREED_A || maximum === FREED_B) throw new RangeError('일반 픽셀 모양 frameCheck 실패(00419850)');
        return shape.frames[frame];
    }

    // 기본 프레임을 디코더 생성 때 캡처한 뒤 유효 칸마다 SHP 머리를 처리해 픽셀 범위를 합친다. 반환은 새 객체이며 도중 거부하면 아무것도 돌려주지 않는다. (2026-11-09 WB04-E147)
    measure(type, argument, direction, explicitFrame = false) {
        if (!Number.isInteger(type) || type < 0 || type >= this.#types.length) throw new RangeError('일반 픽셀 모양 타입 번호 오류');
        const meta = this.#frames[type];
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame | 0, this.#geometryState.patternTypes,
            { type, argument: argument | 0, direction: direction | 0, x: 0, y: 0, explicitFrame: !!explicitFrame }, this.#geometryState.decoderDiagnostics ?? null);
        const result = { bounds: { left: EMPTY_LEFT, top: EMPTY_TOP, right: 0, bottom: 0 }, anchorX: Math.fround(this.#state.scaleX), anchorY: Math.fround(this.#state.scaleY) };
        const bounds = result.bounds;
        // 빈 칸을 건너뛴 디코더의 모든 유효 프레임을 원본 순서로 합친다.
        while (decoder.valid) {
            const scaleX = Math.fround(this.#state.scaleX);
            const scaleY = Math.fround(this.#state.scaleY);
            if (!Number.isFinite(scaleX) || !Number.isFinite(scaleY)) throw new RangeError('일반 픽셀 모양 배율의 비유한 값');
            const header = this.#header(type, decoder.frame);
            const hot = this.#hotspots[type];
            const dx = (header.hotspotX - (hot.x | 0)) | 0;
            const dy = (header.hotspotY - (hot.y | 0)) | 0;
            const offsetX = dx;
            const offsetY = Math.fround(dy);
            const pixelX = decoder.x * scaleX;
            const pixelY = Math.fround(decoder.y * scaleY);
            const leftWide = pixelX - offsetX;
            const topWide = pixelY - offsetY;
            const left = Math.fround(leftWide);
            const top = Math.fround(topWide);
            if (leftWide < bounds.left) result.anchorX = Math.fround(dx);
            if (topWide < bounds.top) result.anchorY = offsetY;
            if (left <= bounds.left) bounds.left = left;
            if (top <= bounds.top) bounds.top = top;
            const right = left + header.width;
            const bottom = topWide + header.height;
            // 패치는 같은 최댓값도 저장한다.
            if (bounds.right <= right) bounds.right = Math.fround(right);
            if (bounds.bottom <= bottom) bounds.bottom = Math.fround(bottom);
            decoder.advance();
        }
        return result;
    }
}
