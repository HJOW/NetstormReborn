// Client 수명의 005c8aa4 Template 구역 이름표(512BYTE) 한 개를 파일 reader 와 신규 manager 초기화가 함께 빌리는 lazy 소유자를 옮긴 모듈이다(cpppj `client/ClientTemplateSectionNames.cpp/.h`, 10.78 전용).
// 생성자는 할당도 쓰기도 하지 않고, 처음 names() 에서만 원문 부모(`raw-template-section-names.js` 의 getTemplateSectionNames)를 실행해 실제 512BYTE 한 개를 할당하고 277BYTE 를 쓴다.
// 이후 호출은 같은 배열(같은 참조)을 돌려주며, Session/Storage 교체와 무관하다(Client 수명, 정리에서 reset 하지 않음). 이름표는 원본처럼 제자리 변환될 수 있는(SNART) 가변 배열이다.
// 표의 끝 235BYTE(277~511)는 원문이 쓰지 않는 미정 영역이라 게임 규칙으로 읽지 않는다: JS 의 기본 할당은 0 으로 채워지며, 검사에서는 알려진 채움 값을 넣는 할당 장치를 주입해 건드리지 않음을 확인한다.
// 현재 토큰(최초 0, 성공 1)은 호스트 불투명 값이며 실제 주소를 DWORD 로 자르지 않는다. 할당이 null 이면 물리 실패로 남아 같은 토큰 0 을 보존한다(다시 부르면 다시 할당을 시도).
// 사용: const names = new ClientTemplateSectionNames(); const bytes = names.names(); names.currentToken() === 1. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E281: 4-B ClientTemplateSectionNames 이식)
import { TEMPLATE_SECTION_NAMES_CAPACITY, getTemplateSectionNames } from './raw-template-section-names.js';

// 이름표 소유자이다. 복사/이동하지 않는다(살아 있는 배열 참조와 토큰 상태를 한 객체가 가진다). (2026-11-11 WB04-E281)
export class ClientTemplateSectionNames {
    // 최초 선택에서만 실행하는 실제 또는 검사 할당 장치(size → Uint8Array(512) 또는 null)이다.
    #allocate;
    // 값 초기화 없이 확보한 실제 512BYTE 이며 원문이 쓴 277BYTE 만 정의된다.
    #bytes = null;
    // 005c8aa4 의 새 Client BSS 0 이다. Session 정리에서 reset 하지 않는다.
    #globals = { current: 0 };

    // allocate 를 주지 않으면 0 으로 채운 새 Uint8Array(size)를 쓴다. (2026-11-11 WB04-E281)
    constructor(allocate = null) {
        if (allocate !== null && typeof allocate !== 'function') throw new TypeError('Template 이름표 할당 장치는 함수여야 합니다.');
        this.#allocate = allocate ?? ((size) => new Uint8Array(size));
    }

    // 실제 512BYTE 의 같은 배열을 lazy 생성/반환한다. 원문 부모가 현재 토큰을 공개한 뒤 같은 할당에만 쓴다. (2026-11-11 WB04-E281)
    names() {
        // 원문 서식의 정의된 7BYTE('Terr' + 숫자 두 자리 + NUL)만 출력하며 미정 local256 끝을 빌리지 않는다.
        const formatted = new Uint8Array(7);
        const hooks = {
            allocate: (size) => {
                if (size !== TEMPLATE_SECTION_NAMES_CAPACITY) throw new Error('Client 이름표 실제 512 할당 계약');
                const bytes = this.#allocate(size);
                if (bytes !== null && bytes !== undefined && (!(bytes instanceof Uint8Array) || bytes.length !== TEMPLATE_SECTION_NAMES_CAPACITY)) throw new TypeError('Client 이름표 할당은 정확히 512BYTE 여야 합니다.');
                this.#bytes = bytes ?? null;
                return this.#bytes ? 1 : 0;
            },
            buffer: (token) => {
                if (token !== 1 || !this.#bytes) throw new RangeError('Client 이름표 실제 NULL 할당 접근');
                return this.#bytes;
            },
            format: (index) => {
                if (index >= 20) throw new RangeError('Template Terr 원문 서식 0..19 범위');
                formatted.set([0x54, 0x65, 0x72, 0x72, 0x30 + Math.floor(index / 10), 0x30 + (index % 10), 0]);
                return formatted;
            },
        };
        const token = getTemplateSectionNames(this.#globals, hooks);
        return hooks.buffer(token);
    }

    // 최초 0, 성공 1 인 호스트 불투명 토큰을 읽는다. (2026-11-11 WB04-E281)
    currentToken() { return this.#globals.current; }
}
