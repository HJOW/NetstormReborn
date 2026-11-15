// 10.78 의 커서 마나 표시(원문 004737f0 생성자·00473d00 프레임·00473c30 통지·00473b90 소멸)를 실제 SID 객체와 BaseProcess 수명으로 실행하는 모듈이다(cpppj `client/RawCursorMana.cpp/.h`, 10.78 전용).
// 사용:
//   const logic = new RawCursorMana(pool, types, clock, globals, cursor, hooks);
//     pool = SidPool(10.78), types = 타입 레코드 188개(`theme`·`footX`·`footY`), clock = SquidProcessState(현재 double 게임 시각 `now`), globals = new CursorManaGlobals(),
//     cursor = CursorDropState(소멸 때 같은 `cursorGump` 토큰을 지운다), hooks = { create(type, flags) → sid, setWord(sid, 하위 WORD), themeFrame(theme) → 프레임, detach(sid, 0)/unpop(sid, 0)/destroy(sid, 0), position(sid, x, y, flags), center(parent) → [x, y] }.
//   logic.construct(fields, parent, form, attach) → 객체 생성 뒤 attach(fields, parent, form, 0x10)(Kernel 소유권 이전은 호출자가 제공)를 호출하고 현재 시각/표시 상태를 초기화한다.
//   logic.frame(fields, kill) → 원형(기본)/사각형 프레임 이동·확대/축소·종료 판정(kill 은 실제 Kill 이며 사각형 꼬리 시각 쓰기 뒤에도 필드가 살아 있어야 한다).
//   logic.notify(fields, event, info, value) → event 3 은 종료 표식, 1 은 숨김 값 전체 DWORD 와 각 객체의 현재 void 여부 반영, 그 밖은 3초 뒤 기한이다(세 번째 인자는 원본에서도 쓰지 않는다).
//   logic.destroy(fields, token) → 현재 커서 프로세스 토큰 해제 → 표시 객체 삭제 → Baseprocess 필드 무효화 순서.
//   createCursorManaProcess(host, logic, parent, form = 0) / bindCursorManaLifecycle(hooks, host, kernel, logic) / cursorManaThemeFrame(theme) / bindCursorManaWorld(hooks, world, center, notifySurface) — 프로세스·수명·월드 연결.
// 필드는 원본 new(0xe0) 의 바이트(미사용 padding 포함)를 보존하며 float 저장 위치 외의 연산은 double 로 수행한다. 선택한 경계의 누락은 그 위치에서 거부한다(Error '미연결').
// 원본 관측: `cursor-mana-x86.tsv` 642입력(생성·프레임·통지·소멸·반복 호출의 이벤트·필드 224바이트·전역·raw 슬롯·단계별 필드)과 `cursor-mana-display-x86.tsv`·`mana-bolt-display-x86.tsv` 각 432입력(표시 변경 표)을
//   tests/raw-cursor-mana.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E549: 커서 마나 표시 RawCursorMana 이식)
import { SquidProcess } from './squid-process.js';

// 비교용 원본 주소(호스트 함수 포인터가 아님)와 원본 필드 폭이다. (2026-11-15 WB04-E549)
const MANA_VTABLE = 0x50c468;
const BASE_VTABLE = 0x5019ec;
export const CURSOR_MANA_FIELD_BYTES = 0xe0;
// wrtsWRTS.?> 를 대문자로 조회한 초기 theme→프레임 표이다. (2026-11-15 WB04-E549)
const THEME_FRAMES = Object.freeze([4, 5, 6, 7, 4, 5, 6, 7, 8, 9, 10]);

// 선택된 실제 효과만 요구하며 빈 경계로 동작을 생략하지 않는다. (2026-11-15 WB04-E549)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('실제 커서 마나 경계 미연결');
    return effect;
}

// 원본 new(0xe0) 필드 저장소를 만든다(+c 의 30 SID·+84 개수·+8c 반경·+c0/c8/d0 시각 등을 원본 폭으로 저장). (2026-11-15 WB04-E549)
export function createCursorManaFields() { return { bytes: new Uint8Array(CURSOR_MANA_FIELD_BYTES) }; }

