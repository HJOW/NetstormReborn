// 실제 운반체 내용 메뉴와 공통 객체 메뉴를 현재 Client 메뉴 스택에 결합한다.
import { formatPlayerMenuText } from '../core/raw-player-menu.js';

// 주문·운반물 행의 조회와 활성 조건은 같은 Session이 계산한다.
export class ClientCarrierMenu {
    // 같은 Session의 원본 운반체 작성기다.
    #menu;
    constructor(session, writer, objects, translate) {
        if (!session?.makeCarrierMenu || !writer?.header || !writer?.entry || !objects?.openDamageable || typeof translate !== 'function') throw new TypeError('운반 메뉴의 실제 Session·작성 장치 누락');
        this.#menu = session.makeCarrierMenu({ description: (type) => translate(session.core().types[type].description),
            format: (pattern, values) => formatPlayerMenuText(translate(pattern), values), header: (value) => writer.header(value),
            item: (text, command, parameter, enabled) => writer.entry(text, command, parameter, true, enabled, false),
            damageableMenu: (sid) => objects.openDamageable(sid) });
    }
    // 현재 운반체 내용물 뒤에 실제 공통 객체 메뉴를 작성한다.
    open(sid) { return this.#menu.open(sid); }
}
