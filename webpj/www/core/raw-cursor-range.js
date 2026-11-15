// 10.78 의 커서 사정거리 표시(원문 474520 생성자·474b80 프레임·474c30 직선·474ed0 원형·474790 통지·474870 소멸)를 실제 SID 객체와 BaseProcess 수명으로 실행하는 모듈이다(cpppj `client/RawCursorRange.cpp/.h`, 10.78 전용).
// 사용:
//   const logic = new RawCursorRange(pool, types, clock, globals, cursor, hooks);
//     pool = SidPool(10.78), types = 타입 레코드 188개(`group`·`flags2`·`hotspotX/Y`), clock = SquidProcessState(현재 double 게임 시각 `now`),
//     globals = new CursorRangeGlobals(coordinates, targeting)(coordinates = 마나와 공유하는 CursorManaGlobals, targeting = { scaleX, scaleY, windArcherType, windHalfAngle }),
//     cursor = CursorDropState, hooks = { world: { create, detach, unpop, destroy, position(sid, x, y, flags), center(parent) → [x, y] }, range(typeNumber), firstContent(parent), mode(parent),
//       cannonDirection(query, direction), footprint(parent), directionFrame(parent, direction), firingOffset(type, frame) → [x, y], frameRun(type, variant) → { count, first }, diagnostic(color, text) }.
//   logic.construct(fields, parent, form, attach) → 부모의 group/범위/방향에 따라 직선(네 방향) 또는 원형(여덟 개) 표시 객체를 만들고 attach(fields, parent, form, 0x10) 호출 뒤 시각/상태를 초기화한다.
//   logic.frame(fields, kill) → 종료 기한을 검사하고 직선/원형 갱신 결과로 마지막 시각을 저장한다(kill 은 실제 Kill 이며 종료 뒤에도 필드가 살아 있어야 한다).
//   logic.notify(fields, event, info, value) → 3 은 종료 표식, 1 은 숨김 값(100칸 고정 SID0 건너뜀), 그 밖은 3초 뒤 기한.   logic.destroy(fields, token) → 커서 토큰 해제 뒤 100칸 중 free 가 아닌 객체를 삭제한다.
//   createCursorRangeProcess(host, logic, parent, form) / bindCursorRangeLifecycle(hooks, host, logic) → 수명 연결(제거/이벤트 분배는 마나 쪽 bindCursorManaLifecycle 가 같은 Kernel 번호로 처리한다).
// 필드는 원본 new(0x200) 바이트를 보존한다(100SID 목록·+1a4/1a8 개수·+1b0 사거리·+1d0/1d8/1e0 시각). float 저장 위치 외의 연산은 double 이며 선택한 경계의 누락은 그 위치에서 거부한다(Error '미연결').
// 원본 관측: `cursor-range-x86.tsv` 593입력(생성·프레임·통지·소멸·반복 호출의 이벤트·필드 512바이트·전역·raw 슬롯 102개·단계별 필드)을 tests/raw-cursor-range.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E550: 커서 사정거리 표시 RawCursorRange 이식)
import { SquidProcess } from './squid-process.js';
import { RawCursorMana } from './raw-cursor-mana.js';

// 비교용 원본 주소(호스트 함수 포인터가 아님)와 원본 필드 폭이다. (2026-11-15 WB04-E550)
const RANGE_VTABLE = 0x50c498;
const BASE_VTABLE = 0x5019ec;
export const CURSOR_RANGE_FIELD_BYTES = 0x200;
// 직선 모드의 네 방향(위·오른쪽·아래·왼쪽) 이동 단위이다(원본 0052f83c/0052f85c 의 짝수 방향). (2026-11-15 WB04-E550)
const DIRECTIONS = Object.freeze([[0, -1], [1, 0], [0, 1], [-1, 0]]);

// 선택된 실제 효과만 요구하며 빈 경계로 동작을 생략하지 않는다. (2026-11-15 WB04-E550)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('실제 커서 사정거리 경계 미연결');
    return effect;
}

// 원본 new(0x200) 필드 저장소를 만든다. (2026-11-15 WB04-E550)
export function createCursorRangeFields() { return { bytes: new Uint8Array(CURSOR_RANGE_FIELD_BYTES) }; }

