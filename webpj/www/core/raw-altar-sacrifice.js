// 제단 20~24 사건과 다이스의 희생 완료·사제 해방·삭제 전처리를 같은 raw 월드에 연결한 모듈이다(cpppj `o/RawAltarSacrifice.cpp/.h`, 10.78 전용).
//  - handle: 실제 제단 가상표(0x005076b8)만 받아 raw-ritual-regular 의 runAltarRegular 로 20/21/22/23/24 사건을 분배한다(조회·난수·예약·음향은 훅).
//  - daisAt/count: 기준점의 일반 typed 다이스 조회와 그 다이스의 가상 개수. escapeFrame: 첫 3회는 프레임 지정, 이후에는 결과를 버려도 실제 난수를 소비한다.
//  - releaseFromAltar/releasePriest: 패치 자동 해방(진단 → 가상 Unpop → 현재 P 첫 번호 직접 쓰기 → 가상 Repop)과 포획 사제 해방(WORD 복사·owner 0·중심+0.5 Pop·5초 효과·원본 삭제·소리·604 취소).
//  - daisPreDestroy: 실제 제단 24 전체 순회 → 완료 → 604 취소 → 제단 삭제 → owner 갱신 → 공통 Damageable 의 순서(RawDaisDeletion.daisPre).
// 선택한 위치에서 도달한 훅만 요구하고 호출 효과 뒤에는 현재 raw 좌표·프레임·권한을 다시 읽는다. 10.72(CD) 분기는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// 근거: cpppj `o/RawAltarSacrifice.cpp/.h`(기준선 1f00a25a 뒤 `20261109-95`·`20261110-52 CP` 의 장기 소유자/현재 전역 연결 반영), 원본 관측 altarsacrifice·daisprogress(handle).
// (2026-11-13 WB04-E368: 7-P3 RawAltarSacrifice 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { pathDirection } from './raw-path-animation.js';
import { setRawSquidWord } from './raw-squid-word.js';
import { runAltarReleaseParent } from './raw-altar-release-parent.js';
import { runAltarRegular } from './raw-ritual-regular.js';
import { RawDaisDeletion } from './raw-dais-deletion.js';
import { PATCH_DAIS_VTABLE } from './raw-dais-content-notice.js';

// 실제 제단(164) 생성자의 최종 가상표(10.78)이다. 가상 +5c 는 0044a5e0, +80 은 0044a480 이다. (2026-11-13 WB04-E368)
export const PATCH_ALTAR_VTABLE = 0x005076b8;
// 슬롯 필드 위치: TYPE(+10)·좌표(+14/+18)·프레임(+36)·원래 owner WORD(+12)이다. (2026-11-13 WB04-E368)
const TYPE_AT = 10, X_AT = 14, Y_AT = 18, FRAME_AT = 36, ORIGINAL_OWNER_AT = 12;
// 해방 사제를 제단 중심에서 오른쪽으로 옮기는 float 오프셋(0x3f000000)이다. (2026-11-13 WB04-E368)
const RELEASE_OFFSET = 0.5;

// 선택한 위치에서 필수 훅을 꺼낸다(없으면 거부). (2026-11-13 WB04-E368)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('제단의 선택된 실제 장치 미연결');
    return hook;
}

// 비정렬 little endian 값을 폭만큼 부호 없이 읽는다. (2026-11-13 WB04-E368)
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// DWORD 비트를 float32 로 읽는다. (2026-11-13 WB04-E368)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(raw, offset), true);
    return view.getFloat32(0, true);
}

// 정상 지도 좌표를 0 방향으로 절삭한다(비유한/signed32 밖은 거부). (2026-11-13 WB04-E368)
function cell(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('제단 원본 좌표 절삭 범위');
    return Math.trunc(value);
}

