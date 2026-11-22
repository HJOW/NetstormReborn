// Session 생성자1236..1267의 실제 Player·Template·Regular·소유권을 같은 기반 owner에 연결한다(225553df, WB04-E578).
// 사용: 섬/사제 구간 뒤 owner.connectPlayersAndOwnership(). 전체 생성자/Load/Runtime 공개는 아직 별도다.
import { RawCampaignOwnership } from './raw-campaign-ownership.js';
import { RawPlayerRegular } from './raw-player-regular.js';
import { RawPlayerTemplateState } from './raw-player-template-state.js';
import { PLAYER_WORLD_ASSERTION, RawPlayerWorld } from './raw-player-world.js';
import { reportVortexOwnerMissingIsland } from './raw-vortex-owner-diagnostic.js';

// 같은 현재 속성 슬롯을 상자로 빌린다. 값 스냅샷이나 별도 동기화 칸을 만들지 않는다.
function reference(owner, key) { return { get value() { return owner[key]; }, set value(value) { owner[key] = value; } }; }
// 원문 SessionMissing/FillMissing의 선택 지점과 최초 연결 위치를 명시한다.
function missing(line) { return () => { throw new Error(`실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:${line}`); }; }
// 원문 SessionCheck의 실제 예약/자료 성공 조건이다.
function check(value) { if (!value) throw new Error('실제 캠페인 세션 예약/자료 조건 오류'); }

// 같은 Impl의 Player 기록/Template 저장소와 섬 소유권을 조립하는 생성자 구간이다.
export class RawCampaignSessionPlayers {
    // 실제 기반 owner와 한 번 실행/정상 복귀/반납 상태다. 실패 접두는 되돌리거나 재실행하지 않는다.
    #owner; #started = false; #ready = false; #disposed = false;

    // 실제 자료만 빌리며 하위 모듈은 원문 위치의 bind에서 생성한다.
    constructor(owner) { this.#owner = owner; }

    // 원문1236..1267 typed 캠페인 순서다. 실제 Player 생성/전체 월드 적재는 나중의 호출자 몫이다.
    bind() {
        const o = this.#owner; const c = o.core; const services = o.services;
        if (this.#disposed || !o.priestModules?.ready || this.#started) throw new Error('Session Player/소유권의 초기화 전/반복 결선');
        this.#started = true;
        const playerHooks = {
            assertion: (kind) => {
                const report = services.ai?.assertion; if (!report) throw new Error('Player 실제 진단 장치 미연결');
                report(kind === PLAYER_WORLD_ASSERTION.playerAlreadyExists ? '!findSquid(bc,playerType)' : 'playerIdKnown');
            },
            find: (x, y, type) => o.world.find(x, y, type), create: (type, flags) => o.world.create(type, flags),
            setOwner: (sid, owner) => c.owner.set(sid, owner), pop: (sid, x, y, flags) => o.world.pop(sid, x, y, flags),
            reserveRegular: () => true, addRegular: (sid, event, payload) => check(c.processes.addRegular(sid, event, payload)),
        };
        o.player = new RawPlayerWorld(c.pool, c.postPop, o.players, o.playerState, playerHooks); c.postPop.setBody('player', o.player);
        // PATCH는 CD 전용 깊이 증감을 실행하지 않는다. 같은 현재 State/name/mission과 실제 저장소 부모만 빌린다.
        o.templateState = new RawPlayerTemplateState(o.headers, (slot) => o.templateStorage.selectState(slot), (slot) => o.templateStorage.name(slot), reference(o.objectState, 'mission'), missing(1246));
        o.templateState.bindInitialization({
            bytes: (slot, bytes) => { if (!services.templateStateBytes) throw new Error('빈 State의 현재 미정 바이트 입력 미연결'); services.templateStateBytes(bytes); },
            replace: (slot, bytes) => o.templateStorage.initializeState(slot, bytes),
        });
        o.templateState.bindMinimumSize((slot, minimum) => o.templateStorage.ensureStateSize(slot, minimum));
        // 원문(RawSurfaceStorage::BindPlayerRuntime)처럼 알 수 없는 사건의 Player.cpp 0x41d 진단(`0`)을 표시한 뒤 계속한다. (2026-11-22 WB04-E1211: 이전에는 오류를 던졌다)
        const regularHooks = { ...services.player, assertion: () => o.world.storage.assertDisplay('0', '\\Ns\\O\\Player.cpp', 0x41d) };
        if (!regularHooks.log) regularHooks.log = missing(1251);
        regularHooks.reserveRegular = () => true; regularHooks.addRegular = (sid, event, payload) => check(c.processes.addRegular(sid, event, payload));
        regularHooks.templateState = (slot) => o.templateState.get(slot); regularHooks.templateChanged = (slot) => o.templateState.changed(slot);
        o.playerRegular = new RawPlayerRegular(o.player, c.ownerMode, regularHooks);
        const effects = { ...services.ownership, otherOwner: (sid, owner) => o.priests.setOwner(sid, owner) };
        if (!effects.refreshProduction) effects.refreshProduction = missing(1259);
        if (!effects.themeMode) effects.themeMode = missing(1259);
        effects.missingIsland = (sid, island, owner) => {
            const storage = o.world.storage;
            reportVortexOwnerMissingIsland(c.pool, sid, island, owner, o.islands.count, {
                detail: (text) => storage.recordAssertionDetail(text), assertion: (expression, file, line) => storage.assertDisplay(expression, file, line),
            });
        };
        o.ownership = new RawCampaignOwnership(o.world, o.islands, o.state.assetState, o.players, o.metadata.frames, o.themes, o.random, o.paint, o.votes, reference(o, 'revision'), o.terrainType, o.gameplayGlobals.gameAux().edgeFarmType, effects);
        this.#ready = true;
    }

    // 현재 World 프로세스가 먼저 반납된 뒤 이 구간의 Player postPop 등록만 해제한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; if (this.#owner.player) this.#owner.core.postPop.setBody('player', null); }
    // 이 생성자 구간의 정상 복귀다. 전체 Session/Load 완료는 아니다.
    get ready() { return this.#ready; }
}
