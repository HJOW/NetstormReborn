// 현재 Session의 Player 하위 메뉴를 같은 Client 메뉴 스택에 연결한다.
import { formatPlayerMenuText } from '../core/raw-player-menu.js';

// 통신·채팅 경계는 실제 Client가 제공하고 Player·동맹·SP는 Session에서 조회한다.
export class ClientPlayerMenu {
    // 반환된 메뉴와 두 정적 문자열 토큰의 수명을 유지한다.
    #menu;
    constructor(session, identity, writer, devices) {
        if (!session?.makePlayerMenu || !writer || !devices ||
            ['beginFancy', 'beginVertical', 'separator', 'title', 'header', 'takeCurrent', 'submenu', 'entry', 'checked'].some((name) => typeof writer[name] !== 'function') ||
            ['translate', 'assertion', 'parseAddress', 'reliability', 'ignored', 'whispered'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('Player 메뉴의 실제 Session·Client 장치 누락');
        this.#menu = session.makePlayerMenu(identity, {
            assertion: devices.assertion, parseAddress: devices.parseAddress, reliability: devices.reliability, ignored: devices.ignored, whispered: devices.whispered,
            allowSend: () => session.battleOption(13), allowOffer: () => session.battleOption(11), allowBreak: () => session.battleOption(12),
            translate: devices.translate, print: formatPlayerMenuText,
            beginFancy: () => writer.beginFancy(), beginVertical: () => writer.beginVertical(), separator: () => writer.separator(),
            title: (value) => writer.title(value), header: (value) => writer.header(value), takeMenu: () => writer.takeCurrent(),
            submenu: (text, child) => writer.submenu(text, child),
            item: (text, command, parameter, include, enabled) => writer.entry(text, command, parameter, Boolean(include), Boolean(enabled), false),
            checked: (checked, text, command, parameter) => writer.checked(Boolean(checked), text, command, parameter),
        });
    }
    // 대상 subscriber의 현재 Player 정보를 하위 메뉴로 작성해 토큰을 돌려준다.
    open(subscriber) { return this.#menu.open(subscriber); }
    // 원본 정적 asksend 토큰은 같은 메뉴의 현재 문자열로 조회한다.
    textParameter(token) { return this.#menu.textParameter(token); }
}
