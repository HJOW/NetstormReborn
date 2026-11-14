// 동적 Image 를 빌리는 MetaImage 노드(MetaImageGump)의 생성/크기/부착 축소/출력 분기와 실제 픽셀 연결을 기준선 소스 계약과 손 계산 값으로 검증한다. (2026-11-14 WB04-E440)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(MetaImageGump.cpp)에서 옮겼다. 공통 factory 는 실제 GumpCreate 이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpCreate, GumpCreateState } from '../www/ui/gump-create.js';
import { MetaImageGump, MetaImageGumpNode, bindMetaImagePixels } from '../www/ui/meta-image-gump.js';
import { RawImage } from '../www/ui/raw-image.js';
import { GumpPixelTarget, GumpDrawState } from '../www/render/gump-draw.js';
import { ShapeDatabase } from '../www/assets/shapes.js';

// 3×2 비트맵(투명 포함)으로 만든 동적 Image 다. shape 48바이트, 폭 3, 높이 2.
const sample = () => { const image = new RawImage(); image.convert({ width: 3, height: 2, pixels: Uint8Array.from([0, 5, 5, 7, 0, 0]) }, false); return image; };
// 실제 GumpCreate 위의 틀이다. log 는 훅 호출을 기록한다.
function rig({ image = sample() } = {}) {
    const log = []; const nodes = new Map(); const state = new GumpCreateState(); const factory = new GumpCreate(state, { node: (token) => nodes.get(token) ?? null });
    const node = new MetaImageGumpNode(); nodes.set(1, node);
    const hooks = { node: (token) => nodes.get(token) ?? null, attach: (...args) => log.push(['attach', ...args]), position: (token) => { log.push(['position', token]); return { x: 10, y: 20 }; }, color: (address) => log.push(['color', address]), draw: (request) => log.push(['draw', request.position.x, request.position.y, request.scale, request.scaled, request.mapped, request.image === image]) };
    return { log, nodes, node, state, factory, hooks, image, gump: new MetaImageGump(factory, hooks) };
}

test('construct: 공통 기본 생성(크기 0·등록)으로 만든 뒤 가상 표/종류 21/Image/배율 1/색 모드 0 을 쓴다', () => {
    const r = rig(); r.node.colorMode = 7; assert.equal(r.gump.construct(1, r.image), 1);
    assert.deepEqual([r.node.vtable, r.node.command, r.node.image === r.image, r.node.scale, r.node.width, r.node.height, r.node.baseWidth, r.node.baseHeight, r.node.colorMode], [0x50d290, 21, true, 1, 0, 0, 0, 0, 0]); assert.equal(r.state.entries[0], 1);
    assert.equal(r.node.flags, 9); assert.throws(() => r.gump.construct(9, r.image), { name: 'RangeError', message: /파생 저장소 없음/ }); assert.throws(() => r.gump.construct(0, r.image), RangeError);
});

test('resize: 기본 크기는 float32(폭+1, 높이+1)이고 표시 크기는 그 값에 현재 배율을 곱해 float32 로 쓰며 Image 가 없으면 Error 다', () => {
    const r = rig(); r.gump.construct(1, r.image); r.gump.resize(1); assert.deepEqual([r.node.baseWidth, r.node.baseHeight, r.node.width, r.node.height], [4, 3, 4, 3]);
    r.node.scale = Math.fround(0.3); r.gump.resize(1); assert.equal(r.node.width, Math.fround(4 * Math.fround(0.3))); assert.equal(r.node.height, Math.fround(3 * Math.fround(0.3)));
    r.node.image = null; assert.throws(() => r.gump.resize(1), { name: 'Error', message: /현재 Image 없음/ });
});

