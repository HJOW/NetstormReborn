// 훨리기그의 전용 폭탄 투하/초기화/통지/장식 프레임을 같은 136바이트 원문 저장소에서 수행하는 모듈이다(cpppj `o/RawSunFlyerFrame.cpp/.h`, 10.78 전용: 45bd00 초기화·45bd60 통지·45be00 폭탄 정리·45beb0 홈 통지·45be40 장식 프레임·45d2c0 전용 Frame·45e170 복귀 지연).
//  - new RawSunFlyerFrame(pool, shapes, types, globals, hooks): pool 은 같은 PATCH SidPool, shapes = 188칸 SHP({ loaded, frameCount, frames:[{ cellWidth }] })의 살아 있는 참조, types = 188칸 폭탄 TYPE({ fallingFrames, explosionFirst, explosionEnd, sound }, 만들 때 사본),
//    globals = { boss(), paused(), now(), bombType() } 는 모두 현재 값을 읽는 함수이고,
//    hooks = { assertion(line), robust(sid, live)→bool, destroy(sid, flags), unpop(sid, flags), pop(sid, x, y, flags), repop(sid, flags), mayTarget(parent, target)→bool, create(type, flags)→SID, sound(x, y, token), runCount(sid)→int32,
//    baseFrame(fields), baseNotify(fields, message, info1, info2), baseParentDying(fields) } 이다. 선택하지 않은 경계는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존하고 거부한다.
//  - 필드는 E352 의 `FlyerProcessFields`(112바이트 `bytes` + 24바이트 `sunTail`)를 하나의 136바이트 원문으로 읽고 쓴다(두 배열을 이어진 포인터로 취급하지 않음). +96 폭탄 번호·+104 투하 시각·+112 다음 투하·+120 다음 폭발 프레임·+128/+132 저장 좌표.
//  - initialize(fields): 45bd00 — 투하 시각 0·폭탄 0·저장 좌표 −10000·공격/착륙 0·최초 20초 타이머 = now + 20·초기화 표식 1. notify(fields, message, info1, info2): 45bd60 — 메시지 1 만 전용(info1 0 이면 공격 해제·다음 투하 0, 아니면 다음 투하 = now + 2), 그 밖은 공통 통지.
//  - clearBomb(fields): 45be00 — 폭탄 번호가 0..65535 이면 유효할 때 삭제(0x10)하고 항상 번호를 0 으로 쓴다. parentDying(fields): 45beb0 — 폭탄 정리 뒤 공통 홈 통지.
//  - frame(fields): 45d2c0 — 초기화 → 부모 정지면(boss 삭제) 반환 → 폭탄이 있고 정지가 아니면 낙하(프레임 < 낙하 프레임: 시간 경과 제곱 낙하 위치·바닥에 닿으면 폭발 첫 프레임)/폭발(기한마다 프레임 +1, 끝이면 정리)과 부모 복원 →
//    발사 가능·다음 투하 기한·폭탄 없음이면 새 폭탄 생성/투하/소리 → 공통 Frame. animation(fields): 45be40 — 두 번의 fmod 와 float 저장으로 장식 프레임. ReturnDelay: 45e170 의 float 60.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E357: 7-P1 RawSunFlyerFrame 이식으로 추가)

// 원본 outer 기준의 선택 필드 위치(폭탄·투하 시각·다음 투하·다음 폭발 프레임·저장 좌표)이다. (2026-11-13 WB04-E357)
const BOMB = 96, DROP_TIME = 104, NEXT_DROP = 112, NEXT_FRAME = 120, SAVED_X = 128, SAVED_Y = 132;
// 보호 PE 가 공급하는 장식 주기/역수·폭발 프레임 간격(double)과 본체 복귀량(float)이다. (2026-11-13 WB04-E357)
const PERIOD = 0.14535599946975708;
const INVERSE_PERIOD = 6.8796610832214355;
const EXPLOSION_DELAY = 0.03999999910593033;
const RESTORE = Math.fround(0.05000000074505806);
// 전용 필드 전체 크기(기본 112 + 꼬리 24)와 기본 영역 크기이다. (2026-11-13 WB04-E357)
const FIELD_SIZE = 136;
const BASE_SIZE = 112;
// TYPE 표 크기(PATCH)이다. (2026-11-13 WB04-E357)
const TYPE_COUNT = 188;

// float/double 비트 변환용 공용 보기이다. (2026-11-13 WB04-E357)
const SCRATCH = new DataView(new ArrayBuffer(8));

