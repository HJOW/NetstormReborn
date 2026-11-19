// 현재 Session의 사제 건설 메뉴를 실제 Client 메뉴 작성기와 번역에 결합한다.
import { formatPlayerMenuText } from '../core/raw-player-menu.js';

// 같은 Session·메뉴 스택을 빌리며 공통 객체 메뉴와 안내는 상위 소유자가 제공한다.
export class ClientPriestMenu {
    // 원본 사제 메뉴는 현재 자금·기술·커서와 전달된 실제 UI 작성기를 사용한다.
    #menu;
    constructor(session, writer, devices) {
        if (!session?.makePriestMenu || !session?.core || !writer || !devices ||
            ['beginFancy', 'beginVertical', 'title', 'takeCurrent', 'submenu', 'entry', 'separator'].some((name) => typeof writer[name] !== 'function') ||
            ['translate', 'baseMenu', 'tell'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('사제 메뉴의 실제 Session·작성 장치 누락');
        this.#menu = session.makePriestMenu({
            description: (type) => devices.translate(session.core().types[type].description),
            format: (pattern, values) => formatPlayerMenuText(devices.translate(pattern), values),
            beginFancy: () => writer.beginFancy(), beginVertical: () => writer.beginVertical(), title: (text) => writer.title(text),
            takeMenu: () => writer.takeCurrent(), submenu: (text, child, enabled) => writer.submenu(text, child, enabled),
            item: (text, command, parameter, enabled) => writer.entry(text, command, parameter, true, enabled, false),
            separator: () => writer.separator(), baseMenu: (sid) => devices.baseMenu(sid), tell: (name) => devices.tell(name),
        });
    }
    // 부모 객체 메뉴가 이미 시작한 현재 스택에 사제 건설·기도 항목을 작성한다.
    open(priest) { return this.#menu.open(priest); }
    // 실제 선택 관문이 기술·자금과 현재 커서를 검사하고 미리보기를 만든다.
    selectBuild(type, priest) { return this.#menu.selectBuild(type, priest); }
}
