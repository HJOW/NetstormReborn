// 원본 10.78 의 화면 위 가로 Game 메뉴막대(004cc210)를 같은 controlled 메뉴에 작성하는 모듈이다(cpppj `client/RawGameMenuBar.cpp/.h`, 10.78 전용).
// 사용:
//   const state = createGameMenuBarState(tracking);   // tracking = `{ value }`(0054d4e0 검산 DWORD 의 실제 소유 상자, signed 32비트). 별도 사본을 갖지 않는다.
//   const bar = new RawGameMenuBar(state, hooks);  const root = bar.open();   // 완성한 가로 토큰을 반환하며 controlled 스택은 비운다.
//   hooks = { read(GameMenuBarValue) → DWORD(각 원문 조회 시점의 현재 값), prepare(), translate(문자열) → 원본 바이트 번역, about() → 현재 About 제목, child(GameMenuBarChild) → 토큰,
//             horizontal() → 토큰, take() → 토큰, technicalHelp() → DWORD(enabled 가 아니라 include 인자), vertical(), separator(), submenu(문자열, 토큰), item(문자열, 명령, 문자열 인자, include),
//             width(토큰, float), attach(토큰, x, y, 루트, 1), serial(토큰) → 부착 후 Gump+2c 의 현재 생성순번 }.
//   순서: 현재 Player/검산 준비 → 가로 생성 → Game → (Editor 일 때) Edit → View → Options → 검산 +3(DWORD 감김, signed 9600 초과면 9600 빼기) → (Battle 일 때) Players → About 세로 메뉴(General Help·Technical Help·구분선·Version)
//     → controlled 에서 About 을 꺼내 현재 제목으로 부착 → (Cheat 와 Mission 이 모두 참일 때) Cheat 부착 → 남은 하나 꺼내기 → 폭(Width−Left 의 signed 차이) → 폭 설정 뒤 다시 읽은 Left 로 x 부착 → 생성순번 기록.
//   하위 부모/번역/GUI 는 명시 경계이며 누락을 빈 성공으로 바꾸지 않는다(`/연결 누락/` Error). 메뉴 생성 뒤 바뀔 수 있는 모드/폭/생성순번은 원문 접근 순서대로 다시 읽는다.
//   bindGameMenuBarControls(hooks, widgets, controls, menus, node, storeText) — 이미 옮긴 가로/세로/항목/크기/가상 부착 본체(MenuGumpWidgets/Controls/MenuGump)를 hooks 에 연결한다.
// 원본 관측: `game-menu-bar-x86.tsv` 510행을 tests/raw-game-menu-bar.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E539: 6단계 게임 메뉴 RawGameMenuBar 이식)

// 부모가 각 접근 시점에 다시 읽는 서로 다른 원본 전역이다. (2026-11-15 WB04-E539)
export const GameMenuBarValue = Object.freeze({ Editor: 0, Battle: 1, Cheat: 2, Mission: 3, Width: 4, Left: 5, Root: 6 });
// 각 하위 부모는 실제 controlled 메뉴에서 자신의 완성 토큰을 꺼내 반환한다. (2026-11-15 WB04-E539)
export const GameMenuBarChild = Object.freeze({ Game: 0, Edit: 1, View: 2, Options: 3, Players: 4, Cheat: 5 });

// 원본 0054d4e0 의 검산 DWORD 와 005cad24 의 실제 생성순번을 같은 Client 수명에 보관하는 상태이다. tracking 은 같은 SpStore/사제 0054d4e0 소유 주소를 빌리는 `{ value }` 상자이다. (2026-11-15 WB04-E539)
export function createGameMenuBarState(tracking) {
    if (tracking === null || typeof tracking !== 'object') throw new TypeError('게임 메뉴 검산 상자 필요');
    // checksum 은 DWORD 감김 뒤 signed>9600 일 때만 9600 을 한 번 뺀다. menuSerial 은 토큰/등록 배열 칸 id(+1c)와 구별하는 Gump+2c 의 현재 생성순번이다.
    // 00542754/0054281c/00542948 의 현재 명령 전역이 helpCommand/technicalCommand/tellCommand 이다.
    return { checksum: tracking, menuSerial: 0, helpCommand: 0x02000017, technicalCommand: 0x02000049, tellCommand: 0x02000095 };
}

// 도달한 명시 의존만 요구하여 실제 미복원 하위를 빈 메뉴로 숨기지 않는다. (2026-11-15 WB04-E539)
function need(value, text) {
    if (typeof value !== 'function') throw new Error(text);
    return value;
}

// 원본 부모 전체의 메뉴 순서·조건·조회·폭·부착·생성순번 쓰기를 복원한다. (2026-11-15 WB04-E539)
export class RawGameMenuBar {
    // 같은 실제 상태와 명시 경계를 빌린다. (2026-11-15 WB04-E539)
    constructor(state, hooks) {
        this.state = state;
        this.hooks = hooks;
    }

