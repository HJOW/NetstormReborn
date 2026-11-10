// 초기 요새 자산(거주 구역·간헐천·신전·Factory)의 정기 처리 분배와 공통 Damageable 사건을 옮긴 모듈이다(cpppj `o/RawAssetRegular.cpp/.h`, 10.78 전용).
// +5c 가상 표의 기록값(0x50f540 거주 구역, 0x50da88 간헐천, 0x5196f0/0x5197a8/0x519860/0x519918 신전 네 부류, 0x509148/0x509210 Factory 두 부류)으로 몸체를 고르고
// 부모 사건(event/count/payload)을 실제 raw 슬롯·SID 풀·프레임 지정기·프레임 진행기·게임 난수에 그대로 적용한다. 반환은 다음 예약 값(float32)이며 시각 예약은 ProcessHost 가 맡는다.
//  - 거주 구역: `raw-residence-regular.js`(주민 nibble 토글·장식 조사·Fort Dude 생성 부모·지면 테마)를 훅으로 연결한다.
//  - 간헐천: 남은 결정량 비율(WORD / 비용)별 프레임 구간(0~16·17~32·33~48)과 예약값(0.03·0.06·0.09), 앞뒤 표시 갱신(가상 +88)을 처리한다. 기준선 뒤의 장기 owner 용 `runNativeGeyserRegular` 도 제공한다.
//  - 신전: `runVortexRegular`(event0 Ascend 정리·event1/2 애니메이션·event3 이탈·event4 지역 통지)를 독립 입력(typed)과 실제 Client(Native)가 같은 본문으로 쓴다.
//  - Factory: `raw-factory-regular.js` 의 현재 레벨/프레임/난수 본문을 훅으로 연결한다.
//  - 공통 Damageable: 사건 6699 = 지면 피해 + 가상 HP(0 이면 -1 반환), 6700 = abstract 건설 재시도, 그 밖은 payload 반환.
// 선택된 사건이 요구하는 외부 효과가 없으면 raw 상태를 바꾸기 전(또는 도달한 위치)에서 `Error` 로 거부하며 무효 반환으로 대신하지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// 판단 근거: 원본 10.78 관측 `assetregular-x86.tsv`(1,054행)·`residence-coordinates-x86.tsv`(864행)·`factory-regular-current-x86.tsv`(656행)과 cpppj `AssetRegularTests.cpp`·`ResidenceRegularTests.cpp`·`FactoryRegularCurrentTests.cpp`.
// (2026-11-10 WB04-E255: 자산 정기 처리 분배 이식으로 추가. cpppj 기준선 뒤 da2bf5b8·ed20738b·3c5f5092·7fa2b600·c102cdbf 의 공유 본문 분리·Native 진입을 ② 재반영)
import { FRAME_UPDATE_ALTERNATE } from './squid-frame.js';
import { pathSide } from './raw-path-animation.js';
import { runFactoryRegular } from './raw-factory-regular.js';
import { runResidenceRegular } from './raw-residence-regular.js';

// 부모 슬롯의 위치이다: 가상 표 DWORD(+0), TYPE(+10), 상태 BYTE(+11), 남은 결정량/신전 상태 WORD(+12), x/y float(+14/+18), 소유자 BYTE(+34), 프레임 DWORD(+36), 건설용 extra BYTE(+40). (2026-11-10 WB04-E255)
const TABLE_OFFSET = 0;
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const WORD_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;

// 10.78 의 타입 표 크기와 몸체 부류 번호이다(1 거주 구역, 2 간헐천, 3 신전, 4 Factory). 같은 크기의 프레임 표/원소 표만 받는다. (2026-11-10 WB04-E255)
const TYPE_COUNT = 188;
const KIND_RESIDENCE = 1;
const KIND_GEYSER = 2;
const KIND_VORTEX = 3;
const KIND_FACTORY = 4;

// +5c 가상 표의 실제 기록값이다(명령 실행 주소가 아니며 호스트 포인터로 호출하지 않는다). (2026-11-10 WB04-E255)
const RESIDENCE_TABLES = [0x50f540];
const GEYSER_TABLES = [0x50da88];
const VORTEX_TABLES = [0x5196f0, 0x5197a8, 0x519860, 0x519918];
const FACTORY_TABLES = [0x509148, 0x509210];

