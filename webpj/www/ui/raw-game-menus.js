// 원본 10.78 의 미션 중 Game·View·Options·Players 메뉴 부모를 현재 게임과 공통 메뉴 작성기에 연결한다(cpppj `client/RawGameMenus.cpp/.h`, 웹 범위만).
// 웹 범위: 단독 미션·데모만 다루므로 Editor(Edit)·멀티플레이 이탈·Main Menu(메타)·Quit Game·해상도/전체화면·서버 진단 항목은 만들지 않는다(AGENTS.md 웹 제외 목록).
// 사용: const menus = new RawGameMenus(hooks); menus.game(); menus.view(); menus.options(); menus.players(); — 각 호출은 같은 controlled 메뉴 스택에서 자신의 완성 토큰을 꺼내 돌려준다.
//   bindGameMenusControls(hooks, widgets, controls, menus, storeText) — 이미 옮긴 가로/세로/항목/구분선 작성기를 hooks 에 연결한다.
// hooks = { global(주소) → DWORD(각 원문 조회 시점의 현재 값; 명령 전역 주소는 명령 ID), translate(키) → 문자열, config(키) → 정수, demo() → boolean, player(owner) → 180BYTE,
//   enabled(명령 ID) → boolean(선택, 앱이 아직 연결하지 않은 명령은 비활성으로 보인다), configEnabled(설정 키) → boolean(선택, 아직 연결하지 않은 설정 키는 비활성), item(라벨, 명령, 인자, include, enabled), checked(체크, 라벨, 명령, 인자, enabled), submenu(라벨, 자식, enabled, check),
//   vertical(), fancy(), separator(), title(문구), take(), current(), node(토큰), attachCurrent(x, y, 부모), playerMenu(subscriber) → 토큰, refreshPriest(owner) }(도달한 경계만 요구).
// 원본 전역 주소는 원문 조건을 추적하기 위한 식별자이며 호스트 포인터로 읽지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1084: ESC 게임 메뉴막대의 하위 메뉴로 추가)

// 원본 전역 주소 식별자다(cpppj RawGameMenus.cpp 의 상수).
const G = Object.freeze({ editor: 0x5c85a4, missionPost: 0x594fa4, battle: 0x594fbc, fort: 0x594fb8, challenge: 0x594fc8, missionBridge: 0x594fa0, hideBuildings: 0x59a8ac,
    owner: 0x540c70, pausedTime: 0x55b4b0, soundDevice: 0x5c7b14, soundVolume: 0x531900, musicVolume: 0x531904, observer: 0x52f81e });
// 원본 명령 전역 주소 식별자다. 실제 명령 ID 는 `hooks.global` 이 `gameMenuCommandValue` 로 돌려준다.
const C = Object.freeze({ review: 0x542938, restart: 0x542928, leave: 0x542924, tell: 0x542948, hideBuildings: 0x542ab8, chat: 0x542914, home: 0x5429ac, priest: 0x5429d4,
    knowledge: 0x542974, playerList: 0x542ab4, config: 0x5428ac, pause: 0x542a30, rotate: 0x542ab0, soundVolume: 0x542a8c, musicVolume: 0x542a90, draw: 0x542a60 });

// 도달한 명시 의존만 요구하여 미연결 하위를 빈 메뉴로 숨기지 않는다.
function need(callback, text) { if (typeof callback !== 'function') throw new Error(text); return callback; }
// 원본 문자열의 첫 NUL 뒤는 현재 라벨에 넣지 않는다.
const cString = (value) => { const end = String(value).indexOf('\0'); return end < 0 ? String(value) : String(value).slice(0, end); };

