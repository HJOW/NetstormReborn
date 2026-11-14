// 원본 동적 Image 를 빌리는 전용 MetaImage(VT 0050d290, 종류 21)의 생성·기하·가상 출력을 옮긴다(원문 0047f2a0/0047cbd0/0047cc00/0047cae0, cpppj client/MetaImageGump.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 자산 TYPE ImageGump 와 독립된 원본 MetaImage 파생이다. Image 수명은 목록/창이 소유하며 이 노드는 그 객체만 빌린다(노드 소멸이 Image 를 해제하지 않는다).
// 기본 생성/등록은 같은 10.78 공통 factory(GumpCreate)를 쓰고 부착/위치/픽셀·색표 효과는 호스트 훅이다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const meta = new MetaImageGump(factory, { node, attach, position, color, draw }); meta.construct(token, image); meta.resize(token); bindMetaImagePixels(hooks, target, colors, disabled) 로 실제 픽셀 출력을 연결한다.
// (2026-11-14 WB04-E440: RawMenuControls 선행 MetaImage 이식)
import { GumpCreateNode } from './gump-create.js';
import { ShapeDatabase } from '../assets/shapes.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 실제 가상 표/TYPE BYTE/고정(비활성) 색표 주소다. 주소는 호스트에서 실행하지 않는 근거 식별자다.
const META_VTABLE = 0x50d290; const META_KIND = 21; const DISABLED_COLORS = 0x556038;
// float32 1.0 의 비트다(배율 비트가 정확히 1 일 때만 mapped 분기).
const UNIT_SCALE_BITS = 0x3f800000;

// 원문 정수 기준점의 DWORD 덧셈 감김이다.
const add = (a, b) => (a + b) | 0;
// 도달한 가상 효과만 필수로 검사한다(logic_error→Error).
function need(value) { if (typeof value !== 'function') throw new Error('MetaImage 실제 하위 효과 미연결'); return value; }
// float32 값의 비트다.
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }

// 원본 +a0 의 현재 Image 와 +a4 색 모드를 가진 MetaImage 노드다.
export class MetaImageGumpNode extends GumpCreateNode {
    // 현재 빌린 Image(RawImage) 이며 파생 노드 소멸이 Image 를 해제하지 않는다.
    image = null;
    // +a4 비영은 단위 배율에서 00556038 비활성 색표를 고른다.
    colorMode = 0;
}

