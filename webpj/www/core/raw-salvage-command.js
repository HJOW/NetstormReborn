// 건물 회수(Salvage) 요청 44c420 · 일반 삭제 송신 4437c0 · 54h 수신 4436f0 의 순서를 옮긴 모듈이다(cpppj `o/RawSalvageCommand.cpp/.h`, 10.78 전용).
// 현재 미션/Player/제단 전역은 복제하지 않고 같은 저장소(state 객체의 현재 값)를 읽는다. 요청에서는 세계를 삭제하지 않고 송신만 하며, 실제 서버 수신에서 환불과 삭제를 실행한다.
//  - request(sid) [44c420]: 최초 SID 0 은 정상 무동작이다. 반복마다 SID 가 풀 범위 밖이면 RangeError, 상태 bit 0/1(free/dead)이면 끝, 진단(checking)이 켜져 있으면 제단·다이스 타입의 비용이 다를 때 assertion(0x295) 후 현재 값으로 계속한다.
//    슬롯이 제단 타입이면 daisAt(sid)의 다이스로 옮겨 같은 관문을 반복하고, 아니면 종류(70 미만이면 +18 WORD, 아니면 타입 BYTE)를 구해 로컬 소유이고 회수 금지(denySalvage)이며 너깃 종류가 아니면 tell('DenySalvage') 후 끝, 그 밖에는 send(sid, 0x200000)이다.
//  - send(sid, flags) [4437c0]: 8바이트 [protocol&0x7f, message, 길이 WORD, SID 하위 WORD, flags 상위 WORD]를 send(9, 패킷)로 보낸다(헤더 값은 호출마다 state.request 에서 읽음).
//  - receive(sender, packet) [4436f0]: 패킷이 8바이트 미만이면 TypeError. SID 가 0 이거나 풀 밖이거나 free 면 무시, 미션이 아니면 슬롯 소유자가 sender 와 같아야 하며, 서버 권한(boss)이 없으면 assertion(0x563) 뒤 권한을 다시 읽어 없으면 끝낸다.
//    권한 진단 이후의 패킷 flags WORD 를 한 번만 보관(<<16)해, 현재 타입이 70 이상이고 flags&0x200000 이면 refund(sid, 현재 소유자, true) 뒤 destroy(sid, 보관한 flags)이다(dead 는 추가로 거르지 않음).
// state = { localOwner, denySalvage, checking, mission, boss, altarType, daisType, nuggetType, request: { message, protocol, length } }(모두 호출 시점의 현재 값), hooks = { daisAt(sid), tell(문자열), assertion(줄), send(대상, 패킷), refund(sid, 소유자, 회수), destroy(sid, flags) } 이다.
// 도달한 실제 경계만 요구하고 누락은 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E35: 회수 명령 이식으로 추가)

// 원문 상수: 회수 비용 진단 줄, 서버 권한 진단 줄, 회수 flags, 내용물/건물 타입 경계, 선언된 타입 수이다. (2026-11-08 WB04-E35)
const COST_ASSERTION = 0x295;
const BOSS_ASSERTION = 0x563;
const SALVAGE_FLAGS = 0x200000;
const FIRST_ASSET_TYPE = 70;
const TYPE_COUNT = 188;

// 선택된 실제 하위가 비었으면 빈 성공으로 바꾸지 않는다. (2026-11-08 WB04-E35)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('실제 회수 명령 하위 미연결');
    return hook;
}

// 지정한 비정렬 WORD 를 인접 바이트와 섞지 않고 읽고 쓴다. (2026-11-08 WB04-E35)
const word = (raw, at) => raw[at] | (raw[at + 1] << 8);
function putWord(raw, at, value) {
    raw[at] = value & 0xff;
    raw[at + 1] = (value >>> 8) & 0xff;
}

// pool(10.78), types(188개 { cost }), state, hooks 이다. (2026-11-08 WB04-E35)
export class RawSalvageCommand {
    #pool;
    #types;
    #state;
    #hooks;

    // CD 판의 별도 수신/회수 본문을 10.78 과 같다고 추정하지 않는다.
    constructor(pool, types, state, hooks) {
        if (pool.edition !== 'patch1078' || types.length !== TYPE_COUNT) throw new TypeError('회수 명령의 10.78 풀/타입 계약 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 최초 SID 0 은 정상 무동작이며, 제단 조회 뒤에는 다음 SID 의 상태/타입을 새로 읽는다.
    request(start) {
        const state = this.#state;
        let number = start >>> 0;
        // 제단에서 실제 다이스로 옮겨 같은 원본 관문을 반복한다.
        while (number) {
            if (number >= this.#pool.capacity || number > 65535) throw new RangeError('회수 요청 SID 물리 범위');
            if ((this.#pool.slot(number)[11] & 3) !== 0) return;
            if (state.checking) {
                if (state.altarType >= this.#types.length || state.daisType >= this.#types.length) throw new RangeError('회수 비용 진단 TYPE 범위');
                if (this.#types[state.altarType].cost !== this.#types[state.daisType].cost) need(this.#hooks.assertion)(COST_ASSERTION);
            }
            if (this.#pool.slot(number)[10] === state.altarType) { number = need(this.#hooks.daisAt)(number) >>> 0; continue; }
            const raw = this.#pool.slot(number);
            // 내용물은 현재 클래스 BYTE 대신 실제 종류 WORD 를 비교한다.
            const kind = raw[10] < FIRST_ASSET_TYPE ? word(raw, 18) : raw[10];
            if (raw[34] === state.localOwner && state.denySalvage && kind !== state.nuggetType) { need(this.#hooks.tell)('DenySalvage'); return; }
            this.send(number, SALVAGE_FLAGS);
            return;
        }
    }

    // 전체 DWORD 입력의 낮은 SID WORD 와 높은 flags WORD 를 원문 그대로 전송한다.
    send(sid, flags) {
        const packet = new Uint8Array(8);
        packet[0] = this.#state.request.protocol & 0x7f;
        packet[1] = this.#state.request.message & 0xff;
        putWord(packet, 2, this.#state.request.length);
        putWord(packet, 4, sid & 0xffff);
        putWord(packet, 6, (flags >>> 16) & 0xffff);
        need(this.#hooks.send)(9, packet);
    }

    // dead 는 추가로 거르지 않는다. 진단 뒤 packet flags 를 읽고 환불 뒤에도 보관한 flags 를 유지한다.
    receive(sender, packet) {
        if (packet.length < 8) throw new TypeError('회수 54h 패킷 길이 오류');
        const sid = word(packet, 4);
        if (!sid || sid >= this.#pool.capacity || (this.#pool.slot(sid)[11] & 1) !== 0) return;
        if (!this.#state.mission && this.#pool.slot(sid)[34] !== sender) return;
        if (!this.#state.boss) {
            need(this.#hooks.assertion)(BOSS_ASSERTION);
            if (!this.#state.boss) return;
        }
        // 원본은 권한 진단 이후의 caller WORD 를 한 번만 보관한다.
        const flags = (word(packet, 6) << 16) >>> 0;
        if (this.#pool.slot(sid)[10] >= FIRST_ASSET_TYPE && (flags & SALVAGE_FLAGS) !== 0) need(this.#hooks.refund)(sid, this.#pool.slot(sid)[34], true);
        need(this.#hooks.destroy)(sid, flags);
    }
}
