// 실제 Session의 객체 메뉴 제목·상태와 현재 Client 표시 장치를 연결한다.
import { formatPlayerMenuText } from '../core/raw-player-menu.js';

// TYPE·소유자·상태는 같은 Session에서, 번역과 메뉴 쓰기는 실제 Client에서 빌린다.
export class ClientObjectMenuInfo {
    // 원본 제목·상태 작성기다.
    #info;
    constructor(session, game, identity, writer, data, devices) {
        if (!session?.makeObjectMenuInfo || !writer?.title || !writer?.header || !data?.substitute || !devices ||
            ['translate', 'ownerName'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('객체 정보의 실제 Session·표시 장치 누락');
        this.#info = session.makeObjectMenuInfo(game, identity, { description: (type) => devices.translate(session.core().types[type].description),
            ownerName: devices.ownerName, translate: devices.translate, substitute: (value) => data.substitute(value), print: formatPlayerMenuText,
            title: (value) => writer.title(value), header: (value) => writer.header(value) });
    }
    // 현재 객체의 실제 이름·소유 관계와 상태를 원본 순서로 쓴다.
    summary(sid) { this.#info.summary(sid); }
}
