// 서버 건설 요청(Construction.cpp 00444760)의 검증·받침 확정·실패 정리 순서를 옮긴 모듈이다(cpppj `o/RawConstructionRequest.cpp/.h`, 10.78).
//  - request(req): 값형 요청 {type, x, y, player, argument, direction, flags, timeLow, timeHigh, quality}. 요청 타입·섬/받침 패턴 타입이 표 밖이거나 255 초과이거나 좌표가 유한하지 않으면 효과 없이 거부한다.
//    순서: 편집 모드이면 요청 플레이어를 로컬 번호로 바꿈 → 전체 배치 가능 판정 mayPlace({type, argument, x, y, flags: direction, owner: player, mode: 0}) 거짓이면 거짓 →
//    타입별 Canon 디코더의 모든 유효 칸에서 [칸 범위(RawConstructionClear.cellArea) → (관찰 훅 scan) → 일반 finder 로 abstract 예측 중 받침(noIsland 전역 타입)이 아닌 것]이 같은 플레이어·타입·정확한 좌표
//    (float 같음, NaN 은 다름)가 아니면 첫 충돌에서 효과 없이 거짓 → 타입 genus 가 밀어내기 대상(displaceGenus)이면 displace(x, y, type, player) 거짓이면 거짓 →
//    타입 flags1 에 받침 필요 비트(0x400)가 있으면 표면 조회(+0.9999 절삭, 범위 밖은 슬롯 0)를 먼저 읽고 발자국 사각형의 spot 에 bit 2 가 하나도 없거나 표면이 abstract 이면 받침(noIsland)을
//    (x+1−섬 폭, y+1−섬 높이) 위치로 먼저 확정(반환값 무시) → 본 확정 confirm(req) 참이면 참, 거짓이면 현재 flags1 을 다시 읽어 받침 필요 타입이면 실패 정리 cancel(x, y, 0x1000, player) 뒤 거짓.
//  - makeConstructionRequestHooks(pool, placement, confirm, hooks): mayPlace 를 전체 배치 가능 판정 파이프라인(placement.mayPlace)에, confirm 을 실제 서버 확정(confirm.confirm)에 잇는다.
//    풀이 다르면 거부한다.
// 10.72(CD) 경로(displaceCd·소유자 +32)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E140: 건설 요청 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { decodeCanonType } from './raw-canon-type.js';
import { cellArea } from './raw-construction-clear.js';

// raw 타입(+10)·x(+14)·y(+18)·PATCH 소유자(+34)·abstract 비트가 있는 extra(+40) 위치이다. (2026-11-09 WB04-E140)
const TYPE = 10;
const X_AT = 14;
const Y_AT = 18;
const OWNER = 34;
const EXTRA = 40;
// 받침 검사 대상의 flags1 비트, 실패 정리 플래그, 표면 조회의 좌표 편향(00500edc, 0.9998999834060669f)이다. (2026-11-09 WB04-E140)
const NEEDS_BASE = 0x400;
const CANCEL_FLAG = 0x1000;
const SURFACE_BIAS = Math.fround(0.9998999834060669);
// 지도 한 변, spot 표 최소 크기, 타입 표 크기, 섬/받침 패턴 타입 전역의 자리(2, 3)이다. (2026-11-09 WB04-E140)
const MAP_SIDE = 256;
export const SPOT_COUNT = MAP_SIDE * MAP_SIDE;
const TYPE_COUNT = 188;
const ISLAND = 2;
const NO_ISLAND = 3;

// 요청이 읽는 전역이다. 공통 배치 모듈의 editor/localPlayer/패턴 번호와 같은 값을 공급한다: editor(00594fb8)·localPlayer(00540c70)·displaceGenus(005325b4: 이 genus 에 해당하면 기존 유닛을 밀어냄)·
// patternTypes(decoder 와 island/noIsland 번호 전역)·decoderDiagnostics(디코더의 checking·진단 장치)이다. 사용: 월드마다 하나를 두고 RawConstructionRequest 에 넘긴다. (2026-11-09 WB04-E140)
export class ConstructionRequestState {
    editor = false;
    localPlayer = 0;
    displaceGenus = 0;
    patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    decoderDiagnostics = null;
}

