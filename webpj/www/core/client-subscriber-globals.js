// Client 수명의 두 Subscriber DWORD 와 현재 사용자 이름(handle) 64BYTE 버퍼를 보관하는 모듈이다(cpppj `client/ClientSubscriberGlobals.h`, 10.78 전용, 기준선 1f00a25a 뒤 bed549ff 에서 handle 칸 추가).
// 두 DWORD 는 00540c74(my, PE 초기 -1)와 0054265c(login, PE 초기 1)이며 handle 은 PTR00540ca8 이 가리키는 현재 NUL 문자열 물리의 호스트 64BYTE 이다.
// Init/Session/Chat/재접속이 같은 Client 의 이 객체를 **빌린다**: 값 사본을 만들지 않고, 빌린 쪽이 만들어지거나 사라질 때 초기값으로 되돌리지도 않는다(Session 반납/재생성이 지우지 않음).
// 읽는 쪽이 `{ value }` 상자나 `() => 현재값` 함수를 받는 기존 모듈(CampaignWorldGlobals.borrow, Tutorial Init 의 globals, Chat manager 접두의 subscriber)에는 아래 상자를 그대로 넘긴다.
// 사용: const owner = new ClientSubscriberGlobals(); owner.mySubscriber().value = 17; CampaignWorldGlobals.borrow({ my: owner.mySubscriber(), login: owner.loginSubscriber() }); owner.handle().set(bytes).
// 원문 32비트 주소/ABI 자체의 일치는 주장하지 않는다(C++ 도 같은 계약). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-12 WB04-E325: 4-C ClientSubscriberGlobals 이식)

// my 구독자의 PE 초기 DWORD(-1)이다. 메뉴/월드가 없는 상태를 0 으로 표현하지 않는다. (2026-11-12 WB04-E325)
export const MY_SUBSCRIBER_INITIAL = 0xffffffff;

// login 구독자의 PE 초기 DWORD(1)이다. 확인된 직접 읽기는 Init/World 의 두 위치뿐이다. (2026-11-12 WB04-E325)
export const LOGIN_SUBSCRIBER_INITIAL = 1;

// 현재 사용자 이름 버퍼의 호스트 크기이다. 기존 호스트 64BYTE/NUL 계약이며 원문 할당 크기가 아니다. (2026-11-12 WB04-E325)
export const SUBSCRIBER_HANDLE_BYTES = 64;

// 부호 없는 32비트 DWORD 한 칸을 `{ value }` 상자로 노출하는 클래스이다. 쓰기는 `>>> 0` 으로 DWORD 폭에 맞추고 읽기는 현재 값을 그대로 돌려준다.
// 사용: const box = new DwordBox(1); box.value = 0x80000000; 같은 객체를 여러 소비자가 받아 같은 칸을 읽고 쓴다(C++ `std::uint32_t&` 대응). (2026-11-12 WB04-E325)
export class DwordBox {
    // 현재 DWORD 값이다.
    #value;

    // 초기값을 DWORD 로 맞춰 둔다. (2026-11-12 WB04-E325)
    constructor(initial = 0) {
        this.#value = initial >>> 0;
    }

    // 현재 값을 읽는다. 호출 위치에서 매번 새로 조회하며 사본을 만들지 않는다. (2026-11-12 WB04-E325)
    get value() { return this.#value; }

    // 새 값을 DWORD 로 감아 저장한다. (2026-11-12 WB04-E325)
    set value(next) { this.#value = next >>> 0; }
}

// Client 수명의 Subscriber 두 DWORD 와 현재 이름 버퍼의 소유자이다. 각 접근자는 호출마다 같은 객체를 돌려준다(주소 고정). (2026-11-12 WB04-E325)
export class ClientSubscriberGlobals {
    // 00540c74 의 my 구독자 상자이다(PE 초기 0xffffffff).
    #my = new DwordBox(MY_SUBSCRIBER_INITIAL);
    // 0054265c 의 login 구독자 상자이다(PE 초기 1). 생성 뒤 임의 1 쓰기를 추가하지 않는다.
    #login = new DwordBox(LOGIN_SUBSCRIBER_INITIAL);
    // PTR00540ca8 의 현재 사용자 이름 물리(64BYTE, 최초 전체 0)이다. 설정/Player 이름 사본으로 대체하지 않는다.
    #handle = new Uint8Array(SUBSCRIBER_HANDLE_BYTES);

    // 00540c74 의 같은 현재 DWORD 상자이다. Chat 접두 등은 `() => owner.mySubscriber().value` 로 현재 값을 빌린다. (2026-11-12 WB04-E325)
    mySubscriber() { return this.#my; }

    // 0054265c 의 같은 현재 DWORD 상자이다. Init/World 의 현재값 읽기도 같은 칸이다. (2026-11-12 WB04-E325)
    loginSubscriber() { return this.#login; }

    // 현재 사용자 이름 64BYTE 버퍼이다. Session 반납/재생성에서 지우지 않고 같은 배열을 계속 돌려준다. (2026-11-12 WB04-E325: 기준선 뒤 bed549ff 반영)
    handle() { return this.#handle; }
}
