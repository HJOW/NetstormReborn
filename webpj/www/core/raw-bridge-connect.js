// 다리/섬 연결(004213b0), 연결 객체 생성(004210f0), 소유자 전파(00421240)를 실제 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/RawBridgeConnect.cpp/.h`, 10.78 전용).
// 새 객체 생성·가상 소유자 지정·가상 Pop·프레임 지정·표면 알림은 호출자가 훅으로 연결하며 순서·인자만 원본대로 호출한다.
//  - connect(sid): flag 4(abstract 제외) 연결 이웃을 순회하며 다리↔섬 표면 쌍마다 연결 객체를 만들고 소유자를 전파한다. 자기가 다리이면 genus 가 0x1000002(섬/섬 받침)에 걸리는 이웃이,
//    아니면 다리인 이웃이 대상이다. 다리 쪽이 항상 첫 인자이다. 미리보기(connectPreview/preview 배열)는 연결 객체를 등록하지 않고 위치와 화면 좌표만 쓴 뒤 목록에 넣는다.
//  - link(first, second, owner): first 의 위치에서 second 의 (거의 올림한) 중심 쪽으로 한 칸 옮긴 곳에 연결 객체를 만든다. 프레임은 방향/2, +12 단어는 first, +8 단어는 second 이다.
//  - propagateOwner(first, second): second 위치의 섬 받침이 주인이 없으면 first 의 소유자를 주고, 받침 색 프레임 → 같은 위치 종유석 프레임 → 받침 발자국 안의 noIsland 소유자 순서로 전파한다.
//  - findTypeAt(x, y, type): 좌표를 자른 한 칸에서 일반 탐색 순서로 지정 타입의 첫 객체를 찾는다(island/bridge 가 아니면 해시 0단계를 건너뜀).
//  - islandPostPopPrefix/islandPostPop: 섬 받침 postPop 재정의(004421c0)의 접두(최초 등록에서 종유석 생성·소유자·Pop → 연결 순회)와 공통 후처리 호출.
//  - ownerColorFrame/setIslandOwner/makeOwnerDispatch: 소유자 색 프레임(004421a0)과 섬 받침·종유석의 소유자 지정 재정의(00442310).
// 풀·이웃 탐색기·상태는 이 객체보다 오래 살아야 한다. 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기(프레임 1바이트·오프셋)는 옮기지 않았다.
// (2026-11-08 WB04-E42: RawBridgeConnect 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { NEIGHBOR_FLAG, RawSquidNeighborWalk, RawSquidNeighbors } from './raw-squid-neighbors.js';

// 소유자 색이 없을 때의 섬 프레임 번호(중립)와 연결 객체/종유석 생성 flags, 다리가 연결을 만드는 상대 표면의 genus 마스크(island 2 + 섬 받침 계열 0x1000000)이다. (2026-11-08 WB04-E42)
export const NEUTRAL_OWNER_FRAME = 8;
export const CONNECT_CREATE_FLAGS = 2;
export const ISLAND_SURFACE_MASK = 0x01000002;

// 섬 받침·종유석 클래스의 가상 표 주소 기록값(10.78)과 소유자 색 표의 칸 수(0~8이 플레이어 번호)이다. 비교만 하고 호스트에서 역참조하지 않는다. (2026-11-08 WB04-E42)
export const ISLAND_VTABLE = 0x00506d98;
export const STALAG_VTABLE = 0x00506e50;
export const OWNER_COLOR_COUNT = 9;

// 슬롯 필드 위치(10.78): 두 번째 참조(+8)·타입(+10)·첫 참조(+12)·위치 x(+14)/y(+18)·화면 좌표(+22/+24)·소유자(+34)·프레임 DWORD(+36)·extra(+40). (2026-11-08 WB04-E42)
const SECOND_REFERENCE = 8;
const TYPE_OFFSET = 10;
const FIRST_REFERENCE = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const SCREEN_X = 22;
const SCREEN_Y = 24;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;

// extra 의 abstract(1)·buried(8) 마스크이다. (2026-11-08 WB04-E42)
const ABSTRACT_OR_BURIED = 9;