test('attach: 기본 부착 뒤 Image/기본 크기 비율이 1 을 넘으면 큰 비율의 역수로 축소하고(float32 저장 비율·배정도 역수) 아니면 배율 1 이다', () => {
    const r = rig(); r.gump.construct(1, r.image); r.node.baseWidth = 1.5; r.node.baseHeight = 1;
    // 3/1.5=2, 2/1=2 → 역수 0.5 → 크기 0.75×0.5.
    r.gump.attach(1, 5, 6, 7, 2); assert.deepEqual(r.log, [['attach', 1, 5, 6, 7, 2]]); assert.deepEqual([r.node.scale, r.node.width, r.node.height], [0.5, 0.75, 0.5]);
    // 3/2=1.5 vs 2/1=2 → 큰 쪽(2)을 쓴다.
    r.node.baseWidth = 2; r.node.baseHeight = 1; r.gump.attach(1, 0, 0, 0, 0); assert.equal(r.node.scale, 0.5);
    // 비율이 모두 1 이하이면 배율 1 이고 표시 크기는 기본 크기다.
    r.node.baseWidth = 6; r.node.baseHeight = 4; r.gump.attach(1, 0, 0, 0, 0); assert.deepEqual([r.node.scale, r.node.width, r.node.height], [1, 6, 4]);
    // 비율이 정확히 1 이면 축소하지 않는다.
    r.node.baseWidth = 3; r.node.baseHeight = 2; r.gump.attach(1, 0, 0, 0, 0); assert.equal(r.node.scale, 1);
    // 3배 → float32(1/3) 와 배정도 곱의 float32 저장.
    r.node.baseWidth = 1; r.node.baseHeight = 1; r.gump.attach(1, 0, 0, 0, 0); assert.equal(r.node.scale, Math.fround(1 / 3)); assert.equal(r.node.width, Math.fround((1 / 3) * 1));
    // 한쪽 비율만 1 을 넘어도 축소한다(폭만 2배, 높이만 2배). 정확히 2 인 비율도 넘는 것이다.
    r.node.baseWidth = 1.5; r.node.baseHeight = 8; r.gump.attach(1, 0, 0, 0, 0); assert.equal(r.node.scale, 0.5); r.node.baseWidth = 6; r.node.baseHeight = 1; r.gump.attach(1, 0, 0, 0, 0); assert.equal(r.node.scale, 0.5);
    // 폭/높이 1 인 Image 도 비율을 쓴다(1/0.5=2).
    const one = new RawImage(); one.convert({ width: 1, height: 1, pixels: Uint8Array.of(9) }, false); const o = rig({ image: one }); o.gump.construct(1, one); o.node.baseWidth = 0.5; o.node.baseHeight = 0.5; o.gump.attach(1, 0, 0, 0, 0); assert.equal(o.node.scale, 0.5);
    // 폭/높이 0 인 Image 는 비율을 쓰지 않아 배율 1 이다.
    const empty = new RawImage(); const z = rig({ image: empty }); z.gump.construct(1, empty); z.node.baseWidth = 1; z.node.baseHeight = 1; z.gump.attach(1, 0, 0, 0, 0); assert.equal(z.node.scale, 1);
});

test('attach: 기본 부착 중 Image 가 바뀌어도 현재 Image 를 쓰고 Image 가 없으면 부착 뒤 Error 이며 부착 장치가 없으면 Error 다', () => {
    const r = rig(); r.gump.construct(1, r.image); r.node.baseWidth = 1; r.node.baseHeight = 1; const other = new RawImage(); other.convert({ width: 6, height: 2, pixels: new Uint8Array(12).fill(1) }, false);
    const gump = new MetaImageGump(r.factory, { ...r.hooks, attach: () => { r.node.image = other; } }); gump.attach(1, 0, 0, 0, 0); assert.equal(r.node.scale, Math.fround(1 / 6));
    const none = new MetaImageGump(r.factory, { ...r.hooks, attach: () => { r.node.image = null; } }); assert.throws(() => none.attach(1, 0, 0, 0, 0), { name: 'Error', message: /부착 후 Image 없음/ });
    assert.throws(() => new MetaImageGump(r.factory, { ...r.hooks, attach: undefined }).attach(1, 0, 0, 0, 0), /실제 하위 효과 미연결/);
});

