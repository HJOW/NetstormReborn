// 실제 ShapeToBuffer(004a7db0)의 메모리 캡처를 VFX RLE 그리기와 보호 SHP 자료에 연결하는 모듈이다(cpppj `client/RawShapeToBuffer.cpp/.h`, 10.78 전용).
// 현재 frame 헤더 → 투명(255) 버퍼 확보 → 실제 plain/mapped RLE → 파생 추가 그림 → 선택 TYPE 기준점 보정의 원본 순서를 보존한다.
//  - createShapeToBufferState(options): 캡처가 매번 읽는 현재 상태 객체를 만든다. { zoom(fixed16, 기본 65536, 패치 0059a918), addTypeHotspot(패치 00542414),
//    ownerTypeMaps(타입별 색표 토큰 배열, 0 이 아니면 mapped 경로), ownerColors(type, owner) → 256바이트 표, mainFrame(sid) → extra&2 일 때 파생 가상+78 의 기본 프레임(-1 이면 기본 그림 생략),
//    additional(sid, image, x, y) → 같은 캡처 버퍼에 그리는 가상+a4 } 이다. 호출자가 값을 바꾸면 다음 캡처가 그 값을 읽는다.
//  - new RawShapeToBuffer(pool, types, shapes, database, state): pool 은 `core/sid-pool.js`, types 는 188칸 TYPE 레코드({ hotspotX, hotspotY, maxHitPoints } 필요),
//    shapes 는 `squid-renderer.js` 의 squidRendererShapes 결과, database 는 `assets/shapes.js` 의 ShapeDatabase(블록 = TYPE − 70)이다. 모두 이 객체보다 오래 살아야 한다.
//  - capture(sid): { width, height, hotspotX, hotspotY, pixels(Uint8Array) } 를 돌려준다. 해제/무효 객체·범위 밖 frame 은 10.78 처럼 원본 진단 예외다(CD 의 8×8 투명 대체는 범위 밖).
//    추가 그림(extra&2)인데 mainFrame/additional 이 없거나, mapped 색표가 필요한데 소비자가 없으면 거부한다(빈 그림으로 대체하지 않는다).
//  - pool: 같은 월드의 풀인지 확인하는 데 쓰는 현재 SID 풀이다(cpppj Pool()).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E199: Burning/건설 캡처의 실제 ShapeToBuffer 이식으로 추가)
import { FIRST_ASSET_TYPE_NUMBER, PATCH_TYPE_COUNT } from '../assets/type-table.js';
import { drawShape } from './vfx-draw.js';
import { drawMappedShape } from './vfx-mapped-draw.js';

// 원본 해제 가상 표와 타입 maxHP 의 두 진단 표식이다. (2026-11-09 WB04-E199)
const FREED_A = 0xdddddddd;
const FREED_B = 0xcdcdcdcd;

// 캡처 버퍼 크기 상한(원본 signed DWORD 곱)이다. (2026-11-09 WB04-E199)
const INT32_MAX = 0x7fffffff;

// raw 의 비정렬 little endian DWORD 를 부호 없이 읽는다. (2026-11-09 WB04-E199)
function dword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 캡처가 매번 읽는 현재 배율/기준점 옵션/색표/파생 그림 상태를 만든다(cpppj ShapeToBufferState 의 기본값). (2026-11-09 WB04-E199)
export function createShapeToBufferState({ zoom = 65536, addTypeHotspot = false, ownerTypeMaps = [], ownerColors = null, mainFrame = null, additional = null } = {}) {
    return { zoom, addTypeHotspot, ownerTypeMaps, ownerColors, mainFrame, additional };
}

// 004a7db0 의 현재 plain/mapped 및 파생 가상 그림 캡처이다. 파생 분배는 현재 소비자가 state 로 공급한다. (2026-11-09 WB04-E199)
export class RawShapeToBuffer {
    #pool;
    #types;
    #shapes;
    #database;
    #state;

    // 같은 판본/로딩 순서의 TYPE 와 SHP 를 읽기 전용으로 참조하며 창이나 장치를 만들지 않는다. (2026-11-09 WB04-E199)
    constructor(pool, types, shapes, database, state) {
        if (types.length !== PATCH_TYPE_COUNT || shapes.length !== PATCH_TYPE_COUNT) throw new RangeError('ShapeToBuffer TYPE/SHP 판본 크기 오류');
        if (!state || typeof state !== 'object') throw new TypeError('ShapeToBuffer 현재 상태 미연결');
        this.#pool = pool;
        this.#types = types;
        this.#shapes = shapes;
        this.#database = database;
        this.#state = state;
    }

