// 건설 요청의 유닛 밀어내기(00443800)와 섬 실패 정리(0046e6c0)를 옮긴 모듈이다(cpppj `o/RawConstructionEffects.cpp/.h`, 10.78). 공간 이동/삭제의 가상 효과는 호출자가 연결한다.
//  - displace(x, y, type, player): 건설 발자국 안의 유닛(genus 0x218000)을 아래/오른쪽 빈 칸으로 옮긴다. 발자국은 float 사각형(시작점이 지도 밖이면 자른 반대 점 하나)이고 폭이 3 이상이면
//    사방 1칸씩 줄인다. 작은 쪽 모서리는 0방향 절삭, 큰 쪽은 0.99999 를 더해 절삭해 실제 `RawSquidFinder` 에 넘긴다. 후보마다 현재 genus 를 다시 읽고, 가운데 우선(genus 0x200000)이면
//    (x−(폭−1)/2 정수 나눗셈, y+1) 칸을 먼저 본 뒤, 아래 세 줄(각 줄은 오른쪽부터 폭+1칸 역순)과 오른쪽 세 열(각 열은 아래부터 높이+1칸 역순)에서 spot 이 열린 칸(bit 0x10 없음, bit 2/4 중 하나)을 찾는다.
//    칸이 없으면 그 후보를 건너뛴다. 후보 소유자가 요청 플레이어와 다르면 관계 표(player×9+owner)가 0 이고 적대 허용 genus(0x8000)도 없을 때 거짓을 돌려준다(앞서 옮긴 후보는 되돌리지 않음).
//    옮길 때는 경로 contained 타입 조회 → 경로 contained 삭제 → Unpop(0) → genus 재조회(유효성만) → 정수 칸에 Pop 순서이다. 모든 후보를 처리하면 참이다.
//  - dropIsland(x, y, flags, player): 섬 실패 정리이다. 억제(blocked)면 아무 것도 하지 않는다. 서버 역할이고 flags&0x50 이 없으면 좌표를 ftol(좌표×65536)의 16.16 DWORD 로 방송한 뒤, 현재 섬 타입의 발자국으로 섬 범위를
//    finder 에 넘겨 소유자가 일치하는 후보를 처리한다: 표면 조각/종유석은 추락 표시와 소리(flags&0x1000 이 없을 때, 표시 뒤 현재 좌표) 뒤 flags|0x48 로 삭제, 그루터기는 flags|0x48 로 삭제, 섬은 flags 로 삭제,
//    삭제 뒤 현재 genus 에 0x10000 이 있으면 후속 알림. 조각이 하나라도 있었는데 표면 9·섬 1·종유석 1 이 아니면 원본 assert 보고(0x7b/0x7c/0x7d)를 한다.
//  - makeConstructionEffectsRequestHooks(pool, effects, hooks): 요청의 displace/displaceCd/cancel 세 효과를 이 몸체에 잇는다. 풀이 다르면 거부한다.
// 10.72(CD) 경로(탐색 거리 2, 소유자 +32, void 반환)와 10.37 은 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E135: 건설 효과 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 원본 거의 올림 상수(0.9999899864196777), 발자국 축소 폭 조건과 축소량, 섬 정리 메시지의 좌표 배율(00501ad0, 16.16 고정소수)이다. (2026-11-08 WB04-E135)
const ROUND_UP_BIAS = 0.9999899864196777;
const SHRINK_WIDTH = 3;
const SHRINK = 1;
const WIRE_SCALE = 65536;
// 이동 대상 genus(10.78), 가운데 우선 탐색, 적대 이동 허용 비트이다. (2026-11-08 WB04-E135)
const DISPLACE_GENUS = 0x218000;
const CENTER_FIRST = 0x200000;
const ENEMY_ALLOWED = 0x8000;
// 섬 삭제에서 더하는 bit 3/6, 추락 표시/소리 억제, 전파 억제, 후속 알림 비트이다. (2026-11-08 WB04-E135)
const PIECE_FLAGS = 0x48;
const QUIET_FALL = 0x1000;
const NO_BROADCAST = 0x50;
const NOTIFY_GENUS = 0x10000;
// raw 타입(+10)·좌표(+14/+18)·PATCH 소유자(+34) 위치, 지도 한 변, 밀어내기 탐색 거리, 타입 표 크기, 플레이어 수이다. (2026-11-08 WB04-E135)
const TYPE = 10;
const X_AT = 14;
const Y_AT = 18;
const OWNER = 34;
const MAP_SIDE = 256;
const DISTANCE = 3;
const TYPE_COUNT = 188;
const PLAYERS = 9;
// 필수 하위 효과 이름이다(생성 때 모두 확인한다). assertion/scan 은 선택이다. (2026-11-08 WB04-E135)
const HOOK_NAMES = Object.freeze(['pathType', 'clearPath', 'unpop', 'pop', 'fall', 'sound', 'destroy', 'notify', 'broadcast']);

