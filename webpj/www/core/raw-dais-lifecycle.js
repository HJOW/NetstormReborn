// 다이스·제단의 최초 Pop 후처리, 플레이어 SID 쌍 등록·삭제, 제단 파생 전처리를 옮긴 모듈이다(cpppj `o/RawDaisLifecycle.cpp/.h`, 10.78 전용).
//  - postPop: 다이스는 저장된 개수로 첫 프레임을 맞추고 현재 개수 BYTE 를 갱신한 뒤 플레이어 SID 쌍을 등록하고(flags 2 → 두 번째 쌍, flags 1 → 첫 쌍),
//    flags 1 이면 권한이 있을 때 로딩 중 단계 초기화(가상 Unpop → 직접 프레임 쓰기 → Repop) 후 제단을 새로 만들어 owner 와 전체 Pop 을 요청하고,
//    로컬 owner 가 로딩 밖이면 홈 섬 밖 경고를 낸다. 끝은 항상 공통 Damageable 후처리다. 제단은 flags 1 일 때 로컬 21 정기를 예약한 뒤 공통 후처리로 끝난다.
//  - altarPreDestroy: 고유 삭제 실행기(raw-dais-deletion)의 제단 전처리에 위임한다. postDestroy: 실제 Player 기록을 묶었으면 고유 삭제의 다이스 후처리에,
//    아니면 이 모듈의 독립 입력 배열 경로에 맡기고 마지막에 전체 Damageable 삭제 후처리를 부른다.
//  - 선택한 위치에서 도달한 훅만 요구한다(연결하지 않은 훅을 빈 함수로 대신하지 않는다). 훅이 값을 바꿔도 이어서 현재 raw 와 Player 칸을 다시 읽는다.
// 10.72(CD) 분기(판본별 필드 폭·오프셋·CD 전용 TYPE/가상표 계약)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// 근거: cpppj `o/RawDaisLifecycle.cpp/.h`(기준선 1f00a25a 뒤 `20261109-95 CP` 가 고유 삭제 위임으로 바꿈), 원본 관측 daislifecycle(현재판) 10.78 964행.
// (2026-11-13 WB04-E369: 7-P3 RawDaisLifecycle 이식으로 추가)
import { PATCH_ALTAR_VTABLE } from './raw-altar-sacrifice.js';
import { PATCH_DAIS_VTABLE } from './raw-dais-content-notice.js';
import { RawDaisDeletion } from './raw-dais-deletion.js';

// 플레이어 수(중립 0 + 1..8), 10.78 Player 기록의 크기·SID 쌍 네 칸의 시작(+0x8c)·홈 섬 번호(+0x78) 위치이다. (2026-11-13 WB04-E369)
const PLAYER_COUNT = 8;
const PLAYER_RECORD_SIZE = 0xb4;
const PLAYER_PAIR_AT = 0x8c;
const PLAYER_HOME_AT = 0x78;
// 슬롯 필드 위치: TYPE(+10)·좌표(+14/+18)·owner(+34)·프레임(+36)이다. (2026-11-13 WB04-E369)
const TYPE_AT = 10, X_AT = 14, Y_AT = 18, OWNER_AT = 34, FRAME_AT = 36;
// 업그레이드 개수별 다음 첫 프레임(원본 DAT 표 {3, 64, 120, 0})과 단계별 첫 프레임({0, 3, 64, 120}), 표의 길이이다. (2026-11-13 WB04-E369)
const NEXT_FIRST = Object.freeze([3, 64, 120, 0]);
const STAGE_FIRST = Object.freeze([0, 3, 64, 120]);
// 제단 정기 사건 번호(로컬 21)와 홈 섬 밖 경고의 설정 키·채팅 키이다. (2026-11-13 WB04-E369)
const ALTAR_REGULAR_EVENT = 21;
const WARN_OPTION = 'noShowWarnCantUpgrade';
const WARN_CHAT = 'WarnCantUpgradeAltarOffHomeIsland';
// 풀의 TYPE 레코드 수(10.78)이다. (2026-11-13 WB04-E369)
const TYPE_COUNT = 188;

