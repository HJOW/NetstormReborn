// noIsland postPop(004423b0)의 최초 등록 접두(프레임 글자 선택·F 칸의 받침 생성·H 칸의 3×3 받침 소유자 전파·연결 순회)와 받침 소유자 전파(0044bd20)를 옮긴 모듈이다(cpppj `o/RawIslandPostPop.cpp/.h`, 10.78 전용).
// 생성·가상 소유자·가상 Pop·표시·연결 순회는 기존 raw 모듈(raw-bridge-connect.js 의 훅 묶음)과 호출자가 공급하며 이 모듈은 호출 순서와 인자만 원본대로 지킨다.
//  - prefix(surface, flags): 최초 등록(flags 비트 1)에서만 동작한다. 요새를 읽는 중(loadingDepth)이면 서/북 칸의 프레임 글자로 자신의 글자를 정해 프레임을 직접 쓰고(SetFrame 을 부르지 않음), 글자 F 이면 받침을 만들어 소유자를
//    주고 Pop 하며, 저장 게임 관문이 꺼져 있고 글자 H 이면 왼쪽 위 칸 받침의 소유자를 3×3 받침에 전파한 뒤 마지막에 표면 칸 연결 순회를 부른다.
//  - setSupportOwner(x, y, owner): (x, y)가 noIsland 이면 절삭한 오른쪽 아래 기준점의 3×3 칸을 x 먼저 y 다음 순서로 갱신하고 받침과 종유석을 갱신한다. 누락 진단은 모든 정상 칸의 효과 뒤에 호출한다.
//  - surfaceLetter(x, y): 범위 안의 한 칸이 noIsland 이면 프레임 방향 글자, 그 밖에는 0 이다.
// 훅 { objects: { create, setOwner, pop }, connect(sid), findTypeAt(x, y, type), updateDisplay(sid), incompleteSupport(missingCells, noIsland, noStalag), assertion(식, 파일, 줄), detail(문자열), typeWas() }.
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기(프레임 1바이트·H 조회 1.0001 보정·별도 소유자 지도)는 옮기지 않았다. (2026-11-08 WB04-E47: RawIslandPostPop 이식으로 추가)
import { CONNECT_CREATE_FLAGS } from './raw-bridge-connect.js';

// 슬롯 필드 위치(타입·좌표·소유자·프레임 DWORD)와 지도 한 변, 첫 프레임 검색의 변형/번호, 받침 묶음 한 변, 첫 자산 타입이다. (2026-11-08 WB04-E47)
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const SIDE = 256;
const VARIANT = 0x50;
const FRAME_NUMBER = 1;
const SUPPORT_SIZE = 3;
const FIRST_ASSET_TYPE = 70;

// 표면 조회가 더하는 실제 float 0.9999(0x3f7ff972)이다. (2026-11-08 WB04-E47)
const SURFACE_BIAS = Math.fround(0.9999);

// 정렬되지 않은 DWORD 읽기와 float 읽기, 프레임 DWORD 쓰기이다. (2026-11-08 WB04-E47)
const floatView = new DataView(new ArrayBuffer(4));
function read(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}
function coordinate(pool, sid, offset) {
    floatView.setUint32(0, read(pool.slot(sid), offset), true);
    return floatView.getFloat32(0, true);
}
function writeFrame(pool, sid, frame) {
    const raw = pool.physical(sid);
    const bits = frame >>> 0;
    // 패치의 DWORD 프레임 필드 네 바이트를 쓴다(원문의 직접 DWORD 저장은 물리 슬롯에 수행한다).
    for (let index = 0; index < 4; index += 1) raw[FRAME_OFFSET + index] = (bits >>> (8 * index)) & 0xff;
}

// 원본 _ftol 의 하위 DWORD(부호 있는 32비트)이다. 비유한/64비트 밖 입력은 정수 불확정값의 하위 0 이다. (2026-11-08 WB04-E47)
function cell(value) {
    if (!(value > -9223372036854775808 && value < 9223372036854775808)) return 0;
    return Math.trunc(value) | 0;
}

// 북쪽 칸에 따른 다음 행의 첫 글자이다. 서쪽의 알 수 없는 글자도 이 규칙을 따른다. (2026-11-08 WB04-E47)
const F = 0x46;
const B = 0x42;
const I = 0x49;
const fromNorth = (north) => (north === F ? B : north === B ? I : F);

// 원본 분기 순서대로 서쪽/북쪽 칸의 글자로 자신의 글자를 정한다. (2026-11-08 WB04-E47)
function nextLetter(west, north) {
    switch (west) {
    case 0x43: return 0x47; // C → G
    case 0x46: return 0x43; // F → C
    case 0x41: return 0x44; // A → D
    case 0x42: return 0x41; // B → A
    case 0x45: return 0x48; // E → H
    case 0x49: return 0x45; // I → E
    default: return fromNorth(north);
    }
}

// 도달한 장치만 복사해 요구하며 콜백이 자기 연결을 반납해도 현재 호출의 수명을 유지한다. (2026-11-08 WB04-E47)
function need(callback) {
    const selected = callback;
    if (typeof selected !== 'function') throw new Error('표면 후처리의 선택 장치 미연결');
    return selected;
}

