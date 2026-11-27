// Session InstallContents의 실제 통지/공통 삭제·수명/생성3020..3134을 같은 owner에 연결한다(225553df, WB04-E596~E597).
// 사용: connectWorldRuntimeHooks 뒤 bindNotices → bindLifetime. 생산과 최종 World 공개는 후속 구간이다.
import { GumpFormHost } from './gump-form-host.js';
import { RawPlayerContentNotice } from './raw-player-content-notice.js';
import { RawContentParentNotice } from './content-parent-notice.js';
import { RawPriestTakenNotice } from './raw-priest-taken-notice.js';
import { makeCampaignDeletionEffects, RawCampaignLifetimeBindings } from './raw-campaign-lifetime-bindings.js';
import { ContentFormHost } from './content-form-host.js';
import { RawCarrierContentNotice } from './raw-carrier-content-notice.js';
import { RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetPrevious } from './squid-contained-unpop.js';
import { RawContainedCreate } from './contained-create.js';
import { startSelectionContentPreview } from './raw-squid-selection.js';
import { SoundProcess } from './sound-process.js';
import { SQUID_FORM_VTABLES } from './squid-vtables.js';
import { typeString } from '../assets/type-table.js';

// 원문 FillMissing의 선택된 호출에서만 실패하는 실제 미연결 장치다.
function missing(line) { return () => { throw new Error(`실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:${line}`); }; }
// SP의 DWORD 비트를 원문 bit_cast<float>처럼 해석한다. 수치 정수 변환을 하지 않는다.
function spFloat(bits) { const bytes = new DataView(new ArrayBuffer(4)); bytes.setUint32(0, bits, true); return bytes.getFloat32(0, true); }
// 선택 지점의 현재 서비스만 요구한다. 생성 때 미선택 장치를 실행하지 않는다.
function need(fn, message) { if (typeof fn !== 'function') throw new Error(message); return fn; }

// 회수된 세션의 접근을 명시 거부하며 현재 호출 task 동안만 실제 owner를 빌린다.
function contentBorrow(reference) { const value = reference.deref(); if (!value) throw new Error('회수된 내용물 통지의 세션 전역'); return value; }
// JIT의 accessor/객체 형상 보관이 이전 세션을 붙잡지 않도록 약한 주소만 캡처한다.
// SP·진단·남은 횟수의 원래 주소는 유지하고 Player/TYPE/사건은 호출 때 owner에서 읽는다.
function playerContentState(reference) {
    const store = new WeakRef(contentBorrow(reference).sp);
    const patch = new WeakRef(contentBorrow(reference).priestState.regen.patch);
    const globals = new WeakRef(contentBorrow(reference).gameplayGlobals);
    // 원래 바인딩과 같이 설치 전에 추적 전역이 연결됐는지 확인한다.
    void contentBorrow(store).trackingOffset;
    return {
        cursorA: () => contentBorrow(store).offset >>> 0,
        cursorB: () => contentBorrow(store).trackingOffset >>> 0,
        firstSpPool: () => contentBorrow(store).firstPoolReady ? 1 : 0,
        firstPermutation: () => contentBorrow(store).permutation()[0],
        get problemCode() { return contentBorrow(globals).patchDiagnostic().value; },
        set problemCode(value) { contentBorrow(globals).patchDiagnostic().value = value; },
        fallbackCode: () => contentBorrow(patch).readSample(),
        get moneyChecks() { return contentBorrow(patch).remaining; },
        set moneyChecks(value) { contentBorrow(patch).remaining = value; },
        battleShown: () => contentBorrow(reference).playerState.playerKnown,
        localPlayer: () => contentBorrow(reference).playerState.battleOwner,
        gumpFormType: () => contentBorrow(reference).contentGumpType,
        contentEvent: () => contentBorrow(reference).contentEvent,
    };
}

// 같은 Impl의 Gump/Player/내용물/포획 통지 부모를 소유하는 초기화 단계다.
export class RawCampaignSessionContents {
    // 부모 참조와 부분 실패/반복/반납 관문이다. 수명/생산 준비는 별도 완료값으로 다룬다.
    #owner; #started = false; #noticesReady = false; #disposed = false;
    // 통지 뒤 수명 초기화의 반복/부분 실패와 실제 정상 복귀를 구별한다.
    #lifetimeStarted = false; #lifetimeReady = false;
    // 실제 포획 통지 객체가 빌리는 값형 훅이다. 서비스는 호출 때 조회한다.
    takenHooks = null;
    // 실제 하위 객체가 빌리는 원문 값형 효과 표와 현재 상태·부모 분배 함수다.
    lifetimeEffects = null; carrierEffects = null; carrierNoticeState = null; parentNotice = null; unpopHooks = null;

