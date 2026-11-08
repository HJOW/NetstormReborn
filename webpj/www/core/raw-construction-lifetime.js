// 표시 목록(RenderProcess 시스템 배열)의 등록/제거와 건설 표시 버퍼의 소멸 순서를 옮긴 모듈이다(cpppj `o/RawConstructionLifetime.cpp/.h`, 10.78 전용. 원본 Particle.cpp 의 표시 목록 등록/제거와 Build.cpp 의 버퍼 소멸).
//  - RawRenderRegistry: 원본 32비트 포인터 배열을 0 이 아닌 호스트 토큰 배열로 표현한 표시 목록. construct 는 필드 초기화 → 첫 빈 칸(없으면 40칸 확장) → slot/슬롯 0 깊이 → 부모 부착(flags 0x10)
//    → 진입 부모의 현재 깊이 순서이고, destroy 는 일치한 등록만 지우고 base 부모/form 을 -12345 로 바꾼다(slot/zOrder/사각형은 남김). 잘못된 번호·중복 삭제는 변경 전에 예외로 거부한다.
//  - destroyConstructionVisual: 캡처 → 출력 → 입자 버퍼를 존재할 때만 차례로 반납하고, 표시 등록 제거/base 소멸 → flags bit 0 이면 객체 반납. 공유 팔레트는 반납하지 않는다.
//  - makeConstructionRegistryHooks: 건설 표시 생성의 부착 경계를 이 목록의 construct 로 연결하고 상위 부착의 현재 부모/초기 사각형/zOrder 를 표시 객체로 되돌려준다.
// 표시 객체의 소유권은 호출자가 가지며 이 목록은 token 만 보관한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E129: RawConstructionLifetime 이식으로 추가)

// 원본 목록 확장 단위와 포인터 폭, base 소멸자의 부모/form 표식, 부착 flags(client 번호 할당 0x10), 패치 깊이 BYTE 의 raw 위치이다. (2026-11-08 WB04-E129)
const GROWTH = 40;
const POINTER_BYTES = 4;
const DESTROYED = -12345;
const ATTACH_FLAGS = 0x10;
const PATCH_Z = 35;
const INT32_MAX = 2147483647;

// 원본 free 호출에서 구별하는 세 소유 버퍼와 scalar deleting destructor 의 객체 자체이다. (2026-11-08 WB04-E129)
export const ConstructionRelease = Object.freeze({ SOURCE: 'Source', OUTPUT: 'Output', PARTICLES: 'Particles', OBJECT: 'Object' });

// RenderProcess 의 공통 필드이다. token 은 원본 포인터 대신 쓰는 0 이 아닌 호스트 식별자이다.
// form/parent 는 +4/+8(부착 전 0, base 소멸 뒤 -12345), left..bottom 은 +0xc..+0x18 표시 영역, slot/zOrder 는 +0x1c/+0x20(목록 번호·부모 깊이)이다. 사용: const registration = createRenderProcessFields(). (2026-11-08 WB04-E129)
export function createRenderProcessFields() {
    return { token: 0, form: 0, parent: 0, left: 0, top: 0, right: 0, bottom: 0, slot: -1, zOrder: 0 };
}

// 부모 raw 의 부호 있는 깊이 BYTE 를 읽는다. SID 0 도 원본처럼 읽을 수 있다. (2026-11-08 WB04-E129)
function signedByte(value) {
    return (value << 24) >> 24;
}

// 표시 목록이다. 사용: const registry = new RawRenderRegistry(pool, { reserve, releaseArray, attach }, initialTokens); registry.construct(fields, token, parentSid, type).
// hooks.reserve(바이트 수)→성공 여부 / hooks.releaseArray() 기존 배열 반납 / hooks.attach(fields, type, parent, flags) 부모 부착이다. pool 은 목록보다 오래 살아야 한다. (2026-11-08 WB04-E129)
export class RawRenderRegistry {
    // 부모 raw 자료이다.
    #pool;
    // 메모리 확보/반납/부착 경계이다.
    #hooks;
    // 원본 systems 배열의 토큰 표현이다(0 은 빈 칸).
    #slots;

    // 경계 누락과 원본 signed DWORD 범위를 넘는 목록은 초기화 때 거부한다. (2026-11-08 WB04-E129)
    constructor(pool, hooks, initial = []) {
        if (typeof hooks?.reserve !== 'function' || typeof hooks.releaseArray !== 'function' || typeof hooks.attach !== 'function') throw new TypeError('표시 목록 경계 누락');
        if (initial.length > Math.floor(INT32_MAX / POINTER_BYTES)) throw new RangeError('표시 목록 크기 오류');
        this.#pool = pool;
        this.#hooks = { reserve: hooks.reserve, releaseArray: hooks.releaseArray, attach: hooks.attach };
        this.#slots = initial.map((token) => token >>> 0);
    }

