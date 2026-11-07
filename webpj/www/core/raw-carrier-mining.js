// CarrierArrival 의 채취 통지(429800 안의 40ac20 6 BYTE 작성 → 4e2e30 현재 연결 표 송신 → 4266c0 같은 지면 로컬 수신)를 옮긴 모듈이다(cpppj `o/RawCarrierMining.cpp/.h`, 10.78 전용).
// 현재 헤더(protocol bit7 를 지운 값, message, length 하위·상위 BYTE)와 지면 SID(하위·상위 BYTE) 6 BYTE 를 한 번 작성하고, 같은 버퍼를 송신 뒤 로컬 수신에 그대로 전달한다(송신 효과가 헤더 전역을 바꾸어도 버퍼는 보존).
// 길이를 4 이상으로 보정하지 않으며 지면은 미리 검사하지 않는다(수신이 송신 효과가 바꾼 현재 지면을 다시 읽음). 전체 도착의 채취 조건/너겟 생성은 RawCarrierArrival 이 맡는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E14: 캐리어 채취 통지 이식으로 추가)

// 선택된 경계의 누락을 성공이나 빈 처리로 바꾸지 않는다. (2026-11-08 WB04-E14)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('채취 통지 실제 하위 미연결');
    return hook;
}

// pool 은 같은 월드의 SID 풀, header 는 현재 메시지/프로토콜/선언 길이를 읽는 살아 있는 객체({ message, protocol, length }), hooks = { broadcast(bytes), receive(bytes) }. (2026-11-08 WB04-E14)
export class RawCarrierMining {
    #pool;
    #header;
    #hooks;

    // 다른 판본의 상위 도착 몸체를 같은 것으로 가정하지 않는다.
    constructor(pool, header, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('채취 통지 판본 오류');
        this.#pool = pool;
        this.#header = header;
        this.#hooks = hooks;
    }

    // 현재 헤더를 한 번 작성한 뒤 송신 효과가 헤더 전역을 바꾸어도 로컬 버퍼는 보존한다.
    notify(geyser) {
        const header = this.#header;
        const packet = Uint8Array.of(header.protocol & 0x7f, header.message & 0xff, header.length & 0xff, (header.length >> 8) & 0xff, geyser & 0xff, (geyser >> 8) & 0xff);
        need(this.#hooks.broadcast)(packet);
        need(this.#hooks.receive)(packet);
    }

    // 실제 수신/세션의 같은 저장소를 확인할 때 쓰는 풀이다.
    get pool() { return this.#pool; }
}
