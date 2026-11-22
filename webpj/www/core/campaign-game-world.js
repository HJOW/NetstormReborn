// 지연 로그인 캠페인의 GameWorld 앞단을 같은 Session·카메라·자산 표시기와 묶는다.
// 사용: new CampaignGameWorld(assets, files, fort, mission, services, { x, y }) → startLocalConnection() → 같은 session의 로그인 진행 → dispose().
// 화면 이벤트와 브라우저 앱 수명은 후속이며 이 생성자만으로 플레이할 수 없다.
import { ClientGameplayGlobals } from './client-gameplay-globals.js';
import { FIRST_ASSET_TYPE_NUMBER } from '../assets/type-table.js';
import { TypeFlag2 } from '../assets/type-flags.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { RawUserHighlight } from './raw-user-highlight.js';
import { RawCursorPosition } from './raw-cursor-position.js';
import { RawCarrierDropPoint } from './raw-carrier-drop-point.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawUserPick } from './raw-user-pick.js';
import { selectNextMovingUnit, selectPriestByKey } from './selection-shortcuts.js';
import { gameMenuCommandValue } from './game-menu-commands.js';
import { createCameraHomeState } from './camera-home.js';
import { RawCampaignSession } from './raw-campaign-session.js';
import { RawCampaignFactoryDisplay } from '../render/raw-campaign-factory-display.js';
import { cursorDisplayLogicalPalette } from '../render/raw-campaign-cursor-display.js';
import { traceSpecialPointer } from '../ui/special-pointer-trace.js';

// 원본 기본 지도 영역에서 상단 게임 메뉴가 차지하는 논리 픽셀 높이다.
const MENU_HEIGHT = 28;

// 단정도 좌표와 원본 DWORD 비트를 손실 없이 교환한다.
function floatBits(value) { const bytes = new DataView(new ArrayBuffer(4)); bytes.setFloat32(0, value, true); return bytes.getUint32(0, true); }
function bitsFloat(value) { const bytes = new DataView(new ArrayBuffer(4)); bytes.setUint32(0, value >>> 0, true); return bytes.getFloat32(0, true); }

// 실제 캠페인 Session과 표시기의 지연 로그인 수명이다.
export class CampaignGameWorld {
    // 같은 Client 전역·Session·표시기와 화면/외부 표시 장치다.
    #gameplay; #session = null; #factoryDisplay = null; #width; #height; #services; #cameraHome = createCameraHomeState();
    // 로그인 대기·화면 변경 및 외곽 반납 상태다.
    #connecting = false; #changed = false; #disposed = false;
    // 독립 Step과 외부 시계의 마지막 실제 프레임 시각이다.
    #surfaceTime = { game: 0, wall: 0, delta: 0, number: 0 };
    // 현재 실제 자산 보기와 그 보기를 구성하는 같은 자산 저장소다.
    #assets; #objects = [];
    // 현재 파일 팔레트·ColorSort 대여와 커서 설치 전까지 보유하는 색 사본이다.
    #palette = null; #pendingPalette = null; #colorSortLogical = []; #colorSortMaps = null;
    // 선택 입력 부분은 같은 Session과 장기 선택 전역을 빌린다.
    #selection = null; #input = null;
    // Client가 설치하기 전에는 원본 초기값 0인 현재 건물 숨김 조회다.
    #hideBuildings = () => 0;