// 비정렬 원본 슬롯의 DWORD 만 읽는다. (2026-11-13 WB04-E357)
function rawRead(raw, at) {
    let value = 0;
    // 원본 little endian 네 바이트이다.
    for (let index = 0; index < 4; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 원본 슬롯의 DWORD 만 바꾸며 인접 필드는 유지한다. (2026-11-13 WB04-E357)
function rawWrite(raw, at, value) {
    // 현재 네 바이트의 낮은 자리부터 쓴다.
    for (let index = 0; index < 4; index += 1) raw[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// 분리한 두 배열을 넘는 호스트 포인터 산술 없이 실제 136바이트 필드를 읽는다. (2026-11-13 WB04-E357)
function read(fields, at) {
    if (at + 4 > FIELD_SIZE) throw new RangeError('훨리기그 필드 범위 오류');
    let value = 0;
    // 각 바이트가 속한 원본 영역을 선택한다.
    for (let index = 0; index < 4; index += 1) {
        const offset = at + index;
        value += (offset < BASE_SIZE ? fields.bytes[offset] : fields.sunTail[offset - BASE_SIZE]) * 2 ** (8 * index);
    }
    return value;
}

// 원본 DWORD 폭만 선택 영역에 쓴다. (2026-11-13 WB04-E357)
function write(fields, at, value) {
    if (at + 4 > FIELD_SIZE) throw new RangeError('훨리기그 필드 범위 오류');
    // 두 저장소를 이어진 포인터로 취급하지 않는다.
    for (let index = 0; index < 4; index += 1) {
        const offset = at + index;
        const byte = Math.floor(value / 2 ** (8 * index)) & 0xff;
        if (offset < BASE_SIZE) fields.bytes[offset] = byte;
        else fields.sunTail[offset - BASE_SIZE] = byte;
    }
}

// 실제 두 DWORD 의 double 저장 비트를 읽는다. (2026-11-13 WB04-E357)
function time(fields, at) {
    SCRATCH.setUint32(0, read(fields, at), true);
    SCRATCH.setUint32(4, read(fields, at + 4), true);
    return SCRATCH.getFloat64(0, true);
}

// 원본 double 저장만 수행한다. (2026-11-13 WB04-E357)
function putTime(fields, at, value) {
    SCRATCH.setFloat64(0, value, true);
    write(fields, at, SCRATCH.getUint32(0, true));
    write(fields, at + 4, SCRATCH.getUint32(4, true));
}

// float 비트(DWORD)를 float 로 읽는다. (2026-11-13 WB04-E357)
function floatOf(bitsValue) {
    SCRATCH.setUint32(0, bitsValue, true);
    return SCRATCH.getFloat32(0, true);
}

// float 값의 비트(DWORD)이다. (2026-11-13 WB04-E357)
function bitsOf(value) {
    SCRATCH.setFloat32(0, value, true);
    return SCRATCH.getUint32(0, true);
}

// 현재 raw 위치의 binary32 성분이다. (2026-11-13 WB04-E357)
function position(raw, at) {
    return floatOf(rawRead(raw, at));
}

// 실제 DWORD 폭의 폭탄 번호를 호스트 SID 범위로 확인한다. (2026-11-13 WB04-E357)
function bombOf(fields) {
    const value = read(fields, BOMB);
    if (value > 65535) throw new RangeError('훨리기그 폭탄 SID 범위 오류');
    return value;
}

// 원본 signed 프레임 증가의 DWORD 감김이다. (2026-11-13 WB04-E357)
function increment(value) {
    return (value + 1) | 0;
}

// 원본 idiv 정상 입력을 실행하며 0/오버플로 경로를 명시 오류로 남긴다(−0 은 0). (2026-11-13 WB04-E357)
function remainder(value, divisor) {
    if (!divisor || (value === -2147483648 && divisor === -1)) throw new RangeError('훨리기그 원본 idiv 무효 경로');
    return (value % divisor) | 0;
}

// 선택된 경계만 실제 호출 직전에 요구한다. (2026-11-13 WB04-E357)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('훨리기그 실제 하위 몸체 미연결');
    return hook;
}

// 훨리기그 전용 프레임 로직이다. (2026-11-13 WB04-E357)
export class RawSunFlyerFrame {
    #pool;
    #shapes;
    #types;
    #globals;
    #hooks;

    // 다른 판본/타입 번호/셰이프 자료를 혼용하지 않는다. SHP 는 살아 있는 참조, 폭탄 TYPE 은 만들 때 사본이다. (2026-11-13 WB04-E357)
    constructor(pool, shapes, types, globals, hooks) {
        if (pool?.edition !== 'patch1078' || shapes?.length !== TYPE_COUNT || types?.length !== TYPE_COUNT) throw new TypeError('훨리기그 Frame PATCH 자료 불일치');
        // 모든 전역은 처리 도중 바뀔 수 있어 값이 아니라 현재 값 조회 함수여야 한다.
        for (const name of ['boss', 'paused', 'now', 'bombType']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`훨리기그 현재 전역 조회기 누락: ${name}`);
        }
        this.#pool = pool;
        this.#shapes = shapes;
        this.#types = types.map((type) => ({ fallingFrames: type.fallingFrames, explosionFirst: type.explosionFirst, explosionEnd: type.explosionEnd, sound: type.sound }));
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // Host 가 같은 풀인지 확인할 수 있게 공개한다. (2026-11-13 WB04-E357)
    get pool() { return this.#pool; }

    // 번호에 해당하는 폭탄 TYPE 사본이다(표 밖은 거부). (2026-11-13 WB04-E357)
    #type(number) {
        const type = this.#types[number];
        if (!type) throw new RangeError('훨리기그 폭탄 TYPE 번호가 표 밖입니다');
        return type;
    }

    // 생성자가 미정으로 둔 위치 두 성분은 전용 첫 초기화에서만 −10000 으로 쓴다. (2026-11-13 WB04-E357)
    initialize(fields) {
        putTime(fields, DROP_TIME, 0);
        write(fields, BOMB, 0);
        write(fields, SAVED_Y, bitsOf(-10000));
        write(fields, SAVED_X, bitsOf(-10000));
        write(fields, 80, 0);
        write(fields, 88, 0);
        putTime(fields, 72, this.#globals.now() + 20);
        write(fields, 84, 1);
    }

    // 일반 메시지 1 은 전용 발사/정지이며 그 밖은 실제 공통 통지로 넘긴다. (2026-11-13 WB04-E357)
    notify(fields, message, info1, info2) {
        if (message !== 1) {
            need(this.#hooks.baseNotify)(fields, message, info1, info2);
            return;
        }
        if (!info1) {
            write(fields, 80, 0);
            putTime(fields, NEXT_DROP, 0);
        } else putTime(fields, NEXT_DROP, this.#globals.now() + 2);
    }

    // 삭제 효과가 현재 폭탄 필드를 바꿔도 원본은 마지막에 0 을 다시 쓴다. (2026-11-13 WB04-E357)
    clearBomb(fields) {
        const value = read(fields, BOMB);
        // 4ac140 은 전체 DWORD 가 1..fffe 밖이면 부수효과 없이 0 이며 첫 Frame 전 미정값도 포함한다.
        if (value <= 65535) {
            if (need(this.#hooks.robust)(value, true)) need(this.#hooks.destroy)(value, 0x10);
        }
        write(fields, BOMB, 0);
    }

    // 폭탄 정리 뒤에는 같은 현재 필드를 공통 홈 통지에 전달한다. (2026-11-13 WB04-E357)
    parentDying(fields) {
        this.clearBomb(fields);
        need(this.#hooks.baseParentDying)(fields);
    }

    // frameCheck 와 실제 현재 물리 프레임의 첫 float 를 사용한다. (2026-11-13 WB04-E357)
    #width(parent) {
        const raw = this.#pool.slot(parent);
        const type = raw[10];
        if (type >= this.#shapes.length) throw new RangeError('훨리기그 SHP TYPE 범위 오류');
        const shape = this.#shapes[type];
        const frame = rawRead(raw, 36);
        if (!shape.loaded || frame >= shape.frames.length) throw new RangeError('훨리기그 현재 SHP 프레임 오류');
        return shape.frames[frame].cellWidth;
    }

    // 전체 전용 폭탄/부모/타이머 분기를 수행한 뒤에만 기존 공통 Frame 을 실행한다. (2026-11-13 WB04-E357)
    frame(fields) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const globals = this.#globals;
        if (!read(fields, 84)) this.initialize(fields);
        const parent = fields.parent;
        // 첫 부모/폭탄 TYPE 포인터 snapshot 이다.
        const bombType = this.#type(globals.bombType());
        if (pool.slot(parent)[40] & 0x20) {
            if (globals.boss()) need(hooks.destroy)(parent, 0x200000);
            return;
        }
        // pause 관문보다 먼저 읽은 현재 폭탄이다.
        const bomb = bombOf(fields);
        if (bomb && !globals.paused()) {
            if (!(pool.slot(bomb)[11] & 4)) need(hooks.unpop)(bomb, 0);
            // Unpop 뒤 현재 프레임이다.
            const frame = rawRead(pool.slot(bomb), 36) | 0;
            if (frame < bombType.fallingFrames) {
                // 원본 세 snapshot/float 저장이다.
                const before = pool.slot(parent);
                const x = position(before, 14);
                const ceiling = Math.fround(position(before, 18) + 4);
                const elapsed = Math.fround(globals.now() - time(fields, DROP_TIME));
                const left = Math.fround(x - this.#width(parent) * 0.5);
                const displacement = (elapsed * 9 + 10) * elapsed;
                // 넓은 값을 비교 전 float 로 좁히지 않는다.
                const candidate = displacement + position(pool.slot(parent), 18);
                let y = ceiling;
                if (!(candidate > ceiling)) y = Math.fround(displacement + position(pool.slot(parent), 18));
                rawWrite(pool.allocated(bomb), 36, remainder(increment(frame), bombType.fallingFrames) >>> 0);
                need(hooks.pop)(bomb, left, y, 0);
                if (y >= ceiling) rawWrite(pool.allocated(bomb), 36, bombType.explosionFirst >>> 0);
            } else {
                if (time(fields, NEXT_FRAME) <= globals.now()) {
                    const next = increment(frame);
                    rawWrite(pool.allocated(bomb), 36, next >>> 0);
                    if (next >= bombType.explosionEnd) {
                        this.clearBomb(fields);
                        return;
                    }
                    pool.allocated(bomb)[35] = 0xed;
                    putTime(fields, NEXT_FRAME, globals.now() + EXPLOSION_DELAY);
                }
                need(hooks.repop)(bomb, 0);
            }
            // 후속 공간 경계 전 현재 두 위치이다.
            const current = pool.slot(parent);
            const currentX = position(current, 14);
            const currentY = position(current, 18);
            if (currentY > floatOf(read(fields, SAVED_Y))) {
                need(hooks.unpop)(parent, 0);
                need(hooks.pop)(parent, currentX, Math.fround(currentY - RESTORE), 0);
            }
        }
        if (need(hooks.mayTarget)(fields.parent, read(fields, 12) & 0xffff) && time(fields, NEXT_DROP) !== 0 && time(fields, NEXT_DROP) <= globals.now() && !read(fields, BOMB)) {
            const created = need(hooks.create)(globals.bombType(), 2);
            const raw = pool.allocated(created);
            raw[40] |= 8;
            rawWrite(raw, 36, 0);
            if (read(fields, BOMB)) need(hooks.assertion)(0x5be);
            write(fields, BOMB, created);
            putTime(fields, DROP_TIME, globals.now());
            need(hooks.unpop)(parent, 0);
            const current = pool.slot(parent);
            write(fields, SAVED_X, rawRead(current, 14));
            write(fields, SAVED_Y, rawRead(current, 18));
            need(hooks.pop)(parent, position(current, 14), Math.fround(position(current, 18) + 0.4), 0);
            // Pop 뒤의 현재 위치와 생성 폭탄 TYPE 이다.
            const after = pool.slot(parent);
            const sound = this.#type(pool.slot(created)[10]).sound;
            need(hooks.sound)(position(after, 14), position(after, 18), sound);
            putTime(fields, NEXT_DROP, globals.now() + 2);
        }
        need(hooks.baseFrame)(fields);
    }

    // 첫 fmod 의 Form 위상은 float scratch 저장 뒤에도 넓은 값을 유지한다. (2026-11-13 WB04-E357)
    animation(fields) {
        const count = need(this.#hooks.runCount)(fields.parent);
        const countFloat = Math.fround(count);
        const phase = (fields.form % PERIOD) * INVERSE_PERIOD + this.#globals.now();
        const modulo = phase % PERIOD;
        const rounded = Math.fround((countFloat - 1) * modulo * INVERSE_PERIOD);
        const value = rounded + 0.5;
        if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('훨리기그 원본 장식 프레임 정수화 범위 오류');
        return Math.trunc(value) | 0;
    }

    // 보호 PE 의 00509f3c 는 float 60 이다. (2026-11-13 WB04-E357)
    static returnDelay() { return 60; }
}
