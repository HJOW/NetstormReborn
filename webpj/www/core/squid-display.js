// 공통 표시 갱신(update88/update8c)의 raw 프레임·SHP 경계 계산을 옮긴 모듈이다(cpppj `o/SquidDisplay.cpp/.h`, 10.78 전용: 패치 004ad500/004990d0/00498ff0).
// raw 프레임·위치와 SHP 칸 크기 목록·카메라(원점·Q16 배율·viewport)로 객체 출력 사각형(본그림·그림자·선택 표식)을 만들어 출력 장치(sink)의 변경 표에 순서대로 보낸다.
//  - update: 현재 가상 표의 특수 8c(간헐천 고정 외형·마나/볼트 부분 표시·신전 rune 부분 표시)를 먼저 분배하고, 나머지 확인된 가상 경로는 공통 표시 부모(RawSquidDisplayParent)로 보낸다.
//  - frameSize: 해시 단계용 칸 크기(00419850)이며 표시 억제와 무관하게 읽는다. validate: 공간 쓰기 전에 미지원 가상 표·물리 프레임 범위 오류를 거부한다. bounds: 렌더러의 원본 경계 계산.
//  - 출력 장치 sink = { suppressed() → bool, invalidate(rect, flags), checking() → DWORD(기본 0), frameError(번호, 프레임, abstract), assertion(식, 파일, 줄) }. 오류 상세·진단 장치가
//    없는데 도달하면 빈 성공 대신 오류를 낸다. 이 모듈은 DOM 을 쓰지 않는다.
// 웹은 10.78 전용이라 CD 10.72 분기(1바이트 프레임·표 밖 메모리 접근)는 옮기지 않았다. (2026-11-08 WB04-E59: 객체 표시 이식으로 추가)

import { readRiftShapeFrame } from './raw-rift-shape-access.js';
import { runRawSquidDisplayParent } from './raw-squid-display-parent.js';
import { supportsUnpopDisplay } from './squid-unpop.js';

// 표시 레이어와 extra 의 선택/표식 비트, 마나·간헐천·볼트 가상 표, 간헐천 상하 픽셀 여백이다. (2026-11-08 WB04-E59)
const DOUBLE_FRAMES = 0x440000;
const SHADOW = 0x40000;
const MARKER = 0x10;
const MANA_VTABLE = 0x50c3b0;
const BOLT_VTABLE = 0x509810;
const GEYSER_VTABLE = 0x50da88;
const GEYSER_VERTICAL_INSET = 50;
const ALTERNATE = 0x2000;
const INT_MIN = -0x80000000;
const INT_MAX = 0x7fffffff;

// WORD 0 인 신전 가상 8c 가 쓰는 rune 사각형의 네 inset(좌·상·우·하)이다. 썬 신전은 기본 88 의 thunk 이다. (2026-11-08 WB04-E59)
const VORTEX_INSETS = new Map([
    [0x5196f0, Object.freeze([12, 29, 12, 23])],
    [0x519860, Object.freeze([39, 30, 33, 34])],
    [0x519918, Object.freeze([27, 0, 28, 30])]
]);

// raw 의 리틀엔디언 값을 읽되 부족한 슬롯을 먼저 거부한다(width 바이트, 기본 DWORD). (2026-11-08 WB04-E59)
function read(bytes, offset, width = 4) {
    if (offset > bytes.length || width > bytes.length - offset) throw new RangeError('표시 raw 슬롯 범위 오류');
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value;
}

// DWORD 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E59)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 큰/손상된 입력의 정수 좌표 넘침을 거부한다. (2026-11-08 WB04-E59)
function coordinate(value) {
    if (value < INT_MIN || value > INT_MAX) throw new RangeError('표시 좌표 넘침');
    return value;
}

// x87 의 x*16 / y*11 + 0.5 뒤 절삭이다(float 곱셈으로 먼저 반올림하지 않는다). 카메라 픽셀을 빼서 화면 좌표를 만든다. (2026-11-08 WB04-E59)
function project(value, scale, camera) {
    const pixel = value * scale + 0.5;
    if (!Number.isFinite(pixel) || pixel < INT_MIN || pixel > INT_MAX) throw new RangeError('표시 좌표 범위 오류');
    return coordinate(Math.trunc(pixel) - camera);
}