// 같은 공통 저장소/부착/위치와 현재 픽셀·색표 효과를 빌린다. factory 는 GumpCreate(construct 를 가진 객체), hooks = { node(token), attach(token,x,y,parent,mode), position(token) → {x,y}, color(주소), draw(요청) }.
// draw 요청 = { image, position: {x,y}, scale(16.16 signed), scaled, mapped } 이다.
export class MetaImageGump {
    // 실제 기본 생성자/등록표와 훅 묶음이다.
    #factory; #hooks;
    constructor(factory, hooks) { this.#factory = factory; this.#hooks = hooks; }

    // 각 효과가 현재 저장소를 다시 조회하므로 기본 부착 중의 Image 교체도 보인다. 없는 파생은 RangeError 다.
    #node(token) {
        const node = token ? need(this.#hooks.node)(token) : null;
        if (!node) throw new RangeError('MetaImage 실제 파생 저장소 없음');
        return node;
    }

    // 0047f2a0: 원본 생성은 Image 크기를 아직 측정하지 않고 기본 크기 0/배율 1 을 보존한다(공통 factory.construct(token,0,0,0,1) 뒤 가상 표/종류/Image/배율/색 모드를 쓴다).
    construct(token, image) {
        const node = this.#node(token); this.#factory.construct(token, 0, 0, 0, 1);
        node.vtable = META_VTABLE; node.command = META_KIND; node.image = image; node.scale = 1; node.width = node.baseWidth; node.height = node.baseHeight; node.colorMode = 0;
        return token;
    }

    // 0047cbd0: 원문 fild→fst(float)→fmul 은 정수+1 을 먼저 float 로 좁히지 않는다: 기본 크기 = float32(폭+1, 높이+1), 표시 크기 = float32((폭+1)×scale). 현재 Image 가 없으면 Error 다.
    resize(token) {
        const node = this.#node(token);
        if (!node.image) throw new Error('MetaImage 현재 Image 없음');
        const width = add(node.image.width(), 1); const height = add(node.image.height(), 1);
        node.baseWidth = Math.fround(width); node.width = Math.fround(width * node.scale); node.baseHeight = Math.fround(height); node.height = Math.fround(height * node.scale);
    }

    // 0047cc00: 기본 부착 뒤 현재 Image/기본 크기의 비율로 축소한다. 두 비율은 각각 float32 로 저장하지만 reciprocal 은 크기 곱셈까지 배정도를 유지한다(큰 비율의 역수만큼 줄이고 작으면 1).
    attach(token, x, y, parent, mode) {
        need(this.#hooks.attach)(token, x, y, parent, mode);
        const node = this.#node(token);
        if (!node.image) throw new Error('MetaImage 부착 후 Image 없음');
        let scale = 1;
        if (node.image.width() !== 0 && node.image.height() !== 0) {
            const width = Math.fround(node.image.width() / node.baseWidth); const height = Math.fround(node.image.height() / node.baseHeight);
            if (width > 1 || height > 1) scale = 1.0 / (height < width ? width : height);
        }
        node.scale = Math.fround(scale); node.width = Math.fround(scale * node.baseWidth); node.height = Math.fround(scale * node.baseHeight);
    }

    // 0047cae0: 배율 비트가 정확히 1 일 때만 mapped(색 모드 비영이면 먼저 비활성 색표 설치)를 쓰고 기준점은 위치+Image 폭/높이다. 다른 배율은 16.16 배율의 plain 변환이며 기준점은 ftol(크기×배율+위치)다.
    // shape 가 없으면 위치 조회 전에 돌아온다. Image 가 없으면 Error 다.
    draw(token) {
        const node = this.#node(token);
        if (!node.image) throw new Error('MetaImage 출력 Image 없음');
        if (node.image.shape().length === 0) return;
        const point = need(this.#hooks.position)(token); const unit = floatBits(node.scale) === UNIT_SCALE_BITS;
        if (unit) {
            if (node.colorMode) need(this.#hooks.color)(DISABLED_COLORS);
            need(this.#hooks.draw)({ image: node.image, position: { x: add(point.x, node.image.width()), y: add(point.y, node.image.height()) }, scale: 65536, scaled: false, mapped: node.colorMode !== 0 });
            return;
        }
        const scale = cameraFtolLow(node.scale * 65536.0);
        const y = cameraFtolLow(node.image.height() * node.scale + point.y); const x = cameraFtolLow(node.image.width() * node.scale + point.x);
        need(this.#hooks.draw)({ image: node.image, position: { x, y }, scale, scaled: true, mapped: false });
    }
}

// 기존 VFX 해제/변환 커널을 동적 shape 와 현재 UI 클립/공유 색표에 연결한 훅 사본을 돌려준다. target 은 GumpPixelTarget(image/scaledImage), current 는 공유 색표(Uint8Array 등, 제자리 갱신), disabled 는 비활성 색표다.
// color(주소)는 고정 주소 00556038 만 받아 current 에 disabled 를 복사하고(다른 주소는 RangeError), draw 는 매 출력에서 shape 의 현재 내용을 읽어 frame 0 을 해독한다(shape 가 없거나 특수 프레임이면 아무것도 하지 않음).
// 확대/축소는 같은 배율 두 축의 scaledImage(색 변환 없음), 단위 배율은 기준점에 프레임 rect 를 더한 image(mapped 이면 current 색표)다.
export function bindMetaImagePixels(hooks, target, current, disabled) {
    const bound = { ...hooks };
    bound.color = (token) => { if (token !== DISABLED_COLORS) throw new RangeError('MetaImage 고정 색표 주소 오류'); current.set(disabled); };
    bound.draw = (request) => {
        if (!request.image) throw new Error('MetaImage 실제 픽셀 Image 없음');
        const raw = request.image.shape(); if (raw.length === 0) return;
        const database = new ShapeDatabase(Uint8Array.from(raw)); const frame = database.blocks[0].frames[0];
        if (frame.special) return;
        const image = database.decode(0, 0); const colors = request.mapped ? current : null;
        if (request.scaled) target.scaledImage(image, frame, request.position, request.scale, request.scale, colors);
        else target.image(image, { x: add(request.position.x, frame.rect.left), y: add(request.position.y, frame.rect.top) }, colors);
    };
    return bound;
}
