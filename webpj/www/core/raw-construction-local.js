// 로컬 건설 배치(Construction.cpp 004433b0)를 옮긴 모듈이다(cpppj `o/RawConstructionLocal.cpp/.h`, 10.78): 커서 미리보기를 없애고 abstract 예측 조각을 놓은 뒤 서버에 요청한다.
//  - place(request): 요청 {preview, x, y, argument, direction}(원본 스택 인자처럼 복사한다). 미리보기 번호가 5 미만이거나 풀 밖이거나 좌표가 유한하지 않으면, 타입 번호가 표 밖이거나 받침이 필요한 타입의
//    spot 좌표가 지도 밖이면 첫 Form 효과 전에 거부한다. 순서: 미리보기 raw 의 타입 WORD(+18)를 먼저 읽음 → previewSource(미리보기 연관 객체 번호) → (그 효과 뒤의 현재 raw 에서) 반환 플래그 WORD(+20)와
//    품질 BYTE(+30)를 읽음 → destroyPreview(미리보기, 0) → 반환 플래그 비트3 이면 returnPiece(플래그, 연관 번호, 품질) → 본 조각 SID 확보(allocate: 디코더 유효 칸마다 create(type, 2), 18개 초과·번호 오류 거부) →
//    받침이 필요한 타입이면 시작 칸 하나의 spot bit 2 가 없을 때 받침(noIsland)을 (x+1−섬 폭, y+1−섬 높이) 위치로 확보해 place(abstract 1, 품질 0) → 본 place(abstract 1, 품질) →
//    (배치 경계 뒤의 현재 flags1 에) 사제가 지어야 함(0x8000)이 있으면 moveBuilder(로컬, 첫 SID, 0, 0) → send(9, 서버 요청 메시지).
//  - 서버 요청 메시지는 {type, x, y, argument(WORD), direction, quality, timeLow 0, timeHigh 0} 논리 본문이며 wire 직렬화는 별도(RawConstructionCommand)이다.
//  - makeConstructionLocalHooks(pool, factory, place, hooks): create 를 같은 풀의 실제 factory.create 에, place 를 실제 건설 배치(place.place)에 잇는다. 풀이 다르면 거부한다.
// 비용 차감은 상위 커서 내려놓기의 책임이다. 10.72(CD) 경로(품질 BYTE +26)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E140: 로컬 건설 배치 이식으로 추가)
import { decodeCanonType } from './raw-canon-type.js';

// 미리보기 Form 안의 타입 WORD(+18)·반환 플래그 WORD(+20)·PATCH 품질 BYTE(+30) 위치이다. (2026-11-09 WB04-E140)
const PREVIEW_TYPE = 18;
const PREVIEW_FLAGS = 20;
const PATCH_QUALITY = 30;
// spot 좌표 절삭 전 보정값(float 덧셈으로 먼저 반올림하지 않고 넓게 더한다)이다. (2026-11-09 WB04-E140)
const SPOT_BIAS = 0.9998999834060669;
// 타입의 받침/사제 비트, 생산 칸 반환 비트, 클라이언트 할당 플래그, 지도 한 변, 로컬 SID 배열 상한(WORD 18개), 일반 객체의 첫 번호, 타입 표 크기이다. (2026-11-09 WB04-E140)
const NEEDS_BASE = 0x400;
const NEEDS_BUILDER = 0x8000;
const RETURN_PIECE = 8;
const CLIENT = 2;
const MAP_SIDE = 256;
const MAX_SIDS = 18;
const FIRST_SID = 5;
const TYPE_COUNT = 188;
// 섬/받침 타입 전역의 자리와 서버 번호(요청 수신자)이다. (2026-11-09 WB04-E140)
const ISLAND = 2;
const NO_ISLAND = 3;
const SERVER_PLAYER = 9;
// 필수 하위 효과 이름이다(생성 때 모두 확인한다). (2026-11-09 WB04-E140)
const HOOK_NAMES = Object.freeze(['previewSource', 'destroyPreview', 'returnPiece', 'create', 'place', 'moveBuilder', 'send']);

