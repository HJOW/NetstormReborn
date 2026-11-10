// 초기 요새 자산의 실제 postPop 재정의(가상 표 +0x20)를 raw SID 와 공통 장부에 연결하는 분배 본체이다(cpppj `o/RawAssetPostPop.cpp/.h`, 10.78 전용).
// 10.78 가상 표 24개를 20가지 몸체 부류로 나눠 실제 순서대로 실행한다. 각 몸체는 공통 장부(공통 postPop 또는 직접 Damageable)를 정확히 한 번 부른다.
//   - 제네레이터(0x501ea0): 최초 flag 에서 현재 TYPE 조회(theme, 반환은 버림) → Damageable.
//   - Gun 네 부류(각도 0x50b6e8·최초 0x50b7c0·썬 0x50b9b8/0x50bad0·아처 0x50b888): 각도 부류는 flags&7 에서 현재 각도의 Gunanim 프레임을 그래프 번호로 쓴다. 분기(각도 flags&7, 그 밖 flags&1)가 서고
//     (썬은 abstract 가 아닐 때) 서버 권한·abstract 아님이면 [아처 애니메이션 제거 →] GunProcess 생성 → 썬/아처의 로딩·편집 진단 → checking 이 켜져 있으면 Form 체인 판정과 진단. 썬은 로딩 중 flags1 0x1000 이면
//     WORD 를 프레임 DWORD 에 백업하고, 언제나 flags1 0x1000 이면 프레임 DWORD 를 WORD 로 되돌린다(WORD setter: 현재 TYPE 이 다리 TYPE 이면 표면 통지). 마지막에 Damageable.
//   - Gun 장식(0x50b570): 최초 flag 에서 Regular(0, 0.05) → 공통 postPop.
//   - 거주 구역(0x50f540): 최초 flag 에서 현재 지면의 소유자(0 아님)를 가상 소유자 지정으로 넘기고 → 거주 목록 추가 → Regular 0·1·3 → WORD 하위 nibble 지움 → 표면 통지 → 잔여 프레임(+42) 기록. 마지막에 공통 postPop.
//   - 바리케이드(0x50e940·0x50ea08·0x50ead0): 서버·최초·비로딩이면 차단 프로세스 시작 → Damageable.
//   - 간헐천(0x50da88): 최초·일반(extra&9 없음)이면 간헐천 목록 추가 → 등록(sid, 1) → 현재 TYPE 비용의 절삭값을 WORD 로 → Regular(0, 0.1) → 현재 좌표의 후보마다 섬/종유석 TYPE 이면 Unpop → 프레임 8 → Repop(0x40). 마지막에 Damageable.
//   - 신전 세 부류(레인 0x5196f0·윈드 0x519860/0x5197a8·썬더 0x519918): 공통 몸체(Damageable 먼저 → flags&2 의 새 신전 Player 필드 → 최초 flag 의 개수·음향·WORD 0·초기 신전 필드 → 일반 객체의 두 목록(중복 없이)·
//     선택 Form 의 배치 취소·프레임(TYPE+0x130)·섬 등록·소유자·(boss 면) 같은 영역의 다른 신전 삭제 → 비로딩이면 slot 내용물 생성과 생산 갱신) 뒤 최초 flag 에서 원소별 프레임(TYPE+0x134, 윈드 제외)과 Regular(1, 0.08 또는 썬더 0.1).
//   - 전체 몸체를 훅으로 받는 부류: 훨리베이스(0x509930)·맨 오 워 풀(0x5099e8)·데빌메이커(0x509aa0)·썬 바리케이드(0x509470)·너겟(0x50dc30)·성장형 아이스(0x50e888). 아크 스파이어(0x50e668)·애시드 펜스(0x50e738)는 E49·E50 의 후처리를 직접 부른다.
// 모든 외부 효과 뒤에 raw(TYPE·extra·소유자·WORD·프레임)와 전역(서버·boss·로딩·편집·checking·선택·다리 TYPE·목록)을 다시 읽는다. 장치(훅)는 그 몸체가 실제로 도달한 위치에서만 요구하고, 없으면 그 위치에서 거부하며 앞선 효과는 되돌리지 않는다.
// 쓰기는 free/contained 여부와 무관하게 같은 물리 슬롯에 한다(원문과 같음).
//  - RawAssetPostPop(pool, types, base, damageable, state, hooks): types 는 188칸 TYPE 표(참조, 현재값을 읽음), base 는 같은 풀의 SquidPostPop, damageable 은 같은 풀의 RawCarrierPostPop, state 는 AssetPostPopState(E61),
//    hooks 는 아래 이름의 함수 묶음(모두 선택; 도달한 것만 필요):
//      reserveRegular()→bool, addRegular(sid, event, delay), createGunProcess(sid), removeAnimation(sid), checkForms(sid)→bool, assertion(식, 파일, 줄), gunFrame(각도)→프레임, setGraph(sid, 번호),
//      surface(x, y)→SID, setOwner(sid, owner), notifySurface(sid), startBlocker(sid), geyserRegistration(sid, 값), atPosition(x, y)→SID 배열 또는 visitAtPosition(x, y, visitor), unpop(sid, flags), repop(sid, flags),
//      typeDword(sid, 오프셋)→DWORD, playImpactSound(토큰), clearPlacement(), islandNumber(x, y)→번호, islandExists(번호)→bool, registerTemple(번호, sid), regionNumber(x, y)→번호, destroy(sid, flags),
//      hasContent(sid, type)→bool, createSlot(sid, type), refreshProduction(), sunAviaryPostPop·rainAviaryPostPop·windAviaryPostPop·sunFencePostPop·nuggetPostPop·growingIcePostPop(sid, flags).
//    handles(sid): 복원한 가상 표인가. validate(sid, flags, pop): 명시 사전 검사(독립 호출·SquidPostPop.validate 용; 현재 분기가 요구하는 장치와 공통 장부를 쓰기 전에 확인).
//    postPop(sid, flags): 실제 가상 진입. 10.78 은 알려진 부류에 사전 검사를 끼우지 않는다. geyserPostPop(sid, flags): 실제 장기 owner 가 고른 간헐천 본문(Native 정수화: 비유한·범위 밖 비용은 0).
//    bindPlayerRecords(records, currentLocalOwner): 실제 Player 9기록(Uint8Array 9×0xb4, 신전 네 DWORD 는 +0x7c..+0x88)을 빌린다. currentLocalOwner 는 현재 battleOwner DWORD getter(생략하면 state.localOwner).
//  - RawAssetPostPop.hasDedicatedVtable(table): 전용 +0x20 몸체가 있는 가상 표인가(인스턴스 없이).
//  - makeAssetPostPopFinder(pool, hash, types, native): 간헐천의 visitAtPosition 에 쓰는 실제 일반 finder(좌표를 그대로 절삭한 한 칸, 각 visitor 뒤 Next).
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기(가상 표 15개·필드 폭/위치·CD 전용 assert)는 옮기지 않았다. (2026-11-10 WB04-E254: 3-C RawAssetPostPop 본체 이식으로 추가)
import { postPopAcidFence } from './raw-acid-fence.js';
import { postPopArcSpire } from './raw-arc-spire.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 공통 raw 필드 위치(가상 표·TYPE·상태·원래 소유자 WORD·좌표·종속 kind·소유자·프레임 DWORD·extra·거주 잔여 프레임)와 TYPE 표 길이, Player 수이다. (2026-11-10 WB04-E254)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const WORD_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const KIND_OFFSET = 18;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
const RESIDUE_OFFSET = 42;
const TYPE_COUNT = 188;
const PLAYER_COUNT = 8;

