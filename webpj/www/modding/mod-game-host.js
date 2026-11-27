// 실제 Netstorm Client의 값만 복사하고 원래 큐로 명령을 받는 신뢰 호스트다. 이 클래스 자체는 모드에 주지 않는다.
import { ModError, emptyModSnapshot, validateModSnapshot } from './mod-api.js';
import { queueModOutput } from './mod-input.js';

// Player 참여 flags와 선택 자산의 원본 필드는 이 호스트 안에서만 읽는다.
const PLAYER_STRIDE = 180, PLAYER_FLAGS = 0x50, ASSET_TYPE = 70, SLOT_TYPE = 10, SLOT_STATE = 11, SLOT_OWNER = 34, SLOT_HP = 26;

// 앱 수명에 붙이고 미션은 WeakRef로만 기억해 종료된 Session을 붙잡지 않는다.
export class ModGameHost {
    // 현재 앱과 미션 식별자/초기 seed 사본이다. 종료 시 앱 참조도 놓는다.
    #app; #world = null; #sessionId = null; #counter = 0; #seed = 0;
    // 같은 앱의 실제 화면·큐·시계가 준비되어 있어야 한다.
    constructor(app) {
        if (!app || typeof app._campaignDisplayActive !== 'function' || !app._clientInputQueue) throw new TypeError('모드 호스트의 실제 Client 장치 누락');
        this.#app = app;
    }
    // Ready 전에는 미션을 노출하지 않고 월드가 교체되면 새 공개 식별자를 발급한다.
    #current() {
        if (!this.#app) throw new ModError('disposed', '모드 게임 호스트 수명이 끝났습니다.');
        const app = this.#app, world = app._campaignWorld;
        if (!world || !app._clientGuide?.current()?.script?.name || !app._campaignDisplayActive() || world.connecting || !world.session().localConnectionReady() || !world.session().spStorage().initialized) { this.#world = null; this.#sessionId = null; return null; }
        if (this.#world?.deref() !== world) {
            this.#world = new WeakRef(world); this.#sessionId = `session-${++this.#counter}`;
            this.#seed = app._clientRandom.fortCoreSeed >>> 0;
        }
        return world;
    }
    // 게임 RNG를 전진시키지 않는 원래 미션 초기 seed다.
    sessionSeed() { return this.#current() ? this.#seed : 0; }
    // 시계/Player/SP/선택을 부작용 없이 읽는다. 내부 바이트와 typed 객체는 결과에 포함하지 않는다.
    snapshot() {
        const world = this.#current(), app = this.#app, layout = app._layout, output = emptyModSnapshot();
        output.state.logicalSize = { width: layout.logicalWidth, height: layout.logicalHeight };
        output.state.mapRect = { left: 0, top: 0, right: layout.logicalWidth, bottom: layout.logicalHeight };
        if (!world) return validateModSnapshot(output);
        const session = world.session(), records = session.playerRecords(), players = new DataView(records.buffer, records.byteOffset, records.byteLength), allies = session.alliances();
        Object.assign(output.state, { sessionId: this.#sessionId, mission: app._clientGuide.current().script.name, gameSeconds: app._gameClock.time().game, localOwner: session.localOwner() || null, paused: app._gameClock.paused(), mapRect: { ...world.mapView() } });
        // 로그인 또는 실제 AI brain이 있는 Player를 읽는다. AI-only의 flags0x20은 로그인bit와 다르며, 반납 뒤 남은0x20만으로 참가자를 만들지 않는다.
        for (let owner = 1; owner <= 8; owner += 1) {
            if (!(players.getUint32(owner * PLAYER_STRIDE + PLAYER_FLAGS, true) & 1) && !session.participation().brain(owner)) continue;
            const allied = [];
            // 방향별 현재 동맹 행을 그대로 읽되 자기 번호는 제외한다.
            for (let other = 1; other <= 8; other += 1) if (other !== owner && allies[owner * 9 + other]) allied.push(other);
            output.players.push({ owner, stormPower: session.sp(owner), allies: allied });
        }
        const selected = world.selected(), pool = session.core().pool;
        if (selected >= 5 && selected < pool.capacity) {
            const raw = pool.slot(selected);
            if (!(raw[SLOT_STATE] & 7) && raw[SLOT_TYPE] >= ASSET_TYPE) {
                const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
                output.selection = { typeId: raw[SLOT_TYPE], owner: raw[SLOT_OWNER], health: session.hitPoints().hasHitPoints(selected) ? view.getUint32(SLOT_HP, true) : 0 };
            }
        }
        return validateModSnapshot(output);
    }
    // 브리핑/모달/채팅·정지·사람의 연속 누름 중에는 입력을 가로채지 않는다.
    inputBlocked() {
        if (!this.#current()) return true;
        const app = this.#app;
        return Boolean(app._modPanel || app._gameClock.paused() || app._mainMenu?.dialog || app._campaignInputDialogOpen() || app._campaignChatInputActive() || app._input.pointer.buttons || app._clientComposing);
    }
    // 출력이 생성된 미션 번호를 별도로 받아 현재 미션으로 바뀐 요청을 거절한다.
    commit(output, { eventSequence, sessionId }) {
        const snapshot = this.snapshot(), app = this.#app;
        const result = queueModOutput(output, { eventSequence, sessionId, snapshot, queue: app._clientInputQueue, blocked: this.inputBlocked(), pointer: { x: app._input.pointer.x | 0, y: app._input.pointer.y | 0 } });
        if (result.pointer) Object.assign(app._input.pointer, result.pointer, { inside: true });
        return result.results;
    }
    // 앱 종료 전에 신뢰 호스트도 반납한다. WeakRef가 아닌 앱/큐 참조를 남기지 않는다.
    dispose() { this.#app = null; this.#world = null; this.#sessionId = null; }
}
