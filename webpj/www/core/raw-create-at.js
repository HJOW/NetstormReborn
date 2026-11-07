// 대본 CreateAt 의 생성 부모(0044c660, Damageable.cpp)를 기존 위치 해석기와 실제 월드 경계에 조립하는 모듈이다(cpppj `o/RawCreateAt.cpp/.h`, 10.78 전용).
// createObjectAt(source, hooks): 권한(boss)이 없으면 진단 803 을 부른 뒤 권한을 다시 읽어 없으면 끝낸다. 첫 NUL 앞 문자열(256바이트 이상이면 원문 지역 버퍼 범위라 RangeError)을 위치 해석(parse: {x, y, type, radius, owner})해
//   x, y, signed(type) 가 모두 양수일 때만 계속한다. 그 뒤 create(type, 0)으로 만든 현재 SID 의 소유자를 정한다: 해석된 owner 가 0 이면 원래 정수 좌표의 지면(surface(x, y))이 있을 때만 그 지면 슬롯의 owner BYTE(+34)를 쓰고(없으면 0 유지),
//   생성 SID 가 0 이면 원문의 NULL 역참조를 호스트에서 `CreateAtNullObject` 로 명시 거부하되 선행 지면 조회는 보존한다. 이어 owner(생성, owner) → hasHitPoints 이면 최대 HP 를 hitPoints 로 직접 쓰고(사제 가상 HP 전환 없음) →
//   방향형 TYPE 이면 TYPE+15c 기준 offset = 0x15c + (radius/2, 0 쪽 절삭)×4 (DWORD 감김)의 frameField 값을 공간 등록 전에 raw+24 직접 쓰기(frame)로 쓴다 → pop(생성, float32(x), float32(y), 0) 이다.
//   미도달 하위는 요구하지 않고 앞선 생성/소유자/HP 효과는 되돌리지 않는다.
// createAtFrameField(frames, offset): 현재 TYPE 의 복원된 0x12c(글자 종류 수)/0x130..0x16f(글자 A..P 의 첫 프레임)/0x170..0x1af(개수) 물리 DWORD 를 읽는다. frames = { letterKinds, run(글자 코드) → { first, count } } 이다.
//   이웃 미복원 필드/포인터는 frame 0 으로 합성하지 않고 RangeError 로 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E31: CreateAt 부모 이식으로 추가)

// 원문 진단 줄, 위치 문자열 지역 버퍼 크기, TYPE+15c 방향 프레임 기준과 TYPE 프레임 표 구간(글자 종류 수/첫 프레임/개수)이다. (2026-11-08 WB04-E31)
const BOSS_ASSERTION = 803;
const SOURCE_BUFFER = 256;
const FRAME_BASE = 0x15c;
const KINDS_OFFSET = 0x12c;
const FIRST_OFFSET = 0x130;
const COUNT_OFFSET = 0x170;
const TABLE_END = 0x1b0;

// 원문의 NULL 생성 객체 가상 74 역참조를 호스트에서 명시 거부하는 오류이다(생성이 SID 0 을 돌려준 경우). (2026-11-08 WB04-E31)
export class CreateAtNullObject extends Error {
    // 사용: try { createObjectAt(...) } catch (error) { if (error instanceof CreateAtNullObject) ... }.
    constructor() {
        super('CreateAt 의 생성 객체가 NULL 입니다');
        this.name = 'CreateAtNullObject';
    }
}

// 도달한 하위의 누락만 거부한다. (2026-11-08 WB04-E31)
function need(device) {
    if (typeof device !== 'function') throw new Error('CreateAt 의 실제 현재 부모 미연결');
    return device;
}

// hooks = { boss, assertion(줄), parse(문자열), create(type, flags), surface(x, y), slot(sid), owner(sid, 소유자), hasHitPoints(sid), maximumHitPoints(sid), hitPoints(sid, 값), directional(type), frameField(sid, offset), frame(sid, 값), pop(sid, x, y, flags) } 이다. (2026-11-08 WB04-E31)
export function createObjectAt(source, hooks) {
    if (!need(hooks.boss)()) {
        need(hooks.assertion)(BOSS_ASSERTION);
        if (!need(hooks.boss)()) return;
    }
    const nul = source.indexOf('\0');
    const text = nul < 0 ? source : source.slice(0, nul);
    if (text.length >= SOURCE_BUFFER) throw new RangeError('CreateAt 위치 parser 의 원문 256바이트 지역 버퍼 범위');
    const spot = need(hooks.parse)(text);
    if (spot.x <= 0 || spot.y <= 0 || (spot.type | 0) <= 0) return;
    const created = need(hooks.create)(spot.type, 0);
    let owner = spot.owner;
    if (!owner) {
        // 지역/로컬 소유자를 새로 추론하지 않고 현재 지면 BYTE 만 사용한다.
        const terrain = need(hooks.surface)(spot.x, spot.y);
        if (terrain) {
            const slot = need(hooks.slot)(terrain);
            if (slot.length <= 34) throw new RangeError('CreateAt 의 실제 지면 owner BYTE 범위');
            owner = slot[34];
        }
    }
    if (!created) throw new CreateAtNullObject();
    need(hooks.owner)(created, owner);
    if (need(hooks.hasHitPoints)(created)) {
        const maximum = need(hooks.maximumHitPoints)(created);
        need(hooks.hitPoints)(created, maximum);
    }
    if (need(hooks.directional)(spot.type)) {
        // 원문 signed 0 방향 나눗셈 뒤 LEA 의 DWORD 감김이다. owner 인자가 아니다.
        const half = Math.trunc(spot.radius / 2);
        const offset = (FRAME_BASE + Math.imul(half, 4)) >>> 0;
        const value = need(hooks.frameField)(created, offset);
        need(hooks.frame)(created, value);
    }
    // 각 축은 FILD signed32 → FSTP32 한 번이며 x/y 를 뒤집지 않는다.
    need(hooks.pop)(created, Math.fround(spot.x), Math.fround(spot.y), 0);
}

// 원문 TYPE+15c 기준 인덱스가 첫 프레임/개수/종류 필드로 넘어가는 정상 물리도 보존한다. (2026-11-08 WB04-E31)
export function createAtFrameField(frames, offset) {
    if (offset === KINDS_OFFSET) return frames.letterKinds >>> 0;
    if (offset >= FIRST_OFFSET && offset < COUNT_OFFSET && (offset & 3) === 0) return frames.run(0x41 + (offset - FIRST_OFFSET) / 4).first >>> 0;
    if (offset >= COUNT_OFFSET && offset < TABLE_END && (offset & 3) === 0) return frames.run(0x41 + (offset - COUNT_OFFSET) / 4).count >>> 0;
    // 실제 raw TYPE 500바이트 전체와 32비트 포인터 ABI 를 합성하지 않는다. 앞선 생성/Owner/HP 는 남는다.
    throw new RangeError('CreateAt frame 이 참조한 TYPE 의 이웃 필드/포인터 물리 미복원');
}