    // 원본 imul 의 low DWORD 뒤 sar16 이다(signed 감김 보존). (2026-11-09 WB04-E199)
    #scale(value) {
        return Math.imul(value | 0, this.#state.zoom | 0) >> 16;
    }

    // Sprite source 의 현재 프레임은 그림자 레이어가 아닌 헤더/물리표의 실제 번호다. (2026-11-09 WB04-E199)
    capture(sid) {
        const raw = this.#pool.slot(sid);
        const type = raw[10];
        if (type < FIRST_ASSET_TYPE_NUMBER || type >= this.#types.length) throw new RangeError('ShapeToBuffer 현재 자산 타입 오류');
        const vtable = dword(raw, 0);
        const frame = dword(raw, 36) | 0;
        const shape = this.#shapes[type];
        const valid = !(raw[11] & 1) && vtable !== 0 && vtable !== FREED_A && vtable !== FREED_B && frame >= 0 && frame < shape.frameCount && shape.loaded;
        if (!valid) throw new Error('ShapeToBuffer 현재 source/frame 원본 진단');
        const record = this.#types[type];
        if (!Number.isInteger(record.maxHitPoints)) throw new TypeError('ShapeToBuffer TYPE maxHitPoints 미연결');
        const maxHitPoints = record.maxHitPoints >>> 0;
        if (maxHitPoints === FREED_A || maxHitPoints === FREED_B) throw new Error('ShapeToBuffer frameCheck 원본 진단');
        const state = this.#state;
        const additional = (raw[40] & 2) !== 0;
        if (additional && (typeof state.mainFrame !== 'function' || typeof state.additional !== 'function')) throw new Error('ShapeToBuffer 추가 가상 그림 미연결');
        const owner = raw[34];
        // mapped 일 때만 현재 실제 소유자 색표를 고른다.
        let colors = null;
        if (owner) {
            if (type >= state.ownerTypeMaps.length) throw new Error('ShapeToBuffer 현재 소유자 색표 미연결');
            if (state.ownerTypeMaps[type]) {
                if (typeof state.ownerColors !== 'function') throw new Error('ShapeToBuffer mapped 소유자 색 소비자 미연결');
                colors = state.ownerColors(type, owner);
                if (!colors || colors.length !== 256) throw new Error('ShapeToBuffer mapped 소유자 색표 크기 오류');
            }
        }
        if (frame >= shape.frames.length) throw new RangeError('ShapeToBuffer 실제 프레임 헤더 오류');
        const header = shape.frames[frame];
        const width = this.#scale(header.width);
        const height = this.#scale(header.height);
        if (width < 0 || height < 0 || width * height > INT32_MAX) throw new RangeError('ShapeToBuffer 버퍼 크기 오류');
        const image = { width, height, hotspotX: this.#scale(header.hotspotX), hotspotY: this.#scale(header.hotspotY), pixels: new Uint8Array(width * height).fill(255) };
        // 현재 헤더 크기와 가상 기본 프레임을 구별한다.
        const main = additional ? state.mainFrame(sid) : frame;
        // 캡처는 새 버퍼 전체를 pane 으로 쓰며, 메타 배율로 RLE 를 다시 늘리지 않는다.
        if (main !== -1) {
            const pane = { left: 0, top: 0, right: width - 1, bottom: height - 1 };
            const block = type - FIRST_ASSET_TYPE_NUMBER;
            if (!colors) drawShape(image.pixels, width, height, this.#database, block, main, image.hotspotX, image.hotspotY, pane);
            // 원본 mapped 추가 분기는 가상 main 대신 현재 frame 을 쓴다. 이 비대칭을 고치지 않는다.
            else drawMappedShape(image.pixels, width, height, this.#database, block, frame, image.hotspotX, image.hotspotY, pane, colors);
        }
        if (additional) state.additional(sid, image, image.hotspotX, image.hotspotY);
        if (state.addTypeHotspot) {
            if (!Number.isInteger(record.hotspotX) || !Number.isInteger(record.hotspotY)) throw new TypeError('ShapeToBuffer TYPE 기준점 미연결');
            image.hotspotX = (image.hotspotX + this.#scale(record.hotspotX)) | 0;
            image.hotspotY = (image.hotspotY + this.#scale(record.hotspotY)) | 0;
        }
        return image;
    }

    // 같은 피해/건설 캡처 어댑터가 다른 월드의 풀을 섞지 않도록 확인하는 현재 풀이다. (2026-11-09 WB04-E199)
    get pool() { return this.#pool; }
}
