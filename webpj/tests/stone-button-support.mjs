// CPP GumpTextTests의 실제 공통 트리/부모/배경/글꼴/수명을 같은 메모리 장치로 조립한다. (2026-11-14 WB04-E417)
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { FontSet, fontCachePath } from '../www/assets/font-set.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { GumpCreateNode, GumpCreateState, GumpCreate } from '../www/ui/gump-create.js';
import { GumpTreeState, GumpTree } from '../www/ui/gump-tree.js';
import { GumpLifetimeState, GumpLifetime } from '../www/ui/gump-lifetime.js';
import { GumpEvents, GumpEventState } from '../www/ui/gump-events.js';
import { GumpInput, GumpInputState } from '../www/ui/gump-input.js';
import { ButtonGumpState, ButtonGump } from '../www/ui/button-gump.js';
import { GumpBackgroundNode, GumpBackgroundCreate } from '../www/ui/gump-background-create.js';
import { StoneButtonGumpNode, StoneButtonGumpState, StoneButtonGump, bindStoneButtonGump } from '../www/ui/stone-button-gump.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpText, bindStoneButtonText } from '../www/render/gump-text.js';
import { bindGumpBackgroundAssets } from '../www/render/gump-background-draw.js';

// 원문 Font 입력과 Case 기대값을 그대로 보관한다. 실제 파일은 다시 쓰지 않는다.
export const STONE_FONT_ROWS = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumptext-x86.tsv'));
// 원문 폭 표/메트릭만 실제 .chfnt 메모리 캐시로 바꾼다. 결과 폭/높이는 계산하지 않는다.
export function stoneFonts(pixel = false) { const files = new Map();
    // 모든 슬롯·스타일 입력을 기존 실제 BitmapFont parser/FontSet에 공급한다.
    for (const row of STONE_FONT_ROWS.filter((r) => r[0] === 'Font')) { const bytes = new Uint8Array(4132); bytes.set(Buffer.from('BitmapFontData\x1a\0')); const height = Number(row[3]); put(bytes, 16, 1); put(bytes, 20, height); put(bytes, 24, height - 5); put(bytes, 28, 5); put(bytes, 32, 256); bytes.set(Buffer.from(row[4], 'hex'), 36); files.set(fontCachePath(Number(row[1]), Number(row[2])), bytes); }
    if (pixel) { const cache = new Uint8Array(4175); cache.set(Buffer.from('BitmapFontData\x1a\0')); put(cache, 16, 1); put(cache, 20, 14); put(cache, 24, 11); put(cache, 28, 3); put(cache, 32, 256); put(cache, 36 + 65 * 4, 2); put(cache, 36 + 2048 + 65 * 4, 2); put(cache, 36 + 3072 + 65 * 4, 43); put(cache, 4132, 0x30312e31); put(cache, 4136, 1); put(cache, 4140, 16); cache.set([2, 100, 0], 4172); files.set(fontCachePath(0, 0), cache); }
    return new FontSet(files);
}
// CPP 독립 SHP 입력의 실제3×3 RLE/메타3×3을 사용한다.
export function stoneShapes() { const bytes = new Uint8Array(97); put(bytes, 0, 0x30312e31); put(bytes, 4, 1); put(bytes, 8, 52); put(bytes, 40, 3, 2); put(bytes, 42, 3, 2); put(bytes, 52, 2, 2); put(bytes, 54, 2, 2); put(bytes, 68, 2); put(bytes, 72, 2); bytes.set([2, 1, 2, 2, 2, 1, 0, 2, 3, 2, 4, 2, 3, 0, 2, 1, 2, 2, 2, 1, 0], 76); return new ShapeDatabase(bytes); }
// 소스 색을 명시+10/+20으로 구별한다.
export const STONE_MAPS = { bright: Uint8Array.from({ length: 256 }, (_, i) => i + 10), dark: Uint8Array.from({ length: 256 }, (_, i) => i + 20) };
// 원본은 문자열 포인터 입력이므로 검사 호출은 실제 NUL까지 명시한다.
export const stoneLabel = (value) => Uint8Array.from(Buffer.from(value + '\0', 'latin1'));

