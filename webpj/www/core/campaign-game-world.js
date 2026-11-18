// 지연 로그인 캠페인의 GameWorld 앞단을 같은 Session·카메라·자산 표시기와 묶는다.
// 사용: new CampaignGameWorld(assets, files, fort, mission, services, { x, y }) → startLocalConnection() → 같은 session의 로그인 진행 → dispose().
// 화면 이벤트·프레임과 브라우저 앱 수명은 후속이며 이 생성자만으로 플레이할 수 없다.
import { ClientGameplayGlobals } from './client-gameplay-globals.js';
import { FIRST_ASSET_TYPE_NUMBER } from '../assets/type-table.js';
import { TypeFlag2 } from '../assets/type-flags.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { RawUserHighlight } from './raw-user-highlight.js';
import { RawCursorPosition } from './raw-cursor-position.js';
import { createCameraHomeState } from './camera-home.js';
import { RawCampaignSession } from './raw-campaign-session.js';
import { RawCampaignFactoryDisplay } from '../render/raw-campaign-factory-display.js';
import { cursorDisplayLogicalPalette } from '../render/raw-campaign-cursor-display.js';

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
    // 현재 실제 자산 보기와 그 보기를 구성하는 같은 자산 저장소다.
    #assets; #objects = [];
    // 현재 파일 팔레트·ColorSort 대여와 커서 설치 전까지 보유하는 색 사본이다.
    #palette = null; #pendingPalette = null; #colorSortLogical = []; #colorSortMaps = null;
    // 선택 입력 부분은 같은 Session과 장기 선택 전역을 빌린다.
    #selection = null; #input = null;

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
            const palette = new Uint8Array(1024), rgb = assets.palette.rgb;
            // 같은 파일 팔레트를 원본 화면 장치의 BGRX 순서로 복사한다.
            for (let index = 0; index < 256; index += 1) {
                palette[index * 4] = rgb[index * 3 + 2]; palette[index * 4 + 1] = rgb[index * 3 + 1]; palette[index * 4 + 2] = rgb[index * 3];
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
    // 카메라 적용 뒤 같은 영역을 표면·입자·선택적 위치 음향에 전달한다.
    #surfaceView() {
        const session = this.#live(), pixels = this.#gameplay.camera().rendererView().pixels, clip = this.mapView();
        const view = { cameraX: pixels.x, cameraY: pixels.y, left: clip.left, top: clip.top, right: clip.right, bottom: clip.bottom };
        session.world().setDisplay({ cameraX: pixels.x, cameraY: pixels.y, zoom: 65536, viewport: clip }, this.#services.surfaceDisplay);
        session.setParticleView(view, this.#services.surfaceDisplay, false);
        if (this.#services.soundViewChanged) this.#services.soundViewChanged(view);
    }
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
    // 같은 Session의 운반 입력·강조·선택을 현재 카메라와 하나의 선택 전역에 조립한다.
    bindUserInput(selectionHooks = {}, highlightHooks = {}, carrierHooks = {}) {
        const session = this.#live(); if (this.#input || this.#selection) throw new Error('실제 월드 입력의 새 수명 계약');
        const state = this.#gameplay.selection; let highlight;
        const carrier = session.makeCarrierInput({ ...carrierHooks, highlight: (mode) => highlight.highlight(mode), multiple: () => state.multipleCount > 0 ? 1 : 0 });
        highlight = new RawUserHighlight(state.highlightState(), { ...highlightHooks,
            project: (x, y) => { const view = this.#gameplay.camera().snapshot().data; const point = RawCursorPosition.screenToWorld(x, y, [view.getUint32(0, true), view.getUint32(4, true)]); return point.map(floatBits); },
            move: (sid, x, y, force) => carrier.highlight(sid, { x: bitsFloat(x), y: bitsFloat(y) }, force),
        });
        this.bindUserSelection({ ...selectionHooks, highlight: (mode) => highlight.highlight(mode) });
        this.#input = { carrier, highlight };
    }
    // 실제 선택의 세 인자를 원본처럼 유지하고 다음 화면 갱신을 요청한다.
    select(sid) { this.#live(); if (!this.#input) throw new Error('실제 선택 장치 미설치'); this.#selection.select(sid, 0, 1); this.#changed = true; }
    // 현재 마우스와 선택 전역을 사용하는 강조 모드다.
    highlight(mode) { this.#live(); if (!this.#input) throw new Error('실제 선택/강조 장치 미설치'); this.#input.highlight.highlight(mode); }
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