// Native DEC 의 DWORD 순환 뒤 signed 값을 좌표 덧셈에 올린다. x87 FILD 는 중간 float 저장 없이 현재 좌표를 더한다. (2026-11-08 WB04-E47)
const footDelta = (footprint) => ((footprint >>> 0) - 1) | 0;

// noIsland 후처리 실행기이다. 지도는 256×256 SID 머리 배열(Uint16Array)이다. 모든 참조/지도는 이 객체보다 오래 살아야 한다.
//  new RawIslandPostPop(pool, surfaceMap, types, frames, typeState, state, hooks) — typeState: { islandType, stalagType, noIslandType }(BridgeConnectState),
//  state: { loadingDepth, savedGame() }(요새 읽기 깊이와 저장 게임 관문, 현재값을 매번 읽음), frames: 타입별 프레임 코드 배열.
// (2026-11-08 WB04-E47)
export class RawIslandPostPop {
    #pool;
    #surfaceMap;
    #types;
    #frames;
    #typeState;
    #state;
    #hooks;

    // 지도/타입 표의 물리 크기는 구성 때 확인한다. 현재 TYPE/장치는 실제로 도달한 호출 위치에서만 요구한다.
    constructor(pool, surfaceMap, types, frames, typeState, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('noIsland postPop 은 10.78 풀만 받습니다.');
        if (surfaceMap.length !== SIDE * SIDE || types.length !== frames.length) throw new RangeError('noIsland postPop 지도/타입 표 범위 오류');
        this.#pool = pool;
        this.#surfaceMap = surfaceMap;
        this.#types = types;
        this.#frames = frames;
        this.#typeState = typeState;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 지도 밖은 0, 지도 안은 머리 SID 그대로이다(Next 체인을 순회하지 않는다).
    #surfaceAt(x, y) {
        if (x < 0 || y < 0 || x >= SIDE || y >= SIDE) return 0;
        return this.#surfaceMap[y * SIDE + x];
    }