// 실제 Player 한 기록의 BYTE 크기와 첫 신전 DWORD 위치(+0x7c: 초기, +0x80: 초기 복사, +0x84: 새, +0x88: 새 복사)이다. (2026-11-10 WB04-E254)
const PLAYER_STRIDE = 0xb4;
const PLAYER_TEMPLE = 0x7c;
const TEMPLE_FIELDS = Object.freeze(['initialTemple', 'initialTempleCopy', 'newTemple', 'newTempleCopy']);

// 실제 후처리 몸체의 부류이다. 숫자는 분배용이며 원본 TYPE 번호와 다르다. (2026-11-10 WB04-E254)
const KIND = Object.freeze({
    generator: 1, gunAngle: 2, gunFirst: 3, gunSun: 4, gunArcher: 5, gunDecoration: 6, residence: 7, blocker: 8, geyser: 9, vortexRain: 10, vortexWind: 11, vortexThunder: 12,
    sunAviary: 13, sunFence: 14, rainAviary: 15, nugget: 16, windAviary: 17, arcSpire: 18, acidFence: 19, growingIce: 20,
});

// 10.78 의 실제 +0x20 가상 표 주소와 몸체 부류이다. 캐논과 빈 패치 TYPE 186 도 같은 윈드 신전 몸체다. (2026-11-10 WB04-E254)
const PATCH_TABLES = new Map([
    [0x501ea0, KIND.generator], [0x50b6e8, KIND.gunAngle], [0x50b7c0, KIND.gunFirst], [0x50b9b8, KIND.gunSun], [0x50bad0, KIND.gunSun],
    [0x50b888, KIND.gunArcher], [0x50b570, KIND.gunDecoration], [0x50f540, KIND.residence],
    [0x50e940, KIND.blocker], [0x50ea08, KIND.blocker], [0x50ead0, KIND.blocker], [0x50da88, KIND.geyser], [0x50e888, KIND.growingIce],
    [0x5196f0, KIND.vortexRain], [0x519860, KIND.vortexWind], [0x5197a8, KIND.vortexWind], [0x519918, KIND.vortexThunder], [0x509930, KIND.sunAviary], [0x509470, KIND.sunFence],
    [0x5099e8, KIND.rainAviary], [0x50dc30, KIND.nugget], [0x509aa0, KIND.windAviary], [0x50e668, KIND.arcSpire], [0x50e738, KIND.acidFence],
]);

