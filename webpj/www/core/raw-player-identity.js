// Player/Subscriber/이름/IP 조회(원문 004841c0)의 실제 60 BYTE 레코드와 검색 순서를 옮긴 모듈이다(cpppj `o/RawPlayerIdentity.cpp/.h`, PATCH 10.78).
//  - 레코드(60 BYTE): +0 owner DWORD, +4 subscriber DWORD, +8 이름 40 BYTE(+NUL), +48 주소 DWORD, +52 모드 DWORD. 이름 뒤 미작성 바이트와 마지막 DWORD 도 보존한다.
//  - initialize(00484190): owner·subscriber·이름 첫 BYTE·주소·모드만 초기화하며 전체 레코드를 지우지 않는다.
//  - resolve: ① 로컬 일치(owner/subscriber/이름/주소 중 하나만 맞아도 현재 로컬 정보로 덮어씀) → ② 전투/편집 모드면 Player 1..8 의 첫 OR 일치(Player 0 은 검색하지 않음) →
//    ③ metamap 모드면 활성 기록 40개를 subscriber 또는 이름으로 비교 → ④ 채팅 100개(전체 첫 일치, 활성 flag/subscriber 비영 필터 없음)의 순서이다.
//    반환 true 는 모든 필드가 완성됐다는 뜻이 아니며 원본의 단계별 부분 완료 조건을 유지한다. 채팅의 40 BYTE 이름 조회는 인접 주소 첫 BYTE 의 NUL 까지 읽는다.
//  - 네 getter(subscriberForOwner·ownerForSubscriber·addressForSubscriber·nameForSubscriber/nameForOwner): resolve 의 bool 을 별도 성공 관문으로 쓰지 않고 레코드 필드를 그대로 돌려주며,
//    이름 실패는 현재 unknownName(SOMEONE)이다.
// 문자열은 원본 바이트 그대로의 이진 문자열(각 문자 = 1 BYTE, UTF-8 변환 없음)이며 비교는 C locale 의 ASCII 대소문자만 접는다.
// 근거: cpppj 기준선 1f00a25a, 원본 관측 player-identity-x86.tsv(10.78 851행, tests/player-identity.test.mjs). CD 10.72 배치(172/296 폭)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E4: Player 식별자 이식으로 추가)

// 레코드 폭과 필드 위치: 이름(+8, 40 BYTE), 주소(+48), 모드(+52). 채팅 캡처의 40 BYTE 뒤 NUL 은 조회 레코드의 주소 첫 BYTE 이므로 NUL 까지 41 BYTE 를 읽는다. (2026-11-08 WB04-E4)
export const PLAYER_IDENTITY_RECORD_SIZE = 60;
const NAME = 8, NAME_BYTES = 40, ADDRESS = 48, MODE = 52, TERMINATED_NAME_BYTES = NAME_BYTES + 1;
// 채팅 기록 폭·수, metamap 기록 수, 10.78 Player/metamap 기록 폭이다. (2026-11-08 WB04-E4)
const CHAT_STRIDE = 48, CHAT_COUNT = 100, META_COUNT = 40, PLAYER_STRIDE = 180, META_STRIDE = 460;

// 도달한 현재 전역을 가짜 0/이름으로 대체하지 않는다. (2026-11-08 WB04-E4)
function required(function_, name) {
    if (typeof function_ !== 'function') throw new Error(`실제 Player 식별자 전역(${name}) 미연결`);
    return function_;
}

