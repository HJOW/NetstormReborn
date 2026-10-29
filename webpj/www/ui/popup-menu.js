// 메인 메뉴의 펼침 메뉴(Help·Options)를 다루는 모듈이다. 튀어나온 돌 목록(MenuList raised) 한 단계 이상을 쌓아 열고, 입력 규칙을 원본 기록대로 처리한다.
// 입력 규칙(docs/videos/menu-buttons-20261003.md, docs/screens/options-menu.md 의 원본 관측):
//  - 포인터가 올라간 행은 어둡게 강조되지만 `>` 표시가 붙은 하위 메뉴는 올리는 것만으로 열리지 않고 **행을 눌러야** 오른쪽에 자식 목록이 열린다.
//    다른 하위 메뉴 행을 누르면 자식이 바뀌고 부모는 유지된다.
//  - 일반 행은 **누르는 즉시** 명령이 실행되고 `openSubGump.wav` 가 난다(돌 버튼과 달리 뗄 때까지 기다리지 않는다). 실행하면 열려 있던 펼침 메뉴가 모두 닫힌다.
//  - 메뉴 밖을 누르면 메뉴만 닫히고 그 누름은 다른 곳에 전달되지 않는다. 구분선·메뉴 가장자리는 아무 일도 하지 않는다.
// 위치: 최상위 목록은 호출자가 준 기준점(open 의 point)에 왼쪽 위 모서리를 두고 화면 밖으로 나가지 않게 당긴다(원본 Attach 의 화면 안 보정 Limit 과 같은 방식).
// 메인 메뉴 버튼의 기준점 규칙(버튼 가운데 x, 버튼 위쪽 + popupDy)은 ui/main-menu.js 에 있고 원본 스크린샷 측정값(Help (630,311), Options (551,294))과 일치한다.
// 자식 목록은 부모 오른쪽 가장자리에 부모 행의 위쪽을 맞춘다(원본 스크린샷에서 확인). 자세한 근거는 docs/webpj-popup-menus-20261029.md.
// (2026-10-29 WB03-F: Help·Options 펼침 메뉴 복원)
import { MenuList } from './menu-list.js';

// 한 단계의 펼침 목록이다. list 는 MenuList, spec 은 이 목록을 만든 항목 정의 배열, parentRow 는 부모 목록에서 이 목록을 연 행 번호(최상위는 -1)다. (2026-10-29 WB03-F)

// 펼침 메뉴 모음이다. 항목 정의 배열로 열고(open), 포인터 입력을 pointerMove/pointerDown 으로 받으며, draw 로 모든 단계를 아래에서 위로 그린다.
// 항목 정의: { kind: 'text', label, command, enabled? } | { kind: 'check', label, checked, command } | { kind: 'submenu', label, items } | { kind: 'separator' }.
// 명령은 { name, argument } 객체이며 실행될 때 onCommand(명령) 이 불린다. 실행 전에 모든 펼침 메뉴가 먼저 닫힌다.
// 사용: const popup = new PopupMenu({ resources, screen, onCommand, onSound }); popup.open(items, { x, y }); popup.pointerDown(x, y); popup.draw(surface).
// (2026-10-29 WB03-F)
export class PopupMenu {
    // resources 는 메인 메뉴 자원 묶음(fonts·colors·texture·marker 등), screen 은 논리 화면 크기 { width, height } 이다.
    constructor({ resources, screen, onCommand = null, onSound = null }) {
        // 그리기·측정 자원 묶음이다.
        this.resources = resources;
        // 논리 화면 크기이다. 펼침 목록이 화면 밖으로 나가지 않게 당기는 기준이다.
        this.screen = screen;
        // 명령 실행 콜백이다.
        this.onCommand = onCommand;
        // 효과음 콜백이다. 행을 누를 때 'menu' 를 넘긴다.
        this.onSound = onSound;
        // 열려 있는 단계 목록이다(첫 번째가 최상위). 비어 있으면 닫힌 상태이다.
        this.levels = [];
    }

    // 하나라도 열려 있는지 여부이다.
    get isOpen() {
        return this.levels.length > 0;
    }

    // 항목 정의 배열로 목록 하나를 만든다. 구분선이 맨 앞·연속이면 MenuList 가 넣지 않으므로, 목록 항목마다 만든 정의(spec)를 붙여 둔다. (2026-10-29 WB03-F)
    _build(items) {
        const list = new MenuList({ fonts: this.resources.fonts, colors: this.resources.colors, raised: true });
        // 정의 순서대로 항목을 추가하고, 실제로 추가된 항목에 정의를 달아 둔다.
        for (const spec of items) {
            const before = list.items.length;
            if (spec.kind === 'separator') list.addSeparator();
            else if (spec.kind === 'submenu') list.addSubmenu(spec.label, spec.items);
            else if (spec.kind === 'check') list.addChecked(spec.label, spec.command, spec.checked, spec.enabled !== false);
            else list.addText(spec.label, spec.command, spec.enabled !== false);
            if (list.items.length > before) list.items[list.items.length - 1].spec = spec;
        }
        return list;
    }

