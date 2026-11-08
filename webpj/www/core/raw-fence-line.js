// 썬 바리케이드 사이의 기존 연결선 삭제(00455b20 mode 0)를 실제 일반 finder 와 가상 삭제에 연결해 옮긴 모듈이다(cpppj `o/RawFenceLine.cpp/.h`, 10.78 전용).
//  - searchPoints(끝점 네 float): 원본 보정 지점의 순서다. 수직선(x 가 같음)이면 y 방향 ±1 로 3칸 당겨 끝점과 합치고, 수평/대각이면 양 끝 x 를 1칸씩 넓히고 y 를 1 내려 시작 보정 뒤 100회 상한 안에서 끝점을 한 번 포함하는 `_ftol64` 하위 signed 정수 좌표를 낸다.
//    모든 float 연산은 단정도 반올림을 거친다.
//  - clear(끝점): 시작 코드 J(0x4a, x 가 같을 때)/K(0x4b)가 맞는 현재 연결선(현재 연결선 타입 번호의 객체)만 각 지점의 일반 finder 로 찾아 가상 삭제(flags 0x40)한다. 삭제 효과 뒤에도 같은 finder 의 미리 보관한 next 와 다음 현재 버킷을 쓴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E74: 썬 바리케이드 연결선 삭제 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';

// 슬롯의 TYPE/프레임 위치, 연결선 방향 코드(J 수직, K 수평), 삭제 flags, 검색 상한, 타입 표 크기이다. (2026-11-08 WB04-E74)
const TYPE_OFFSET = 10;
const FRAME_OFFSET = 36;
const VERTICAL = 0x4a;
const HORIZONTAL = 0x4b;
const DELETE_FLAGS = 0x40;
const SEARCH_LIMIT = 100;
const TYPE_COUNT = 188;
const f = Math.fround;

// 유효 CRT 입력에서 signed64 절삭의 낮은 DWORD(signed)를 반환한다. 비유한/범위 밖은 거부한다. (2026-11-08 WB04-E74)
function fenceInt(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) throw new RangeError('펜스 연결선 CRT 범위 오류');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// pool 은 10.78 SidPool, hash 는 같은 일반 finder 의 공간 해시, types 는 188칸 TYPE 표, frames 는 188칸 FrameCodeTable 표, markType 은 현재 연결선 타입 번호(DAT00541300)를 읽는 getter, destroy(sid, flags) 는 원본 가상 삭제 전체 몸체이다. (2026-11-08 WB04-E74)
export class RawFenceLine {
    // 현재 mark 기록과 삭제 후 체인을 읽는 풀, mode 0 의 flags 0 일반 finder 가 쓰는 해시, finder 발자국 표, 첫 프레임 코드 표, 현재 연결선 타입 getter, 가상 삭제이다.
    #pool;
    #hash;
    #types;
    #frames;
    #markType;
    #destroy;

    // CD 의 다른 연결선과 표 크기를 PATCH 로 취급하지 않는다. 삭제 소비자는 선택한 경우 필수이므로 도달 때 확인한다. (2026-11-08 WB04-E74)
    constructor(pool, hash, types, frames, markType, destroy) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== TYPE_COUNT || typeof markType !== 'function') {
            throw new TypeError('펜스 연결선 PATCH 표 크기 오류');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#markType = markType;
        this.#destroy = destroy;
    }

    // mode 0 도 동일한 좌표 보정과 100개 검색 상한을 사용한다. 끝점은 한 번 포함한다. (2026-11-08 WB04-E74)
    static searchPoints(endpoints) {
        let x = f(endpoints[0] - 1);
        let y = f(endpoints[1] - 1);
        let endX = f(endpoints[2] - 1);
        let endY = f(endpoints[3] - 1);
        let dx = 0;
        let dy = 0;
        if (x === endX) {
            if (endY <= y) {
                dy = -1;
                y = f(y - 3);
                if (y < endY) endY = y;
            } else {
                dy = 1;
                endY = f(endY - 3);
                if (endY <= y) y = endY;
            }
            if (x <= endX) x = endX;
            if (dy > 0) {
                if (endY <= y) y = endY;
            } else if (y <= endY) y = endY;
        } else {
            if (endX <= x) {
                x = f(x + 1);
                dx = -1;
                endX = f(endX - 1);
            } else {
                x = f(x - 1);
                dx = 1;
                endX = f(endX + 1);
            }
            y = f(y - 1);
            endY = f(endY - 1);
            x = f(f(dx + dx) + x);
            endX = f(endX - f(dx + dx));
            if (dx <= 0) {
                if (x <= endX) x = endX;
            } else if (endX <= x) x = endX;
            if (y <= endY) y = endY;
        }
        const result = [];
        // 원본 100회 상한을 유지하고 끝점 처리 뒤 종료한다.
        for (let remaining = SEARCH_LIMIT; remaining; remaining -= 1) {
            result.push([fenceInt(x), fenceInt(y)]);
            if (x === endX && y === endY) break;
            x = f(x + dx);
            y = f(y + dy);
        }
        return result;
    }

    // 방향 코드가 맞는 현재 연결선만 가상 삭제(flags 0x40)한다. (2026-11-08 WB04-E74)
    clear(endpoints) {
        const direction = endpoints[0] === endpoints[2] ? VERTICAL : HORIZONTAL;
        // 원본 보정 지점을 순서대로 검색한다.
        for (const point of RawFenceLine.searchPoints(endpoints)) {
            const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
            // 표면도 포함하는 실제 일반 탐색이다.
            for (let sid = finder.begin({ left: point[0], top: point[1], right: point[0], bottom: point[1] }, 0); sid; sid = finder.next()) {
                const raw = this.#pool.slot(sid);
                if (raw[TYPE_OFFSET] !== (this.#markType() >>> 0)) continue;
                const type = raw[TYPE_OFFSET];
                // 현재 실제 프레임을 가상 getter 위치에서 읽는다.
                const frame = (raw[FRAME_OFFSET] | (raw[FRAME_OFFSET + 1] << 8) | (raw[FRAME_OFFSET + 2] << 16) | (raw[FRAME_OFFSET + 3] << 24)) | 0;
                const codes = this.#frames[type].codes;
                if (frame < 0 || frame >= codes.length) throw new RangeError('펜스 연결선 현재 프레임 범위 오류');
                if (((codes[frame].side << 24) >> 24) !== direction) continue;
                if (typeof this.#destroy !== 'function') throw new Error('펜스 연결선 실제 가상 삭제 미연결');
                this.#destroy(sid, DELETE_FLAGS);
            }
        }
    }
}