// 한 칸 조회가 0단계 해시부터 찾는 타입의 genus: island(2)·bridge(4)와 다리 genus 비트이다. (2026-11-08 WB04-E42)
const GENUS_ISLAND = 2;
const GENUS_BRIDGE = 4;
const SURFACE_LEVEL_GENUS = GENUS_ISLAND | GENUS_BRIDGE;

// 방향(0 북부터 시계 방향 8개)별 한 칸 이동량(0052f87c·0052f89c)이다. (2026-11-08 WB04-E42)
const STEP_X = Object.freeze([0, 1, 1, 1, 0, -1, -1, -1]);
const STEP_Y = Object.freeze([-1, -1, 0, 1, 1, 1, 0, -1]);

// 화면 좌표의 칸 크기와 반올림 값(00503358·0050334c·00503350)이다. (2026-11-08 WB04-E42)
const SCREEN_SCALE_X = 16;
const SCREEN_SCALE_Y = 11;
const SCREEN_BIAS = 0.5;

// 00502310 의 실제 float 비트(0x3f7fff58)이다. 중심과 사각형의 오른쪽/아래를 거의 올림하는 값이며 spot 조회의 0.9999 와 다르다. (2026-11-08 WB04-E42)
const biasView = new DataView(new ArrayBuffer(4));
biasView.setUint32(0, 0x3f7fff58, true);
const ROUND_UP_BIAS = biasView.getFloat32(0, true);