// 필드 접근자들이다(비정렬 little endian, float/double 비트 패턴 유지). (2026-11-15 WB04-E550)
const view = (fields) => new DataView(fields.bytes.buffer, fields.bytes.byteOffset, fields.bytes.byteLength);
const U = (fields, offset) => view(fields).getUint32(offset, true);
const I = (fields, offset) => view(fields).getInt32(offset, true);
const F = (fields, offset) => view(fields).getFloat32(offset, true);
const D = (fields, offset) => view(fields).getFloat64(offset, true);
const put = (fields, offset, value) => view(fields).setUint32(offset, value >>> 0, true);
const putF = (fields, offset, value) => view(fields).setFloat32(offset, value, true);
const putD = (fields, offset, value) => view(fields).setFloat64(offset, value, true);
// raw 객체의 단정도 좌표를 비정렬 읽기한다. (2026-11-15 WB04-E550)
const rawFloat = (raw, offset) => new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
const timeInteger = (value) => RawCursorMana.timeInteger(value);
const trig = (angle, cosine) => RawCursorMana.trig(angle, cosine);

// 실제 468e70 은 360 을 그대로 허용하며 음수에서 한 바퀴를 더한 뒤 나머지를 구한다(입력은 단정도 값이다). (2026-11-15 WB04-E550)
function normalize(value) {
    if (value < 0) value = Math.fround(((1 - timeInteger(value / 360.0)) | 0) * 360.0 + value);
    if (value > 360) value = Math.fround(value % 360.0);
    return value;
}

// 원본 Angle::contains 의 원형 최소 구간은 양 끝점을 포함한다. (2026-11-15 WB04-E550)
function contains(value, first, last) {
    if (first > last) [first, last] = [last, first];
    return last - first <= 180 ? first <= value && value <= last : value <= first || value >= last;
}

// 각도의 짧은 쪽 거리를 원본의 중간 float 저장 없이 계산한다. (2026-11-15 WB04-E550)
function angleDistance(value, endpoint) {
    const distance = Math.abs(value - endpoint);
    return distance > 180 ? 360.0 - distance : distance;
}

// 사정거리 전역이다: coordinates 는 마나와 같은 맵 한계/대체점을, targeting 은 같은 픽셀 배율/Wind 타입/반각을 읽는다(사본을 만들지 않음). 객체/프로세스 번호는 005411f8/0054113c 이다. (2026-11-15 WB04-E550)
export class CursorRangeGlobals {
    rangeType = 104;
    processType = 51;
    constructor(coordinates, targeting) {
        this.coordinates = coordinates;
        this.targeting = targeting;
    }
}

// 생성·직선/원형 프레임·이벤트·소멸의 실제 몸체를 같은 현재 필드에 적용하는 논리이다. 모든 참조와 효과는 연결한 프로세스보다 오래 살아야 한다. (2026-11-15 WB04-E550)
export class RawCursorRange {
    // 다른 판본의 raw 오프셋이나 TYPE 번호 표를 혼용하지 않는다.
    constructor(pool, types, clock, globals, cursor, hooks) {
        if (pool.edition !== 'patch1078' || types.length !== 188) throw new TypeError('사정거리 10.78 Pool/TYPE 계약 오류');
        this.pool = pool;
        this.types = types;
        this.clock = clock;
        this.globals = globals;
        this.cursor = cursor;
        this.hooks = hooks;
    }

    // 실제 풀/프로세스 종류를 Host 연결 시 확인한다.
    get processType() { return this.globals.processType; }

    // 468e70 각도 정규화(단정도 값 입력)이다. 직접 값 검사용으로 공개한다. (2026-11-15 WB04-E550)
    static normalize(value) { return normalize(value); }

    // 원본 Angle::contains 의 원형 최소 구간 포함 판정이다(양 끝점 포함). 직접 값 검사용으로 공개한다. (2026-11-15 WB04-E550)
    static contains(value, first, last) { return contains(value, first, last); }

    // 각도의 짧은 쪽 거리이다. 직접 값 검사용으로 공개한다. (2026-11-15 WB04-E550)
    static angleDistance(value, endpoint) { return angleDistance(value, endpoint); }

    // 도달한 raw 슬롯만 호스트 범위 안에서 읽는다(접근 시점마다 현재 슬롯의 사본).
    raw(sid) {
        if (sid < 5 || sid >= this.pool.capacity) throw new RangeError('사정거리 SID 범위 오류');
        return this.pool.slot(sid);
    }

    // Baseprocess 는 현재 부모의 종속 머리가 살아 있음을 진단한다.
    parent(fields) {
        const value = U(fields, 8);
        if (value >= this.pool.capacity) throw new RangeError('사정거리 부모 SID 범위 오류');
        const raw = this.raw(value);
        if (!(raw[6] || raw[7])) throw new Error('parent->getHead() != 0 원본 사정거리 진단');
        return value;
    }

