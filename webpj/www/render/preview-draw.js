// 원본 커서 미리보기 그리기(10.78 00446270 — 공통 표시 목록의 kind 4 항목 Draw)를 옮긴 모듈이다: 본체/보조 레이어의 그림·윤곽·불법 격자·방향 글자·경고 문구·비용 표시(cpppj `client/PreviewDraw.cpp/.h`, 10.78 전용).
//  - PreviewDraw.draw(entry): 항목의 대리 토큰(`tokenBase + SID × 50`)을 같은 raw 풀의 SID 로 되돌리고(기준 미만·0·stride 배수 아님·용량 밖·TYPE 표 밖은 거부), raw 의 부호 있는 화면 WORD(+22/+24)에서 카메라 원점을 빼 화면 위치를 정한다.
//    tail 비트 2 가 없으면 본체: 윤곽(부모 Gump 없음 && flags2 의 0x100004 비트 없음 && 윤곽 제외 타입이 아님)이면 허용/거부 색 사각형을 그리고, 배치 가능이면 소유자 변환표가 있고 로컬 플레이어가 양수일 때 소유자 색 문맥 뒤 현재 색 그림, 아니면 원색 그림 + 덧그림 타입이면 같은 위치에 한 장 더,
//    불가능이면 삭제 모드(선택 타입 == 삭제 타입)에서 삭제 색 문맥 뒤 현재 색 그림, 그 밖에는 명시 불법 표 그림을 낸다. tail 비트 2 가 있으면 보조: 배치 불가이고 윤곽이며 격자 옵션 비트 0 또는 글자 표시가 켜져 있으면 footprint 칸마다(행 먼저) 격자 표 (열+1)*12+행+1 이
//    참인 칸의 잘린 사각형(체크 무늬, 불량 색)을 내고, 부모 Gump 가 없으면 방향 글자(그림자 + 본문, 소문자 번호 글자는 글자 표시가 꺼져 있으면 숨기되 기준은 한 칸씩 이동), 경고 문구 2~4(차단/동력 없음/다리 없음), 비용 문구 5와 비용 그림 한 장을 낸다.
//    출력 경계(setColors/shape/rectangle/text)가 바꾼 값은 다음 원본 접근에서 다시 읽는다(state/collection 은 현재 참조). 빠진 필수 출력은 조용히 생략하지 않고 해당 분기에서 TypeError 로 거부한다.
//  - 토큰 되돌리기/타입 검사/덧셈·곱셈 감김(DWORD)/격자 좌표의 x87 절삭(비유한은 거부)/격자 footprint 상한(10) 모두 원문 순서를 따른다. 항목은 16바이트 `Uint8Array`(토큰 +8, tail +15)이다.
//  - bindPreviewDraw(hooks, draw): 공통 분배 묶음의 `preview` 를 이 Draw 로 연결한다(항목 전체를 받아 tail 을 잃지 않음).
//  - makePreviewAssetShapes(assets, priceType, price): 원본 타입 번호를 `GameAssets` 의 SHP 블록으로 연결하고 비용 그림 타입만 별도 바인딩으로 돌려준다.
//  - PreviewPixelTarget: 현재 8비트 출력 표면에서 실제 SHP 투명/색 표/격자·윤곽을 즉시 합성한다. 표면·clip 을 먼저 검증하고(오른쪽/아래 제외) 색 문맥은 이후 현재 색 그림에만 쓰며 원색 그림은 항상 원색이다.
//    체크 무늬는 절대 좌표 위상 `(x ^ y) & 1`(오른쪽/아래 모서리 제외), 윤곽은 모서리 선 네 개(폭/높이 0 도 전달, 잘린 면에 새 선을 만들지 않음)이다. text 는 기존 번역/글꼴 계층에 연결한다.
// bindCursor(source)는 실제 표시 판정·배치 격자·목록/측정값을 빌리고 makeCursorPreviewAssetShapes는 현재 비용 그림 토큰을 매번 읽는다. (2026-11-14 WB04-E404)
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E387: 5-A 타입별 Draw·미리보기로 추가, 기준선 뒤 cpppj 변경 없음)
import { clipRect, drawIndexedImage } from './indexed-surface.js';
import { GameAssets } from '../assets/game-assets.js';
import { captureCursorPreviewSource, cursorPreviewCollectionInput, cursorPreviewDrawInput } from './cursor-preview-source.js';
import { FIRST_ASSET_TYPE_NUMBER } from '../assets/type-table.js';

