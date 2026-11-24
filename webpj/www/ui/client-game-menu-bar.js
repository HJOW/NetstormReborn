// 미션 화면 위쪽의 원본 가로 게임 메뉴막대(Game·View·Options·Players·About)를 실제 Client Gump 루트에 열고 닫는다(cpppj `UberGump::OpenGameMenu`·`ToggleGameMenu`).
// 사용: const bar = new ClientGameMenuBar(root, menu, devices); bar.toggle(); bar.isOpen(); bar.close().
//   menu = 같은 Client의 ClientMenuGump, devices = { global(주소), translate(키), config(키) → 정수, demo(), player(owner), refreshPriest(owner), playerMenu(subscriber) → 토큰, enabled(명령), storeText(문자열) → 토큰,
//   about() → 문자열, technicalHelp() → 0|1, width() → 논리 화면 폭, prepare()(선택), tracking(선택): 실제 SP 추적 Int32Box }.
// 원본 `ToggleGameMenu`: 같은 생성순번(serial)의 살아 있는 막대가 있으면 지우고, 없으면 새로 만든다. 열어도 게임 시계를 멈추지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1084)
import { GameMenuBarChild, GameMenuBarValue, RawGameMenuBar, createGameMenuBarState } from '../core/raw-game-menu-bar.js';
import { RawGameMenus } from './raw-game-menus.js';
import { RawUberRootLookup } from './raw-uber-root-lookup.js';