    // 생성은 World/Player 상태를 바꾸지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 원문 순서대로 Gump 등록→삭제→Player/부모 통지→현재 Form 상태→포획 통지를 설치한다.
    bindNotices() {
        const o = this.#owner; const c = o.core; const g = o.gameplayGlobals;
        if (this.#disposed || !o.runtimeModules?.ready || this.#started) throw new Error('Session 내용물 통지의 초기화 전/반복 결선');
        this.#started = true;
        o.gumpForms = new GumpFormHost(c.pool, c.factory, c.types, c.destroy, {
            notice: (owner, parent) => { this.#live(); need(o.services.gumpNotice, '현재Session GumpForm UI통지미연결')(owner, parent); },
            fallback: c.forms.hooks(),
        });
        c.forms.bind(o.gumpForms);
        if (!o.services.unitLost) o.services.unitLost = missing(3025);
        o.world.bindDeletion(o.hp, o.worldHooks.assets.selected, makeCampaignDeletionEffects(o.worldHooks.assets, o.services.unitLost));
        o.world.deletionState.silentType = 147;
        // 원문 Native(RawSurfaceStorage::RestoreDeletionGlobals)처럼 unit-lost 소리/좌표 기록의 두 억제 전역을 현재 값에 연결한다: 적재 깊이(005c89b8)가 0 이 아니거나 fort 모드이면 억제한다.
        // 이전에는 두 값이 항상 false 라 Fort/템플릿 적재 중 삭제되는 로컬 유닛도 손실 소리와 좌표를 남겼다. 쓰기는 독립 입력용으로 값을 덮는다. (2026-11-22 WB04-E1216)
        const suppressed = (key, read) => { let override = null; Object.defineProperty(o.world.deletionState, key, { configurable: true, enumerable: true, get: () => (override === null ? read() : override), set: (value) => { override = value; } }); };
        suppressed('unitLostSuppressed1', () => (o.objectState.loadingDepth >>> 0) !== 0); suppressed('unitLostSuppressed2', () => Boolean(c.ownerMode.fort));
        const hooks = { ...o.services.playerContent, money: (owner) => { this.#live(); return spFloat(o.sp.get(owner)); } };
        if (!hooks.gump) hooks.gump = missing(3030); if (!hooks.handle) hooks.handle = missing(3030);
        o.playerContent = new RawPlayerContentNotice(c.pool, playerContentState(new WeakRef(o)), hooks);
        o.contentNotice = new RawContentParentNotice(c.pool, (parent, form, reason) => {
            this.#live(); if (!o.lifetime) throw new Error('현재 내용물 수명 부모 미설치'); o.lifetime.parentNotice(parent, form, reason);
        });
        o.contentFormState = {
            contentType: () => g.finder.containedType, inBattle: () => c.ownerMode.battle, checkingLinks: () => g.aiDebug().checking().value,
            assertion: (expression, file, line) => { this.#live(); need(o.services.ai.assertion, `ContentForm 실제 진단 장치 미연결 ${line}`)(expression); },
        };
        this.takenHooks = {
            refresh: (owner) => { this.#live(); o.priestParticipation.refresh(owner); },
            wallSeconds: () => { this.#live(); return need(o.services.sound.wallSeconds, '포획 안내 실제 벽시각 미연결')(); },
            name: (owner) => {
                this.#live(); const fn = need(o.services.priestTakenName, '포획 안내 실제 Player 이름 미연결'); const end = o.handle.indexOf(0);
                if (end < 0) throw new Error('포획 안내 현재 이름 NUL 누락');
                return fn(owner, Array.from(o.handle.subarray(0, end), (byte) => String.fromCharCode(byte)).join(''));
            },
            sound: (name) => { this.#live(); const play = need(o.services.sound.play, '포획 안내 실제 전역 음향 미연결'); play(o.soundNames.lookup(name), 0, 0, 0, 1, 0); },
            tell: (section) => { this.#live(); need(o.services.factoryTell, '포획 안내 실제 Tell 미연결')(section); },
        };
        o.priestTakenNotice = new RawPriestTakenNotice(c.pool, () => o.playerState.battleOwner, g.priestTakenDeadlines(), this.takenHooks);
        this.#noticesReady = true;
    }

    // 같은 Form6/Carrier/Player 수명을 등록하고 World의 종속 해제·현재 가상 부착과 생성기를 연결한다.
    bindLifetime() {
        const o = this.#owner; const c = o.core; const g = o.gameplayGlobals; const s = o.state;
        if (this.#disposed || !this.#noticesReady || this.#lifetimeStarted) throw new Error('Session 내용물 수명의 초기화 전/반복 결선');
        this.#lifetimeStarted = true;
        o.contentForms = new ContentFormHost(c.pool, c.types, (parent, form, reason) => { this.#live(); o.contentNotice.notice(parent, form, reason); }, {
            transmit: o.services.dependencyTransmit, state: o.contentFormState,
            captured: (parent, previous) => { this.#live(); o.priestTakenNotice.notify(parent, previous); },
        });
        o.contentForms.bindCurrentTypes(c.types);
        const effects = { ...o.services.contentLifetime }; this.lifetimeEffects = effects;
        effects.cancelCursor = () => { this.#live(); if (!o.cursor) throw new Error('현재 ContentForm 커서 수명 미설치'); o.cursor.cancel(); };
        s.contentLifetimeState.currentCursor = () => o.cursorDrop.preview;
        if (!effects.localNotice) effects.localNotice = missing(3060);
        if (!effects.remoteNotice) effects.remoteNotice = (owner) => {
            this.#live(); if (owner >= o.services.battlePeers.length) throw new RangeError('너겟 수입 송신 Player 범위');
            if ((owner && !(o.players[owner * 180 + 0x50] & 1)) || !o.services.battlePeers[owner]) return;
            throw new Error('너겟 수입의 연결된 원격5a 패킷 송신 미연결');
        };
        // 세 누락은 실제 선택 위치까지 지연하며 같은 SquidReward가 지갑/SP를 맡는다.
        for (const key of ['wallet', 'addSp', 'sound']) if (!effects[key]) effects[key] = missing(3066);
        s.contentLifetimeState.localOwner = o.playerState.battleOwner & 0xff; s.contentLifetimeState.notice = 0x020000b4;
        const carrier = { ...o.services.carrierContent }; this.carrierEffects = carrier;
        carrier.center = (parent) => { this.#live(); return o.gunTarget.center(parent); };
        carrier.checkCarrier = (parent) => { this.#live(); startSelectionContentPreview(c.pool, parent, o.objectState.loadingDepth, (sid, event, payload) => { c.processes.addRegular(sid, event, payload); }, missing(3072)); };
        const otherWord = carrier.typeWord;
        carrier.typeWord = (type, offset) => {
            this.#live(); if (offset !== 0x7c && offset !== 0x6c) return need(otherWord, 'Carrier의다른현재TYPE DWORD 미연결')(type, offset);
            const types = o.assets.game.types(); if (type < 70 || type - 70 >= types.length) throw new RangeError('포획소리현재TYPE 범위오류');
            const name = typeString(types[type - 70].definition, offset === 0x6c ? 'activeSound' : 'pickupSound'); return name ? o.soundNames.lookup(name) : 0;
        };
        if (!carrier.sound) carrier.sound = (sound) => { this.#live(); need(o.services.sound.play, '포획실제전역음향미연결')(sound, 0, 0, 0, 1, 0); };
        if (!carrier.bolt) carrier.bolt = (parent, first, last, life) => { this.#live(); if (!o.particles) throw new Error('획득번개 실제표시시스템 미설치'); o.particles.addScreenBolt(parent, first, last, life); };
        if (!carrier.soundAt) carrier.soundAt = (x, y, name) => { this.#live(); need(o.services.sound.playOnceAt, '획득번개 실제위치음향 미연결')(x, y, o.soundNames.lookup(name), 0, 0); };
        if (!carrier.allocate) carrier.allocate = (amount) => {
            this.#live(); if (amount !== 40 || o.pendingPickupSound) throw new Error('획득Sound47 예약크기/중첩 오류');
            // JS는 미부착 실제 객체를 확보한다. 생성 인자는 create에서 같은 객체에 채우며 별도 토큰용 메모리로 대신하지 않는다.
            o.pendingPickupSound = new SoundProcess(o.sounds, 0, 0, 0); return 1;
        };
        if (!carrier.create) carrier.create = (reservation, parent, sound, alternate, flags) => {
            this.#live(); if (reservation !== 1 || !o.pendingPickupSound) throw new Error('획득Sound47 실제예약 누락');
            const process = o.pendingPickupSound; o.pendingPickupSound = null; process.initializeUnattached(sound, alternate, flags);
            const attached = c.processes.attach(process, s.soundState.currentType(), parent, 0x10); return attached ? attached.form >>> 0 : 0;
        };
        if (!carrier.duration) carrier.duration = (token, delay) => {
            this.#live(); if (token > 65535) throw new RangeError('획득Sound47 Form번호범위'); const process = c.processes.fromForm(token);
            if (!(process instanceof SoundProcess)) throw new Error('획득Sound47 현재객체 누락'); process.delay(delay);
        };
        carrier.typeAt = (x, y, type) => { this.#live(); return o.world.find(x, y, type); };
        carrier.genusAt = (x, y, genus) => { this.#live(); if (!o.damageablePost) throw new Error('내용물 실제 DamageablePost 미설치'); return o.damageablePost.genusAt(x, y, genus); };
        if (!carrier.freeSound) carrier.freeSound = (name) => { this.#live(); need(o.services.sound.play, '사제 해방 실제 전역 음향 미연결')(o.soundNames.lookup(name), 0, 0, 0, 1, 0); };
        this.carrierNoticeState = {
            get savedGame() { return o.contentSavedGame; }, get loadingDepth() { return o.objectState.loadingDepth; }, get bombBlocked() { return o.contentBombBlocked; },
            get localOwner() { return o.playerState.battleOwner; }, get priestType() { return o.priestState.movement.priest; },
            get cameraX() { return g.camera().snapshot().pixels.x; }, get cameraY() { return g.camera().snapshot().pixels.y; }, get freeSoundBlockerType() { return o.priestState.movement.rainWalker; },
        };
        o.carrierContentNotice = new RawCarrierContentNotice(c.pool, c.types, this.carrierNoticeState, carrier);
        const otherParent = o.services.contentNotice;
        this.parentNotice = (parent, form, reason) => {
            this.#live(); if (o.daisNotice?.handles(parent)) { o.daisNotice.notice(parent, form, reason); return; }
            if (o.factoryResearch?.handlesNotice(parent)) { o.factoryResearch.notice(parent, form, reason); return; }
            if (o.carrierContentNotice.handles(parent)) { o.carrierContentNotice.notice(parent, form, reason); return; }
            need(otherParent, '다른 캠페인 내용물 실제 부모 통지 미연결')(parent, form, reason);
        };
        o.lifetime = new RawCampaignLifetimeBindings(o.world, o.contentForms, s.contentLifetimeState, effects, o.hp, o.playerContent, this.parentNotice);
        const queue = o.services.assets.queueCommand; const assertion = o.services.ai.assertion;
        this.unpopHooks = {
            parent: (sid) => commonContainedParent(c.pool, sid), previous: (sid) => commonContainedPrevious(c.pool, sid), setPrevious: (sid, previous) => { commonContainedSetPrevious(c.pool, sid, previous); },
            parentNotice: (parent, form, reason) => { this.#live(); o.contentNotice.notice(parent, form, reason); },
            command: (command, first, second) => { this.#live(); need(queue, '종속 Unpop의 실제 UpdateContents 입력 큐 미연결')(command, first | 0, second | 0); },
            assertion: (expression, file, line) => { this.#live(); need(assertion, `종속 Unpop의 실제 진단 장치 미연결 ${line}`)(expression); },
        };
        o.containedUnpop = new RawSquidContainedUnpop(c.pool, {
            contentFormType: () => o.contentFinder.containedType, gumpFormType: () => o.contentGumpType, uiEvent: () => o.contentUnpopEvent,
            checking: () => g.aiDebug().checking().value, inMetamapMode: () => c.ownerMode.challenge, boss: () => g.authority().boss,
        }, this.unpopHooks);
        o.world.bindContainedUnpop(o.containedUnpop);
        o.contentForms.bindPop((sid, flags) => c.factory.popForm(sid, flags)); o.contentForms.bindUnpop((sid) => c.factory.unpopForm(sid));
        o.contentForms.bindReattach((sid, parent, flags) => {
            this.#live(); const raw = c.pool.slot(sid); const table = (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
            if (table === SQUID_FORM_VTABLES.content) { o.contentForms.attach(sid, parent, flags); return; }
            // 기존 세 클래스의 현재 VT만 공통 부착으로 보낸다. TYPE BYTE로 확장하지 않는다.
            for (const kind of ['depend', 'process', 'gump']) if (table === SQUID_FORM_VTABLES[kind]) { c.factory.attachForm(sid, parent, flags); return; }
            throw new Error('Content Regular의 현재 가상+50 미연결');
        });
        o.contained = new RawContainedCreate(c.pool, { containedType: () => o.contentFinder.containedType, bridgeType: () => o.world.storage.links.bridgeType }, o.contentForms.bindCreate(c.factory, { notifySurface: (sid) => c.building.notifySurface(sid) }));
        this.#lifetimeReady = true;
    }

    // 초기화 중 생성 효과도 허용하되 반납 뒤 자기 closure는 거부한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session 내용물 통지'); }
    // World 정리 뒤 부모가 한 번 무효화한다. 등록된 Form을 다른 풀로 옮기지 않는다.
    dispose() { this.#disposed = true; this.#owner.pendingPickupSound = null; }
    // InstallContents 전체가 아니라 통지 접두의 정상 복귀 여부다.
    get noticesReady() { return this.#noticesReady; }
    // 내용물 수명/생성 구간만 완료했음을 뜻하며 생산/전체 InstallContents는 포함하지 않는다.
    get lifetimeReady() { return this.#lifetimeReady; }
}
