// 같은 월드 저장소·Gump 트리·dirty 큐에 Sky Overview를 연결한다(cpppj client/RawMinimapControls.cpp/.h, 기준선 1f00a25a).
// 사용: new RawMinimapControls(tree, menus, storage, dirtyState, globals, hooks).construct(); 공통 메뉴는 생성 전 node(token)을 조회할 수 있어야 한다.
// 실제 Session/메뉴 소유자 연결은 후속 조립에서 수행한다. 창이나 DOM은 만들지 않는다.
import { GumpCreateNode } from './gump-create.js';
import { MinimapGeometryState, clampMinimapAttachment, dragMinimap, minimapTarget, planMinimapDraw, rebuildMinimap } from './minimap-geometry.js';
import { RawMetadisplayDirty } from '../core/metadisplay-dirty.js';
import { RawMinimapRaster, createMinimapRasterState } from '../core/raw-minimap-raster.js';

// 미니맵의 원본 가상표와 좌클릭/해제 사건 비트다.
const MINIMAP_VTABLE = 0x0050d51c;
const LEFT = 0x10000;
const RELEASE = 0x80000000;

// 원본 미니맵의 파생 노드다. 실제 Client Gump 등록표가 기본 노드와 파생 노드를 구별하므로 별도 클래스로 둔다.
// 사용: RawMinimapControls가 내부에서 하나만 만들며 호출자는 `tree.token(node)`로 같은 주소를 등록한다. (2026-11-20 WB04-E1082: 앱 Gump 루트 등록용으로 추가)
export class MinimapNode extends GumpCreateNode {}

// Client 수명 전역을 생성한다. current/serial과 색표는 개별 창보다 오래 산다.
export function createRawMinimapGlobals() { return { current: 0, serial: 0, colors: createMinimapRasterState() }; }

// 공유 갱신 소유자가 있으면 그 실제 DWORD를 사용한다.
function setRefresh(state, value) { if (state.fullRefreshOwner) state.fullRefreshOwner.value = value >>> 0; else state.fullRefresh = value >>> 0; }
// 공유 갱신 DWORD의 현재 값이다.
function getRefresh(state) { return state.fullRefreshOwner ? state.fullRefreshOwner.value : state.fullRefresh; }

// 하나의 고정 Gump와 현재 월드 통지 대여를 소유한다.
export class RawMinimapControls {
    // 트리와 메뉴, 장기 물리, 공유 표시 상태, 현재 입력 장치다.
    #tree; #menus; #storage; #state; #globals; #hooks;
    // 같은 기본 노드와 지도 기하, 셀 화가, dirty 처리기다.
    #node = new MinimapNode(); #token = 0; #geometry = new MinimapGeometryState(); #raster; #dirty;
    // 현재 대여한 Pop/Unpop, 외부 표시 등록 및 갱신 여부다.
    #pop = null; #unpop = null; #registration; #modified = false;

    // 모든 외부 장치와 실제 10.78 물리를 확인하고 같은 메뉴에 표시 처리기를 등록한다.
    constructor(tree, menus, storage, state, globals, hooks) {
        if (!tree || !menus || storage?.pool?.edition !== 'patch1078' || !storage.hash || !Array.isArray(storage.types) || !state || !globals?.colors) throw new TypeError('미니맵의 실제 10.78 물리와 메뉴 필요');
        for (const name of ['view', 'center', 'move', 'inputMode', 'poll']) if (typeof hooks?.[name] !== 'function') throw new TypeError(`미니맵 장치 누락: ${name}`);
        if (typeof menus.registerExternalGumps !== 'function' || typeof menus.unregisterExternalGumps !== 'function') throw new TypeError('미니맵 메뉴 표시 등록 경계 누락');
        this.#tree = tree; this.#menus = menus; this.#storage = storage; this.#state = state; this.#globals = globals; this.#hooks = hooks;
        this.#raster = new RawMinimapRaster(storage.pool, storage.hash, storage.types, globals.colors);
        this.#dirty = new RawMetadisplayDirty(storage.pool, state, {
            rebuild: () => { this.#syncSize(); if (rebuildMinimap(this.#geometry, hooks.view())) { tree.requestRedrawBase(this.#token, false); this.#modified = true; } },
            redrawCell: (x, y) => { globals.colors.challenge = storage.ownerMode.challenge; globals.colors.frame = state.frames; this.#raster.drawCell(state.bitmap, x, y); this.#modified = true; },
        });
        this.#registration = menus.registerExternalGumps({ owns: (value) => !!value && value === this.#token, draw: (value, pixels) => this.draw(value, pixels) });
    }