// 정렬되지 않은 DWORD 를 부호 변경 없이 읽는다. (2026-11-08 WB04-E4)
function read(bytes, offset) {
    if (offset + 4 > bytes.length) throw new RangeError('Player 식별자 필드 범위 오류');
    return ((bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0);
}

// 원본 DWORD 의 낮은 바이트부터 쓴다(현재 필드 밖의 표식 바이트는 보존). (2026-11-08 WB04-E4)
function put(bytes, offset, value) {
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// NUL 바이트 문자열을 이진 문자열로 읽는다. 용량 초과·NUL 없음은 호스트 예외이다. (2026-11-08 WB04-E4)
function text(bytes, offset, capacity) {
    if (offset + capacity > bytes.length) throw new RangeError('Player 식별자 문자열 범위 오류');
    let end = offset;
    while (end < offset + capacity && bytes[end] !== 0) end += 1;
    if (end === offset + capacity) throw new RangeError('Player 식별자 NUL 없는 문자열');
    return String.fromCharCode(...bytes.subarray(offset, end));
}

// 원본 strcpy 처럼 최대 40 BYTE 이름과 종결자만 쓴다. 40 BYTE 의 NUL 은 인접 주소 첫 BYTE 이며 기존 60 BYTE 물리 안이다. (2026-11-08 WB04-E4)
function setName(record, name) {
    const nul = name.indexOf('\0');
    const value = nul < 0 ? name : name.slice(0, nul);
    if (value.length > NAME_BYTES) throw new RangeError('Player 식별자 40바이트 이름 초과');
    for (let index = 0; index < value.length; index += 1) record[NAME + index] = value.charCodeAt(index) & 0xff;
    record[NAME + value.length] = 0;
}

// C locale 의 ASCII 대소문자만 접고 확장 바이트는 같은 값끼리만 일치한다. (2026-11-08 WB04-E4)
function equal(first, second) {
    if (first.length !== second.length) return false;
    const fold = (code) => (code >= 65 && code <= 90 ? code + 32 : code);
    for (let index = 0; index < first.length; index += 1) if (fold(first.charCodeAt(index)) !== fold(second.charCodeAt(index))) return false;
    return true;
}

// 원본 PlayerFinder 이다. reads: { localOwner(), subscriber(), address(), name() → 이진 문자열, battle(), editor(), metamap(), unknownName() }(각 현재 값을 호출 위치에서 조회).
// players 는 9×180 BYTE, meta 는 40×460 BYTE(미연결이면 빈 배열이며 metamap 모드 진입 시 거부), chat 은 100×48 BYTE 이다.
// 사용: const identity = new RawPlayerIdentity(players, meta, chat, reads); identity.nameForOwner(2). (2026-11-08 WB04-E4)
export class RawPlayerIdentity {
    #players;
    #meta;
    #chat;
    #reads;

    // 저장소 크기는 생성 시 확인하고 실제 레코드 검색/효과는 호출 때 실행한다.
    constructor(players, meta, chat, reads) {
        if (players.length !== 9 * PLAYER_STRIDE || (meta.length !== 0 && meta.length !== META_COUNT * META_STRIDE) || chat.length !== CHAT_COUNT * CHAT_STRIDE) {
            throw new RangeError('Player 식별자 공유 저장소 크기 오류');
        }
        this.#players = players;
        this.#meta = meta;
        this.#chat = chat;
        this.#reads = reads;
    }

    // 00484190: 원본이 쓴 필드만 초기화하며 이름의 뒤·마지막 DWORD 는 보존한다.
    static initialize(record, mode) {
        put(record, 0, 0);
        put(record, 4, 0);
        record[NAME] = 0;
        put(record, ADDRESS, 0);
        put(record, MODE, mode >>> 0);
    }

    // 현재 로컬 → battle/editor Player 1..8 → metamap 40 → 채팅 100 순서로 실제 레코드를 갱신한다.
    resolve(record) {
        const reads = this.#reads;
        const local = () => required(reads.localOwner, 'localOwner')();
        if ((read(record, 0) && read(record, 0) === local()) || (read(record, 4) && read(record, 4) === required(reads.subscriber, 'subscriber')()) ||
            (record[NAME] && equal(text(record, NAME, TERMINATED_NAME_BYTES), required(reads.name, 'name')())) ||
            (read(record, ADDRESS) && read(record, ADDRESS) === required(reads.address, 'address')())) {
            setName(record, required(reads.name, 'name')());
            put(record, ADDRESS, required(reads.address, 'address')());
            put(record, 4, required(reads.subscriber, 'subscriber')());
            put(record, 0, local());
            return true;
        }
        if (required(reads.battle, 'battle')() || required(reads.editor, 'editor')()) {
            // Player 0 은 검색하지 않고 실제 1..8 의 첫 OR 일치를 사용한다.
            for (let owner = 1; owner < 9; owner += 1) {
                const player = this.#players.subarray(owner * PLAYER_STRIDE, (owner + 1) * PLAYER_STRIDE);
                if ((read(record, 0) && read(record, 0) === owner) || (read(record, 4) && read(record, 4) === read(player, 0x2c)) ||
                    (record[NAME] && equal(text(record, NAME, TERMINATED_NAME_BYTES), text(player, 0xc, 32)))) {
                    put(record, 0, owner);
                    put(record, 4, read(player, 0x2c));
                    setName(record, text(player, 0xc, 32));
                    put(record, ADDRESS, 0);
                    break;
                }
            }
            if (read(record, 0) && read(record, 4) && record[NAME]) return true;
        }
        if (required(reads.metamap, 'metamap')()) {
            if (this.#meta.length === 0) throw new Error('실제 metamap 식별자 기록 미연결');
            // 실제 40개의 활성 기록만 subscriber 또는 이름으로 비교한다. owner 는 match 조건이 아니다.
            for (let index = 0; index < META_COUNT; index += 1) {
                const entry = this.#meta.subarray(index * META_STRIDE, (index + 1) * META_STRIDE);
                const active = (entry[0x64] & 1) !== 0;
                if (active && ((read(record, 4) && read(record, 4) === read(entry, 4)) || (record[NAME] && equal(text(record, NAME, TERMINATED_NAME_BYTES), text(entry, 0x26, 40))))) {
                    put(record, 0, read(entry, 0));
                    put(record, 4, read(entry, 4));
                    setName(record, text(entry, 0x26, 40));
                    put(record, ADDRESS, 0);
                    break;
                }
            }
        }
        if (read(record, 4) && record[NAME]) return true;
        // 채팅은 100개 전체의 첫 일치이며 활성 flag 나 subscriber 비영 필터를 추가하지 않는다.
        for (let index = 0; index < CHAT_COUNT; index += 1) {
            const entry = this.#chat.subarray(index * CHAT_STRIDE, (index + 1) * CHAT_STRIDE);
            if ((read(record, 4) && read(record, 4) === read(entry, 0x2c)) || (record[NAME] && equal(text(record, NAME, TERMINATED_NAME_BYTES), text(entry, 0, TERMINATED_NAME_BYTES)))) {
                put(record, 0, 0);
                put(record, 4, read(entry, 0x2c));
                setName(record, text(entry, 0, TERMINATED_NAME_BYTES));
                break;
            }
        }
        return Boolean(read(record, 4) && record[NAME]);
    }

    // 새 60 BYTE 레코드를 모드와 함께 초기화하는 작은 도우미이다.
    #fresh(mode) {
        const record = new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE);
        RawPlayerIdentity.initialize(record, mode);
        return record;
    }

    // 004844f0: owner 번호를 받아 실제 subscriber DWORD 를 반환한다(resolve 의 bool 은 관문이 아님).
    subscriberForOwner(owner) {
        const record = this.#fresh(2);
        put(record, 0, owner >>> 0);
        this.resolve(record);
        return read(record, 4);
    }

    // 00484550: subscriber 를 받아 owner 번호를 반환한다(부분 완료 결과도 그대로).
    ownerForSubscriber(subscriber) {
        const record = this.#fresh(2);
        put(record, 4, subscriber >>> 0);
        this.resolve(record);
        return read(record, 0);
    }

    // 00484520: subscriber 를 받아 주소 DWORD 를 반환한다.
    addressForSubscriber(subscriber) {
        const record = this.#fresh(2);
        put(record, 4, subscriber >>> 0);
        this.resolve(record);
        return read(record, ADDRESS);
    }

    // 00484410: subscriber 이름이며 조회 실패 시 현재 unknownName 이다.
    nameForSubscriber(subscriber) {
        const record = this.#fresh(0);
        put(record, 4, subscriber >>> 0);
        return this.resolve(record) ? text(record, NAME, TERMINATED_NAME_BYTES) : required(this.#reads.unknownName, 'unknownName')();
    }

    // 00484480: mode 0 의 owner 입력으로 같은 resolve 를 실행하고 실패 시 현재 SOMEONE 을 반환한다(부분 완료 레코드의 이름을 성공으로 승격하지 않음).
    nameForOwner(owner) {
        const record = this.#fresh(0);
        put(record, 0, owner >>> 0);
        return this.resolve(record) ? text(record, NAME, TERMINATED_NAME_BYTES) : required(this.#reads.unknownName, 'unknownName')();
    }

    // 실제 메뉴가 같은 기록/전역을 사용하도록 원본 참조를 빌려 준다(재복사하지 않음).
    get playerRecords() { return this.#players; }

    get metaRecords() { return this.#meta; }

    get currentInputs() { return this.#reads; }
}
