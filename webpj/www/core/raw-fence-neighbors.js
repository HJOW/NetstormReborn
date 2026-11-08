// 10.78 썬 바리케이드(Fence)의 방향별 최근접 이웃 검색과 실제 연결 개수(00456220/00455fb0 이웃·00455f00 축 사거리·004573d0 연결 수)를 같은 공간 해시에 연결한 모듈이다(cpppj `o/RawFenceNeighbors.cpp/.h`, 10.78 전용).
//  - searchBounds(sid, direction): 원본 Rect min/max 의 현재 float 경계이다. 현재 위치에서 (발자국−1 float) 만큼 빼고 정수 사거리(TYPE+44 의 CRT 절삭, imul 하위 DWORD)×방향 벡터를 더한 점을 새 corner 로 1..255 에 clamp 해 포함한다.
//  - select(sid, candidates, allied): finder 가 반환한 순서(중복·자기 자신 포함)대로 같은 TYPE·살아 있음(state bit1 없음)·extra&9 없음·같은 축(x 또는 y 일치)인 후보 중 float 중심 사이 거리가 **엄격히** 더 가까운 첫 후보를 고르고(최대 거리는 정수 사거리의 두 배),
//    allied 이면 최근접을 고른 뒤에만 후보 소유자 행→현재 소유자 열의 동맹 DWORD 를 읽어 다른 소유자인데 동맹이 아니면 빈 값(0)을 돌려준다(가까운 적대를 건너뛰어 먼 동맹으로 대체하지 않음).
//  - neighbor(sid, direction, allied = false): 검색 경계를 CRT 절삭한 정수 영역으로 일반 finder(flags 1: 표면 건너뜀)를 순회해 후보를 모으고 select 를 실행한다.
//  - withinRange(sid, endpoints): 수직이면 Y 차, 아니면 X 차의 절댓값을 CRT 절삭(float 로 변환)해 TYPE 사거리(float)와 `<=` 비교한다.
//  - connections(sid): extra&9 이면 0, 아니면 0·2·4·6 방향마다 동맹 이웃을 찾아 사거리 안이고 이웃 extra bit0 이 없을 때 한 번씩 센다(적대의 다른 후보로 대체하지 않음).
// ranges 는 TYPE+44 의 float 표(188), alliances 는 후보 소유자 행→원본 소유자 열의 현재 81 DWORD 이다. 호출 때마다 현재 값을 읽는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E88: 썬 바리케이드 이웃 검색 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 0052f83c/0052f85c 의 실제 여덟 방향이다. 이웃 검색은 짝수 방향만 사용한다. (2026-11-08 WB04-E88)
const DX = Object.freeze([0, 1, 1, 1, 0, -1, -1, -1]);
const DY = Object.freeze([-1, -1, 0, 1, 1, 1, 0, -1]);
const fround = Math.fround;
// 슬롯의 TYPE(+10)·상태(+11)·소유자(+34)·extra(+40)·좌표(+14/+18) 위치와 Player 수이다. (2026-11-08 WB04-E88)
const TYPE = 10;
const STATE = 11;
const OWNER = 34;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;
const PLAYER_SLOTS = 9;

// 비정렬 float 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E88)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 원본 BoardCoord 의 정상 보드 내부 조건이다. (2026-11-08 WB04-E88)
const valid = (point) => point[0] > 0 && point[0] < 256 && point[1] > 0 && point[1] < 256;
const clamp = (value) => (value < 1 ? 1 : 255 < value ? 255 : value);

// 현재 Rect 의 양 끝을 별도로 검사하고 새 점만 1..255 에 제한한다. rect = [left, top, right, bottom]. (2026-11-08 WB04-E88)
function include(rect, point) {
    point = [clamp(point[0]), clamp(point[1])];
    if (valid([rect[0], rect[1]])) { rect[0] = point[0] < rect[0] ? point[0] : rect[0]; rect[1] = point[1] < rect[1] ? point[1] : rect[1]; } else { rect[0] = point[0]; rect[1] = point[1]; }
    if (valid([rect[2], rect[3]])) { rect[2] = rect[2] < point[0] ? point[0] : rect[2]; rect[3] = rect[3] < point[1] ? point[1] : rect[3]; } else { rect[2] = point[0]; rect[3] = point[1]; }
}

// 원본 유효 CRT 입력에서 signed 64 비트 절삭의 낮은 32 비트(signed)를 보존한다. (2026-11-08 WB04-E88)
function truncate(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) throw new RangeError('펜스 CRT 절삭 범위 오류');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

export class RawFenceNeighbors {
    // 동적 후보의 같은 50바이트 풀, 후보 순서를 정하는 해시, 발자국 TYPE 표, TYPE+44 사거리 표, 현재 81 DWORD 동맹 행렬이다.
    #pool;
    #hash;
    #types;
    #ranges;
    #alliances;