// 표 선택(원본 전역 VFX 문맥)과 한 그림에만 적용하는 모드이다. (2026-11-13 WB04-E387)
export const PreviewColors = Object.freeze({ owner: 1, destroy: 2, illegal: 3, labelShadow: 4 });
export const PreviewShapeMode = Object.freeze({ plain: 'plain', currentColors: 'currentColors', illegal: 'illegal' });
// 풀 레이아웃(10.78): SID 슬롯 50바이트, 용량과 곱한 토큰 범위가 32비트를 넘으면 거부한다. (2026-11-13 WB04-E387)
const STRIDE = 50;
const TOKEN_SPACE = 0x100000000;
// 항목 안의 바이트 위치(토큰 +8, tail +15)와 tail 의 보조 레이어 비트이다. (2026-11-13 WB04-E387)
const ENTRY_TOKEN = 8;
const ENTRY_TAIL = 15;
const SECONDARY = 2;

// 준비한 실제 커서 전역과 격자/글자 표이다(원본 전역의 현재 값). 사용: new PreviewDrawState({ goodColor: 231, badColor: 189 }). (2026-11-13 WB04-E387)
export class PreviewDrawState {
    // 선택/삭제/윤곽 제외/덧그림 발동/덧그림/라벨/비용 그림 타입, 화면 원점·간격·라벨 오프셋·크기, 비용 문구 치수·아이콘 위치이다.
    selectedType = 0;
    destroyType = 0;
    noOutlineType = 0;
    overlayTrigger = 0;
    overlayType = 0;
    labelType = 0;
    priceType = 0;
    cameraX = 0;
    cameraY = 0;
    spacing = 0;
    labelOffsetX = 0;
    labelOffsetY = 0;
    labelWidth = 0;
    labelHeight = 0;
    priceWidth = 0;
    priceIconOffset = 0;
    priceIconX = 0;
    priceIconY = 0;
    // 격자 옵션·글자 표시·호출 플래그·문구 덮어쓰기·차단/동력/다리 판정, 허용/불가 색 번호이다.
    gridOption = 0;
    showLetters = 0;
    flags = 0;
    messageOverride = 0;
    blocked = 0;
    powered = 0;
    reachable = 0;
    goodColor = 0;
    badColor = 0;
    // 원본 (열+1)*12+(행+1) 격자(최대 footprint 10), signed char 로 조회하는 글자 → 프레임 표(256), 방향 글자 바이트 열, 경고 문구 5개의 너비/높이이다.
    grid = new Uint8Array(144);
    labelFrames = new Uint32Array(256);
    labels = new Uint8Array(0);
    messageWidths = new Int32Array(5);
    messageHeights = new Int32Array(5);
    // 타입별 소유자 색 변환표 존재 여부이다. 실제 557940 의 현재 포인터 표를 빌리면 sharedOwnerMaps(DWORD 배열)가 우선한다.
    ownerMaps = [];
    sharedOwnerMaps = [];

    // 일부 필드를 덮어쓴다.
    constructor(init = {}) { Object.assign(this, init); }
}

// 빠진 필수 출력은 조용히 생략하지 않고 해당 분기에서 계약 오류를 알린다. (2026-11-13 WB04-E387)
function required(hook) {
    if (typeof hook !== 'function') throw new TypeError('Preview Draw hook required');
    return hook;
}

// 원본 DWORD 덧셈·뺄셈·곱셈의 하위 32비트(signed)이다. (2026-11-13 WB04-E387)
const add = (a, b) => (a + b) | 0;
const sub = (a, b) => (a - b) | 0;
const mul = (a, b) => Math.imul(a, b);