// 건설 효과가 읽는 원본 관계 표와 섬 타입/모드 전역이다. alliances(00595200: [요청 플레이어×9+후보 소유자], 0 이외면 동맹)·섬/표면/종유석/그루터기 타입 번호(94/157/95/96)·
// blocked(0054db84: 섬 정리 전체 생략)·server(00540bc0: 섬 정리 메시지 전파)와 같은 이름의 shared* 함수(실제 Session 이 현재 전역을 빌려줌)이다.
// 사용: 월드마다 하나를 두고 RawConstructionEffects 에 넘긴다. 값은 사용 위치마다 다시 읽는다. (2026-11-08 WB04-E135)
export class ConstructionEffectsState {
    alliances = new Array(PLAYERS * PLAYERS).fill(0);
    islandType = 94;
    noIslandType = 157;
    stalagType = 95;
    stumpType = 96;
    blocked = false;
    server = false;
    sharedBlocked = null;
    sharedIslandType = null;
    sharedNoIslandType = null;
    sharedStalagType = null;
    sharedStumpType = null;
    sharedServer = null;

    // 진입 시 현재 억제 DWORD 를 한 번 읽는다. (2026-11-08 WB04-E135)
    blockedNow() { return this.sharedBlocked ? this.sharedBlocked() !== 0 : this.blocked; }
    // 각 원문 TYPE 비교/발자국 조회에서 같은 현재 섬 번호를 읽는다. (2026-11-08 WB04-E135)
    islandTypeNow() { return this.sharedIslandType ? this.sharedIslandType() >>> 0 : this.islandType; }
    // 시각/삭제 콜백 뒤의 표면 수량 판정도 현재 번호를 다시 읽는다. (2026-11-08 WB04-E135)
    noIslandTypeNow() { return this.sharedNoIslandType ? this.sharedNoIslandType() >>> 0 : this.noIslandType; }
    // 후보 비교 위치의 현재 종유석 번호이다. (2026-11-08 WB04-E135)
    stalagTypeNow() { return this.sharedStalagType ? this.sharedStalagType() >>> 0 : this.stalagType; }
    // 앞선 삭제 콜백 뒤 현재 그루터기 번호이다. (2026-11-08 WB04-E135)
    stumpTypeNow() { return this.sharedStumpType ? this.sharedStumpType() >>> 0 : this.stumpType; }
    // 섬 정리 방송 직전에 현재 서버 역할(boss 와 독립인 전파 권한)을 읽는다. (2026-11-08 WB04-E135)
    serverNow() { return this.sharedServer ? Boolean(this.sharedServer()) : this.server; }
}

// 원본 __ftol 의 하위 signed DWORD 이다. 비유한 값과 64비트 범위 밖은 하위 DWORD 가 0 이다. (2026-11-08 WB04-E135)
function ftol(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    const whole = Math.trunc(value);
    if (Math.abs(whole) <= 2147483647) return whole | 0;
    return Number(BigInt.asIntN(32, BigInt(whole)));
}