// 호출 중 장치가 바뀌어도 선택한 함수를 복귀까지 유지하도록 필수 훅을 꺼낸다(없으면 거부). (2026-11-13 WB04-E369)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('다이스 수명의 선택된 실제 장치 미연결');
    return hook;
}

// 비정렬 little endian 값을 폭만큼 부호 없이 읽는다(옆 필드를 읽지 않는다). (2026-11-13 WB04-E369)
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// DWORD 비트를 float32 로 읽는다. (2026-11-13 WB04-E369)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(raw, offset), true);
    return view.getFloat32(0, true);
}

// 다이스 수명이 읽는 장기 전역 묶음이다. loading 은 같은 Damageable 후처리의 로딩 깊이를 가진 CarrierPostPopState 이고,
// gameTime 은 실제 로그인 종료 벽시계 시각이다(Process 시계가 아니며 0 이면 게임 시작 전 로딩 중). players 는 독립 입력용 owner 0..8 의 SID 쌍 기록이다.
// 실제 Client 는 bindGameTime({ value }) 로 장기 값을 빌린다(C++ 의 double& 참조 대응).
// 사용: const state = new DaisLifecycleState(carrierState, 0); state.players[1].first = 5. (2026-11-13 WB04-E369)
export class DaisLifecycleState {
    // 같은 로딩 깊이를 읽는 Carrier 후처리 상태이다.
    loading;
    // owner 0..8 의 { first, firstMirror, second, secondMirror, homeIsland } 독립 입력이다(SID 칸은 16비트로 저장).
    players;
    // 독립 게임 시각과 실제 장기 값 상자({ value })이다.
    #gameTime;
    #sharedGameTime = null;

    // 로딩 상태를 빌리고 독립 게임 시각과 Player 쌍 아홉 칸을 0 으로 만든다.
    constructor(loading, gameTime = 0) {
        this.loading = loading;
        this.#gameTime = Number(gameTime);
        this.players = Array.from({ length: PLAYER_COUNT + 1 }, () => ({ first: 0, firstMirror: 0, second: 0, secondMirror: 0, homeIsland: 0 }));
    }