    // 원본 목록의 DWORD SID 를 16비트로 조용히 절단하지 않는다(100칸).
    marker(fields, index) {
        if (index < 0 || index >= 100) throw new RangeError('사정거리 100칸 목록 범위 오류');
        const value = U(fields, 20 + 4 * index);
        if (value >= this.pool.capacity) throw new RangeError('사정거리 목록 SID 범위 오류');
        return value;
    }

    // NaN 과 맵 끝점도 원본의 엄격한 부등식을 통과하지 못한다.
    clamp(point) {
        const coordinates = this.globals.coordinates;
        const limit = Math.fround(coordinates.mapLimit | 0);
        if (!(point[0] > 0 && point[0] < limit && point[1] > 0 && point[1] < limit)) {
            const invalid = Math.fround(coordinates.invalidPoint);
            return [invalid, invalid];
        }
        return point;
    }

    // 방향 0/2/4/6 의 현재 8 DWORD 문맥과 생성 순서를 유지한다(474280).
    createRays(fields, parent) {
        const h = this.hooks;
        const g = this.globals;
        need(h.footprint)(parent);
        // 펜스(group 6)는 같은 문맥을 읽지만 Cannon 방향 제한을 적용하지 않는다.
        for (let direction = 0; direction < 8; direction += 2) {
            let raw = this.raw(parent);
            const query = { x: rawFloat(raw, 14), y: rawFloat(raw, 18), type: raw[10], direction: need(h.mode)(parent) | 0 };
            raw = this.raw(parent);
            query.owner = raw[34];
            query.frame = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getInt32(36, true);
            const center = need(h.world.center)(parent);
            query.centerX = center[0];
            query.centerY = center[1];
            put(fields, 0x1f0 + 2 * direction, U(fields, 0x10) || need(h.cannonDirection)(query, direction) ? 1 : 0);
        }
        put(fields, 0x1a4, 0);
        put(fields, 0x1ac, 0);
        // 목록은 방향별 현재 변 길이로 색인하고 원본처럼 1a4 는 0 인 채 생성 수만 1ac 에 더한다.
        for (let side = 0; side < 4; side += 1) {
            // 생성/위치 콜백 뒤 다음 조건도 현재 개수를 읽는다.
            for (let item = 0; item < I(fields, 0x1a8); item += 1) {
                if (!U(fields, 0x1f0 + 4 * side)) continue;
                const index = I(fields, 0x1a8) * side + item;
                if (index < 0 || index >= 100) throw new RangeError('사정거리 직선 생성 목록 범위 오류');
                const sid = need(h.world.create)(g.rangeType, 2);
                this.raw(sid);
                this.pool.allocated(sid)[40] |= 8;
                put(fields, 20 + 4 * index, sid);
                put(fields, 0x1ac, U(fields, 0x1ac) + 1);
                need(h.world.position)(sid, Math.fround(g.coordinates.invalidPoint), Math.fround(g.coordinates.invalidPoint), 0);
            }
        }
        putF(fields, 0x1c0, 15);
    }

    // 원형 표시 8개는 현재 A/B 첫 프레임을 적용한 뒤 실제 가상 Pop 에 넘긴다(474450).
    createCircle(fields) {
        const h = this.hooks;
        const g = this.globals;
        put(fields, 0x1a4, 8);
        put(fields, 0x1ac, 8);
        // 각 생성/위치 효과 뒤 현재 개수·변형을 다시 읽는다.
        for (let index = 0; index < I(fields, 0x1a4); index += 1) {
            if (index >= 100) throw new RangeError('사정거리 원형 생성 목록 범위 오류');
            const sid = need(h.world.create)(g.rangeType, 2);
            const raw = this.pool.allocated(sid);
            raw[40] |= 8;
            const first = need(h.frameRun)(raw[10], U(fields, 0x1ec)).first;
            new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(36, first >>> 0, true);
            put(fields, 20 + 4 * index, sid);
            need(h.world.position)(sid, Math.fround(g.coordinates.invalidPoint), Math.fround(g.coordinates.invalidPoint), 0);
        }
    }