    // 화면 크기와 자산을 확인하고 실제 Session과 표시기를 만든다. 로그인은 호출자가 나중에 시작한다.
    constructor(assets, files, fort, mission, services, viewport) {
        if (!viewport || !(viewport.x > 0) || !(viewport.y > 0)) throw new RangeError('실제 캠페인 설치 전 화면 크기 누락');
        this.#width = viewport.x; this.#height = viewport.y; this.#services = { ...services }; this.#assets = assets;
        const storage = services?.surfaceStorage;
        this.#gameplay = services?.gameplayGlobals ?? new ClientGameplayGlobals(services?.subscriberGlobals, storage?.authority, storage?.gameAux);
        const otherDeletion = services?.deletion?.emit;
        const input = { ...services, gameplayGlobals: this.#gameplay, install: { ...services?.install, resetCamera: () => this.resetCampaignCamera() },
            deletion: { ...services?.deletion, selected: () => this.#gameplay.selection.selected & 0xffff, emit: (event) => {
                if (event.effect === SQUID_DESTROY_EFFECT.clearSelection) { this.clearSelection(); return; }
                if (typeof otherDeletion !== 'function') throw new Error('GameWorld의 다른 파생 삭제 효과 미연결'); otherDeletion(event);
            } } };
        try {
            this.#session = new RawCampaignSession(assets, files, fort, mission, input);
            this.#factoryDisplay = new RawCampaignFactoryDisplay(this.#session.core(), assets, (sid, frame) => this.#session.drawTemple(sid, frame), this.#gameplay.patchDiagnostic());
            if (services?.ownerColorTables) this.#factoryDisplay.bindOwnerColors(services.ownerColorTables);
            let palette;
            if (services?.screenPalette) {
                palette = services.screenPalette();
                if (!(palette instanceof Uint8Array) || palette.length !== 1024) throw new TypeError('World의 현재 Screen 팔레트 계약');
            } else {
                palette = new Uint8Array(1024); const rgb = assets.palette.rgb;
                // 독립 호출자는 실제 파일 팔레트를 원본 화면 장치의 BGRX 순서로 복사한다.
                for (let index = 0; index < 256; index += 1) {
                    palette[index * 4] = rgb[index * 3 + 2]; palette[index * 4 + 1] = rgb[index * 3 + 1]; palette[index * 4 + 2] = rgb[index * 3];
                }
            }
            const colors = services?.colorSortInput;
            this.#palette = palette.slice(); this.#colorSortLogical = colors?.mapsReady ? colors.logical : []; this.#colorSortMaps = colors?.mapsReady ? colors.maps : null;
            this.#factoryDisplay.setPalette(palette, this.#colorSortLogical, this.#colorSortMaps);
            if (!services?.localLogin) throw new Error('실제 Client 지연 로그인 장치 누락');
        } catch (error) {
            this.#factoryDisplay = null;
            try { this.#session?.dispose(); }
            catch (cleanupError) { console.error('캠페인 GameWorld 생성 실패 후 정리 오류:', cleanupError); }
            finally { this.#session = null; this.#disposed = true; }
            throw error;
        }
    }

    // 반납한 World가 이전 Session이나 표시기를 다시 공개하지 못하게 한다.
    #live() { if (this.#disposed || !this.#session) throw new Error('반납한 캠페인 GameWorld'); return this.#session; }
    // 같은 실제 Session을 빌린다. 외곽이 더 오래 살아야 한다.
    session() { return this.#live(); }
    // 같은 Session의 풀·자산·신전 그림을 빌리는 표시기다.
    factoryDisplay() { this.#live(); return this.#factoryDisplay; }
    // World와 Session이 함께 쓰는 Client 카메라다.
    cameraGlobals() { this.#live(); return this.#gameplay.camera(); }
    // 원본 도킹 장치가 없으면 상단 메뉴 아래의 논리 화면을 사용한다.
    mapView() { this.#live(); return this.#services.mapView ? this.#services.mapView() : { left: 0, top: Math.min(MENU_HEIGHT, this.#height), right: this.#width, bottom: this.#height }; }
    // 호스트 논리 크기를 바꾸되 현재 카메라와 대기 목표는 유지하고 새 한계·표면 보기를 갱신한다.
    resize(width, height) {
        this.#live();
        if (!Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0) throw new RangeError('캠페인 World 화면 정수 크기 오류');
        if (width === this.#width && height === this.#height) { this.#surfaceView(); return; }
        this.#width = width; this.#height = height;
        const cells = this.#gameplay.worldCells().value;
        this.#gameplay.camera().updateTargetLimits(width, height, cells, cells);
        this.#surfaceView(); this.#changed = true;
    }
    // 카메라 적용 뒤 같은 영역을 표면·입자·선택적 위치 음향에 전달한다.
    #surfaceView() {
        const session = this.#live(), pixels = this.#gameplay.camera().rendererView().pixels, clip = this.mapView();
        const view = { cameraX: pixels.x, cameraY: pixels.y, left: clip.left, top: clip.top, right: clip.right, bottom: clip.bottom };
        session.world().setDisplay({ cameraX: pixels.x, cameraY: pixels.y, zoom: 65536, viewport: clip }, this.#services.surfaceDisplay);
        session.setParticleView(view, this.#services.surfaceDisplay, false);
        if (this.#services.soundViewChanged) this.#services.soundViewChanged(view);
    }
    // 프레임 말미에 적용한 실제 카메라를 같은 표면·입자·음향에 공개한다.
    cameraApplied() { this.#surfaceView(); this.#changed = true; }
    // 실제 카메라 적용 장치 뒤 픽셀 또는 배율이 바뀌면 현재 표면과 음향을 갱신한다.
    #commitCamera() {
        const session = this.#live(), camera = this.#gameplay.camera(), before = camera.rendererView().pixels, zoom = camera.rendererView().zoom;
        session.commitRequestedCamera(this.#width, this.#height);
        const after = camera.rendererView().pixels;
        if (before.x !== after.x || before.y !== after.y || zoom !== camera.rendererView().zoom) { this.#surfaceView(); this.#changed = true; }
    }
    // 원본 로더가 호출하는 현재 사제 기준 초기 카메라다.
    resetCampaignCamera() {
        const session = this.#live(); this.#cameraHome.width = this.#width; this.#cameraHome.height = this.#height;
        session.resetCamera(this.#cameraHome, () => this.#commitCamera());
    }
    // 외곽이 Session을 공개한 뒤 같은 World의 비동기 로컬 연결을 시작한다.
    startLocalConnection() {
        const session = this.#live(); if (this.#connecting || session.hasCursorRuntimeForState()) throw new Error('비동기 World 로그인 새 수명 계약 오류');
        this.#connecting = true; session.startLocalConnection();
    }
    // 이전 World를 반납한 State 부모가 같은 Session의 후반 로그인을 준비한다.
    prepareLocalConnectionForState() {
        const session = this.#live(); if (this.#connecting || session.hasCursorRuntimeForState()) throw new Error('부모 State7 World 로그인 새 수명 계약 오류');
        this.#connecting = true; session.prepareLocalConnectionForState();
    }
    // 실제 Ready 뒤 현재 슬롯의 표시·선택용 보기를 갱신하고 한 번만 로그인 관문을 연다.
    completeLocalConnection() {
        const session = this.#live(); if (!this.#connecting || !session.localConnectionReady()) return false;
        if (this.#pendingPalette) { session.cursor().setPalette(this.#pendingPalette, this.#colorSortLogical, this.#colorSortMaps); this.#pendingPalette = null; }
        this.refreshCampaignObjects(); this.#connecting = false; return true;
    }
    // 접속이 끝난 뒤 실제 전송과 Session Kernel을 같은 프레임에 실행한다.
    runFrame() {
        const session = this.#live(); if (this.#connecting) return;
        const stopped = Boolean(this.#services.paused?.());
        if (this.#services.frameTime) {
            this.#surfaceTime = { ...this.#services.frameTime() }; this.#surfaceView();
            session.pumpLocalConnection(); session.runFrame(this.#surfaceTime, stopped); this.refreshCampaignObjects();
        } else if (this.#services.elapsed && !stopped) this.step(this.#services.elapsed());
    }
    // Client 단일 Kernel 전에 시각·표시 영역만 준비하고 Kernel을 직접 돌리지 않는다.
    prepareKernelFrame() {
        const session = this.#live(); if (typeof this.#services.frameTime !== 'function') throw new Error('실제 Client Kernel 프레임의 Session/시각 공급자 누락');
        if (this.#connecting) return;
        this.#surfaceTime = { ...this.#services.frameTime() }; this.#surfaceView();
        // 원본 주 루프는 Kernel 앞에서 RunClientBridgeDecay 를 부른다. 다리 붕괴 검사를 준비보다 먼저 한다(Session.runFrame 과 같은 순서). (2026-11-22 WB04-E1230)
        const paused = Boolean(this.#services.paused?.()); session.scanBridgeDecay(this.#surfaceTime, paused); session.prepareFrame(this.#surfaceTime, paused);
    }
    // 단일 Kernel 뒤 현재 객체 보기만 다시 읽는다.
    // 원문 UberGump::PollBattleResources 의 월드 분기이다: 접속(Fort 준비) 중이 아니면 Session 의 전투 자원 프레임을 실행한다. (2026-11-22 WB04-E1232)
    runBattleResources() { const session = this.#live(); if (!this.#connecting) session.runBattleResources(); }
    finishKernelFrame() { this.#live(); if (!this.#connecting) this.refreshCampaignObjects(); }
    // 독립 실행에서는 시간차를 누적하고 실제 로그인 전송·Session Kernel을 진행한다.
    step(seconds) {
        const session = this.#live(); if (this.#connecting) return;
        if (!Number.isFinite(seconds) || seconds < 0) throw new RangeError('월드 시간차 범위 오류');
        this.#surfaceTime.game += seconds; this.#surfaceTime.delta = seconds; this.#surfaceTime.number += 1;
        this.#surfaceView(); this.#surfaceTime.wall += seconds; session.pumpLocalConnection(); session.runFrame(this.#surfaceTime, false); this.refreshCampaignObjects();
    }
    // 현재 화면 색을 표시기에 적용하고 아직 없는 커서의 원래 색은 값으로 보유한다.
    setPalette(palette, colorSortLogical = [], colorMaps = null) {
        const session = this.#live(), logical = cursorDisplayLogicalPalette(palette), physical = new Uint8Array(1024);
        // 논리 RGBX를 같은 색 번호의 화면 BGRX로 복사한다.
        for (let index = 0; index < 256; index += 1) {
            const color = logical[index], at = index * 4;
            physical[at] = (color >>> 16) & 255; physical[at + 1] = (color >>> 8) & 255; physical[at + 2] = color & 255; physical[at + 3] = color >>> 24;
        }
        this.#palette = physical;
        if (colorSortLogical.length !== 0 && colorSortLogical.length !== 256) throw new TypeError('World 고정 ColorSort 256색 계약');
        this.#colorSortLogical = colorSortLogical; this.#colorSortMaps = colorMaps;
        if (session.hasCursorRuntimeForState()) { session.cursor().setPalette(physical, colorSortLogical, colorMaps); this.#pendingPalette = null; }
        else this.#pendingPalette = physical.slice();
        this.#factoryDisplay?.setPalette(physical, colorSortLogical, colorMaps); this.#changed = true;
    }
    // 같은 실제 자산 슬롯의 좌표·프레임·체력을 읽어 원본 월드 표시 보기를 갱신한다.
    refreshCampaignObjects() {
        const session = this.#live(), core = session.core(), hp = session.hitPoints(), assets = this.#assets;
        this.#objects = [];
        // 로더가 제공한 현재 활성 자산만 순서대로 읽는다.
        for (const snapshot of session.world().objects()) {
            const raw = snapshot.object, index = raw.type - FIRST_ASSET_TYPE_NUMBER;
            if (index < 0 || index >= assets.types().length) continue;
            const type = assets.types()[index], slot = core.pool.slot(snapshot.sid), frame = Math.max(raw.frame, 0);
            const pictures = assets.shapes.blocks[type.block].frames;
            const visible = type.assetName.toLowerCase() !== 'noisland' && frame < pictures.length && !pictures[frame].special;
            const selectable = visible && !(type.definition?.flags ?? []).some((flag) => flag.toLowerCase() === 'not_selectable');
            const view = { id: snapshot.sid, type: raw.type, owner: raw.owner, x: raw.x, y: raw.y, cell: { x: Math.trunc(raw.x), y: Math.trunc(raw.y) },
                frame, initialFrame: frame, mobile: (core.types[raw.type].flags2 & TypeFlag2.walker) !== 0, visible, selectable, hitPoints: 0, maxHitPoints: 0 };
            if (hp.hasHitPoints(snapshot.sid)) { view.maxHitPoints = hp.maxHitPoints(snapshot.sid); view.hitPoints = new DataView(slot.buffer, slot.byteOffset, slot.byteLength).getInt32(26, true); }
            this.#objects.push(view);
        }
        const selection = this.#gameplay.selection;
        if (selection.selected && !this.#objects.some((view) => view.id === selection.selected)) selection.selected = 0;
    }
    // 현재 Session의 선택 처리기에 같은 장기 전역을 빌려준다. 강조·운반 입력은 외곽에서 나중에 연결한다.
    bindUserSelection(input = {}) {
        const session = this.#live(); if (this.#selection) throw new Error('실제 월드 선택의 새 수명 계약');
        const state = this.#gameplay.selection;
        this.#selection = session.makeUserSelection(state.selectionState(), { ...input, enabled: () => state.enabled,
            appendMultiple: (sid) => state.appendMultiple(sid), removeMultiple: (sid) => state.removeMultiple(sid), writeEnabled: (value) => { state.enabled = value >>> 0; } });
        return this.#selection;
    }
    // 현재 카메라 snapshot을 사용해 정수 화면 좌표를 원본 월드 좌표로 역투영한다.
    #project(x, y) { const view = this.#gameplay.camera().snapshot().data; return RawCursorPosition.screenToWorld(x, y, [view.getUint32(0, true), view.getUint32(4, true)]); }
    // 같은 Session의 운반 입력·강조·선택을 현재 카메라와 하나의 선택 전역에 조립한다.
    bindUserInput(selectionHooks = {}, highlightHooks = {}, carrierHooks = {}) {
        const session = this.#live(); if (this.#input || this.#selection) throw new Error('실제 월드 입력의 새 수명 계약');
        const state = this.#gameplay.selection; let highlight;
        const carrier = session.makeCarrierInput({ ...carrierHooks, highlight: (mode) => highlight.highlight(mode), multiple: () => state.multipleCount > 0 ? 1 : 0 });
        highlight = new RawUserHighlight(state.highlightState(), { ...highlightHooks,
            project: (x, y) => this.#project(x, y).map(floatBits),
            move: (sid, x, y, force) => carrier.highlight(sid, { x: bitsFloat(x), y: bitsFloat(y) }, force),
        });
        this.bindUserSelection({ ...selectionHooks, highlight: (mode) => highlight.highlight(mode) });
        this.#input = { carrier, highlight };
    }
    // 현재 Client의 건물 숨김 DWORD 조회기를 빌린다.
    bindHideBuildings(current) { this.#live(); if (typeof current !== 'function') throw new TypeError('건물 숨김의 현재 조회기 누락'); this.#hideBuildings = current; this.#changed = true; }
    // 화면 영역 안에서 같은 실제 해시·카메라·픽셀 판정으로 현재 후보를 찾는다.
    pick(x, y, force = 0) {
        const session = this.#live(), view = this.mapView();
        if (y < view.top || x < view.left || y >= view.bottom || x >= view.right) return 0;
        const core = session.core(), finder = new RawSquidFinder(core.pool, core.hash, core.types);
        const hooks = {
            paused: () => this.#services.paused ? this.#services.paused() : 0, localOwner: () => session.localOwner(),
            metamap: () => core.ownerMode.challenge ? 1 : 0, hideBuildings: () => this.#hideBuildings(), project: (px, py) => this.#project(px, py),
            begin: (point, radiusX, radiusY) => { const snap = RawCarrierDropPoint.snap(point), cx = snap[0] | 0, cy = snap[1] | 0; return finder.begin({ left: cx - radiusX, top: cy - radiusY, right: cx + radiusX, bottom: cy + radiusY }, 0); },
            next: () => finder.next(), center: (sid) => core.neighbors.center(sid),
            hit: (sid, px, py) => { const camera = this.#gameplay.camera().snapshot().pixels; return this.#factoryDisplay.hit(sid, px, py, [camera.x, camera.y], session.localOwner(), (object, bx, by) => session.drawBridgeAdditional(object, bx, by)); },
        };
        return new RawUserPick(core.pool, core.types, hooks).pick(x, y, force);
    }
    // 실제 선택의 세 인자를 원본처럼 유지하고 다음 화면 갱신을 요청한다.
    select(sid) { this.#live(); if (!this.#input) throw new Error('실제 선택 장치 미설치'); this.#selection.select(sid, 0, 1); this.#changed = true; }
    // 일반 좌클릭 선택 뒤 현재 특수 권한으로 같은 지점의 진단을 수행한다.
    selectFromPointer(sid, force, control, event) {
        const session = this.#live(); if (!this.#input) throw new Error('일반 좌클릭의 실제 선택 장치 미설치');
        this.#selection.select(sid, force, 1);
        const core = session.core(), finder = new RawSquidFinder(core.pool, core.hash, core.types);
        traceSpecialPointer(control, this.#gameplay.player.special, event, {
            project: (x, y) => this.#project(x, y),
            begin: (point) => { const p = RawCursorPosition.truncatePoint(point); return finder.begin({ left: p[0], top: p[1], right: p[0], bottom: p[1] }, 0); },
            trace: (number) => { if (typeof this.#services.traceObject !== 'function') throw new Error('현재 지점의 실제 객체 진단 미연결'); this.#services.traceObject(number); },
            next: () => finder.next(),
        });
        if (session.editor() && this.#gameplay.selection.selected) throw new Error('웹 제외 편집 좌클릭 미연결');
        this.#changed = true;
    }
    // 실제 폴링의 화면 정수 좌표를 같은 Client 선택 전역에 기록한다.
    setMousePoint(x, y) { this.#live(); this.#gameplay.selection.mouse.x = x | 0; this.#gameplay.selection.mouse.y = y | 0; }
    // 현재 강조 모드와 특수 선택 권한은 같은 Client 전역에서 읽는다.
    inputMode() { this.#live(); return this.#gameplay.selection.mode >>> 0; }
    specialSelection() { this.#live(); return this.#gameplay.player.special >>> 0; }
    // 현재 Client 커서의 원본 회전 모드 DWORD다.
    cursorRotationMode() { this.#live(); return this.#gameplay.cursorGlobals().rotationMode().value >>> 0; }
    // 현재 마우스와 선택 전역을 사용하는 강조 모드다.
    highlight(mode) { this.#live(); if (!this.#input) throw new Error('실제 선택/강조 장치 미설치'); this.#input.highlight.highlight(mode); }
    // 현재 SID의 위치·발자국을 읽어 같은 즉시 카메라 적용 장치로 이동한다.
    centerOnObject(sid) {
        const session = this.#live(); this.#cameraHome.width = this.#width; this.#cameraHome.height = this.#height;
        session.centerCameraOn(sid, this.#cameraHome, () => this.#commitCamera());
    }
    // 원본 ViewYourPriest는 현재 참여 사제 또는 대체 대상의 위치를 같은 카메라로 본다.
    viewYourPriest(owner) {
        const session = this.#live(); this.#cameraHome.width = this.#width; this.#cameraHome.height = this.#height;
        session.viewYourPriest(owner, this.#cameraHome, () => this.#commitCamera());
    }
    // 같은 풀에서 다음 이동 유닛을 선택하고 원본 다중 선택과 카메라 순서를 유지한다.
    selectNextUnit(add) {
        const session = this.#live(); if (!this.#input) throw new Error('다음 유닛 선택의 실제 월드/입력 없음');
        const core = session.core(), state = this.#gameplay.selection;
        selectNextMovingUnit(core.pool, core.types, state, add, {
            localOwner: () => session.localOwner(), paused: () => this.#services.paused ? this.#services.paused() : 0,
            select: (sid, force, activate) => this.#selection.select(sid, force, activate), center: (sid) => this.centerOnObject(sid),
            multiple: (sid, value) => this.#selection.toggleMultiple(sid, value),
        });
        this.#changed = true;
    }
    // 같은 사제 참여 조회·반복 카운터·큐와 커서 취소를 사용하는 P/R 선택이다.
    selectPriest(queue) {
        const session = this.#live(); if (!this.#input) throw new Error('사제 단축키의 실제 월드/입력 없음');
        const current = this.#gameplay.selection, state = { get selected() { return current.selected; }, get repeated() { return current.priestSelections; }, set repeated(value) { current.priestSelections = value >>> 0; } };
        selectPriestByKey(state, {
            localOwner: () => session.localOwner(), find: (owner) => session.findPriestParticipation(owner),
            select: (sid, force, activate) => this.#selection.select(sid, force, activate),
            viewCommand: () => { const value = gameMenuCommandValue(0x005429d4); if (value === null) throw new Error('사제 보기 명령 누락'); return value; }, queue,
            pieceHeld: () => { const drop = session.cursorDropSettings(); return drop.lockDepth === 0 && drop.preview !== 0; }, cancelPiece: () => session.cursor().cancel(),
        });
        this.#changed = true;
    }
    // 같은 선택 처리기의 운반체 또는 아처 메뉴 입구에서 현재 공통 메뉴를 호출한다.
    openSelectionMenu(sid, event, archer, common) {
        this.#live(); if (!this.#input) throw new Error('실제 객체 메뉴 선택 장치 미설치');
        return archer ? this.#selection.archerMenu(sid, event, common) : this.#selection.carrierMenu(sid, event, common);
    }
    // 현재 마우스 위치는 역투영 뒤 정수 절삭하여 선택된 운반체의 이동 커서를 다시 평가한다.
    updateMovementCursor() {
        this.#live(); const state = this.#gameplay.selection;
        if (this.#input && (state.mode === 2 || state.mode === 5)) {
            const point = RawCursorPosition.truncatePoint(this.#project(state.mouse.x, state.mouse.y));
            this.#input.carrier.highlight(state.selected, { x: point[0], y: point[1] }, 0);
        }
    }
    // 이동 모드의 Shift 클릭은 실제 다중 선택 처리기에 전달한다.
    extendSelection(sid, force) {
        this.#live(); if (!this.#input) throw new Error('다중 선택의 실제 월드/입력 없음');
        this.#selection.extendSelection(sid, force); this.#changed = true;
    }
    // 클릭 좌표를 역투영·절삭하고 명령 큐, 다중 선택, 현재 기본 선택 순서로 이동한다.
    moveSelected(x, y, destination) {
        this.#live(); if (!this.#input) throw new Error('실제 클릭 명령 장치 미설치');
        const point = RawCursorPosition.truncatePoint(this.#project(x, y));
        if (destination) destination(point[0] | 0, point[1] | 0);
        this.#selection.moveMultiple(floatBits(point[0]), floatBits(point[1]), (sid, px, py) => this.#input.carrier.commit(sid, { x: bitsFloat(px), y: bitsFloat(py) }));
        const selected = this.#gameplay.selection.selected;
        this.#input.carrier.commit(selected, { x: point[0], y: point[1] }); this.#changed = true; return selected !== 0;
    }
    // 미니맵 누름·드래그의 카메라 중심 요청이다(cpppj `GameWorld::CenterCamera`). 목표만 저장하며 실제 적용은 화면 출력 뒤 카메라 프레임이 맡는다. (2026-11-20 WB04-E1082)
    centerCamera(x, y) { this.#live().requestCameraCenter(x, y, this.#width, this.#height); }
    // 미니맵 좌표의 이동 명령 뒤 같은 강조를 해제한다.
    moveFromMinimap(x, y) {
        this.#live(); if (!this.#input) throw new Error('실제 미니맵 선택 장치 없음');
        this.#input.carrier.commit(this.#gameplay.selection.selected, { x, y }); this.#input.highlight.highlight(0); this.#changed = true;
    }
    // 삭제가 선택 객체를 반납한 뒤에는 원본 Clear만 호출하여 이전 슬롯을 다시 읽지 않는다.
    clearSelection() { this.#live(); if (!this.#selection) throw new Error('미션 선택 해제 장치 미설치'); this.#selection.clear(); this.#changed = true; }
    // 원본 선택 초기화는 활성 이동을 켜지 않는 세 인자를 그대로 전달한다.
    resetSelection() { this.#live(); if (!this.#selection) throw new Error('미션 선택 초기화 장치 미설치'); this.#selection.select(0, 0, 0); this.#changed = true; }
    // 현재 Client의 전체 DWORD 선택 번호다.
    selected() { this.#live(); return this.#gameplay.selection.selected >>> 0; }
    // 호출자가 현재 보기의 좌표나 목록을 수정해 내부 상태를 바꾸지 못하도록 사본을 돌려준다.
    objects() { this.#live(); return this.#objects.map((view) => ({ ...view, cell: { ...view.cell } })); }
    // 완료 공개 전의 로그인 대기 여부이며 Session Ready와 구별한다.
    get connecting() { this.#live(); return this.#connecting; }
    // 화면 변경을 한 번 읽고 원본처럼 지운다.
    takeChanged() { this.#live(); const changed = this.#changed; this.#changed = false; return changed; }
    // 표시기를 Session보다 먼저 놓고 같은 외곽 수명을 반납한다.
    dispose() {
        if (this.#disposed) return;
        this.#input = null; this.#selection = null; this.#factoryDisplay = null;
        try { this.#session?.dispose(); } finally { this.#session = null; this.#disposed = true; }
    }
}