// 공통 Damageable 의 지면 피해 TYPE 플래그(flags2 의 0x200000 = 체력이 0 이어도 삭제하지 않는 TYPE)이다. (2026-11-10 WB04-E255)
const KEEP_ON_ZERO_HP = 0x200000;

// 간헐천 예약값이다: 비율 > 0.75 → 0.03, > 0.5 → 0.06, 그 밖 → 0.09 (단정도 float 비트 0x3cf5c28f·0x3d75c28f·0x3db851ec). (2026-11-10 WB04-E255)
const GEYSER_DELAYS = [0x3cf5c28f, 0x3d75c28f, 0x3db851ec];

// 리틀엔디언 값을 읽는다(폭 4 는 부호 없음). (2026-11-10 WB04-E255)
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 지정한 폭만 모은다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 지정한 폭만 쓰고 슬롯의 이웃 필드를 보존한다. (2026-11-10 WB04-E255)
function write(raw, offset, value, width = 4) {
    // 낮은 바이트부터 지정한 폭만 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// 단정도 float 비트를 수로, 수를 비트로 바꾼다(신호 NaN 페이로드를 계산하지 않고 전달만 한다). (2026-11-10 WB04-E255)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// signed 덧셈의 32비트 감김이다. (2026-11-10 WB04-E255)
const add = (value, delta) => (value + delta) | 0;

// 선택된 사건이 요구하는 외부 효과가 없으면 그 위치에서 거부한다. (2026-11-10 WB04-E255)
function needEffect(callback, message) {
    if (typeof callback !== 'function') throw new Error(message);
}

// 거주 구역·간헐천·신전이 읽고 갱신하는 실제 전역이다(cpppj `AssetRegularState`). 신전 목록은 postPop 저장소와 같은 객체를 공유한다.
// 사용: const state = new AssetRegularState(); state.fortMode = true. (2026-11-10 WB04-E255)
export class AssetRegularState {
    // 신전 event1 의 역방향 프레임 진행(편집 모드)을 선택한다.
    fortMode = false;
    // 신전 이탈 event3 의 3회/23회 대기를 선택한다(독립 입력 값, 공유 권한이 있으면 그 값을 쓴다).
    boss = true;
    // 신전 이탈 패킷 생성을 억제하는 원본 전역이다.
    suppressLeaveRequest = false;
    // event4 가 count 4 이후 대기를 생략하는 진단 전역이다. 값(불리언/정수) 또는 소비 위치마다 현재 DWORD 를 돌려주는 함수이다.
    checkingForms = 0;
    // 독립 호출의 고정 옵션 입력이다. 실제 캠페인은 hooks.themeMode 로 현재 설정을 읽는다.
    themeMode = true;
    // event1 이 탐색하는 장식 TYPE 번호이다(두 PE 초기값 102).
    decorationType = 102;
    // 한 번의 finder 최대 조사 개수를 저장하는 원본 전역이다.
    maxDecorationScan = 0;
    // Damageable 의 폭발 연쇄/abstract 건설 재시도 사건 번호이다.
    damageEvent = 6699;
    constructionEvent = 6700;
    // abstract 건설 재시도가 찾는 부착 프로세스 번호이다.
    constructionProcessType = 30;
    // 신전 event4 독립 입력의 ViewMeta 명령(삭제 통지 대상 토큰)과 상위 WORD 를 만드는 서버 선택이다. 실제 Client 는 hooks.descend 에서 현재 값을 읽는다.
    animationType = 0x2000013;
    regionGeneration = 0;
    // Session/물리 저장소와 같은 권한({ boss })만 공유하며 예약/목록은 실행기에 남긴다. 없으면 null.
    sharedAuthority = null;
    // 실제 Client 의 현재 Fort 모드를 돌려주는 함수이다. 없으면 null(독립 입력은 fortMode 를 쓴다).
    sharedFortMode = null;

    // 신전 이탈 대기의 실제 비교 위치에서 읽는 boss 이다(공유 권한이 있으면 그 값). (2026-11-10 WB04-E255)
    readBoss() { return this.sharedAuthority ? Boolean(this.sharedAuthority.boss) : this.boss; }

    // 사건1 의 실제 분기 위치에서 읽는 Fort 모드이다. (2026-11-10 WB04-E255)
    readFortMode() { return this.sharedFortMode ? Boolean(this.sharedFortMode()) : this.fortMode; }

    // event4 의 관문에서 읽는 checking 의 현재 DWORD 이다(0 이면 진단 꺼짐). (2026-11-10 WB04-E255)
    readChecking() {
        const value = typeof this.checkingForms === 'function' ? this.checkingForms() : this.checkingForms;
        return Number(value) >>> 0;
    }
}

// 현재 프레임 DWORD(10.78)를 부호 있는 수로 읽는다. (2026-11-10 WB04-E255)
function currentFrameOf(pool, sid) {
    return read(pool.slot(sid), FRAME_OFFSET) | 0;
}

// 신전 Regular 가 쓰는 프레임/난수 장치 묶음의 필요 장치 검사이다. (2026-11-10 WB04-E255)
const VORTEX_DEVICE_MESSAGES = Object.freeze({
    advance: '신전 현재 프레임 진행 미연결',
    frame: '신전 현재 프레임 지정 미연결',
    random: '신전 현재 게임 난수 미연결',
});

// 신전 4dc220/4dcf00 의 한 원문 본문이다. 독립 typed 호출과 실제 Client(Native)가 같은 코드를 쓴다.
// pool 은 10.78 SidPool, frames 는 TYPE 별 FrameCodeTable 표, state 는 AssetRegularState, hooks 는 AssetRegularHooks 형 객체
//   ({ ascendTick, countForeignOwnedIslands, playerLeavingNotice, state?, suppressLeaveRequest?, islandNumber, requestLeave, descend?, regionNumber?, removeAnimation? }),
// devices = { frame(sid, value, flags), advance(sid, delta, flags), random(limit) }(선택된 사건에서만 요구), damageable 은 그 밖의 사건을 넘기는 공통 처리기이다.
// 반환은 다음 예약 값이다. (2026-11-10 WB04-E255)
export function runVortexRegular(pool, frames, state, hooks, devices, damageable, sid, event, count, payload) {
    const currentFrame = () => currentFrameOf(pool, sid);
    if (event === 0) {
        needEffect(hooks.ascendTick, '신전 Ascend 정리 Handle 누락');
        return hooks.ascendTick(sid, count) ? payload : -1;
    }
    if (event === 1) {
        let delta = 1;
        if (state.readFortMode()) {
            // 편집 모드는 현재 방향 글자 구간의 길이 - 1 칸을 되감는다.
            const type = pool.slot(sid)[TYPE_OFFSET];
            const side = pathSide(frames[type], currentFrame());
            delta = add(frames[type].run(side & 255).count, -1);
        }
        needEffect(devices.advance, VORTEX_DEVICE_MESSAGES.advance);
        devices.advance(sid, delta, FRAME_UPDATE_ALTERNATE);
        return payload;
    }
    if (event === 2) {
        if ((count & 31) === 31) {
            needEffect(devices.frame, VORTEX_DEVICE_MESSAGES.frame);
            devices.frame(sid, 0, FRAME_UPDATE_ALTERNATE);
            return 2;
        }
        if (!currentFrame()) {
            needEffect(devices.frame, VORTEX_DEVICE_MESSAGES.frame);
            devices.frame(sid, 1, FRAME_UPDATE_ALTERNATE);
        }
        needEffect(devices.random, VORTEX_DEVICE_MESSAGES.random);
        needEffect(devices.advance, VORTEX_DEVICE_MESSAGES.advance);
        devices.advance(sid, (devices.random(2) + 1) | 0, FRAME_UPDATE_ALTERNATE);
        return Math.fround(0.1);
    }
    if (event === 3) {
        const signedCount = count | 0;
        if (!count && read(pool.slot(sid), WORD_OFFSET, 2) === 2) return 0;
        needEffect(hooks.countForeignOwnedIslands, '신전 이탈 다른 소유자 섬 조회 누락');
        if (!count) {
            needEffect(hooks.playerLeavingNotice, '신전 이탈 알림 누락');
            // 상태 WORD 2 는 실제 Client 의 004ac220 통지(hooks.state)를 거치거나 독립 입력에서는 직접 쓴다.
            if (hooks.state) hooks.state(sid, 2);
            else write(pool.allocated(sid), WORD_OFFSET, 2, 2);
            hooks.playerLeavingNotice(pool.slot(sid)[OWNER_OFFSET]);
        }
        // 반환은 원본처럼 버리지만 조회 자체는 매번 실행한다.
        hooks.countForeignOwnedIslands(pool.slot(sid)[OWNER_OFFSET]);
        if (signedCount < (state.readBoss() ? 3 : 23)) return 1;
        if (!(hooks.suppressLeaveRequest ? hooks.suppressLeaveRequest() : state.suppressLeaveRequest)) {
            needEffect(hooks.islandNumber, '신전 이탈 섬 조회 누락');
            needEffect(hooks.requestLeave, '신전 이탈 요청 누락');
            const raw = pool.slot(sid);
            const island = hooks.islandNumber(floatOf(read(raw, X_OFFSET)), floatOf(read(raw, Y_OFFSET)));
            hooks.requestLeave(sid, island & 255);
            if (pool.slot(sid)[STATE_OFFSET] & 1) return -1;
        }
        return 0;
    }
    if (event === 4) {
        const signedCount = count | 0;
        if (!count) {
            if (hooks.state) hooks.state(sid, 4);
            else write(pool.allocated(sid), WORD_OFFSET, 4, 2);
        }
        if (!state.readChecking() && Math.fround(signedCount) <= 4) return 1;
        const raw = pool.slot(sid);
        if (hooks.descend) {
            hooks.descend(floatOf(read(raw, X_OFFSET)), floatOf(read(raw, Y_OFFSET)));
        } else {
            // 기존 독립 fixture 의 명시 입력 경계만 보존한다. 실제 Client 는 위의 Descend 본체를 쓴다.
            needEffect(hooks.regionNumber, '신전 event4 지역 조회 누락');
            needEffect(hooks.removeAnimation, '신전 event4 통지 누락');
            const region = hooks.regionNumber(floatOf(read(raw, X_OFFSET)), floatOf(read(raw, Y_OFFSET)));
            const token = ((state.regionGeneration << 16) | region) >>> 0;
            hooks.removeAnimation(state.animationType, token);
        }
        if (hooks.state) hooks.state(sid, 0);
        else write(pool.allocated(sid), WORD_OFFSET, 0, 2);
        return 0;
    }
    needEffect(damageable, '신전 현재 Damageable 부모 미연결');
    return damageable(sid, event, count, payload);
}

// 간헐천 485640 의 실제 표시 접두 → 현재 비용/프레임 → 후행 Damageable 을 typed 본문과 분리해 장기 owner 에 제공한다.
// pool 은 10.78 SidPool, types 는 TYPE 별 { cost } 표, display(sid, flags) 는 가상 +88 표시 갱신, damageable(sid, event, count, payload) 는 후행 공통 처리기이다.
// 비용과 프레임은 원문의 반복 조회 위치마다 현재 슬롯에서 다시 읽는다. (2026-11-10 WB04-E255)
export function runNativeGeyserRegular(pool, types, sid, event, count, payload, display, damageable) {
    const update = () => {
        const current = display;
        needEffect(current, 'Native간헐천현재표시미연결');
        current(sid, 0);
    };
    update();
    if (!event) {
        // 남은 결정량 / 현재 TYPE 비용이다(0 비용은 IEEE 나눗셈 그대로).
        const ratio = () => {
            const quantity = read(pool.slot(sid), WORD_OFFSET, 2);
            const type = pool.slot(sid)[TYPE_OFFSET];
            if (type >= types.length) throw new RangeError('Native간헐천현재비용TYPE물리부족');
            return quantity / types[type].cost;
        };
        const frame = () => read(pool.slot(sid), FRAME_OFFSET) | 0;
        let next;
        if (ratio() > 0.75 && ratio() <= 1.0) {
            next = frame() < 16 ? add(frame(), 1) : 0;
            payload = floatOf(GEYSER_DELAYS[0]);
        } else if (ratio() > 0.5 && ratio() <= 0.75) {
            next = frame() < 32 ? add(frame(), 1) : 17;
            payload = floatOf(GEYSER_DELAYS[1]);
        } else {
            next = frame() < 48 ? add(frame(), 1) : 33;
            payload = floatOf(GEYSER_DELAYS[2]);
        }
        // 직접 DWORD 쓰기 뒤 현재 가상 +88 이며 별도 SetFrame/Unpop 을 더하지 않는다.
        write(pool.physical(sid), FRAME_OFFSET, next >>> 0);
        update();
    }
    needEffect(damageable, 'Native간헐천의실제DamageableHandle미연결');
    return damageable(sid, event, count, payload);
}

// +5c 가상 표의 실제 주소로 몸체를 선택하고 공통 Damageable 의 event 분배까지 복원한다.
// pool/hash/types/frames/elements/setter/advance/random/assets/state 는 이 객체보다 오래 살아야 하며(복사하지 않음) 지정기·진행기는 같은 풀이어야 한다.
// hooks 는 외부 효과 묶음(notifySurface·surface·setOwner·residenceEvent2·displayUpdate·ascendTick·playerLeavingNotice·countForeignOwnedIslands·islandNumber·requestLeave·regionNumber·removeAnimation·
//   damageGround·hitPoints·findAttached·startConstruction·beginFactoryUpgrade·finishFactoryUpgrade·factoryFlare·themeMode·state·suppressLeaveRequest·descend·virtualCount)이며 필요한 사건에서만 요구한다.
// 사용: const regular = new RawAssetRegular(pool, hash, types, frames, elements, setter, advance, random, assets, state, hooks); regular.handle(sid, event, count, payload). (2026-11-10 WB04-E255)
export class RawAssetRegular {
    // 실행 부모와 이웃을 다시 조회하는 단일 raw 풀, 장식 finder 의 동적 해시, 비용/genus 를 읽는 공유 TYPE 표, 현재 프레임 코드 표, 타입 원소 표이다.
    #pool;
    #hash;
    #types;
    #frames;
    #elements;
    // 실제 표시/해시/공간 효과를 보존하는 프레임 지정기, 한 번 감싸기 프레임 진행기, 모든 자산 사건이 소비하는 같은 난수 상태이다.
    #setter;
    #advance;
    #random;
    // postPop 과 같은 신전 목록을 가진 AssetPostPopState, 모드/통지/최대 조사 전역, 외부 효과 묶음이다.
    #assets;
    #state;
    #hooks;

    // 같은 월드의 표·프레임 지정기만 허용하고 비용·원소를 사본으로 만들지 않는다. 불일치는 상태를 바꾸기 전에 `TypeError` 로 거부한다. (2026-11-10 WB04-E255)
    constructor(pool, hash, types, frames, elements, setter, advance, random, assets, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('자산 Handle 은 10.78 SidPool 만 받습니다');
        if (setter?.pool !== pool || advance?.pool !== pool || advance?.setter !== setter ||
            types.length !== TYPE_COUNT || frames.length !== TYPE_COUNT || elements.length !== TYPE_COUNT) {
            throw new TypeError('자산 Handle 풀/프레임/타입 표 불일치');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#elements = elements;
        this.#setter = setter;
        this.#advance = advance;
        this.#random = random;
        this.#assets = assets;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 다른 월드의 풀을 섞지 않게 어댑터가 확인하는 풀이다. (2026-11-10 WB04-E255)
    get pool() { return this.#pool; }

    // raw 의 vtable 기록값만 비교해 몸체 부류(1 거주 구역/2 간헐천/3 신전/4 Factory, 없으면 0)를 구한다. 원본 주소를 호출하지 않는다. (2026-11-10 WB04-E255)
    #kind(sid) {
        const table = read(this.#pool.slot(sid), TABLE_OFFSET);
        if (RESIDENCE_TABLES.includes(table)) return KIND_RESIDENCE;
        if (GEYSER_TABLES.includes(table)) return KIND_GEYSER;
        if (VORTEX_TABLES.includes(table)) return KIND_VORTEX;
        if (FACTORY_TABLES.includes(table)) return KIND_FACTORY;
        return 0;
    }

    // +5c 가 복원된 부모인지 실제 가상 표로 확인한다. (2026-11-10 WB04-E255)
    handles(sid) { return this.#kind(sid) !== 0; }

    // 해당 부모의 실제 사건을 실행한다. 최초 float 는 지연이 아닌 payload 이며 시각 예약은 ProcessHost 가 담당한다.
    // 잘못된 TYPE/가상 표는 외부 효과보다 먼저 거부한다(`RangeError`/`TypeError`). (2026-11-10 WB04-E255)
    handle(sid, event, count, payload) {
        if (this.#pool.slot(sid)[TYPE_OFFSET] >= this.#types.length) throw new RangeError('자산 Handle 타입 범위 오류');
        switch (this.#kind(sid)) {
            case KIND_RESIDENCE: return this.#residence(sid, event, payload);
            case KIND_GEYSER: return this.#geyser(sid, event, count, payload);
            case KIND_VORTEX: return this.#vortex(sid, event, count, payload);
            case KIND_FACTORY: return this.#factory(sid, event, count, payload);
            default: throw new TypeError('자산 Handle 가상 표 미복원');
        }
    }

    // 이미 복원된 사제 등과 합성할 처리기이다. 미복원 부모를 받아주는 기본 payload 처리는 넣지 않으며 fallback 이 없으면 거부한다. (2026-11-10 WB04-E255)
    handler(fallback = null) {
        return (sid, event, count, payload) => {
            if (this.handles(sid)) return this.handle(sid, event, count, payload);
            if (!fallback) throw new TypeError('Regular 부모 Handle 미연결');
            return fallback(sid, event, count, payload);
        };
    }

    // Factory 004549c0: 현재 레벨/프레임/난수 본문(raw-factory-regular.js)에 실제 지정기·진행기·난수·업그레이드 훅을 연결한다. (2026-11-10 WB04-E255)
    #factory(sid, event, count, payload) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        const frames = this.#frames;
        // 레벨/프레임 표 조회는 레벨 범위(0..15)를 먼저 검사한다.
        const checkedLevel = (level, message) => {
            if (level < 0 || level >= 16) throw new RangeError(message);
            return level;
        };
        const current = {
            level: (parent) => (hooks.virtualCount ? hooks.virtualCount(parent) : pool.slot(parent)[STATE_OFFSET] >> 5),
            firstFrame: (parent, level) => frames[pool.slot(parent)[TYPE_OFFSET]].run(0x41 + checkedLevel(level, 'Factory 현재 firstFrame 레벨 범위')).first,
            frameCount: (parent, level) => frames[pool.slot(parent)[TYPE_OFFSET]].run(0x41 + checkedLevel(level, 'Factory 현재 frameCount 레벨 범위')).count,
            // 0 범위는 원문의 나눗셈 오류 전에 시드를 한 번 소비한다.
            random: (limit) => {
                if (!limit) {
                    this.#random.next(1);
                    throw new RangeError('Factory 원문 난수0범위 나눗셈');
                }
                return this.#random.next(limit);
            },
            frame: (parent, value, flags) => this.#setter.set(parent, value, flags),
            advance: (parent, delta, flags) => { this.#advance.advance(parent, delta, flags); },
            begin: hooks.beginFactoryUpgrade,
            finish: hooks.finishFactoryUpgrade,
            flare: hooks.factoryFlare,
            damageable: (parent, code, times, value) => this.damageableHandle(parent, code, times, value),
        };
        return runFactoryRegular(pool, sid, event, count, payload, current);
    }

    // 거주 구역 0049a4e0: 현재 주민/장식/지면 테마 본문(raw-residence-regular.js)에 실제 난수·지정기·신전 목록·설정을 연결한다. (2026-11-10 WB04-E255)
    #residence(sid, event, payload) {
        const hooks = this.#hooks;
        const state = this.#state;
        const pool = this.#pool;
        const elements = this.#elements;
        const current = {
            random: () => this.#random,
            notify: hooks.notifySurface,
            frame: (parent, value, flags) => this.#setter.set(parent, value, flags),
            surface: hooks.surface,
            owner: hooks.setOwner,
            dude: hooks.residenceEvent2,
            element: (temple) => {
                const type = pool.slot(temple)[TYPE_OFFSET];
                if (type >= elements.length) throw new RangeError('거주 구역 현재 신전 원소 범위');
                return elements[type];
            },
            themeMode: () => (hooks.themeMode ? hooks.themeMode() : Number(state.themeMode)),
            decorationType: () => state.decorationType,
            // 최대 조사 DWORD 는 signed 로 현재 값과 비교해 같거나 작으면 새 값으로 쓴다.
            recordScan: (value) => { if ((state.maxDecorationScan | 0) <= value) state.maxDecorationScan = value >>> 0; },
        };
        return runResidenceRegular(pool, this.#hash, this.#types, this.#frames, this.#assets.currentTemples(), sid, event, 0, payload, current);
    }

    // 패치 간헐천: 직접 프레임 DWORD 를 쓰며 앞뒤로 가상 +88 표시 갱신을 부른다. (2026-11-10 WB04-E255)
    #geyser(sid, event, count, payload) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        needEffect(hooks.displayUpdate, '간헐천 표시 갱신 누락');
        hooks.displayUpdate(sid, 0);
        if (event === 0) {
            const raw = pool.slot(sid);
            const ratio = read(raw, WORD_OFFSET, 2) / this.#types[raw[TYPE_OFFSET]].cost;
            const frame = currentFrameOf(pool, sid);
            let next;
            if (ratio > 0.75 && ratio <= 1.0) {
                next = frame < 16 ? add(frame, 1) : 0;
                payload = floatOf(GEYSER_DELAYS[0]);
            } else if (ratio > 0.5 && ratio <= 0.75) {
                next = frame < 32 ? add(frame, 1) : 17;
                payload = floatOf(GEYSER_DELAYS[1]);
            } else {
                next = frame < 48 ? add(frame, 1) : 33;
                payload = floatOf(GEYSER_DELAYS[2]);
            }
            write(pool.allocated(sid), FRAME_OFFSET, next >>> 0);
            hooks.displayUpdate(sid, 0);
        }
        return this.damageableHandle(sid, event, count, payload);
    }

    // 신전: 독립 typed 입력은 같은 멤버(지정기·진행기·난수)를 장치로 넘겨 공통 본문(runVortexRegular)에 위임한다. (2026-11-10 WB04-E255)
    #vortex(sid, event, count, payload) {
        const devices = {
            frame: (parent, frame, flags) => this.#setter.set(parent, frame, flags),
            advance: (parent, delta, flags) => { this.#advance.advance(parent, delta, flags); },
            random: (limit) => this.#random.next(limit),
        };
        return runVortexRegular(this.#pool, this.#frames, this.#state, this.#hooks, devices,
            (parent, code, times, value) => this.damageableHandle(parent, code, times, value), sid, event, count, payload);
    }

    // 사건 판정 뒤 실제 피해 지면/HP/현재 genus 본문을 같은 원문 순서로 공유한다. native 는 장기 owner 의 현재 사건값으로 HP 장치 검사를 지면 피해 뒤로 미룬다.
    // 반환: 체력이 0 이고 TYPE flags2 에 0x200000 이 없으면 -1(삭제 요청), 그 밖은 0. (2026-11-10 WB04-E255)
    damageRegular(sid, count, native = false) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        needEffect(hooks.damageGround, 'Damageable 지면 피해 몸체 누락');
        if (!native) needEffect(hooks.hitPoints, 'Damageable 가상 HP 몸체 누락');
        const raw = pool.slot(sid);
        hooks.damageGround(floatOf(read(raw, X_OFFSET)), floatOf(read(raw, Y_OFFSET)));
        // 상위 16비트는 signed 출처, 하위 16비트는 피해량이다.
        const source = (count | 0) >> 16;
        if (native) needEffect(hooks.hitPoints, 'NativeDamageable의현재HP종단미연결');
        const result = hooks.hitPoints(sid, count & 0xffff, source, 0);
        const type = this.#types[pool.slot(sid)[TYPE_OFFSET]];
        if (!type) throw new RangeError('Damageable 현재 TYPE 범위 오류');
        if (!result && !(type.flags2 & KEEP_ON_ZERO_HP)) return -1;
        return 0;
    }

    // Damageable 의 기본 사건만 payload 반환을 사용한다. 지정된 두 사건(피해·건설 재시도)은 실제 하위 효과를 요구한다.
    // 다른 Damageable 파생이 공통 피해 연쇄/abstract 건설 재시도 분배를 직접 재사용한다. (2026-11-10 WB04-E255)
    damageableHandle(sid, event, count, payload) {
        const hooks = this.#hooks;
        const state = this.#state;
        const pool = this.#pool;
        if (event === state.damageEvent) return this.damageRegular(sid, count);
        if (event === state.constructionEvent) {
            if (pool.slot(sid)[EXTRA_OFFSET] & 1) {
                needEffect(hooks.findAttached, 'Damageable 건설 프로세스 조회 누락');
                if (!hooks.findAttached(sid, state.constructionProcessType)) {
                    needEffect(hooks.startConstruction, 'Damageable abstract 건설 시작 누락');
                    hooks.startConstruction(sid, 0);
                }
            }
            return 0;
        }
        return payload;
    }
}