    // 현재 게임 시각이다(실제 장기 상자가 있으면 그 값을 읽고 쓴다).
    get gameTime() { return this.#sharedGameTime ? Number(this.#sharedGameTime.value) : this.#gameTime; }

    set gameTime(value) {
        if (this.#sharedGameTime) this.#sharedGameTime.value = Number(value);
        else this.#gameTime = Number(value);
    }

    // 실제 Client 의 로그인 종료 시각 장기 값({ value })을 빌린다. null 이면 독립 값으로 돌아간다.
    bindGameTime(box) {
        if (box !== null && (typeof box !== 'object' || !('value' in box))) throw new TypeError('다이스 수명 게임 시각은 { value } 상자여야 합니다.');
        this.#sharedGameTime = box;
    }
}

// 다이스·제단 수명 실제 몸체이다. 모든 참조는 몸체보다 오래 살아야 한다.
//  - pool(10.78 SidPool)·hash(SquidHash)·types(188칸 TYPE 레코드)·footprint(RawSquidNeighbors)·temple(RawTempleRitual)·altar(RawAltarSacrifice)·
//    postPop(RawCarrierPostPop)·preDestroy(RawDamageablePreDestroy) 는 같은 풀이어야 한다. daisState 는 DaisRegularState, altarState 는 AltarSacrificeState,
//    state 는 DaisLifecycleState 이다.
//  - hooks: setFrame(sid, frame, 0)·unpop(sid, 0)·repop(sid, 0)·create(type, 0)→SID·setOwner(sid, owner)·pop(sid, x, y, 0)·reserveRegular()→bool·
//    addRegular(sid, 21, 초)·surface(x, y)→SID·hasIsland(sid)→bool·islandNumber(sid)→번호·option(키)→bool·chat(키)·destroy(sid, flags)·
//    damageablePostDestroy(sid, flags) 와 선택 훅 suppressed()·boss()·count(sid)(없으면 기존 독립 상태/신전 의식을 쓴다). 도달할 때만 필요하다.
// 사용: new RawDaisLifecycle(pool, hash, types, footprint, temple, altar, postPop, preDestroy, daisState, altarState, state, hooks).postPop(5, 1). (2026-11-13 WB04-E369)
export class RawDaisLifecycle {
    // 유일한 raw 저장소·해시·현재 TYPE 표와 기존 부품들이다.
    #pool;
    #hash;
    #types;
    #footprint;
    #temple;
    #altar;
    #postPop;
    #preDestroy;
    // 공유 다이스 전역·제단 저항 전역·수명 전역·효과 훅이다(훅은 몸체가 소유하는 사본).
    #daisState;
    #altarState;
    #state;
    #hooks;

    // 같은 풀/188칸 TYPE 표를 확인한다(10.78 전용). 효과 훅은 여기서 요구하지 않고 도달할 때 요구한다.
    constructor(pool, hash, types, footprint, temple, altar, postPop, preDestroy, daisState, altarState, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || pool !== footprint?.pool || pool !== temple?.pool || pool !== altar?.pool || pool !== postPop?.pool || pool !== preDestroy?.pool
            || !Array.isArray(types) || types.length !== TYPE_COUNT) {
            throw new TypeError('다이스 수명의 실제 공통 몸체/필수 효과/같은 풀 연결 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#footprint = footprint;
        this.#temple = temple;
        this.#altar = altar;
        this.#postPop = postPop;
        this.#preDestroy = preDestroy;
        this.#daisState = daisState;
        this.#altarState = altarState;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 후속 가상 등록이 같은 풀을 검증하는 데 쓴다.
    get pool() { return this.#pool; }

    // 효과 뒤에도 항상 현재 owner BYTE 를 읽는다.
    #owner(sid) { return this.#pool.slot(sid)[OWNER_AT]; }

    // 중립 0 도 등록표 범위(0..8)에 포함한다.
    #checkPlayer(owner) {
        if (owner > PLAYER_COUNT) throw new RangeError('다이스 수명 현재Player범위');
    }

    // 연결된 실제 Player 기록(없으면 독립 입력 배열)에서 DWORD 칸을 현재 값으로 읽는다. field 0..3 은 SID 쌍 네 칸, 4 는 홈 섬 번호이다.
    #playerField(owner, field) {
        this.#checkPlayer(owner);
        if (field > 4) throw new RangeError('다이스 Player 필드 범위');
        const records = this.#daisState.playerRecords;
        // 독립 입력이면 기록 객체의 칸을 읽는다.
        if (!records) {
            const value = this.#state.players[owner];
            return field === 4 ? value.homeIsland : [value.first, value.firstMirror, value.second, value.secondMirror][field];
        }
        return read(records, owner * PLAYER_RECORD_SIZE + (field === 4 ? PLAYER_HOME_AT : PLAYER_PAIR_AT + field * 4));
    }

    // SID 칸을 원본 DWORD 폭으로 바꾼다(독립 입력은 16비트 SID 로 저장). field 0..3 만 허용한다.
    #setPlayerField(owner, field, value) {
        this.#checkPlayer(owner);
        if (field > 3) throw new RangeError('다이스 Player SID 필드 범위');
        const records = this.#daisState.playerRecords;
        // 독립 입력이면 SID 16비트 칸에 저장한다.
        if (!records) {
            const target = this.#state.players[owner];
            const sid = value & 0xffff;
            if (field === 0) target.first = sid;
            else if (field === 1) target.firstMirror = sid;
            else if (field === 2) target.second = sid;
            else target.secondMirror = sid;
            return;
        }
        const at = owner * PLAYER_RECORD_SIZE + PLAYER_PAIR_AT + field * 4;
        // 현재 DWORD 의 네 바이트만 바꾼다.
        for (let index = 0; index < 4; index += 1) records[at + index] = (value >>> (index * 8)) & 0xff;
    }

    // 원본 최종 가상표가 다이스/제단이 아니면 파생 수명을 허용하지 않는다.
    handles(sid) {
        const vtable = read(this.#pool.slot(sid), 0);
        return vtable === PATCH_DAIS_VTABLE || vtable === PATCH_ALTAR_VTABLE;
    }

    // 10.78 의 가상 진입 부류는 표시 TYPE 이 바뀌어도 선택한 함수 부류를 유지하므로 가상표만 본다.
    #isDais(sid) { return read(this.#pool.slot(sid), 0) === PATCH_DAIS_VTABLE; }

    // 표시/설정 효과 뒤 현재 미션 억제를 다시 읽는다(주입 훅이 있으면 그 값을 쓴다).
    #suppressed() { return this.#hooks.suppressed ? need(this.#hooks.suppressed)() : this.#daisState.rune.readSuppressDuration(); }

    // 제단 생성 가지의 현재 Client 권한이다(주입 훅이 있으면 그 값을 쓴다).
    #boss() { return this.#hooks.boss ? need(this.#hooks.boss)() : this.#daisState.ritual.currentBoss; }

    // 현재 가상 +0x80 개수를 쓰거나, 독립 입력이면 실제 다이스 몸체의 신전 개수를 읽는다. 다이스가 아닌 가상표를 개수로 바꾸지 않는다.
    #count(sid) {
        if (this.#hooks.count) return need(this.#hooks.count)(sid) >>> 0;
        if (read(this.#pool.slot(sid), 0) !== PATCH_DAIS_VTABLE) throw new Error('다이스 수명의 현재 가상80 미연결');
        return this.#temple.count(sid);
    }

    // 실제 최초 Pop 의 타입/가상표와 초기 Player 범위를 공간 쓰기 전에 검사한다. 공통 Graph 검사는 분배기에 남긴다.
    validate(sid, flags) {
        const raw = this.#pool.slot(sid);
        const vtable = read(raw, 0);
        const rune = this.#daisState.rune;
        // 다이스 TYPE+다이스 가상표 또는 제단 TYPE+제단 가상표만 최초 Pop 대상이다.
        if (!((raw[TYPE_AT] === rune.readDaisType() && vtable === PATCH_DAIS_VTABLE) || (raw[TYPE_AT] === rune.readAltarType() && vtable === PATCH_ALTAR_VTABLE))) {
            throw new TypeError('다이스/제단 Pop 검사 타입/가상표 오류');
        }
        if (this.#pool.slot(sid)[TYPE_AT] !== rune.readDaisType()) return;
        this.#checkPlayer(this.#owner(sid));
        // 로딩 시작 전 저장된 개수의 첫 번호가 표 밖이면 공간 쓰기 전에 거부한다.
        if (!rune.readSuppressDuration() && this.#state.gameTime === 0 && (flags & 7) && this.#daisState.grantFlag(this.#owner(sid)) >= NEXT_FIRST.length) {
            throw new RangeError('다이스 Pop 검사 초기개수첫번호 범위');
        }
    }

    // 로딩 초기화에서는 표시 setter 를 부르지 않고 프레임 DWORD 네 바이트만 직접 쓴다.
    #writeFrame(sid, frame) {
        const raw = this.#pool.physical(sid);
        // 정확한 프레임 필드의 바이트만 쓴다.
        for (let index = 0; index < 4; index += 1) raw[FRAME_AT + index] = (frame >>> (index * 8)) & 0xff;
    }

    // 다이스 최초 Pop 후처리이다. 현재 owner 와 로딩 상태가 최초 생성/경고 조건을 정한다.
    #daisPostPop(sid, flags) {
        const hooks = this.#hooks;
        const state = this.#state;
        const daisState = this.#daisState;
        if (!this.#suppressed()) {
            this.#checkPlayer(this.#owner(sid));
            // 게임 시작 전 로딩의 최초 생성이면 저장된 개수의 다음 첫 프레임으로 맞춘다.
            if (state.gameTime === 0 && (flags & 7)) {
                const saved = daisState.grantFlag(this.#owner(sid));
                if (saved >= NEXT_FIRST.length) throw new RangeError('다이스 로딩개수첫번호범위');
                need(hooks.setFrame)(sid, NEXT_FIRST[saved], 0);
            }
            const owner = this.#owner(sid);
            this.#checkPlayer(owner);
            // 두 판본 모두 가상 Count 앞에서 owner 를 저장한다. PATCH 는 Player 쓰기 주소도 앞에서 고정한다.
            const write = daisState.grantFlagWriter(owner);
            const count = this.#count(sid);
            write((count - 1) & 0xff);
        }
        if ((flags & 2) && this.#owner(sid)) {
            this.#checkPlayer(this.#owner(sid));
            this.#setPlayerField(this.#owner(sid), 2, sid);
            this.#setPlayerField(this.#owner(sid), 3, sid);
        }
        if ((flags & 1) && this.#owner(sid)) {
            this.#checkPlayer(this.#owner(sid));
            this.#setPlayerField(this.#owner(sid), 0, sid);
            this.#setPlayerField(this.#owner(sid), 1, sid);
        }
        if (flags & 1) {
            if (this.#boss()) {
                // 로딩 중 현재 단계가 0 이 아니면 가상 Unpop → 직접 프레임 쓰기 → Repop 으로 단계 첫 프레임을 되돌린다.
                if (state.loading.readLoadingDepth() && this.#temple.stage(sid) !== 0) {
                    need(hooks.unpop)(sid, 0);
                    const count = this.#count(sid);
                    if (count >= STAGE_FIRST.length) throw new RangeError('다이스 현재 단계 첫번호범위');
                    this.#writeFrame(sid, STAGE_FIRST[count]);
                    need(hooks.repop)(sid, 0);
                }
                const created = need(hooks.create)(daisState.rune.readAltarType(), 0);
                need(hooks.setOwner)(created, this.#owner(sid));
                const raw = this.#pool.slot(sid);
                need(hooks.pop)(created, floatAt(raw, X_AT), floatAt(raw, Y_AT), 0);
            }
            // 로컬 owner 이고 로딩 밖이면 현재 지면이 있는 섬 번호가 홈 섬과 다를 때 경고한다.
            if (this.#owner(sid) === daisState.localOwner && !state.loading.readLoadingDepth()) {
                const raw = this.#pool.slot(sid);
                const surface = need(hooks.surface)(floatAt(raw, X_AT), floatAt(raw, Y_AT));
                if (surface && surface < this.#pool.capacity && need(hooks.hasIsland)(surface)) {
                    this.#checkPlayer(this.#owner(sid));
                    if (need(hooks.islandNumber)(surface) !== this.#playerField(this.#owner(sid), 4) && !need(hooks.option)(WARN_OPTION) && !this.#suppressed()) need(hooks.chat)(WARN_CHAT);
                }
            }
        }
        this.#postPop.damageablePostPop(sid, flags);
    }

    // 최초 Pop 후처리이다. 제단의 21 은 로컬 전용이며 최초 예약 뒤 항상 공통 Damageable 후처리를 실행한다.
    postPop(sid, flags) {
        if (!this.handles(sid)) throw new TypeError('다이스 수명 Pop 타입/가상표 오류');
        // 다이스는 전용 후처리 안에서 공통 후처리로 끝난다.
        if (this.#isDais(sid)) {
            this.#daisPostPop(sid, flags);
            return;
        }
        if ((flags & 1) && need(this.#hooks.reserveRegular)()) need(this.#hooks.addRegular)(sid, ALTAR_REGULAR_EVENT, Math.fround(this.#altarState.struggleSeconds));
        this.#postPop.damageablePostPop(sid, flags);
    }

    // 제단 파생 전처리이다. 고유 삭제 실행기가 같은 살아 있는 다이스 삭제 → 주변 추상 룬 정리 → 전체 Damageable 전처리를 실행한다.
    altarPreDestroy(altar, flags) {
        if (!this.handles(altar) || this.#isDais(altar)) throw new TypeError('제단 파생전처리 타입/가상표 오류');
        const hooks = {
            findDais: (sid) => this.#altar.daisAt(sid),
            boss: () => this.#boss(),
            destroy: this.#hooks.destroy,
            pre: (sid, value) => this.#preDestroy.preDestroy(sid, value),
        };
        this.#deletion(hooks).altarPre(altar, flags);
    }

    // 삭제 후처리이다. 실제 Player 기록을 묶은 다이스는 고유 삭제의 기록 대여 경로로, 독립 입력과 제단은 이 모듈의 경로로 처리하고 마지막 전체 Damageable 의 flags 는 그대로 전달한다.
    postDestroy(sid, flags) {
        if (this.#daisState.playerRecords && this.handles(sid) && this.#isDais(sid)) {
            this.#deletion({ players: () => ({ records: this.#daisState.playerRecords }), post: this.#hooks.damageablePostDestroy }).daisPost(sid, flags);
            return;
        }
        if (!this.handles(sid)) throw new TypeError('다이스 수명 삭제후처리 타입/가상표 오류');
        // 다이스의 현재 owner 와 정확히 일치하는 SID 쌍만 지운다(두 번째 쌍 → 첫 쌍 순서).
        if (this.#isDais(sid) && this.#owner(sid)) {
            this.#checkPlayer(this.#owner(sid));
            if (this.#playerField(this.#owner(sid), 2) === sid) {
                this.#setPlayerField(this.#owner(sid), 2, 0);
                this.#setPlayerField(this.#owner(sid), 3, 0);
            }
            if (this.#playerField(this.#owner(sid), 0) === sid) {
                this.#setPlayerField(this.#owner(sid), 0, 0);
                this.#setPlayerField(this.#owner(sid), 1, 0);
            }
        }
        need(this.#hooks.damageablePostDestroy)(sid, flags);
    }

    // 현재 제단/룬 TYPE 을 후보마다 다시 읽는 고유 삭제 실행기를 이 몸체의 풀·발자국 위에 만든다.
    #deletion(hooks) {
        return new RawDaisDeletion(this.#pool, this.#hash, this.#types, this.#footprint, () => this.#daisState.rune.readAltarType(), () => this.#daisState.ritual.currentRuneType, hooks);
    }
}

// 실제 Frame/Factory/Owner/로컬 Host 몸체를 같은 월드의 기존 모듈에 연결한 새 훅 묶음을 돌려준다(나머지 훅은 보존, 풀이 다르면 거부).
//  - setFrame→frame.set, create→factory.create, setOwner→owner.set, addRegular→host.addRegular(로컬 Regular).
//  - Pop·공통 파생 삭제·UI·지면 조회는 호출자가 실제로 연결한다. (2026-11-13 WB04-E369)
export function makeDaisLifecycleHooks(pool, frame, host, factory, owner, hooks = {}) {
    if (pool !== frame?.pool || pool !== host?.pool || pool !== factory?.pool || pool !== owner?.pool) throw new TypeError('다이스 수명 실제 Frame/Host/Factory 풀 불일치');
    return {
        ...hooks,
        setFrame: (sid, value, flags) => frame.set(sid, value, flags),
        create: (type, flags) => factory.create(type, flags),
        setOwner: (sid, value) => owner.set(sid, value),
        addRegular: (sid, event, payload) => host.addRegular(sid, event, payload),
    };
}