    // 다른 판본이나 복사한 불완전 동맹 표를 허용하지 않는다. (2026-11-08 WB04-E88)
    constructor(pool, hash, types, ranges, alliances) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || ranges.length !== 188 || alliances.length !== 81) throw new TypeError('펜스 이웃의 PATCH 타입/동맹 크기 불일치');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#ranges = ranges;
        this.#alliances = alliances;
    }

    // free 를 임의 필터링하지 않고 원본처럼 SID 1 부터 유효한 현재 슬롯을 허용한다. (2026-11-08 WB04-E88)
    #slot(sid) {
        if (!Number.isInteger(sid) || sid < 1 || sid >= this.#pool.capacity) throw new RangeError('펜스 이웃 SID 범위 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE] >= this.#types.length) throw new RangeError('펜스 이웃 TYPE 범위 오류');
        return raw;
    }

    // 캡처 위치와 현재 타입 중심은 원본의 서로 다른 읽기 위치를 유지한다. (2026-11-08 WB04-E88)
    #position(sid) {
        const raw = this.#slot(sid);
        return [coordinate(raw, X_AT), coordinate(raw, Y_AT)];
    }

    // 004adea0 은 각 성분을 float 에 저장한 뒤 거리 helper 에 전달한다. (2026-11-08 WB04-E88)
    #center(sid) {
        const point = this.#position(sid);
        const type = this.#types[this.#slot(sid)[TYPE]];
        return [fround(point[0] - type.footX * 0.5), fround(point[1] - type.footY * 0.5)];
    }

    // 현재 TYPE 사거리를 매번 읽는다(CRT 절삭). (2026-11-08 WB04-E88)
    #range(sid) {
        return truncate(this.#ranges[this.#slot(sid)[TYPE]]);
    }

    // float Rect 구성은 타입 발자국을 뺀 지점에 원본 정수 사거리의 방향 벡터를 더한다(signed 곱셈의 낮은 DWORD 보존). (2026-11-08 WB04-E88)
    searchBounds(sid, direction) {
        if (!Number.isInteger(direction) || direction < 0 || direction >= 8) throw new RangeError('펜스 방향 범위 오류');
        const point = this.#position(sid);
        const type = this.#types[this.#slot(sid)[TYPE]];
        const range = this.#range(sid);
        const dx = Math.imul(range, DX[direction]);
        const dy = Math.imul(range, DY[direction]);
        const bounds = [point[0], point[1], point[0], point[1]];
        include(bounds, [fround(point[0] - fround(type.footX - 1) + fround(dx)), fround(point[1] - fround(type.footY - 1) + fround(dy))]);
        return bounds;
    }

    // 같은 거리의 첫 후보를 유지한다. 동맹 검사는 가장 가까운 후보를 고른 뒤에만 수행한다. (2026-11-08 WB04-E88)
    select(sid, candidates, allied) {
        const position = this.#position(sid);
        let result = 0;
        // 정렬 축 비교는 원문 진입 시 Pop 좌표를 사용한다. 원본 최대 거리는 정수 사거리의 두 배이다.
        let best = fround(Math.imul(this.#range(sid), 2));
        // finder 가 준 순서와 중복을 그대로 소비한다.
        for (const candidate of candidates) {
            const raw = this.#slot(candidate);
            const source = this.#slot(sid);
            const point = this.#position(candidate);
            if (raw[TYPE] !== source[TYPE] || candidate === sid || (raw[STATE] & 2) || (raw[EXTRA] & 9) || (point[0] !== position[0] && point[1] !== position[1])) continue;
            const a = this.#center(sid);
            const b = this.#center(candidate);
            const dx = b[0] - a[0];
            const dy = b[1] - a[1];
            const distance = Math.sqrt(dx * dx + dy * dy);
            if (best <= distance) continue;
            best = fround(distance);
            result = candidate;
        }
        if (result && allied) {
            // 최근접 선택 뒤 현재 원문 소유자를 읽는다.
            const owner = this.#slot(sid)[OWNER];
            const other = this.#slot(result)[OWNER];
            if (owner >= PLAYER_SLOTS || other >= PLAYER_SLOTS) throw new RangeError('펜스 동맹 소유자 범위 오류');
            if (owner !== other && this.#alliances[other * PLAYER_SLOTS + owner] === 0) return 0;
        }
        return result;
    }

    // flags 1 의 실제 일반 finder 가 선택한 후보만 거리 선택기에 전달한다. (2026-11-08 WB04-E88)
    neighbor(sid, direction, allied = false) {
        const bounds = this.searchBounds(sid, direction);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        const candidates = [];
        // 표면을 건너뛰고 현재 해시의 순서대로 수집한다.
        for (let candidate = finder.begin({ left: truncate(bounds[0]), top: truncate(bounds[1]), right: truncate(bounds[2]), bottom: truncate(bounds[3]) }, 1); candidate; candidate = finder.next()) candidates.push(candidate);
        return this.select(sid, candidates, allied);
    }

    // 수직이면 Y 차, 그 외에는 X 차의 절댓값을 절삭한 뒤 원본 float 사거리와 비교한다. (2026-11-08 WB04-E88)
    withinRange(sid, endpoints) {
        const distance = endpoints[0] === endpoints[2] ? Math.abs(endpoints[1] - endpoints[3]) : Math.abs(endpoints[0] - endpoints[2]);
        return fround(truncate(distance)) <= this.#ranges[this.#slot(sid)[TYPE]];
    }

    // 로컬/적대의 다른 가까운 후보로 대체하지 않고 각 실제 동맹 이웃을 한 번씩 센다. (2026-11-08 WB04-E88)
    connections(sid) {
        if (this.#slot(sid)[EXTRA] & 9) return 0;
        const position = this.#position(sid);
        let result = 0;
        // 원본은 방향 0/2/4/6 을 순서대로 검사한다.
        for (let direction = 0; direction < 8; direction += 2) {
            const neighbor = this.neighbor(sid, direction, true);
            if (!neighbor) continue;
            const point = this.#position(neighbor);
            if (this.withinRange(sid, [position[0], position[1], point[0], point[1]]) && !(this.#slot(neighbor)[EXTRA] & 1)) result += 1;
        }
        return result;
    }
}
