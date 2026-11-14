// 원본 생산 아이콘 ImageGump(VT 00501bb0, 종류 3)의 SHP Gump 생성·배율·색·그림자·투명 적중을 옮긴다(원문 00419fc0/004198f0/00419990/00419cd0/00419cb0, cpppj client/ImageGump.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 원본 x87 의 최종 float 저장·CRT 하위 DWORD(ftol)·배율 비트 비교·색표 설치 순서를 보존한다. 기본 생성/등록/수명은 같은 공통 GumpCreate 와 공유하고 SHP 자산/표면/색표는 호스트 훅이다.
// bindImageGumpAssets 는 실제 GameAssets(TYPE 표·SHP)로 TYPE/프레임 헤더 조회를, ImageGumpPixelTarget 은 현재 UI 표면과 실제 200×200 적중 버퍼로 일반/배율 VFX 출력·색표·투명 적중을 연결한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const gump = new ImageGump(factory, state, hooks); gump.construct(token, type, frame, mode); gump.draw(token); gump.hit(token, x, y) → 1(불투명)/0.
// (2026-11-14 WB04-E441: RawMenuControls 선행 TYPE 이미지 Gump 이식)
import { GumpCreateNode } from './gump-create.js';
import { cameraFtolLow } from '../render/camera-view.js';
import { drawScaledVfxImage } from '../render/vfx-transform.js';
import { FIRST_ASSET_TYPE_NUMBER } from '../assets/type-table.js';
import { buildTypeRecords } from '../assets/type-flags.js';

// 원본 가상 표와 세 현재 색표 주소(그림자/선택/비활성)다. 주소는 호스트에서 실행하지 않는 근거 식별자다.
const VTABLE = 0x501bb0; const SHADOW_COLORS = 0x556338; const SELECTED_COLORS = 0x556438; const DISABLED_COLORS = 0x556038;
// 실제 16.16 단위와 적중 버퍼의 변 길이다.
const FIXED_UNIT = 65536; const HIT_SIZE = 200;
// float32 1.0 의 비트다(배율 비트가 정확히 1 일 때만 단위 배율).
const UNIT_SCALE_BITS = 0x3f800000;

// 정수 좌표/색표 주소의 low DWORD 덧셈이다.
const add = (a, b) => (a + b) | 0;
// 도달한 실제 효과만 필수로 검사한다(logic_error→Error).
function need(callback) { if (typeof callback !== 'function') throw new Error('이미지 Gump 실제 하위 미연결'); return callback; }
// float32 값의 비트다.
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }

// 실제 501bb0 의 +a0~b0 필드다. 기본 저장소/등록/수명은 기존 Gump 와 공유한다.
export class ImageGumpNode extends GumpCreateNode {
    // +a0/+a4/+a8: 실제 TYPE/SHP 프레임과 출력 비트(1 선택·2 비활성·4 그림자·8 색표 무시 등)다.
    type = 0; frame = 0; mode = 0;
    // +ac/+b0: SHP signed 기준점 − TYPE signed 기준점의 최종 float32 다.
    hotspotX = 0; hotspotY = 0;
}

