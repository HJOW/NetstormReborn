// Client의 장기 게임 전역과 짧은 Session 대여를 구별한다(cpppj client/ClientGameplayGlobals.cpp/.h, 225553df).
// 사용: new ClientGameplayGlobals(subscribers?, authority?, gameAux?).acquire()/acquirePrepared() → use.publish(session), withdraw(session), release().
// DWORD/float 참조 접근자는 같은 {value} 상자다. Player/커서/선택/finder/에너지는 같은 객체를 제자리 초기화하며 카메라/진단/캐시/옵션은 대여로 재생성하지 않는다.
// 현재 Session 최소 계약은 bindRuntimePin(callback)이다. 이 owner의 단독 대여/공개는 전체 Session 구성/Load나 State 자식 정리 증빙을 대신하지 않는다.
// pin은 weak로 공개한 실제 전체 owner를 강하게 잡으며 caller는 try/finally에서 release해야 한다. UI/OS/원본 주소 역참조는 하지 않는다. (2026-11-16 WB04-E570)
import { DwordBox } from './client-subscriber-globals.js';
import { ClientAiDebugGlobals, Int32Box } from './client-ai-debug-globals.js';
import { ClientCameraGlobals } from './client-camera-globals.js';
import { ClientCursorGlobals } from './client-cursor-globals.js';
import { ClientUserSelectionGlobals } from './client-user-selection-globals.js';
import { RawAuthorityState } from './raw-authority-state.js';
import { GameAuxState } from './game-aux-state.js';
import { PlayerWorldState } from './raw-player-world.js';
import { CursorDropState } from './raw-cursor-drop.js';
import { CampaignWorldGlobals } from './raw-campaign-world-load.js';
import { ContainedFinderState } from './raw-contained-finder.js';
import { EnergySupplyState } from './raw-energy-supply.js';
import { TerrainBitmapState } from './raw-terrain-bitmap.js';
import { TerrainStampState } from './raw-terrain-stamp.js';

// 원문의 장기 참조 칸이다. 주소는 읽기 전용 증거 식별자이고 호스트 포인터로 쓰지 않는다.
export const CLIENT_GAMEPLAY_FIELDS = Object.freeze([
    ['manaRadius', 0x52f488, 'f32', 22], ['fullDeck', 0x54db14, 'u32', 0], ['fortKeepGenus', 0x542640, 'u32', 0x40480002],
    ['stateAuxFC4', 0x594fc4, 'u32', 0], ['mapRevision', 0x5949d4, 'u32', 0], ['showGameTime', 0x594fc0, 'u32', 0],
    ['battleClockStamp', 0x594fb0, 'u32', 0], ['battleClockOrigin', 0x594fb4, 'u32', 0], ['drawWater', 0x54d930, 'u32', 0],
    ['hideBuildings', 0x59a8ac, 'u32', 0], ['denySalvage', 0x595078, 'i32', 0], ['denyAscend', 0x59507c, 'i32', 0],
    ['patchDiagnostic', 0x5318ec, 'u32', 6], ['patchSample', 0x540cb8, 'u32', 6], ['worldCells', 0x531928, 'i32', 256],
    ['gameServerPort', 0x5318e8, 'u32', 6799], ['localAddress', 0x5953fc, 'u32', 0], ['drawHash', 0x54d91c, 'u32', 0],
    ['drawFire', 0x54d920, 'u32', 0], ['drawSpot', 0x54d924, 'u32', 0], ['drawBoard', 0x54d92c, 'u32', 0], ['fireMode', 0x595144, 'u32', 0],
].map(([name, address, kind, initial]) => Object.freeze({ name, address, kind, initial })));
// 같은 현재 GameAux 칸을 직접 빌리는 네 별칭이다. 독립 DWORD 사본을 만들지 않는다.
const AUX_ALIASES = Object.freeze(['geyserType', 'firstTimerFactoryType', 'secondTimerFactoryType', 'outpostType']);
// 안전한 원문 주소식별자 중 다섯 진단 칸과 현재 특별 모드 칸이다.
const DEBUG_FIELDS = new Map(CLIENT_GAMEPLAY_FIELDS.filter((field) => ['drawHash', 'drawFire', 'drawSpot', 'drawBoard', 'fireMode'].includes(field.name)).map((field) => [field.address, field.name]));
// C++64비트 size_t 호출 수의 실제 상한이다. JS 안전 정수로 줄이지 않는다.
const MAX_CALLS = 0xffffffffffffffffn;
// 대여 생성자/State 전용 브리지는 모듈이 발급한 실제 대여만 소비한다.
const TOKEN = Symbol('ClientGameplayGlobals.Use');
const STATE_WITHDRAW = new WeakMap();
// 표에서 만든 참조 접근자만 내부 고정 상자에 닿는다.
let fieldReference;