test('draw: shape 가 없으면 위치 조회 전 반환하고 단위 배율은 위치+Image 크기·16.16 배율 65536·mapped 를 색 모드로 정하며 비단위 배율은 ftol 변환(plain)이다', () => {
    const empty = new RawImage(); const e = rig({ image: empty }); e.gump.construct(1, empty); e.gump.draw(1); assert.deepEqual(e.log, []);
    const r = rig(); r.gump.construct(1, r.image); r.gump.draw(1); assert.deepEqual(r.log, [['position', 1], ['draw', 13, 22, 65536, false, false, true]]);
    r.log.length = 0; r.node.colorMode = 1; r.gump.draw(1); assert.deepEqual(r.log, [['position', 1], ['color', 0x556038], ['draw', 13, 22, 65536, false, true, true]]);
    // 배율 0.5: scale=ftol(0.5×65536)=32768, y=ftol(2×0.5+20)=21, x=ftol(3×0.5+10)=11(11.5 절삭). 색 모드는 무시한다.
    r.log.length = 0; r.node.scale = 0.5; r.gump.draw(1); assert.deepEqual(r.log, [['position', 1], ['draw', 11, 21, 32768, true, false, true]]);
    // 단위에 가깝지만 비트가 다른 float32(1.0000001)는 단위가 아니다.
    r.log.length = 0; r.node.scale = Math.fround(1.0000001); r.gump.draw(1); assert.deepEqual(r.log.at(-1), ['draw', 13, 22, 65536, true, false, true]);
    r.node.image = null; assert.throws(() => r.gump.draw(1), { name: 'Error', message: /출력 Image 없음/ });
    const noDraw = rig(); noDraw.gump.construct(1, noDraw.image); assert.throws(() => new MetaImageGump(noDraw.factory, { ...noDraw.hooks, draw: undefined }).draw(1), /실제 하위 효과 미연결/);
});

test('bindMetaImagePixels: 단위 배율은 프레임 rect 만큼 어긋난 기준점에 실제 픽셀을 그리고 mapped 는 비활성 색표를, 확대/축소는 같은 배율의 scaledImage 를 쓴다', () => {
    const pitch = 20; const surface = new Uint8Array(pitch * 20).fill(0xaa); const drawState = new GumpDrawState(); Object.assign(drawState.vfxClip, { left: 0, top: 0, right: 19, bottom: 19 });
    const target = new GumpPixelTarget(surface, pitch, 20, drawState); const current = Uint8Array.from({ length: 256 }, (_, index) => index); const disabled = Uint8Array.from({ length: 256 }, (_, index) => 255 - index);
    const r = rig(); const hooks = bindMetaImagePixels(r.hooks, target, current, disabled); const gump = new MetaImageGump(r.factory, hooks); gump.construct(1, r.image);
    // 프레임 rect 왼쪽/위는 (-2,-1)이라 기준점 (10+3, 20+2) 에서 (11,1)... 위치 훅은 (10,20)이므로 그림 원점은 (11, 21) 이 아니라 (13-2, 22-1)=(11,21). 표면 안으로 옮기기 위해 위치를 바꾼다.
    r.hooks.position = () => ({ x: 4, y: 3 }); const placed = new MetaImageGump(r.factory, bindMetaImagePixels({ ...r.hooks, position: () => ({ x: 4, y: 3 }) }, target, current, disabled));
    const frame = new ShapeDatabase(Uint8Array.from(r.image.shape())).blocks[0].frames[0]; assert.deepEqual([frame.rect.left, frame.rect.top], [-2, -1]);
    placed.draw(1);
    // 기준점 (4+3, 3+2)=(7,5), 프레임 rect 를 더해 그림 원점 (5,4): (0,0)=투명(건드리지 않음), (1,0)·(2,0)=5, (0,1)=7.
    const at = (x, y) => surface[y * pitch + x]; assert.deepEqual([at(5, 4), at(6, 4), at(7, 4), at(5, 5), at(6, 5)], [0xaa, 5, 5, 7, 0xaa]);
    // mapped(색 모드 1): 색표가 비활성(255-색)으로 바뀐다. 호출 전 current 는 항등이다.
    surface.fill(0xaa); r.node.colorMode = 1; assert.equal(current[5], 5); placed.draw(1); assert.deepEqual([at(6, 4), at(7, 4), at(5, 5)], [250, 250, 248]); assert.deepEqual(Array.from(current), Array.from(disabled));
    // 확대/축소: 같은 배율의 scaledImage 로 그려 표면이 바뀐다(정확한 래스터는 scaledImage 계약 시험이 맡는다).
    surface.fill(0xaa); r.node.colorMode = 0; r.node.scale = 0.5; placed.draw(1); assert.equal(surface.some((value) => value !== 0xaa), true);
    // 고정 색표 주소가 아니면 RangeError, Image/shape 없음은 조용히 반환한다.
    assert.throws(() => hooks.color(0x556039), { name: 'RangeError', message: /고정 색표 주소/ }); hooks.color(0x556038); assert.throws(() => hooks.draw({ image: null }), { name: 'Error', message: /실제 픽셀 Image 없음/ }); hooks.draw({ image: new RawImage(), position: { x: 0, y: 0 }, scale: 1, scaled: false, mapped: false });
});