// 필드 접근자들이다(비정렬 little endian, float/double 비트 패턴 유지). (2026-11-15 WB04-E549)
const view = (fields) => new DataView(fields.bytes.buffer, fields.bytes.byteOffset, fields.bytes.byteLength);
const U = (fields, offset) => view(fields).getUint32(offset, true);
const I = (fields, offset) => view(fields).getInt32(offset, true);
const F = (fields, offset) => view(fields).getFloat32(offset, true);
const D = (fields, offset) => view(fields).getFloat64(offset, true);
const put = (fields, offset, value) => view(fields).setUint32(offset, value >>> 0, true);
const putF = (fields, offset, value) => view(fields).setFloat32(offset, value, true);
const putD = (fields, offset, value) => view(fields).setFloat64(offset, value, true);
// raw 객체의 단정도 좌표를 비정렬 읽기한다. (2026-11-15 WB04-E549)
const rawFloat = (raw, offset) => new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);

// 원본 _ftol 의 64비트 결과 중 EAX 만 쓰는 시간 절삭이다. (2026-11-15 WB04-E549)
function timeInteger(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// x87 fcos/fsin 은 범위 밖 유한 입력을 그대로 남긴다. 게임 범위에서는 실제 삼각 함수를 계산한다. (2026-11-15 WB04-E549)
function trig(angle, cosine) {
    if (Number.isFinite(angle) && Math.abs(angle) >= 9223372036854775808) return angle;
    return cosine ? Math.cos(angle) : Math.sin(angle);
}

// 생성/좌표 제한이 사용하는 현재 원본 전역이며 프로세스 사이에서 하나를 공유한다. 실제 Client 는 장기 주소를 빌리는 접근자(getter/setter)로 `mapLimit`·`radiusLimit` 를 대체할 수 있다. (2026-11-15 WB04-E549)
export class CursorManaGlobals {
    manaType = 103; // 005411f4 의 마나 객체 번호이다.
    processType = 50; // 00541138 의 마나 프로세스 번호이다.
    mapLimit = 256; // 00531928 의 맵 경계(독립 입력의 기존 256)이다.
    invalidPoint = 1; // 0052f838 의 범위 밖 위치이다.
    radiusLimit = 22; // 0052f488 의 현재 공급 반경(독립 입력의 기존 22)이다.
}

// 실제 theme 문자 표에서 확인한 프레임만 공급한다. (2026-11-15 WB04-E549)
export function cursorManaThemeFrame(theme) {
    if (theme < 0 || theme >= THEME_FRAMES.length) throw new RangeError('마나 theme 프레임 범위 오류');
    return THEME_FRAMES[theme];
}

// 생성·원형/사각형 프레임·이벤트·소멸의 실제 몸체를 같은 현재 필드에 적용하는 논리이다. 모든 참조와 효과는 연결한 프로세스보다 오래 살아야 한다. (2026-11-15 WB04-E549)
export class RawCursorMana {
    // 실제 타입/슬롯 레이아웃이 다른 판본을 연결하지 않는다.
    constructor(pool, types, clock, globals, cursor, hooks) {
        if (pool.edition !== 'patch1078' || types.length !== 188) throw new TypeError('커서 마나 10.78 Pool/TYPE 계약 오류');
        this.pool = pool;
        this.types = types;
        this.clock = clock;
        this.globals = globals;
        this.cursor = cursor;
        this.hooks = hooks;
    }

    // 실제 풀/프로세스 종류를 Host 연결 시 확인한다.
    get processType() { return this.globals.processType; }

    // 원본 _ftol 의 EAX 만 쓰는 시간 절삭이다(비유한 값·2^63 이상은 0). 직접 값 검사용으로 공개한다. (2026-11-15 WB04-E549)
    static timeInteger(value) { return timeInteger(value); }

    // x87 fcos/fsin 대응이다(유한 입력이 2^63 이상이면 입력을 그대로 남김). 직접 값 검사용으로 공개한다. (2026-11-15 WB04-E549)
    static trig(angle, cosine) { return trig(angle, cosine); }

    // 원본 SID 범위 밖의 호스트 메모리를 읽지 않는다(접근 시점마다 현재 슬롯을 읽는다).
    raw(sid) {
        if (sid < 5 || sid >= this.pool.capacity) throw new RangeError('커서 마나 SID 범위 오류');
        return this.pool.slot(sid);
    }

    // 목록은 원본 DWORD SID 이며 호스트의 16비트 SID 로 조용히 절단하지 않는다.
    marker(fields, index) {
        if (index < 0 || index >= 30) throw new RangeError('커서 마나 30칸 목록 범위 오류');
        const value = U(fields, 12 + 4 * index);
        if (value >= this.pool.capacity) throw new RangeError('커서 마나 목록 SID 범위 오류');
        return value;
    }

    // 0041bfb0 의 현재 부모와 종속 머리 진단을 유지한다.
    parent(fields) {
        const value = U(fields, 8);
        if (value >= this.pool.capacity) throw new RangeError('커서 마나 부모 SID 범위 오류');
        const raw = this.raw(value);
        if (!(raw[6] || raw[7])) throw new Error('parent->getHead() != 0 원본 마나 진단');
        return value;
    }

    // NaN/끝점도 원본의 엄격한 부등식을 통과하지 못한다(0041d150/0040e800 의 좌표 경계와 현재 범위 밖 대체점).
    clamp(point) {
        const limit = Math.fround(this.globals.mapLimit | 0);
        if (!(point[0] > 0 && point[0] < limit && point[1] > 0 && point[1] < limit)) {
            const invalid = Math.fround(this.globals.invalidPoint);
            return [invalid, invalid];
        }
        return point;
    }

    // 004737f0: 객체 생성 뒤 Attach(type50, parent, form, 10), 그 뒤 현재 시각/표시 상태를 초기화한다. TYPE 포인터는 유지하고 theme·현재 개수·효과 뒤 시각은 매번 다시 읽는다.
    construct(fields, parent, form, attach) {
        const h = this.hooks;
        const g = this.globals;
        put(fields, 0, MANA_VTABLE); put(fields, 4, 0); put(fields, 8, 0); putD(fields, 0xc0, 0); putD(fields, 0xc8, 0); putD(fields, 0xd0, 0);
        const raw = this.raw(parent);
        const type = raw[10];
        if (type >= this.types.length) throw new RangeError('커서 마나 부모 TYPE 범위 오류');
        const definition = this.types[type];
        put(fields, 0x98, 0);
        putF(fields, 0xbc, (((definition.footX >>> 0) - 1) | 0));
        putF(fields, 0xb8, (((definition.footY >>> 0) - 1) | 0));
        const parentX = rawFloat(raw, 14);
        const parentY = rawFloat(raw, 18);
        // 객체 목록 30칸을 초기화하며 원본의 나머지 padding 을 건드리지 않는다.
        for (let index = 0; index < 30; index += 1) put(fields, 12 + 4 * index, 0);
        // 각 표시 객체는 실제 생성·WORD setter·extra·프레임 쓰기 뒤 목록에 들어간다.
        const create = (index) => {
            const sid = need(h.create)(g.manaType, 2);
            this.raw(sid);
            need(h.setWord)(sid, 1);
            const bytes = this.pool.allocated(sid);
            bytes[40] |= 8;
            const frame = need(h.themeFrame)(definition.theme);
            new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).setUint32(36, frame >>> 0, true);
            put(fields, 12 + 4 * index, sid);
            return sid;
        };
        if (!U(fields, 0x98)) {
            put(fields, 0x84, 8);
            // 가상 위치 효과가 현재 개수를 바꾸는 경우에도 다음 조건은 현재 필드를 읽는다.
            for (let index = 0; index < I(fields, 0x84); index += 1) {
                if (index >= 30) throw new RangeError('커서 마나 생성 목록 범위 오류');
                const sid = create(index);
                need(h.position)(sid, Math.fround(g.invalidPoint), Math.fround(g.invalidPoint), 0);
            }
        } else {
            put(fields, 0x88, 5); put(fields, 0x84, 20);
            // 원본 생성자는 5변을 생성한다. 프레임/소멸의 4변·개수 20 과 임의로 통일하지 않는다.
            for (let side = 0; side < 5; side += 1) {
                // 원본의 현재 변별 개수를 생성 효과 뒤마다 다시 읽는다.
                for (let index = 0; index < I(fields, 0x88); index += 1) {
                    const offset = I(fields, 0x88) * side + index;
                    if (offset < 0 || offset >= 30) throw new RangeError('커서 마나 사각 생성 목록 범위 오류');
                    const sid = create(offset);
                    const delta = index * Math.fround(1.600000023841858);
                    let point;
                    if (!(side & 1)) point = [Math.fround(parentX - delta), Math.fround(parentY - (side === 0 ? F(fields, 0xb8) : 0))];
                    else point = [Math.fround(parentX - (side === 1 ? F(fields, 0xbc) : 0)), Math.fround(parentY - delta)];
                    point = this.clamp(point);
                    need(h.position)(sid, point[0], point[1], 0x10);
                }
            }
            // 사각형 위상 7칸은 이 생성 분기에서만 초기화된다.
            for (let index = 0; index < 7; index += 1) put(fields, 0x9c + 4 * index, 0);
        }
        putF(fields, 0x8c, g.radiusLimit);
        putF(fields, 0x90, 0);
        need(attach)(fields, parent, form, 0x10);
        put(fields, 0x94, 0);
        putD(fields, 0xc0, this.clock.now); putD(fields, 0xc8, this.clock.now); putD(fields, 0xd0, 0); put(fields, 0xd8, 0);
    }

    // 00473d00: float 저장 위치를 제외한 연산은 원본 기본 53비트 정밀도의 double 로 수행한다. kill 은 실제 0041bf10(this,0)이다.
    frame(fields, kill) {
        const h = this.hooks;
        const parent = this.parent(fields);
        const raw = this.raw(parent);
        const parentX = rawFloat(raw, 14);
        const parentY = rawFloat(raw, 18);
        const delta = Math.fround(this.clock.now - D(fields, 0xc0));
        if (D(fields, 0xd0) !== 0 && D(fields, 0xd0) < this.clock.now) put(fields, 0x94, 1);
        if (U(fields, 0x98)) {
            const phase = (item) => {
                if (item < 0 || item >= 7) throw new RangeError('커서 마나 사각 위상 범위 오류');
                return F(fields, 0x9c + 4 * item);
            };
            let index = 0;
            // 앞 절반은 현재 위상을 증가시키고 뒤 절반은 원본의 대칭 순서로 복사한다.
            for (; index <= Math.trunc(I(fields, 0x88) / 2); index += 1) {
                if (index >= 7) throw new RangeError('커서 마나 사각 위상 범위 오류');
                putF(fields, 0x9c + 4 * index, (index * 0.03 + phase(index)) + 0.3);
            }
            // 원본의 두 칸씩 증가하는 대칭 간격이다.
            for (let distance = 2; index < I(fields, 0x88); index += 1, distance += 2) {
                const value = phase(index - distance);
                if (index >= 7) throw new RangeError('커서 마나 사각 위상 범위 오류');
                putF(fields, 0x9c + 4 * index, value);
            }
            let processed = 0;
            // 원본 frame 은 생성자의 5변 중 4변을 이동시킨다.
            for (let side = 0; side < 4; side += 1) {
                // SID 0 은 건너뛰며 삭제한 뒤 다음 인덱스 계산도 현재 변별 개수를 읽는다.
                for (let item = 0; item < I(fields, 0x88); item += 1) {
                    const sid = this.marker(fields, I(fields, 0x88) * side + item);
                    if (!sid) continue;
                    processed += 1;
                    need(h.detach)(sid, 0);
                    const current = this.raw(sid);
                    let dx = Math.fround((parentX - (side === 1 ? F(fields, 0xbc) : 0)) - rawFloat(current, 14));
                    let dy = Math.fround((parentY - (side === 0 ? F(fields, 0xb8) : 0)) - rawFloat(current, 18));
                    if (side >= 2) { dx = -dx; dy = -dy; }
                    const movement = delta * phase(item);
                    let point;
                    if (side & 1) point = [Math.fround((parentX - (side === 1 ? F(fields, 0xbc) : 0)) + (dx + movement) * (side === 1 ? -1.0 : 1.0)), rawFloat(current, 18)];
                    else point = [rawFloat(current, 14), Math.fround((parentY - (side === 0 ? F(fields, 0xb8) : 0)) + (dy + movement) * (side === 0 ? -1.0 : 1.0))];
                    point = this.clamp(point);
                    need(h.position)(sid, point[0], point[1], 0x10);
                    if (Math.abs(dx) > F(fields, 0x8c) || Math.abs(dy) > F(fields, 0x8c)) {
                        need(h.destroy)(sid, 0);
                        const offset = I(fields, 0x88) * side + item;
                        if (offset < 0 || offset >= 30) throw new RangeError('커서 마나 사각 삭제 목록 범위 오류');
                        put(fields, 12 + 4 * offset, 0);
                    }
                }
            }
            if (!processed) need(kill)();
        } else {
            const radius = F(fields, 0x90);
            if (!U(fields, 0x94)) {
                if (radius < F(fields, 0x8c)) {
                    const value = delta * 15.0 + radius;
                    putF(fields, 0x90, value >= F(fields, 0x8c) ? F(fields, 0x8c) : value);
                }
            } else {
                if (radius <= Math.fround(0.10000000149011612)) { need(kill)(); return; }
                const value = radius - delta * 30.0;
                putF(fields, 0x90, value < 0 ? 0 : value);
            }
            if (!U(fields, 0xd8)) {
                // 가상 해제/중심/이동이 다음 현재 시각·개수·부모를 바꿀 수 있어 반복마다 다시 읽는다.
                for (let index = 0; index < I(fields, 0x84); index += 1) {
                    const sid = this.marker(fields, index);
                    if (this.raw(sid)[11] & 3) { need(kill)(); return; }
                    need(h.detach)(sid, 0);
                    const integer = timeInteger(this.clock.now);
                    const angle = (Math.fround(6.28000020980835) / I(fields, 0x84)) * index + ((this.clock.now - integer) * Math.fround(3.140000104904175)) * 0.5;
                    const x = Math.fround(trig(angle, true) * F(fields, 0x90));
                    const y = Math.fround(trig(angle, false) * F(fields, 0x90));
                    const center = need(h.center)(this.parent(fields));
                    const point = this.clamp([Math.fround(center[0] + x), Math.fround(center[1] + y)]);
                    need(h.position)(sid, point[0], point[1], 0);
                }
            }
        }
        putD(fields, 0xc0, this.clock.now);
    }

    // 00473c30: event 1 의 숨김 값은 DWORD 전체를 유지하고 각 객체의 현재 void 여부를 읽는다.
    notify(fields, event, info, value) {
        const h = this.hooks;
        if (event === 3) { put(fields, 0x94, 1); return; }
        if (event !== 1) { putD(fields, 0xd0, this.clock.now + 3.0); return; }
        put(fields, 0xd8, info);
        // 효과가 개수/숨김/목록을 바꾸면 다음 조건에서 그 변경을 읽는다.
        for (let index = 0; index < I(fields, 0x84); index += 1) {
            const sid = this.marker(fields, index);
            const raw = this.raw(sid);
            if (!U(fields, 0xd8)) { if (raw[11] & 4) need(h.unpop)(sid, 0); } else if (!(raw[11] & 4)) need(h.detach)(sid, 0);
        }
    }

    // 00473b90: 원본은 커서 토큰을 먼저 지워 중첩 삭제에서 같은 프로세스를 재취득하지 않게 한다. 현재 목록과 현재 signed 개수를 유지하며 0/free/dead 는 원본처럼 건너뛴다.
    destroy(fields, token) {
        if ((this.cursor.cursorGump >>> 0) === (token >>> 0)) this.cursor.cursorGump = 0;
        for (let index = 0; index < I(fields, 0x84); index += 1) {
            const sid = this.marker(fields, index);
            if (sid && !(this.raw(sid)[11] & 3)) need(this.hooks.destroy)(sid, 0);
        }
        put(fields, 0, BASE_VTABLE); put(fields, 4, 0xffffcfc7); put(fields, 8, 0xffffcfc7);
    }
}