// 제단 몸체이다. 모든 참조는 같은 풀을 공유하고 이 객체보다 오래 살아야 한다.
//  - pool/hash/types(188칸)/frames(188칸 FrameCodeTable), footprint(RawSquidNeighbors), rune(RawRunePostPop), temple(RawTempleRitual), carrier(RawCarrierPreDestroy),
//    dais(RawDaisRegular), advance(RawFrameAdvance), host(SquidProcessHost), damageable(RawDamageablePreDestroy), daisState(DaisRegularState), state(AltarSacrificeState).
//  - hooks: setFrame·destroy·reserveShared·addShared·randomInt·randomFloat·playAt(x, y, name, flags)·playGlobal(name, 0, 0, 0, 1, 0)·removeAnimation·unpop·repop·create(type, flags)·
//    setOwner·pop(sid, x, y, flags)·escapeEffect(sid, seconds, flags)·refreshOwner·damageable 와 assertion(식, 파일, 줄)·boss()·count(dais)·notifySurface(sid). 도달할 때만 필요하다.
// 사용: new RawAltarSacrifice(pool, hash, types, frames, footprint, rune, temple, carrier, dais, advance, host, damageable, daisState, altarState, hooks).handle(altar, 24, 0, 0.12). (2026-11-13 WB04-E368)
export class RawAltarSacrifice {
    // 유일한 raw 저장소·해시·현재 TYPE/프레임 표이다.
    #pool;
    #hash;
    #types;
    #frames;
    // 기존 부품: 발자국·룬 조회·신전 의식·Carrier 선택·다이스·프레임 진행·Host·Damageable 전처리이다.
    #footprint;
    #rune;
    #temple;
    #carrier;
    #dais;
    #advance;
    #host;
    #damageable;
    // 공유 다이스 전역·제단 저항 전역·효과 훅이다.
    #daisState;
    #state;
    #hooks;

