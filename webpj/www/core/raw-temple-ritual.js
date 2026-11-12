// 신전 의식의 현재 단계·SHP 목표점·사제/룬 조회와 실제 하위 요청 순서를 옮긴 모듈이다(cpppj `o/RawTempleRitual.cpp/.h`, 10.78 전용: 00447e40 의식 전체·00447aa0 단계 목표점).
//  - new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, hitPoints, actions, state, hooks): types 는 188칸 TYPE 레코드({ flags1, flags2 }), frames 는 188칸 FrameCodeTable,
//    shapes 는 188칸 SHP 자료({ loaded, frameCount, frames[{ muzzleX, muzzleY }] } — 의식 목표점은 헤더 +0x20/+0x22 포구 쌍이며 표시 기준점이 아님), spots 는 지면 지도(256×256 바이트),
//    footprint 는 같은 풀/해시의 RawSquidNeighbors, hitPoints 는 같은 풀의 SquidReward(최대 HP), actions 는 같은 풀의 RawPriestRitualActions, state 는 TempleRitualState 이다.
//    hooks = { create(type, flags)→SID, setHitPoints(sid, 값), setOwner(sid, 소유자), pop(sid, x, y, flags), playAt(x, y, 소리 이름, flags), reserveShared()→bool, addShared(sid, 사건, 지연),
//    count(sid)→1..3(가상 +0x80), assertion(식, 파일, 줄) } 이며 도달한 장치만 요구한다.
//  - count(temple): 현재 TYPE 의 프레임 표에서 현재 프레임(signed DWORD)의 방향 글자로 N→1·M→2·L→3, 그 밖은 프레임 64 미만 1·119 이하 2·그 위 3 이다.
//  - stage(temple, frame = −1): 현재 프레임(−1)을 저장한 뒤 가상 count 를 한 번 조회하고(`hooks.count`, 없으면 실제 Dais 가상 표 0x507950 일 때만 직접 count) count 별 첫 프레임 [1, 3, 64, 120] 을 빼
//    11/22/33/44/54 경계로 0..5 를 돌려준다. stageFromCount(frame, count)는 이미 저장한 값으로 같은 표만 읽는다.
//  - stageAnchor(temple, stage): 신전 좌표 + 첫 count(1) 구간 첫 프레임 3 + 단계 오프셋 [0, 11, 22, 33, 44] 의 SHP 포구 쌍 ÷ 현재 픽셀 배율(각 축을 단정도로 저장). stagePoint: Anchor + 룬 SHP 포구 쌍이며
//    단계 4 이고 절삭 칸의 지도 값이 `(spot&6) && !(spot&8)` 이면 (−9, −6) 픽셀을 더 보정한다.
//  - runeAt(temple, point): 현재 단계의 룬 SHP 포구를 빼고 일반 점 finder 에서 첫 현재 룬 TYPE 슬롯을 찾는다(룬 flags2 의 비트 1·2 가 없으면 표면 건너뛰기). findStrongOwnedPriest: 신전 발자국 ±1 에서 같은 소유자이고
//    HP ≥ 최대 HP ÷ 2 인 첫 사제. hasAbstractRune: 발자국의 첫 룬 중 extra 비트 0 이 선 것이 있는지. spawnRune: 생성→HP 0→현재 단계 프레임→void 진단→abstract 1→현재 소유자→현재 단계 Anchor 에 Pop.
//  - ritual(temple, priest, mode): 목표점 일치(현재 단계, 다음 단계는 단계 4 미만일 때만)와 룬 없음 뒤 mode 0 은 사제 동작 시작·단계별 위치 음향·0x307 공유 예약, 그 밖은 강한 사제의 후속과 권한이 있고 추상 룬이 없으면 룬 생성.
//  - makeTempleRitualHooks(pool, factory, owner, host, hooks): 실제 Factory 생성·공통 HP setter·Owner·Host 공유 Regular 를 연결한 새 묶음을 돌려준다(풀이 다르면 거부, 나머지 훅 보존).
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E365: 7-P3 RawTempleRitual 이식으로 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { pathDirection } from './raw-path-animation.js';
import { setRawSquidHitPoints } from './raw-squid-word.js';