// 원문 진단의 파일 이름과 Gun 부류별 checkForms 진단 줄 번호이다. (2026-11-10 WB04-E254)
const GUN_FILE = '\\Ns\\O\\Gunprocess.cpp';
const VORTEX_FILE = '\\Ns\\O\\Vortex.cpp';
const GUN_CHECK_LINES = Object.freeze({ [KIND.gunAngle]: 0x347, [KIND.gunFirst]: 0x361, [KIND.gunSun]: 0x32b, [KIND.gunArcher]: 0x3c9 });

// Regular 지연의 단정도 상수(장식 0.05, 간헐천·썬더 신전 0.1, 그 밖 신전 0.08)와 signed64 CRT 절삭의 표현 범위이다. (2026-11-10 WB04-E254)
const DELAY_DECORATION = Math.fround(0.05);
const DELAY_TENTH = Math.fround(0.1);
const DELAY_VORTEX = Math.fround(0.08);
const CRT_LIMIT = 9223372036854775808;

// 도달한 장치만 요구한다. 없으면 그 위치에서 거부한다(콜백 중 자기 연결이 바뀌어도 고른 호출 대상을 유지). (2026-11-10 WB04-E254)
function need(device) {
    if (typeof device !== 'function') throw new Error('자산 후처리의 선택된 실제 장치 미연결');
    return device;
}

// 정렬되지 않은 raw 리틀엔디언 값을 읽는다(부호 없음). (2026-11-10 WB04-E254)
function read(raw, offset, width = 4) {
    let value = 0;
    // 필요한 바이트만 현재 슬롯에서 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 원본 WORD/DWORD 의 쓰기 폭을 유지한다(지정 폭 바깥은 보존). (2026-11-10 WB04-E254)
function write(raw, offset, value, width) {
    // 낮은 바이트부터 지정 폭만 대입한다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 현재 float 좌표를 비트 그대로 읽는다. (2026-11-10 WB04-E254)
const floatView = new DataView(new ArrayBuffer(4));
function position(raw, offset) {
    floatView.setUint32(0, read(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// typed: CRT ftol 의 유한 signed64 범위에서 하위 DWORD 를 돌려주고 그 밖은 거부한다. (2026-11-10 WB04-E254)
function truncate(value) {
    const wide = Math.fround(value);
    if (!Number.isFinite(wide) || wide < -CRT_LIMIT || wide >= CRT_LIMIT) throw new RangeError('자산 postPop CRT 정수 범위 오류');
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(wide))));
}

// Native: 원문 _ftol 의 signed64 절삭과 비유한/범위 밖(indefinite)의 하위 DWORD 0 을 보존한다. (2026-11-10 WB04-E254)
function nativeTruncate(value) {
    const wide = Math.fround(value);
    if (!Number.isFinite(wide) || wide < -CRT_LIMIT || wide >= CRT_LIMIT) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(wide))));
}

// 원문 signed count/capacity 비교 뒤에만 실제 배열을 해석하고 store → inc 순서를 유지한다(중복 허용 목록). (2026-11-10 WB04-E254)
function appendAssetList(list, sid) {
    const used = list.readCount();
    const capacity = list.capacity();
    if (capacity > 0xffffffff) throw new RangeError('자산 목록의 DWORD 용량 물리 범위 오류');
    if ((used | 0) >= (capacity | 0)) return;
    if ((used | 0) < 0) throw new RangeError('자산 목록의 음수 저장 위치');
    const storage = list.storage();
    if (used >= storage.length) throw new RangeError('자산 목록의 실제 저장 배열 범위 오류');
    storage[used] = sid;
    list.setCount(list.readCount() + 1);
}

// 00414450: signed count/capacity 관문 뒤에만 중복을 검색하고 저장에는 처음 읽은 count 를 쓴다(신전 목록). (2026-11-10 WB04-E254)
function appendUniqueAssetList(list, sid) {
    const used = list.readCount();
    const capacity = list.capacity();
    if (capacity > 0xffffffff) throw new RangeError('신전 목록 DWORD 용량 물리 범위 오류');
    if ((used | 0) >= (capacity | 0)) return;
    if ((used | 0) > 0) {
        const storage = list.storage();
        let index = 0;
        // 첫 항목은 최초 양수 count 로 고르고 이후 반복은 현재 count 와 비교한다.
        do {
            if (index >= storage.length) throw new RangeError('신전 중복 검색 물리 배열 범위 오류');
            if (storage[index] === sid) return;
            index += 1;
        } while ((index | 0) < (list.readCount() | 0));
    }
    if ((used | 0) < 0) throw new RangeError('신전 목록 음수 저장 위치');
    const storage = list.storage();
    if (used >= storage.length) throw new RangeError('신전 목록 실제 저장 범위 오류');
    storage[used] = sid;
    list.setCount(list.readCount() + 1);
}

