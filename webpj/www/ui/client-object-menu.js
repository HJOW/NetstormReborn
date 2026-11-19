// 실제 Session의 공통 객체·회수 메뉴를 현재 Client 작성기에 연결한다.
import { formatPlayerMenuText } from '../core/raw-player-menu.js';

// Player 하위 메뉴와 안정된 정적 문자열 토큰은 호출자가 제공한다.
export class ClientObjectMenu {
    // 현재 Session이 소유하는 실제 객체 메뉴다.
    #menu;
    constructor(session, identity, players, writer, data, devices) {
        if (!session?.makeObjectMenu || !writer || !data?.set || !devices ||
            ['itemCount', 'separator', 'entry', 'submenu'].some((name) => typeof writer[name] !== 'function') ||
            ['translate', 'helpWord', 'staticText'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('객체 메뉴의 실제 Session·작성 장치 누락');
        this.#menu = session.makeObjectMenu(identity, players, {
            helpWord: devices.helpWord, format: (pattern, values) => formatPlayerMenuText(devices.translate(pattern), values),
            itemCount: () => writer.itemCount(), separator: () => writer.separator(),
            item: (text, command, value) => writer.entry(text, command, typeof value === 'number' ? value : devices.staticText(value), true, true, false),
            submenu: (text, child) => writer.submenu(text, child), setConfig: (key, value) => data.set(key, value),
        });
    }
    // 공통 도움말·소유자 Player 항목을 현재 메뉴에 쓴다.
    open(sid) { return this.#menu.open(sid); }
    // 피해 가능 객체의 회수 항목을 먼저 쓴 뒤 공통 항목을 추가한다.
    openDamageable(sid) { return this.#menu.openDamageable(sid); }
}
