// Client 가 AI 진단의 실제 현재 DWORD 를 메뉴/입력/Session/Renderer 에 같은 주소로 제공하는 소유자를 옮긴 모듈이다(cpppj `client/ClientAiDebugGlobals.h`, 10.78 전용, 헤더만, 기준선 1f00a25a 뒤 변경 없음).
// 원본 전역의 수명은 짧은 World/Session 보다 길다: 최초 PE/BSS 값만 만들고 장치/파일/AI 행동을 실행하지 않으며, 미션 대여/반납으로 다시 초기화하지 않는다.
// 칸: showSoftAsserts 의 `checking`(005e4794, 최초 BSS 0, signed 32비트 — BOOL 로 좁히지 않음), 마지막 오류 상세 문자열 `assertionDetail`(005e4798, 최초 null, 43d20/43d40 이 저장),
//   보호 PE 의 7로그 전역 7DWORD(주소 0x52f3cc·d0·d4·d8·dc·e0·e4, 초기값 99·99·29·99·99·99·29). 주소 식별자는 정확히 일치하는 일곱 개만 현재 저장소에 연결하고 임의 주소는 거부한다.
// 사용: const ai = new ClientAiDebugGlobals(); ai.checking().value = 1; ai.word(0x52f3d4).value = 5; ai.read(0x52f3cc); ai.assertionDetail().value = '상세'.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E333: 4-D ClientAiDebugGlobals 이식)
import { DwordBox } from './client-subscriber-globals.js';

// 부호 있는 32비트 DWORD 한 칸을 `{ value }` 상자로 노출하는 클래스이다(쓰기는 `| 0` 으로 signed 폭에 맞춤). 사용: const box = new Int32Box(); box.value = 0xffffffff → -1. (2026-11-12 WB04-E333)
export class Int32Box {
    // 현재 signed 값이다.
    #value;

    // 초기값을 signed 32비트로 맞춰 둔다. (2026-11-12 WB04-E333)
    constructor(initial = 0) {
        this.#value = initial | 0;
    }

    get value() { return this.#value; }

    set value(next) { this.#value = next | 0; }
}

// 임의 값을 담는 상자(문자열 또는 null)이다. 사용: box.value = '상세'; 마지막 오류 상세를 새 월드 대여/반납으로 지우지 않는다. (2026-11-12 WB04-E333)
class ValueBox {
    value = null;
}

// 보호 PE 의 7로그 전역 주소와 초기값이다(주소 순서가 곧 저장 순서). (2026-11-12 WB04-E333)
const ADDRESSES = Object.freeze([0x52f3cc, 0x52f3d0, 0x52f3d4, 0x52f3d8, 0x52f3dc, 0x52f3e0, 0x52f3e4]);
const INITIAL_WORDS = Object.freeze([99, 99, 29, 99, 99, 99, 29]);

// Client 수명 AI 진단 소유자이다. 접근자는 호출마다 같은 객체를 돌려준다(주소 고정). (2026-11-12 WB04-E333)
export class ClientAiDebugGlobals {
    // 005e4794 의 showSoftAsserts 현재 전체 비트(최초 BSS 0, signed)이다.
    #checking = new Int32Box(0);
    // 005e4798 의 마지막 오류 상세(최초 null)이다.
    #assertionDetail = new ValueBox();
    // 7로그 DWORD 상자 일곱 개이다(각각 별도 주소).
    #words = INITIAL_WORDS.map((initial) => new DwordBox(initial));

    // 현재 checking 상자이다. 메뉴와 Renderer 가 같은 현재 값을 읽는다. (2026-11-12 WB04-E333)
    checking() { return this.#checking; }

    // 마지막 오류 상세 상자(`value` 는 null 또는 문자열)이다. (2026-11-12 WB04-E333)
    assertionDetail() { return this.#assertionDetail; }

    // 원문 주소 식별자에 해당하는 DWORD 상자이다. 정확히 일치하는 주소만 허용하며 아니면 `RangeError`(임의 포인터를 역참조하지 않음). (2026-11-12 WB04-E333)
    word(address) {
        const index = ADDRESSES.indexOf(address);
        if (index < 0) throw new RangeError('현재 AI 진단 DWORD 주소 미연결');
        return this.#words[index];
    }

    // 같은 물리의 현재 값을 읽는다(주소 일치 규칙 동일). (2026-11-12 WB04-E333)
    read(address) {
        return this.word(address).value;
    }
}