// 실제 Dais 가상 표(10.78)와 슬롯 필드 위치(TYPE·상태·소유자·extra·HP·프레임·좌표)이다. (2026-11-13 WB04-E365)
const DAIS_VTABLE = 0x507950;
const TYPE_AT = 10;
const STATE_AT = 11;
const OWNER_AT = 34;
const EXTRA_AT = 40;
const HP_AT = 26;
const FRAME_AT = 36;
const X_AT = 14;
const Y_AT = 18;
const TYPE_COUNT = 188;
// count(1..3) 별 첫 프레임(0 번 칸도 저장된 1 을 쓴다)·단계 오프셋·단계별 위치 음향과 0x307 지연(float)·거의 올림 상수, 의식 후속 사건이다. (2026-11-13 WB04-E365)
const COUNT_FIRST = Object.freeze([1, 3, 64, 120]);
const STAGE_OFFSETS = Object.freeze([0, 11, 22, 33, 44]);
const STAGE_SOUNDS = Object.freeze(['forWind2.wav', 'forSun2.wav', 'forRain2.wav', 'forThunder2.wav', 'forStorm2.wav']);
const STAGE_DELAYS = Object.freeze([1.9600000381469727, 2.25, 1.8799999952316284, 2.2200000286102295, 2.4100000858306885]);
const ALMOST_CEIL = 0.9999899864196777;
const RITUAL_EVENT = 0x307;
const fround = Math.fround;
const INT32_MIN = -2147483648;

// 부호 없는 리틀엔디언 값을 읽는다. (2026-11-13 WB04-E365)
function read(raw, at, width = 4) {
    let value = 0;
    // 지정한 폭의 바이트만 낮은 자리부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// float 비트 변환용 공용 보기이다. (2026-11-13 WB04-E365)
const view = new DataView(new ArrayBuffer(4));

// 슬롯의 좌표 float 를 읽는다. (2026-11-13 WB04-E365)
function coordinate(raw, at) {
    view.setUint32(0, read(raw, at), true);
    return view.getFloat32(0, true);
}

// 정상 좌표 범위를 0 쪽으로 절삭한다. 손상/무한 좌표의 원본 불확정 읽기는 실행하지 않는다. (2026-11-13 WB04-E365)
function cell(value) {
    if (!Number.isFinite(value) || value < INT32_MIN || value >= 2147483648) throw new RangeError('신전 의식 좌표 절삭 범위');
    return Math.trunc(value);
}

// 선택된 장치만 요구하고 없으면 거부한다. (2026-11-13 WB04-E365)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('의식 helper의 선택된 실제 장치 미연결');
    return hook;
}

// 의식 helper 가 효과 뒤에도 다시 읽는 현재 원본 전역이다. 장기 소유자를 빌리는 값은 `{ value }` 상자이다. (2026-11-13 WB04-E365)
export class TempleRitualState {
    // 00540bc4 권한, 005412d0/005412f0 의 현재 사제/룬 타입 번호, 0059a92c/930 의 SHP 정수 핫스폿 → 월드 좌표 배율이다.
    boss = true;
    priestType = 158;
    runeType = 166;
    pixelScaleX = 16;
    pixelScaleY = 11;
    // 같은 Client 의 현재 룬/사제 TYPE 와 권한을 빌리는 상자이며 없으면 위의 지역 값을 쓴다.
    sharedRuneType = null;
    sharedPriestType = null;
    sharedBoss = null;

    // 지정한 값만 덮어쓴다. (2026-11-13 WB04-E365)
    constructor(init = {}) { Object.assign(this, init); }

    // 소비 위치마다 현재 값을 읽는다. (2026-11-13 WB04-E365)
    get currentPriestType() { return this.sharedPriestType ? this.sharedPriestType.value : this.priestType; }

    get currentRuneType() { return this.sharedRuneType ? this.sharedRuneType.value : this.runeType; }

    get currentBoss() { return this.sharedBoss ? this.sharedBoss.value : this.boss; }
}

// 신전 의식 실행기이다. (2026-11-13 WB04-E365)
export class RawTempleRitual {
    #pool;
    #hash;
    #types;
    #frames;
    #shapes;
    #spots;
    #footprint;
    #hitPoints;
    #actions;
    #state;
    #hooks;