// 모든 실제 처리기가 같은 기본/파생 객체를 빌리는 작은 장면이다. 다른 월드/게임 상태를 만들지 않는다.
export class StoneScene {
    // 현재 실제 생성/트리/수명/사건/입력·버튼/돌 버튼 전역과 안정된 노드다.
    create = new GumpCreateState(); treeState = new GumpTreeState(); lifeState = new GumpLifetimeState(); eventState = new GumpEventState(); inputState = new GumpInputState(); buttonState = new ButtonGumpState(); stoneState = new StoneButtonGumpState();
    // 루트1/부모2/배경3은 실제 공통 등록표에도 같은 토큰이다.
    root = new GumpCreateNode(); node = new StoneButtonGumpNode(); background = new GumpBackgroundNode(); nodes = new Map([[1, this.root], [2, this.node], [3, this.background]]);
    // 같은 현재 장치/처리기·실제 해제 기록과 명시 할당 실패 입력이다.
    factory; collection; tree; lifetime; events; input; buttons; backgrounds; stones; text; released = []; failAllocation = false; branch; lifeHooks; stoneHooks;
    // 실제 FontSet/측정과 선택 자산 어댑터를 공급한다. 준비 단계에서는 그리지 않는다.
    constructor(fonts = stoneFonts(), assets = null) { this.create.screenWidth = 640; this.create.screenHeight = 480; Object.assign(this.treeState.mapView, { left: 0, top: 0, right: 640, bottom: 480 }); this.stoneState.color = -7; this.stoneState.palette = { black: 9, white: 243, edge: 57 }; if (assets) this.stoneState.backgroundType = 82; this.text = new GumpText(fonts); const lookup = (token) => this.nodes.get(token); this.factory = new GumpCreate(this.create, { node: lookup, allocate: (size) => { assert.equal(size, 160); return 1; } }); this.factory.initializeRoot(); this.collection = new GumpCollection(this.create.collection, this.factory.collectionHooks());
        this.branch = { node: lookup, redraw: () => {}, push: (token) => this.tree.push(token), attach: (...args) => this.tree.attach(...args), kill: (token) => this.lifetime.kill(token) }; this.lifeHooks = { node: lookup, push: this.branch.push, deletingDestructor: (token, flags) => this.lifetime.deleteScalar(token, flags), release: (token) => { this.released.push(token); this.nodes.delete(token); } }; bindStoneButtonGump(this.branch, this.lifeHooks, (token) => token === 2 ? this.stones : null); this.tree = new GumpTree(this.create, this.treeState, this.branch); this.lifetime = new GumpLifetime(this.create, this.lifeState, this.lifeHooks); this.events = new GumpEvents(this.create, this.lifeState, this.eventState, this.collection, { node: lookup }); this.input = new GumpInput(this.create, this.lifeState, this.inputState, this.collection, this.events, { node: lookup });
        this.buttons = new ButtonGump(this.create, this.buttonState, this.factory, this.collection, this.events, this.input, this.lifetime, { node: (token) => token === 2 ? this.node : null, baseNode: lookup, redraw: () => {}, backgroundFlags: (token) => token === 3 ? { get value() { return assetsNode.backgroundFlags; }, set value(value) { assetsNode.backgroundFlags = value; } } : null }); const assetsNode = this.background; const bg = { node: (token) => token === 3 ? this.background : null }; if (assets) bindGumpBackgroundAssets(bg, {}, assets); this.backgrounds = new GumpBackgroundCreate(this.create, this.factory, () => -4, bg); this.stoneHooks = { node: (token) => token === 2 ? this.node : null, allocate: (size) => { assert.equal(size, 0xb8); return this.failAllocation ? 0 : 3; }, push: this.branch.push, attach: this.branch.attach }; bindStoneButtonText(this.stoneHooks, this.text); this.stones = new StoneButtonGump(this.create, this.stoneState, this.factory, this.buttons, this.backgrounds, this.tree, this.lifetime, this.stoneHooks);
    }
}