    // 현재 프레임 코드의 방향 글자를 검증해 읽는다. PATCH 는 form 타입이면 진단(0x854) 뒤 TYPE/프레임을 다시 읽고 글자를 signed char 로 해석한다.
    #letter(sid) {
        const pool = this.#pool;
        if (pool.slot(sid)[TYPE_OFFSET] < FIRST_ASSET_TYPE) need(this.#hooks.assertion)('!isForm()', '\\Ns\\O\\Squid.cpp', 0x854);
        const raw = pool.slot(sid);
        if (raw[TYPE_OFFSET] >= this.#frames.length) throw new RangeError('noIsland 프레임 타입 범위 오류');
        const codes = this.#frames[raw[TYPE_OFFSET]];
        const frame = read(raw, FRAME_OFFSET);
        if (frame >= codes.length) throw new RangeError('noIsland 프레임 번호 범위 오류');
        return (codes[frame].side << 24) >> 24;
    }

    // 00442350: 범위 안의 한 칸이 noIsland 이면 프레임 방향 글자, 그 밖에는 0 이다. 서/북 조회는 고정 표면 지도를 읽는다.
    surfaceLetter(x, y) {
        const sid = this.#surfaceAt(x, y);
        return sid && this.#pool.slot(sid)[TYPE_OFFSET] === this.#typeState.noIslandType ? this.#letter(sid) : 0;
    }

    // 최초 등록(flags & 1)에서만 프레임 선택·F 받침 생성·H 소유자 전파·연결 순회를 수행한다. 뒤따르는 공통 postPop 은 SquidPostPop.activate 가 같은 flags 로 호출한다.
    prefix(surface, flags) {
        if ((flags & 1) === 0) return;
        const pool = this.#pool;
        const hooks = this.#hooks;
        const typeState = this.#typeState;
        if (this.#state.loadingDepth) {
            // 1 을 뺀 뒤 절삭한다. 소수 좌표의 칸을 먼저 절삭한 뒤 빼지 않는다.
            const x = coordinate(pool, surface, X_OFFSET);
            const y = coordinate(pool, surface, Y_OFFSET);
            const west = this.surfaceLetter(cell(x - 1), cell(y));
            // PATCH 는 서쪽 조회의 진단 복귀 뒤 현재 좌표를 다시 읽는다.
            const north = this.surfaceLetter(cell(coordinate(pool, surface, X_OFFSET)), cell(coordinate(pool, surface, Y_OFFSET) - 1));
            const letter = nextLetter(west, north);
            const raw = pool.slot(surface);
            if (raw[TYPE_OFFSET] >= this.#frames.length) throw new RangeError('noIsland 선택 프레임 타입 범위 오류');
            let frame = -1;
            const codes = this.#frames[raw[TYPE_OFFSET]];
            // 방향·변형·번호의 첫 일치 프레임을 찾는다.
            for (let index = 0; index < codes.length; index += 1) {
                if (codes[index].side === letter && codes[index].variant === VARIANT && codes[index].number === FRAME_NUMBER) { frame = index; break; }
            }
            // 필수 검색은 진단 뒤에도 -1 을 반환하며 다시 검색하지 않는다.
            if (frame < 0) need(hooks.assertion)('0', '\\Ns\\O\\Rifttype.cpp', 299);
            // 원본은 SetFrame 을 호출하지 않고 프레임 필드를 직접 쓴다.
            writeFrame(pool, surface, frame);
            if (this.#letter(surface) !== letter) need(hooks.assertion)('getOrientation() == orientation', '\\Ns\\O\\Construction.cpp', 0xfa);
        }
        if (this.#letter(surface) === F) {
            const island = need(hooks.objects.create)(typeState.islandType, CONNECT_CREATE_FLAGS);
            if (typeState.islandType >= this.#types.length) throw new RangeError('noIsland 생성 뒤 받침 타입 범위 오류');
            // 생성 뒤 선택한 항목의 주소를 소유자 호출 동안 유지한다. 필드 값은 그 뒤에 읽는다.
            const type = this.#types[typeState.islandType];
            need(hooks.objects.setOwner)(island, pool.slot(surface)[OWNER_OFFSET]);
            need(hooks.objects.pop)(island, Math.fround(footDelta(type.footX) + coordinate(pool, surface, X_OFFSET)), Math.fround(footDelta(type.footY) + coordinate(pool, surface, Y_OFFSET)), 0);
        }
        if (!this.#state.savedGame() && this.#letter(surface) === 0x48) {
            const x = coordinate(pool, surface, X_OFFSET);
            const y = coordinate(pool, surface, Y_OFFSET);
            // 패치는 2 를 뺀 float 에 조회 함수가 0.9999 를 더한다.
            const hCell = (value) => cell(Math.fround(value - 2) + SURFACE_BIAS);
            const first = this.#surfaceAt(hCell(x), hCell(y));
            if (!first) need(hooks.assertion)('gSid', '\\Ns\\O\\Construction.cpp', 0x10a);
            else this.setSupportOwner(x, y, pool.slot(first)[OWNER_OFFSET]);
        }
        need(hooks.connect)(surface);
    }

    // (x,y)가 noIsland 인 경우 x/y 를 절삭한 오른쪽 아래 기준점의 3×3 칸을 x 먼저 y 다음 순서로 갱신한다. 이어 받침과 종유석을 갱신하며 누락 진단은 모든 정상 칸의 효과 뒤에 호출한다.
    setSupportOwner(x, y, owner) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const typeState = this.#typeState;
        const right = cell(x);
        const bottom = cell(y);
        // 진입 조건의 표면 조회는 0.9999 를 더해 절삭하고 실제 3×3 반복 기준점은 보정 없는 절삭값이다.
        const target = this.#surfaceAt(cell(x + SURFACE_BIAS), cell(y + SURFACE_BIAS));
        if (!target || pool.slot(target)[TYPE_OFFSET] !== typeState.noIslandType) return;
        let missingCells = false;
        let detail = '';
        // 원본은 x 를 바깥 반복문, y 를 안쪽 반복문으로 순회한다. 타입을 제한하지 않고 각 칸의 현재 머리 객체에 소유자와 표시 갱신을 준다.
        for (let cx = right - SUPPORT_SIZE + 1; cx <= right; cx += 1) {
            for (let cy = bottom - SUPPORT_SIZE + 1; cy <= bottom; cy += 1) {
                const sid = this.#surfaceAt(cx, cy);
                if (!sid) {
                    missingCells = true;
                    detail += '-';
                    continue;
                }
                need(hooks.objects.setOwner)(sid, owner);
                need(hooks.updateDisplay)(sid);
                const type = pool.slot(sid)[TYPE_OFFSET];
                detail += type === typeState.noIslandType ? 'X' : `[${type}]`;
            }
        }
        const island = need(hooks.findTypeAt)(x, y, typeState.islandType);
        if (island) {
            need(hooks.objects.setOwner)(island, owner);
            need(hooks.updateDisplay)(island);
        } else detail += ' (No islandSid) ';
        const stalag = need(hooks.findTypeAt)(x, y, typeState.stalagType);
        if (stalag) {
            need(hooks.objects.setOwner)(stalag, owner);
            need(hooks.updateDisplay)(stalag);
        } else detail += ' (No islandStalagSid) ';
        if (!(missingCells || !island || !stalag)) return;
        // 실제 World 는 상세 문자열 소유자와 진단 장치를 연결한다. 연결이 없는 독립 입력은 기존 경계(incompleteSupport)를 쓴다.
        if (!hooks.detail && !hooks.assertion) {
            need(hooks.incompleteSupport)(missingCells, !island, !stalag);
            return;
        }
        const typeWas = hooks.typeWas ? need(hooks.typeWas)() : 0;
        if (typeWas) detail += ` TypeWas ${typeWas | 0}`;
        need(hooks.detail)(detail);
        need(hooks.assertion)('0', '\\Ns\\O\\Damageable.cpp', 0x18b);
    }
}