// 단정도 값 한 칸의 주소 고정 상자다.
class FloatBox {
    // float32 저장 지점이다.
    #value;
    // 초기값을 같은 폭으로 저장한다.
    constructor(value) { this.#value = Float32Array.of(value); }
    // 현재 float를 읽는다.
    get value() { return this.#value[0]; }
    // 모든 명시 쓰기는 단정도로 저장한다.
    set value(value) { this.#value[0] = value; }
}
// 공유 owner의 실제 속성을 현재 시점에 읽고 쓰는 참조 상자다.
function propertyReference(owner, name, normalize = (value) => value) {
    return { get value() { return owner[name]; }, set value(value) { owner[name] = normalize(value); } };
}
// C++ aggregate 대입을 기존 객체의 같은 필드에 적용한다. raw 슬롯이나 새 게임 owner를 만들지 않는다.
function assignState(target, fresh) { for (const name of Object.keys(fresh)) target[name] = fresh[name]; }
// 별도 기록기0059a7dc/e8는 현재 커서 초기화로 지우지 않는다.
function resetCursor(cursor) { const recording = cursor.recording; const recorder = cursor.recorder; assignState(cursor, new CursorDropState()); cursor.lockDepth = 1; cursor.recording = recording; cursor.recorder = recorder; }

// 단일 Session의 짧은 대여다. 새 대여는 owner만 발급하며 해제 후 낡은 대여의 공개/반납을 거부한다.
class GameplayUse {
    // 소유자의 원문 공개/반납/해제 장치와 현재 반납 여부다.
    #devices; #released = false;
    // owner가 이미 대여를 선점한 뒤 발급한다.
    constructor(token, devices) { if (token !== TOKEN) throw new TypeError('게임 대여는 실제 소유자만 발급합니다'); this.#devices = devices; STATE_WITHDRAW.set(this, (session) => { this.#live(); devices.withdrawForState(session); }); }
    // 반납한 대여는 다음 세대의 전역에 접근할 수 없다.
    #live() { if (this.#released) throw new Error('이미 반납한 게임 전역 대여'); }
    // 완성한 현재 Session을 한 번 공개한다.
    publish(session) { this.#live(); this.#devices.publish(session); }
    // 하위 소멸 전에 현재 공개 대상만 닫는다.
    withdraw(session) { this.#live(); this.#devices.withdraw(session); }
    // C++ 소멸자에 대응하는 명시 반납이다. 중복 반납은 다른 세대를 건드리지 않는다.
    release() { if (this.#released) return; this.#devices.release(); this.#released = true; STATE_WITHDRAW.delete(this); }
}
// C++ RawCampaignSession friend 경계다. Session 소멸자가 실제 자식 정리 증빙을 먼저 확인한 뒤에만 호출한다.
// 이 함수 자체는 증빙을 만들거나 자식을 정리하지 않는다. 전체 Session 증빙 소비는 D/E 통합에서 연결한다.
export function withdrawGameplayForState(use, session) { const withdraw = STATE_WITHDRAW.get(use); if (!withdraw) throw new TypeError('State 반납의 같은 실제 게임 대여 필요'); withdraw(session); }

// 실제 장기 데이터를 소유하고 짧은 실행 대상과 배송 중 전체 owner만 별도로 관리한다.
export class ClientGameplayGlobals {
    // 생성 시 빌리거나 직접 소유하는 권한/게임 보조/Subscriber다.
    #authority; #aux; #subscribers; #server; #boss;
    // 표의 현재 참조 상자와 보조 전역의 같은 별칭이다.
    #fields = new Map(); #aliases = new Map();
    // Client 최초 상태이며 대여 시 객체 주소를 바꾸지 않는다.
    #player = new PlayerWorldState(); #cursor = Object.assign(new CursorDropState(), { lockDepth: 1 }); #selection = new ClientUserSelectionGlobals();
    #finder = new ContainedFinderState(); #world; #energy = new EnergySupplyState();
    // 대여나 Session 교체로 다시 만들지 않는 실제 하위 소유자/배열이다.
    #camera = new ClientCameraGlobals(); #cursorGlobals = new ClientCursorGlobals(); #aiDebug = new ClientAiDebugGlobals();
    #bitmap = new TerrainBitmapState(); #stamp = new TerrainStampState(); #sidDiagnostics = { lastRobust: 0, type: 0 };
    #deadlines = new Float64Array(9); #battleOptions = new Uint8Array(87); #manaRanges = Float32Array.of(14, 22, 30, 38);
    // 특별 모드의 별도 보조 DWORD다. 나머지 특별 값은 같은 Player 필드다.
    #specialAuxiliary = new DwordBox(0);
    // 대여/현재 Session과 weak 전체 owner, 현재 배송 수는 게임 전역과 다른 호스트 수명 장부다.
    #lease = null; #current = null; #runtimeOwner = null; #runtimeOwnerSession = null; #calls = 0n;

    // 각 참조 접근자는 같은 상자만 반환한다.
    static { fieldReference = (owner, name) => owner.#fields.get(name) ?? owner.#aliases.get(name); }
    // Subscriber가 없으면 기존 독립 입력0/0을 유지하고 공급된 두 owner에는 초기화 쓰기를 하지 않는다.
    constructor(subscribers = null, authority = null, gameAux = null) {
        this.#authority = authority ?? new RawAuthorityState(); this.#aux = gameAux ?? new GameAuxState(); this.#subscribers = subscribers;
        this.#server = propertyReference(this.#authority, 'server', Boolean); this.#boss = propertyReference(this.#authority, 'boss', Boolean);
        this.#world = CampaignWorldGlobals.borrow({ my: subscribers ? subscribers.mySubscriber() : null, login: subscribers ? subscribers.loginSubscriber() : null });
        // 원문에서 독립 소유하는 참조 칸만 한 번 확보한다.
        for (const field of CLIENT_GAMEPLAY_FIELDS) this.#fields.set(field.name, field.kind === 'f32' ? new FloatBox(field.initial) : field.kind === 'i32' ? new Int32Box(field.initial) : new DwordBox(field.initial));
        // 보조 전역 별칭은 매번 같은 실제 aux를 읽는다.
        for (const name of AUX_ALIASES) this.#aliases.set(name, propertyReference(this.#aux, name));
        this.#bindFinder(); this.#cursorGlobals.lifecycle().checking = this.#aiDebug.checking();
    }
    // fresh 뒤에도 같은 현재 boss/checking을 빌린다.
    #bindFinder() { this.#finder.bindBoss(this.#boss); this.#finder.bindChecking(() => this.#aiDebug.checking().value); }
    // 현재 Runtime 공개만 닫으며 게임 전역은 쓰지 않는다.
    #clearRuntime() { this.#runtimeOwner = null; this.#runtimeOwnerSession = null; this.#current = null; }
    // 같은 현재 대여인지 검사해 낡은 대여가 다음 Session을 바꾸지 못하게 한다.
    #leaseCurrent(lease) { if (this.#lease !== lease) throw new Error('현재 게임 전역 대여 불일치'); }
    // 원문 일반 fresh 입력만 실행한다. 명시 Native 정리/prefix로 세지 않는다.
    #fresh() {
        const special = this.#player.special; const cheat = this.#energy.userCheat;
        assignState(this.#player, new PlayerWorldState()); this.#player.special = special; resetCursor(this.#cursor); this.#cursorGlobals.resetForGeneralWorld();
        assignState(this.#finder, new ContainedFinderState()); this.#bindFinder(); this.#world.editor = 0; this.#world.missionPost = true;
        this.#authority.server = true; this.#authority.boss = true; this.fullDeck().value = 0;
        assignState(this.#energy, new EnergySupplyState()); this.#energy.radiusSquared = 484; this.#energy.userCheat = cheat; this.manaRadius().value = 22; this.#selection.resetForGeneralWorld();
    }
    // 단일 대여를 선점한 뒤 선택한 초기화만 실행한다.
    #acquire(prepared) {
        if (this.#lease) throw new Error('현재 게임 전역 중복 대여');
        const policy = { prepared }; let lease;
        lease = new GameplayUse(TOKEN, {
            publish: (session) => { this.#leaseCurrent(lease); if (this.#current) throw new Error('현재 게임 Runtime 중복 연결'); if (!session || typeof session !== 'object') throw new TypeError('실제 Session 참조 필요'); this.#current = session; },
            withdraw: (session) => { this.#leaseCurrent(lease); if (!session || this.#current !== session) throw new Error('현재 게임 Runtime 반납 불일치'); if (this.runtimeCallActive(session)) throw new Error('실제 Runtime 배송 종료 전 공개 반납'); this.#clearRuntime(); },
            withdrawForState: (session) => { this.#leaseCurrent(lease); if (!session || !this.#lease || this.#current !== session) throw new Error('State 반납의 같은 현재 게임 Runtime 필요'); if (this.runtimeCallActive(session)) throw new Error('실제 Runtime 배송 종료 전 State 반납'); this.#clearRuntime(); policy.prepared = true; },
            release: () => { this.#leaseCurrent(lease); this.#clearRuntime(); if (!policy.prepared) { this.#player.playerKnown = 0; resetCursor(this.#cursor); this.#cursorGlobals.resetForGeneralWorld(); this.#world.editor = 0; } this.#lease = null; },
        });
        this.#lease = lease; if (!prepared) this.#fresh(); return lease;
    }
    // 기존 독립 Session의 일반 초기화 정책이다.
    acquire() { return this.#acquire(false); }
    // 준비된 실제 상태를 쓰지 않고 대여한다. 준비 완료 판정은 호출자가 소유한다.
    acquirePrepared() { return this.#acquire(true); }
    // 미연결 효과를 가짜 Session으로 대체하지 않는다.
    requireSession() { if (!this.#current) throw new Error('선택한 생산 효과의 현재 게임 Runtime 미연결'); return this.#current; }
    // 현재 실제 전체 owner만 weak로 공개하고 Session의 같은 후속 pin 입구에 연결한다.
    bindRuntimeOwner(session, owner) {
        if (!session || !owner || (typeof owner !== 'object' && typeof owner !== 'function') || this.#current !== session) throw new TypeError('현재 Session과 실제 Runtime owner 공개 불일치');
        if (this.#calls) throw new Error('실제 Runtime 배송 중 owner 재공개');
        this.#runtimeOwner = new WeakRef(owner); this.#runtimeOwnerSession = session;
        if (typeof session.bindRuntimePin !== 'function') throw new Error('현재 Session의 실제 Runtime pin 입구 미연결');
        session.bindRuntimePin(() => { const current = this.requireSession(); if (current !== session) throw new Error('전역대여pin의현재Session교체'); return this.pinCurrentRuntime(current); });
    }
    // 실제 strong pin을 반환한다. 소멸 순서에 대응해 호출 수를 먼저 반납하고 owner 참조를 놓는다.
    pinCurrentRuntime(session) {
        if (!session || this.#current !== session || this.#runtimeOwnerSession !== session) throw new Error('선택한 현재 Runtime의 실제 owner 미공개');
        let owner = this.#runtimeOwner.deref(); if (!owner) throw new Error('선택한 현재 Runtime의 실제 owner 수명 종료');
        if (this.#calls === MAX_CALLS) throw new RangeError('현재 Runtime 배송 호출 수 넘침'); this.#calls += 1n;
        return { get owner() { return owner; }, release: () => { if (owner) { this.#calls -= 1n; owner = null; } } };
    }
    // weak 공개만으로는 호출 중으로 세지 않고 같은 현재 전체 owner의 pin만 센다.
    runtimeCallActive(session) { return Boolean(session) && this.#runtimeOwnerSession === session && this.#calls !== 0n; }
    // 게임 상태를 쓰지 않는 대여 관측이다.
    borrowed() { return this.#lease !== null; }
    // 두 Subscriber DWORD/handle의 실제 장기 owner다.
    subscribers() { return this.#subscribers; }
    // 같은 두 권한 bool을 소유한 객체다.
    authority() { return this.#authority; }
    // 실제 현재 서버 관문의 참조다.
    server() { return this.#server; }
    // 서버와 독립인 실제 권한 참조다.
    boss() { return this.#boss; }
    // TYPE/자원/헤더의 같은 실제 게임 보조 owner다.
    gameAux() { return this.#aux; }
    // 현재 전역 snapshot과 Renderer 목표를 소유하는 카메라다.
    camera() { return this.#camera; }
    // 실행 콜백과 구별한 장기 커서 데이터다.
    cursorGlobals() { return this.#cursorGlobals; }
    // 같은 checking/로그/상세 진단 owner다.
    aiDebug() { return this.#aiDebug; }
    // 원문 마지막 SID 진단 두 DWORD다.
    sidRobustDiagnostics() { return this.#sidDiagnostics; }
    // 최초 분류 캐시와 fringe TYPE의 같은 상태다.
    terrainBitmaps() { return this.#bitmap; }
    // 최초FrameAt에서 채우는99 변형의 별도 상태다.
    terrainStamp() { return this.#stamp; }
    // 현재87BYTE 옵션 물리다. acquire/release로 reset하지 않는다.
    battleOptionsOwner() { return this.#battleOptions; }
    // 현재4float 반경 선택표다.
    manaRanges() { return this.#manaRanges; }
    // 벽시각9기한의 같은 배열이다.
    priestTakenDeadlines() { return this.#deadlines; }
    // 같은 현재 Player 전역을 빌린다.
    get player() { return this.#player; }
    // 보정 마우스/현재 조각/잠금/기록기의 같은 상태다.
    get cursor() { return this.#cursor; }
    // 선택/강조/다중 목록의 같은 상태다.
    get selection() { return this.#selection; }
    // 실제 현재 내용물 검색 전역이다.
    get finder() { return this.#finder; }
    // 미션/편집/두 Subscriber의 같은 전역이다.
    get world() { return this.#world; }
    // 게임 판정이 빌리는 현재 에너지 상태다.
    get energy() { return this.#energy; }
    // 특별 모드의 별도 보조 DWORD를 읽는다.
    get specialAuxiliary() { return this.#specialAuxiliary.value; }
    // 전체 비트를 같은 DWORD에 쓴다.
    set specialAuxiliary(value) { this.#specialAuxiliary.value = value; }
    // 허용된 원문 식별자에 해당하는 현재 DWORD만 읽는다.
    readDebugWord(address) { if (DEBUG_FIELDS.has(address)) return this[DEBUG_FIELDS.get(address)]().value; if (address === 0x55a480) return this.#player.special >>> 0; if (address === 0x55a484) return this.specialAuxiliary; if (address === 0x5e4794) return this.#aiDebug.checking().value >>> 0; return this.#aiDebug.read(address); }
    // 같은 현재 진단 DWORD만 쓰며 checking은 signed 비트를 보존한다.
    writeDebugWord(address, value) { if (DEBUG_FIELDS.has(address)) { this[DEBUG_FIELDS.get(address)]().value = value; return; } if (address === 0x55a480) { this.#player.special = value >>> 0; return; } if (address === 0x55a484) { this.specialAuxiliary = value; return; } if (address === 0x5e4794) { this.#aiDebug.checking().value = value | 0; return; } this.#aiDebug.word(address).value = value; }
}
// C++ 참조 getter와 같은 이름으로 고정 상자를 빌려 준다. 현재값 사본을 반환하지 않는다.
for (const name of [...CLIENT_GAMEPLAY_FIELDS.map((field) => field.name), ...AUX_ALIASES]) Object.defineProperty(ClientGameplayGlobals.prototype, name, {
    // 호출할 때마다 같은 실제 물리 참조를 돌려준다.
    value() { return fieldReference(this, name); }, configurable: false,
});