    // 474520: 현재 TYPE 포인터와 초기 부모 주소를 유지하되 콜백 뒤 필요한 필드는 다시 읽는다.
    construct(fields, parent, form, attach) {
        const h = this.hooks;
        const g = this.globals;
        put(fields, 0, RANGE_VTABLE); put(fields, 4, 0); put(fields, 8, 0);
        putF(fields, 0x1c4, -1); putF(fields, 0x1c8, -1);
        putD(fields, 0x1d0, 0); putD(fields, 0x1d8, 0); putD(fields, 0x1e0, 0);
        let raw = this.raw(parent);
        const type = raw[10];
        if (type >= this.types.length) throw new RangeError('사정거리 부모 TYPE 범위 오류');
        const definition = this.types[type];
        put(fields, 12, 1);
        put(fields, 16, definition.group === 6 ? 1 : 0);
        if (definition.group === 1 || U(fields, 16)) put(fields, 12, 0);
        if (raw[10] === (g.targeting.windArcherType >>> 0)) {
            const mode = need(h.mode)(parent) >>> 0;
            const angle = normalize(Math.fround(mode * 45.0));
            putF(fields, 0x1c4, normalize(normalize(Math.fround(angle - g.targeting.windHalfAngle))));
            putF(fields, 0x1c8, normalize(normalize(Math.fround(g.targeting.windHalfAngle + angle))));
            putF(fields, 0x1c4, normalize(F(fields, 0x1c4)));
            putF(fields, 0x1c8, normalize(F(fields, 0x1c8)));
            putF(fields, 0x1cc, (360.0 - angleDistance(F(fields, 0x1c4), F(fields, 0x1c8))) * 0.5);
        }
        raw = this.raw(parent);
        const currentType = raw[10];
        if (currentType >= this.types.length) throw new RangeError('사정거리 현재 TYPE 범위 오류');
        let rangeType = currentType;
        if (this.types[currentType].flags2 & 0x30000) {
            const content = this.raw(need(h.firstContent)(parent));
            rangeType = content[18] | (content[19] << 8);
        }
        putF(fields, 0x1b0, need(h.range)(rangeType));
        putF(fields, 0x1bc, 5);
        put(fields, 0x1a8, timeInteger(F(fields, 0x1b0) * Math.fround(0.20000000298023224)) >>> 0);
        // 목록 100칸만 0 으로 초기화하고 원본의 다른 padding 을 유지한다.
        for (let index = 0; index < 100; index += 1) put(fields, 20 + 4 * index, 0);
        put(fields, 0x1ec, (definition.flags2 & 0x100) ? 1 : 0);
        if (!U(fields, 12)) this.createRays(fields, parent);
        else if (U(fields, 12) === 1) this.createCircle(fields);
        putF(fields, 0x1b4, 0);
        need(attach)(fields, parent, form, 0x10);
        put(fields, 0x1b8, 0);
        putD(fields, 0x1d0, this.clock.now);
        putD(fields, 0x1d8, this.clock.now);
        putD(fields, 0x1e0, 0);
        put(fields, 0x1e8, 0);
    }

    // 종료 시 preview extra8 은 다른 현재 토큰도 지우므로 소멸자의 동일성 검사로 대체하지 않는다.
    stop(fields, kill) {
        if (this.raw(this.parent(fields))[40] & 8) this.cursor.cursorGump = 0;
        need(kill)();
    }