export class RawAssetPostPop {
    // 객체 슬롯 풀, 현재 TYPE 표(참조), 공통 장부, 직접 Damageable, 전역/목록 상태, 외부 효과 묶음이다.
    #pool;
    #types;
    #base;
    #damageable;
    #state;
    #hooks;
    // Player 생성·AI·경제와 공유하는 현재 9기록(연결되면 state.players 사본을 쓰지 않음)과 현재 battleOwner DWORD getter 이다.
    #playerRecords = null;
    #localOwner = null;

    // 같은 풀의 직접 Damageable 과 공통 장부를 써야 깊이를 한 번만 줄일 수 있다. 참조 대상은 이 객체보다 오래 살아야 한다. (2026-11-10 WB04-E254)
    constructor(pool, types, base, damageable, state, hooks) {
        if (pool?.edition !== 'patch1078' || base?.pool !== pool || damageable?.pool !== pool || !Array.isArray(types) || types.length !== TYPE_COUNT) {
            throw new TypeError('자산 postPop 풀/판본/타입 표 불일치');
        }
        this.#pool = pool;
        this.#types = types;
        this.#base = base;
        this.#damageable = damageable;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // SquidPostPop 구성 때 같은 풀인지 확인하는 참조이다. (2026-11-10 WB04-E254)
    get pool() { return this.#pool; }

    // 전용 +0x20 몸체가 있는 실제 가상 표인지 인스턴스 없이 읽는다. (2026-11-10 WB04-E254)
    static hasDedicatedVtable(table) { return PATCH_TABLES.has(table >>> 0); }

    // 실제 9 Player 기록(9×0xb4 BYTE)과 현재 소유자 전역 getter 를 빌린다. 크기가 다르면 기존 연결을 바꾸기 전에 거부한다. (2026-11-10 WB04-E254)
    bindPlayerRecords(records, currentLocalOwner = null) {
        if (!(records instanceof Uint8Array) || records.length !== (PLAYER_COUNT + 1) * PLAYER_STRIDE) throw new TypeError('자산 postPop 실제 9 Player 기록 크기 오류');
        if (currentLocalOwner !== null && typeof currentLocalOwner !== 'function') throw new TypeError('자산 postPop 현재 소유자 getter 오류');
        this.#playerRecords = records;
        this.#localOwner = currentLocalOwner;
    }

    // 외부 효과 뒤에도 같은 현재 TYPE 을 다시 읽고 잘못된 TYPE 번호는 명시적으로 거부한다. (2026-11-10 WB04-E254)
    #type(sid) {
        const number = this.#pool.slot(sid)[TYPE_OFFSET];
        if (number >= this.#types.length) throw new RangeError('자산 postPop 현재 TYPE 범위 오류');
        return this.#types[number];
    }

    // free/contained 와 무관하게 원문의 같은 물리 슬롯을 쓴다(SID 범위는 풀이 확인). (2026-11-10 WB04-E254)
    #slot(sid) { return this.#pool.physical(sid); }

    // 가상 표 주소를 실제 몸체 부류로 분류한다(없으면 0). (2026-11-10 WB04-E254)
    #kind(sid) { return PATCH_TABLES.get(read(this.#pool.slot(sid), 0)) ?? 0; }

    // 복원한 가상 표인가. TYPE 번호로 다른 파생 재정의를 대체하지 않는다. (2026-11-10 WB04-E254)
    handles(sid) { return this.#kind(sid) !== 0; }

    // 실제 9기록의 신전 필드 주소이다. field 0..3 은 초기/초기 복사/새/새 복사 순서다. (2026-11-10 WB04-E254)
    #templeOffset(owner, field) {
        if (owner > PLAYER_COUNT || field >= TEMPLE_FIELDS.length) throw new RangeError('자산 postPop Player 신전 필드 범위 오류');
        return owner * PLAYER_STRIDE + PLAYER_TEMPLE + field * 4;
    }

    // 외부 음향/가상 효과 뒤에 바뀐 Player DWORD 도 매번 현재 원문에서 읽는다. (2026-11-10 WB04-E254)
    #playerTemple(owner, field) {
        const offset = this.#templeOffset(owner, field);
        return this.#playerRecords ? read(this.#playerRecords, offset) : this.#state.players[owner][TEMPLE_FIELDS[field]] >>> 0;
    }

    // 공유 기록 모드에서는 별도 상태 사본을 갱신하지 않아 쓰기 원천을 하나로 유지한다. (2026-11-10 WB04-E254)
    #setPlayerTemple(owner, field, sid) {
        const offset = this.#templeOffset(owner, field);
        if (this.#playerRecords) write(this.#playerRecords, offset, sid, 4);
        else this.#state.players[owner][TEMPLE_FIELDS[field]] = sid >>> 0;
    }

    // 콜백이 owner 를 바꾼 경우에도 다음 비교는 바뀐 같은 전역을 쓴다. (2026-11-10 WB04-E254)
    #currentLocalOwner() { return this.#localOwner ? this.#localOwner() >>> 0 : this.#state.localOwner; }

    // 004ac220: WORD 저장 뒤 현재 TYPE 이 현재 다리 TYPE DWORD 와 같으면 표면 통지를 같은 물리 슬롯에 적용한다. (2026-11-10 WB04-E254)
    #setWord(sid, value) {
        write(this.#slot(sid), WORD_OFFSET, value, 2);
        if (this.#pool.slot(sid)[TYPE_OFFSET] === this.#state.readBridgeType()) need(this.#hooks.notifySurface)(sid);
    }

    // 실패할 수 있는 Regular 확보를 실행한다. 확보가 실패해도 뒤의 다음 확보/공통 후처리는 계속한다. (2026-11-10 WB04-E254)
    #regular(sid, event, delay) {
        if (need(this.#hooks.reserveRegular)()) need(this.#hooks.addRegular)(sid, event, delay);
    }

    // 원문 assertion 의 식과 정확한 파일/줄을 현재 장치로 전달하며 성공을 합성하지 않는다. (2026-11-10 WB04-E254)
    #gunAssertion(expression, line) { need(this.#hooks.assertion)(expression, GUN_FILE, line); }

    // 명시 사전 검사: 외부 효과가 필요한 현재 입력 분기를 쓰기 전에 확인한다. 실제 10.78 가상 진입은 이 검사를 거치지 않는다. (2026-11-10 WB04-E254)
    validate(sid, flags, pop = null) {
        const kind = this.#kind(sid);
        const raw = this.#pool.slot(sid);
        const hooks = this.#hooks;
        const state = this.#state;
        const has = (name) => typeof hooks[name] === 'function';
        if (!kind || raw[TYPE_OFFSET] >= this.#types.length) throw new Error('자산 postPop 가상 표/타입 미복원');
        this.#base.validateBase(sid, flags, pop);
        if (kind === KIND.nugget) { if (!has('nuggetPostPop')) throw new Error('너겟 실제 전체 후처리 미연결'); return; }
        if (kind === KIND.sunAviary) { if (!has('sunAviaryPostPop')) throw new Error('훨리베이스 실제 전체 후처리 미연결'); return; }
        if (kind === KIND.rainAviary) { if (!has('rainAviaryPostPop')) throw new Error('맨 오 워 풀 실제 전체 후처리 미연결'); return; }
        if (kind === KIND.windAviary) { if (!has('windAviaryPostPop')) throw new Error('데빌메이커 실제 전체 후처리 미연결'); return; }
        if (kind === KIND.sunFence) { if (!has('sunFencePostPop')) throw new Error('썬 바리케이드 실제 전체 후처리 미연결'); return; }
        if (kind === KIND.arcSpire || kind === KIND.acidFence) {
            if ((flags & 1) && !(raw[EXTRA_OFFSET] & 9) && (!has('reserveRegular') || !has('addRegular'))) throw new Error('스파이어/애시드 최초 Regular 미연결');
            return;
        }
        if (kind === KIND.generator) return;
        if (kind === KIND.growingIce) { if (!has('growingIcePostPop')) throw new Error('성장형 아이스 전체 후처리 미연결'); return; }
        if (kind === KIND.blocker) {
            if (state.readServer() && (flags & 1) && !state.loadingDepth && !has('startBlocker')) throw new Error('바리케이드 최초 프로세스 경계 누락');
            return;
        }
        if (kind >= KIND.gunAngle && kind <= KIND.gunArcher) {
            if (!has('createGunProcess') || (kind === KIND.gunAngle && (!has('gunFrame') || !has('setGraph'))) || (kind === KIND.gunArcher && !has('removeAnimation'))) throw new Error('Gun postPop 효과 누락');
            return;
        }
        if (!(flags & 1) && kind < KIND.vortexRain) return;
        if (!has('reserveRegular') || !has('addRegular')) throw new Error('자산 postPop Regular 경계 누락');
        if (kind === KIND.residence && (!has('surface') || !has('setOwner') || !has('notifySurface'))) throw new Error('거주 구역 지면/소유자/연결 경계 누락');
        if (kind === KIND.geyser && (!has('geyserRegistration') || (!has('atPosition') && !has('visitAtPosition')) || !has('unpop') || !has('repop'))) throw new Error('간헐천 등록/공간 경계 누락');
        if (kind >= KIND.vortexRain && ['typeDword', 'playImpactSound', 'setOwner', 'islandNumber', 'islandExists', 'registerTemple', 'regionNumber', 'destroy', 'hasContent', 'createSlot', 'refreshProduction', 'clearPlacement']
            .some((name) => !has(name))) throw new Error('Vortex 전체 postPop 효과 누락');
        // 손상된 목록 count 는 여기서 거부한다.
        state.currentResidences().items();
        state.currentTemples().items();
        state.currentGeysers().items();
    }

    // Gun 몸체의 네 분기·현재 raw 재조회·flags1 0x1000 의 WORD/프레임 쓰기다. 실제 외부 효과 뒤 현재 extra/TYPE/전역을 읽어 원본 분기 순서를 유지한다. (2026-11-10 WB04-E254)
    #gun(sid, flags, kind) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const state = this.#state;
        if (kind === KIND.gunAngle && (flags & 7)) {
            const graph = need(hooks.gunFrame)(read(pool.slot(sid), FRAME_OFFSET) | 0);
            need(hooks.setGraph)(sid, graph);
        }
        const branch = kind === KIND.gunAngle ? (flags & 7) !== 0 : (flags & 1) !== 0;
        if (branch && (kind !== KIND.gunSun || !(pool.slot(sid)[EXTRA_OFFSET] & 1))) {
            if (state.readServer() && !(pool.slot(sid)[EXTRA_OFFSET] & 1)) {
                if (kind === KIND.gunArcher) need(hooks.removeAnimation)(sid);
                need(hooks.createGunProcess)(sid);
                // 생성/애니메이션 제거가 바꾼 현재 로딩·편집 상태를 읽는다. 이 두 진단은 checking 과 독립이다.
                const clientPop = (flags & 0x50) || (sid > 4 && sid < 15000);
                if (kind === KIND.gunSun && !state.loadingDepth && clientPop) this.#gunAssertion('duringLoad || !(popFlags &( pfDONT_TRANSMIT|pfCLIENT_POP)) && !isClientSid()', 0x32a);
                if (kind === KIND.gunArcher && !state.fortMode && clientPop) this.#gunAssertion('inFortMode || ( !(popFlags &( pfDONT_TRANSMIT|pfCLIENT_POP)) && !isClientSid() )', 0x3c8);
                // 판정 장치가 checking 을 꺼도 이미 고른 실패 진단은 취소하지 않는다.
                if (state.checkingForms() && !need(hooks.checkForms)(sid)) this.#gunAssertion('checkForms()', GUN_CHECK_LINES[kind]);
            }
            if (kind === KIND.gunSun && state.loadingDepth && (this.#type(sid).flags1 & 0x1000)) write(this.#slot(sid), FRAME_OFFSET, read(pool.slot(sid), WORD_OFFSET, 2), 4);
        }
        if (kind === KIND.gunSun && (this.#type(sid).flags1 & 0x1000)) this.#setWord(sid, read(pool.slot(sid), FRAME_OFFSET));
        this.#damageable.damageablePostPop(sid, flags);
    }

    // 거주 구역: extra 차단은 없다. 최초 소유자·목록·예약 세 개·연결·잔여 프레임(+42)을 처리하고 공통 postPop 으로 끝난다. (2026-11-10 WB04-E254)
    #residence(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const state = this.#state;
        if (flags & 1) {
            const raw = pool.slot(sid);
            const surface = need(hooks.surface)(position(raw, X_OFFSET), position(raw, Y_OFFSET));
            if (surface) {
                const owner = pool.slot(surface)[OWNER_OFFSET];
                if (owner) need(hooks.setOwner)(sid, owner);
            }
            appendAssetList(state.currentResidences(), sid);
            this.#regular(sid, 0, 0);
            this.#regular(sid, 1, 0);
            this.#regular(sid, 3, 0);
            this.#slot(sid)[WORD_OFFSET] &= 0xf0;
            need(hooks.notifySurface)(sid);
            // 통지 뒤 현재 프레임·nibble·애니메이션 플래그로 잔여 값을 계산한다.
            let value = read(pool.slot(sid), FRAME_OFFSET) | 0;
            if (state.residenceAnimation) {
                value = (Math.trunc(value / 2) * 2 + (pool.slot(sid)[WORD_OFFSET] & 15)) | 0;
                const group = value < 6 ? 0 : value < 12 ? 1 : value < 18 ? 2 : 3;
                value = (value - group * 6) | 0;
            }
            write(this.#slot(sid), RESIDUE_OFFSET, value >>> 0, 4);
        }
        this.#base.postPopBase(sid, flags);
    }

    // 간헐천: 비용이 실제 결정량 WORD 가 된다. 주변 후보는 원본 순서로 현재 raw TYPE 을 다시 읽는다. native 는 실제 장기 owner 의 정수화다. (2026-11-10 WB04-E254)
    #geyser(sid, flags, native) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const state = this.#state;
        if ((flags & 1) && !(pool.slot(sid)[EXTRA_OFFSET] & 9)) {
            appendAssetList(state.currentGeysers(), sid);
            need(hooks.geyserRegistration)(sid, 1);
            const cost = this.#type(sid).cost;
            this.#setWord(sid, native ? nativeTruncate(cost) : truncate(cost));
            this.#regular(sid, 0, DELAY_TENTH);
            const raw = pool.slot(sid);
            // 현재 후보의 TYPE 을 읽고 Unpop → 같은 물리 프레임 → 현재 가상 Repop 순서를 실행한다.
            const visit = (neighbor) => {
                const type = pool.slot(neighbor)[TYPE_OFFSET];
                if (type === state.readIslandType() || type === state.readStalagType()) {
                    need(hooks.unpop)(neighbor, 0);
                    write(this.#slot(neighbor), FRAME_OFFSET, 8, 4);
                    need(hooks.repop)(neighbor, 0x40);
                }
            };
            if (typeof hooks.visitAtPosition === 'function') hooks.visitAtPosition(position(raw, X_OFFSET), position(raw, Y_OFFSET), visit);
            else {
                // 독립 입력의 기존 배열 경계는 실제 현재 finder 와 구별해 유지한다. 후속 후보의 TYPE 은 각 효과 뒤 다시 읽는다.
                for (const neighbor of need(hooks.atPosition)(position(raw, X_OFFSET), position(raw, Y_OFFSET))) visit(neighbor);
            }
        }
        this.#damageable.damageablePostPop(sid, flags);
    }

    // 신전 공통 몸체: Damageable 을 먼저 불러 공통 깊이를 줄이고, 그 뒤 Player·목록·영역·다른 신전 삭제·slot 생성을 원본 순서로 처리한다. (2026-11-10 WB04-E254)
    #vortexBase(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const state = this.#state;
        this.#damageable.damageablePostPop(sid, flags);
        if (flags & 2) {
            if (!pool.slot(sid)[OWNER_OFFSET]) need(hooks.assertion)('getPlayerId() != INVALID_PLAYER_ID', VORTEX_FILE, 0x259);
            // 진단 복귀 뒤 현재 owner 0 기록도 원문처럼 물리 범위 안에서 허용한다.
            const owner = pool.slot(sid)[OWNER_OFFSET];
            this.#setPlayerTemple(owner, 2, sid);
            const copyOwner = pool.slot(sid)[OWNER_OFFSET];
            this.#setPlayerTemple(copyOwner, 3, this.#playerTemple(copyOwner, 2));
        }
        if (!(flags & 1)) return;
        state.templeCount = (state.templeCount + 1) >>> 0;
        if (!(pool.slot(sid)[EXTRA_OFFSET] & 8)) {
            const sound = need(hooks.typeDword)(sid, 0x74);
            need(hooks.playImpactSound)(sound);
            this.#setWord(sid, 0);
            const owner = pool.slot(sid)[OWNER_OFFSET];
            if (owner && owner <= PLAYER_COUNT && !this.#playerTemple(owner, 0) && !(pool.slot(sid)[EXTRA_OFFSET] & 1)) {
                this.#setPlayerTemple(owner, 0, sid);
                this.#setPlayerTemple(owner, 1, sid);
            }
        }
        if (!(pool.slot(sid)[EXTRA_OFFSET] & 9)) {
            appendUniqueAssetList(state.currentTemples(), sid);
            appendUniqueAssetList(state.currentResidences(), sid);
            if (state.selected && pool.slot(sid)[OWNER_OFFSET] === this.#currentLocalOwner()) {
                if (state.selected > 0xffff) throw new RangeError('신전 선택 SID의 물리 범위 오류');
                const type = read(pool.slot(state.selected), KIND_OFFSET, 2);
                if (type >= this.#types.length) throw new RangeError('신전 선택 Form의 현재 TYPE 물리 범위 오류');
                if (this.#types[type].flags2 & 0x200) need(hooks.clearPlacement)();
            }
            write(this.#slot(sid), FRAME_OFFSET, need(hooks.typeDword)(sid, 0x130), 4);
            const raw = pool.slot(sid);
            const island = need(hooks.islandNumber)(position(raw, X_OFFSET), position(raw, Y_OFFSET));
            if (!need(hooks.islandExists)(island)) need(hooks.assertion)('islandList.exists( islandId )', VORTEX_FILE, 0x281);
            need(hooks.registerTemple)(island, sid);
            need(hooks.setOwner)(sid, pool.slot(sid)[OWNER_OFFSET]);
            if (state.readBoss()) {
                const capacity = pool.capacity;
                let current = 2;
                // 최초 번호는 해제 슬롯을 건너뛴다.
                while (current < capacity && (pool.slot(current)[STATE_OFFSET] & 1)) current += 1;
                // 원본 iterator 처럼 다음 번호를 삭제 호출 전에 미리 확보한다(삭제 효과가 다음 슬롯을 바꿔도 그 번호를 유지).
                while (current < capacity) {
                    const other = current;
                    let next = current + 1;
                    // 다음 할당 슬롯을 찾는다.
                    while (next < capacity && (pool.slot(next)[STATE_OFFSET] & 1)) next += 1;
                    const otherRaw = pool.slot(other);
                    if (!(otherRaw[EXTRA_OFFSET] & 8) && (this.#type(other).flags2 & 0x200) && other !== sid) {
                        if (need(hooks.regionNumber)(position(otherRaw, X_OFFSET), position(otherRaw, Y_OFFSET)) === island) need(hooks.destroy)(other, 0x100000);
                    }
                    current = next;
                }
            }
        }
        if (!state.loadingDepth && !(pool.slot(sid)[EXTRA_OFFSET] & 9)) {
            if (!need(hooks.hasContent)(sid, state.slotType)) need(hooks.createSlot)(sid, state.slotType);
            need(hooks.refreshProduction)();
        }
    }

    // 신전 wrapper: 공통 몸체가 현재 raw 를 바꾼 다음 원소별 프레임(TYPE+0x134, 윈드 제외)과 Regular 예약을 한다. (2026-11-10 WB04-E254)
    #vortex(sid, flags, kind) {
        this.#vortexBase(sid, flags);
        if (flags & 1) {
            if (kind !== KIND.vortexWind) write(this.#slot(sid), FRAME_OFFSET, need(this.#hooks.typeDword)(sid, 0x134), 4);
            this.#regular(sid, 1, kind === KIND.vortexThunder ? DELAY_TENTH : DELAY_VORTEX);
        }
    }

    // 실제 장기 owner 가 현재 가상 표로 고른 간헐천 전체 본문이다(Native 정수화). 다른 자산의 몸체를 넓혀 허용하지 않는다. (2026-11-10 WB04-E254)
    geyserPostPop(sid, flags) { this.#geyser(sid, flags, true); }

    // 가상 표마다 복원한 몸체만 호출한다. 각 몸체는 공통 장부를 내부에서 정확히 한 번 호출한다. 알려진 10.78 부류에는 사전 검사를 끼우지 않는다. (2026-11-10 WB04-E254)
    postPop(sid, flags) {
        const kind = this.#kind(sid);
        const hooks = this.#hooks;
        const state = this.#state;
        const damageable = (parent, mode) => this.#damageable.damageablePostPop(parent, mode);
        if (!kind) this.validate(sid, flags);
        if (kind === KIND.sunAviary) need(hooks.sunAviaryPostPop)(sid, flags);
        else if (kind === KIND.nugget) need(hooks.nuggetPostPop)(sid, flags);
        else if (kind === KIND.rainAviary) need(hooks.rainAviaryPostPop)(sid, flags);
        else if (kind === KIND.windAviary) need(hooks.windAviaryPostPop)(sid, flags);
        else if (kind === KIND.sunFence) need(hooks.sunFencePostPop)(sid, flags);
        else if (kind === KIND.arcSpire) postPopArcSpire(this.#pool, sid, flags, hooks.reserveRegular, hooks.addRegular, damageable);
        else if (kind === KIND.acidFence) postPopAcidFence(this.#pool, sid, flags, hooks.reserveRegular, hooks.addRegular, damageable);
        else if (kind === KIND.growingIce) need(hooks.growingIcePostPop)(sid, flags);
        else if (kind === KIND.generator) {
            // 004ac970 의 최초 현재 theme 조회는 반환값이 버려져도 먼저 수행한다(TYPE 범위 확인).
            if (flags & 1) void this.#type(sid).theme;
            damageable(sid, flags);
        } else if (kind >= KIND.gunAngle && kind <= KIND.gunArcher) this.#gun(sid, flags, kind);
        else if (kind === KIND.gunDecoration) {
            if (flags & 1) this.#regular(sid, 0, DELAY_DECORATION);
            this.#base.postPopBase(sid, flags);
        } else if (kind === KIND.residence) this.#residence(sid, flags);
        else if (kind === KIND.blocker) {
            if (state.readServer() && (flags & 1) && !state.loadingDepth) need(hooks.startBlocker)(sid);
            damageable(sid, flags);
        } else if (kind === KIND.geyser) this.#geyser(sid, flags, false);
        else this.#vortex(sid, flags, kind);
    }
}

// 0041def0 의 좌표 그대로 정수화와 실제 일반 finder 를 공유하는 간헐천 방문 장치를 만든다. 각 visitor 뒤 Next 를 실행한다(후보 next 저장과 각 효과 뒤 현재 링크/버킷 전진 순서 보존).
// 풀·해시·TYPE 표는 반환한 장치보다 오래 살아야 한다. native 면 비유한/범위 밖 좌표를 0 으로 본다. (2026-11-10 WB04-E254)
export function makeAssetPostPopFinder(pool, hash, types, native = false) {
    return (x, y, visitor) => {
        const action = need(visitor);
        const cellX = (native ? nativeTruncate(x) : truncate(x)) | 0;
        const cellY = (native ? nativeTruncate(y) : truncate(y)) | 0;
        const finder = new RawSquidFinder(pool, hash, types);
        // 첫 후보부터 방문 뒤 다음 후보로 전진한다.
        for (let sid = finder.begin({ left: cellX, top: cellY, right: cellX, bottom: cellY }); sid; sid = finder.next()) action(sid);
    };
}
