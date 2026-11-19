// 실제 Session Factory의 생산·지식 메뉴를 현재 Client 메뉴 작성기에 결합한다.
import { formatPlayerMenuText } from '../core/raw-player-menu.js';

// 공통 객체 메뉴와 정적 도움말 토큰은 같은 Client 수명에서 빌린다.
export class ClientFactoryMenu {
    // 같은 Session의 원본 Factory 메뉴 작성기다.
    #menu;
    constructor(session, writer, objects, devices) {
        if (!session?.makeFactoryMenu || !writer || !objects?.openDamageable || !devices ||
            ['beginFancy', 'title', 'header', 'takeCurrent', 'submenu', 'entry'].some((name) => typeof writer[name] !== 'function') ||
            ['translate', 'helpWord', 'staticText'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('Factory 메뉴의 실제 Session·작성 장치 누락');
        this.#menu = session.makeFactoryMenu({ description: (type) => devices.translate(session.core().types[type].description),
            helpWord: devices.helpWord, format: (pattern, values) => formatPlayerMenuText(devices.translate(pattern), values),
            beginFancy: () => writer.beginFancy(), title: (value) => writer.title(value), header: (value) => writer.header(value),
            takeMenu: () => writer.takeCurrent(), submenu: (text, child) => writer.submenu(text, child),
            item: (text, command, value, include, enabled) => writer.entry(text, command, typeof value === 'number' ? value : devices.staticText(value), include, enabled, false),
            baseMenu: (sid) => objects.openDamageable(sid) });
    }
    // 실제 생산·지식 하위 메뉴 뒤에 공통 객체 메뉴를 작성한다.
    open(sid) { return this.#menu.open(sid); }
}