// 실제 Kernel 객체이다. 원본 사각형 Kill 뒤 쓰기를 안전하게 유지하도록 프레임 동안 필드를 같은 참조로 유지한다. (2026-11-15 WB04-E549)
export class CursorManaProcess extends SquidProcess {
    // 새 필드는 값 초기화하되 원본 몸체는 자신이 쓰는 바이트만 변경한다.
    constructor(logic) {
        super();
        this.logic = logic;
        this.fieldStore = createCursorManaFields();
    }

    // 독립 관찰/진단이 현재 단일 저장소를 읽는다.
    get fields() { return this.fieldStore; }

    // 호스트 포인터 대신 현재 Kernel 등록 번호를 UI 에 전달한다.
    get token() { return this.pid; }

    // 같은 현재 필드로 원본 frame 을 실행한다(자기 삭제 프레임의 꼬리 쓰기까지 필드가 살아 있다).
    runFrame() {
        const fields = this.fieldStore;
        this.logic.frame(fields, () => {
            if (!this.host) throw new Error('미부착 커서 마나 실행');
            this.host.kill(this, 0);
        });
    }

    // Baseprocess 의 실제 Form/부모 DWORD 를 필드에 반영한다(Attach 는 원본 생성자의 시각 초기화보다 먼저 발생한다).
    onAttached() { put(this.fieldStore, 4, this.form); put(this.fieldStore, 8, this.parent); }

