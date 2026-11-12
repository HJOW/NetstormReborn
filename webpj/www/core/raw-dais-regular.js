// 다이스(Dais) 의식의 상위 몸체를 옮긴 모듈이다(cpppj `o/RawDaisRegular.cpp/.h`, 10.78 전용). 룬 304 와 다이스 305/306/307 정기 사건 분배,
// 단계 진행(Progress)·다음 목표 명령(BridgeCommand)·희생 시작/진행/종료(StartSacrifice·SacrificeAdvance·FinishSacrifice)·제단 이탈(AltarEscape)·
// 사제 의식 중지(StopPriestRitual)·다리 자격(CanBridge)·획득 종류 반영(GrantCapturedType)을 원본 순서로 실행한다.
//  - 각 하위 효과 뒤 현재 프레임·소유자·좌표·Player 전역을 매번 다시 읽는다(진단/훅이 값을 바꿔도 이어서 현재 raw 를 쓴다).
//  - 기존 부품을 같은 풀에서 재사용한다: 신전 의식(raw-temple-ritual)·룬 후처리(raw-rune-post-pop)·Carrier 내용물 조회(raw-carrier-pre-destroy)·
//    Host(squid-process)·경로 시작(raw-path-start)·종속 조회(raw-contained-lookup). 프레임 지정·삭제·예약·음향·UI 는 호출자가 주는 훅이며
//    makeDaisRegularHooks 가 실제 Frame/Host/Damageable/로컬 SP 를 연결한 새 묶음을 돌려준다.
//  - 선택한 위치에서 도달한 훅만 요구한다(연결하지 않은 훅을 빈 함수로 대신하지 않는다).
// 10.72(CD) 분기(판본별 필드 폭·CD SP 직접 저장 등)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// 근거: cpppj `o/RawDaisRegular.cpp/.h`(기준선 1f00a25a 뒤 바뀐 부분 포함), 원본 관측 daisregular-core·daisprogress·daisbridge·daiscontentnotice.
// (2026-11-13 WB04-E367: 7-P3 RawDaisRegular 이식으로 추가)
import { RawContainedFinder } from './raw-contained-finder.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawPathProcess } from './raw-path-process.js';
import { pathDirection } from './raw-path-animation.js';
import { setRawSquidWord } from './raw-squid-word.js';
import { RUNE_VTABLE } from './raw-rune-post-pop.js';
import { runRuneRegular, runDaisHandle } from './raw-ritual-regular.js';
import { PATCH_DAIS_VTABLE } from './raw-dais-content-notice.js';

// 실제 Dais 생성자의 최종 가상표(10.78, 0x00507950)는 다이스 알림 모듈의 PATCH_DAIS_VTABLE 을 같이 쓴다. 룬 가상표는 raw-rune-post-pop 의 RUNE_VTABLE 이다. (2026-11-13 WB04-E367)
// 플레이어 수(1..8, 0 은 중립)와 10.78 Player 기록의 크기·희생 결과 DWORD(+0xa0)·획득 플래그 BYTE(+0x71) 위치이다. (2026-11-13 WB04-E367)
const PLAYER_COUNT = 8;
const PLAYER_RECORD_SIZE = 0xb4;
const PLAYER_RESULT_AT = 0xa0;
const PLAYER_GRANT_AT = 0x71;
// 슬롯 필드 위치: TYPE(+10)·원래 owner WORD(+12)·좌표(+14/+18)·종류 WORD(+18)·mask(+20)·owner(+34)·프레임(+36)·희생 중 비트가 있는 extra(+40)이다. (2026-11-13 WB04-E367)
const TYPE_AT = 10, ORIGINAL_OWNER_AT = 12, X_AT = 14, Y_AT = 18, KIND_AT = 18, MASK_AT = 20, OWNER_AT = 34, FRAME_AT = 36, EXTRA_AT = 40;
// 희생 중 표시 비트(extra +40 의 비트 1)와 사제 genus 비트(flags2 의 0x200000)이다. (2026-11-13 WB04-E367)
const SACRIFICING_BIT = 2;
const PRIEST_GENUS = 0x200000;
// 원본 DAT 표의 count 별 다음 첫 프레임({3, 64, 120, 0})이다. (2026-11-13 WB04-E367)
const UPGRADE_FIRST = Object.freeze([3, 64, 120, 0]);
// 최초 예약(0.01)·제단 24 와 희생 진행 지연(0.12)은 float32 값이고, 천둥 소리 지연은 double 2.8 이다. (2026-11-13 WB04-E367)
const INITIAL_DELAY = Math.fround(0.01);
const SACRIFICE_DELAY = Math.fround(0.12);
const THUNDER_DELAY = 2.799999952316284;
// 지연 소리의 flags 와 포획 내용물의 실제 가상 삭제 flags 이다. (2026-11-13 WB04-E367)
const SOUND_FLAGS = 0xc;
const CONSUMED_FLAGS = 0x400048;
// 포획 내용물 mask(4)·사제 포획 표식 mask(0x20)와 사제 소유 알림에 쓰는 5000 SP 이다. (2026-11-13 WB04-E367)
const CAPTURED_MASK = 4;
const PRIEST_CAPTURE_MASK = 0x20;
const REWARD_SP = 5000;

