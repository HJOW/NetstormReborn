// 로컬 간헐천 생성(Early1·전투 시작의 Player별 간헐천 배치)을 옮긴 모듈이다(cpppj `o/RawLocalGeyser.cpp/.h`, 10.78 전용).
//  - generate(): 시드 0xfed(설정 predictableGeysers 가 0 이면 현재 TimeSeed)의 같은 로컬 난수를 Player 1~8 순서로 공유한다. 각 차례에서 현재 Player 기록의 flags(0x22)·지역(127 아님)을 다시 읽고
//    현재 Player 사각형(앞선 생성 효과가 바꾼 뒤의 기록 포함)을 구해 수량 전역(1~4 번 / 5~8 번)을 읽은 뒤 populate 를 부른다.
//  - populate(): 권한(boss)/요새 관문 → 수량 음수 진단 → 최근 목록 비움 → (요청 시) Player 제외 영역 캡처 → 수량이 0 이 될 때까지 후보 100회 시도 → 같은 자리의 빈 간헐천 삭제(0x400000) →
//    받침 확정(지형이 간헐천 자리가 아니면 noIsland) → 현재 TYPE 의 factory 생성 → Pop → 최근 5목록 추가 → 자원 pool 감소(float 저장 뒤 비용 절삭)이다. 100회 실패는 요청 전체를 끝낸다.
//  - tryCandidate(): 높이 난수 뒤 폭 난수로 고른 점을 반올림하고 제외 사각형/3×3 발자국 포함 경계/spot(하위 6비트)/허용 지형(땅 2·허공 0)/한 종류 지형/프레임 문자 무늬 "FBICANGDH" 를 검사한다.
//  - playerExclusions(): 현재 flags bit0 이고 현재 지역이 존재하는 Player 의 사각형을 양끝 12 확장한다.
// 모든 전역(권한·요새·TYPE 네 종·수량·자원 pool·Player 9기록)은 소비 위치마다 현재 값을 다시 읽고(공유 getter 또는 독립 값), 미연결 하위 효과는 도달한 위치에서 `Error` 로 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. 근거: 원본 10.78 관측 `local-geyser-x86.tsv`(221행)·`geyser-placement-x86.tsv`(183행)와 cpppj `LocalGeyserTests.cpp`.
// (2026-11-10 WB04-E256: 로컬 간헐천 이식으로 추가. cpppj 기준선 뒤 ed29febb·3102fadd·5f4b9d30 의 현재 Player base 선택 레코드와 공유 전역 getter 를 ② 재반영)
import { timeSeed } from './raw-add-random.js';
import { GameRandom } from './random.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 실제 PATCH Player 필드 위치이다: 한 기록 폭 0xb4, flags BYTE(+0x50), 지역 DWORD(+0x78), Player 기록 수 9. (2026-11-10 WB04-E256)
const PLAYER_STRIDE = 0xb4;
const PLAYER_FLAGS = 0x50;
const PLAYER_REGION = 0x78;
const PLAYER_COUNT = 9;

// 원본 로컬 시드/생성별 최대 시도/최근 목록 상한/타입 표 크기/spot 표 크기이다. (2026-11-10 WB04-E256)
const PREDICTABLE_SEED = 0xfed;
const ATTEMPT_LIMIT = 100;
const RECENT_LIMIT = 5;
const TYPE_COUNT = 188;
const SPOT_COUNT = 65536;

// ROM 의 실제 float 상수이다: 발자국 반지름 2.0·반올림 0.5·spot 보정 0.9999(float 0.99989998340606689453125)·Player 제외 영역 추가 확장 12.0. (2026-11-10 WB04-E256)
const RADIUS = 2.0;
const HALF = 0.5;
const SPOT_BIAS = 0.99989998340606689453125;
const EXCLUSION_MARGIN = 12.0;

