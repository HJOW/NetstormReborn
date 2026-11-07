// 애시드(145)의 11바이트 포획 명령 작성(40ad30)과 로컬/원격 수신(48bfe0)을 옮긴 모듈이다(cpppj `o/RawAcidZap.cpp/.h`, 10.78 전용).
//  - build(victim, first, second, orientation): 현재 등록 헤더를 호출마다 읽어 11바이트 패킷을 만든다: [protocol&0x7f, message, 길이 WORD, 대상 WORD, 첫 폴 WORD, 둘째 폴 WORD, 방향 signed BYTE].
//    선언 길이는 보정하지 않고 그대로 기록한다(원본 헤더 선언 길이는 반환 크기와 별개).
//  - receive(owner, flags, packet): 원본은 송신 owner/flags 를 쓰지 않는다. 대상이 free/dead(상태 비트 1·2)이거나 첫 폴이 free 이거나 대상 TYPE 이 제단/성장 중 아이스 타워이거나 사제 genus(0x200000)이면 끝이다
//    (둘째 폴/동맹/boss/editor 는 검사하지 않음). 아니면 소리(좌표/dissolve.wav/반복 0) → 분해 생성 → 현재 가상 삭제(원 소유자 << 16 | 0x400048) 순서이며 효과 뒤의 패킷/슬롯 변화는 처음 읽은 표적과 첫 폴로 유지한다.
// 훅 { sound(x, y, 이름, 반복), dissolve(대상, 첫 폴, 둘째 폴, 방향), destroy(대상, 플래그) }는 모두 필요하다. 전역 { altar(164), growingIceTower(150) }는 수신 시점마다 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E48: RawAcidZap 이식으로 추가)

// 슬롯 필드 위치(상태·타입·좌표·소유자)와 타입 수, 패킷 길이이다. (2026-11-08 WB04-E48)
const STATE_OFFSET = 11;
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const TYPE_COUNT = 188;
const PACKET_SIZE = 11;

// 사제 제외 genus 와 원본 포획 삭제 플래그, 분해 소리 이름이다. (2026-11-08 WB04-E48)
const PRIEST_GENUS = 0x200000;
const DESTROY_FLAGS = 0x400048;
const SOUND_NAME = 'dissolve.wav';

// 정렬되지 않은 필드 읽기와 float 읽기이다. (2026-11-08 WB04-E48)
const floatView = new DataView(new ArrayBuffer(4));
function read(raw, offset, width = 4) {
    let value = 0;
    // 필드 폭의 바이트만 낮은 쪽부터 합친다(주변 raw 는 읽지 않는다).
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}
function floatOf(raw, offset) {
    floatView.setUint32(0, read(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// 애시드 포획 실행기이다. 사용: new RawAcidZap(pool, types, { altar, growingIceTower }, { message, protocol, length }, { sound, dissolve, destroy }). 상태/헤더는 현재 객체를 그대로 읽는다.
// (2026-11-08 WB04-E48)
export class RawAcidZap {
    #pool;
    #types;
    #state;
    #header;
    #hooks;

    // 효과 경계 누락은 소리나 메모리 생성 전에 확인한다.
    constructor(pool, types, state, header, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== TYPE_COUNT || typeof hooks?.sound !== 'function' || typeof hooks.dissolve !== 'function' || typeof hooks.destroy !== 'function') {
            throw new TypeError('애시드 수신 판본/실제 하위 경계 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#header = header;
        this.#hooks = hooks;
    }

    // wire 는 protocol/message 순서이며 등록 표의 message/protocol 저장 순서와 구분한다. 길이는 4 미만 보정 없이 선언값 그대로이다.
    build(victim, first, second, orientation) {
        const header = this.#header;
        const packet = new Uint8Array(PACKET_SIZE);
        packet[0] = header.protocol & 0x7f;
        packet[1] = header.message & 0xff;
        packet[2] = header.length & 0xff;
        packet[3] = (header.length >>> 8) & 0xff;
        packet[4] = victim & 0xff;
        packet[5] = (victim >>> 8) & 0xff;
        packet[6] = first & 0xff;
        packet[7] = (first >>> 8) & 0xff;
        packet[8] = second & 0xff;
        packet[9] = (second >>> 8) & 0xff;
        packet[10] = orientation & 0xff;
        return packet;
    }

    // 원본 수신은 둘째 폴/동맹/boss/editor 를 별도 검사하지 않는다. owner/flags 는 읽지 않는다.
    receive(owner, flags, packet) {
        void owner;
        void flags;
        if (packet.length < PACKET_SIZE) throw new RangeError('애시드 포획 명령 길이 오류');
        const pool = this.#pool;
        const victim = read(packet, 4, 2);
        const first = read(packet, 6, 2);
        const raw = pool.slot(victim);
        if ((raw[STATE_OFFSET] & 3) || (pool.slot(first)[STATE_OFFSET] & 1) || raw[TYPE_OFFSET] === this.#state.altar || raw[TYPE_OFFSET] === this.#state.growingIceTower) return;
        if (raw[TYPE_OFFSET] >= this.#types.length) throw new RangeError('애시드 대상 TYPE 범위 오류');
        if (this.#types[raw[TYPE_OFFSET]].flags2 & PRIEST_GENUS) return;
        const hooks = this.#hooks;
        hooks.sound(floatOf(raw, X_OFFSET), floatOf(raw, Y_OFFSET), SOUND_NAME, 0);
        // 효과 뒤 패킷이 바뀌어도 소리 뒤 현재 패킷을 다시 읽어 분해에 전달한다(원문 인자 평가 순서).
        hooks.dissolve(read(packet, 4, 2), read(packet, 6, 2), read(packet, 8, 2), (packet[10] << 24) >> 24);
        hooks.destroy(victim, (((pool.slot(first)[OWNER_OFFSET] << 16) | DESTROY_FLAGS) >>> 0));
    }
}