    // 같은 풀/해시/표 크기를 확인한다(PATCH 효과는 도달한 위치에서 요구). (2026-11-13 WB04-E365)
    constructor(pool, hash, types, frames, shapes, spots, footprint, hitPoints, actions, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('신전 의식은 PATCH 만 복원했습니다');
        if (pool !== footprint.pool || hash !== footprint.hash || pool !== hitPoints.pool || pool !== actions.pool) throw new TypeError('신전 의식 실제 하위 연결의 풀/해시 불일치');
        if (types.length !== TYPE_COUNT || frames.length !== TYPE_COUNT || shapes.length !== TYPE_COUNT || spots.length !== WORLD_CELLS * WORLD_CELLS) throw new RangeError('신전 의식 표 크기 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#shapes = shapes;
        this.#spots = spots;
        this.#footprint = footprint;
        this.#hitPoints = hitPoints;
        this.#actions = actions;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 동일 월드의 PathCleanup/Host 연결을 검증하는 읽기 참조이다. (2026-11-13 WB04-E365)
    get pool() { return this.#pool; }

    // 현재 프레임(signed DWORD)이다. (2026-11-13 WB04-E365)
    #currentFrame(sid) { return read(this.#pool.slot(sid), FRAME_AT) | 0; }

    // 효과 뒤의 현재 좌표를 raw 에서 다시 읽는다. (2026-11-13 WB04-E365)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return { x: coordinate(raw, X_AT), y: coordinate(raw, Y_AT) };
    }

    // 원본 effect 뒤에도 현재 소유자를 읽도록 값을 보관하지 않는다. (2026-11-13 WB04-E365)
    #owner(sid) { return this.#pool.slot(sid)[OWNER_AT]; }

    // 실제 Dais +0x80: 현재 TYPE 의 프레임 표에서 현재 프레임의 방향 글자(N→1, M→2, L→3)를 먼저 보고 아니면 프레임 구간으로 1..3 을 돌려준다. (2026-11-13 WB04-E365)
    count(temple) {
        const raw = this.#pool.slot(temple);
        if (raw[TYPE_AT] >= this.#types.length || raw[TYPE_AT] >= this.#frames.length) throw new RangeError('Dais Count 현재 TYPE/프레임 표 범위 오류');
        const frame = this.#currentFrame(temple);
        const direction = pathDirection(this.#frames[raw[TYPE_AT]], frame);
        if (direction === 13) return 1;
        if (direction === 12) return 2;
        if (direction === 11) return 3;
        return frame < 64 ? 1 : frame <= 119 ? 2 : 3;
    }

    // 같은 대상의 현재 가상 +0x80 을 조회한다. 연결하지 않은 독립 입력은 실제 Dais 가상 표일 때만 직접 count 를 쓴다. (2026-11-13 WB04-E365)
    #virtualCount(temple) {
        if (this.#hooks.count) return this.#hooks.count(temple);
        if (read(this.#pool.slot(temple), 0) !== DAIS_VTABLE) throw new Error('의식 Stage의 현재 가상80 분배 장치 미연결');
        return this.count(temple);
    }

    // frame −1 의 현재 프레임도 가상 조회 전에 저장한다. 조회 뒤에는 반환 count 로만 원문 표를 선택한다. (2026-11-13 WB04-E365)
    stage(temple, frame = -1) {
        if (frame === -1) frame = this.#currentFrame(temple);
        const count = this.#virtualCount(temple);
        return RawTempleRitual.stageFromCount(frame, count);
    }

    // 가상 조회의 부작용 뒤 저장한 frame/count 만 소비하며 새로운 조회를 추가하지 않는다. (2026-11-13 WB04-E365)
    static stageFromCount(frame, count) {
        if (count >= COUNT_FIRST.length) throw new RangeError('의식 Stage의 가상 count 첫 프레임 표 범위 오류');
        const relative = (frame - COUNT_FIRST[count]) | 0;
        if (relative < 11) return 0;
        if (relative < 22) return 1;
        if (relative < 33) return 2;
        if (relative < 44) return 3;
        return relative <= 54 ? 4 : 5;
    }

    // 원본 419850 반환 헤더 +20/+22 의 의식 목표점(포구 쌍)이다. 표시 기준점과 구별한다. (2026-11-13 WB04-E365)
    #hotspot(type, frame) {
        if (type >= this.#shapes.length || frame < 0) throw new RangeError('신전 의식 SHP 타입/프레임 범위');
        const shape = this.#shapes[type];
        const limit = shape.frameCount * ((this.#types[type].flags1 & 0x440000) ? 2 : 1);
        if (!shape.loaded || frame >= limit || frame >= shape.frames.length) throw new RangeError('신전 의식 SHP frameCheck 오류');
        const current = shape.frames[frame];
        return [current.muzzleX, current.muzzleY];
    }

    // 원본 x87 의 int/float+float 계산은 각 축 저장에서만 단정도로 좁힌다. (2026-11-13 WB04-E365)
    #addPixels(point, x, y) {
        const state = this.#state;
        if (!Number.isFinite(state.pixelScaleX) || !Number.isFinite(state.pixelScaleY) || state.pixelScaleX === 0 || state.pixelScaleY === 0) throw new RangeError('신전 의식 픽셀 배율 오류');
        return { x: fround(x / state.pixelScaleX + point.x), y: fround(y / state.pixelScaleY + point.y) };
    }

    // 00447aa0: 신전의 첫 count 1 구간 핫스폿이 모든 단계 Anchor 의 기준이다. 현재 count 별 첫 프레임을 넣지 않는다. (2026-11-13 WB04-E365)
    stageAnchor(temple, stage) {
        if (stage >= STAGE_OFFSETS.length) throw new RangeError('신전 의식 numBridgesMissing<=4 assert');
        const hotspot = this.#hotspot(this.#pool.slot(temple)[TYPE_AT], COUNT_FIRST[1] + STAGE_OFFSETS[stage]);
        return this.#addPixels(this.#position(temple), hotspot[0], hotspot[1]);
    }

    // 단계 4 의 지면 검사는 거의 올림이 아닌 원본 __ftol 절삭 칸이다. (2026-11-13 WB04-E365)
    stagePoint(temple, stage) {
        let point = this.stageAnchor(temple, stage);
        const hotspot = this.#hotspot(this.#state.currentRuneType, stage);
        point = this.#addPixels(point, hotspot[0], hotspot[1]);
        if (stage === 4) {
            const index = cell(point.y) * WORLD_CELLS + cell(point.x);
            if (index < 0 || index >= this.#spots.length) throw new RangeError('신전 의식 마지막 목표 지면 범위');
            const spot = this.#spots[index];
            if ((spot & 6) && !(spot & 8)) point = this.#addPixels(point, -9, -6);
        }
        return point;
    }

    // 현재 룬 타입의 genus 에 따라 0/1 단계부터 실제 점 finder 를 탐색한다. 첫 타입 일치에서 종료하며 별도 dead/free/abstract 필터를 넣지 않는다. (2026-11-13 WB04-E365)
    runeAt(temple, point) {
        const stage = this.stage(temple);
        const type = this.#state.currentRuneType;
        if (type >= this.#types.length) throw new RangeError('신전 의식 현재Rune 타입 범위');
        const hotspot = this.#hotspot(type, stage);
        point = this.#addPixels(point, -hotspot[0], -hotspot[1]);
        const x = cell(point.x);
        const y = cell(point.y);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 첫 일치에서 반환한다.
        for (let sid = finder.begin({ left: x, top: y, right: x, bottom: y }, (this.#types[type].flags2 & 6) === 0 ? 1 : 0); sid; sid = finder.next()) {
            if (this.#pool.slot(sid)[TYPE_AT] === type) return sid;
        }
        return 0;
    }

    // float 발자국을 사각형의 절삭 좌상단/거의 올림 우하단으로 바꾼다. (2026-11-13 WB04-E365)
    #searchArea(area) {
        return { left: cell(area.left), top: cell(area.top), right: cell(area.right + ALMOST_CEIL), bottom: cell(area.bottom + ALMOST_CEIL) };
    }

    // 기존 실제 발자국만 ±1 확장하고 후보의 signed 현재 HP 와 현재 최대 HP 를 비교한다. (2026-11-13 WB04-E365)
    findStrongOwnedPriest(temple) {
        return RawTempleRitual.findStrongOwnedPriestIn(this.#pool, this.#hash, this.#types, this.#footprint, this.#hitPoints, this.#state.currentPriestType, temple);
    }

    // 의식 실행기/효과를 새로 만들지 않고 조회에 실제로 필요한 같은 현재 자료만 쓴다(삭제의 장기 소유자도 빌린다). (2026-11-13 WB04-E365)
    static findStrongOwnedPriestIn(pool, hash, types, footprint, hitPoints, priestType, temple) {
        const area = footprint.footprint(temple);
        const left = fround(area.left - 1);
        const top = fround(area.top - 1);
        const right = fround(area.right + 1);
        const bottom = fround(area.bottom + 1);
        const finder = new RawSquidFinder(pool, hash, types);
        // 일반 finder 순서에서 현재 type/owner/HP 를 읽는다. 사제 자체 extra/state 는 조건이 아니다.
        for (let sid = finder.begin({ left: cell(left), top: cell(top), right: cell(right + ALMOST_CEIL), bottom: cell(bottom + ALMOST_CEIL) }); sid; sid = finder.next()) {
            const raw = pool.slot(sid);
            if (raw[TYPE_AT] !== priestType || raw[OWNER_AT] !== pool.slot(temple)[OWNER_AT]) continue;
            const hp = read(raw, HP_AT) | 0;
            if (hp >= Math.trunc(hitPoints.maxHitPoints(sid) / 2)) return sid;
        }
        return 0;
    }

    // 현재 실제 발자국과 일반 finder 를 재사용해 추상 룬 존재만 읽는다(일반 finder 의 buried 제외를 유지). (2026-11-13 WB04-E365)
    hasAbstractRune(temple) {
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 첫 현재 룬 타입/extra 비트 0 일치만 참이다.
        for (let sid = finder.begin(this.#searchArea(this.#footprint.footprint(temple))); sid; sid = finder.next()) {
            const raw = this.#pool.slot(sid);
            if (raw[TYPE_AT] === this.#state.currentRuneType && (raw[EXTRA_AT] & 1)) return true;
        }
        return false;
    }

    // HP/소유자 효과 이후 각각 현재 단계를 다시 읽는 실제 룬 생성 순서이다. (2026-11-13 WB04-E365)
    spawnRune(temple) {
        const hooks = this.#hooks;
        const rune = need(hooks.create)(this.#state.currentRuneType, 0);
        if (!rune || rune >= this.#pool.capacity) throw new RangeError('신전 의식Rune 생성SID 오류');
        need(hooks.setHitPoints)(rune, 0);
        const stage = this.stage(temple);
        const raw = this.#pool.physical(rune);
        // 원본 DWORD 프레임만 쓰고 옆 필드는 보존한다.
        for (let index = 0; index < 4; index += 1) raw[FRAME_AT + index] = (stage >>> (8 * index)) & 0xff;
        if (!(raw[STATE_AT] & 4)) need(hooks.assertion)('isVoid()', 'c:\\ns\\o\\squid.h', 0x282);
        raw[EXTRA_AT] |= 1;
        need(hooks.setOwner)(rune, this.#owner(temple));
        const point = this.stageAnchor(temple, this.stage(temple));
        need(hooks.pop)(rune, point.x, point.y, 0);
    }

    // 00447e40: 초기 단계는 mode 0 음향/지연에 유지하고 후속 mode 는 각 효과 뒤 현재 값을 재조회한다. (2026-11-13 WB04-E365)
    ritual(temple, priest, mode) {
        const hooks = this.#hooks;
        const stage = this.stage(temple);
        if (stage > 4) return;
        let expected = this.stagePoint(temple, stage);
        let actual = this.#position(priest);
        if (actual.x !== expected.x || actual.y !== expected.y) {
            if (stage >= 4) return;
            expected = this.stagePoint(temple, stage + 1);
            actual = this.#position(priest);
            if (actual.x !== expected.x || actual.y !== expected.y) return;
        }
        if (this.runeAt(temple, this.#position(priest))) return;
        if (!mode) {
            this.#actions.begin(priest, stage);
            actual = this.#position(priest);
            need(hooks.playAt)(actual.x, actual.y, STAGE_SOUNDS[stage], 0);
            if (need(hooks.reserveShared)()) need(hooks.addShared)(temple, RITUAL_EVENT, STAGE_DELAYS[stage]);
            return;
        }
        const found = this.findStrongOwnedPriest(temple);
        if (found && found < this.#pool.capacity) this.#actions.continue(found, this.stage(temple));
        if (this.#state.currentBoss && !this.hasAbstractRune(temple)) this.spawnRune(temple);
    }
}

// 실제 Factory/공통 HP setter/Owner/Host 를 연결한다. Pop/음향/확보 경계는 실제 호출자가 공급한다(새 묶음, 풀이 다르면 거부). (2026-11-13 WB04-E365)
export function makeTempleRitualHooks(pool, factory, owner, host, hooks) {
    if (pool !== factory.pool || pool !== owner.pool || pool !== host.pool) throw new TypeError('신전 의식 Factory/owner/Host SID 풀 불일치');
    return {
        ...hooks,
        create: (type, flags) => factory.create(type, flags),
        setHitPoints: (sid, value) => { setRawSquidHitPoints(pool, sid, value); },
        setOwner: (sid, player) => { owner.set(sid, player); },
        addShared: (sid, event, payload) => { host.addSharedRegular(sid, event, payload); },
    };
}
