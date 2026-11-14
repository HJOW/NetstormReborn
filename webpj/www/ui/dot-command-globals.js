// 원본 PE 의 이름/콜백/표시/count 인접 표를 Client 수명에 같은 바이트로 소유한다(cpppj client/ClientDotCommandGlobals.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 최초 say 슬롯과 count 1 은 PE 정적 데이터(00531b30~00532537)이며 Chat 초기화 때 빈 표로 다시 만들지 않는다. 정적 say 함수 값과 표시 주소는 원본 식별자이며 host 함수 포인터로 실행하지 않는다.
// 사용: const globals = new ClientDotCommandGlobals(); globals.register(nameBytes, 0x1111, 1); dispatchDotCommand(globals.bytes(), globals.dispatchState(), input, hooks).
// (2026-11-14 WB04-E443: RawMenuControls 선행 점 명령 전역 이식)
import { DOT_COMMAND_NAMES_TOKEN, DOT_COMMAND_STATE_BYTES, registerDotCommand } from './dot-command-register.js';
import { DotCommandDispatchState } from './dot-command-dispatch.js';

// 고정 PE 정적 DWORD 를 원래 little endian 순서로 같은 표에 쓴다(DWORD 의 네 바이트만 초기화하며 인접 항목을 건드리지 않는다).
function initialDword(bytes, offset, value) { for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff; }

// 점 명령 표/검색 상태의 장기 소유자다. 한 표와 한 검색 상태를 복제하지 않고 같은 물리로 빌려준다.
export class ClientDotCommandGlobals {
    // PE 의 영 초기화 영역과 비영 정적 슬롯을 함께 보관하는 2568바이트 표다.
    #bytes = new Uint8Array(DOT_COMMAND_STATE_BYTES);
    // 00557db4/00557df0 의 실제 장기 검색 상태이며 등록이나 Chat 재생성으로 초기화하지 않는다.
    #dispatch = new DotCommandDispatchState();

    // 최초 00531b30~00532537 의 실제 데이터만 한 번 구성한다: 슬롯 0 이름 'say', 함수 0x0042f1e0, 표시 0 은 이름표 첫 슬롯, count 1(등록은 say 다음의 슬롯 1 에서 시작).
    constructor() {
        this.#bytes[0] = 0x73; this.#bytes[1] = 0x61; this.#bytes[2] = 0x79;
        initialDword(this.#bytes, 0x800, 0x0042f1e0); initialDword(this.#bytes, 0x900, DOT_COMMAND_NAMES_TOKEN); initialDword(this.#bytes, 0xa04, 1);
    }

    // count 를 따로 복사하지 않고 전체 2568바이트의 같은 가변 물리를 빌린다(Session/호출 교체에 따라 다시 초기화하지 않는다).
    bytes() { return this.#bytes; }
    // 같은 Client 의 기본 슬롯/Regexp 캡처를 빌린다. 제출·재진입·미션 교체에서 복제하지 않는다.
    dispatchState() { return this.#dispatch; }
    // 실제 0043f660 본체에 같은 표를 전달한다(복사 callback 뒤 단일 count 캡처와 부분 효과는 등록 부모가 수행).
    register(name, callback, flag, hooks = {}) { return registerDotCommand(this.#bytes, name, callback, flag, hooks); }
}