// 원본 __ftol 의 하위 signed DWORD 이다. 비유한 값과 64비트 범위 밖은 0 이다. (2026-11-09 WB04-E140)
function ftol(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    const whole = Math.trunc(value);
    if (Math.abs(whole) <= 2147483647) return whole | 0;
    return Number(BigInt.asIntN(32, BigInt(whole)));
}

// 정렬되지 않은 raw 좌표 DWORD 를 리틀엔디언으로 읽어 단정도 float 로 복원한다. (2026-11-09 WB04-E140)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 원본 정수의 32비트 순환 연산을 보존한 발자국 오프셋(1 − 발자국)이다. (2026-11-09 WB04-E140)
function offset(foot) {
    return (1 - (foot >>> 0)) | 0;
}

// 1~255 로 자른다(float 비교는 NaN 이면 그대로). (2026-11-09 WB04-E140)
function clamp(value) {
    return Math.min(Math.max(value, 1), 255);
}

// 0049ae80: 발자국만큼 물린 점을 단정도로 저장·1~255 로 자르고 유효한 원래 좌표와 잇는 사각형 [left, top, right, bottom] 이다. (2026-11-09 WB04-E140)
function footprint(x, y, width, height) {
    const pointX = clamp(Math.fround(x + offset(width)));
    const pointY = clamp(Math.fround(y + offset(height)));
    if (!(x > 0 && y > 0 && x < MAP_SIDE && y < MAP_SIDE)) return [pointX, pointY, pointX, pointY];
    return [Math.min(x, pointX), Math.min(y, pointY), Math.max(x, pointX), Math.max(y, pointY)];
}

// 전체 서버 요청의 실행 계층이다. pool(10.78 SidPool)·hash(SquidHash)·types·frames({ frames, defaultFrame })·spots(256×256 이상 Uint8Array)·state·hooks 는 더 오래 살아야 하며 복사하지 않는다.
// hooks: mayPlace(query)→bool, displace(x, y, type, player)→bool, confirm(request)→bool, cancel(x, y, flags, player) 필수, scan(area) 선택. 로컬 예측/커서와 메시지 수신은 이 계층의 호출자가 담당한다.
// 사용: new RawConstructionRequest(pool, hash, types, frames, spots, state, hooks).request({ type, x, y, player, argument, direction, flags, timeLow, timeHigh, quality }). (2026-11-09 WB04-E140)
export class RawConstructionRequest {
    #pool;
    #hash;
    #types;
    #frames;
    #spots;
    #state;
    #hooks;

