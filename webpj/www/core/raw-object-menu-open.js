// 원본 10.78 객체 메뉴의 공통 가상 +ac(004adb30)와 객체 중심 위치 계산(00425f90), Factory +ac(004754b0), 포인터 열기(004d9df0)를 복원한 모듈이다(cpppj `client/RawObjectMenuOpen.cpp/.h`, 10.78 전용).
// 사용:
//   const open = new RawObjectMenuOpen(pool, types, hooks);   // pool = SidPool(10.78), types = 타입 레코드 188개(`flags2`), hooks = 아래(실행 경로에서 도달한 것만 요구).
//   open.open(object, event) → 0(extra bit1 거부/제단 위임 반환) | 1(열림). event = { code, x, y } 이며 메뉴 작성 뒤 같은 참조에서 x/y 를 읽어 콜백의 변경을 보존한다.
//   open.position(object) → 현재 객체 투영 → 첫 Form kind 사정거리 → 현재 메뉴 폭/화면 폭의 signed DWORD 계산 뒤 부착한다.
//   open.openFactory(object, event) → 위치 보정/선택/제단 위임 없이 같은 사건 좌표에 부착한다.
//   openPointerObjectMenu(open, sound) → 실제 가상 +ac 가 비영을 돌려줄 때만 opengump.wav 를 한 번 재생하고 원래 반환값을 보존한다.
//   hooks = { altar(sid) → 447930 의 같은 좌표 제단, delegate(altar, event) → 제단의 가상 +ac 반환, beginFancy(), summary(sid), populate(sid), project(x, y) → [화면 x, y](497220),
//             content(sid) → 첫 종속 Form(4ac9f0), range(kind) → TYPE+44 float, menuWidth() → 475c40 메뉴의 현재 폭+3c, screenWidth() → 5c78f0 현재 화면 폭, attach(sid, x, y) }.
//   원본 _ftol 의 signed64 하위 DWORD 와 불확정값 하위 0, DWORD 감김·부호 비교를 보존한다. 다른 판본의 슬롯/상수를 섞지 않는다(CD 대응 4aebf0/4e35f0 은 웹 범위 밖).
// 원본 관측: `object-menu-open-x86.tsv` 788입력(Open·Position 두 부모와 현재 TYPE/사건/화면 폭 변경 조합의 반환·효과 순서·현재 필드)을 tests/raw-object-menu-open.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E543: 객체 메뉴 공통 Open RawObjectMenuOpen 이식)

// 미연결 제목/가상 메뉴/부착을 빈 성공으로 처리하지 않는다. (2026-11-15 WB04-E543)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('공통 객체 메뉴 실제 하위 미연결');
    return hook;
}

// DWORD 의 비트를 원본 signed 비교용 값으로 바꾼다. (2026-11-15 WB04-E543)
const signed = (value) => value | 0;

// 단정도 비트를 수로 바꾼다(비정렬 슬롯 좌표를 포인터 캐스트 없이 읽는다). (2026-11-15 WB04-E543)
function bitsToFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 비정렬 little endian 필드를 낮은 바이트부터 조립한다. (2026-11-15 WB04-E543)
function read(raw, offset, width) {
    let value = 0;
    // 필요한 바이트만 조립한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (index * 8);
    return value >>> 0;
}