// 호출 중 장치가 바뀌어도 선택한 함수를 복귀까지 유지하도록 필수 훅을 꺼낸다(없으면 거부). (2026-11-13 WB04-E367)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('다이스의 선택된 실제 장치 미연결');
    return hook;
}

// 비정렬 little endian 값을 폭만큼 부호 없이 읽는다(옆 필드를 읽지 않는다). (2026-11-13 WB04-E367)
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// DWORD 비트를 float32 로 읽는다. (2026-11-13 WB04-E367)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(raw, offset), true);
    return view.getFloat32(0, true);
}

// 현재 float 좌표를 원본 __ftol 의 정상 signed32 범위에서 0 쪽 절삭한다(비유한/범위 밖은 거부). (2026-11-13 WB04-E367)
function integer(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('Dais 정상 절삭 범위');
    return Math.trunc(value);
}

// 기록 칸 안에서 owner 번호의 DWORD 위치를 돌려준다(중립 0 과 플레이어 1..8 만). (2026-11-13 WB04-E367)
function checkPlayer(owner, message) {
    if (!Number.isInteger(owner) || owner < 0 || owner > PLAYER_COUNT) throw new RangeError(message);
}

// 다이스가 소비 위치마다 다시 읽는 현재 전역 묶음이다. ritual 은 TempleRitualState(권한·사제 종류), rune 은 RunePostPopState(룬/다이스/제단 TYPE·시간 억제)로
// 신전 의식·룬 조회와 같은 객체를 공유한다. localOwner 는 현재 로컬 플레이어 번호(콜백 뒤 다시 읽음)이고, 실제 Client 는 bindLocalOwner({ value }) 로 장기 값을 빌린다.
// 사용: const state = new DaisRegularState(templeState, runeState, clock); state.localOwner = 1; state.alliances[row * 9 + column] = 1. (2026-11-13 WB04-E367)
export class DaisRegularState {
    // 같은 boss·사제 종류·룬 종류를 신전 의식과 공유하는 상태이다.
    ritual;
    // 같은 제단/다이스 TYPE·시간 억제를 룬 조회와 공유하는 상태이다.
    rune;
    // 공유 Regular 의 현재 게임 시각 상태이다(원문 선언 보존 — 이 몸체는 읽지 않는다).
    clock;
    // 희생 코드표 TYPE(169)·획득 SP 종류(154)·로컬 특수 분기 TYPE(93)의 독립 값이다.
    sacrificeType = 169;
    nuggetType = 154;
    noIslandType = 93;
    // 방향별 9×9 동맹 DWORD 표(행·열 0..8)이다. 독립 입력이 쓴다.
    alliances = new Array((PLAYER_COUNT + 1) * (PLAYER_COUNT + 1)).fill(0);
    // 플레이어별 획득 플래그 BYTE(독립 입력)이다. 실제 Player 기록을 묶으면 기록의 +0x71 을 대신 읽는다.
    grantFlags = new Uint8Array(PLAYER_COUNT + 1);
    // 로컬 획득 종류 선택과 희생 종료에서 멈추는 전역 cue 이다.
    selectedType = 0;
    voiceCue = 0;
    // 사제 genus/깊이/경로 생성 진단(checking) 입력이다: 숫자·불리언 또는 현재 DWORD 를 돌려주는 getter 이며 소비 위치마다 다시 읽는다.
    debug = 0;
    // 실제 005412c0 장기 owner 의 현재 값 getter(없으면 독립 nuggetType)이다.
    sharedNuggetType = null;
    // 공통 WORD setter 가 읽는 현재 다리 TYPE 의 독립 값(초기 82)과 getter 이다.
    bridgeType = 82;
    sharedBridgeType = null;
    // 독립 희생 결과 DWORD 저장소(플레이어 9칸 × 4바이트)이다.
    #resultBytes = new Uint8Array((PLAYER_COUNT + 1) * 4);
    // 실제 Player 기록 9칸(바이트 배열, 없으면 null)이다.
    #playerRecords = null;
    // 독립 로컬 플레이어 번호와 실제 장기 값 상자({ value })이다.
    #localOwner = 0;
    #sharedLocalOwner = null;

    // ritual·rune·clock 을 빌리고 로컬 플레이어 번호를 정한다.
    constructor(ritual, rune, clock = null, localOwner = 0) {
        this.ritual = ritual;
        this.rune = rune;
        this.clock = clock;
        this.#localOwner = localOwner >>> 0;
    }