    // 목록의 왼쪽 위 모서리를 (x, y) 로 두되 화면 밖으로 나가지 않게 당긴다(0 이상, 화면 크기 - 목록 크기 이하). 크기가 화면보다 크면 0 이다. (2026-10-29 WB03-F)
    _place(list, x, y) {
        const maxX = this.screen.width - list.baseWidth;
        const maxY = this.screen.height - list.baseHeight;
        list.x = Math.fround(Math.max(0, Math.min(x, maxX)));
        list.y = Math.fround(Math.max(0, Math.min(y, maxY)));
    }

    // 최상위 펼침 메뉴를 연다. 이미 열려 있던 것은 닫는다. point 는 왼쪽 위 모서리의 바람직한 위치(논리 좌표)이다. (2026-10-29 WB03-F)
    open(items, point) {
        this.close();
        const list = this._build(items);
        this._place(list, point.x, point.y);
        this.levels.push({ list, spec: items, parentRow: -1 });
    }

    // 모든 단계를 닫는다. (2026-10-29 WB03-F)
    close() {
        this.levels = [];
    }

    // 화면 크기가 바뀌면 위치 기준이 사라지므로 닫는다(원본도 화면 모드가 바뀌면 열린 메뉴를 거둔다). (2026-10-29 WB03-F)
    resize(screen) {
        this.screen = screen;
        this.close();
    }

    // (x, y) 를 포함하는 가장 위(나중에 연) 단계 번호를 돌려준다. 없으면 -1. 포함은 정수 경계의 [왼쪽, 오른쪽) × [위, 아래) 이다. (2026-10-29 WB03-F)
    _levelAt(x, y) {
        for (let index = this.levels.length - 1; index >= 0; index -= 1) {
            const bounds = this.levels[index].list.bounds();
            if (x >= bounds.left && x < bounds.right && y >= bounds.top && y < bounds.bottom) return index;
        }
        return -1;
    }

    // 포인터 이동에 맞춰 강조 행을 갱신한다. 포인터가 있는 가장 위 단계만 강조하고 나머지 단계의 강조는 지운다. 그림이 바뀌면 true. (2026-10-29 WB03-F)
    pointerMove(x, y) {
        const hit = this._levelAt(x, y);
        let changed = false;
        // 모든 단계를 훑어 포인터 아래 단계만 강조를 갱신한다.
        this.levels.forEach((level, index) => {
            if (index === hit) {
                if (level.list.pointerMove(x, y)) changed = true;
            } else if (level.list.pointerLeave()) {
                changed = true;
            }
        });
        return changed;
    }

    // 포인터가 Canvas 밖으로 나가거나 포커스를 잃었을 때 모든 강조를 지운다. 그림이 바뀌면 true. (2026-10-29 WB03-F)
    pointerLeave() {
        let changed = false;
        for (const level of this.levels) if (level.list.pointerLeave()) changed = true;
        return changed;
    }

    // 왼쪽 버튼 누름을 처리한다. 항상 입력을 소비하므로 true 이다.
    //  - 메뉴 밖: 모두 닫는다. 구분선·비활성 행: 아무 일도 없다.
    //  - 하위 메뉴 행: 효과음 + 자식 목록을 부모 오른쪽에 연다(이미 열린 더 깊은 단계는 닫고 교체).
    //  - 일반 행: 효과음 + 모두 닫은 뒤 명령 실행(누르는 즉시).
    pointerDown(x, y) {
        const index = this._levelAt(x, y);
        if (index < 0) {
            this.close();
            return true;
        }
        const list = this.levels[index].list;
        list.pointerMove(x, y);
        const row = list.selection;
        if (row < 0) return true;
        const item = list.items[row];
        if (item.kind === 'separator') return true;
        this.onSound?.('menu');
        if (item.kind === 'submenu') {
            this._openChild(index, row, item);
            return true;
        }
        const command = item.command;
        this.close();
        if (command) this.onCommand?.(command);
        return true;
    }

    // 하위 메뉴 행의 자식 목록을 연다. 부모 오른쪽 가장자리에 부모 행의 위쪽을 맞추고(원본 관측: Music Volume 행 위쪽에 Volume 목록이 붙음) 화면 안으로 당긴다. (2026-10-29 WB03-F)
    _openChild(levelIndex, row, item) {
        this.levels.length = levelIndex + 1;
        const parent = this.levels[levelIndex].list;
        const bounds = parent.bounds();
        const child = this._build(item.child);
        this._place(child, bounds.right, bounds.top + parent.offsets[row]);
        this.levels.push({ list: child, spec: item.child, parentRow: row });
    }

    // 열려 있는 모든 단계를 아래(부모)에서 위(자식) 순서로 그린다. (2026-10-29 WB03-F)
    draw(surface) {
        for (const level of this.levels) level.list.draw(surface, this.resources);
    }
}
