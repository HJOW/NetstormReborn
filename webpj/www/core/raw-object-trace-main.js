// 자산 Main/Task 진단(004b0fb0/00450d60)과 원본 가상 +0x70 분배, 원본 CRT 좌표 서식(`%2.5f`)을 옮긴 모듈이다(cpppj `o/RawObjectTraceMain.cpp/.h`, 10.78 전용).
//  - traceRawObjectMain(pool, types, suppress, sid, task, log): 공통 Base 줄을 기록한 **뒤에** 슬롯을 다시 읽어 Main 한 줄을 기록한다: `Main( x y) hl=… q=… r=… z=… [fram=…] plId=… [HP=…] [GrNum=…]` 와
//    상태(Astral·Abstract·OvDraw·Selected·Invisible·Paralyzed·InWorld/!InWorld)·부모/이전(contained)·섬 id·genus(`%X`). task 이면 Main 로그 복귀 뒤 현재 작업 WORD(+12)를 다시 읽어 `Task "<이름>"` 줄을 더한다.
//    수치 인자(float32→double 승격 좌표 둘, 부호 확장 BYTE 둘, 부호 없는 WORD 둘)는 Main 줄과 함께 numbers 로 전달한다.
//  - traceRawObjectVirtual(pool, types, suppress, sid, log): 슬롯의 가상 표 기록값으로 원문 +0x70 본체를 고른다. 네 내장 Form(512110 Depend·512098 Process·512188 Form·512208 Content)은 필드 서식, 검증된 자산 83개와
//    base 표는 Main(표 508cc8 은 Task 포함)을 기록한다. 미등록 표는 임의 Base 로 대체하지 않고 로그 전에 거부한다.
//  - formatOriginalTraceCoordinateBits(bits)/formatOriginalTraceCoordinate(value): 원본 CRT 의 `%2.5f`(17자리 가수 → 5자리 고정소수점 → 다섯 번째 이하가 5 이상이면 올림, 음의 0 은 양의 0, 특수값 표기)를 float32 에서 복원한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E235: 객체 Main/Task trace 이식으로 추가)

import { ObjectTraceFieldKind, traceRawObjectFields } from './raw-object-trace-fields.js';
import { supportsConstructedVtable } from './squid-constructors.js';

// 로그 레벨(원문 29)과 Task WORD 의 상/하한, 작업 이름 표(마지막 포인터 0051a688 의 세 BYTE 도 그대로)이다. (2026-11-10 WB04-E235)
const LOG_LEVEL = 29;
const TASK_FIRST = 0x5101;
const TASK_LAST = 0x5107;
const TASK_NAMES = Object.freeze(['TEND_MANA_PRODUCERS', 'GO_HOME', 'GO_TO', 'FALL', 'BLEED', 'RunBurning', '\x87YN']);
// 호스트 서식 버퍼 한도(원문 1024 BYTE 출력 버퍼)와 네 내장 Form 의 가상 표 기록값, Task 를 더하는 Dude 가상 표이다. (2026-11-10 WB04-E235)
const BUFFER_BYTES = 1024;
const FORM_TABLES = Object.freeze(new Map([[0x512110, ObjectTraceFieldKind.Depend], [0x512098, ObjectTraceFieldKind.Process], [0x512188, ObjectTraceFieldKind.Form], [0x512208, ObjectTraceFieldKind.Content]]));
const TASK_TABLE = 0x508cc8;

// 원문의 DWORD SID 를 줄이기 전에 물리 범위를 검사한 슬롯 사본이다. (2026-11-10 WB04-E235)
function rawOf(pool, sid) {
    if (sid >= pool.capacity) throw new RangeError('Main 진단 물리 SID 범위');
    return pool.slot(sid);
}

// 요청한 원문 폭의 little endian 정수만 읽는다. (2026-11-10 WB04-E235)
function read(raw, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (index * 8);
    return value;
}

// 정수의 부호/0 패딩은 C 의 `%0*d` 와 같은 규칙(폭이 부호를 포함)으로 쓴다. (2026-11-10 WB04-E235)
function number(value, width = 0) {
    const negative = value < 0;
    const digits = String(negative ? -value : value);
    const sign = negative ? '-' : '';
    return sign + '0'.repeat(Math.max(0, width - sign.length - digits.length)) + digits;
}

// 현재 TYPE 은 로그 콜백 뒤 변경될 수 있으므로 소비 위치마다 다시 조회한다. (2026-11-10 WB04-E235)
function typeOf(types, raw) {
    if (raw[10] >= types.length) throw new RangeError('Main 진단 현재 TYPE 범위');
    return types[raw[10]];
}