// 원본 TYPE 이미지 Gump 본체다. state = { localOwner(40da40 현재 로컬 owner DWORD), ownerMaps(557940 TYPE별 9색표 현재 기본 토큰 배열), shadowOffset({x,y}, 52f434/52f444) }는 읽는 시점의 현재 값이고,
// hooks = { node(token), type(type) → { shape, hotspotX, hotspotY }, header(type, frame) → { width, height, hotspotX, hotspotY }, position(token) → {x,y}, color(주소), draw(요청), hit(요청) → 0/1, attach(token,x,y,parent,mode) } 이다.
// draw 요청 = { type, shape, frame, position:{x,y}, scale(16.16), scaled, mapped }, hit 요청 = { type, shape, frame, point:{x,y}, origin:{x,y}, scale } 이다.
export class ImageGump {
    // 같은 공통 기본 생성자, 현재 전역 보기, 훅 묶음이다.
    #factory; #state; #hooks;
    constructor(factory, state, hooks) { this.#factory = factory; this.#state = state; this.#hooks = hooks; }

    // 기본 생성자가 쓰는 동일 파생 주소를 돌려준다(없으면 RangeError).
    #node(token) {
        const node = token ? need(this.#hooks.node)(token) : null;
        if (!node) throw new RangeError('이미지 Gump 저장소 없음');
        return node;
    }

    // 같은 기본 생성/등록(크기 0) 뒤 가상 표/종류 3 을 쓰고 프레임을 설정한다.
    construct(token, type, frame, mode) {
        const node = this.#node(token);
        this.#factory.construct(token, 0, 0, 0, 1);
        node.vtable = VTABLE; node.command = 3; this.setFrame(token, type, frame, mode);
        return token;
    }

    // fisub 는 TYPE 정수를 float 으로 먼저 좁히지 않는다. 헤더의 폭/높이를 기본 크기로(float32), 표시 크기는 현재 배율을 곱해 float32 로 쓰고, 기준점은 마지막 차이만 float32 로 저장한다.
    setFrame(token, type, frame, mode) {
        const node = this.#node(token); node.type = type >>> 0; node.frame = frame >>> 0; node.mode = mode >>> 0;
        const data = need(this.#hooks.type)(type); const header = need(this.#hooks.header)(type, node.frame);
        node.baseWidth = Math.fround(header.width); node.width = Math.fround(header.width * node.scale);
        node.baseHeight = Math.fround(header.height); node.height = Math.fround(header.height * node.scale);
        node.hotspotX = Math.fround(header.hotspotX - data.hotspotX); node.hotspotY = Math.fround(header.hotspotY - data.hotspotY);
    }

    // 모드 4 는 먼저(그림자 색표, 현재 그림자 오프셋·단위 배율이면 y-1) 그리며 뒤의 일반/지정색 본체를 생략하지 않는다. 모드 1/2 는 선택/비활성 색표로, 그 밖에는 현재 owner 색표(TYPE 의 표가 있고 owner>0, 단위 배율에서 모드 8 이 아닐 때)로 그리고 아니면 색표 없이 그린다.
    // 각 출력의 기준점 = ftol(위치 + 기준점×(단위면 1, 아니면 배율)), 16.16 배율 = 단위면 65536 아니면 ftol(배율×65536) 이다. TYPE 번호가 owner 색표 배열 밖이면 RangeError 다.
    draw(token) {
        const node = this.#node(token); const point = need(this.#hooks.position)(token); const data = need(this.#hooks.type)(node.type);
        const unit = floatBits(node.scale) === UNIT_SCALE_BITS; const state = this.#state; const hooks = this.#hooks;
        // 각 출력의 현재 기준점/16.16 값과 색표 설치 순서를 보존한다.
        const draw = (mapped, shadow) => {
            const px = shadow ? add(point.x, state.shadowOffset.x) : point.x; const py = shadow ? add(point.y, state.shadowOffset.y) : point.y;
            const x = cameraFtolLow(px + node.hotspotX * (unit ? 1.0 : node.scale));
            const y = cameraFtolLow(py + node.hotspotY * (unit ? 1.0 : node.scale) - (shadow && unit ? 1.0 : 0.0));
            need(hooks.draw)({ type: node.type, shape: data.shape, frame: node.frame, position: { x, y }, scale: unit ? FIXED_UNIT : cameraFtolLow(node.scale * FIXED_UNIT), scaled: !unit, mapped });
        };
        if (node.mode & 4) { need(hooks.color)(SHADOW_COLORS); draw(true, true); }
        if (node.mode & 3) { need(hooks.color)((node.mode & 1) ? SELECTED_COLORS : DISABLED_COLORS); draw(true, false); return; }
        if (node.type >= state.ownerMaps.length) throw new RangeError('이미지 Gump 현재TYPE 색표 범위 오류');
        const owner = state.localOwner | 0; const table = state.ownerMaps[node.type] >>> 0;
        if ((!unit || !(node.mode & 8)) && table && owner > 0) { need(hooks.color)((table + (state.localOwner << 8)) >>> 0); draw(true, false); }
        else draw(false, false);
    }

    // 그림 자식의 로컬 위치를 포함한 기준점을 만든 뒤 실제 버퍼 판정에 전달한다(검사점 = ftol(x), ftol(y), 기준점 = ftol(기준점×배율+노드 위치), 배율 = ftol(배율×65536)). 반환은 불투명이면 1 이다.
    hit(token, x, y) {
        const node = this.#node(token); const type = need(this.#hooks.type)(node.type); const scale = cameraFtolLow(node.scale * FIXED_UNIT);
        return need(this.#hooks.hit)({
            type: node.type, shape: type.shape, frame: node.frame, point: { x: cameraFtolLow(x), y: cameraFtolLow(y) },
            origin: { x: cameraFtolLow(node.hotspotX * node.scale + node.x), y: cameraFtolLow(node.hotspotY * node.scale + node.y) }, scale,
        });
    }

    // 기본 부착을 그대로 호출하며 추가 좌표 보정/자식 생성은 없다.
    attach(token, x, y, parent, mode) { need(this.#hooks.attach)(token, x, y, parent, mode); }
}

// 생성/프레임 조회를 실제 GameAssets 의 TYPE/SHP 에 연결한다(hooks 의 type/header 를 채운다). TYPE 번호와 실제 로딩 블록 순서를 혼용하지 않는다: TYPE 번호는 70 부터이고 SHP 블록은 번호-70 이다.
// type(번호) → { shape(번호-70+1), hotspotX, hotspotY }(타입 표 188칸 안만, 밖은 RangeError), header(번호, 프레임) → 실제 Squid 그림 헤더(프레임 수는 클러스터 수 × (그림자 플래그 0x440000 이면 2) 안만 허용).
export function bindImageGumpAssets(hooks, assets) {
    const table = assets.typeTable; const records = buildTypeRecords(table.definitions);
    hooks.type = (type) => {
        if (type < FIRST_ASSET_TYPE_NUMBER || type >= records.length) throw new RangeError('이미지 Gump 실제TYPE 자산 범위 오류');
        const view = table.view(type);
        return { shape: type - FIRST_ASSET_TYPE_NUMBER + 1, hotspotX: view.hotspotX, hotspotY: view.hotspotY };
    };
    hooks.header = (type, frame) => {
        if (type < FIRST_ASSET_TYPE_NUMBER || type - FIRST_ASSET_TYPE_NUMBER >= assets.types().length) throw new RangeError('이미지 Gump 실제SHP 타입 범위 오류');
        const block = type - FIRST_ASSET_TYPE_NUMBER; const record = records[type];
        const logical = assets.types()[block].definition.clusters.length * ((record.flags1 & 0x440000) ? 2 : 1);
        if (frame >= logical) throw new RangeError('이미지 Gump 실제frameCheck 범위 오류');
        const data = assets.shapes.squidMetrics(block, frame);
        return { width: data.width, height: data.height, hotspotX: data.hotspotX, hotspotY: data.hotspotY };
    };
    return hooks;
}

// 기존 VFX 일반/직교 배율을 현재 UI 표면(GumpPixelTarget)과 실제 200×200 적중 버퍼에 연결한다. colors(토큰) 는 현재 실제 원본 색표를 돌려주는 공급자(누락/미복원 표를 항등으로 바꾸지 않는다)이고,
// currentColors 는 이후 mapped 그림이 쓰는 공유 256색 문맥(제자리 복사)이다.
export class ImageGumpPixelTarget {
    // 같은 UI 표면과 실제 자산, 공유 현재 색표, 색표 공급자, TYPE/프레임별 해제 픽셀 캐시다.
    #target; #assets; #currentColors; #colors; #images = new Map();
    constructor(target, assets, currentColors, colors) { this.#target = target; this.#assets = assets; this.#currentColors = currentColors; this.#colors = colors; }

    // 기존 저장소/부착 훅을 보존하고 실제 그리기·색·적중만 연결한 훅 사본을 돌려준다.
    hooks(hooks) {
        const bound = { ...hooks };
        bound.color = (token) => { this.#currentColors.set(need(this.#colors)(token)); };
        bound.draw = (request) => this.#draw(request); bound.hit = (request) => this.#hit(request);
        return bound;
    }
    // 실제 타입 범위만 확인하고 표시 이름으로 그림을 바꾸지 않는다(블록 = 번호-70).
    #block(type) {
        if (type < FIRST_ASSET_TYPE_NUMBER || type - FIRST_ASSET_TYPE_NUMBER >= this.#assets.types().length) throw new RangeError('이미지 Gump 픽셀TYPE 오류');
        return type - FIRST_ASSET_TYPE_NUMBER;
    }
    // 같은 실제 프레임만 한 번 해제한다.
    #image(type, frame) {
        const key = `${type}:${frame}`; let image = this.#images.get(key);
        if (!image) { image = this.#assets.shapes.decode(this.#block(type), frame); this.#images.set(key, image); }
        return image;
    }
    // 프레임 객체를 얻는다(범위 밖 프레임은 RangeError).
    #frame(type, frame) {
        const value = this.#assets.shapes.blocks[this.#block(type)].frames[frame];
        if (!value) throw new RangeError('이미지 Gump 프레임 범위 오류');
        return value;
    }
    // 특수 메타프레임은 기존 VFX 와 같이 그림을 만들지 않는다. 확대/축소는 같은 배율의 scaledImage, 단위는 프레임 rect 만큼 어긋난 기준점의 image 다.
    #draw(request) {
        const frame = this.#frame(request.type, request.frame);
        if (frame.special) return;
        const image = this.#image(request.type, request.frame); const colors = request.mapped ? this.#currentColors : null;
        if (request.scaled) this.#target.scaledImage(image, frame, request.position, request.scale, request.scale, colors);
        else this.#target.image(image, { x: add(request.position.x, frame.rect.left), y: add(request.position.y, frame.rect.top) }, colors);
    }
    // 원본 4daca0→4dabf0 의 버퍼/클램프·검사점 보정 뒤 실제 VFX 출력 색 255 를 판정한다: 기준점이 0..199 밖이면 검사점을 그만큼 옮기고 기준점은 clamp 하며, 보정한 검사점이 버퍼 밖이면 RangeError 다.
    #hit(request) {
        const frame = this.#frame(request.type, request.frame); const point = { ...request.point }; const origin = request.origin;
        if (origin.x < 0) point.x = add(point.x, (0 - origin.x) | 0);
        if (origin.x > 199) point.x = add(point.x, add(199, (0 - origin.x) | 0));
        if (origin.y < 0) point.y = add(point.y, (0 - origin.y) | 0);
        if (origin.y > 199) point.y = add(point.y, add(199, (0 - origin.y) | 0));
        const index = point.x + point.y * HIT_SIZE;
        if (index < 0 || index >= HIT_SIZE * HIT_SIZE) throw new RangeError('이미지 Gump 원본 적중 버퍼 밖 읽기');
        const scratch = new Uint8Array(HIT_SIZE * HIT_SIZE).fill(255);
        if (!frame.special) drawScaledVfxImage(scratch, HIT_SIZE, HIT_SIZE, this.#image(request.type, request.frame), frame, { x: Math.min(Math.max(origin.x, 0), 199), y: Math.min(Math.max(origin.y, 0), 199) }, request.scale, request.scale, { left: 0, top: 0, right: HIT_SIZE, bottom: HIT_SIZE });
        return scratch[index] !== 255 ? 1 : 0;
    }
}
