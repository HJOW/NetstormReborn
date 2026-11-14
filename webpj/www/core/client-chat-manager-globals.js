// Client 수명의 실제 Chat manager 현재 토큰(00548714)과 미초기화 객체/수신자 배열 할당, 공통 Regexp 서식 캡처 앞 81바이트를 소유한다(cpppj client/ClientChatManagerGlobals.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 원문 0042e050 재초기화는 이전 manager 를 free 하지 않으므로 current 와 할당 수명을 구별한다: 모든 할당은 비영 불투명 토큰으로 `allocations` 표에 남고, 수신자 저장소(초기 내부 128바이트 배열과 수신자 확장 요청)만 반납할 수 있고 manager 객체 자체의 반납은 거부한다.
// 같은 토큰의 Uint8Array 를 반복해서 빌려 주며(사본 없음) 0/미등록 토큰은 도달할 때 RangeError 다. JS 에는 값 초기화 없는 할당이 없으므로 새 할당은 0 으로 채워진다(원문은 미초기화이며 어떤 소비자도 미작성 바이트를 읽지 않는다).
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const owner = new ClientChatManagerGlobals(); const hooks = makeClientChatManagerHooks(owner); makeClientChatSubmitHooks(owner); makeClientChatPacketHooks(owner).
// (2026-11-14 WB04-E447: RawMenuControls 선행 Chat manager 전역 이식)
import { bindChatManagerMemoryCopy, CHAT_MANAGER_BUFFER_BYTES, CHAT_MANAGER_BYTES } from './raw-chat-manager-prefix.js';
import { CHAT_FORMAT_CAPTURE_BYTES } from '../ui/raw-chat-format.js';
import { addChatRecipient } from '../ui/raw-chat-submit.js';

// 현재 manager 토큰/서식 캡처/할당 표를 한 Client 수명으로 보관한다. 복제하거나 이동하지 않는다(소비자가 빌린 상자와 뷰의 주소를 유지한다).
export class ClientChatManagerGlobals {
    // 00548714 의 실제 한 DWORD(최초 0)를 직접 빌려 주는 상자다. 할당 자체는 이 값을 바꾸지 않는다.
    #current = { value: 0 };
    // 0059a830 공통 Regexp 캡처 중 서식 명령이 접근하는 앞 81바이트다(BSS 0). 명령/manager 재초기화 때 지우지 않는다.
    #formatCapture = new Uint8Array(CHAT_FORMAT_CAPTURE_BYTES);
    // host 포인터를 32비트로 자르지 않는 비영 수명 토큰의 다음 값이다.
    #nextToken = 1;
    // 토큰 → { bytes, recipientBuffer } 표다. current 교체 때 이전 manager 를 유지하고 수신자 확장만 이전 배열을 반납한다.
    #allocations = new Map();

    // 00548714 의 `{ value }` 상자를 값 사본 없이 돌려준다(같은 객체).
    current() { return this.#current; }

    // 서식 명령이 쓰는 같은 81바이트 캡처를 빌려 준다(미작성 뒤쪽 바이트와 재제출 흔적을 유지한다).
    formatCapture() { return this.#formatCapture; }

    // 원문 두 크기(manager 0xa98 / 내부 0x80 수신자 배열)만 할당한다. 그 밖의 크기는 TypeError(C++ invalid_argument)다. 새 current 대입/이전 할당 해제는 하지 않고 비영 토큰(또는 실패 0)을 돌려준다.
    allocate(size) {
        size >>>= 0;
        if (size !== CHAT_MANAGER_BYTES && size !== CHAT_MANAGER_BUFFER_BYTES) throw new TypeError('실제 Chatmanager 할당 크기 오류');
        return this.#allocateStorage(size, size === CHAT_MANAGER_BUFFER_BYTES);
    }

    // 수신자 저장소는 원문 DWORD 배열 요청의 양수 BYTE 폭(4의 배수)을 그대로 할당한다(0 이거나 4 의 배수가 아니면 TypeError).
    allocateRecipientBuffer(size) {
        size >>>= 0;
        if (!size || (size & 3)) throw new TypeError('Chat 수신자 배열의 실제 DWORD 폭 오류');
        return this.#allocateStorage(size, true);
    }

    // 새 current 쓰기/0채움 없이 실제 배열과 할당 종류를 같은 토큰에 보관한다. 토큰이 소진되면 Error(C++ overflow_error), 호스트 할당 실패는 0 이다.
    #allocateStorage(size, recipientBuffer) {
        if (this.#nextToken > 0xffffffff) throw new Error('Chatmanager host 토큰 소진');
        let bytes;
        try { bytes = new Uint8Array(size); } catch { return 0; }
        const token = this.#nextToken; // 표 삽입 성공 뒤에만 다음 토큰으로 진행한다.
        this.#allocations.set(token, { bytes, recipientBuffer }); this.#nextToken += 1;
        return token;
    }

    // free(NULL)은 원문처럼 허용한다. 이미 반납된 토큰/미등록 토큰/manager 객체 자체는 실제 접근 오류(RangeError)다(내부 128바이트 배열은 수신자 저장소 종류라 반납할 수 있다)(종류는 크기 추측으로 대체하지 않는다).
    releaseRecipientBuffer(token) {
        token >>>= 0; if (!token) return;
        const found = this.#allocations.get(token);
        if (!found || !found.recipientBuffer) throw new RangeError('Chat 수신자 반납의 실제 할당 없음/종류 오류');
        this.#allocations.delete(token);
    }

    // 같은 살아 있는 할당을 빌린다. 0/미등록 토큰은 실제 선택된 메모리 접근에서 RangeError 로 드러낸다(별도 캐시/사본 없음).
    bytes(token) {
        token >>>= 0; const found = this.#allocations.get(token);
        if (token === 0 || !found) throw new RangeError('Chatmanager 실제 할당 토큰 없음');
        return found.bytes;
    }
}

// 같은 owner 를 장기 캡처해 allocate/memory/순수 CRT copy 만 연결한다(나머지 하위 registerCommand/readConfig/max/minor/core 는 도달 시 계속 필수 외부 경계다). owner 가 없으면 TypeError 다.
export function makeClientChatManagerHooks(owner) {
    if (!owner) throw new TypeError('Chatmanager 실제 소유자 없음');
    const hooks = { allocate: (size) => owner.allocate(size), memory: (token) => owner.bytes(token) };
    return bindChatManagerMemoryCopy(hooks);
}

// 같은 소유자의 memory/allocate/release 만 실제 제출에 연결한다(조회/확장/반납이 같은 할당을 쓰고 나머지 조회/분배는 계속 필수 외부 장치다).
export function makeClientChatSubmitHooks(owner) {
    if (!owner) throw new TypeError('Chat 제출의 실제 소유자 없음');
    return { memory: (token) => owner.bytes(token), allocate: (size) => owner.allocateRecipientBuffer(size), release: (token) => owner.releaseRecipientBuffer(token) };
}

// 송신의 현재 물리와 0042d1b0 추가/확장/반납을 같은 실제 소유자에 연결한다(자기 추가도 기존 32슬롯 확장 몸체를 쓰고 이전 배열을 반납한다). 전역/전송/수신은 호출자의 장치다.
export function makeClientChatPacketHooks(owner) {
    if (!owner) throw new TypeError('Chat 패킷의 실제 소유자 없음');
    return { memory: (token) => owner.bytes(token), addRecipient: (manager, subscriber) => addChatRecipient(manager, subscriber, makeClientChatSubmitHooks(owner)) };
}
