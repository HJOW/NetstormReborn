// 배치 통지 전의 로컬 예측 조각 정리(Construction.cpp 00441fd0)를 옮긴 모듈이다(cpppj `o/RawConstructionClear.cpp/.h`, 10.78).
// 배치 통지 처리기가 조각을 놓기 직전에 부른다. 통지가 놓을 칸마다 그 자리에 먼저 놓여 있던 클라이언트 영역의 abstract 조각(로컬 배치가 예측으로 놓은 것)을 하나씩 지워 통지의 조각이 들어갈 자리를 비운다.
//  - clear(request): 요청 {type, x, y, argument, direction, player} 로 타입별 Canon 디코더(raw-canon-type.js)를 만들고, 디코더의 유효 칸마다 [칸 범위 계산 → (관찰 훅 scan) → 실제 `RawSquidFinder` 탐색 →
//    후보 판정]을 한다. 후보는 extra(+40) abstract 비트가 있고, 번호가 5 이상 서버 영역(15000) 미만이며, 타입이 요청과 같거나(권한이 없을 때는 타입 무관) 해야 한다. 조건에 맞는 첫 후보 하나만
//    `destroy(후보, 플래그)` 로 지우고 그 칸을 끝낸다. 플래그는 후보가 통지와 같은 조각(타입이 같고, 프레임 DWORD[+36]가 디코더 프레임과 같거나 다리 타입이면 방향 글자가 같음)이면서 소유자(+34)가
//    통지 플레이어와 같을 때만 0x2000000(SAME_PIECE_FLAG), 그 밖에는 0 이다. 한 칸에서 둘 이상 지우지 않는다. 후보를 지운 뒤에도 해시에서 빼는 일은 destroy 경계의 몫이다.
//  - cellArea(x, y, footX, footY): 칸 좌표와 타입 발자국으로 탐색 범위를 구한다. (x−(폭−1), y−(높이−1))을 1~255 로 자른 점과 칸 좌표를 잇는 사각형이며 칸 좌표가 지도 밖(0 이하 또는 256 이상)이면 자른 점
//    하나로 줄어든다. 작은 쪽은 0방향 절삭, 큰 쪽은 0.99999 를 더해 절삭한다(0041d770).
// 타입 번호가 표 밖이거나 255 초과면 거부하고, 삭제 경계가 없으면 생성 때 거부한다. 10.72(CD)와 10.37 경로(프레임 BYTE +34, 소유자 +32)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-09 WB04-E137: 건설 예측 정리 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { decodeCanonType } from './raw-canon-type.js';

// raw 타입(+10)·abstract 비트가 있는 extra(+40)·소유자(+34)·프레임 DWORD(+36) 위치이다. (2026-11-09 WB04-E137)
const TYPE = 10;
const EXTRA = 40;
const OWNER = 34;
const FRAME = 36;
// 일반 객체의 첫 번호, 서버 영역 첫 번호, 통지와 같은 조각을 지울 때 넘기는 삭제 플래그, 타입 번호 상한이다. (2026-11-09 WB04-E137)
const FIRST_SID = 5;
const SERVER_FIRST = 15000;
export const SAME_PIECE_FLAG = 0x2000000;
const MAX_TYPE = 0xff;
// 좌표를 자르는 범위(005012d0·005022e8)와 좌표 유효성의 상한(지도 크기 256), 큰 쪽 모서리를 올림할 때 더하는 값(00502310)이다. (2026-11-09 WB04-E137)
const CLAMP_LOW = 1;
const CLAMP_HIGH = 255;
const MAP_LIMIT = 256;
const ROUND_UP_BIAS = Math.fround(0.9999899864196777);

// 정리가 읽는 전역이다. authority(00540bc4: 꺼져 있으면 타입이 다른 abstract 조각도 후보)·bridgeType(005411a0: 프레임이 달라도 방향 글자가 같으면 같은 조각으로 보는 타입, 82)·
// patternTypes(Canon 디코더가 패턴으로 분기하는 여덟 타입 전역의 현재 번호)·sharedAuthority(실제 Client boss 를 빌리는 함수)·decoderDiagnostics(디코더의 checking·진단 장치)이다.
// 사용: 월드마다 하나를 두고 RawConstructionClear 에 넘긴다. 값은 후보 비교마다 다시 읽는다. (2026-11-09 WB04-E137)
export class ConstructionClearState {
    authority = true;
    bridgeType = 82;
    patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    sharedAuthority = null;
    decoderDiagnostics = null;

    // 앞선 가상 삭제 뒤의 다음 후보도 현재 권한을 읽는다. (2026-11-09 WB04-E137)
    authorityNow() { return this.sharedAuthority ? Boolean(this.sharedAuthority()) : this.authority; }
}

// 원본 __ftol 의 하위 signed DWORD 이다. 비유한 값과 64비트 범위 밖은 하위 DWORD 가 0 이다. (2026-11-09 WB04-E137)
function ftol(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    const whole = Math.trunc(value);
    if (Math.abs(whole) <= 2147483647) return whole | 0;
    return Number(BigInt.asIntN(32, BigInt(whole)));
}

// 0041d0d0: 좌표 하나를 1~255 로 자른다. 먼저 아래를 올리고 그 다음 위를 내린다. (2026-11-09 WB04-E137)
function clamp(value) {
    const raised = CLAMP_LOW > value ? CLAMP_LOW : value;
    return CLAMP_HIGH < raised ? CLAMP_HIGH : raised;
}