// 원래 로그 호출 한 번으로 바이트와 최종 수치 인자를 함께 전달한다. 로그 장치가 없으면 거부한다. (2026-11-10 WB04-E235)
function emit(log, level, line, numbers = null) {
    if (typeof log !== 'function') throw new TypeError('Main 진단 실제 로그 미연결');
    log(level, line, numbers);
}

// 비트 변환용 공유 뷰이다. (2026-11-10 WB04-E235)
const view = new DataView(new ArrayBuffer(8));

// float32 비트의 값을 수로 읽는다(NaN 의 페이로드는 이 수에 남지 않는다). (2026-11-10 WB04-E235)
function floatOfBits(bits) {
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// float32 비트를 double 로 승격한 값의 실제 64비트(BigInt)이다. NaN 은 부호·가수를 보존하고 조용한 NaN 비트를 켠다(SSE cvtss2sd). (2026-11-10 WB04-E235)
function promotedDoubleBits(bits) {
    const exponent = (bits >>> 23) & 0xff;
    const mantissa = BigInt(bits & 0x7fffff);
    const sign = BigInt(bits >>> 31) << 63n;
    if (exponent === 0xff && mantissa !== 0n) return sign | (0x7ffn << 52n) | (1n << 51n) | (mantissa << 29n);
    view.setFloat64(0, floatOfBits(bits), true);
    return view.getBigUint64(0, true);
}

// 원본 CRT 의 `%2.5f` 를 float32 비트에서 복원한다: 17자리 가수 → decpt+5 자리까지 유지 → 다음 자리가 5 이상이면 올림(연속 9 전파) → 소수점 배치. 0 과 음의 0 은 "0.00000", 특수값은 `1.#INF0`·`-1.#IND0`·`1.#QNAN` 형태이다. (2026-11-10 WB04-E235)
export function formatOriginalTraceCoordinateBits(bits) {
    const value = bits >>> 0;
    const absolute = value & 0x7fffffff;
    const negative = (value >>> 31) !== 0;
    if (!absolute) return '0.00000'; // 원본 I10 은 음의 0 을 양의 0 과 같이 출력한다.
    if (absolute >= 0x7f800000) {
        if (absolute === 0x7f800000) return negative ? '-1.#INF0' : '1.#INF0';
        if (negative && absolute === 0x7fc00000) return '-1.#IND0';
        return negative ? '-1.#QNAN' : '1.#QNAN';
    }
    const scientific = floatOfBits(absolute).toExponential(16); // float32 의 정확한 double 승격을 17자리 10진 가수로 바꾼다.
    const split = scientific.indexOf('e');
    const digits = scientific.slice(0, split).replace('.', ''); // 부호/소수점을 제외한 17개 가수이다.
    const keep = Number(scientific.slice(split + 1)) + 6; // decpt+5 자리까지 유지한 값은 고정소수점 출력의 정수부이다.
    let units = '';
    if (keep > 0) {
        units = digits.slice(0, keep);
        if (units.length < keep) units += '0'.repeat(keep - units.length);
    }
    if (units === '') units = '0';
    if (keep >= 0 && keep < digits.length && digits[keep] >= '5') {
        let position = units.length; // 원문의 9 연속 올림을 오른쪽에서 왼쪽으로 전파한다.
        const characters = units.split('');
        while (position && characters[position - 1] === '9') { position -= 1; characters[position] = '0'; }
        if (position) characters[position - 1] = String.fromCharCode(characters[position - 1].charCodeAt(0) + 1);
        else characters.unshift('1');
        units = characters.join('');
    }
    if (units.length < 6) units = '0'.repeat(6 - units.length) + units;
    units = `${units.slice(0, units.length - 5)}.${units.slice(units.length - 5)}`;
    return negative ? `-${units}` : units;
}

// 수(float32 로 줄인 값)에서 같은 서식을 만든다. 음의 0 구별과 NaN 페이로드가 중요하면 비트 판을 쓴다. (2026-11-10 WB04-E235)
export function formatOriginalTraceCoordinate(value) {
    view.setFloat32(0, value, true);
    return formatOriginalTraceCoordinateBits(view.getUint32(0, true));
}

// 004b0fb0/00450d60 의 직접 본체이다. Base 가 복귀한 뒤 모든 Main 필드/모드를 현재 물리 주소에서 읽는다. task 가 참이면 Main 로그 복귀 뒤 현재 작업 WORD 를 다시 읽는다.
//  log(level, line, numbers): numbers 는 Main 줄일 때만 { x, y, xBits, yBits, highlight, quantity, region, z }(x/y 는 double 승격 좌표, xBits/yBits 는 그 64비트 BigInt), 다른 줄은 null 이다.
//  suppress 는 정수 또는 `() => 정수`(덤프 모드, Base 로그 뒤에 다시 읽음)이다. (2026-11-10 WB04-E235)
export function traceRawObjectMain(pool, types, suppress, sid, task, log) {
    traceRawObjectFields(pool, types, suppress, sid, ObjectTraceFieldKind.Base, (level, line) => emit(log, level, line));
    const raw = rawOf(pool, sid);
    const dump = typeof suppress === 'function' ? suppress() : suppress;
    let contained = '';
    let hp = '';
    let graph = '';
    let frame = '';
    let island = '';
    if (raw[11] & 8) {
        const previous = read(raw, 16, 2);
        const parent = read(raw, 14, 2); // 실제 +4(이전) 호출 뒤 +0(부모)를 읽는다. 83개 자산 VT 가 같은 두 몸체이다.
        contained = `(parent=~y${number(parent, 4)}~n prev=~y${number(previous, 4)}~n) `;
    }
    if ((typeOf(types, raw).flags2 & 0x400000) ? (raw[40] & 1) !== 0 : (typeOf(types, raw).flags1 & 0x10) !== 0) hp = `HP=${number(read(raw, 26, 4) | 0)} `;
    if (typeOf(types, raw).flags1 & 0x800) graph = `GrNum=${number(raw[30])} `; // 004ad390 의 현재 graph BYTE 이며 조건상 assert 가 발생하지 않는다.
    if (!dump) frame = `fram=${number(read(raw, 36, 4) | 0)} `;
    if (typeOf(types, raw).flags2 & 2) island = `islandId=${number((typeOf(types, raw).flags2 & 4) ? 127 : read(raw, 8, 2))} `; // 004acda0 의 현재 bridge sentinel 127/섬 WORD 이다.
    const flags = raw[40]; // 원문 Main 상태는 공통 free/dead BYTE 와 별개인 +0x28 이다.
    const xBits = read(raw, 14, 4);
    const yBits = read(raw, 18, 4);
    const numbers = {
        x: floatOfBits(xBits), y: floatOfBits(yBits), xBits: promotedDoubleBits(xBits), yBits: promotedDoubleBits(yBits),
        highlight: (raw[33] << 24) >> 24, quantity: read(raw, 12, 2), region: read(raw, 8, 2), z: (raw[35] << 24) >> 24,
    };
    const line = `~1     Main( ${formatOriginalTraceCoordinateBits(xBits)} ${formatOriginalTraceCoordinateBits(yBits)}) hl=${numbers.highlight} q=${numbers.quantity} r=${numbers.region} z=${numbers.z} ${frame}plId=${raw[34]} ${hp}${graph}\n`
        + `     Main ${(flags & 8) ? '~rAstral~n ' : ''}${(flags & 1) ? '~rAbstract~n ' : ''}${(flags & 2) ? 'OvDraw ' : ''}${(flags & 4) ? 'Selected ' : ''}${(flags & 16) ? 'Invisible' : ''}${(flags & 32) ? 'Paralyzed' : ''}${(flags & 128) ? 'InWorld ' : '~r!InWorld~n'}${contained}${island}genus=${(typeOf(types, raw).flags2 >>> 0).toString(16).toUpperCase()}\n`;
    if (line.length >= BUFFER_BYTES) throw new RangeError('Main 진단 호스트 서식 범위');
    emit(log, LOG_LEVEL, line, numbers);
    if (task) {
        const current = Math.min(Math.max(read(rawOf(pool, sid), 12, 2), TASK_FIRST), TASK_LAST); // Main 로그 복귀 뒤 현재 작업 WORD 를 읽는 원문 상/하한이다.
        emit(log, LOG_LEVEL, `~1~y     Task "${TASK_NAMES[current - TASK_FIRST]}"\n`);
    }
}

// 실제 원본 +70 분배표의 83개 자산과 네 Form 만 선택하며 미지 VT 를 임의 Base 로 대체하지 않는다(로그 전에 거부). (2026-11-10 WB04-E235)
export function traceRawObjectVirtual(pool, types, suppress, sid, log) {
    if (pool?.edition !== 'patch1078') throw new TypeError('가상 진단 PATCH 풀 필요');
    const table = read(rawOf(pool, sid), 0, 4); // 슬롯의 현재 가상 표 기록값이다.
    if (FORM_TABLES.has(table)) {
        traceRawObjectFields(pool, types, suppress, sid, FORM_TABLES.get(table), (level, line) => emit(log, level, line));
        return;
    }
    if (!supportsConstructedVtable(table)) throw new Error('객체 trace 미등록 가상 표');
    traceRawObjectMain(pool, types, suppress, sid, table === TASK_TABLE, log);
}