    // 474c30: 실제 직선 간격·이동 위상·프레임을 현재 자료로 계산한다. 종료했으면 false 이다.
    rays(fields, kill) {
        const h = this.hooks;
        const g = this.globals;
        const step = Math.fround(F(fields, 0x1b0) / I(fields, 0x1a8));
        const grown = (this.clock.now - D(fields, 0x1d8)) * F(fields, 0x1c0);
        const visible = Math.fround(grown);
        const phase = Math.fround(grown % step);
        if (U(fields, 0x1b8)) { this.stop(fields, kill); return false; }
        const parent = this.parent(fields);
        const type = this.raw(parent)[10];
        if (type >= this.types.length) throw new RangeError('사정거리 직선 TYPE 범위 오류');
        const definition = this.types[type];
        const parentRaw = this.raw(this.parent(fields));
        const x = rawFloat(parentRaw, 14);
        const y = rawFloat(parentRaw, 18);
        need(h.footprint)(this.parent(fields));
        // 네 방향의 현재 부모 가상 프레임과 발사구를 각각 읽는다.
        for (let side = 0; side < 4; side += 1) {
            const frame = need(h.directionFrame)(this.parent(fields), side * 2);
            const offset = need(h.firingOffset)(type, frame);
            const originX = Math.fround(x + offset[0] / g.targeting.scaleX);
            const originY = Math.fround(y + offset[1] / g.targeting.scaleY);
            const beginX = Math.fround(originX - definition.hotspotX / g.targeting.scaleX);
            const beginY = Math.fround(originY - definition.hotspotY / g.targeting.scaleY);
            // 숨김/미허용이어도 현재 Run 을 원본처럼 읽는다.
            for (let item = 0; item < I(fields, 0x1a8); item += 1) {
                const variant = U(fields, 0x1ec);
                const count = need(h.frameRun)(g.rangeType, variant).count;
                const first = need(h.frameRun)(g.rangeType, variant).first;
                const product = item * step;
                if (U(fields, 0x1f0 + 4 * side) && !U(fields, 0x1e8) && product < visible) {
                    const sid = this.marker(fields, I(fields, 0x1a8) * side + item);
                    if (!(this.raw(sid)[11] & 4)) need(h.world.detach)(sid, 0);
                    const distance = Math.fround(Math.fround(product) + phase);
                    const point = this.clamp([Math.fround(DIRECTIONS[side][0] * distance + beginX), Math.fround(DIRECTIONS[side][1] * distance + beginY)]);
                    if (count <= 0) throw new Error('사정거리 실제 Run 개수 오류');
                    const animation = (timeInteger(distance * 2.0) % count) + first;
                    const bytes = this.pool.allocated(sid);
                    new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).setUint32(36, animation >>> 0, true);
                    need(h.world.position)(sid, point[0], point[1], 0x10);
                }
            }
        }
        return true;
    }

    // 474ed0: 실제 원형 반경과 Wind 관측각의 근사 상수를 유지하고 각 효과 뒤 현재 상태를 읽는다. 종료했으면 false 이다.
    circle(fields, delta, kill) {
        const h = this.hooks;
        const radius = F(fields, 0x1b4);
        if (!U(fields, 0x1b8)) {
            if (radius < F(fields, 0x1b0)) {
                const value = delta * 15.0 + radius;
                putF(fields, 0x1b4, value >= F(fields, 0x1b0) ? F(fields, 0x1b0) : value);
            }
        } else {
            if (radius <= Math.fround(0.10000000149011612)) { this.stop(fields, kill); return false; }
            const value = radius - delta * 30.0;
            putF(fields, 0x1b4, value <= 0 ? 0 : value);
        }
        if (!U(fields, 0x1e8)) {
            // 반복마다 현재 개수와 raw dead/void 를 읽는다.
            for (let index = 0; index < I(fields, 0x1a4); index += 1) {
                const sid = this.marker(fields, index);
                const raw = this.raw(sid);
                if (raw[11] & 2) { need(h.diagnostic)(29, 'Dead circle range!'); continue; }
                if (!(raw[11] & 4)) need(h.world.detach)(sid, 0);
                const phase = F(fields, 0x1b4) === F(fields, 0x1b0) ? (this.clock.now - timeInteger(this.clock.now)) * 3.1415927410125732 : 0.0;
                let angle = Math.fround((Math.fround(6.283180236816406) / I(fields, 0x1a4)) * index + phase);
                const degrees = normalize(Math.fround((Math.fround(1.5707900524139404) - angle) * 57.29583));
                let factor = 1;
                let theta = angle;
                if (F(fields, 0x1c4) >= 0 && F(fields, 0x1c4) < 360 && !contains(degrees, F(fields, 0x1c4), F(fields, 0x1c8))) {
                    angle = angleDistance(degrees, F(fields, 0x1c8)) <= angleDistance(degrees, F(fields, 0x1c4)) ? F(fields, 0x1c8) : F(fields, 0x1c4);
                    factor = 1.0 - angleDistance(degrees, angle) / F(fields, 0x1cc);
                    theta = Math.fround(1.5707900524139404) - angle * Math.fround(0.01744999922811985);
                }
                const x = Math.fround((trig(theta, true) * factor) * F(fields, 0x1b4));
                const y = Math.fround(-(trig(theta, false) * factor) * F(fields, 0x1b4));
                const center = need(h.world.center)(this.parent(fields));
                const point = this.clamp([Math.fround(center[0] + x), Math.fround(center[1] + y)]);
                need(h.world.position)(sid, point[0], point[1], 0);
            }
        }
        return true;
    }

    // 474b80: 내부 frame 은 현재 값을 다시 읽으므로 바깥 몸체는 시간 차와 종료 시각을 계산한다.
    frame(fields, kill) {
        this.raw(this.parent(fields));
        const delta = Math.fround(this.clock.now - D(fields, 0x1d0));
        if (D(fields, 0x1e0) !== 0 && D(fields, 0x1e0) < this.clock.now) put(fields, 0x1b8, 1);
        const mode = U(fields, 12);
        if ((!mode && !this.rays(fields, kill)) || (mode === 1 && !this.circle(fields, delta, kill))) return;
        putD(fields, 0x1d0, this.clock.now);
    }

    // 474790: event 1 은 100칸 고정으로 SID0 을 건너뛴다. 마나의 현재 개수 반복과 구별한다.
    notify(fields, event, info, value) {
        const h = this.hooks;
        if (event === 3) { put(fields, 0x1b8, 1); return; }
        if (event !== 1) { putD(fields, 0x1e0, this.clock.now + 3.0); return; }
        put(fields, 0x1e8, info);
        // 실제 가상 효과 뒤 다음 칸과 현재 숨김 값을 읽는다.
        for (let index = 0; index < 100; index += 1) {
            const sid = this.marker(fields, index);
            if (!sid) continue;
            const raw = this.raw(sid);
            if (!U(fields, 0x1e8)) { if (raw[11] & 4) need(h.world.unpop)(sid, 0); } else if (!(raw[11] & 4)) need(h.world.detach)(sid, 0);
        }
    }

    // 474870: 원본 소멸자는 free 만 제외한다. dead 도 제외하는 마나와 구별한다.
    destroy(fields, token) {
        if ((this.cursor.cursorGump >>> 0) === (token >>> 0)) this.cursor.cursorGump = 0;
        // 고정 100칸의 현재 SID/free 를 읽고 선택한 가상 삭제를 실행한다.
        for (let index = 0; index < 100; index += 1) {
            const sid = this.marker(fields, index);
            if (sid && !(this.raw(sid)[11] & 1)) need(this.hooks.world.destroy)(sid, 0);
        }
        put(fields, 0, BASE_VTABLE); put(fields, 4, 0xffffcfc7); put(fields, 8, 0xffffcfc7);
    }
}