// 정렬과 무관한 리틀엔디언 float 좌표를 읽는다. (2026-11-08 WB04-E135)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 1~255 로 자르는 float32 clamp 이다(min/max 는 float 값이라 단정도로 보존). (2026-11-08 WB04-E135)
function clamp(value, low, high) {
    return Math.min(Math.max(value, low), high);
}

// 원본 발자국 helper 의 float 사각형 [left, top, right, bottom] 이다. 시작점이 지도 밖이면 자른 반대 점 하나로 축소한다. (2026-11-08 WB04-E135)
function footprint(x, y, width, height) {
    const px = clamp(Math.fround(x + 1 - width), 1, 255);
    const py = clamp(Math.fround(y + 1 - height), 1, 255);
    if (!(x > 0 && y > 0 && x < MAP_SIDE && y < MAP_SIDE)) return [px, py, px, py];
    return [Math.min(x, px), Math.min(y, py), Math.max(x, px), Math.max(y, py)];
}

// finder 에 전달할 모서리를 작은 쪽 절삭/큰 쪽 거의 올림으로 정수화한다. 지도 범위 보정은 finder 가 수행한다. (2026-11-08 WB04-E135)
function area(rectangle) {
    return { left: ftol(rectangle[0]), top: ftol(rectangle[1]), right: ftol(rectangle[2] + ROUND_UP_BIAS), bottom: ftol(rectangle[3] + ROUND_UP_BIAS) };
}

// 유한 좌표와 0..256 발자국을 검증한다. 원본 49ae80 은 Dude 의 foot 0 도 x..x+1 사각형으로 펼친다. (2026-11-08 WB04-E135)
function validate(x, y, type) {
    if (!Number.isFinite(x) || !Number.isFinite(y) || type.footX < 0 || type.footY < 0 || type.footX > 256 || type.footY > 256) throw new RangeError('건설 효과 좌표/발자국 오류');
}

// 건설 효과 몸체이다. pool(10.78 SidPool)·hash(SquidHash)·types(현재 타입 표 188칸)·spots(256×256 이상 Uint8Array)·state·hooks 는 이 객체보다 오래 살아야 하며 복사하지 않는다.
// 사용: const effects = new RawConstructionEffects(pool, hash, types, spots, state, hooks); effects.displace(20, 21, 100, 3); effects.dropIsland(20, 21, 0x1000, 3). (2026-11-08 WB04-E135)
export class RawConstructionEffects {
    #pool;
    #hash;
    #types;
    #spots;
    #state;
    #hooks;

