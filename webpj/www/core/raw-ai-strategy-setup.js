// AI 의 첫 영역·다리·생산 준비와 전략 목표 선택을 같은 본체에서 실행하는 모듈이다(cpppj `o/RawAiStrategySetup.cpp/.h`, 10.78: 패치 00414af0 초기화·004139f0 목표 선택·00410360/004103c0 생산 조회·00410420 생산 등록·0040fc60 다리 시드·0040fcb0 다리 보충·0040fb30/0040fbd0 다리 추가/스캔).
//  - new RawAiStrategySetup(memory, player, preparation, finder, create, finderState, types, random, allies, globals, hooks): memory 는 E164 `RawAiBrainMemory`, player 는 `RawPlayerWorld`(현재 Player 9기록 `records` 와 `technology(owner)` 기술 부모), preparation 은 E181 `RawAiPreparation`(공유 00410300 적 선택),
//    finder 는 `RawCarrierPreDestroy`(공유 004ac9f0 내용물 조건 검색 `findContained`), create 는 `RawContainedCreate`(공유 004afa70 실제 생산 내용물 생성), finderState 는 `ContainedFinderState`(004b2270 순회의 현재 권한/내용물 TYPE), types 는 신전의 현재 BYTE TYPE 에서 `theme` 을 읽는 현재 TYPE 표(비어 있지 않아야 함),
//    random 은 다른 게임 계산과 같은 원본 난수(`next(limit)`), allies 는 방향별 현재 9×9 동맹 표(81), 같은 PATCH 풀/Player/동맹 물리 계약을 생성 때 확인한다. globals = { localOwner()(00540c70), noRegion()(0050c038 지역 없음 번호, signed), templeProduct()(0054126c 첫 신전 생산 TYPE), bridgeType()(005411a0 다리 내용물 TYPE),
//    mapLimit()(00531928 signed 지도 좌표 상한), now()(0055b4d0 현재 QWORD 시각) } 는 각 원본 위치에서 현재 값을 다시 읽고, hooks = { regionBounds(region)→[left, top, right, bottom](00432210→0040d900 실제 자기 청크 경계), temple(region)→SID(0046f260 현재 지역 신전),
//    destroy(sid, flags)(가상 +10 실제 내용물 삭제), assertion(text)(원본 assert 이후 계속하는 진단, 미공급이면 명시 실패), demo()→bool(0040fcb0 이 기한 전 로컬 재고를 보충할 때만 읽는 데모 여부) } 는 도달한 위치에서만 요구한다.
//  - initialize(token): 00414af0 — 적(+0x264)이 없거나 방향별 동맹이면 적을 재선택하고, 적/자기/적의 시작 지역이 준비되지 않았으면 아무것도 쓰지 않고 재시도를 허용한다. 준비되면 자기 청크 경계(+0x270..+0x27c)와 중심(+0x268/+0x26c, x87 배정도 중간 합 뒤 float 저장)을 쓰고
//    기술 부모에 다리 재고를 시드(+0x238 개수)하며 현재 신전에 첫 신전 생산이 없으면 생산을 등록하고 신전 theme(+0x390)을 쓴 뒤 완료 표식(+0x258)을 쓴다(최초 월드 스캔 표식 +0x25c 는 별도).
//  - selectGoal(token): 004139f0 — 목표 1(경제 충족·지갑 > 비용+1200(signed)·공격 능력 0x4·적 그래프가 다름)/2(간헐천 미부착·능력 0x10)/4(지도 안 strict 특정 좌표)/3 으로 고르고, 목표가 바뀔 때만 대상/시작점/기한/탐색 상태 여섯 필드를 초기화한다.
//  - findProduct(token, type): mask 8 기술 부모 조회(두 원본 함수가 실제 PE 에서 같음), addProduct(token, parent, type): 중복 진단을 계속 진행시킨 뒤 원격 AI 는 현재 기술 부모 생산을 삭제·재등록하고 신전 생산을 등록, seedBridges/refreshBridges: 기존 다리 스캔과 부족분 생성/기한·로컬 데모 관문 뒤 한 조각만 생성한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E182: AI 전략 준비 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { RawContainedFinder } from './raw-contained-finder.js';
import { selectBridgePattern } from './raw-canon-type.js';

// Player 한 기록과 5f0 재고의 실제 패턴 칸 수, 원본 미정 좌표(−10000.0f) 비트, 목표 선택의 비용 여유이다. (2026-11-09 WB04-E182)
const PLAYER_BYTES = 180;
const BRIDGE_SLOTS = 8;
const UNSET = 0xc61c4000;
const RESERVE = 1200;