// 빈 간헐천 삭제 flags 와 지면 프레임 문자 무늬, 기본 요청의 허용 지형이다. (2026-11-10 WB04-E256)
const DESTROY_FLAGS = 0x400000;
const SURFACE_PATTERN = 'FBICANGDH';

// 비정렬 리틀엔디언 DWORD 를 읽는다(부호 없음). (2026-11-10 WB04-E256)
function word(bytes, offset) {
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-10 WB04-E256)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// x87 `_ftol` 의 signed64 절삭 뒤 하위 DWORD(int32)이다. 비유한/범위 밖은 불확정 값의 하위 0 이다(원본 4e49c0). (2026-11-10 WB04-E256)
function truncate(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 원본 004bca20: limit <= 0 이면 로컬 시드를 소비하지 않고 0 이다. (2026-11-10 WB04-E256)
function next(random, limit) {
    return limit > 0 ? random.next(limit) : 0;
}

// 선택된 하위 효과가 없으면 그 위치에서 거부한다. (2026-11-10 WB04-E256)
function need(callback, message) {
    if (typeof callback !== 'function') throw new Error(message);
    return callback;
}

// 로컬 간헐천이 읽는 전역이다(cpppj `LocalGeyserState`). 독립 값과 실제 Client 의 현재 전역을 빌리는 getter(shared*)를 모두 가진다. 읽기 접근자(readXxx)는 소비 위치마다 현재 값을 다시 읽는다.
// 사용: const state = new LocalGeyserState(); state.sharedBoss = () => client.boss. (2026-11-10 WB04-E256)
export class LocalGeyserState {
    // 00540bc4/00594fb8 의 독립 생성 권한/요새 모드, 00540bd4/d8 의 독립 1~4/5~8 생성 요청 수, 00595088 의 독립 자원 pool 이다.
    boss = true;
    fort = false;
    firstPlayerQuantity = 3;
    lastPlayerQuantity = 4;
    resourcePool = 0;
    // 현재 삭제(빈 간헐천)/생성(간헐천)/건설 확정(받침)/프레임 문자 수집(섬) 타입 전역의 독립 값이다.
    emptyGeyserType = 84;
    geyserType = 122;
    noIslandType = 157;
    isleType = 97;
    // 실제 Client 의 현재 값을 돌려주는 함수이다. 없으면 null(독립 값 사용).
    sharedGeyserType = null;
    sharedBoss = null;
    sharedFort = null;
    sharedEmptyGeyserType = null;
    sharedNoIslandType = null;
    sharedIsleType = null;
    sharedFirstQuantity = null;
    sharedLastQuantity = null;

    // 생성/Pop 효과 뒤에도 현재 TYPE 을 읽는다. (2026-11-10 WB04-E256)
    readGeyserType() { return this.sharedGeyserType ? this.sharedGeyserType() >>> 0 : this.geyserType; }

    // 각 populate 진입의 권한 관문에서 현재 값을 읽는다. (2026-11-10 WB04-E256)
    readBoss() { return this.sharedBoss ? Boolean(this.sharedBoss()) : this.boss; }

    // boss 관문 뒤에만 읽는 요새 모드이다. (2026-11-10 WB04-E256)
    readFort() { return this.sharedFort ? Boolean(this.sharedFort()) : this.fort; }

    // 후보 constructor 와 실제 교체 조회가 각 시점의 번호를 선택한다. (2026-11-10 WB04-E256)
    readEmptyGeyserType() { return this.sharedEmptyGeyserType ? this.sharedEmptyGeyserType() >>> 0 : this.emptyGeyserType; }

    // 앞선 빈 간헐천 삭제 효과 뒤의 현재 받침 확정 번호이다. (2026-11-10 WB04-E256)
    readNoIslandType() { return this.sharedNoIslandType ? this.sharedNoIslandType() >>> 0 : this.noIslandType; }

    // 각 실제 지면 슬롯 비교에서 읽는 프레임 문자 수집 TYPE 이다. (2026-11-10 WB04-E256)
    readIsleType() { return this.sharedIsleType ? this.sharedIsleType() >>> 0 : this.isleType; }

    // Player 경계 효과 뒤 현재 수량(1~4 번)을 읽는다. (2026-11-10 WB04-E256)
    readFirstQuantity() { return this.sharedFirstQuantity ? this.sharedFirstQuantity() | 0 : this.firstPlayerQuantity; }

    // 후반 Player(5~8 번) 수량도 같은 현재 전역에서 읽는다. (2026-11-10 WB04-E256)
    readLastQuantity() { return this.sharedLastQuantity ? this.sharedLastQuantity() | 0 : this.lastPlayerQuantity; }
}

// 원본 생성/공간/설정의 필수 하위를 확인하는 목록이다(playerBoundsRecord 는 현재 Player base 를 쓰는 연결에서만 필요). (2026-11-10 WB04-E256)
const REQUIRED_HOOKS = Object.freeze(['configInt', 'wallSeconds', 'playerBounds', 'negativeQuantity', 'destroy', 'confirm', 'create', 'pop']);

// 로컬/일반 간헐천 생성이 같은 난수·빈 간헐천 교체·프레임 문자 필터를 공유하는 처리기이다.
// pool 은 10.78 SidPool, hash 는 SquidHash, spots 는 65536 BYTE spot 표(후보/확정 사이 효과가 바꾼 현재 값), types 는 188칸 TYPE 표({ flags2, footX, footY, cost }),
// frames 는 TYPE 별 FrameCodeTable 표, players 는 9×0xb4 Player 기록(또는 deferred 에서는 null), recent 는 최근 5생성 SquidPostPopList, state 는 LocalGeyserState,
// hooks = { configInt(key, fallback), wallSeconds(), playerBounds(owner), playerBoundsRecord?(owner, record), negativeQuantity(), destroy(sid, flags), confirm(request),
//           create(type, flags) → sid, pop(sid, x, y, flags), regionExists?(region) → bool } 이다. 불일치/누락은 `TypeError` 로 거부한다.
// 사용: const body = new RawLocalGeyser(pool, hash, spots, types, frames, players, recent, state, hooks); body.generate(). (2026-11-10 WB04-E256)
export class RawLocalGeyser {
    // 같은 월드의 풀·해시·spot·TYPE·프레임 표와 Player 기록(독립 입력) 또는 현재 Player 조회 함수(장기 owner)이다.
    #pool;
    #hash;
    #spots;
    #types;
    #frames;
    #currentPlayers;
    #players;
    // 최근 5생성 목록·전역 상태·외부 효과 묶음, 후설치되는 실제 자원 pool 상자({ value })이다.
    #recent;
    #state;
    #hooks;
    #resourcePool = null;

    // 독립 입력은 Player 9기록을 생성 때 확인하고, currentPlayers 가 있으면 생성 때 Player 를 조회하지 않고 선택 위치에서 확인한다. (2026-11-10 WB04-E256)
    constructor(pool, hash, spots, types, frames, players, recent, state, hooks, currentPlayers = null) {
        const given = { ...hooks };
        if (pool?.edition !== 'patch1078' || types.length !== TYPE_COUNT || frames.length !== types.length || spots.length !== SPOT_COUNT ||
            (!currentPlayers && players?.length !== PLAYER_COUNT * PLAYER_STRIDE) || (currentPlayers && typeof given.playerBoundsRecord !== 'function') ||
            recent.entries.length < RECENT_LIMIT || REQUIRED_HOOKS.some((name) => typeof given[name] !== 'function')) {
            throw new TypeError('로컬 간헐천의 현재 원본 자료/실제 생성 경계 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#spots = spots;
        this.#types = types;
        this.#frames = frames;
        this.#currentPlayers = currentPlayers;
        this.#players = players ?? null;
        this.#recent = recent;
        this.#state = state;
        this.#hooks = given;
    }

    // Native 장기 구성: Player 를 할당/조회하지 않고 실제 반복이 선택한 기록만 그 자리에서 확인한다. (2026-11-10 WB04-E256)
    static deferred(currentPlayers, pool, hash, spots, types, frames, recent, state, hooks) {
        return new RawLocalGeyser(pool, hash, spots, types, frames, null, recent, state, hooks, currentPlayers);
    }

    // 현재 Player 조회 함수만 연결한다(조회/초기화는 실제 반복의 선택 위치까지 미룬다). 중복 연결·누락은 거부한다. (2026-11-10 WB04-E256)
    bindCurrentPlayers(players) {
        if (typeof players !== 'function' || this.#currentPlayers || typeof this.#hooks.playerBoundsRecord !== 'function') throw new TypeError('현재 간헐천 Player/선택 this 부모의 중복/누락');
        this.#currentPlayers = players;
    }

    // 상위가 전체 간헐천 목록이나 다른 풀을 잘못 연결하지 않도록 같은 자료 정체성을 공개한다. (2026-11-10 WB04-E256)
    get pool() { return this.#pool; }

    // 최근 5생성 목록이다(전체 장부가 아님). (2026-11-10 WB04-E256)
    get recent() { return this.#recent; }

    // 실제 자원 전역({ value } 상자)의 주소만 빌린다. 값 복사/주기적 동기화/예산 사본을 만들지 않으며 다른 상자로 다시 연결하면 거부한다. (2026-11-10 WB04-E256)
    bindResourcePool(box) {
        if (this.#resourcePool && this.#resourcePool !== box) throw new Error('로컬 간헐천 자원 전역의 중복 다른 주소 연결');
        this.#resourcePool = box;
    }

    // 현재 자원 pool 이다(연결 전 독립 입력은 state.resourcePool). 삭제/Pop 효과 뒤에도 다시 읽는다. (2026-11-10 WB04-E256)
    readResourcePool() { return this.#resourcePool ? this.#resourcePool.value | 0 : this.#state.resourcePool | 0; }

    // 현재 자원 pool 에 int32 로 쓴다. (2026-11-10 WB04-E256)
    #writeResourcePool(value) {
        if (this.#resourcePool) this.#resourcePool.value = value | 0;
        else this.#state.resourcePool = value | 0;
    }

    // 각 원문 반복의 현재 9기록 물리만 조회한다(크기가 다르면 거부). (2026-11-10 WB04-E256)
    #playerRecords() {
        const read = this.#currentPlayers;
        const result = read ? read() : this.#players;
        if (!result || result.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new RangeError('현재 간헐천 Player9 물리 부족');
        return result;
    }

    // Native 는 같은 선택 레코드를 경계에 전달하고 독립 입력은 기존 owner 인자를 쓴다. (2026-11-10 WB04-E256)
    #selectedBounds(owner, record) {
        if (this.#currentPlayers) return need(this.#hooks.playerBoundsRecord, '간헐천의 실제 선택 Player this 부모 미연결')(owner, record);
        return this.#hooks.playerBounds(owner);
    }

    // 원본이 직접 산술한 linear spot 인덱스를 읽는다. 물리 배열 밖은 명시 오류다. (2026-11-10 WB04-E256)
    #spot(x, y) {
        const index = y * 256 + x;
        if (index < 0 || index >= this.#spots.length) throw new RangeError('로컬 간헐천 spot 물리 범위');
        return this.#spots[index];
    }

    // rectangle 은 값형 snapshot 으로 다뤄 처리 중 외부 사각형 변경과 분리한다. point({ x, y })에는 실패한 시도도 마지막 반올림 좌표를 출력한다.
    // 인자 3개 형태는 허용 지형(땅 0·허공 1)과 제외 영역 없음이고, 6개 형태는 명시 finder 입력(최대 8개 제외 사각형)이다. 생성/Player 조회는 하지 않는다. (2026-11-10 WB04-E256)
    tryCandidate(random, rectangle, point, allowLand = 0, allowVoid = 1, exclusions = []) {
        if (exclusions.length > 8) throw new RangeError('간헐천 finder 제외 사각형의 실제8개 물리 범위');
        return this.#candidate(random, { ...rectangle }, point, this.#state.readEmptyGeyserType(), allowLand, allowVoid, exclusions);
    }

    // 004857d0: 현재 flags bit0 → 현재 지역 존재 → 실제 Player 경계 → 양끝 12 확장 순서이다(AI/everLoggedIn 비트는 보지 않는다). (2026-11-10 WB04-E256)
    playerExclusions() {
        const result = [];
        // 각 Player 차례에서 앞선 실제 하위가 바꾼 기록을 다시 읽는다.
        for (let owner = 1; owner <= 8; owner += 1) {
            const selected = this.#playerRecords().subarray(owner * PLAYER_STRIDE, (owner + 1) * PLAYER_STRIDE);
            if (!(selected[PLAYER_FLAGS] & 1)) continue;
            const regionExists = need(this.#hooks.regionExists, '간헐천 제외 영역의 실제 IslandList 조회 미연결');
            if (!regionExists(word(selected, PLAYER_REGION) | 0)) continue;
            const bounds = this.#selectedBounds(owner, selected);
            result.push({
                left: Math.fround(bounds.left - EXCLUSION_MARGIN), top: Math.fround(bounds.top - EXCLUSION_MARGIN),
                right: Math.fround(bounds.right + EXCLUSION_MARGIN), bottom: Math.fround(bounds.bottom + EXCLUSION_MARGIN),
            });
        }
        return result;
    }

    // 부모 생성자가 덮어쓰는 +ac=emptyType/+b0=1 을 포함해 로컬 후보 전체를 실행한다. 반환은 후보 사용 가능 여부이다. (2026-11-10 WB04-E256)
    #candidate(random, rectangle, point, reuseType, allowLand, allowVoid, exclusions) {
        const width = truncate(rectangle.right - rectangle.left);
        const height = truncate(rectangle.bottom - rectangle.top);
        if (width !== 0 && height !== 0) {
            // 원본은 높이 난수 다음 폭 난수를 소비한다.
            const dy = next(random, height);
            const dx = next(random, width);
            point.y = Math.fround(dy + rectangle.top);
            point.x = Math.fround(dx + rectangle.left);
        } else {
            point.x = rectangle.left;
            point.y = rectangle.top;
        }
        point.x = Math.fround(Math.floor(point.x + HALF));
        point.y = Math.fround(Math.floor(point.y + HALF));
        // 00485890 은 3×3 발자국보다 먼저 반올림한 기준점의 포함 경계를 검사한다.
        for (const excluded of exclusions) {
            if (excluded.left <= point.x && point.x <= excluded.right && excluded.top <= point.y && point.y <= excluded.bottom) return false;
        }
        // 원본이 한 번 저장한 3×3 첫 칸이다.
        const left = Math.fround(point.x - RADIUS);
        const top = Math.fround(point.y - RADIUS);
        if (!(rectangle.left <= left && left <= rectangle.right && rectangle.top <= top && top <= rectangle.bottom)) return false;
        const firstX = truncate(left);
        const firstY = truncate(top);
        let pattern = '';
        let previousSurface = 0xffffffff;
        // 원본은 x 바깥/y 안쪽이며 끝점도 포함해 최대 3×3 의 현재 spot 을 읽는다.
        for (let x = firstX; Math.fround(x) <= point.x; x += 1) {
            // 첫 거부 칸에서 즉시 반환하여 다음 칸이나 난수를 추가 소비하지 않는다.
            for (let y = firstY; Math.fround(y) <= point.y; y += 1) {
                if (x < -0x80000000 || x > 0x7fffffff || y < -0x80000000 || y > 0x7fffffff) throw new RangeError('로컬 간헐천 후보 정수 범위');
                // 원본은 typedFind 호출 전에 이 칸의 BYTE 를 보존한다.
                const spot = this.#spot(x, y);
                if (reuseType) {
                    const existing = this.#findType(x, y, reuseType);
                    if (existing) {
                        const raw = this.#pool.slot(existing);
                        point.x = floatOf(word(raw, 14));
                        point.y = floatOf(word(raw, 18));
                        return true;
                    }
                }
                if (spot & 0x3d) return false;
                const kind = spot & 0x3f;
                if ((!allowLand && kind === 2) || (!allowVoid && kind === 0)) return false;
                if (previousSurface !== 0xffffffff && kind !== previousSurface) return false;
                previousSurface = kind;
                // 원본 WORD getter 는 범위 밖에서 0 을 읽고, SID 0 도 원본처럼 TYPE 0 을 조회한다.
                const surface = x >= 0 && y >= 0 && x < 256 && y < 256 ? this.#hash.view(0)[y * 256 + x] : 0;
                const raw = this.#pool.slot(surface);
                if (raw[10] === this.#state.readIsleType()) {
                    const frame = word(raw, 36) | 0;
                    if (raw[10] >= this.#frames.length || frame < 0 || frame >= this.#frames[raw[10]].codes.length) throw new RangeError('로컬 간헐천 isle프레임 물리 범위');
                    let side = this.#frames[raw[10]].codes[frame].side;
                    if (side >= 0x61 && side <= 0x7a) side = side - 0x61 + 0x41;
                    pattern += String.fromCharCode(side);
                }
            }
        }
        // 원본 stricmp 의 첫 NUL 종료를 보존한다.
        const zero = pattern.indexOf('\0');
        if (zero !== -1) pattern = pattern.slice(0, zero);
        return pattern !== SURFACE_PATTERN;
    }

    // 현재 empty 간헐천을 기존 일반 finder 의 exact type/현재 공간 순서로 조회한다. (2026-11-10 WB04-E256)
    #findEmpty(x, y) {
        return this.#findType(x, y, this.#state.readEmptyGeyserType());
    }

    // 부모가 보존한 reuse 타입과 성공 뒤의 현재 empty 전역 모두 같은 실제 typed finder 를 쓴다. 표면/다리가 아닌 부류는 hash0 을 건너뛴다. 없으면 0 이다. (2026-11-10 WB04-E256)
    #findType(x, y, number) {
        if (number >= this.#types.length) throw new RangeError('로컬 간헐천 현재 empty타입 범위');
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        const ix = truncate(x);
        const iy = truncate(y);
        const flags = (this.#types[number].flags2 & 6) ? 0 : 1;
        // 같은 raw 타입을 매 후보마다 읽고 첫 exact 일치만 반환한다.
        for (let sid = finder.begin({ left: ix, top: iy, right: ix, bottom: iy }, flags); sid !== 0; sid = finder.next()) {
            if (this.#pool.slot(sid)[10] === number) return sid;
        }
        return 0;
    }

    // 로컬 요청: 권한/요새 관문 → 최근 목록 0 → 각 생성 100회 시도 → 실제 생성/자원 순서이다. 100회 실패는 이 요청 전체를 끝내고 상위 Player 순회는 계속된다.
    // options = { excludePlayers, allowLand, allowVoid }(00486130 의 마지막 세 DWORD, 비영 판정; 기본 0/0/1)이며 수량 0 에서도 최근 목록 비움과 제외 영역 조회는 일어난다. (2026-11-10 WB04-E256)
    populate(quantity, random, rectangle, options = {}) {
        const { excludePlayers = 0, allowLand = 0, allowVoid = 1 } = options;
        const state = this.#state;
        const hooks = this.#hooks;
        if (!state.readBoss() || state.readFort()) return;
        let remaining = quantity | 0;
        if (remaining < 0) {
            hooks.negativeQuantity();
            remaining = 0;
        }
        this.#recent.setCount(0);
        const exclusions = excludePlayers ? this.playerExclusions() : [];
        // 원본 ctor 뒤 finder+ac 는 이 시점 타입을 한 번 보존한다.
        const reuseType = state.readEmptyGeyserType();
        // 요청 수량은 값형 인자이며 실제 생성 성공마다 한 번 감소한다.
        while (remaining !== 0) {
            const point = { x: 0, y: 0 };
            let found = false;
            // 로컬 난수와 현재 spot 을 같은 후보 몸체에 전달한다.
            for (let attempt = 0; attempt < ATTEMPT_LIMIT; attempt += 1) {
                if (this.#candidate(random, { ...rectangle }, point, reuseType, allowLand, allowVoid, exclusions)) {
                    found = true;
                    break;
                }
            }
            if (!found) return;
            const empty = this.#findEmpty(point.x, point.y);
            if (empty) hooks.destroy(empty, DESTROY_FLAGS);
            remaining = (remaining - 1) | 0;
            // 원본 confirm 의 별도 단정도 저장 좌표이다.
            const x = Math.fround(point.x - RADIUS);
            const y = Math.fround(point.y - RADIUS);
            if (!(this.#spot(truncate(point.x + SPOT_BIAS), truncate(point.y + SPOT_BIAS)) & 2)) {
                hooks.confirm({ type: state.readNoIslandType(), x, y, player: 0, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 });
            }
            // 실제 factory 실패를 존재하는 간헐천으로 계산하지 않는다.
            const created = hooks.create(state.readGeyserType(), 0);
            if (!created) throw new Error('로컬 간헐천 실제 factory 생성 실패');
            hooks.pop(created, point.x, point.y, 0);
            // 최근 목록은 현재 개수/저장소를 매번 다시 읽는다(연결된 장기 목록이면 Player 쪽 값).
            const used = this.#recent.readCount();
            if ((used | 0) < RECENT_LIMIT) {
                if (used >= this.#recent.capacity()) throw new RangeError('로컬 간헐천 최근목록 물리 범위');
                this.#recent.storage()[used] = created;
                this.#recent.setCount(used + 1);
            }
            // 원본은 삭제/Pop 뒤 같은 00595088 을 다시 읽어 TYPE 조회 전에 float 로 한 번 저장한다.
            const pool = Math.fround(this.readResourcePool());
            const geyserType = state.readGeyserType();
            if (geyserType >= this.#types.length) throw new RangeError('로컬 간헐천 현재 cost타입 범위');
            this.#writeResourcePool(Math.max(0, truncate(pool - this.#types[geyserType].cost)));
        }
    }

    // 00486360: 시드 0xfed 또는 현재 TimeSeed, 대상 Player 1~8 순서에서 같은 로컬 난수를 공유한다. 부모는 권한 관문 없이 진입하고 각 populate 가 현재 값을 검사한다. (2026-11-10 WB04-E256)
    generate() {
        const hooks = this.#hooks;
        const random = new GameRandom(PREDICTABLE_SEED);
        if (hooks.configInt('predictableGeysers', 1) === 0) random.setState(timeSeed(hooks.wallSeconds()));
        // 플레이어 번호 순으로 현재 flags/지역을 읽으며 앞선 생성 효과가 바꾼 뒤 기록도 반영한다.
        for (let owner = 1; owner <= 8; owner += 1) {
            // 다음 반복은 현재 base 를 다시 고르고 이 차례의 선택 레코드는 경계 호출까지 유지한다.
            const selected = this.#playerRecords().subarray(owner * PLAYER_STRIDE, (owner + 1) * PLAYER_STRIDE);
            if (!(selected[PLAYER_FLAGS] & 0x22) || word(selected, PLAYER_REGION) === 127) continue;
            // 원본은 수량 전역을 읽기 전에 현재 Player 사각형을 구한다.
            const rectangle = this.#selectedBounds(owner, selected);
            this.populate(owner <= 4 ? this.#state.readFirstQuantity() : this.#state.readLastQuantity(), random, rectangle);
        }
    }
}