// 로컬 배치가 읽는 플레이어와 Canon 디코더 타입 전역이다: localPlayer(00540c70: 예측 조각 소유자와 사제 이동의 플레이어)·patternTypes(여덟 타입 전역)·decoderDiagnostics(디코더의 checking·진단 장치).
// 비용 차감은 상위 커서 내려놓기의 책임이다. 사용: 월드마다 하나를 두고 RawConstructionLocal 에 넘긴다. (2026-11-09 WB04-E140)
export class ConstructionLocalState {
    localPlayer = 0;
    patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    decoderDiagnostics = null;
}

// raw Form 의 두 바이트를 리틀엔디언 WORD 로 읽는다. 정렬과 호스트 표현에 의존하지 않는다. (2026-11-09 WB04-E140)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 원본의 32비트 (1−foot) 계산을 보존해 좌표 오프셋으로 넓힌다. (2026-11-09 WB04-E140)
function baseOffset(foot) {
    return (1 - (foot >>> 0)) | 0;
}

// 기존 SID 풀·타입/프레임 표와 spot bit 2 지도를 이용하는 로컬 배치 계층이다. pool(10.78 SidPool)·types(188칸)·frames({ frames, defaultFrame })·spots(256×256 이상 Uint8Array)·state·hooks 는
// 객체보다 오래 살아야 하며 효과 경계는 모두 필수이다. 사용: new RawConstructionLocal(pool, types, frames, spots, state, hooks).place({ preview, x, y, argument, direction }). (2026-11-09 WB04-E140)
export class RawConstructionLocal {
    #pool;
    #types;
    #frames;
    #spots;
    #state;
    #hooks;