// 원본 검산 DWORD(0054d4e0)를 별도 사본 없이 소유하는 상자 수명은 Client와 같다.
export class ClientGameMenuBar {
    // 현재 Gump 루트·메뉴 작성기·장치와 같은 막대 상태·serial 조회기다.
    #root; #menu; #devices; #state; #lookup; #labels = new Map();
    constructor(root, menu, devices) {
        if (!root || !menu || !devices) throw new TypeError('게임 메뉴막대의 실제 루트·메뉴·장치 필요');
        for (const name of ['global', 'translate', 'config', 'demo', 'player', 'refreshPriest', 'playerMenu', 'enabled', 'storeText', 'about', 'technicalHelp', 'width']) {
            if (typeof devices[name] !== 'function') throw new TypeError(`게임 메뉴막대 장치 누락: ${name}`);
        }
        this.#root = root; this.#menu = menu; this.#devices = devices; this.#lookup = new RawUberRootLookup(root);
        // 실제 앱은 SP 저장소와 같은 검산 상자를 빌린다. 저장소가 없는 독립 UI 호출자는 지역 상자로 기존 계약을 유지한다. (WB04-E1327)
        this.#state = createGameMenuBarState(devices.tracking === undefined ? { value: 0 } : devices.tracking);
    }
    // 진단용: 메뉴 토큰별로 작성기에 쓴 항목 라벨(구분선은 빈 문자열이며 메뉴 처음과 연속 구분선은 작성기가 버리므로 기록하지 않는다)을 돌려준다. 가장 최근에 연 막대와 하위 메뉴만 보관한다.
    labels(token) { return [...(this.#labels.get(token) ?? [])]; }
    // 현재 열려 있는 막대의 토큰이다(없으면 0). 같은 생성순번의 비dead 노드만 인정한다.
    token() { return this.#state.menuSerial ? this.#lookup.reference(this.#state.menuSerial) : 0; }
    // 막대가 열려 있는지 알려 준다.
    isOpen() { return this.token() !== 0; }
    // 원본 순서(Player 준비 → 가로 생성 → Game/View/Options/Players → About → 폭·부착)로 막대를 연다. 이미 열려 있으면 그 토큰을 돌려준다.
    open() {
        const existing = this.token(); if (existing) return existing;
        const root = this.#root, devices = this.#devices, menus = {}, bar = {};
        this.#menu.bindGameMenus(bar, menus, devices.storeText);
        // 작성기에 쓰는 항목 라벨을 메뉴 토큰별로 기록한다(가로/세로/장식 시작이 새 목록을 쌓고 take 가 그 목록을 토큰에 묶는다).
        this.#labels.clear(); const stack = [];
        for (const hooks of [bar, menus]) {
            for (const name of ['horizontal', 'vertical', 'fancy']) if (hooks[name]) { const original = hooks[name]; hooks[name] = (...args) => { stack.push([]); return original(...args); }; }
            for (const [name, index] of [['item', 0], ['checked', 1], ['submenu', 0]]) { const original = hooks[name]; hooks[name] = (...args) => { if (name !== 'item' || args[3] !== 0) stack.at(-1)?.push(String(args[index])); return original(...args); }; }
            if (hooks.separator) { const original = hooks.separator; hooks.separator = (...args) => { const rows = stack.at(-1); if (rows?.length && rows.at(-1) !== '') rows.push(''); return original(...args); }; }
            const take = hooks.take; hooks.take = (...args) => { const rows = stack.pop(), token = take(...args); if (rows && token) this.#labels.set(token, rows); return token; };
        }
        // Game·View·Options·Players 부모의 현재 게임 경계다.
        Object.assign(menus, { global: (address) => devices.global(address), translate: (key) => devices.translate(key), config: (key) => devices.config(key), demo: () => devices.demo(),
            player: (owner) => devices.player(owner), refreshPriest: (owner) => devices.refreshPriest(owner), playerMenu: (subscriber) => devices.playerMenu(subscriber), enabled: (code) => devices.enabled(code), configEnabled: (key) => devices.configEnabled?.(key) ?? true,
            // 선택 장치: Player 이름 바이트를 표시 문자열로 바꾼다(한국어·일본어 확장 문자). (2026-11-21 WB04-E1166)
            ...(typeof devices.decodeName === 'function' ? { decodeName: (bytes) => devices.decodeName(bytes) } : {}) });
        // 프레임 상한(Frame Rate) 하위 메뉴 항목 장치다. 없으면 하위 메뉴를 만들지 않는다. (2026-11-20 WB04-E1094)
        if (typeof devices.frameRates === 'function') menus.frameRates = () => devices.frameRates();
        // 언어(Language) 하위 메뉴 항목 장치다. 없으면 하위 메뉴를 만들지 않는다. (2026-11-20 WB04-E1095)
        if (typeof devices.languages === 'function') menus.languages = () => devices.languages();
        // 난이도(Difficulty) 하위 메뉴 항목 장치다. 없으면 하위 메뉴를 만들지 않는다. (2026-11-20 WB04-E1096)
        if (typeof devices.difficulties === 'function') menus.difficulties = () => devices.difficulties();
        const parents = new RawGameMenus(menus);
        // 막대 자체의 현재 조회·번역·하위 부모 경계다. Edit·Cheat 는 웹 범위 밖이라 읽기 값이 0 이다.
        Object.assign(bar, { prepare: () => devices.prepare?.(), translate: (key) => devices.translate(key), about: () => devices.about(), technicalHelp: () => devices.technicalHelp(),
            read: (kind) => {
                switch (kind) {
                case GameMenuBarValue.Editor: case GameMenuBarValue.Cheat: return 0;
                case GameMenuBarValue.Battle: return devices.global(0x594fbc);
                case GameMenuBarValue.Mission: return devices.global(0x594fa4);
                case GameMenuBarValue.Width: return devices.width();
                case GameMenuBarValue.Left: return root.mapView().left | 0;
                case GameMenuBarValue.Root: return root.createState().collection.root;
                default: throw new RangeError('게임 메뉴막대 조회 종류 오류');
                }
            },
            child: (kind) => {
                switch (kind) {
                case GameMenuBarChild.Game: return parents.game();
                case GameMenuBarChild.View: return parents.view();
                case GameMenuBarChild.Options: return parents.options();
                case GameMenuBarChild.Players: return parents.players();
                default: throw new Error('웹 범위 밖: 게임 메뉴막대의 Edit·Cheat 하위');
                }
            } });
        return new RawGameMenuBar(this.#state, bar).open();
    }
    // 원본 `View Player List - F9`: 현재 Player 목록을 화면 중앙의 장식 메뉴로 연다(막대 없이 `RawGameMenus.players(true)` 만 사용). 열린 메뉴 토큰을 돌려주며 메타맵이면 0 이다.
    openPlayerList() {
        const devices = this.#devices, menus = {}; this.#menu.bindGameMenus({}, menus, devices.storeText);
        Object.assign(menus, { global: (address) => devices.global(address), translate: (key) => devices.translate(key), config: (key) => devices.config(key), demo: () => devices.demo(),
            player: (owner) => devices.player(owner), refreshPriest: (owner) => devices.refreshPriest(owner), playerMenu: (subscriber) => devices.playerMenu(subscriber), enabled: (code) => devices.enabled(code), configEnabled: (key) => devices.configEnabled?.(key) ?? true,
            // 선택 장치: Player 이름 바이트를 표시 문자열로 바꾼다(한국어·일본어 확장 문자). (2026-11-21 WB04-E1166)
            ...(typeof devices.decodeName === 'function' ? { decodeName: (bytes) => devices.decodeName(bytes) } : {}) });
        return new RawGameMenus(menus).players(true);
    }
    // 열린 막대와 펼쳐진 하위 메뉴를 지운다. 열려 있지 않으면 아무것도 하지 않는다.
    close() {
        const token = this.token(); if (!token) return false;
        this.#root.remove(token); this.#state.menuSerial = 0; this.#menu.closePopups(); return true;
    }
    // 원본 ToggleGameMenu: 열려 있으면 닫고 닫혀 있으면 연다. 연 결과로 열려 있으면 true 다.
    toggle() { if (this.isOpen()) { this.close(); return false; } this.open(); return true; }
}
