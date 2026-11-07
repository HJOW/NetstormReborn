// PATCH 10.78 Chatmanager 의 실제 100개 기록(이름 40 BYTE·BYTE 플래그·DWORD subscriber)과 채널 플래그 변경을 같은 저장소 위에서 실행하는 모듈이다(cpppj `client/RawChatChannels.cpp/.h`, 00444ee0/00445080/00445140).
// 저장소: records 는 100×48(0x30) BYTE 이며 복사하지 않고 빌린다. 한 기록은 이름 40/플래그 1(+0x28)/미사용 3/subscriber 4(+0x2c)이다.
//  - add(00444ee0): 현재 자기 subscriber 와 같으면 이름·효과를 읽기 전에 -1. -1 subscriber 는 기존 -1 을 갱신하지 않고 첫 빈 기록(0)을 예약하며 플래그 bit 0x10 을 제거한다. 그 밖은 첫 기존 기록,
//    없으면 첫 빈 기록을 쓴다. 가득 차면 -1. 새 기록은 subscriber 를 쓰고 이름(NULL 이면 `Unknown%08x`)을 40 BYTE 로 채우며 기존 기록은 이름 첫 BYTE 를 0 으로 쓴 뒤 이름이 있으면 덮고 플래그를 OR 한다.
//    마지막에 +0x27=0, 플래그 쓰기, 이름 정제(0042b0e0), 빈 몸체 갱신(004437f0) 두 번 순이다.
//  - clear(마스크): subscriber 가 비제로인 기록의 플래그에서 하위 BYTE 마스크를 지운다(이름/효과는 그대로). resetOccupied(00445140): 점유 기록의 48 BYTE 전체를 0 으로 쓴다.
// 이름은 원본 바이트 그대로의 이진 문자열(문자 하나 = 1 BYTE)이며 null/undefined 는 NULL 이름이다. 이 모듈은 DOM 과 UI 를 쓰지 않는다. CD 10.72 는 대상이 아니다.
// (2026-11-08 WB04-E6: Chat 채널 기록 이식으로 추가)

// 하위 몸체 진입을 검사에서 관찰하는 호출 종류다. 게임 계산이나 이름 정제를 대체하지 않는다. (2026-11-08 WB04-E6)
export const CHAT_CHANNEL_CALL = Object.freeze({ nameChanged: 'NameChanged', refresh: 'Refresh' });

// 기록 개수·한 기록의 폭·저장소 전체 폭(원본 00558ed0..0055a18f)이다. (2026-11-08 WB04-E6)
export const CHAT_RECORD_COUNT = 100;
export const CHAT_RECORD_STRIDE = 0x30;
export const CHAT_RECORD_BYTES = CHAT_RECORD_COUNT * CHAT_RECORD_STRIDE;

// 이름 물리 폭·BYTE 플래그 위치·DWORD subscriber 위치이다. (2026-11-08 WB04-E6)
const NAME_BYTES = 40;
const FLAGS = 0x28;
const SUBSCRIBER = 0x2c;

// 0042b0e0 이 첫 두 BYTE 를 고친 뒤 strchr 에 전달하는 금지 문자 표(0xa0 · % . ? = ( ) [ ] ~ " ` ; : 공백)이다. (2026-11-08 WB04-E6)
const FORBIDDEN = '\xa0%.?=()[]~"`;: ';

// 채팅 기록 100개를 빌려 실제 Add/Clear/Reset 효과를 쓴다. 사용: new RawChatChannels(records(4800 BYTE), () => 현재 자기 subscriber, 관찰 콜백(선택)). (2026-11-08 WB04-E6)
export class RawChatChannels {
    #records;
    #mySubscriber;
    #observe;

    // 저장소 크기와 현재 subscriber 조회가 없으면 어떤 기록도 건드리기 전에 거부한다. 생성자는 기록을 초기화하지 않는다.
    constructor(records, mySubscriber, observe = null) {
        if (!(records instanceof Uint8Array) || records.length !== CHAT_RECORD_BYTES || typeof mySubscriber !== 'function') {
            throw new TypeError('Chat 채널의 실제 100기록/현재 subscriber 조회 누락');
        }
        this.#records = records;
        this.#mySubscriber = mySubscriber;
        this.#observe = observe;
    }

