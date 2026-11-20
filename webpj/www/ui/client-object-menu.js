// 실제 Session의 공통 객체·회수 메뉴를 현재 Client 작성기에 연결한다.
import { formatPlayerMenuText } from '../core/raw-player-menu.js';

// Player 하위 메뉴와 안정된 정적 문자열 토큰은 호출자가 제공한다.
export class ClientObjectMenu {
    // 현재 Session이 소유하는 실제 객체 메뉴다.
    #menu;
    // 파생 객체 메뉴 본체이다. 세션이 파생 메뉴를 만들 수 없으면(구버전 세션 대역) null 이다. (2026-11-20 WB04-E1127)
    #variants = null;
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
        // 항목은 포함·활성 인자를 그대로 넘기고, 머리말·구분선·공통 메뉴 작성은 같은 작성기와 위의 메뉴를 쓴다.
        if (typeof session.makeObjectMenuVariants === 'function' && typeof writer.header === 'function') {
            this.#variants = session.makeObjectMenuVariants({
                translate: devices.translate, helpWord: devices.helpWord, format: (pattern, value) => formatPlayerMenuText(pattern, [value]),
                item: (text, command, value, include, enabled) => writer.entry(text, command, typeof value === 'number' ? value : devices.staticText(value), include !== 0, enabled !== 0, false),
                header: (text) => writer.header(text), separator: () => writer.separator(),
                object: (sid) => this.#menu.open(sid), damageable: (sid) => this.#menu.openDamageable(sid), salvage: (sid, type, count) => this.#menu.addSalvage(sid, type, count) });
        }
    }
    // 공통 도움말·소유자 Player 항목을 현재 메뉴에 쓴다.
    open(sid) { return this.#menu.open(sid); }
    // 신전·배터리·발전기 같은 파생 객체의 본체(`populate` 주소)에 맞는 파생 메뉴를 쓴다. 파생 장치가 없으면 던진다. (2026-11-20 WB04-E1127)
    openVariant(sid, populate) { if (!this.#variants) throw new Error('파생 객체 메뉴의 실제 Session 장치 미연결'); return this.#variants.open(sid, populate); }
    // 피해 가능 객체의 회수 항목을 먼저 쓴 뒤 공통 항목을 추가한다.
    openDamageable(sid) { return this.#menu.openDamageable(sid); }
}