// 원본 imul 하위 DWORD 와 산술 우측 이동(Q16 배율 적용)이다. (2026-11-08 WB04-E59)
function scale(value, zoom) {
    return Math.imul(value, zoom) >> 16;
}

// 원본은 교집합 계산 대신 각 모서리를 독립 clamp 한다. (2026-11-08 WB04-E59)
function clip(rect, view) {
    const clamp = (value, low, high) => (value < low ? low : high < value ? high : value);
    return {
        left: clamp(rect.left, view.left, view.right),
        top: clamp(rect.top, view.top, view.bottom),
        right: clamp(rect.right, view.left, view.right),
        bottom: clamp(rect.bottom, view.top, view.bottom)
    };
}

// 양의 면적만 변경 표에 보낸다. (2026-11-08 WB04-E59)
function nonempty(rect) {
    return rect.left < rect.right && rect.top < rect.bottom;
}

// 00436d40 의 원문 서식/이름/현재 프레임 개수를 문자열로 만든다(힙을 공유하지 않는다). types 는 { name } 표, shapes 는 { frameCount } 표이다. TYPE 은 부호 있는 범위를 먼저 판정한다. (2026-11-08 WB04-E59)
export function buildSquidFrameAssertionDetail(types, shapes, type, frame, abstract) {
    if (types.length !== shapes.length) throw new RangeError('표시 오류 상세 TYPE/shape 개수 불일치');
    const signedType = type | 0;
    const valid = signedType >= 10 && (type >>> 0) < types.length;
    const name = valid ? types[type].name : 'invalidType';
    const frames = valid ? shapes[type].frameCount : 0;
    return `Frame assertion failed. Asking for type=${signedType} (${name}) frame=${frame | 0} abs=${abstract | 0}\n\nType has ${frames} frames\n`;
}

// 타입·프레임 자료와 카메라에서 객체 출력 요청을 만드는 처리기이다. types 는 { flags1, flags2, maxHitPoints, footX, footY, defaultFrame, name } 표,
//   shapes 는 타입 번호와 같은 인덱스의 { frameCount, loaded, frames:[{ width, height, hotspotX, hotspotY, cellWidth, cellHeight }] } 목록이다. 두 목록과 sink 는 이 객체보다 오래 살아야 한다.
//   view = { cameraX, cameraY, zoom(Q16, 기본 65536), viewport:{ left, top, right, bottom } }. (2026-11-08 WB04-E59)
export class SquidDisplay {
    // 타입 표·SHP 목록·출력 장치·카메라이다.
    #types;
    #shapes;
    #sink;
    #view;

    // 타입 번호 체계가 다른 목록의 혼용을 막는다. (2026-11-08 WB04-E59)
    constructor(types, shapes, sink, view) {
        if (!Array.isArray(types) || !Array.isArray(shapes) || types.length !== shapes.length) throw new RangeError('표시 타입/SHP 목록 크기 오류');
        this.#types = types;
        this.#shapes = shapes;
        this.#sink = sink;
        this.setView(view);
    }

    // 역전 viewport 와 양수 아닌 확대율을 장치 호출 전에 거부한다(카메라/viewport 변경은 호출자의 전체 갱신과 함께 쓴다). (2026-11-08 WB04-E59)
    setView(view) {
        const { zoom, viewport } = view;
        if (!(zoom > 0) || viewport.left > viewport.right || viewport.top > viewport.bottom) throw new RangeError('표시 카메라/viewport 오류');
        this.#view = { cameraX: view.cameraX, cameraY: view.cameraY, zoom, viewport: { ...viewport } };
    }