    // 경계와 지도 표 크기를 검증한다. (2026-11-09 WB04-E140)
    constructor(pool, hash, types, frames, spots, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('건설 요청은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || !(spots?.length >= SPOT_COUNT) || !state) throw new TypeError('건설 요청 자료/효과 연결 오류');
        for (const name of ['mayPlace', 'confirm', 'cancel', 'displace']) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 요청 자료/효과 연결 오류');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#spots = spots;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 연결 검증에 쓰는 실제 풀 참조이다. (2026-11-09 WB04-E140)
    get pool() { return this.#pool; }

    // float 발자국 사각형 안의 정수 칸에 spot bit 2 가 하나도 없으면 참이다. 큰 모서리는 올림하지 않는다(x 우선, y 안쪽 순서). (2026-11-09 WB04-E140)
    #noSurface(rectangle) {
        const [left, top, right, bottom] = rectangle.map((value) => Math.trunc(value));
        // 모서리는 유한 요청 좌표와 1~255 로 자른 점에서 나오므로 항상 표 안이다.
        for (let x = left; x <= right; x += 1) {
            for (let y = top; y <= bottom; y += 1) {
                if ((this.#spots[y * MAP_SIDE + x] & 2) !== 0) return false;
            }
        }
        return true;
    }

    // 표면 조회는 +0.9999 뒤 절삭한다. 범위 밖 좌표에서는 예약 슬롯 0 을 사용한다. (2026-11-09 WB04-E140)
    #surface(x, y) {
        const cellX = ftol(x + SURFACE_BIAS);
        const cellY = ftol(y + SURFACE_BIAS);
        if (cellX < 0 || cellY < 0 || cellX >= MAP_SIDE || cellY >= MAP_SIDE) return 0;
        return this.#hash.cell(0, cellX, cellY);
    }

    // 후보는 서버 번호도 검사한다. 받침(noIsland)은 소유자/타입/좌표가 달라도 충돌 검사에서 제외한다. (2026-11-09 WB04-E140)
    request(req) {
        const state = this.#state;
        const hooks = this.#hooks;
        const request = { ...req };
        if (request.type >= this.#types.length || request.type >= this.#frames.length || request.type > 0xff
            || state.patternTypes[ISLAND] >= this.#types.length || state.patternTypes[NO_ISLAND] >= this.#types.length
            || !Number.isFinite(request.x) || !Number.isFinite(request.y)) throw new RangeError('건설 요청 타입/좌표 오류');
        const record = this.#types[request.type];
        const meta = this.#frames[request.type];
        if (state.editor) request.player = state.localPlayer;
        if (!hooks.mayPlace({ type: request.type, argument: request.argument, x: request.x, y: request.y, flags: request.direction, owner: request.player, mode: 0 })) return false;
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame | 0, state.patternTypes,
            { type: request.type, argument: request.argument | 0, direction: request.direction | 0, x: request.x, y: request.y, explicitFrame: false }, state.decoderDiagnostics);
        // decoder 의 모든 유효 칸에서 abstract 예측이 같은 플레이어·타입·정확한 좌표인지 검사한다.
        for (; decoder.valid; decoder.advance()) {
            const area = cellArea(decoder.x, decoder.y, record.footX, record.footY);
            if (hooks.scan) hooks.scan(area);
            const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
            // 일반 finder 의 순서를 유지하고 첫 충돌에서 효과 없이 종료한다.
            for (let current = finder.begin(area); current !== 0; current = finder.next()) {
                const raw = this.#pool.slot(current);
                if ((raw[EXTRA] & 1) === 0 || raw[TYPE] === state.patternTypes[NO_ISLAND]) continue;
                if (raw[OWNER] !== request.player || raw[TYPE] !== request.type || coordinate(raw, X_AT) !== decoder.x || coordinate(raw, Y_AT) !== decoder.y) return false;
            }
        }
        if ((record.flags2 & state.displaceGenus) !== 0 && !hooks.displace(request.x, request.y, request.type, request.player)) return false;
        const island = this.#types[state.patternTypes[ISLAND]];
        const baseX = Math.fround(request.x + offset(island.footX));
        const baseY = Math.fround(request.y + offset(island.footY));
        const rectangle = footprint(request.x, request.y, record.footX, record.footY);
        if ((record.flags1 & NEEDS_BASE) !== 0) {
            // 원본은 먼저 표면을 조회한다. spot 에 bit 2 가 없거나 표면이 abstract 이면 받침을 선행 확정한다.
            const surface = this.#pool.slot(this.#surface(request.x, request.y));
            if (this.#noSurface(rectangle) || (surface[EXTRA] & 1) !== 0) {
                hooks.confirm({ type: state.patternTypes[NO_ISLAND], x: baseX, y: baseY, player: request.player, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 });
            }
        }
        if (hooks.confirm(request)) return true;
        // 확정 경계가 타입 플래그를 바꿀 수 있으므로 실패 후 현재 flags1 을 다시 읽는다.
        if ((record.flags1 & NEEDS_BASE) !== 0) hooks.cancel(request.x, request.y, CANCEL_FLAG, request.player);
        return false;
    }
}

// 전체 MayPlace 파이프라인과 실제 서버 확정을 같은 풀로 연결한다. 밀어내기/실패 정리/진단은 받은 경계를 둔다. 풀이 다르면 거부한다. (2026-11-09 WB04-E140)
export function makeConstructionRequestHooks(pool, placement, confirm, hooks) {
    if (pool !== placement?.pool || pool !== confirm?.pool) throw new TypeError('건설 요청 모듈 풀 불일치');
    return { ...hooks, mayPlace: (query) => placement.mayPlace(query), confirm: (request) => confirm.confirm(request) };
}