// 슬롯의 비정렬 little endian 필드(1~4바이트)를 읽는다. (2026-11-13 WB04-E387)
function readField(raw, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// finite x87 결과를 signed 64비트 절삭 뒤 하위 DWORD 로 바꾼다. 비유한 격자 좌표는 거부한다. (2026-11-13 WB04-E387)
function truncate(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) throw new RangeError('Preview Draw coordinate');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 타입 footprint 가 뒤집힌 경우도 원본의 하한 우선 독립 clamp 를 유지한다. (2026-11-13 WB04-E387)
const clamp = (value, lower, upper) => Math.max(lower, Math.min(value, upper));

// 0 방향 정수 나눗셈(signed)이다. (2026-11-13 WB04-E387)
const half = (value) => Math.trunc(value / 2) | 0;

// 커서 미리보기 항목 Draw 이다. 풀/타입/수집 상태/Draw 상태와 훅은 이 객체보다 오래 살아야 한다. 사용: new PreviewDraw(pool, types, collection, state, hooks, tokenBase). (2026-11-13 WB04-E387)
export class PreviewDraw {
    // 객체 슬롯 풀, 타입 표(footX/footY/hotspotX/hotspotY/flags2), 수집 상태, Draw 상태, 출력 훅, 토큰 기준이다.
    #pool;
    #types;
    #collection;
    #state;
    #hooks;
    #tokenBase;
    // 현재 실제 커서 참조 묶음이다. (2026-11-14 WB04-E404)
    #cursor = null;

    // 실제 저장소가 같은 범위의 raw 토큰과 타입 표를 쓰도록 검증한다. (2026-11-13 WB04-E387)
    constructor(pool, types, collection, state, hooks = {}, tokenBase = 0) {
        if (types.length === 0 || tokenBase + pool.capacity * STRIDE > TOKEN_SPACE) throw new TypeError('Preview Draw tables/token range');
        this.#pool = pool;
        this.#types = types;
        this.#collection = collection;
        this.#state = state;
        this.#hooks = hooks;
        this.#tokenBase = tokenBase >>> 0;
    }

    // 같은 PATCH 풀의 실제 커서/격자를 참조한다. 독립 상태는 연결 뒤 커서 값을 덮지 않는다. (2026-11-14 WB04-E404)
    bindCursor(source) { this.#cursor = captureCursorPreviewSource(this.#pool, source, 'Preview Draw'); }

    // 본체와 보조의 공통 화면 WORD/footprint 를 먼저 저장하고 출력 분기를 나눈다. entry 는 16바이트 Uint8Array 이다. (2026-11-13 WB04-E387)
    draw(entry) {
        const token = new DataView(entry.buffer, entry.byteOffset, entry.byteLength).getUint32(ENTRY_TOKEN, true);
        const tail = entry[ENTRY_TAIL];
        if (token < this.#tokenBase) throw new RangeError('Preview Draw token');
        const offset = token - this.#tokenBase;
        if (!offset || offset % STRIDE || offset / STRIDE >= this.#pool.capacity) throw new RangeError('Preview Draw SID');
        const raw = this.#pool.slot(offset / STRIDE);
        const type = raw[10];
        if (type >= this.#types.length) throw new RangeError('Preview Draw type');
        const record = this.#types[type];
        const c = this.#cursor ? cursorPreviewCollectionInput(this.#cursor) : this.#collection;
        const s = this.#cursor ? cursorPreviewDrawInput(this.#cursor, this.#state) : this.#state;
        const hooks = this.#hooks;
        // 출력 경계가 바꾼 값은 다음 원본 접근에서 참조를 통해 다시 읽는다.
        const x = sub((readField(raw, 22, 2) << 16) >> 16, s.cameraX);
        const y = sub((readField(raw, 24, 2) << 16) >> 16, s.cameraY);
        const left = sub(x, mul(record.footX, 16));
        const top = sub(y, mul(record.footY, 11));
        const outlined = !(c.parentGump >>> 0) && !(record.flags2 & 0x100004) && type !== (s.noOutlineType >>> 0);
        // 표 선택과 각 그림은 원본 호출 순서 그대로 전달한다.
        const colors = (mode) => { required(hooks.setColors)({ colors: mode, type, owner: c.localPlayer >>> 0 }); };
        const shape = (key, frame, sx, sy, mode) => { required(hooks.shape)({ type: key >>> 0, frame: frame >>> 0, position: { x: sx, y: sy }, mode }); };
        const rectangle = (rect, color, checker) => { required(hooks.rectangle)(rect, color, checker); };
        if (!(tail & SECONDARY)) {
            if (outlined) rectangle({ left, top, right: x, bottom: y }, (c.legal >>> 0) ? s.goodColor : s.badColor, false);
            const frame = readField(raw, 36, 4);
            const sx = sub(x, record.hotspotX);
            const sy = sub(y, record.hotspotY);
            if (c.legal >>> 0) {
                const shared = s.sharedOwnerMaps.length > 0;
                const mapCount = shared ? s.sharedOwnerMaps.length : s.ownerMaps.length;
                if (type >= mapCount) throw new RangeError('Preview Draw owner maps');
                const mapped = shared ? (s.sharedOwnerMaps[type] >>> 0) !== 0 : Boolean(s.ownerMaps[type]);
                if (mapped && ((c.localPlayer | 0) > 0)) {
                    colors(PreviewColors.owner);
                    shape(type, frame, sx, sy, PreviewShapeMode.currentColors);
                } else shape(type, frame, sx, sy, PreviewShapeMode.plain);
                if (type === (s.overlayTrigger >>> 0)) shape(s.overlayType, 0, sx, sy, PreviewShapeMode.plain);
            } else if ((s.selectedType >>> 0) === (s.destroyType >>> 0)) {
                colors(PreviewColors.destroy);
                shape(type, frame, sx, sy, PreviewShapeMode.currentColors);
            } else shape(type, frame, sx, sy, PreviewShapeMode.illegal);
            return;
        }
        if (!(c.legal >>> 0) && outlined && ((s.gridOption & 1) || (s.showLetters >>> 0))) {
            if (record.footX > 10 || record.footY > 10) throw new RangeError('Preview Draw grid footprint');
            const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            const wx = view.getFloat32(14, true);
            const wy = view.getFloat32(18, true);
            const snappedX = Math.fround(Math.floor(wx + 0.5));
            const snappedY = Math.fround(Math.floor(wy + 0.5));
            const anchorX = sub(truncate(snappedX * 16 + 0.5), s.cameraX);
            const anchorY = sub(truncate(snappedY * 11 + 0.5), s.cameraY);
            const firstX = wx - truncate(wx) < 0.5 ? -1 : 0;
            const firstY = wy - truncate(wy) < 0.5 ? -1 : 0;
            // 원본은 행을 먼저 순회하고 표는 열*12+행으로 읽는다. 빈/퇴화 사각형도 출력 경계에 전달한다.
            for (let row = firstY; row <= record.footY; row += 1) {
                for (let column = firstX; column <= record.footX; column += 1) {
                    const cellIndex = (column + 1) * 12 + row + 1;
                    if (cellIndex < 0 || cellIndex >= s.grid.length) throw new RangeError('Preview Draw grid index');
                    if (!s.grid[cellIndex]) continue;
                    const right = sub(anchorX, mul(column, 16));
                    const bottom = sub(anchorY, mul(row, 11));
                    rectangle({ left: clamp(sub(right, 16), left, x), top: clamp(sub(bottom, 11), top, y), right: clamp(right, left, x), bottom: clamp(bottom, top, y) }, s.badColor, true);
                }
            }
        }
        if (c.parentGump >>> 0) return;
        const labelX = add(x, sub(s.spacing, s.labelOffsetX));
        let labelY = sub(y, 2);
        // MSVC signed char 로 문자를 비교하며 숨긴 글자도 한 칸씩 아래쪽 기준을 이동한다.
        for (const code of s.labels) {
            if (!code) break;
            if (((code << 24) >> 24) < 0x61 || (s.showLetters >>> 0)) {
                colors(PreviewColors.labelShadow);
                shape(s.labelType, s.labelFrames[code], add(labelX, 1), add(labelY, 1), PreviewShapeMode.currentColors);
                shape(s.labelType, s.labelFrames[code], labelX, labelY, PreviewShapeMode.plain);
            }
            labelY = sub(labelY, s.spacing);
        }
        let message = 0;
        if (!(s.flags & 0x20) || (s.messageOverride >>> 0)) {
            if (!(c.legal >>> 0) && (s.blocked >>> 0)) message = 2;
            else if (!(s.powered >>> 0)) message = 3;
            else if (!(c.legal >>> 0) && !(s.reachable >>> 0)) message = 4;
        }
        if (message) {
            const mx = add(x, sub(sub(half(s.labelWidth), half(s.messageWidths[message])), s.labelOffsetX));
            // 실제 호출의 스택 인수로 확인한 y 기준이다. CD 디컴파일의 겹친 지역 변수와 구분한다.
            const my = add(y, sub(sub(half(s.labelHeight), half(s.messageHeights[message])), s.labelOffsetY));
            required(hooks.text)({ index: message, position: { x: mx, y: my } });
        }
        if (s.flags & 0x40) {
            const side = half(sub(x, left));
            required(hooks.text)({ index: 5, position: { x: sub(sub(x, half(s.priceWidth)), side), y } });
            // 두 판본 모두 실제 스택에서는 원래 화면 x 에 비용 이미지 보정을 더한다.
            const iconX = add(add(x, sub(sub(s.priceIconOffset, half(s.priceWidth)), side)), s.priceIconX);
            shape(s.priceType, 3, iconX, add(add(y, 3), s.priceIconY), PreviewShapeMode.plain);
        }
    }
}

// 공통 kind 4 분배의 전체 항목을 실제 미리보기 Draw 로 전달하도록 연결한다(tail 을 잃지 않음). (2026-11-13 WB04-E387)
export function bindPreviewDraw(hooks, draw) {
    hooks.preview = (entry) => { draw.draw(entry); };
}

// 원본 타입 번호를 실제 GameAssets SHP 블록으로 연결한다. 비용 그림 타입은 별도 바인딩(price)이다. 반환 함수: type → { database, block }. (2026-11-13 WB04-E387)
export function makePreviewAssetShapes(assets, priceType, price) {
    return (type) => {
        if ((type >>> 0) === (priceType >>> 0)) return price;
        const types = assets.types();
        if (type < FIRST_ASSET_TYPE_NUMBER || type - FIRST_ASSET_TYPE_NUMBER >= types.length) throw new RangeError('Preview Draw asset type');
        return { database: assets.shapes, block: types[type - FIRST_ASSET_TYPE_NUMBER].block };
    };
}

// Take가 얻은 현재 비용 이미지 토큰으로 SHP 블록을 고른다. GameAssets 생성자의10.78 크기 계약을 사용한다. (2026-11-14 WB04-E404)
export function makeCursorPreviewAssetShapes(assets, priceType, lifecycle) {
    if (!(assets instanceof GameAssets)) throw new TypeError('Preview cursor asset edition');
    const ordinary = makePreviewAssetShapes(assets, priceType, null);
    return (type) => {
        if ((type >>> 0) !== (priceType >>> 0)) return ordinary(type);
        const token = lifecycle.costImage >>> 0;
        if (!token || token > assets.shapes.blocks.length) throw new RangeError('Preview cursor cost image');
        return { database: assets.shapes, block: token - 1 };
    };
}

// 현재 8비트 출력 표면에서 실제 SHP 투명/색 표/격자·윤곽을 즉시 합성하는 출력 경계이다. clip 은 오른쪽/아래 제외이다. (2026-11-13 WB04-E387)
export class PreviewPixelTarget {
    // 출력 픽셀·행 간격·높이·원본 dirty clip, 그림 바인딩/색 표/글자 콜백, 마지막으로 고른 현재 색 표이다.
    #pixels;
    #pitch;
    #height;
    #clip;
    #shape;
    #color;
    #text;
    #current = null;

    // 표면 범위를 먼저 검증하고 원본 dirty clip 으로 제한한다. 사용: new PreviewPixelTarget(pixels, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, shapeOf, colorOf, textOf). (2026-11-13 WB04-E387)
    constructor(pixels, pitch, height, clip, shape, color, text) {
        if (pitch <= 0 || height <= 0 || pitch * height > pixels.length || clip.left > clip.right || clip.top > clip.bottom) throw new RangeError('Preview Draw surface/clip');
        this.#pixels = pixels;
        this.#pitch = pitch;
        this.#height = height;
        this.#clip = clipRect(clip, { left: 0, top: 0, right: pitch, bottom: height });
        this.#shape = shape;
        this.#color = color;
        this.#text = text;
    }

    // 색 문맥 선택은 이후 현재 색 그림에만 적용하고 원색 그림은 항상 원색을 쓴다. 이 객체를 캡처한 출력 훅을 돌려준다(사용 중 객체를 버리지 않는다). (2026-11-13 WB04-E387)
    hooks() {
        return {
            setColors: (request) => { this.#current = required(this.#color)(request); },
            shape: (request) => { this.#drawShape(request); },
            rectangle: (rect, color, checker) => { this.#drawRectangle(rect, color, checker); },
            text: this.#text,
        };
    }

    // 기존 VFX 해제/투명 합성을 사용하고 화면 기준점에 프레임 상대 사각형을 더한다. (2026-11-13 WB04-E387)
    #drawShape(request) {
        const binding = required(this.#shape)(request.type);
        if (!binding?.database) throw new TypeError('Preview Draw shape binding');
        const blocks = binding.database.blocks;
        if (binding.block >= blocks.length || request.frame >= blocks[binding.block].frames.length) throw new RangeError('Preview Draw frame');
        const frame = blocks[binding.block].frames[request.frame];
        if (frame.special) return;
        const image = binding.database.decode(binding.block, request.frame);
        let colors = null;
        if (request.mode === PreviewShapeMode.currentColors) {
            if (!this.#current) throw new Error('Preview Draw current colors missing');
            colors = this.#current;
        } else if (request.mode === PreviewShapeMode.illegal) colors = required(this.#color)({ colors: PreviewColors.illegal, type: request.type, owner: 0 });
        drawIndexedImage(this.#pixels, this.#pitch, this.#height, image, add(request.position.x, frame.rect.left), add(request.position.y, frame.rect.top), this.#clip, colors);
    }

    // 원본 격자는 오른쪽/아래 제외 체크 무늬(위상 (x^y)&1)이며 윤곽은 모서리 선 네 개이다. (2026-11-13 WB04-E387)
    #drawRectangle(rect, color, checker) {
        const clip = this.#clip;
        const pixels = this.#pixels;
        const pitch = this.#pitch;
        if (!checker) {
            const right = sub(rect.right, 1);
            const bottom = sub(rect.bottom, 1);
            // 원본은 폭/높이 0 도 선 네 개 호출로 전달한다. 역전 끝점은 선분 구간으로 정규화한다.
            const horizontal = (y) => {
                if (y < clip.top || y >= clip.bottom) return;
                const low = Math.max(Math.min(rect.left, right), clip.left);
                const high = Math.min(Math.max(rect.left, right), clip.right - 1);
                for (let x = low; x <= high; x += 1) pixels[y * pitch + x] = color;
            };
            const vertical = (x) => {
                if (x < clip.left || x >= clip.right) return;
                const low = Math.max(Math.min(rect.top, bottom), clip.top);
                const high = Math.min(Math.max(rect.top, bottom), clip.bottom - 1);
                for (let y = low; y <= high; y += 1) pixels[y * pitch + x] = color;
            };
            horizontal(rect.top);
            horizontal(bottom);
            vertical(rect.left);
            vertical(right);
            return;
        }
        const visible = clipRect(rect, clip);
        if (visible.left >= visible.right || visible.top >= visible.bottom) return;
        // 장치 밖 좌표를 건드리지 않고 현재 dirty 의 실제 픽셀만 갱신한다.
        for (let y = visible.top; y < visible.bottom; y += 1) {
            for (let x = visible.left; x < visible.right; x += 1) {
                if ((x ^ y) & 1) pixels[y * pitch + x] = color;
            }
        }
    }
}