    // 프레임 연결부가 같은 타입 목록을 쓰는지 구성 때 확인한다: 길이와 표시·공간 재등록이 쓰는 필드를 비교한다. (2026-11-08 WB04-E59)
    validateFrameBinding(types) {
        if (types.length !== this.#types.length) throw new RangeError('프레임 표시 판본/타입 목록 불일치');
        // 표시 레이어/유효성 및 공간 재등록이 사용하는 타입 필드를 비교한다.
        for (let index = 0; index < types.length; index += 1) {
            const a = types[index];
            const b = this.#types[index];
            if (a.flags1 !== b.flags1 || a.flags2 !== b.flags2 || a.maxHitPoints !== b.maxHitPoints || a.footX !== b.footX || a.footY !== b.footY) {
                throw new RangeError('프레임 표시 타입 내용 불일치');
            }
        }
    }

    // 00419850 의 현재 프레임 검사와 실제 SHP 칸 크기 공급이다: 현재 슬롯의 TYPE 번호로 SHP 접근(범위 오류는 상세 writer·진단 뒤 같은 프레임으로 계속)을 실행한다.
    //   004ace40 은 raw abstract 대신 고정 인자 0 을 전달한다. (2026-11-08 WB04-E59)
    frameSize(bytes, frame) {
        const type = read(bytes, 10, 1);
        if (type >= this.#shapes.length) throw new RangeError('프레임 SHP 타입 번호 범위 오류');
        const shapes = this.#shapes;
        const hooks = {
            typeNumber: (selected) => selected.token,
            frameError: (number, savedFrame, abstract) => this.#need('frameError')(number, savedFrame, abstract),
            assertion: (expression, file, line) => this.#need('assertion')(expression, file, line),
            shape: (selected, savedFrame) => {
                const current = shapes[selected];
                if (!current.loaded || savedFrame < 0 || savedFrame >= current.frames.length) throw new RangeError('진단복귀뒤SHP물리범위오류');
                const header = current.frames[savedFrame];
                return [header.cellWidth, header.cellHeight];
            }
        };
        return readRiftShapeFrame(this.#typeView(type), frame, 0, hooks);
    }

    // 공간 쓰기 전에 미지원 가상 표·물리 프레임 범위 오류를 확인한다. 억제 중이면 경계 계산을 생략한다(마나/간헐천/신전 부분 경로는 원문과 같은 위치에서 경계를 계산한다). (2026-11-08 WB04-E59)
    validate(bytes, flags) {
        const table = read(bytes, 0);
        if (!supportsUnpopDisplay(table, flags)) throw new Error('파생 표시 갱신 미복원');
        if (table === GEYSER_VTABLE && (flags & ALTERNATE)) {
            const type = read(bytes, 10, 1);
            if (type >= this.#types.length) throw new RangeError('간헐천 고정 외형 TYPE 범위 오류');
            const frame = this.#types[type].defaultFrame;
            if (!this.#sink.suppressed()) this.bounds(type, frame, floatOf(read(bytes, 14)), floatOf(read(bytes, 18)));
            return;
        }
        if (table === MANA_VTABLE || table === BOLT_VTABLE) {
            if ((table === BOLT_VTABLE || read(bytes, 12, 2) !== 0) && !this.#sink.suppressed()) {
                this.bounds(read(bytes, 10, 1), read(bytes, 36) | 0, floatOf(read(bytes, 14)), floatOf(read(bytes, 18)));
            }
            return;
        }
        const type = read(bytes, 10, 1);
        if (type >= this.#types.length) throw new RangeError('표시 타입 번호 범위 오류');
        const frame = read(bytes, 36) | 0;
        // 부분 갱신의 물리 계약만 검증한다. 실제 변경 영역은 update 에서 한 번만 등록한다.
        if ((flags & ALTERNATE) && read(bytes, 12, 2) === 0 && VORTEX_INSETS.has(table)) {
            if (!this.#sink.suppressed()) this.bounds(type, frame, floatOf(read(bytes, 14)), floatOf(read(bytes, 18)));
            return;
        }
        read(bytes, 40, 1);
        if (!this.#patchFrameValid(type, frame) || this.#sink.suppressed()) return;
        const shape = this.#shapes[type];
        if (!shape.loaded) return;
        const last = frame + ((this.#types[type].flags1 & SHADOW) ? shape.frameCount : 0);
        if (frame < 0 || shape.frameCount < 0 || last < 0 || last >= shape.frames.length) throw new RangeError('표시 물리 프레임 범위 오류');
    }

    // VFX 압축 사각형 대신 별도 Squid 헤더의 픽셀 크기와 핫스폿을 쓰는 원본 경계 계산이다. 표시 억제/선택 확장은 여기서 적용하지 않는다. SHP 가 없으면 빈 사각형이다. (2026-11-08 WB04-E59)
    bounds(type, frame, x, y) {
        if (type >= this.#types.length) throw new RangeError('표시 타입 번호 범위 오류');
        const shape = this.#shapes[type];
        if (!shape.loaded) return { left: 0, top: 0, right: 0, bottom: 0 };
        if (frame < 0 || frame >= shape.frames.length) throw new RangeError('표시 프레임 범위 오류');
        const header = shape.frames[frame];
        const { cameraX, cameraY, zoom } = this.#view;
        const left = coordinate(project(x, 16, cameraX) - scale(header.hotspotX, zoom));
        const top = coordinate(project(y, 11, cameraY) - scale(header.hotspotY, zoom));
        return {
            left,
            top,
            right: coordinate(left + scale(header.width, zoom) + 1),
            bottom: coordinate(top + scale(header.height, zoom) + 1)
        };
    }

    // 현재 가상 표의 특수 8c 를 먼저 분배하고 나머지 확인된 가상 경로만 공통 표시 부모에 전달한다. bytes 는 50바이트 슬롯 사본, flags 는 0 또는 0x2000(8c) 이다. (2026-11-08 WB04-E59)
    update(bytes, flags = 0) {
        const table = read(bytes, 0);
        if (table === GEYSER_VTABLE && (flags & ALTERNATE)) {
            const type = read(bytes, 10, 1);
            if (type >= this.#types.length) throw new RangeError('간헐천 고정 외형 TYPE 범위 오류');
            const frame = this.#types[type].defaultFrame;
            if (this.#sink.suppressed()) return;
            const rect = this.bounds(type, frame, floatOf(read(bytes, 14)), floatOf(read(bytes, 18)));
            rect.top = coordinate(rect.top + GEYSER_VERTICAL_INSET);
            rect.bottom = coordinate(rect.bottom - GEYSER_VERTICAL_INSET);
            const clipped = clip(rect, this.#view.viewport);
            if (nonempty(clipped)) this.#sink.invalidate(clipped, 0);
            return;
        }
        const patchParent = table !== MANA_VTABLE && table !== BOLT_VTABLE;
        const vortexPart = patchParent && (flags & ALTERNATE) !== 0 && read(bytes, 12, 2) === 0 && VORTEX_INSETS.has(table);
        if (patchParent && !vortexPart) {
            if (!supportsUnpopDisplay(table, flags)) throw new Error('파생 표시 갱신 미복원');
            runRawSquidDisplayParent(bytes, () => (this.#sink.checking ? this.#sink.checking() : 0), this.#parentHooks());
            return;
        }
        this.validate(bytes, flags);
        // 473770→499240 의 실제 부분 갱신이다: 추가 10픽셀은 Q16 배율을 곱하지 않으며 선택/그림자를 더하지 않는다.
        if (table === MANA_VTABLE || table === BOLT_VTABLE) {
            const bolt = table === BOLT_VTABLE;
            if ((!bolt && !read(bytes, 12, 2)) || this.#sink.suppressed()) return;
            const rect = this.bounds(read(bytes, 10, 1), read(bytes, 36) | 0, floatOf(read(bytes, 14)), floatOf(read(bytes, 18)));
            rect.right = coordinate(rect.right + 10);
            rect.bottom = coordinate(rect.bottom + 10);
            const clipped = clip(rect, this.#view.viewport);
            if (nonempty(clipped)) this.#sink.invalidate(clipped, 0);
            return;
        }
        const type = read(bytes, 10, 1);
        const frame = read(bytes, 36) | 0;
        // 실제 부분 갱신 helper 는 표시 억제부터 검사하고 선택 표식/그림자 갱신을 추가하지 않는다.
        if ((flags & ALTERNATE) && read(bytes, 12, 2) === 0) {
            const insets = VORTEX_INSETS.get(table);
            if (insets) {
                if (this.#sink.suppressed()) return;
                const rect = this.bounds(type, frame, floatOf(read(bytes, 14)), floatOf(read(bytes, 18)));
                rect.left = coordinate(rect.left + insets[0]);
                rect.top = coordinate(rect.top + insets[1]);
                rect.right = coordinate(rect.right - insets[2]);
                rect.bottom = coordinate(rect.bottom - insets[3]);
                const clipped = clip(rect, this.#view.viewport);
                if (nonempty(clipped)) this.#sink.invalidate(clipped, 0);
                return;
            }
        }
        if (!this.#patchFrameValid(type, frame)) return;
        this.#mark(type, frame, floatOf(read(bytes, 14)), floatOf(read(bytes, 18)), (read(bytes, 40, 1) & 4) !== 0 ? MARKER : 0);
    }

    // 공통 표시 부모에 건네는 장치 묶음이다: TYPE 뷰는 같은 배열의 현재 필드를 읽는 속성이고 번호 getter 는 표 인덱스를 반환한다(실제 49adc0). (2026-11-08 WB04-E59)
    #parentHooks() {
        return {
            type: (type) => {
                if (type >= this.#types.length) throw new RangeError('표시 타입 번호 범위 오류');
                return this.#typeView(type);
            },
            typeNumber: (view) => view.token,
            frameError: (number, frame, abstract) => this.#need('frameError')(number, frame, abstract),
            assertion: (expression, file, line) => this.#need('assertion')(expression, file, line),
            mark: (type, frame, x, y, marker) => this.#mark(type, frame, floatOf(x), floatOf(y), marker)
        };
    }

    // 같은 배열의 현재 필드를 가리키는 TYPE 뷰(flags1·frameCount·loaded·header=maxHitPoints)이다. (2026-11-08 WB04-E59)
    #typeView(type) {
        const types = this.#types;
        const shapes = this.#shapes;
        return {
            token: type,
            get flags1() { return types[type].flags1; },
            get frameCount() { return shapes[type].frameCount; },
            get loaded() { return shapes[type].loaded; },
            get header() { return types[type].maxHitPoints; }
        };
    }

    // 선택된 출력 장치 함수를 찾고 없으면 빈 성공 대신 거부한다. (2026-11-08 WB04-E59)
    #need(name) {
        const device = this.#sink[name];
        if (typeof device !== 'function') throw new Error(name === 'frameError' ? '표시 오류 상세 writer 미연결' : '표시 원문 진단 장치 미연결');
        return (...args) => device.apply(this.#sink, args);
    }

    // 패치의 frameCheck(false) 경로이다: 프레임 수(두 배 비트 포함, 감김 없는 64비트 곱) 안이고 SHP 가 있고 첫 DWORD 가 해제 표식이 아니어야 한다. (2026-11-08 WB04-E59)
    #patchFrameValid(type, frame) {
        const record = this.#types[type];
        const shape = this.#shapes[type];
        const count = shape.frameCount * ((record.flags1 & DOUBLE_FRAMES) !== 0 ? 2 : 1);
        return frame >= 0 && frame < count && Boolean(shape.loaded) && record.maxHitPoints !== -0x22222223 && record.maxHitPoints !== -0x32323233;
    }

    // 4990d0: 억제 중이면 아무것도 하지 않고, 본그림을 viewport 에 자른 사각형(선택 표식이면 위 15픽셀·좌우 3픽셀·좁으면 9픽셀 확장, 확장 뒤에는 다시 자르지 않음)과
    //   그림자 레이어(flags1 0x40000, frameCount 만큼 뒤의 물리 프레임)를 순서대로 변경 표에 보낸다. (2026-11-08 WB04-E59)
    #mark(type, frame, x, y, marker) {
        if (this.#sink.suppressed()) return;
        const main = clip(this.bounds(type, frame, x, y), this.#view.viewport);
        if (marker) {
            if (main.right - main.left < 19) {
                main.left = coordinate(main.left - 9);
                main.right = coordinate(main.right + 9);
            }
            main.top = coordinate(main.top - 15);
            main.left = coordinate(main.left - 3);
            main.right = coordinate(main.right + 3);
        }
        // 자른 뒤 확장한 범위는 다시 viewport 로 자르지 않는다(Dirty/Draw 단계가 화면을 처리한다).
        if (nonempty(main)) this.#sink.invalidate(main, marker);
        if ((this.#types[type].flags1 & SHADOW) !== 0) {
            const shadow = clip(this.bounds(type, frame + this.#shapes[type].frameCount, x, y), this.#view.viewport);
            if (nonempty(shadow)) this.#sink.invalidate(shadow, marker);
        }
    }
}