// 원본 부모 몸체다. 모든 조회는 각 원문 조회 시점에 hooks 에서 다시 읽는다.
export class RawGameMenus {
    // 현재 게임·메뉴 작성기 경계를 보관한다.
    #hooks;
    constructor(hooks) { if (!hooks) throw new TypeError('게임 메뉴의 실제 경계 필요'); this.#hooks = hooks; }
    // 원문 독립 전역 값을 조회한다.
    #global(address) { return need(this.#hooks.global, '게임 메뉴의 현재 전역 조회 연결 누락')(address) >>> 0; }
    // 현재 번역 결과를 소유한다.
    #text(key) { return cString(need(this.#hooks.translate, '게임 메뉴의 실제 번역 연결 누락')(key)); }
    // 앱이 아직 연결하지 않은 명령 ID 는 비활성으로 보이게 한다(연결 여부를 모르는 호출자는 항상 활성이다).
    #wired(code, enabled) { return Boolean(enabled) && (typeof this.#hooks.enabled !== 'function' || this.#hooks.enabled(code >>> 0)); }
    // 번역 뒤 명령 전역을 읽어 일반 항목을 쓴다(원본은 번역을 마친 뒤 명령 전역을 읽는다).
    #item(key, address, parameter = 0, include = 1, enabled = 1) {
        const label = this.#text(key), code = this.#global(address);
        need(this.#hooks.item, '게임 메뉴의 실제 항목 작성 연결 누락')(label, code, parameter, include, this.#wired(code, enabled) ? 1 : 0);
    }
    // 현재 체크 값과 이미 번역한 문구를 같은 체크 행에 쓴다.
    #check(check, text, address, parameter = 0, enabled = 1) {
        const code = this.#global(address);
        need(this.#hooks.checked, '게임 메뉴의 실제 체크 항목 작성 연결 누락')(check ? 1 : 0, text, code, parameter, this.#wired(code, enabled) ? 1 : 0);
    }
    // 설정 반전 문자열과 번역 뒤 다시 읽는 체크 값은 원본의 두 별개 조회다. 인자는 `키=0|1` 이며 설정 명령(0x6d)으로 간다.
    #toggle(key, label, enabledAddress = 0) {
        const hooks = this.#hooks, parameter = `${key}${need(hooks.config, '게임 메뉴의 설정 조회 연결 누락')(key) ? '=0' : '=1'}`, text = this.#text(label);
        // 앱이 아직 연결하지 않은 설정 키(configEnabled 가 false)는 비활성으로 보인다.
        const enabled = (enabledAddress ? this.#global(enabledAddress) : 1) && (typeof hooks.configEnabled !== 'function' || hooks.configEnabled(key)), code = this.#global(C.config), checked = hooks.config(key);
        need(hooks.checked, '게임 메뉴의 실제 체크 항목 작성 연결 누락')(checked ? 1 : 0, text, code, parameter, this.#wired(code, enabled) ? 1 : 0);
    }
    // Game: 미션이면 목표 보기·재시작·나가기, 데모이면 재시작·나가기만 둔다(원본 Quit Game 은 웹 범위 밖이다).
    game() {
        const hooks = this.#hooks; need(hooks.vertical, '게임 메뉴의 세로 작성 연결 누락')();
        if (this.#global(G.editor)) throw new Error('웹 범위 밖: Editor 의 Game 메뉴');
        if (this.#global(G.missionPost)) {
            if (!need(hooks.demo, '게임 메뉴의 데모 판정 연결 누락')()) {
                this.#item('Review Mission Objectives - ~iF8~.', C.review); need(hooks.separator, '게임 메뉴의 구분선 연결 누락')(); this.#item('Restart Mission', C.restart); this.#item('Leave Mission', C.leave);
            } else { this.#item('Restart Demo', C.restart); this.#item('Exit Demo', C.leave, 1); }
        }
        return need(hooks.take, '게임 메뉴의 꺼내기 연결 누락')();
    }
    // View: 원본 상황에 따라서만 게임 조작행을 추가하며 테마·배너 체크값은 같은 설정을 다시 읽는다.
    view() {
        const hooks = this.#hooks; need(hooks.vertical, '게임 메뉴의 세로 작성 연결 누락')();
        // 번역이 바꿀 수 있는 현재 미션 조건은 번역 뒤에 읽는다.
        const chat = () => { const label = this.#text('Hide/Show chat - ~iF3~.'), include = this.#global(G.missionPost) === 0, code = this.#global(C.chat); hooks.item(label, code, 2, include ? 1 : 0, this.#wired(code, 1) ? 1 : 0); };
        if (this.#global(G.challenge)) { chat(); this.#item('Refresh Zones - ~iF9~.', 0x542a0c); }
        if (this.#global(G.fort) || this.#global(G.battle)) {
            need(hooks.separator, '게임 메뉴의 구분선 연결 누락')(); let label = this.#text('Hide buildings - ~iF2~.'); this.#check(this.#global(G.hideBuildings), label, C.hideBuildings);
            // 원문 철자 theammode 와 초기 0 을 보존한다.
            const islands = need(hooks.config, '게임 메뉴의 설정 조회 연결 누락')('theammode');
            label = this.#text('Hide/Show Island Themes - ~iShift F3~.'); this.#check(islands === 0, label, C.tell, islands ? 'SetIslands(0)' : 'SetIslands(1)');
            chat(); this.#item('View Home Temple - ~iF4~.', C.home);
            label = this.#text('View Your Priest - ~iF5~.'); const owner = this.#global(G.owner), code = this.#global(C.priest);
            hooks.item(label, code, owner, 1, this.#wired(code, 1) ? 1 : 0);
            if (!this.#global(G.editor)) this.#item('View Netstorm Knowledge - ~iF6~.', C.knowledge);
            this.#toggle('useBanners', 'Hide Island Ownership - ~iF7~.');
            if (this.#global(G.missionBridge)) this.#item('Review Mission Objectives - ~iF8~.', C.review);
            this.#item('View Player List - ~iF9~.', C.playerList, 1);
        }
        return hooks.take();
    }
    // Options: 웹에서 의미 있는 소리·음량·화면 이동·정지·회전 항목만 둔다(해상도·전체화면·서버 진단은 웹 범위 밖).
    options() {
        const hooks = this.#hooks; need(hooks.vertical, '게임 메뉴의 세로 작성 연결 누락')();
        const separator = need(hooks.separator, '게임 메뉴의 구분선 연결 누락');
        // 언어(Language) 하위 메뉴다. 항목 이름은 각 언어 자신의 표기이며 선택은 Tell 대상 `Patch1101.SetLanguage(n)` 으로 가서 앱이 설정에 쓰고 다시 불러 적용한다(cpppj 6차 한국어). 장치가 없으면 만들지 않는다. (2026-11-20 WB04-E1095)
        if (typeof hooks.languages === 'function') {
            hooks.vertical();
            for (const choice of hooks.languages()) this.#check(choice.checked, choice.label, C.tell, `Patch1101.SetLanguage(${choice.number})`);
            need(hooks.submenu, '게임 메뉴의 하위 작성 연결 누락')(this.#text('Language'), hooks.take(), 1, 0);
        }
        // 프레임 상한(Frame Rate) 하위 메뉴다. 선택은 Tell 대상 `Patch1101.SetFrameRate(n)` 이며 앱이 maxFPS 설정에 쓴다(cpppj 후순위 F1). 장치가 없으면 만들지 않는다. (2026-11-20 WB04-E1094)
        if (typeof hooks.frameRates === 'function') {
            hooks.vertical();
            for (const rate of hooks.frameRates()) this.#check(rate.checked, rate.label, C.tell, `Patch1101.SetFrameRate(${rate.number})`);
            need(hooks.submenu, '게임 메뉴의 하위 작성 연결 누락')(this.#text('Frame Rate'), hooks.take(), 1, 0); separator();
        }
        this.#toggle('sound', 'Sound On', G.soundDevice); this.#toggle('music', 'Play Music', G.soundDevice); this.#toggle('playWind', 'Wind Noise', G.soundDevice); this.#toggle('swapLeftRightSpeakers', 'Speaker Swap L/R', G.soundDevice);
        // 각 음량 하위는 현재 전역을 매 행 조회하고 제목 번역 뒤 현재 장치 활성을 읽는다.
        for (let channel = 0; channel < 2; channel += 1) {
            hooks.vertical();
            for (let volume = 1; volume <= 5; volume += 1) { const label = this.#text(`Volume ${volume}`); this.#check(this.#global(channel ? G.musicVolume : G.soundVolume) === volume, label, channel ? C.musicVolume : C.soundVolume, volume); }
            const label = this.#text(channel ? 'Music Volume' : 'Sound Effect Volume'), child = hooks.take();
            need(hooks.submenu, '게임 메뉴의 하위 작성 연결 누락')(label, child, this.#global(G.soundDevice) ? 1 : 0, 0);
        }
        separator(); this.#toggle('edgeScroll', 'Edge Scroll in Fullscreen');
        if (this.#global(G.battle) || this.#global(G.editor)) this.#toggle('cursorSnapToGrid', 'Cursor Snap To Grid');
        this.#toggle('autoDemo', 'Auto-Demo'); this.#toggle('tellTips', 'Tell Tips at Startup');
        if (this.#global(G.missionPost) || this.#global(G.editor)) { const label = this.#text('Pause - ~iShift-F9~.'); this.#check((this.#global(G.pausedTime) | 0) > 0, label, C.pause); }
        separator();
        if (!this.#global(G.fort) && !this.#global(G.challenge)) this.#toggle('rotmode', 'Counterclockwise Piece Rotation - ~iC~.');
        return hooks.take();
    }
    // Players: 현재 Player 1..8 을 원본 stride 180 순서로 방문한다. 메타맵은 작성 전에 0 을 반환한다. 각 하위는 기존 Player 메뉴가 만든다.
    players(fancy = false) {
        const hooks = this.#hooks; if (this.#global(G.challenge)) return 0;
        if (fancy) { need(hooks.fancy, '게임 메뉴의 장식 작성 연결 누락')(); need(hooks.title, '게임 메뉴의 제목 작성 연결 누락')(this.#text('Player List\n')); } else need(hooks.vertical, '게임 메뉴의 세로 작성 연결 누락')();
        const word = (player, offset) => (player[offset] | (player[offset + 1] << 8) | (player[offset + 2] << 16) | (player[offset + 3] << 24)) >>> 0;
        for (let owner = 1; owner <= 8; owner += 1) {
            const player = need(hooks.player, '게임 메뉴의 현재 Player 조회 연결 누락')(owner); if (player.length < 180) throw new RangeError('메뉴의 현재 Player 180BYTE 범위');
            if (!word(player, 0x2c)) continue;
            need(hooks.refreshPriest, '게임 메뉴의 사제 참여 갱신 연결 누락')(owner);
            // 원본은 후속 번역·하위 효과 전에 체크를 저장한다. 이름은 +c 의 최대 32BYTE 첫 NUL 까지다.
            const checked = word(player, 0x50) & 0x21; let length = 0; while (length < 32 && player[0xc + length]) length += 1;
            if (length === 32) throw new RangeError('원본 Player 이름의 NUL 누락');
            const label = String.fromCharCode(...player.subarray(0xc, 0xc + length));
            const enabled = this.#global(G.observer + owner) === 0, child = need(hooks.playerMenu, '게임 메뉴의 Player 하위 연결 누락')(word(player, 0x2c));
            hooks.submenu(label, child, enabled ? 1 : 0, checked);
        }
        const menu = need(hooks.current, '게임 메뉴의 현재 메뉴 조회 연결 누락')();
        if (!fancy) return hooks.take();
        const node = need(hooks.node, '게임 메뉴의 노드 조회 연결 누락')(menu); if (!node) throw new Error('현재 Player 목록 메뉴 노드 소멸');
        const x = Math.trunc(Math.trunc(need(hooks.global, '게임 메뉴의 전역 조회 연결 누락')(0x531860) / 2) - node.baseWidth * 0.5), y = Math.trunc(Math.trunc(hooks.global(0x531864) / 2) - node.baseHeight * 0.5);
        need(hooks.attachCurrent, '게임 메뉴의 현재 부착 연결 누락')(x | 0, y | 0, this.#global(0x563e38)); return menu;
    }
}

// 이미 옮긴 가로/세로/항목/구분선 작성기를 hooks 에 연결한다. 문자열 인자는 같은 문자열 고리에 저장한 토큰으로 바꾼다.
export function bindGameMenusControls(hooks, widgets, controls, menus, storeText) {
    hooks.vertical = () => { widgets.beginVertical(0); }; hooks.fancy = () => { widgets.beginFancy(); };
    hooks.separator = () => controls.addSeparator(); hooks.title = (text) => controls.addTitle(text);
    hooks.take = () => menus.takeCurrent(); hooks.current = () => menus.current(); hooks.attachCurrent = (x, y, parent) => menus.attachCurrent(x, y, parent);
    const parameter = (value) => typeof value === 'number' ? value >>> 0 : need(storeText, '게임 메뉴의 문자열 고리 연결 누락')(String(value));
    hooks.item = (label, code, value, include, enabled) => controls.addText(label, code, parameter(value), include !== 0, enabled !== 0, false);
    hooks.checked = (check, label, code, value, enabled) => controls.addChecked(check !== 0, label, code, parameter(value), true, enabled !== 0);
    hooks.submenu = (label, child, enabled, check) => controls.addSubmenu(label, child, enabled !== 0, check !== 0);
}
