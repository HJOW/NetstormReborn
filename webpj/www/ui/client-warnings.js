// 미션 화면 오른쪽 위에 쌓이는 경고·상태 안내(Game Paused!·Game Time·Warning 문장·SP 수신 등)를 실제 Client Gump 루트와 글자 장치에 연결한다.
// 근거: cpppj `UberGump` 의 `warnings_`(`RawButtonControls::EnableWarnings` → `WarningMessages`)와 `MessageGumpHooks pauseContext`.
// 사용: const warnings = new ClientWarnings(root, text, fonts, { now, sound, translate, context, suppressed, view, changed });
//   warnings.pauseStatus(true/false) → 'Game Paused!' 안내를 만들거나 지운다(원본 Pause 메뉴 명령의 상태 표시, 종류 11).
//   warnings.add(문장)·addNull()·gameTimeStatus()·battleStatus(kind, player, parameter)·remove(kind, player, parameter) → 같은 20칸 표에 요청.
//   warnings.hasMessages()·report()·close() → 살아 있는 안내 확인, 원문 TSV 보고, 미션 반납 때 정리.
// 실제 안내는 Gump 루트의 직계 자식(글자 Gump 파생 노드)이며 화면 오른쪽 끝에서 12픽셀 간격의 칸에 붙고 기한이 지나면 idle 이 지운다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-20 WB04-E1089: 일시정지 Shift-F9 와 Game Paused! 표시를 위해 추가)
import { WarningMessages } from './warning-messages.js';

// 같은 Client Gump 루트에 안내 노드를 만들고 지우는 소유자다.
export class ClientWarnings {
    // 실제 루트·글자 장치·글꼴, 원본 안내 처리기와 현재 살아 있는 파생 노드 토큰들이다.
    #root; #text; #fonts; #devices; #messages; #owned = new Set();
    // devices = { now(): 벽 시각(초), sound(): 경고음, translate(문장), context: 메시지 이름·모드 조회 장치, suppressed(): 안내 억제 여부, view(): { width, height, palette }, changed(): 화면 갱신 요청 }.
    constructor(root, text, fonts, devices) {
        if (!root || !text || !fonts || !devices || ['now', 'sound', 'translate', 'suppressed', 'view', 'changed'].some((name) => typeof devices[name] !== 'function') || !devices.context) throw new TypeError('경고 안내의 실제 Client·글자·시계 장치 누락');
        this.#root = root; this.#text = text; this.#fonts = fonts; this.#devices = devices;
        const lifetime = root.lifetime();
        // 원본 공통 등록·수명·트리 장치를 같은 Client 루트의 것으로 빌려 준다. 파생 노드는 allocate 에서 글자 Gump 로 등록한다.
        const common = { create: root.createState(), life: root.lifetimeState(), collection: root.collection(), tree: { attach: (...args) => root.attachBase(...args) }, lifetime, node: (token) => root.node(token), changed: () => devices.changed(),
            allocate: (node) => this.#allocate(node) };
        // 글자 구성·적합·소멸은 같은 ClientTextGump 에, 스타일 해석은 같은 스타일 엔진에 위임한다. 팔레트는 글자 Gump 의 스타일 장치가 매번 현재 색을 읽으므로 따로 바꾸지 않는다.
        const menus = { textBackend: () => text, styleBackend: () => ({ parse: (field, value, context) => text.styleEngine().parse(field, fonts.encode(value), context) }), setPalette: () => {}, setExternalGumps: () => {} };
        this.#messages = new WarningMessages(common, menus, () => devices.now(), () => devices.sound(), { ...devices.context, translate: (value) => devices.translate(value) }, () => devices.suppressed());
    }
    // 안내 노드를 글자 Gump 파생 노드로 등록하고 소유 목록에 보관한다. idle 은 기한 만료 삭제를, 소멸자는 글자 본체 정리를 맡는다.
    #allocate(node) {
        const token = this.#text.allocateDerived(node, { idle: (current) => this.#messages.idle(current), destroy: (current, flags) => this.#destroy(current, flags) });
        this.#owned.add(token); return token;
    }
    // 파생 소멸자가 같은 글자 본체를 정리한 뒤 실제 해제 때 보관 노드와 소유 토큰을 지운다.
    #destroy(token, flags) {
        const result = this.#messages.deleteScalar(token, flags);
        if (flags & 1) { this.#messages.release(token); this.#owned.delete(token); }
        return result;
    }
    // 안내를 만들기 전에 현재 논리 화면 크기를 원본처럼 안내 처리기에 알린다(`SetView`).
    #view() { const view = this.#devices.view(); this.#messages.setView(view.width, view.height, view.palette); }
    // 원본 Warning(종류 17) 문장을 요청한다.
    add(text) { this.#view(); this.#messages.add(text); this.#devices.changed(); }
    // 원본 NULL 문장을 요청한다.
    addNull() { this.#view(); this.#messages.addNull(); this.#devices.changed(); }
    // 영구 'Game Paused!' 안내를 만들거나 지연 삭제한다(종류 11).
    pauseStatus(enabled) { this.#view(); this.#messages.pauseStatus(Boolean(enabled)); this.#devices.changed(); }
    // 게임 시간 표시(종류 16, 플레이어 1)를 요청한다.
    gameTimeStatus() { this.#view(); this.#messages.gameTimeStatus(); this.#devices.changed(); }
    // 전투 통지류 안내를 요청한다.
    battleStatus(kind, player, parameter) { this.#view(); this.#messages.battleStatus(kind, player, parameter); this.#devices.changed(); }
    // 같은 종류·플레이어·인자의 안내를 지운다.
    remove(kind, player, parameter) { this.#view(); this.#messages.remove(kind, player, parameter); this.#devices.changed(); }
    // 살아 있는 안내가 하나라도 있으면 true 이다.
    hasMessages() { return this.#messages.hasMessages(); }
    // 현재 안내 표를 원문 TSV 로 돌려준다.
    report() { return this.#messages.report(); }
    // 미션을 반납할 때 아직 남은 안내 노드를 모두 실제 수명으로 닫는다.
    close() {
        for (const token of [...this.#owned]) { if (this.#root.node(token)) this.#root.remove(token); else this.#owned.delete(token); }
        this.#messages.destroy(); this.#owned.clear();
    }
}
