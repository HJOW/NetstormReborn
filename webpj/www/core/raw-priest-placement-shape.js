// 사제 비패턴 픽셀 범위 getter(0043cbd0)와 실제 SHP 프레임 메타 자료를 연결한 모듈이다(cpppj `o/RawPriestPlacementShape.cpp/.h`, 10.78). 사제 배치의 비패턴 계약을 유지하면서 E147 일반 픽셀 getter 로 계산한다.
//  - new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometryState, state): E147 RawCanonPixelShape 와 같은 자료(188칸 타입 표·프레임 표·SHP 표·핫스폿·패턴 전역·픽셀 배율)를 받는다. 길이가 다르면 거부한다.
//  - measure(type, argument, direction, explicitFrame): 사제 genus(0x200000)이고 현재 패턴 타입 전역이 아닌 타입만 받는다(비패턴 계약). 원본처럼 decoder 좌표는 항상 (0,0)이며 결과 { bounds, anchorX, anchorY } 는 일반 getter 와 같다.
//  - makePriestShapePlacementHooks(pool, shape, hooks): 초기 shape 경계만 실제 픽셀 범위 getter 에 연결한다. 배치 경로는 명시 프레임을 켜지 않고 타입 번호를 첫 인자로 그대로 전달하며 미리보기/충돌 연결은 유지한다.
// 10.72(CD)의 중간 단정도 저장·직접 SHP 조회는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E156: 사제 배치 픽셀 모양 이식으로 추가)
import { RawCanonPixelShape } from './raw-canon-pixel-shape.js';

// 사제 genus 비트와 타입 표 크기이다. (2026-11-09 WB04-E156)
const GENUS_PRIEST = 0x200000;
const TYPE_COUNT = 188;

// 사제 비패턴 픽셀 범위 계층이다. 참조의 수명은 호출자가 보장하며 값은 호출 때마다 다시 읽는다. (2026-11-09 WB04-E156)
export class RawPriestPlacementShape {
    #pool;
    #types;
    #frames;
    #shapes;
    #hotspots;
    #geometryState;
    #state;

    // 같은 타입 번호 체계로 프레임 표·SHP·기준점을 받는다. 공통 getter 의 구성 검사도 같은 시점에 한다. (2026-11-09 WB04-E156)
    constructor(pool, types, frames, shapes, hotspots, geometryState, state) {
        if (pool?.edition !== 'patch1078') throw new TypeError('사제 픽셀 모양은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== types.length || !Array.isArray(shapes) || shapes.length !== types.length
            || !Array.isArray(hotspots) || hotspots.length !== types.length || !geometryState || !state) {
            throw new TypeError('사제 픽셀 모양 타입/메타/SHP/기준점 자료 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#shapes = shapes;
        this.#hotspots = hotspots;
        this.#geometryState = geometryState;
        this.#state = state;
    }

    // 배치 접두와 같은 풀/판본에 연결하는지 확인할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E156)
    get pool() { return this.#pool; }

    // 사제 배치의 비패턴 계약을 유지하면서 공통 픽셀 getter 로 계산한다. (2026-11-09 WB04-E156)
    measure(type, argument, direction, explicitFrame = false) {
        if (!Number.isInteger(type) || type < 0 || type >= this.#types.length || (this.#types[type].flags2 & GENUS_PRIEST) === 0 || this.#geometryState.patternTypes.includes(type)) {
            throw new RangeError('사제 픽셀 모양 비패턴 타입 계약 오류');
        }
        return new RawCanonPixelShape(this.#pool, this.#types, this.#frames, this.#shapes, this.#hotspots, this.#geometryState, this.#state).measure(type, argument, direction, explicitFrame);
    }
}

// 배치 경로는 명시 프레임을 켜지 않고 타입 번호를 첫 인자로 그대로 전달한다. 초기 shape 경계만 실제 픽셀 범위 getter 에 연결한다. (2026-11-09 WB04-E156)
export function makePriestShapePlacementHooks(pool, shape, hooks = {}) {
    if (pool !== shape.pool) throw new TypeError('사제 픽셀 모양/배치의 SID 풀이 다릅니다');
    return { ...hooks, shape: (type, argument, direction) => shape.measure(type, argument, direction).bounds };
}