    // 같은 풀/188칸 표를 확인한다(10.78 전용).
    constructor(pool, hash, types, frames, footprint, rune, temple, carrier, dais, advance, host, damageable, daisState, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || pool !== footprint?.pool || pool !== rune?.pool || pool !== temple?.pool || pool !== carrier?.pool || pool !== dais?.pool
            || pool !== advance?.pool || pool !== host?.pool || pool !== damageable?.pool || !Array.isArray(types) || !Array.isArray(frames) || types.length !== 188 || frames.length !== types.length) {
            throw new TypeError('제단 실제 하위 몸체/같은 풀/필수 효과 연결 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#footprint = footprint;
        this.#rune = rune;
        this.#temple = temple;
        this.#carrier = carrier;
        this.#dais = dais;
        this.#advance = advance;
        this.#host = host;
        this.#damageable = damageable;
        this.#daisState = daisState;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 후속 실제 세계의 같은 풀 검증에 쓴다.
    get pool() { return this.#pool; }

    // 실제 월드의 권한 getter 를 우선하고 독립 호출자의 기존 신전 의식 상태도 유지한다.
    #boss() { return this.#hooks.boss ? Boolean(need(this.#hooks.boss)()) : Boolean(this.#daisState.ritual.currentBoss); }

    // 원본 isValid 는 SID 1..capacity-1 만 본다(free/dead 를 더하지 않는다).
    #valid(sid) { return sid > 0 && sid < this.#pool.capacity; }

    // 현재 프레임 DWORD(부호 있음)이다.
    #frame(sid) { return read(this.#pool.slot(sid), FRAME_AT) | 0; }

    // 현재 raw 타입을 읽어 실제 코드표의 방향을 돌려준다(TYPE 이 표 밖이면 거부).
    #direction(sid) {
        const type = this.#pool.slot(sid)[TYPE_AT];
        if (type >= this.#frames.length) throw new RangeError('제단 현재 방향 TYPE 물리 범위');
        return pathDirection(this.#frames[type], this.#frame(sid));
    }

    // 현재 타입의 실제 글자 구간 첫 번호이다.
    #first(sid, letter) {
        const type = this.#pool.slot(sid)[TYPE_AT];
        if (type >= this.#frames.length) throw new RangeError('제단 현재 frame TYPE 물리 범위');
        return this.#frames[type].run(letter).first;
    }

    // 표시 setter 를 부르지 않는 원본 해방의 프레임 DWORD 직접 쓰기이다(같은 물리 슬롯의 네 바이트만).
    #writeFrame(sid, value) {
        const raw = this.#pool.physical(sid);
        // 낮은 바이트부터 쓴다.
        for (let index = 0; index < 4; index += 1) raw[FRAME_AT + index] = (value >>> (index * 8)) & 0xff;
    }

    // 호출 뒤 좌표가 바뀌었으면 다음 위치 효과는 새 값을 읽는다.
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return [floatAt(raw, X_AT), floatAt(raw, Y_AT)];
    }

    // 실제 최종 가상표를 쓰는 제단만 처리한다.
    handles(altar) { return read(this.#pool.slot(altar), 0) === PATCH_ALTAR_VTABLE; }

    // 기준점 typed 조회(00448bb0)는 추가 owner/dead/free 필터를 적용하지 않는다. 좌표 절삭 → 현재 다이스 TYPE 의 genus 로 시작 단계 → 첫 일치이다.
    daisAt(sid) {
        const point = this.#position(sid);
        const type = this.#daisState.rune.readDaisType();
        if (type >= this.#types.length) throw new RangeError('제단 현재 다이스 타입 범위');
        const x = cell(point[0]);
        const y = cell(point[1]);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 일반 finder 의 현재 버킷과 buried 조건을 재사용한다.
        for (let found = finder.begin({ left: x, top: y, right: x, bottom: y }, (this.#types[type].flags2 & 6) === 0 ? 1 : 0); found !== 0; found = finder.next()) {
            if (this.#pool.slot(found)[TYPE_AT] === type) return found;
        }
        return 0;
    }

    // 원본 specialized 조회는 kind WORD/순번 0/삭제 false 로만 호출된다. 공통 Carrier 의 같은 몸체를 쓰며 사제 종류 0 은 일반 wildcard 와 달라 거부한다.
    #containedPriest(dais) {
        const priestType = this.#daisState.ritual.currentPriestType;
        if (!priestType) throw new Error('제단 specialized kind0은 일반 Carrier wildcard와 다르다');
        return this.#carrier.findContained(dais, 0, priestType, 0, false);
    }

    // 지정된 대상의 현재 가상 +0x80 만 조회하며 typed 검색이나 대상을 바꾸는 효과를 추가하지 않는다. 미연결 독립 입력은 실제 Dais 가상표에서만 신전 의식 count 를 쓴다.
    #virtualCount(dais) {
        if (this.#hooks.count) return need(this.#hooks.count)(dais);
        if (read(this.#pool.slot(dais), 0) !== PATCH_DAIS_VTABLE) throw new Error('제단 Count의 현재 가상80 분배 장치 미연결');
        return this.#temple.count(dais);
    }

    // 제단 +0x80 은 typed 조회 한 번의 결과를 고정하고 대상의 현재 가상 +0x80 으로 위임한다(다이스가 없으면 0).
    count(altar) {
        const dais = this.daisAt(altar);
        return this.#valid(dais) ? this.#virtualCount(dais) : 0;
    }

    // 첫 프레임/길이는 현재 raw 타입에서 읽으며 count 3 이상의 난수 소비도 생략하지 않는다. count 0 은 방향을 14 - 가상 count 로 다시 정한다(DWORD 감김 보존).
    escapeFrame(dais, count) {
        let direction = this.#direction(dais);
        const type = this.#pool.slot(dais)[TYPE_AT];
        if (count < 3) {
            if (!count) {
                direction = (14 - this.#virtualCount(dais)) | 0;
                // 원문 16 DWORD 방향 표 밖을 BYTE 절단으로 유효 글자에 다시 대응시키지 않는다.
                if (direction < 0 || direction > 15) throw new RangeError('다이스 이탈의 가상 count 첫 프레임 표 범위 오류');
            }
            need(this.#hooks.setFrame)(dais, this.#frames[type].run((direction + 65) & 0xff).first + count, 0);
            return;
        }
        const length = this.#frames[type].run((direction + 65) & 0xff).count;
        need(this.#hooks.randomInt)(3000, length * 1000);
    }

    // 제단/다이스의 선택된 원문 진단만 요구하며 진단 전 효과를 되돌리지 않는다.
    #assertRelease(expression, line) {
        if (!this.#hooks.assertion) throw new Error('제단/다이스 해방 원문 진단 장치 미연결');
        need(this.#hooks.assertion)(expression, '.\\Dais.cpp', line);
    }

    // 해방의 표시 없는 프레임 쓰기는 일반 Frame.Set 으로 대체하지 않는다. 패치는 자동 해방 부모 몸체(진단 → 해방 → Unpop → P 첫 번호 직접 쓰기 → Repop)를 빌린다.
    releaseFromAltar(altar) {
        runAltarReleaseParent(altar, () => (this.#daisState.currentDebug ? 1 : 0), {
            findDais: (sid) => this.daisAt(sid),
            direction: (sid) => this.#direction(sid),
            assertion: this.#hooks.assertion,
            valid: (sid) => this.#valid(sid),
            release: (sid) => this.releasePriest(sid),
            unpop: this.#hooks.unpop,
            repop: this.#hooks.repop,
            plainP: (sid) => this.#writeFrame(sid, this.#first(sid, 'P')),
        });
    }

    // 두 번 중심 조회와 가상 효과 뒤의 현재 raw 재읽기를 원본 순서대로 수행한다.
    releasePriest(dais) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        const state = this.#daisState;
        const altar = this.#rune.altarAt(dais);
        if (!this.#valid(altar)) this.#assertRelease('altar.isValid()', 0x3cb);
        if (state.currentDebug && (this.#direction(altar) === 12 || this.#direction(altar) === 11)) this.#assertRelease('!altar.isInvisibleFrame() && !altar.isSacrifice()', 0x3cc);
        // 원문은 진단이 돌아오면 기존 내용물/음향 조회를 계속한다.
        if (!this.#boss()) this.#assertRelease('boss', 0x3cd);
        const captured = this.#containedPriest(dais);
        if (captured) {
            const created = need(hooks.create)(state.ritual.currentPriestType, 0);
            const word = read(pool.slot(captured), ORIGINAL_OWNER_AT, 2);
            setRawSquidWord(pool, created, word, state.currentBridgeType, hooks.notifySurface);
            need(hooks.setOwner)(created, 0);
            const y = this.#footprint.center(dais)[1];
            const x = this.#footprint.center(dais)[0];
            need(hooks.pop)(created, Math.fround(x + RELEASE_OFFSET), y, 0);
            need(hooks.escapeEffect)(created, 5.0, 0);
            need(hooks.destroy)(captured, 0);
        }
        need(hooks.playGlobal)('priestFree.wav', 0, 0, 0, 1, 0);
        const priest = this.#temple.findStrongOwnedPriest(dais);
        if (this.#valid(priest)) this.#dais.stopPriestRitual(priest);
    }

    // 다음 링크는 원본처럼 현재 후보를 반환하기 전에 저장한다.
    #finishOnAltarForms(dais, altar) {
        RawDaisDeletion.finishForms(this.#pool, this.#host, dais, altar, () => this.#daisState.clock.regularType, () => this.#daisState.clock.sharedRegularType, (sid) => this.#dais.finishSacrifice(sid));
    }

    // 다이스 파생 전처리는 extra 를 최초 한 번만 검사하며 마지막 공통 몸체는 항상 호출한다.
    daisPreDestroy(dais, flags) {
        const pool = this.#pool;
        const state = this.#daisState;
        if (read(pool.slot(dais), 0) !== PATCH_DAIS_VTABLE) throw new TypeError('다이스 파생 전처리 타입/가상표 오류');
        new RawDaisDeletion(pool, this.#hash, this.#types, this.#footprint, () => state.rune.readAltarType(), () => state.ritual.currentRuneType, {
            findAltar: (sid) => this.#rune.altarAt(sid),
            findPriest: (sid) => this.#temple.findStrongOwnedPriest(sid),
            // 독립 typed 의 기존 손상 입력 계약을 유지한다.
            finishForms: (sid, altar) => {
                if (!this.#valid(altar)) throw new Error('다이스 삭제의 제단 프로세스 부모 누락');
                this.#finishOnAltarForms(sid, altar);
            },
            stopPriest: (sid) => this.#dais.stopPriestRitual(sid),
            destroy: this.#hooks.destroy,
            refreshOwner: this.#hooks.refreshOwner,
            pre: (sid, value) => this.#damageable.preDestroy(sid, value),
        }).daisPre(dais, flags);
    }

    // 최상위 Regular 분배도 호출 효과 뒤 같은 풀/전역을 다시 읽는다.
    handle(altar, event, count, payload) {
        if (!this.handles(altar)) throw new TypeError('제단 Handle 타입/가상표 오류');
        const hooks = this.#hooks;
        const state = this.#daisState;
        return runAltarRegular(this.#pool, this.#frames, altar, event, count, payload, this.#state, {
            dais: (sid) => this.daisAt(sid),
            priest: (sid) => this.#containedPriest(sid),
            boss: () => this.#boss(),
            alliance: (original, owner) => state.alliances[original + owner * 9],
            release: (sid) => this.releaseFromAltar(sid),
            escapeFrame: (sid, number) => this.escapeFrame(sid, number),
            advance: (sid, delta, flags) => this.#advance.advance(sid, delta, flags),
            frame: (sid, value, flags) => need(hooks.setFrame)(sid, value, flags),
            destroy: (sid, flags) => need(hooks.destroy)(sid, flags),
            reserveShared: () => need(hooks.reserveShared)(),
            addShared: (sid, code, value) => need(hooks.addShared)(sid, code, value),
            randomInt: (low, high) => need(hooks.randomInt)(low, high),
            randomFloat: (low, high) => need(hooks.randomFloat)(low, high),
            sound: (x, y, name, flags) => need(hooks.playAt)(x, y, name, flags),
            removeAnimation: (sid) => need(hooks.removeAnimation)(sid),
            damageable: (sid, code, number, value) => need(hooks.damageable)(sid, code, number, value),
        });
    }
}

// 실제 Frame/Host/Factory/Damageable/난수를 같은 월드의 기존 모듈에 연결한 새 훅 묶음을 돌려준다(나머지 훅은 보존, 풀이 다르면 거부).
//  - setFrame→frame.set, addShared→host.addSharedRegular, create→factory.create, damageable→assets.damageableHandle,
//    randomInt→현재 게임 난수의 [low, high) 범위 소비, randomFloat→현재 게임 난수의 원문 실수 구간. (2026-11-13 WB04-E368)
export function makeAltarSacrificeHooks(pool, frame, host, factory, assets, random, hooks = {}) {
    if (pool !== frame?.pool || pool !== host?.pool || pool !== factory?.pool || pool !== assets?.pool) throw new TypeError('제단 Frame/Host/Factory/Asset 풀 불일치');
    return {
        ...hooks,
        setFrame: (sid, value, flags) => frame.set(sid, value, flags),
        addShared: (sid, event, payload) => host.addSharedRegular(sid, event, payload),
        create: (type, flags) => factory.create(type, flags),
        damageable: (sid, event, count, payload) => assets.damageableHandle(sid, event, count, payload),
        randomInt: (low, high) => ((low + random.next((high - low) >>> 0)) | 0),
        randomFloat: (low, high) => Math.fround(random.next(65536) * (1 / 65536) * Math.abs(high - low) + low),
    };
}