    // 첫 호출 전에 지도 크기와 필수 경계를 검사한다. (2026-11-09 WB04-E140)
    constructor(pool, types, frames, spots, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('로컬 건설은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || !(spots?.length >= MAP_SIDE * MAP_SIDE) || !state) throw new TypeError('로컬 건설 지도/필수 효과 연결 오류');
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('로컬 건설 지도/필수 효과 연결 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#spots = spots;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 어댑터가 같은 풀인지 확인할 때 쓰는 실제 풀 참조이다. (2026-11-09 WB04-E140)
    get pool() { return this.#pool; }

    // 모든 유효 칸에 타입 자체를 사용해 SID 를 만든다. 디코더가 골라 준 프레임은 뒤의 실제 place 가 사용한다. 원본 로컬 WORD 배열(18개)에 들어가는 칸 수인지 SID 생성 전에 검사한다. (2026-11-09 WB04-E140)
    #allocate(type, x, y, argument, direction) {
        const meta = this.#frames[type];
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame | 0, this.#state.patternTypes,
            { type, argument: argument | 0, direction: direction | 0, x, y, explicitFrame: false }, this.#state.decoderDiagnostics);
        let pieces = 0;
        for (const probe = decoder.clone(); probe.valid; probe.advance()) {
            pieces += 1;
            if (pieces > MAX_SIDS) throw new RangeError('로컬 건설 조각 수가 지역 SID 배열을 넘는다');
        }
        const sids = [];
        // decoder 순서 그대로 클라이언트 슬롯을 확보한다.
        for (; decoder.valid; decoder.advance()) {
            const sid = this.#hooks.create(type, CLIENT);
            if (sid < FIRST_SID || sid >= this.#pool.capacity) throw new RangeError('로컬 건설 SID 생성 번호 오류');
            sids.push(sid);
        }
        return sids;
    }

    // 좌표·타입 오류는 미리보기 삭제 전에 거부한다. 실제 호출 순서는 원본 로컬 배치 몸체와 같다. (2026-11-09 WB04-E140)
    place(req) {
        const state = this.#state;
        const hooks = this.#hooks;
        const request = { ...req };
        if (request.preview < FIRST_SID || request.preview >= this.#pool.capacity || !Number.isFinite(request.x) || !Number.isFinite(request.y)) throw new RangeError('로컬 건설 미리보기/좌표 오류');
        const type = word(this.#pool.slot(request.preview), PREVIEW_TYPE);
        const island = state.patternTypes[ISLAND];
        const noIsland = state.patternTypes[NO_ISLAND];
        if (type >= this.#types.length || type >= this.#frames.length || type > 0xff || island >= this.#types.length || noIsland > 0xff
            || noIsland >= this.#types.length || noIsland >= this.#frames.length) throw new RangeError('로컬 건설 타입 번호 오류');
        const spotX = Math.trunc(request.x + SPOT_BIAS);
        const spotY = Math.trunc(request.y + SPOT_BIAS);
        const inSpotRange = spotX >= 0 && spotX < MAP_SIDE && spotY >= 0 && spotY < MAP_SIDE;
        if ((this.#types[type].flags1 & NEEDS_BASE) !== 0 && !inSpotRange) throw new RangeError('로컬 건설 spot 좌표가 지도 밖입니다');
        const source = hooks.previewSource(request.preview);
        // 원본의 raw 는 Form 의 살아 있는 바이트이므로 previewSource 가 바꾼 값을 그 뒤에 읽는다.
        const raw = this.#pool.slot(request.preview);
        const flags = word(raw, PREVIEW_FLAGS);
        const quality = raw[PATCH_QUALITY];
        hooks.destroyPreview(request.preview, 0);
        if ((flags & RETURN_PIECE) !== 0) hooks.returnPiece(flags, source, quality);
        const main = this.#allocate(type, request.x, request.y, request.argument, request.direction);
        // 받침 여부는 시작 칸 하나의 spot bit 2 만 읽는다. 서버 요청의 전체 발자국 검사와 다르다.
        if ((this.#types[type].flags1 & NEEDS_BASE) !== 0) {
            // 효과 경계가 받침 비트를 켠 경우에도 잘못된 지도 인덱스는 조회하지 않는다.
            if (!inSpotRange) throw new RangeError('로컬 건설 spot 좌표가 지도 밖입니다');
            if ((this.#spots[spotY * MAP_SIDE + spotX] & 2) === 0) {
                const baseX = Math.fround(request.x + baseOffset(this.#types[island].footX));
                const baseY = Math.fround(request.y + baseOffset(this.#types[island].footY));
                const base = this.#allocate(noIsland, baseX, baseY, 0, 0);
                hooks.place({ type: noIsland, x: baseX, y: baseY, argument: 0, direction: 0, player: state.localPlayer, sids: base, abstract: 1, quality: 0 });
            }
        }
        hooks.place({ type, x: request.x, y: request.y, argument: request.argument, direction: request.direction, player: state.localPlayer, sids: main, abstract: 1, quality });
        // 배치 경계가 타입 플래그를 바꿀 수 있으므로 현재 값으로 사제 이동을 판정한다. 빈 디코더의 첫 SID 는 초기값 0 이다.
        if ((this.#types[type].flags1 & NEEDS_BUILDER) !== 0) hooks.moveBuilder(state.localPlayer, main.length === 0 ? 0 : main[0], 0, 0);
        hooks.send(SERVER_PLAYER, { type: type & 0xff, x: request.x, y: request.y, argument: request.argument & 0xffff, direction: request.direction & 0xff, quality: quality & 0xff, timeLow: 0, timeHigh: 0 });
    }
}

// 같은 풀의 실제 factory 와 건설 배치를 연결한다. 나머지 Form/이동/전송 경계는 받은 그대로 유지한다. 풀이 다르면 거부한다. (2026-11-09 WB04-E140)
export function makeConstructionLocalHooks(pool, factory, place, hooks) {
    if (pool !== factory?.pool || pool !== place?.pool) throw new TypeError('로컬 건설/factory/배치 SID 풀이 다릅니다');
    // 생성은 클라이언트 플래그를 그대로 전달하고 배치는 SID 목록을 호출 중에만 빌려 쓴다.
    return { ...hooks, create: (type, flags) => factory.create(type, flags), place: (request) => { place.place(request); } };
}