    // 현재 Gump 기본 크기를 기하 처리기에 동기화한다.
    #syncSize() { this.#geometry.width = this.#node.baseWidth; this.#geometry.height = this.#node.baseHeight; }
    // 소유 노드만 반환한다.
    node(token) { return token && token === this.#token ? this.#node : null; }
    // 현재 표시 토큰이다.
    get rootToken() { return this.#token; }
    // 같은 물리의 Pop/Unpop에만 통지를 대여한다.
    bindWorld(core) {
        const s = this.#storage;
        if (this.#pop || this.#unpop || core?.pool !== s.pool || core.hash !== s.hash || core.unpop?.pool !== s.pool || core.pop?.pool !== s.pool || core.ownerMode !== s.ownerMode || core.types !== s.types || core.spots !== s.spots) throw new TypeError('미니맵 현재 월드 물리/대여 불일치');
        this.#pop = core.pop; this.#unpop = core.unpop; this.#pop.setMetadisplayDirty(this.#dirty); this.#unpop.setMetadisplayDirty(this.#dirty); setRefresh(this.#state, 1);
    }
    // 통지만 반납하고 장기 bitmap은 보존한다.
    unbindWorld() { if (this.#pop) this.#pop.releaseMetadisplayDirty(this.#dirty); if (this.#unpop) this.#unpop.releaseMetadisplayDirty(this.#dirty); this.#pop = null; this.#unpop = null; }
    // 원본 생성자는 현재 bitmap 폭으로 기본 노드를 만든 뒤 현재 포인터와 serial을 기록한다.
    construct() {
        if (this.#token) throw new Error('현재 미니맵 중복 생성');
        this.#token = this.#tree.token(this.#node); this.#tree.factory.construct(this.#token, 0, this.#state.bitmapWidth, this.#state.bitmapWidth, 0x301);
        this.#node.vtable = MINIMAP_VTABLE; this.#node.command = 29; this.#geometry = new MinimapGeometryState();
        this.#menus.setHelpText(this.#token, 'Sky Overview'); this.#menus.setHelpWord(this.#token, 'minimapGump');
        this.#globals.serial = this.#node.serial; this.#globals.current = this.#token; this.#state.displayPresent = true; this.#state.displayFlags = this.#node.flags & 255; return this.#token;
    }
    // bitmap을 확보하고 기존 Client의 기본 128 크기와 루트 부착을 적용한다.
    open() {
        this.#dirty.refresh(); this.construct(); this.#node.baseWidth = this.#node.width = 128; this.#node.baseHeight = this.#node.height = 128;
        this.#state.displayPresent = true; this.#state.displayFlags = this.#node.flags & 255; setRefresh(this.#state, 1);
        this.attach(this.#token, 0, 0, this.#tree.create.collection.root, 0); this.#node.flags = (this.#node.flags | 0x20000) >>> 0;
    }
    // 같은 공통 수명으로 표시를 정리한다.
    close() { if (this.#token) { this.#tree.lifetime.kill(this.#token); this.#tree.lifetime.cleanup(); } }
    // 현재 Gump 반납, World 대여 해제, 외부 Draw 등록 제거 순서다.
    dispose() { this.close(); this.unbindWorld(); if (this.#registration !== null) { this.#menus.unregisterExternalGumps(this.#registration); this.#registration = null; } }
    // 기본 HUD mode3의 크기와 위치를 지정한다.
    resize(height) {
        if (!this.#token || height <= 80) throw new TypeError('미니맵 실제 화면 높이/수명 오류');
        const y = Math.fround(height - 75); const n = this.#node; if (n.baseWidth === 80 && n.baseHeight === 75 && n.x === 0 && n.y === y) return;
        n.baseWidth = n.width = 80; n.baseHeight = n.height = 75; this.#tree.tree.push(this.#token); this.attach(this.#token, 0, y, this.#tree.create.collection.root, 0); this.#tree.changed();
    }
    // 실제 팔레트 검색 결과를 미니맵 전용 색 전역에 반영한다.
    setPalette(palette) {
        if (!palette?.named || palette.named.length < 20) throw new TypeError('미니맵 팔레트 색 전역 누락');
        const c = this.#globals.colors; const before = [c.black, ...c.owners, c.neutralIsle, c.neutralNoIsland, c.challengeGreen];
        c.black = palette.basic[0]; c.owners = [palette.named[11].index, palette.basic[1], palette.basic[7], palette.named[10].index, palette.basic[4], palette.basic[5], palette.named[12].index, palette.basic[6]];
        c.neutralIsle = palette.named[18].index; c.neutralNoIsland = palette.named[19].index; c.challengeGreen = palette.basic[2];
        if (before.some((value, index) => value !== [c.black, ...c.owners, c.neutralIsle, c.neutralNoIsland, c.challengeGreen][index])) setRefresh(this.#state, getRefresh(this.#state) + 1);
    }
    // 현재 표시일 때만 카메라와 dirty 큐를 소비한다.
    refresh() {
        if (!this.#token || this.#globals.current !== this.#token) return false;
        const view = this.#hooks.view(); this.#modified = false; this.#state.displayPresent = true; this.#state.displayFlags = this.#node.flags & 255;
        this.#dirty.refresh({ camera: view.camera, rectangle: [view.viewport.left, view.viewport.top, view.viewport.right, view.viewport.bottom] }); return this.#modified;
    }
    // bitmap crop과 카메라 외곽 네 선을 현재 표면에 출력한다.
    draw(token, target) {
        if (!this.node(token)) throw new RangeError('미니맵 표시 대상 없음'); if (!this.#state.bitmap) return;
        this.#syncSize(); const p = planMinimapDraw(this.#geometry, this.#hooks.view(), this.#tree.collection.position(token)); const bounds = this.#tree.collection.bounds(token);
        target.rawBitmap(this.#state.bitmap, 128, BigInt(p.source.y) * 128n + BigInt(p.source.x), p.size, p.destination, bounds);
        const { left, top } = p.camera; const right = p.camera.right - 1; const bottom = p.camera.bottom - 1; const color = this.#globals.colors.owners[2];
        target.linesClipped([{ first: { x: left, y: top }, last: { x: right, y: top }, color }, { first: { x: left, y: bottom }, last: { x: right, y: bottom }, color }, { first: { x: left, y: top }, last: { x: left, y: bottom }, color }, { first: { x: right, y: top }, last: { x: right, y: bottom }, color }], bounds);
    }
    // 부착 직전 crop을 조정하고 실제 공통 트리에 위임한다.
    attach(token, x, y, parent, mode) { if (!this.node(token)) throw new RangeError('미니맵 부착 대상 없음'); this.#syncSize(); clampMinimapAttachment(this.#geometry); this.#tree.tree.attach(token, x, y, parent, mode); }
    // 선택 이동 모드와 카메라 드래그를 구별하고 나머지는 기본 Help에 전달한다.
    handle(token, event, first, second) {
        if (!this.node(token)) throw new RangeError('미니맵 입력 대상 없음'); this.#syncSize(); const local = this.#tree.collection.screenToLocal(token, { x: first | 0, y: second | 0 });
        if (((event >>> 0) & 0x03ffffff) === LEFT) {
            if ((event >>> 0) & RELEASE) { this.#geometry.scrolling = 0; this.#tree.lifetime.setIdle(token, false); return 1; }
            const [x, y] = minimapTarget(this.#geometry, local[0], local[1]); const mode = this.#hooks.inputMode();
            if (mode === 2 || mode === 5) { this.#hooks.move(x, y); return 1; }
            this.#hooks.center(x, y); this.#tree.requestRedrawBase(token, false); this.#tree.input.capture(token); this.#geometry.scrolling = 1; this.#tree.lifetime.setIdle(token, true);
        }
        return this.#menus.baseHandle(token, event, first, second) >>> 0;
    }
    // 캡처 중 현재 포인터를 폴링해 경계 밖 crop 이동과 카메라 중심 변경을 수행한다.
    idle(token) {
        if (!this.node(token) || !this.#geometry.scrolling) throw new Error('미니맵 드래그 Idle 상태 오류'); this.#syncSize(); const mouse = this.#hooks.poll();
        const local = this.#tree.collection.screenToLocal(token, { x: mouse.x, y: mouse.y }); const [x, y] = dragMinimap(this.#geometry, local[0], local[1]); this.#hooks.center(x, y); this.#tree.requestRedrawBase(token, false);
    }
    // 기본 scalar 소멸자에 위임한다.
    deleteScalar(token, flags) { this.#tree.lifetime.deleteScalar(token, flags); }
    // 이전 노드 반납이 새 현재 표시를 지우지 않도록 토큰을 대조한다.
    release(token) { if (token === this.#token) { this.#token = 0; if (this.#globals.current === token) this.#state.displayPresent = false; } }
}
