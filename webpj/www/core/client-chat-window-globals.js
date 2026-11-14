// ClientChatWindowGlobals.h의 Client 수명 Chat 창/로그/완성/무시 목록/문자열 별칭을 소유한다(기준선 1f00a25a).
// 일시적인 Chat Gump를 교체해도 장기 하위 상태는 유지한다. 후속 뉴스 키/동맹 요청도 실제 상태만 소유하며 발생원 연결은 후속이다.
// 사용: const owner = new ClientChatWindowGlobals(); context.state = owner.window; owner.store(byteString)로 안정된 문자열 토큰을 빌린다.
// (2026-11-14 WB04-E453: RawMenuControls/ClientChatRuntime 선행 장기 소유자 이식)
import { ChatWindowState } from '../ui/chat-window.js';
import { ChatNameCompletionState } from '../ui/chat-name-completion.js';
import { StyleTextBufferState } from '../ui/style-text-buffer.js';
import { AllianceRequestState } from './raw-player-participation.js';

// 현재 Chat 창과 별개인 로그/완성/문자열 표의 같은 Client 소유자다.
export class ClientChatWindowGlobals {
    // 548740/44·54da91와 최초 완성 목록 세 개를 가진 같은 창 상태다.
    #window = new ChatWindowState();
    // 548758의 공유 로그 상태다. 실제 128줄 제한은 같은 StyleTextBuffer 처리기가 공급한다.
    #log = new StyleTextBufferState();
    // 548810..54c89f의 장기 BSS 완성 상태이며 창 생성/미션 교체에서 초기화하지 않는다.
    #completion = new ChatNameCompletionState();
    // 후속 548790의 서버별 뉴스 키 BSS다. 네트워크나 설정 읽기를 생성자에서 시작하지 않는다.
    #newsKey = new Uint8Array(128);
    // 후속 동맹 요청 제한의 같은 Client 소유 상태다. 일반 입력에는 단독 관문이 없어 임의 제외하지 않는다.
    #allianceRequests = new AllianceRequestState();
    // 같은 Client의 548750/54 무시 목록 객체다. 배열 재할당으로 별도 목록을 만들지 않는다.
    #ignored = [];
    // 토큰→원문 문자열과 전체 바이트 문자열→토큰의 두 안정된 map이다.
    #strings = new Map(); #stringTokens = new Map();
    // 54874c·548710·548748의 최초 BSS 0이며 수신/중계가 같은 필드를 사용한다.
    logFile = 0; receivedMask = 0; newsTime = 0;
    // 54db18의 입력줄 전환 DWORD와 단일 UI 조각/글꼴 저장소 대여 표식이다.
    inputLineOff = 0; uiOwner = null;
    // 입력 고리/Help 영역과 겹치지 않는 최초 호스트 문자열 토큰이다.
    nextString = 0x71000000;

    // 같은 ChatWindowState를 반환한다. 현재 Gump 토큰 변경이 다른 하위 owner를 초기화하지 않는다.
    get window() { return this.#window; }
    // 같은 공유 로그 객체를 값 사본 없이 빌린다.
    get log() { return this.#log; }
    // 같은 장기 자동완성 BSS를 빌린다.
    get completion() { return this.#completion; }
    // 같은 128BYTE 뉴스 키를 빌린다. 기존 뉴스 시각 DWORD와 별개다.
    get newsKey() { return this.#newsKey; }
    // 같은 제한 상태를 빌리며 입력 처리기/수신 연결은 별도다.
    get allianceRequests() { return this.#allianceRequests; }
    // 실제 무시 목록을 빌려 기존 수신/메뉴가 함께 수정하게 한다.
    get ignored() { return this.#ignored; }
    // 실제 토큰별 문자열 표를 빌린다. 빈 문자열/NUL 뒤 바이트도 보존한다.
    get strings() { return this.#strings; }
    // 역방향 전체 문자열 표를 빌린다. NUL까지만 비교하는 표로 바꾸지 않는다.
    get stringTokens() { return this.#stringTokens; }

    // 동일 바이트 문자열은 기존 토큰을 재사용하고 새 값만 다음 토큰에 등록한다. 한도에서 기존 값은 여전히 조회할 수 있다.
    store(value) {
        if (typeof value !== 'string') throw new TypeError('Chat 문자열 별칭은 원문 바이트 문자열이어야 합니다.');
        if (this.#stringTokens.has(value)) return this.#stringTokens.get(value);
        const token = this.nextString >>> 0; if (token === 0x7fffffff) throw new Error('Chat 문자열 주소 별칭 소진');
        this.nextString = (token + 1) >>> 0;
        this.#strings.set(token, value); this.#stringTokens.set(value, token); return token;
    }
}