    // 현재 로컬 플레이어 번호이다(실제 장기 상자가 있으면 그 값을 읽고 쓴다).
    get localOwner() { return this.#sharedLocalOwner ? this.#sharedLocalOwner.value >>> 0 : this.#localOwner; }

    set localOwner(value) {
        if (this.#sharedLocalOwner) this.#sharedLocalOwner.value = value >>> 0;
        else this.#localOwner = value >>> 0;
    }

    // 실제 Client 의 로컬 플레이어 장기 값({ value })을 빌린다. null 이면 독립 값으로 돌아간다.
    bindLocalOwner(box) {
        if (box !== null && (typeof box !== 'object' || !('value' in box))) throw new TypeError('다이스 로컬 owner 는 { value } 상자여야 합니다.');
        this.#sharedLocalOwner = box;
    }

    // 현재 획득 SP 종류 번호이다(실제 장기 getter 우선).
    get currentNuggetType() { return this.sharedNuggetType ? this.sharedNuggetType() >>> 0 : this.nuggetType; }

    // 현재 다리 TYPE 번호이다(공통 WORD setter 가 저장 뒤 같은 현재 번호를 읽는다).
    get currentBridgeType() { return this.sharedBridgeType ? this.sharedBridgeType() >>> 0 : this.bridgeType; }

    // 현재 진단(checking) 값이 비영인지 소비 위치마다 읽는다.
    get currentDebug() {
        const value = typeof this.debug === 'function' ? this.debug() : this.debug;
        return Number(value) !== 0 && value !== false && value !== null && value !== undefined;
    }

    // 실제 Player 기록 9칸(0xb4 바이트씩)을 같은 다이스 수명/획득이 공유하도록 묶는다. 크기가 다르면 거부한다.
    bindPlayerRecords(records) {
        if (records !== null && records.length !== (PLAYER_COUNT + 1) * PLAYER_RECORD_SIZE) throw new RangeError('다이스 실제 Player 기록 크기 오류');
        this.#playerRecords = records;
    }

    // 현재 획득 플래그 BYTE 이다(효과 뒤 다시 읽는다).
    grantFlag(owner) {
        checkPlayer(owner, '다이스 grant Player 범위');
        return this.#playerRecords ? this.#playerRecords[owner * PLAYER_RECORD_SIZE + PLAYER_GRANT_AT] : this.grantFlags[owner];
    }

    // 같은 현재 BYTE 만 쓰고 다른 필드를 보존한다.
    setGrantFlag(owner, value) {
        checkPlayer(owner, '다이스 grant Player 범위');
        if (this.#playerRecords) this.#playerRecords[owner * PLAYER_RECORD_SIZE + PLAYER_GRANT_AT] = value & 0xff;
        else this.grantFlags[owner] = value & 0xff;
    }

    // 희생 결과 DWORD 의 실제 네 바이트(살아 있는 보기)이다. 가상 Count 앞에서 주소를 저장하므로 이후 기록 교체와 무관하게 그 칸을 쓴다.
    playerResultSlot(owner) {
        checkPlayer(owner, '다이스 결과 Player 범위');
        if (!this.#playerRecords) return this.#resultBytes.subarray(owner * 4, owner * 4 + 4);
        const at = owner * PLAYER_RECORD_SIZE + PLAYER_RESULT_AT;
        return this.#playerRecords.subarray(at, at + 4);
    }

    // 현재 기록의 결과 DWORD 에 값을 쓴다(little endian 네 바이트만).
    setPlayerResult(owner, value) {
        const slot = this.playerResultSlot(owner);
        // 네 바이트를 낮은 쪽부터 쓴다.
        for (let index = 0; index < 4; index += 1) slot[index] = (value >>> (index * 8)) & 0xff;
    }

    // 플레이어별 현재 결과 DWORD 목록(부호 없음, 중립 포함 9칸)이다.
    get playerResults() {
        const result = [];
        // 중립 0 과 플레이어 1..8 의 결과를 순서대로 읽는다.
        for (let owner = 0; owner <= PLAYER_COUNT; owner += 1) result.push(read(this.playerResultSlot(owner), 0));
        return result;
    }
}

// 다이스 소비자에게 넘기는 알림 요청이다. kind 는 'Upgrade' | 'Stormpower' | 'LocalPriestDead' | 'OtherPriestDead',
// originalOwner 는 제단에 저장된 원래 owner, currentOwner 는 요청 시 다이스의 현재 owner, status 는 멀티플레이어 상태 helper 반환값이다. (2026-11-13 WB04-E367)
function chat(kind, originalOwner = 0, currentOwner = 0, status = 0) {
    return { kind, originalOwner, currentOwner, status };
}

// 다이스 실제 상위 몸체이다. 모든 참조는 몸체보다 오래 살아야 한다.
//  - pool/hash/types(188칸 TYPE 레코드)/frames(188칸 FrameCodeTable) 와 ritual(RawTempleRitual)·rune(RawRunePostPop)·carrier(RawCarrierPreDestroy)·
//    host(SquidProcessHost)·lookup(RawContainedLookup)·path(RawPathStart) 는 같은 풀이어야 한다. state 는 DaisRegularState, hooks 는 아래 효과들이다.
//  - hooks: setFrame(sid, frame, flags)·destroy(sid, flags)·reserveShared()→bool·addShared(sid, event, payload)·playAt(x, y, name, flags)·reserveSound()→bool·
//    loadSound(name, 0, flags)→소리·attachSound(sid, sound, 0, flags)→프로세스(0 실패)·wallSeconds()→double·setSoundStart(process, 시각)·stopVoice(cue, 0, 0)·
//    processType(parentType)→경로 프로세스·pathMetadata(sid)→{ speed, turningSpeed }·damageable(sid, event, count, payload)→지연·addLocalSp(delta)·
//    normalizeForeignSp(owner)·grantForeignType(owner, type)·localSpecial(0)·localType(type)·refreshOwner(owner)·refreshLocal()·priestExists(owner)→bool·
//    multiplayerStatus()→DWORD·chat({ kind, originalOwner, currentOwner, status })·assertion(expression, file, line)·notifySurface(sid) 와
//    선택 훅 boss()·alliance(row, column)·count(dais)·priestType() (없으면 기존 독립 상태/신전 의식을 쓴다). 도달할 때만 필요하다. (2026-11-13 WB04-E367)
export class RawDaisRegular {
    // 유일한 현재 raw 저장소·해시·현재 TYPE/프레임 표이다.
    #pool;
    #hash;
    #types;
    #frames;
    // 기존 부품: 신전 의식·룬 조회·Carrier 선택·Host·종속 조회·경로 시작이다.
    #ritual;
    #rune;
    #carrier;
    #host;
    #lookup;
    #path;
    // 공유 전역과 효과 훅이다(훅은 몸체가 소유하는 사본).
    #state;
    #hooks;

    // 같은 풀/188칸 TYPE·프레임 표를 확인한다(10.78 전용).
    constructor(pool, hash, types, frames, ritual, rune, carrier, host, lookup, path, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || pool !== ritual?.pool || pool !== rune?.pool || pool !== carrier?.pool || pool !== host?.pool || pool !== lookup?.pool || pool !== path?.pool
            || !Array.isArray(types) || !Array.isArray(frames) || types.length !== 188 || types.length !== frames.length) {
            throw new TypeError('Dais 실제 몸체/풀/필수 효과 연결 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#ritual = ritual;
        this.#rune = rune;
        this.#carrier = carrier;
        this.#host = host;
        this.#lookup = lookup;
        this.#path = path;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 후속 GameWorld/등록이 같은 풀을 검증하는 데 쓴다.
    get pool() { return this.#pool; }

    // 선택된 진단만 실제 장치를 요구하며 정상 복귀 뒤 현재 상태 조회를 계속한다.
    #assert(expression, file, line) {
        if (typeof this.#hooks.assertion !== 'function') throw new Error('Dais의 선택된 실제 원문 진단 장치 미연결');
        need(this.#hooks.assertion)(expression, file, line);
    }

    // 실제 Client 의 현재 권한과 기존 독립 신전 의식 권한을 구별한다.
    #boss() { return this.#hooks.boss ? Boolean(need(this.#hooks.boss)()) : Boolean(this.#state.ritual.currentBoss); }

    // 원문 방향별 9×9 행/열을 바꾸지 않고 현재 동맹 DWORD 를 읽는다.
    #alliance(row, column) {
        this.#player(row);
        this.#player(column);
        return this.#hooks.alliance ? need(this.#hooks.alliance)(row, column) : this.#state.alliances[row * (PLAYER_COUNT + 1) + column];
    }

    // 원문 자기 가상 +0x80 지점만 이 경계에 위임하고 연결이 없으면 기존 신전 의식 count 를 쓴다.
    #count(dais) { return this.#hooks.count ? need(this.#hooks.count)(dais) : this.#ritual.count(dais); }

    // 진단 뒤에도 현재 원본 사제 종류를 실제 소유자에서 읽는다.
    #priestType() { return this.#hooks.priestType ? need(this.#hooks.priestType)() : this.#state.ritual.currentPriestType; }

    // 현재 프레임 DWORD(부호 있음)이다.
    #currentFrame(sid) { return read(this.#pool.slot(sid), FRAME_AT) | 0; }

    // 현재 raw 좌표를 효과 뒤에도 다시 읽는다.
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return { x: floatAt(raw, X_AT), y: floatAt(raw, Y_AT) };
    }

    // 현재 owner BYTE 를 효과 뒤 다시 읽는다.
    #owner(sid) { return this.#pool.slot(sid)[OWNER_AT]; }

    // 현재 raw 타입의 글자별 첫 프레임이다(타입이 표 밖이면 거부).
    #first(sid, letter) {
        const type = this.#pool.slot(sid)[TYPE_AT];
        if (type >= this.#frames.length) throw new RangeError('다이스 현재 frame TYPE 물리 범위');
        return this.#frames[type].run(letter).first;
    }

    // Dais wrapper 의 dead 허용(true)과 CanBridge 의 거부(false)를 기존 공통 선택 몸체에 전달한다.
    #contained(dais, mask, kind, allowDead = true) { return this.#carrier.findContained(dais, mask, kind, 0, allowDead); }

    // 포획한 내용물(mask 4, 종류 조건 없음)의 종류 WORD 이다(없으면 0).
    #capturedWord(dais) {
        const captured = this.#contained(dais, CAPTURED_MASK, 0);
        return captured ? read(this.#pool.slot(captured), KIND_AT, 2) : 0;
    }

    // 중립 0 과 플레이어 1..8 배열만 접근한다. 손상 원본 범위 밖 읽기는 실행하지 않는다.
    #player(owner) { checkPlayer(owner, 'Dais 현재 플레이어 범위'); }

    // 제단 이탈이 결과를 버리는 Dais typed 조회도 같은 일반 finder 에서 수행한다(buried 제외·버킷 순서는 기존 일반 몸체).
    #findDaisAt(sid) {
        const point = this.#position(sid);
        const type = this.#state.rune.readDaisType();
        if (type >= this.#types.length) throw new RangeError('Dais 현재 타입 범위');
        const x = integer(point.x);
        const y = integer(point.y);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 현재 칸의 후보를 원본 순서로 훑어 첫 Dais TYPE 을 돌려준다(추가 free/dead/owner 필터 없음).
        for (let current = finder.begin({ left: x, top: y, right: x, bottom: y }, (this.#types[type].flags2 & 6) === 0 ? 1 : 0); current !== 0; current = finder.next()) {
            if (this.#pool.slot(current)[TYPE_AT] === type) return current;
        }
        return 0;
    }

    // 실제 Rune/Dais 생성자의 가상표만 받아 일반 모형의 기본 가상표로 대체하지 않는다.
    handles(sid) {
        const vtable = read(this.#pool.slot(sid), 0);
        return vtable === RUNE_VTABLE || vtable === PATCH_DAIS_VTABLE;
    }

    // Rune 304 / Dais 305·306·307 및 기존 fallback 의 전체 분배이다. 반환 지연은 원본 float 비트를 유지한다.
    handle(sid, event, count, payload) {
        if (!this.handles(sid)) throw new TypeError('Dais/Rune Handle 타입/가상표 오류');
        const hooks = this.#hooks;
        if (read(this.#pool.slot(sid), 0) === RUNE_VTABLE) {
            return runRuneRegular(this.#pool, sid, event, payload, {
                findDais: (rune) => this.#rune.findOwnedDais(rune),
                progress: (dais) => this.progress(dais),
                destroy: (rune, flags) => need(hooks.destroy)(rune, flags),
            });
        }
        return runDaisHandle(this.#pool, sid, event, count, payload, {
            stage: (dais, frame) => this.#ritual.stage(dais, frame),
            frame: (dais, frame, flags) => need(hooks.setFrame)(dais, frame, flags),
            sacrificeAdvance: (dais) => this.sacrificeAdvance(dais),
            ritual: (dais) => {
                const priest = this.#ritual.findStrongOwnedPriest(dais);
                this.#ritual.ritual(dais, priest, 1);
            },
            damageable: (dais, code, number, value) => need(hooks.damageable)(dais, code, number, value),
        });
    }

    // 현재 K..O 이면 첫 604 를 실제로 지우고 현재 타입 E 첫 프레임을 지정한다.
    stopPriestRitual(priest) {
        RawDaisRegular.stopPriestRitualIn(this.#pool, this.#frames, this.#host, priest, this.#hooks.setFrame);
    }

    // 현재 방향 조회 → 첫 604 실제 삭제 → 다시 현재 TYPE 의 E 프레임 조회/지정 순서를 보존한다. 장기 삭제도 같은 현재 Host/Frame 의 604 중지 본문을 공유한다.
    static stopPriestRitualIn(pool, frames, host, priest, setFrame) {
        const raw = pool.slot(priest);
        const frame = read(raw, FRAME_AT) | 0;
        if (raw[TYPE_AT] >= frames.length) throw new RangeError('다이스 현재 방향 TYPE 물리 범위');
        const direction = pathDirection(frames[raw[TYPE_AT]], frame);
        if (direction <= 9 || direction >= 15) return;
        const process = host.findEvent(priest, 604);
        if (process) host.kill(process, 0);
        const type = pool.slot(priest)[TYPE_AT];
        if (type >= frames.length) throw new RangeError('다이스 현재 frame TYPE 물리 범위');
        need(setFrame)(priest, frames[type].run('E').first, 0);
    }

    // 새 공유 정기 예약 뒤 현재 단계 목표점에서 위치 음향을 낸다(예약 확보가 실패해도 음향은 낸다).
    collapse(dais) {
        const hooks = this.#hooks;
        if (need(hooks.reserveShared)()) need(hooks.addShared)(dais, 0x305, INITIAL_DELAY);
        const point = this.#ritual.stagePoint(dais, this.#ritual.stage(dais));
        need(hooks.playAt)(point.x, point.y, 'altarBurnCollapse.wav', 0);
    }

    // 실제 코드의 extra 비트 2 → 제단 M 프레임 → count 별 다이스 0/45 → 공유 306 예약 순서이다.
    startSacrifice(dais) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        pool.physical(dais)[EXTRA_AT] |= SACRIFICING_BIT;
        const altar = this.#rune.altarAt(dais);
        if (altar && altar < pool.capacity) need(hooks.setFrame)(altar, this.#first(altar, 'M'), 0);
        const count = this.#count(dais);
        if (count === 1) need(hooks.setFrame)(dais, 0, 0);
        else if (count === 2) need(hooks.setFrame)(dais, 45, 0);
        if (need(hooks.reserveShared)()) need(hooks.addShared)(dais, 0x306, INITIAL_DELAY);
    }

    // 전역 169 코드 구간을 사용하며 현재 Dais 타입의 범용 한 번 감김 Advance 로 대신하지 않는다. 다음 프레임/0.12 또는 종료 0 이다.
    sacrificeAdvance(dais) {
        const current = this.#currentFrame(dais);
        const type = this.#state.sacrificeType;
        if (type >= this.#frames.length || current < 0 || current >= this.#frames[type].codes.length) throw new RangeError('Dais 희생 코드 범위');
        const run = this.#frames[type].run(this.#frames[type].codes[current].side);
        let next = run.first;
        // 현재 프레임이 자기 구간 안이면 구간 안에서 한 칸 돌아 진행한다.
        if (run.first <= current && current < run.first + run.count) {
            if (run.count <= 0) throw new Error('Dais 희생 구간 길이 오류');
            next = run.first + ((current - run.first + 1) % run.count);
        }
        if (current <= next) {
            need(this.#hooks.setFrame)(dais, next, 0);
            return SACRIFICE_DELAY;
        }
        this.#pool.physical(dais)[EXTRA_AT] &= ~SACRIFICING_BIT & 0xff;
        this.finishSacrifice(dais);
        return 0;
    }

    // 지연 시각은 실제 부착 성공 뒤에만 읽으며 두 음향의 확보 실패는 서로 독립이다.
    #delaySound(dais, name, delay) {
        const hooks = this.#hooks;
        if (!need(hooks.reserveSound)()) return;
        const sound = need(hooks.loadSound)(name, 0, SOUND_FLAGS);
        const process = need(hooks.attachSound)(dais, sound, 0, SOUND_FLAGS);
        if (process) need(hooks.setSoundStart)(process, need(hooks.wallSeconds)() + delay);
    }

    // 최초 stage 를 유지하되 하위 효과 뒤 사제/권한/포획 조건은 현재 raw 에서 다시 읽는다.
    progress(dais) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        const stage = this.#ritual.stage(dais);
        const priest = this.#ritual.findStrongOwnedPriest(dais);
        if (priest && priest < pool.capacity) this.stopPriestRitual(priest);
        if (stage < 4) {
            if (this.#boss()) this.bridgeCommand(dais, 0, 0);
            this.collapse(dais);
            return;
        }
        const point = this.#position(dais);
        need(hooks.playAt)(point.x, point.y, 'itIsDone2.wav', 0);
        const upgrade = this.#contained(dais, CAPTURED_MASK, pool.slot(dais)[TYPE_AT]) !== 0;
        this.#delaySound(dais, upgrade ? 'priestSacrifice4.wav' : 'priestSacrifice2.wav', upgrade ? 1.5 : 4.0);
        this.#delaySound(dais, 'thunderCrack.wav', THUNDER_DELAY);
        if (upgrade) {
            this.startSacrifice(dais);
            return;
        }
        if (this.#capturedWord(dais) || this.#state.rune.readSuppressDuration()) {
            const altar = this.#rune.altarAt(dais);
            if (altar && altar < pool.capacity) this.altarEscape(altar);
        }
    }

    // 이탈 전에 원본의 Dais 조회도 수행하고 현재 제단 L 프레임 및 공유 24 예약을 연결한다.
    altarEscape(altar) {
        const hooks = this.#hooks;
        this.#findDaisAt(altar);
        need(hooks.setFrame)(altar, this.#first(altar, 'L'), 0);
        if (need(hooks.reserveShared)()) need(hooks.addShared)(altar, 24, SACRIFICE_DELAY);
    }

    // 내용물 첫 후보/next 는 genus 진단 전에 저장하고 복귀 뒤 현재 TYPE/owner/후보 조건/동맹을 읽는다.
    canBridge(dais, priest) {
        const pool = this.#pool;
        if (this.#ritual.stage(dais) === 4 && this.#ritual.hasAbstractRune(dais)) return false;
        const altar = this.#rune.altarAt(dais);
        if (!altar || altar >= pool.capacity) return false;
        // 두 원문 직접 호출은 각각 현재 TYPE/프레임의 signed 코드 BYTE 를 읽는다.
        const direction = () => {
            const type = pool.slot(altar)[TYPE_AT];
            if (type >= this.#frames.length) throw new RangeError('Dais 자격의 현재 제단 TYPE 범위');
            return pathDirection(this.#frames[type], this.#currentFrame(altar));
        };
        if (direction() === 12 || direction() === 11) return false;
        const finder = new RawContainedFinder(pool, this.#carrier.finderState);
        let captured = finder.begin(dais, false);
        // 진단이 고친 현재 사제 TYPE 의 genus 를 같은 실제 표에서 다시 읽는다.
        const genus = () => {
            const type = pool.slot(priest)[TYPE_AT];
            if (type >= this.#types.length) throw new RangeError('Dais 자격의 현재 사제 TYPE 범위');
            return this.#types[type].flags2 >>> 0;
        };
        if (this.#state.currentDebug && !(genus() & PRIEST_GENUS)) this.#assert('priest.getGenus() & gPRIEST', '.\\Dais.cpp', 0x140);
        if (!(genus() & PRIEST_GENUS) || this.#owner(priest) !== this.#owner(dais)) return false;
        // 진단 전 저장한 첫 후보/next 에서 계속하며 현재 mask/kind 와 현재 사제 종류를 비교한다.
        while (captured) {
            const raw = pool.slot(captured);
            if ((raw[MASK_AT] & PRIEST_CAPTURE_MASK) && read(raw, KIND_AT, 2) === this.#priestType()) {
                const originalOwner = raw[ORIGINAL_OWNER_AT] & 127;
                return this.#alliance(this.#owner(altar), originalOwner) === 0;
            }
            captured = finder.next();
        }
        return false;
    }

    // 현재 첫 경로 Form 을 같은 mapper/종속/Host 에서 조회해 이동 깊이를 읽는다(없으면 0).
    moveAwayDepth(priest) {
        const pool = this.#pool;
        const type = need(this.#hooks.processType)(pool.slot(priest)[TYPE_AT]);
        const form = this.#lookup.find(priest, type);
        if (!form) return 0;
        const process = this.#host.fromForm(form);
        if (!(process instanceof RawPathProcess)) throw new Error('Dais 현재 Path form 실제 프로세스 누락');
        // Host.fromForm 과 같은 현재 Form +0x12 DWORD 번호를 저장해 진단 중 경로 삭제를 확인한다.
        const pid = read(pool.slot(form), 18);
        const current = () => {
            if (!this.#host.isRegistered(pid, process)) throw new Error('Dais 이동 깊이 진단 중 실제 경로 수명 종료');
            return process.runtime.moveAwayDepth;
        };
        if (this.#state.currentDebug) {
            // 두 진단의 비교는 최초 depth 를 쓰고 반환만 현재 depth 를 다시 읽는다.
            const saved = current();
            if (saved < 0) this.#assert('0 <= p->moveAwayDepth', '\\Ns\\O\\Carrier.cpp', 0x87b);
            if (this.#state.currentDebug && saved > 3) this.#assert('p->moveAwayDepth <= MAX_MOVE_AWAY_DEPTH', '\\Ns\\O\\Carrier.cpp', 0x87c);
        }
        return current();
    }

    // 실제 명령 꼬리와 마지막 꼭짓점의 위치를 원본과 같은 순서로 지정한다. priest 0 이면 현재 강한 사제를 찾는다.
    bridgeCommand(dais, priest, currentStage) {
        const pool = this.#pool;
        const ritual = this.#ritual;
        if (!priest) priest = ritual.findStrongOwnedPriest(dais);
        if (!priest || !this.canBridge(dais, priest) || ritual.stage(dais) >= 5) return;
        if (!priest || priest >= pool.capacity) return;
        const stage = ritual.stage(dais) + (currentStage ? 0 : 1);
        if (ritual.stage(dais) >= 5) return;
        if (stage > 4) throw new Error('Dais 손상 목표5의 원본 미초기화 좌표 경계');
        const target = ritual.stagePoint(dais, stage);
        const point = this.#position(priest);
        // 사제가 이미 목표 칸이면 명령 대신 즉시 의식(mode 0)을 한다.
        if (integer(point.x) === integer(target.x) && integer(point.y) === integer(target.y)) {
            ritual.ritual(dais, priest, 0);
            return;
        }
        if (!this.#boss()) return;
        const depth = this.moveAwayDepth(priest);
        const next = (depth + 1) | 0;
        if (next >= 4) return;
        const process = this.#path.start({ parent: priest, destination: { x: target.x, y: target.y }, replaceStart: 1, helper: 0, tailWord: dais, tailByte: stage, reserved: 0 });
        if (!process) {
            if (this.#state.currentDebug) this.#assert('p', '.\\Dais.cpp', 0x18e);
            return;
        }
        process.runtime.replacePoint(process.runtime.count - 1, { x: target.x, y: target.y });
        const meta = need(this.#hooks.pathMetadata)(priest);
        process.prepare(0, meta.speed, meta.turningSpeed);
        process.runtime.setMoveAwayDepth(next);
    }

    // 획득 종류를 반영한다: 너겟(로컬 +5000 SP·알림 / 비로컬 SP 정규화)·noIsland(로컬 특수)·그 밖(로컬 선택 초기화 / 비로컬 기술 장부). 획득 플래그를 먼저 지운다.
    grantCapturedType(type, owner) {
        const hooks = this.#hooks;
        const state = this.#state;
        this.#player(owner);
        const local = owner === state.localOwner;
        state.setGrantFlag(owner, 0);
        if (type === state.currentNuggetType) {
            if (local) {
                need(hooks.addLocalSp)(REWARD_SP);
                need(hooks.chat)(chat('Stormpower'));
            } else {
                need(hooks.normalizeForeignSp)(owner);
            }
            return;
        }
        if (type === state.noIslandType) {
            if (local) need(hooks.localSpecial)(0);
            return;
        }
        if (local) {
            need(hooks.localType)(type);
            state.selectedType = type;
        } else {
            need(hooks.grantForeignType)(owner, type);
        }
    }

    // 실제 cue 정지 → 제단 P 프레임 → 현재 count·owner·포획 삭제 → 획득 → UI → 사제 존재 순서를 유지한다.
    finishSacrifice(dais) {
        const hooks = this.#hooks;
        const state = this.#state;
        const pool = this.#pool;
        need(hooks.stopVoice)(state.voiceCue, 0, 0);
        const altar = this.#rune.altarAt(dais);
        if (!altar) this.#assert('altarSid', '.\\Dais.cpp', 0x2ba);
        need(hooks.setFrame)(altar, this.#first(altar, 'P'), 0);
        const originalOwner = read(pool.slot(altar), ORIGINAL_OWNER_AT, 2);
        if (!state.rune.readSuppressDuration() && this.#contained(dais, CAPTURED_MASK, pool.slot(dais)[TYPE_AT])) {
            const count = this.#count(dais);
            if (count >= UPGRADE_FIRST.length) throw new RangeError('Dais 다음 첫 프레임 범위');
            need(hooks.setFrame)(dais, UPGRADE_FIRST[count], 0);
            const owner = this.#owner(dais);
            this.#player(owner);
            // 가상 Count 앞에서 Player 결과 DWORD 의 실제 네 바이트 주소를 저장한다.
            const destination = state.playerResultSlot(owner);
            const currentCount = this.#count(dais);
            // 저장한 주소의 네 바이트에 현재 count 를 쓴다.
            for (let index = 0; index < 4; index += 1) destination[index] = (currentCount >>> (index * 8)) & 0xff;
            let captured = this.#contained(dais, 0, pool.slot(dais)[TYPE_AT]);
            if (captured && captured < pool.capacity) need(hooks.destroy)(captured, CONSUMED_FLAGS);
            captured = this.#contained(dais, 0, this.#priestType());
            if (captured && captured < pool.capacity) {
                need(hooks.destroy)(captured, CONSUMED_FLAGS);
                setRawSquidWord(pool, altar, 0, state.currentBridgeType, hooks.notifySurface);
            }
            if (this.#owner(dais) === state.localOwner) need(hooks.chat)(chat('Upgrade'));
        } else {
            const type = this.#capturedWord(dais);
            if (!state.rune.readSuppressDuration()) this.grantCapturedType(type, this.#owner(dais));
            this.#player(this.#owner(dais));
            state.setPlayerResult(this.#owner(dais), type);
        }
        // 원본은 항상 플레이어 1..8 을 순서대로 갱신한다.
        for (let owner = 1; owner <= PLAYER_COUNT; owner += 1) need(hooks.refreshOwner)(owner);
        if (!state.rune.readSuppressDuration()) {
            if (this.#owner(dais) === state.localOwner) need(hooks.refreshLocal)();
            if (originalOwner === state.localOwner) need(hooks.chat)(chat('LocalPriestDead', originalOwner, this.#owner(dais), need(hooks.multiplayerStatus)()));
            else if (this.#owner(dais) !== state.localOwner) need(hooks.chat)(chat('OtherPriestDead', originalOwner, this.#owner(dais), 0));
            if (!state.rune.readSuppressDuration()) return;
        }
        if (this.#owner(dais) === state.localOwner) {
            need(hooks.addLocalSp)(REWARD_SP);
            let deadOpponent = false;
            this.#player(state.localOwner);
            // 현재 각 플레이어의 사제 존재와 단방향 동맹을 효과 뒤 현재 전역으로 읽는다.
            for (let owner = 1; owner <= PLAYER_COUNT; owner += 1) {
                if (!need(hooks.priestExists)(owner) && !this.#alliance(state.localOwner, owner)) deadOpponent = true;
            }
            if (deadOpponent) need(hooks.chat)(chat('Stormpower'));
        }
    }
}

// 실제 Frame/Host/Damageable/로컬 SP 를 연결한 새 훅 묶음을 돌려준다(나머지 훅은 보존, 풀이 다르면 거부). 비로컬 SP 의 현재 getter/정책 setter 와
// 나머지 UI·음향·삭제·mapper·메타는 호출자가 실제로 연결한다(직접 저장 대체를 설치하지 않는다).
//  - frame: 같은 풀의 SquidFrame, host: 같은 풀의 SquidProcessHost, assets: 같은 풀의 RawAssetRegular, reward: 같은 풀의 SquidReward(10.78 로컬 SP 는 reward.addSp 로 가산)이다.
//  - setFrame→frame.set, addShared→host.addSharedRegular, damageable→assets.damageableHandle, addLocalSp→reward.addSp(현재 로컬 owner, delta). (2026-11-13 WB04-E367)
export function makeDaisRegularHooks(pool, frame, host, assets, reward, state, hooks = {}) {
    if (pool !== frame?.pool || pool !== host?.pool || pool !== assets?.pool || pool !== reward?.pool) throw new TypeError('Dais 실제Frame/Host/SP 풀 연결 누락');
    return {
        ...hooks,
        setFrame: (sid, value, flags) => frame.set(sid, value, flags),
        addShared: (sid, event, payload) => host.addSharedRegular(sid, event, payload),
        damageable: (sid, event, count, payload) => assets.damageableHandle(sid, event, count, payload),
        addLocalSp: (delta) => reward.addSp(state.localOwner, delta),
    };
}