    // 수집기와 표시 등록이 같은 부모 raw 풀을 공유하는지 확인할 때 쓴다. (2026-11-08 WB04-E129)
    get pool() { return this.#pool; }

    // 현재 슬롯 순서와 빈 칸을 읽는 독립 사본이다. 다음 construct 의 확장 뒤에도 이 사본은 바뀌지 않는다. (2026-11-08 WB04-E129)
    get slots() { return this.#slots.slice(); }

    // 현재 raw 부모의 signed BYTE 깊이이다. (2026-11-08 WB04-E129)
    #depth(parent) {
        return signedByte(this.#pool.slot(parent)[PATCH_Z]);
    }

    // 필드 초기화→첫 빈 칸/40칸 확장→slot/슬롯 0 깊이→부모 부착→진입 부모의 현재 깊이 순서이다. parent=0 이면 부착하지 않는다. token 은 목록 안에서 유일해야 한다.
    // 원본: 00487fd0. (2026-11-08 WB04-E129)
    construct(fields, token, parent, type) {
        token >>>= 0;
        if (!token || this.#slots.includes(token)) throw new Error('표시 목록 토큰 중복/0');
        if (fields.token && fields.slot >= 0 && fields.slot < this.#slots.length && this.#slots[fields.slot] === fields.token) throw new Error('살아 있는 표시 등록 필드 재사용');
        // 부모가 풀 밖이면 변경 전에 거부한다.
        this.#pool.slot(parent);
        fields.token = token;
        fields.form = 0;
        fields.parent = 0;
        fields.left = 0;
        fields.top = 0;
        fields.right = 0;
        fields.bottom = 0;
        let free = this.#slots.indexOf(0);
        if (free < 0) {
            const oldSize = this.#slots.length;
            if (oldSize > Math.floor(INT32_MAX / POINTER_BYTES) - GROWTH) throw new RangeError('표시 목록 확장 범위 오류');
            const newSize = oldSize + GROWTH;
            if (!this.#hooks.reserve(newSize * POINTER_BYTES)) throw new Error('표시 목록 메모리 확보 실패');
            const next = new Array(newSize).fill(0);
            // 기존 내용을 복사한 뒤 기존 배열을 반납한다.
            for (let index = 0; index < oldSize; index += 1) next[index] = this.#slots[index];
            if (oldSize) this.#hooks.releaseArray();
            this.#slots = next;
            free = oldSize;
        }
        fields.slot = free;
        this.#slots[free] = token;
        fields.zOrder = this.#depth(0);
        if (parent) {
            this.#hooks.attach(fields, type >>> 0, parent, ATTACH_FLAGS);
            fields.zOrder = this.#depth(parent);
        }
    }

    // 일치한 등록만 지우고 base 부모/form 을 -12345 로 바꾼다. slot/zOrder/사각형은 남긴다. 잘못된 번호·중복 삭제는 변경 전에 예외로 거부한다.
    // 원본: 00487880→0041bd00. (2026-11-08 WB04-E129)
    destroy(fields) {
        if (fields.slot < 0 || fields.slot >= this.#slots.length || !fields.token || this.#slots[fields.slot] !== fields.token) throw new Error('표시 목록 삭제 등록 불일치');
        this.#slots[fields.slot] = 0;
        fields.form = DESTROYED;
        fields.parent = DESTROYED;
    }
}

// 캡처→출력→입자→표시 등록 제거/base 소멸→flags bit 0 의 객체 반납 순서이다. 공유 팔레트는 반납하지 않는다.
// 사용: release(kind) 는 호출 중에만 자료를 읽고 예외를 던지지 않아야 한다. 이 함수 뒤 같은 등록을 다시 삭제하지 않는다.
// 원본: 00422d00/00423540/00423820. (2026-11-08 WB04-E129)
export function destroyConstructionVisual(visual, registration, registry, flags, release) {
    if (typeof release !== 'function') throw new TypeError('건설 표시 반납 경계 누락');
    // 원본 포인터가 있는 버퍼만 차례로 반납한다.
    if (visual.image.pixels.length) {
        release(ConstructionRelease.SOURCE);
        visual.image.pixels = new Uint8Array(0);
    }
    if (visual.output.length) {
        release(ConstructionRelease.OUTPUT);
        visual.output = new Uint8Array(0);
    }
    if (visual.particles.length) {
        release(ConstructionRelease.PARTICLES);
        visual.particles = [];
    }
    registry.destroy(registration);
    if (flags & 1) release(ConstructionRelease.OBJECT);
}

// 기존 표시 생성의 부착 경계를 원본 목록 등록 몸체로 연결한다. 나머지 캡처/팔레트 경계는 그대로 쓴다.
// 사용: registry/registration 은 반환한 hooks 보다 오래 살아야 한다. 실제 Kernel 부착은 registry 의 attach 책임이다. (2026-11-08 WB04-E129)
export function makeConstructionRegistryHooks(registry, registration, token, hooks) {
    if (!token) throw new TypeError('건설 표시 등록 토큰 0');
    // 기존 부착 경계의 책임을 registry.construct 안의 BaseProcess 경계로 넘긴다.
    const attach = (visual, type) => {
        registry.construct(registration, token, visual.parent, type);
        if (registration.parent < 0 || registration.parent > 0xffff) throw new RangeError('건설 표시 부착 부모 범위 오류');
        visual.parent = registration.parent;
        visual.left = registration.left;
        visual.top = registration.top;
        visual.right = registration.right;
        visual.bottom = registration.bottom;
        visual.zOrder = registration.zOrder;
    };
    return { ...hooks, attach };
}