    // 메뉴 생성 뒤 바뀔 수 있는 모드/폭/생성순번은 원문 접근 순서대로 다시 읽는다. 완성한 실제 가로 토큰을 반환한다. (2026-11-15 WB04-E539)
    open() {
        const h = this.hooks;
        const state = this.state;
        need(h.prepare, '게임 메뉴의 현재 Player 준비 연결 누락')();
        // 현재 가로 메뉴 토큰이다.
        const root = need(h.horizontal, '게임 메뉴의 실제 가로 생성 연결 누락')();
        // 원본의 번역→부모 몸체→자식 부착 순서를 공통으로 표현한다.
        const submenu = (name, kind) => {
            // 하위 생성보다 먼저 원본 문자열을 읽는다.
            const label = need(h.translate, '게임 메뉴의 실제 번역 연결 누락')(name);
            // 같은 controlled 스택에서 꺼낸 하위다.
            const child = need(h.child, '게임 메뉴의 실제 하위 부모 연결 누락')(kind);
            need(h.submenu, '게임 메뉴의 실제 자식 작성 연결 누락')(label, child);
        };
        submenu('Game', GameMenuBarChild.Game);
        if (need(h.read, '게임 메뉴의 현재 전역 조회 연결 누락')(GameMenuBarValue.Editor)) submenu('Edit', GameMenuBarChild.Edit);
        submenu('View', GameMenuBarChild.View);
        submenu('Options', GameMenuBarChild.Options);
        // DWORD 감김 덧셈 뒤 signed 9600 초과일 때만 9600 을 한 번 뺀다.
        state.checksum.value = (((state.checksum.value >>> 0) + 3) | 0);
        if (state.checksum.value > 9600) state.checksum.value = (((state.checksum.value >>> 0) - 9600) | 0);
        if (h.read(GameMenuBarValue.Battle)) submenu('Players', GameMenuBarChild.Players);
        need(h.vertical, 'About의 실제 세로 생성 연결 누락')();
        // 원본 첫 행의 실제 바이트이다.
        let label = h.translate('General Help - ~iF1~.');
        need(h.item, 'About의 실제 행 작성 연결 누락')(label, state.helpCommand, 'F1Help', 1);
        label = h.translate('Technical Help');
        // enabled 가 아니라 include 인자이다.
        const technical = need(h.technicalHelp, '기술 도움말의 실제 가용성 조회 연결 누락')();
        h.item(label, state.technicalCommand, '', technical);
        need(h.separator, 'About의 실제 구분선 연결 누락')();
        label = h.translate('Version');
        h.item(label, state.tellCommand, 'About', 1);
        // About 하위를 먼저 꺼낸다.
        let child = need(h.take, '게임 메뉴의 실제 controlled 꺼내기 연결 누락')();
        // 원본 0054db38 은 그대로 읽는다.
        const about = need(h.about, '현재 About 제목 연결 누락')();
        h.submenu(about, child);
        if (h.read(GameMenuBarValue.Cheat) && h.read(GameMenuBarValue.Mission)) {
            // 원본 직접 new/Construct 경계이며 allocation 실패 0 도 보존한다.
            child = h.child(GameMenuBarChild.Cheat);
            label = h.translate('Cheat');
            h.submenu(label, child);
        }
        h.take();
        // 원본 signed 차이를 DWORD 감김으로 계산한다.
        const left = h.read(GameMenuBarValue.Left) >>> 0;
        const width = h.read(GameMenuBarValue.Width) >>> 0;
        need(h.width, '게임 메뉴의 실제 폭 설정 연결 누락')(root, Math.fround((width - left) | 0));
        // 폭 설정 뒤 바뀔 수 있어 다시 읽는다.
        const x = Math.fround(h.read(GameMenuBarValue.Left) | 0);
        // 같은 등록표의 현재 루트이다.
        const parent = h.read(GameMenuBarValue.Root);
        need(h.attach, '게임 메뉴의 실제 루트 부착 연결 누락')(root, x, 0, parent, 1);
        state.menuSerial = need(h.serial, '게임 메뉴의 현재 생성순번 조회 연결 누락')(root);
        return root;
    }
}

// 이미 복원한 가로/세로/항목/크기/가상 부착의 본체를 그대로 공유한다. node(token) 은 현재 노드(`serial` 를 가진 객체) 또는 null 이고 storeText(문자열) 는 문자열 고리 토큰이다. (2026-11-15 WB04-E539)
export function bindGameMenuBarControls(hooks, widgets, controls, menus, node, storeText) {
    hooks.horizontal = () => widgets.beginHorizontal();
    hooks.vertical = () => { widgets.beginVertical(0); };
    hooks.take = () => menus.takeCurrent();
    hooks.separator = () => controls.addSeparator();
    hooks.submenu = (text, child) => controls.addSubmenu(text, child, true, false);
    hooks.item = (text, command, parameter, include) => {
        // 숫자 0 과 문자열 수명을 구별한다.
        const value = parameter === '' || parameter === undefined ? 0 : need(storeText, '게임 메뉴의 실제 문자열 고리 연결 누락')(parameter);
        controls.addText(text, command, value, include !== 0, true, false);
    };
    hooks.width = (token, width) => menus.setWidth(token, width);
    hooks.attach = (token, x, y, parent, mode) => widgets.attach(token, x, y, parent, mode);
    hooks.serial = (token) => {
        // 부착 후 생성순번이며 등록 칸 id 와 구별한다.
        const current = need(node, '게임 메뉴의 실제 노드 조회 연결 누락')(token);
        if (!current) throw new Error('게임 메뉴의 현재 노드 소멸');
        return current.serial;
    };
}