    // 정렬되지 않은 기록 +0x2c 의 DWORD subscriber 를 낮은 BYTE 부터 읽는다.
    #subscriber(index) {
        const base = index * CHAT_RECORD_STRIDE + SUBSCRIBER;
        const raw = this.#records;
        return (raw[base] | (raw[base + 1] << 8) | (raw[base + 2] << 16) | (raw[base + 3] << 24)) >>> 0;
    }

    // 물리 이름 40 BYTE 를 모두 쓰되 입력의 첫 NUL 뒤는 읽지 않고 0 으로 채운다(정확히 40 글자 입력도 보존).
    #copyName(record, name) {
        let ended = false;
        // strncpy 의 0 보충과 40 글자 입력을 한 반복에서 처리한다.
        for (let index = 0; index < NAME_BYTES; index += 1) {
            if (index >= name.length || (!ended && name.charCodeAt(index) === 0)) ended = true;
            record[index] = ended ? 0 : name.charCodeAt(index) & 0xff;
        }
    }

    // 0042b0e0: 금지 문자를 지우고(같은 위치를 다시 검사) NUL 뒤의 stale BYTE 는 보존하며 빈 이름이면 `Cheat<subscriber 하위BYTE>` 를 쓴다.
    #nameChanged(subscriber, record) {
        this.#observe?.(CHAT_CHANNEL_CALL.nameChanged, subscriber);
        let length = 0;
        // Add 가 +0x27 에 NUL 을 쓴 뒤의 실제 문자열 길이만 읽는다.
        while (length < NAME_BYTES && record[length]) length += 1;
        let index = 0;
        // 금지 문자를 지우면 같은 index 를 다시 검사한다. 선행/후행 공백도 원본처럼 전부 지운다.
        while (index < length) {
            if (!FORBIDDEN.includes(String.fromCharCode(record[index]))) {
                index += 1;
                continue;
            }
            // 현재 NUL 까지 왼쪽으로 한 칸 옮긴다(문자열 밖 stale 값은 보존).
            for (let next = index; next < length; next += 1) record[next] = record[next + 1];
            length -= 1;
        }
        record[length] = 0;
        if (length === 0) {
            const replacement = `Cheat${subscriber & 0xff}`;
            // 대체 이름을 앞에서부터 쓰고 NUL 로 닫는다.
            for (let position = 0; position < replacement.length; position += 1) record[position] = replacement.charCodeAt(position);
            record[replacement.length] = 0;
        }
    }

    // 004437f0 은 단일 ret 이다. 관찰 콜백이 있을 때만 호출 위치를 알린다.
    #refresh() {
        this.#observe?.(CHAT_CHANNEL_CALL.refresh, 0);
    }

    // 00444ee0: 반환값은 선택한 기록 번호이며 자기 subscriber/가득 찬 신규는 -1 이다. name 이 null/undefined 이면 NULL 이름이고 flags 는 하위 BYTE 만 쓴다.
    add(subscriber, name, flags) {
        const target = subscriber >>> 0;
        if (target === (this.#mySubscriber() >>> 0)) return -1;
        let empty = -1;
        let existing = -1;
        let incoming = flags & 0xff;
        if (target === 0xffffffff) {
            // -1 은 기존 -1 을 갱신하지 않고 첫 0 기록을 예약하며 bit 0x10 을 항상 제거한다.
            incoming &= 0xef;
            // 첫 빈 subscriber 만 선택한다. 모두 차 있으면 기존 -1 이 있어도 실패한다.
            for (let index = 0; index < CHAT_RECORD_COUNT; index += 1) {
                if (this.#subscriber(index) === 0) {
                    empty = index;
                    break;
                }
            }
        } else {
            // 빈칸을 만난 뒤에도 같은 subscriber 를 끝까지 찾으며 첫 기존 기록에서 중단한다.
            for (let index = 0; index < CHAT_RECORD_COUNT; index += 1) {
                const current = this.#subscriber(index);
                if (current === target) {
                    existing = index;
                    break;
                }
                if (current === 0 && empty === -1) empty = index;
            }
        }
        const chosen = existing !== -1 ? existing : empty;
        if (chosen === -1) return -1;
        const record = this.#records.subarray(chosen * CHAT_RECORD_STRIDE, (chosen + 1) * CHAT_RECORD_STRIDE);
        if (existing === -1) {
            // 새 기록에서만 subscriber DWORD 를 저장한다. 주변 stale 값은 초기화하지 않는다.
            for (let byte = 0; byte < 4; byte += 1) record[SUBSCRIBER + byte] = (target >>> (8 * byte)) & 0xff;
            if (name !== null && name !== undefined) this.#copyName(record, name);
            else {
                // Unknown 7글자 + DWORD hex 8글자 + NUL 의 원본 sprintf 결과를 앞 16 BYTE 에 쓴다.
                const unknown = `Unknown${target.toString(16).padStart(8, '0')}`;
                for (let position = 0; position < unknown.length; position += 1) record[position] = unknown.charCodeAt(position);
                record[unknown.length] = 0;
            }
        } else {
            record[0] = 0;
            if (name !== null && name !== undefined) this.#copyName(record, name);
            incoming |= record[FLAGS];
        }
        record[0x27] = 0;
        record[FLAGS] = incoming & 0xff;
        this.#nameChanged(target, record);
        this.#refresh();
        this.#refresh();
        return chosen;
    }

    // AI 의 Add(sub, name, 0, 0, -1, channel, -1, 0) 호출에서 읽히는 인자 1/2/6 만 같은 add 로 전달한다.
    addToChannel(subscriber, name, channel) {
        return this.add(subscriber, name, channel);
    }

    // 원본 Clear: subscriber 가 비제로인 기록에만 flags &= ~BYTE(mask) 를 쓰며 이름/효과는 건드리지 않는다(0 식별자 기록의 stale 플래그는 유지).
    clear(mask) {
        const keep = ~(mask & 0xff) & 0xff;
        // 모든 100 기록을 방문하고 현재 subscriber 가 비제로일 때만 한 BYTE 를 쓴다.
        for (let index = 0; index < CHAT_RECORD_COUNT; index += 1) {
            if (this.#subscriber(index) !== 0) this.#records[index * CHAT_RECORD_STRIDE + FLAGS] &= keep;
        }
    }

    // 00445140: subscriber 비제로인 기록의 이름/플래그/미사용/ID 48 BYTE 를 전부 0 으로 반납한다. 빈 기록의 stale 값은 유지한다.
    resetOccupied() {
        // 100 기록을 앞에서부터 순회하며 각 기록의 현재 DWORD 를 읽는다.
        for (let index = 0; index < CHAT_RECORD_COUNT; index += 1) {
            if (this.#subscriber(index) !== 0) this.#records.fill(0, index * CHAT_RECORD_STRIDE, (index + 1) * CHAT_RECORD_STRIDE);
        }
    }
}