    // 판본·타입 표 크기·지도 크기·필수 하위 효과를 확인한다. (2026-11-08 WB04-E135)
    constructor(pool, hash, types, spots, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('건설 효과는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !(spots?.length >= MAP_SIDE * MAP_SIDE) || !state) throw new TypeError('건설 효과 자료/경계 연결 오류');
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 효과 자료/경계 연결 오류');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#spots = spots;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 요청 어댑터가 같은 풀을 쓰는지 확인하는 참조이다. (2026-11-08 WB04-E135)
    get pool() { return this.#pool; }

    // 후보의 현재 raw 타입에서 genus 를 조회한다. 경계 뒤의 재조회를 위해 값을 캐시하지 않는다. (2026-11-08 WB04-E135)
    #genus(sid) {
        const type = this.#pool.slot(sid)[TYPE];
        if (type >= this.#types.length) throw new RangeError('건설 효과 후보 타입 오류');
        return this.#types[type].flags2 >>> 0;
    }

    // spot bit 0x10 이 없고 bit 2/4 중 하나가 있는 칸이면 이동 가능하다. 지도 밖은 거부한다. (2026-11-08 WB04-E135)
    #open(x, y) {
        if (x < 0 || y < 0 || x >= MAP_SIDE || y >= MAP_SIDE) throw new RangeError('건설 밀어내기 지도 조회 오류');
        const spot = this.#spots[y * MAP_SIDE + x];
        return (spot & 0x10) === 0 && (spot & 6) !== 0;
    }

    // 후보 하나가 옮겨 갈 빈 칸을 찾는다: 가운데 우선이면 가운데 칸, 아래 세 줄(오른쪽부터 폭+1칸 역순), 오른쪽 세 열(아래부터 높이+1칸 역순) 순이다. 못 찾으면 null 이다. (2026-11-08 WB04-E135)
    #findSpot(current, x, y, record) {
        if ((this.#genus(current) & CENTER_FIRST) !== 0) {
            // C++ 은 (폭−1)/2 를 정수 나눗셈으로 구한다.
            const cx = ftol(x - Math.trunc((record.footX - 1) / 2));
            const cy = ftol(y + 1);
            if (this.#open(cx, cy)) return { x: cx, y: cy };
        }
        const baseY = ftol(y);
        const baseX = ftol(x);
        // 아래쪽 줄을 가까운 순서로, 각 줄의 x 는 오른쪽부터 발자국 폭만큼 역순으로 찾는다.
        for (let row = baseY + 1; row <= baseY + DISTANCE; row += 1) {
            for (let column = baseX; column >= baseX - record.footX; column -= 1) {
                if (this.#open(column, row)) return { x: column, y: row };
            }
        }
        // 아래에서 못 찾으면 오른쪽 열을 가까운 순서로, y 는 아래부터 역순으로 찾는다.
        for (let column = baseX + 1; column <= baseX + DISTANCE; column += 1) {
            for (let row = baseY; row >= baseY - record.footY; row -= 1) {
                if (this.#open(column, row)) return { x: column, y: row };
            }
        }
        return null;
    }

    // 발자국 안의 유닛을 아래/오른쪽으로 옮긴다. 적대 후보에서 false 를 돌려주며, 실패 전에 처리한 후보의 효과는 되돌리지 않는다. 이동 자리가 없으면 그 후보를 건너뛴다. (2026-11-08 WB04-E135)
    displace(x, y, type, player) {
        if (type >= this.#types.length) throw new RangeError('건설 밀어내기 타입 오류');
        const record = this.#types[type];
        x = Math.fround(x);
        y = Math.fround(y);
        validate(x, y, record);
        const hooks = this.#hooks;
        const rectangle = footprint(x, y, record.footX, record.footY);
        if (rectangle[2] - rectangle[0] >= SHRINK_WIDTH) {
            rectangle[0] = Math.fround(rectangle[0] + SHRINK);
            rectangle[1] = Math.fround(rectangle[1] + SHRINK);
            rectangle[2] = Math.fround(rectangle[2] - SHRINK);
            rectangle[3] = Math.fround(rectangle[3] - SHRINK);
        }
        const search = area(rectangle);
        if (hooks.scan) hooks.scan(search);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 현재 finder 순서대로 옮긴다. 뒤의 적대 후보에서 false 여도 앞선 이동은 유지한다.
        for (let current = finder.begin(search); current !== 0; current = finder.next()) {
            if ((this.#genus(current) & DISPLACE_GENUS) === 0) continue;
            const spot = this.#findSpot(current, x, y, record);
            if (!spot) continue;
            const owner = this.#pool.slot(current)[OWNER];
            if (owner !== player) {
                if (owner >= PLAYERS || player >= PLAYERS) throw new RangeError('건설 밀어내기 관계 표 범위 오류');
                if (!this.#state.alliances[player * PLAYERS + owner] && (this.#genus(current) & ENEMY_ALLOWED) === 0) return false;
            }
            const pathType = hooks.pathType(this.#pool.slot(current)[TYPE]);
            hooks.clearPath(current, pathType);
            hooks.unpop(current, 0);
            // 원본은 Unpop 후 genus 를 재조회한다. 이 읽기의 유효성도 보존한다.
            this.#genus(current);
            hooks.pop(current, spot.x, spot.y, 0);
        }
        return true;
    }

    // 섬 발자국의 소유자 일치 후보를 정리하고 9/1/1 조각 수 조건을 보고한다. 실패 정리 flags 는 0x1000 이다. (2026-11-08 WB04-E135)
    dropIsland(x, y, flags, player) {
        const state = this.#state;
        const hooks = this.#hooks;
        // 원문 진입 억제 뒤 원격 전파를 먼저 실행한다.
        if (state.blockedNow()) return;
        x = Math.fround(x);
        y = Math.fround(y);
        if (state.serverNow() && (flags & NO_BROADCAST) === 0) hooks.broadcast(ftol(x * WIRE_SCALE) >>> 0, ftol(y * WIRE_SCALE) >>> 0, flags >>> 0, player);
        if (state.islandTypeNow() >= this.#types.length) throw new RangeError('전파 뒤 섬 정리 타입 오류');
        validate(x, y, this.#types[state.islandTypeNow()]);
        const island = this.#types[state.islandTypeNow()];
        const search = area([Math.fround(x - (island.footX - 1)), Math.fround(y - (island.footY - 1)), x, y]);
        if (hooks.scan) hooks.scan(search);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        let noIslands = 0;
        let islands = 0;
        let stalags = 0;
        // next 를 이미 저장한 finder 로 삭제 중에도 같은 탐색 순서를 유지한다.
        for (let current = finder.begin(search); current !== 0; current = finder.next()) {
            const raw = this.#pool.slot(current);
            if (raw[OWNER] !== player) continue;
            const type = raw[TYPE];
            if (type === state.noIslandTypeNow() || type === state.stalagTypeNow()) {
                if ((flags & QUIET_FALL) === 0) {
                    hooks.fall(current);
                    const after = this.#pool.slot(current);
                    hooks.sound(coordinate(after, X_AT), coordinate(after, Y_AT));
                }
                hooks.destroy(current, (flags | PIECE_FLAGS) >>> 0);
                if (type === state.noIslandTypeNow()) noIslands += 1;
                else stalags += 1;
            }
            if (type === state.stumpTypeNow()) hooks.destroy(current, (flags | PIECE_FLAGS) >>> 0);
            if (type === state.islandTypeNow()) {
                hooks.destroy(current, flags >>> 0);
                islands += 1;
            }
            if ((this.#genus(current) & NOTIFY_GENUS) !== 0) hooks.notify(current);
        }
        if (noIslands || islands || stalags) {
            if (noIslands !== 9 && hooks.assertion) hooks.assertion('noIslandTypeCount == 9', 0x7b);
            if (islands !== 1 && hooks.assertion) hooks.assertion('islandTypeCount == 1', 0x7c);
            if (stalags !== 1 && hooks.assertion) hooks.assertion('islandStalagTypeCount == 1', 0x7d);
        }
    }
}

// 건설 요청의 세 효과(displace/displaceCd/cancel)를 실제 밀어내기/섬 정리 몸체에 연결한다. MayPlace/확정/진단은 받은 값을 보존한다. 풀이 다르면 거부한다. (2026-11-08 WB04-E135)
export function makeConstructionEffectsRequestHooks(pool, effects, hooks) {
    if (pool !== effects?.pool) throw new TypeError('건설 효과 모듈 풀 불일치');
    // 원본 판본별 인자 수/반환 사용을 유지한 채 요청에 실제 몸체를 연결한다.
    return {
        ...hooks,
        displace: (x, y, type, player) => effects.displace(x, y, type, player),
        displaceCd: (x, y, type) => { effects.displace(x, y, type, 0); },
        cancel: (x, y, flags, player) => { effects.dropIsland(x, y, flags, player); },
    };
}