// 정렬되지 않은 리틀엔디언 정수 읽기(폭 1~4)와 float 비트 읽기이다. (2026-11-08 WB04-E42)
function read(raw, offset, width) {
    let value = 0;
    // 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 폭 밖의 바이트를 보존하면서 리틀엔디언 필드를 쓴다. (2026-11-08 WB04-E42)
function write(raw, offset, value, width) {
    // 낮은 바이트부터 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// raw 좌표 float 의 비트를 그대로 읽는다. (2026-11-08 WB04-E42)
const floatView = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    floatView.setUint32(0, read(raw, offset, 4), true);
    return floatView.getFloat32(0, true);
}

// 단정도 값을 raw 비트(uint32)로 바꾼다. (2026-11-08 WB04-E42)
function floatBits(value) {
    floatView.setFloat32(0, value, true);
    return floatView.getUint32(0, true);
}

// 원본 CRT _ftol: 64비트 절삭값의 하위 DWORD(부호 있는 32비트)이다. 비유한/64비트 밖은 정수 불확정값이라 0 이다. (2026-11-08 WB04-E42)
function ftol(value) {
    if (!(value > -9223372036854775808 && value < 9223372036854775808)) return 0;
    return Math.trunc(value) | 0;
}

// 0041d7a0: 0.99999 를 더한 x87 값을 자른 정수이다. 중간값을 단정도로 좁히지 않는다. (2026-11-08 WB04-E42)
const roundUp = (value) => ftol(value + ROUND_UP_BIAS);

// 프레임 DWORD(부호 있는 번호)를 읽고 쓴다. (2026-11-08 WB04-E42)
const readFrame = (pool, sid) => read(pool.slot(sid), FRAME_OFFSET, 4) | 0;
const writeFrame = (pool, sid, frame) => write(pool.physical(sid), FRAME_OFFSET, frame >>> 0, 4);

// 연결 함수들이 읽는 원본 전역이다: bridgeConnector/island/islandStalag/noIsland/bridge 타입 번호, 전투 모드, 미션 중, 소유자 색 표(00531b08 초기값 0..8).
// 사용: Object.assign(new BridgeConnectState(), { connectorType: 155, islandType: 94 }). (2026-11-08 WB04-E42)
export class BridgeConnectState {
    connectorType = 0;
    islandType = 0;
    stalagType = 0;
    noIslandType = 0;
    bridgeType = 0;
    battle = false;
    mission = false;
    ownerColors = [0, 1, 2, 3, 4, 5, 6, 7, 8];
}

// 004421a0: 소유자의 색 프레임이다. 전투 모드이고 번호가 양수이면 색 표 값 - 1(DWORD 감김), 아니면 중립(8)이다. 색 표 밖의 번호는 거부한다. (2026-11-08 WB04-E42)
export function ownerColorFrame(state, owner) {
    if (!state.battle || owner <= 0) return NEUTRAL_OWNER_FRAME;
    if (owner >= state.ownerColors.length) throw new RangeError('소유자 색 표 범위 오류');
    return ((state.ownerColors[owner] >>> 0) - 1) | 0;
}

// 00442310: base 소유자 지정을 부른 뒤 프레임 필드를 소유자 색으로 직접 쓴다(표시 갱신 없음). 미션 중이고 base 뒤의 프레임이 중립이면 그대로 둔다. (2026-11-08 WB04-E42)
export function setIslandOwner(pool, state, sid, owner, base) {
    if (typeof base !== 'function') throw new TypeError('섬 소유자 지정의 base 훅 누락');
    base(sid, owner);
    if (state.mission && readFrame(pool, sid) === NEUTRAL_OWNER_FRAME) return;
    writeFrame(pool, sid, ownerColorFrame(state, owner | 0));
}

// 가상 소유자 지정(+0x74)을 객체의 가상 표 기록값으로 분배한다: 섬 받침·종유석이면 재정의를, 아니면 base 를 부른다. pool/state 는 반환 함수보다 오래 살아야 한다. (2026-11-08 WB04-E42)
export function makeOwnerDispatch(pool, state, base) {
    if (typeof base !== 'function') throw new TypeError('소유자 분배의 base 훅 누락');
    return (sid, owner) => {
        // 슬롯 맨 앞 DWORD 가 가상 표 주소 기록값이다.
        const vtable = read(pool.slot(sid), 0, 4);
        if (vtable === ISLAND_VTABLE || vtable === STALAG_VTABLE) setIslandOwner(pool, state, sid, owner, base);
        else base(sid, owner);
    };
}

// 다리/섬 연결 처리기이다. hooks = { create(type, flags) → sid, setOwner(sid, owner), pop(sid, x, y, flags), setFrame(sid, frame, flags), notifySurface(sid) } 가 모두 있어야 한다.
// 사용: new RawBridgeConnect(pool, neighbors, state, hooks).connect(sid). (2026-11-08 WB04-E42)
export class RawBridgeConnect {
    #pool;
    #neighbors;
    #state;
    #hooks;

    // 훅 누락과 다른 풀의 이웃 탐색기는 거부한다.
    constructor(pool, neighbors, state, hooks) {
        if (neighbors.pool !== pool) throw new TypeError('다리 연결의 이웃 탐색기 풀이 다르다');
        if (!hooks?.create || !hooks.setOwner || !hooks.pop || !hooks.setFrame || !hooks.notifySurface) throw new TypeError('다리 연결 훅이 모두 연결되어야 한다');
        this.#pool = pool;
        this.#neighbors = neighbors;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 커서/건설 연결기가 같은 실제 풀의 연결 객체를 생성하는지 확인하기 위한 풀이다.
    get pool() { return this.#pool; }

    // 객체 타입의 genus(flags2)이다. 타입 번호가 표 밖이면 원본은 표 뒤의 메모리를 읽으므로 거부한다.
    #genus(sid) {
        const types = this.#neighbors.types();
        const number = this.#pool.slot(sid)[TYPE_OFFSET];
        if (number >= types.length) throw new RangeError('다리 연결 타입 범위 오류');
        return types[number].flags2 >>> 0;
    }

    // 004b1fa0: 좌표를 자른 한 칸에서 일반 탐색 순서로 지정 타입의 첫 객체를 찾는다. 없으면 0 이다. island/bridge genus 가 아니면 해시 0단계를 건너뛴다.
    findTypeAt(x, y, type) {
        const types = this.#neighbors.types();
        if (type >= types.length) throw new RangeError('한 칸 조회 타입 범위 오류');
        const cellX = ftol(x);
        const cellY = ftol(y);
        const finder = new RawSquidFinder(this.#pool, this.#neighbors.hash, types);
        // 원본은 현재 번호가 풀 범위 안인 동안 타입을 비교하고 다르면 일반 Next 로 넘어간다.
        for (let current = finder.begin({ left: cellX, top: cellY, right: cellX, bottom: cellY }, (types[type].flags2 & SURFACE_LEVEL_GENUS) === 0 ? 1 : 0); current !== 0; current = finder.next()) {
            if (this.#pool.slot(current)[TYPE_OFFSET] === type) return current;
        }
        return 0;
    }

    // 004210f0: first 의 위치에서 second 의 중심 쪽으로 한 칸 옮긴 곳에 연결 객체를 만든다. 원본은 두 객체의 타입을 검사하지 않는다.
    // preview(배열) 또는 append(함수)가 있으면 등록하지 않고 위치/화면 좌표만 쓴 뒤 목록에 넣는다(참조 단어는 쓰지 않음).
    link(first, second, owner, preview = null, append = null) {
        const hooks = this.#hooks;
        const born = hooks.create(this.#state.connectorType, CONNECT_CREATE_FLAGS);
        // first 의 위치를 읽고 second 의 중심을 거의 올림한 정수 좌표로 만든다.
        const origin = this.#pool.slot(first);
        let x = coordinate(origin, X_OFFSET);
        let y = coordinate(origin, Y_OFFSET);
        const center = this.#neighbors.center(second);
        const targetX = Math.fround(roundUp(center[0]));
        const targetY = Math.fround(roundUp(center[1]));
        // first 에서 그 중심을 보는 네 방향으로 한 칸 옮긴다. 합은 x87 에서 구해 단정도로 저장한다.
        const direction = RawSquidNeighbors.direction(x, y, targetX, targetY);
        x = Math.fround(STEP_X[direction] + x);
        y = Math.fround(STEP_Y[direction] + y);
        write(this.#pool.physical(born), FRAME_OFFSET, (direction / 2) >>> 0, 4);
        hooks.setOwner(born, owner);
        if (preview === null && append === null) {
            hooks.pop(born, x, y, 0);
            // 004ac220: 첫 참조 단어를 쓰고 연결 객체의 타입이 다리 타입 전역과 같으면 표면 알림을 부른다.
            const raw = this.#pool.physical(born);
            write(raw, FIRST_REFERENCE, first, 2);
            if (raw[TYPE_OFFSET] === this.#state.bridgeType) hooks.notifySurface(born);
            write(this.#pool.physical(born), SECOND_REFERENCE, second, 2);
            return born;
        }
        // 미리보기: 등록하지 않고 위치와 화면 좌표만 쓴 뒤 목록에 넣는다. 참조 단어는 쓰지 않는다.
        const raw = this.#pool.physical(born);
        write(raw, X_OFFSET, floatBits(x), 4);
        write(raw, Y_OFFSET, floatBits(y), 4);
        write(raw, SCREEN_X, ftol(x * SCREEN_SCALE_X + SCREEN_BIAS) >>> 0, 2);
        write(raw, SCREEN_Y, ftol(y * SCREEN_SCALE_Y + SCREEN_BIAS) >>> 0, 2);
        if (append) append(born);
        else preview.push(born);
        // 원본은 이 뒤에 004b2070 으로 그 칸의 객체를 조회하지만 결과를 쓰지 않는다. 상태를 바꾸지 않으므로 옮기지 않았다.
        return born;
    }

    // 00421240: second 위치의 섬 받침이 주인이 없으면 first 의 소유자를 준다. 받침 소유자·색 프레임 → 같은 위치 종유석 프레임 → 받침 발자국 안 noIsland 소유자 순서이다.
    propagateOwner(first, second) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const state = this.#state;
        const target = pool.slot(second);
        const base = this.findTypeAt(coordinate(target, X_OFFSET), coordinate(target, Y_OFFSET), state.islandType);
        // first 가 abstract/buried 이거나 섬 받침이 없으면 끝이다. 받침에 이미 주인이 있거나 abstract/buried 여도 끝이다.
        if ((pool.slot(first)[EXTRA_OFFSET] & ABSTRACT_OR_BURIED) !== 0 || base === 0) return;
        if (pool.slot(base)[OWNER_OFFSET] !== 0 || (pool.slot(base)[EXTRA_OFFSET] & ABSTRACT_OR_BURIED) !== 0) return;
        const owner = pool.slot(first)[OWNER_OFFSET];
        if (owner === 0) throw new Error('newPlayerId != INVALID_PLAYER_ID (Bridge.cpp assert)');
        hooks.setOwner(base, owner);
        hooks.setFrame(base, ownerColorFrame(state, owner), 0);
        // 받침의 현재 위치에서 종유석을 찾아 받침의 (방금 바뀐) 프레임을 그대로 준다.
        const island = pool.slot(base);
        const stalag = this.findTypeAt(coordinate(island, X_OFFSET), coordinate(island, Y_OFFSET), state.stalagType);
        if (stalag !== 0) hooks.setFrame(stalag, readFrame(pool, base), 0);
        // 0041def0(발자국, 0): 왼쪽/위는 자르고 오른쪽/아래는 거의 올림한 정수 사각형으로 일반 탐색을 한다.
        const foot = this.#neighbors.footprint(base);
        const finder = new RawSquidFinder(pool, this.#neighbors.hash, this.#neighbors.types());
        // 발자국에 걸리는 모든 객체 가운데 noIsland 타입에만 소유자를 준다. 훅이 바꾼 뒤의 체인/타입을 매번 다시 읽는다.
        for (let current = finder.begin({ left: ftol(foot.left), top: ftol(foot.top), right: roundUp(foot.right), bottom: roundUp(foot.bottom) }); current !== 0; current = finder.next()) {
            if (pool.slot(current)[TYPE_OFFSET] === state.noIslandType) hooks.setOwner(current, owner);
        }
    }

    // 004213b0: flag 4(abstract 제외) 연결 이웃을 순회하며 다리↔섬 표면 쌍마다 연결 객체를 만들고 소유자를 전파한다. preview 배열이 있으면 연결 객체를 등록하지 않는다.
    connect(sid, preview = null) { this.#connectImpl(sid, preview, null); }

    // 원본의 출력 배열 쓰기를 소유권 전파/다음 이웃 생성보다 뒤로 미루지 않는 미리보기이다. append(born) 소비자가 필요하다.
    connectPreview(sid, append) {
        if (typeof append !== 'function') throw new TypeError('다리 미리보기 SID 소비자 누락');
        this.#connectImpl(sid, null, append);
    }

    // 두 호출 방식 모두 실제 이웃 순회와 같은 시점의 소유권 전파를 수행한다.
    #connectImpl(sid, preview, append) {
        // 자기 genus 로 상대를 정한다. 탐색기를 만들기 전에 한 번만 읽는다.
        const bridge = (this.#genus(sid) & GENUS_BRIDGE) !== 0;
        const mask = bridge ? ISLAND_SURFACE_MASK : GENUS_BRIDGE;
        const walk = new RawSquidNeighborWalk(this.#neighbors, sid, NEIGHBOR_FLAG.skipAbstract);
        // 연결 객체의 등록과 소유자 지정이 풀·해시를 바꾸므로 다음 이웃은 그 뒤의 상태에서 읽는다.
        for (let other = walk.current; other !== 0; other = walk.next()) {
            if ((this.#genus(other) & mask) === 0) continue;
            // 다리 쪽이 첫 인자이고 연결 객체의 소유자는 다리 쪽의 현재 소유자이다.
            const first = bridge ? sid : other;
            const second = bridge ? other : sid;
            this.link(first, second, this.#pool.slot(first)[OWNER_OFFSET], preview, append);
            this.propagateOwner(first, second);
        }
    }

    // 004421c0 의 앞부분(섬 받침 postPop 재정의): 최초 등록(flags 비트 1)이면 같은 위치에 종유석을 만들어 소유자와 Pop 을 준 뒤 연결 순회를 한다. 뒤따르는 공통 postPop 은 호출자가 부른다.
    islandPostPopPrefix(island, flags) {
        if ((flags & 1) === 0) return;
        const hooks = this.#hooks;
        const stalag = hooks.create(this.#state.stalagType, CONNECT_CREATE_FLAGS);
        hooks.setOwner(stalag, this.#pool.slot(island)[OWNER_OFFSET]);
        // 소유자 지정 뒤의 위치를 읽어 같은 자리에 등록한다.
        const raw = this.#pool.slot(island);
        hooks.pop(stalag, coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET), 0);
        this.connect(island);
    }

    // 004421c0 전체: 접두 뒤에 같은 flags 로 공통 postPop(base)을 부른다. 생성/소유자/Pop/연결 뒤 도달한 공통 장치만 요구하며 앞선 효과를 되돌리지 않는다.
    islandPostPop(island, flags, base) {
        this.islandPostPopPrefix(island, flags);
        if (typeof base !== 'function') throw new TypeError('섬 받침 postPop의 공통 후처리 훅 누락');
        base(island, flags);
    }
}