// 비정렬 little endian DWORD 를 지정된 물리 저장소에서 읽는다. (2026-11-09 WB04-E182)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 전략 준비 DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 현재 지정 DWORD 만 바꾸고 인접 필드를 보존한다. (2026-11-09 WB04-E182)
function write(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 전략 준비 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 signed 분기에서 DWORD 감김을 보존한다. (2026-11-09 WB04-E182)
const signed = (value) => value | 0;

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E182)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 선택된 하위만 필요로 하며 누락된 효과를 빈 성공으로 처리하지 않는다. (2026-11-09 WB04-E182)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// AI 전략 준비 처리기이다. 생성은 실제 전략 실행 없이 같은 저장소의 연결만 확인한다. (2026-11-09 WB04-E182)
export class RawAiStrategySetup {
    #memory;
    #player;
    #preparation;
    #finder;
    #create;
    #finderState;
    #types;
    #random;
    #allies;
    #globals;
    #hooks;

    // 같은 PATCH 풀과 Player/동맹 물리 계약을 확인한다. (2026-11-09 WB04-E182)
    constructor(memory, player, preparation, finder, create, finderState, types, random, allies, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 전략 준비 본체 소유자 누락');
        const pool = player?.pool;
        if (pool?.edition !== 'patch1078' || preparation?.pool !== pool || finder?.pool !== pool || create?.pool !== pool
            || !(player.records instanceof Uint8Array) || player.records.length !== 9 * PLAYER_BYTES || allies?.length !== 81 || !Array.isArray(types) || types.length === 0) {
            throw new TypeError('AI 전략 준비 같은 PATCH/Player9/동맹81/TYPE 계약 누락');
        }
        if (typeof random?.next !== 'function') throw new TypeError('AI 전략 준비 공용 난수 누락');
        for (const name of ['localOwner', 'noRegion', 'templeProduct', 'bridgeType', 'mapLimit', 'now']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 전략 준비 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#player = player;
        this.#preparation = preparation;
        this.#finder = finder;
        this.#create = create;
        this.#finderState = finderState;
        this.#types = types;
        this.#random = random;
        this.#allies = allies;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 후속 실제 월드 조립 확인용 동일 풀이다. (2026-11-09 WB04-E182)
    get pool() { return this.#player.pool; }

    // 다른 모듈과 같은 살아 있는 본체만 허용하고 하위 효과 이후에도 본체를 다시 조회한다. (2026-11-09 WB04-E182)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 전략 준비 실제0xb08 본체 누락');
        return raw;
    }

    // 현재 본체 DWORD 를 읽고 선택된 DWORD 만 쓴다. (2026-11-09 WB04-E182)
    #field(token, offset) { return read(this.#body(token), offset); }
    #setField(token, offset, value) { write(this.#body(token), offset, value >>> 0); }

    // 축소 이전에 실제 풀 경계를 검사하며 0 도 원본 getter 입력으로 보존한다. (2026-11-09 WB04-E182)
    #physicalSid(value) {
        if (value >= this.pool.capacity || value > 65535) throw new RangeError('AI 전략 준비 SID 범위');
        return value;
    }

    // 별도 지역 사본을 만들지 않고 같은 현재 Player+0x78 을 읽는다. (2026-11-09 WB04-E182)
    #region(owner) {
        if (owner >= 9) throw new RangeError('AI 전략 준비 현재 Player 범위');
        return signed(read(this.#player.records, owner * PLAYER_BYTES + 0x78));
    }

    // 원본 진단이 반환하면 다음 원문 연산으로 진행하며 진단 장치가 없으면 거부한다. (2026-11-09 WB04-E182)
    #assert(text) { need(this.#hooks.assertion, 'AI 전략 준비 진단 미연결')(text); }

    // 범위가 잘못된 타입도 진단 장치 호출 자체는 먼저 보존한다. (2026-11-09 WB04-E182)
    #assertType(type) { if (signed(type) < 1 || this.#types.length <= type) this.#assert('typeNum > 0 && typeNum < numTypes'); }

    // 두 원본 함수(00410360/004103c0)는 서로 다른 이름이어도 실제 PE 에서는 모두 mask 8 이다. (2026-11-09 WB04-E182)
    findProduct(token, type) {
        this.#assertType(type);
        const parent = this.#physicalSid(this.#player.technology(signed(this.#field(token, 0x260))));
        return this.#finder.findContained(parent, 8, type, 0);
    }

    // 00410420: 원본 생산 등록은 자금 차감이나 즉시 Pop 없이 내용물 생성만 요청한다(원격은 현재 기술 부모의 기존 내용물을 삭제하고 재등록). (2026-11-09 WB04-E182)
    addProduct(token, parent, type) {
        this.#assertType(type);
        if (this.findProduct(token, type)) this.#assert('!haveProd( typeNum )');
        if (this.findProduct(token, type)) this.#assert('!haveTeleportable( typeNum )');
        if (this.#field(token, 0x260) !== (this.#globals.localOwner() >>> 0)) {
            const existing = this.findProduct(token, type);
            if (existing) need(this.#hooks.destroy, 'AI 전략 준비 현재 내용물 삭제 미연결')(existing, 0);
            const technology = this.#physicalSid(this.#player.technology(signed(this.#field(token, 0x260))));
            const duplicate = this.#finder.findContained(technology, 8, type, 0);
            if (duplicate) {
                const raw = this.pool.slot(duplicate);
                if (raw[18] | (raw[19] << 8)) this.#assert('!s.findContainedType( CFF_TELEPORT, typeNum )');
            }
            this.#create.create(technology, type, 0, 8, 0x10, 0);
        }
        this.#create.create(parent, type, 0, 0x10, 0x10, 0);
    }

    // 손상된 재고가 원본 고정 배열 뒤의 상태를 덮기 전에 물리 계약 오류로 알린다. (2026-11-09 WB04-E182)
    #storeBridge(token, pattern) {
        const index = signed(this.#field(token, 0x628));
        if (index < 0 || index >= BRIDGE_SLOTS) throw new RangeError('AI 실제 다리 재고8칸 초과');
        this.#setField(token, 0x608 + index * 4, pattern);
    }

    // 0040fb30: 원본은 직선 조각을 강제할 때에도 난수/가중 추첨을 먼저 소비한다(네 번째마다 직선). (2026-11-09 WB04-E182)
    #addBridge(token, parent) {
        const pattern = selectBridgePattern(signed(this.#random.next(10000)));
        this.#storeBridge(token, pattern);
        const serial = this.#field(token, 0x5f4);
        this.#setField(token, 0x5f4, serial + 1);
        if (signed(serial) % 4 === 0) this.#storeBridge(token, 0);
        const index = signed(this.#field(token, 0x628));
        this.#create.create(parent, this.#globals.bridgeType() >>> 0, this.#field(token, 0x608 + index * 4), 0x40, 0x10, 0);
        this.#setField(token, 0x628, this.#field(token, 0x628) + 1);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, bitsFloat(this.#field(token, 0x600)) + this.#globals.now(), true);
        this.#setField(token, 0x630, view.getUint32(0, true));
        this.#setField(token, 0x634, view.getUint32(4, true));
    }

    // 0040fbd0: 사용 중인 다리도 총수에는 포함하고 배치 가능한 패턴만 8칸 재고에 모은다. (2026-11-09 WB04-E182)
    #scanBridges(token, parent) {
        this.#setField(token, 0x628, 0);
        let total = 0;
        const cursor = new RawContainedFinder(this.pool, this.#finderState);
        // 저장된 next 를 이용하는 같은 원본 내용물 순회로 현재 다리 상태를 읽는다.
        for (let sid = cursor.begin(parent); sid; sid = cursor.next()) {
            const raw = this.pool.slot(sid);
            const flags = raw[20] | (raw[21] << 8);
            if (!(flags & 0x40)) continue;
            total += 1;
            if ((flags & 0x140) === 0x40) {
                this.#storeBridge(token, raw[12] | (raw[13] << 8));
                this.#setField(token, 0x628, this.#field(token, 0x628) + 1);
            }
        }
        return total >>> 0;
    }

    // 0040fc60: 목표 개수는 첫 스캔 전에 저장하며 원본 부족분 반복만 수행한다. (2026-11-09 WB04-E182)
    seedBridges(token, parent, count) {
        this.#setField(token, 0x604, count);
        const total = this.#scanBridges(token, parent);
        // 하위가 현재 목표 개수를 바꾸면 다음 비교에서 다시 읽으며 unsigned 감김 뒤 signed 비교한다.
        for (let made = total; signed(made) < signed(this.#field(token, 0x604)); made = (made + 1) >>> 0) this.#addBridge(token, parent);
    }

    // 0040fcb0: 원본은 신전/시간 관문에서 반환할 때에도 먼저 실제 패턴 재고를 갱신한다(한 번에 한 조각만 생성). (2026-11-09 WB04-E182)
    refreshBridges(token, parent) {
        const total = this.#scanBridges(token, parent);
        const owner = this.pool.slot(parent)[34];
        if (owner >= 9) throw new RangeError('AI 다리 보충 현재 Player 범위');
        if (!read(this.#player.records, owner * PLAYER_BYTES + 0x7c) || signed(total) >= signed(this.#field(token, 0x604))) return;
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.#field(token, 0x630), true);
        view.setUint32(4, this.#field(token, 0x634), true);
        if (!(this.#globals.now() >= view.getFloat64(0, true))) {
            if (owner !== (this.#globals.localOwner() >>> 0) || !need(this.#hooks.demo, 'AI 다리 보충 현재 데모 조회 미연결')()) return;
        }
        this.#addBridge(token, parent);
    }

    // 00414af0: 적과 양쪽 시작 지역이 준비되지 않았으면 완료 표식을 쓰지 않는다. (2026-11-09 WB04-E182)
    initialize(token) {
        const enemy = this.#field(token, 0x264);
        if (!enemy) this.#preparation.selectEnemy(token);
        else {
            const owner = this.#field(token, 0x260);
            if (owner >= 9 || enemy >= 9) throw new RangeError('AI 전략 준비 방향별 동맹 범위');
            if (this.#allies[owner * 9 + enemy]) this.#preparation.selectEnemy(token);
        }
        const noRegion = this.#globals.noRegion();
        if (!this.#field(token, 0x264) || this.#region(this.#field(token, 0x260)) === noRegion || this.#region(this.#field(token, 0x264)) === noRegion) return;
        const bounds = need(this.#hooks.regionBounds, 'AI 초기 자기 청크 경계 미연결')(this.#region(this.#field(token, 0x260)));
        // float 경계 네 값의 비트 표현을 같은 본체에 저장한다.
        for (let index = 0; index < 4; index += 1) this.#setField(token, 0x270 + index * 4, floatBits(bounds[index]));
        this.#setField(token, 0x268, this.#field(token, 0x270));
        this.#setField(token, 0x26c, this.#field(token, 0x274));
        const centerX = Math.fround((bitsFloat(this.#field(token, 0x278)) - bitsFloat(this.#field(token, 0x270))) * 0.5 + bitsFloat(this.#field(token, 0x268)));
        this.#setField(token, 0x268, floatBits(centerX));
        const centerY = Math.fround((bitsFloat(this.#field(token, 0x27c)) - bitsFloat(this.#field(token, 0x274))) * 0.5 + bitsFloat(this.#field(token, 0x26c)));
        this.#setField(token, 0x26c, floatBits(centerY));
        const technology = this.#physicalSid(this.#player.technology(signed(this.#field(token, 0x260))));
        this.seedBridges(token, technology, this.#field(token, 0x238));
        const temple = need(this.#hooks.temple, 'AI 초기 현재 신전 조회 미연결')(this.#region(this.#field(token, 0x260)));
        if (temple && !this.findProduct(token, this.#globals.templeProduct() >>> 0)) {
            const sid = this.#physicalSid(temple);
            this.addProduct(token, sid, this.#globals.templeProduct() >>> 0);
            const type = this.pool.slot(sid)[10];
            if (type >= this.#types.length) throw new RangeError('AI 초기 신전 BYTE TYPE 범위');
            this.#setField(token, 0x390, this.#types[type].theme >>> 0);
        }
        this.#setField(token, 0x258, 1);
    }

    // 004139f0: 원본 목표 변경 때만 기존 다리 탐색 상태를 초기화한다. (2026-11-09 WB04-E182)
    selectGoal(token) {
        const attached = this.#field(token, 0x2d0);
        let goal;
        if (attached && signed(this.#field(token, 0x2d8)) > signed(this.#field(token, 0x290) + RESERVE) && (this.#field(token, 0x248) & 4) && this.#field(token, 0x298) !== this.#field(token, 0x2ac)) goal = 1;
        else if (!attached && (this.#field(token, 0x248) & 0x10)) goal = 2;
        else {
            const x = bitsFloat(this.#field(token, 0xaf0));
            const y = bitsFloat(this.#field(token, 0xaf4));
            const limit = Math.fround(this.#globals.mapLimit());
            if (x > 0 && x < limit && y > 0 && y < limit) goal = 4;
            else goal = attached && signed(this.#field(token, 0x2d8)) > signed(this.#field(token, 0x290) + RESERVE) ? 3 : (this.#field(token, 0x248) & 0x10) ? 2 : 3;
        }
        if (goal === this.#field(token, 0x2d4)) return;
        this.#setField(token, 0x3cc, 0);
        this.#setField(token, 0x3d4, UNSET);
        this.#setField(token, 0x3d0, UNSET);
        this.#setField(token, 0x3d8, 1);
        this.#setField(token, 0x5f0, 0);
        this.#setField(token, 0x2d4, goal);
    }
}