    // 같은 가상 +1c 를 분배한다.
    notify(event, info, value) { this.logic.notify(this.fieldStore, event, info, value); }

    // 실제 표시 객체와 현재 커서 토큰을 원본 순서로 정리한다(Kernel 의 가상 소멸자).
    destroy() { this.logic.destroy(this.fieldStore, this.token); }
}

// 실제 new/Attach 를 실행한다. 부착 실패는 예외이며 게임 효과를 반환 0 으로 대체하지 않는다. (2026-11-15 WB04-E549)
export function createCursorManaProcess(host, logic, parent, form = 0) {
    if (host.pool !== logic.pool) throw new TypeError('커서 마나/Host SID 풀 불일치');
    const process = new CursorManaProcess(logic);
    logic.construct(process.fields, parent, form, (fields, actualParent, actualForm, flags) => {
        if (!host.attach(process, logic.processType, actualParent, flags, actualForm)) throw new Error('커서 마나 부모 Attach 실패');
    });
    return process;
}

// 현재 Kernel 번호의 객체가 SquidProcess 일 때만 돌려주고 다른 종류나 빈 슬롯은 null 이다(원본 dynamic_cast 에 해당). (2026-11-15 WB04-E549)
function currentSquidProcess(kernel, token) {
    const process = kernel.get(token);
    return process instanceof SquidProcess ? process : null;
}