// 원본 _ftol 의 signed64 하위 DWORD 이며 유한하지 않거나 signed64 범위 밖(불확정값)은 하위 0 이다. (2026-11-15 WB04-E543)
function truncate(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 메뉴의 활성 상태/루트 토큰을 반환값으로 추정하지 않고 가상 +ac 가 돌려준 DWORD 를 직접 판정한다(004d9df0..004d9e1e). (2026-11-15 WB04-E543)
export function openPointerObjectMenu(open, sound) {
    if (typeof open !== 'function') throw new Error('포인터 객체메뉴의 실제 열기 미연결');
    const accepted = open();
    if (accepted) {
        if (typeof sound !== 'function') throw new Error('포인터 객체메뉴의 실제 열기 음향 미연결');
        sound('opengump.wav');
    }
    return accepted;
}

// 공통 Open 의 조회 시점과 원본 위치 계산의 32비트 감김·부호 비교를 보존하는 처리기이다. (2026-11-15 WB04-E543)
export class RawObjectMenuOpen {
    // 현재 구현은 보호 10.78 의 실제 슬롯 배치와 188개 TYPE 만 받는다.
    constructor(pool, types, hooks) {
        if (pool.edition !== 'patch1078' || types.length !== 188) throw new TypeError('공통 객체 메뉴 10.78 풀/TYPE 오류');
        this.pool = pool;
        this.types = types;
        this.hooks = hooks;
    }

    // 부모 함수에는 free/dead 검사가 없으므로 host 의 물리 범위만 검증한다(접근 시점마다 현재 슬롯을 읽는다).
    raw(object) {
        if (!object || object >= this.pool.capacity) throw new RangeError('공통 객체 메뉴 SID 범위 오류');
        return this.pool.slot(object);
    }

    // 재조회한 현재 객체/내용물의 TYPE 번호만 사용한다.
    type(number) {
        if (number >= this.types.length) throw new RangeError('공통 객체 메뉴 TYPE 범위 오류');
        return this.types[number];
    }

    // 실제 +a8 뒤 TYPE 가 바뀌면 마지막 위치 분기도 바뀐다. extra 는 맨 처음 한 번만 검사한다. (004adb30)
    open(object, event) {
        const h = this.hooks;
        if (this.raw(object)[40] & 1) return 0;
        if (this.type(this.raw(object)[10]).flags2 & 0x400000) {
            const altar = need(h.altar)(object);
            if (altar && altar < this.pool.capacity) return need(h.delegate)(altar, event);
        }
        need(h.beginFancy)();
        need(h.summary)(object);
        need(h.populate)(object);
        if (this.type(this.raw(object)[10]).flags2 & 0x30100) this.position(object);
        else need(h.attach)(object, event.x, event.y);
        return 1;
    }

    // 원본 Factory +ac 는 건설 중 거부 뒤 생성/제목/파생 작성/사건 좌표 부착만 수행한다. (004754b0)
    openFactory(object, event) {
        const h = this.hooks;
        if (this.raw(object)[40] & 1) return 0;
        need(h.beginFancy)();
        need(h.summary)(object);
        need(h.populate)(object);
        need(h.attach)(object, event.x, event.y);
        return 1;
    }

    // 음수와 INT_MIN 의 절댓값도 DWORD 로 유지하여 signed overflow 없이 실제 명령 결과를 재현한다. (00425f90)
    position(object) {
        const h = this.hooks;
        const raw = this.raw(object);
        const point = need(h.project)(bitsToFloat(read(raw, 14, 4)), bitsToFloat(read(raw, 18, 4)));
        let x = point[0] >>> 0;
        const form = need(h.content)(object);
        if (form) {
            const kind = read(this.raw(form), 18, 2);
            this.type(kind);
            let distance = truncate(need(h.range)(kind));
            if (signed(distance) >= 2) distance = 2;
            const padding = (((distance + 1) >>> 0) * 16) >>> 0;
            const width = truncate(need(h.menuWidth)());
            let right = (width - (need(h.screenWidth)() >>> 0) + padding + x) >>> 0;
            if (signed(right) < 0) right = 0;
            let left = (x - width - padding) >>> 0;
            const leftPosition = left;
            if (signed(left) > 0) left = 0;
            if (signed(right) < 0) right = (0 - right) >>> 0;
            if (signed(left) < 0) left = (0 - left) >>> 0;
            x = signed(right) <= signed(left) ? (x + padding) >>> 0 : leftPosition;
        }
        need(h.attach)(object, signed(x), point[1] | 0);
    }
}