// 실제 Kernel 객체이다. 프레임/소멸이 같은 필드를 공유하며 자기 삭제 뒤 꼬리 쓰기까지 필드가 살아 있다. (2026-11-15 WB04-E550)
export class CursorRangeProcess extends SquidProcess {
    constructor(logic) {
        super();
        this.logic = logic;
        this.fieldStore = createCursorRangeFields();
    }

    // 검사/진단이 현재 단일 저장소를 읽는다.
    get fields() { return this.fieldStore; }

    // 호스트 주소 대신 Kernel 번호를 UI 에 준다.
    get token() { return this.pid; }

    // 몸체에서 즉시 자기 삭제해도 현재 호출의 필드는 살아 있다.
    runFrame() {
        this.logic.frame(this.fieldStore, () => {
            if (!this.host) throw new Error('미부착 사정거리 실행');
            this.host.kill(this, 0);
        });
    }

    // 실제 부착 결과를 원본 DWORD 폭으로 반영한다.
    onAttached() { put(this.fieldStore, 4, this.form); put(this.fieldStore, 8, this.parent); }

    // 같은 원본 이벤트를 분배한다.
    notify(event, info, value) { this.logic.notify(this.fieldStore, event, info, value); }

    // 실제 마커와 커서 토큰을 정리한다(Kernel 의 가상 소멸자).
    destroy() { this.logic.destroy(this.fieldStore, this.token); }
}

// 실제 new/Attach 를 실행한다. 부착 실패는 예외이며 게임 효과를 반환 0 으로 대체하지 않는다. (2026-11-15 WB04-E550)
export function createCursorRangeProcess(host, logic, parent, form = 0) {
    if (host.pool !== logic.pool) throw new TypeError('사정거리/Host SID 풀 불일치');
    const process = new CursorRangeProcess(logic);
    logic.construct(process.fields, parent, form, (fields, actualParent, actualForm, flags) => {
        if (!host.attach(process, logic.processType, actualParent, flags, actualForm)) throw new Error('사정거리 부모 Attach 실패');
    });
    return process;
}

// 같은 수명의 remove/event 는 마나 쪽 공유 분배를 사용하고 createRange 만 추가한다. (2026-11-15 WB04-E550)
export function bindCursorRangeLifecycle(hooks, host, logic) {
    if (host.pool !== logic.pool) throw new TypeError('사정거리/수명 SID 풀 불일치');
    hooks.createRange = (parent, form) => {
        if (form > 65535) throw new RangeError('사정거리 Form WORD 범위 오류');
        return createCursorRangeProcess(host, logic, parent, form).token;
    };
}