// 제거/이벤트는 현재 Kernel 객체를 조회하므로 이후 사정거리 프로세스에도 같은 분배기를 쓴다. 사정거리 생성은 별도 몸체가 담당한다. (2026-11-15 WB04-E549)
export function bindCursorManaLifecycle(hooks, host, kernel, logic) {
    if (host.pool !== logic.pool) throw new TypeError('커서 마나/수명 SID 풀 불일치');
    hooks.createMana = (parent, form) => {
        if (form > 65535) throw new RangeError('커서 마나 Form WORD 범위 오류');
        const process = createCursorManaProcess(host, logic, parent, form);
        return process ? process.token : 0;
    };
    // 원본 포인터 토큰은 현재 Kernel 번호로만 바뀌며 실제 객체를 삭제한다.
    hooks.removeProcess = (token, flags) => {
        const process = currentSquidProcess(kernel, token);
        if (!host.isRegistered(token, process)) throw new Error('현재 커서 프로세스 Host 불일치');
        host.kill(process, flags);
    };
    // 원본 +1c 의 세 인자는 DWORD 폭으로 전달한다.
    hooks.processEvent = (token, event, info, value) => {
        const process = currentSquidProcess(kernel, token);
        if (!host.isRegistered(token, process)) throw new Error('현재 커서 이벤트 Host 불일치');
        process.notify(event, info, value);
    };
}

// 원본 가상 효과를 실제 표시/공간 수명에 넘기며 마나용 임시 좌표 사본을 만들지 않는다. world = { create, unpop, repop, pop, destroy, core() → { pool, bridgeState } } 이다. (2026-11-15 WB04-E549)
export function bindCursorManaWorld(hooks, world, center, notifySurface) {
    const core = world.core();
    hooks.create = (type, flags) => world.create(type, flags);
    hooks.setWord = (sid, word) => {
        const bytes = core.pool.allocated(sid);
        new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).setUint16(12, word & 0xffff, true);
        if (bytes[10] === core.bridgeState.bridgeType) need(notifySurface)(sid);
    };
    hooks.themeFrame = cursorManaThemeFrame;
    hooks.center = center;
    hooks.detach = (sid, flags) => world.unpop(sid, flags);
    hooks.unpop = (sid, flags) => world.repop(sid, flags);
    hooks.position = (sid, x, y, flags) => world.pop(sid, x, y, flags);
    hooks.destroy = (sid, flags) => world.destroy(sid, flags);
}