// 0040e800: 두 좌표가 모두 0 초과 지도 크기 미만인지 본다. (2026-11-09 WB04-E137)
function valid(x, y) {
    return x > 0 && x < MAP_LIMIT && y > 0 && y < MAP_LIMIT;
}

// 정렬되지 않은 리틀엔디언 DWORD 를 읽는다. (2026-11-09 WB04-E137)
function readUnsigned(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 칸 좌표와 타입 발자국으로 탐색 범위 {left, top, right, bottom} 을 구한다(0041d770). 발자국만큼 왼쪽/위로 물린 점을 x87 처럼 넓은 정밀도로 구한 뒤 단정도로 저장하고 1~255 로 자른다.
// 칸 좌표가 유효하면 두 모서리를 자른 점과 비교해 넓히고, 유효하지 않으면 두 모서리 모두 자른 점이 된다. 큰 쪽은 0.99999 를 더한 값을 단정도로 좁히지 않고 절삭한다. (2026-11-09 WB04-E137)
export function cellArea(x, y, footX, footY) {
    x = Math.fround(x);
    y = Math.fround(y);
    const pointX = clamp(Math.fround(x - (footX - 1)));
    const pointY = clamp(Math.fround(y - (footY - 1)));
    let left = pointX;
    let top = pointY;
    let right = pointX;
    let bottom = pointY;
    if (valid(x, y)) {
        left = x < pointX ? x : pointX;
        top = y < pointY ? y : pointY;
        right = x > pointX ? x : pointX;
        bottom = y > pointY ? y : pointY;
    }
    return { left: ftol(left), top: ftol(top), right: ftol(right + ROUND_UP_BIAS), bottom: ftol(bottom + ROUND_UP_BIAS) };
}

// 로컬 예측 조각 정리의 실행 계층이다. pool(10.78 SidPool)·hash(SquidHash)·types(현재 타입 표)·frames(타입별 { frames: 프레임 코드 배열, defaultFrame })·state·hooks 는
// 이 객체보다 오래 살아야 하며 복사하지 않는다. hooks.destroy(sid, flags) 는 필수, hooks.scan(area) 는 관찰용 선택이다.
// 사용: new RawConstructionClear(pool, hash, types, frames, state, { destroy }).clear({ type, x, y, argument, direction, player }) — 배치 통지를 받으면 조각을 놓기 전에 한 번 부른다. (2026-11-09 WB04-E137)
export class RawConstructionClear {
    #pool;
    #hash;
    #types;
    #frames;
    #state;
    #hooks;

    // 필수 경계는 첫 효과 전에 검사한다. (2026-11-09 WB04-E137)
    constructor(pool, hash, types, frames, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('건설 예측 정리는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || !Array.isArray(frames) || !state || typeof hooks?.destroy !== 'function') throw new TypeError('건설 예측 정리의 삭제 효과 연결 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 호출자가 같은 풀을 쓰는지 확인할 수 있게 실제 풀을 돌려준다. (2026-11-09 WB04-E137)
    get pool() { return this.#pool; }

    // 원본 순서: 디코더 생성 → 칸마다 (범위 계산 → 탐색 → 첫 후보 판정/삭제) → 다음 칸. 요청의 argument/direction 은 uint32 비트이며 디코더에는 signed 로 넘긴다. (2026-11-09 WB04-E137)
    clear(request) {
        if (request.type >= this.#types.length || request.type >= this.#frames.length || request.type > MAX_TYPE) throw new RangeError('건설 예측 정리 타입 번호 오류');
        const state = this.#state;
        const hooks = this.#hooks;
        const record = this.#types[request.type];
        const meta = this.#frames[request.type];
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame | 0, state.patternTypes,
            { type: request.type, argument: request.argument | 0, direction: request.direction | 0, x: request.x, y: request.y, explicitFrame: false }, state.decoderDiagnostics);
        const codes = meta.frames;
        // 유효 칸마다 그 칸 범위에서 지울 후보를 하나 찾는다.
        for (; decoder.valid; decoder.advance()) {
            const area = cellArea(decoder.x, decoder.y, record.footX, record.footY);
            if (hooks.scan) hooks.scan(area);
            const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
            // 탐색기가 돌려주는 순서대로 후보 조건을 본다. 조건에 맞는 첫 객체를 지우고 이 칸을 끝낸다.
            for (let current = finder.begin(area); current !== 0; current = finder.next()) {
                const raw = this.#pool.slot(current);
                const type = raw[TYPE];
                if ((raw[EXTRA] & 1) === 0 || current < FIRST_SID || current >= SERVER_FIRST || (type !== request.type && state.authorityNow())) continue;
                let flags = 0;
                if (type === request.type) {
                    // 프레임이 같거나, 다리 타입이면서 방향 글자가 같으면 통지와 같은 조각이다(패치는 DWORD 프레임을 비교한다).
                    const frame = readUnsigned(raw, FRAME);
                    let same = frame === (decoder.frame >>> 0);
                    if (!same && type === state.bridgeType) {
                        if (frame >= codes.length) throw new RangeError('건설 예측 정리 후보의 프레임 범위 오류');
                        same = ((codes[frame].side << 24) >> 24) === ((decoder.side << 24) >> 24);
                    }
                    // 같은 조각이라도 소유자가 통지의 플레이어와 같을 때만 같은 조각 플래그로 지운다.
                    if (same && raw[OWNER] === request.player) flags = SAME_PIECE_FLAG;
                }
                hooks.destroy(current, flags);
                break;
            }
        }
    }
}
