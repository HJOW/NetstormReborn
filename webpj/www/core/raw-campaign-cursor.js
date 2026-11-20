// 캠페인 커서의 실제 수명·위치·미리보기·반환·로컬 배치를 같은 World에 조립한다(cpppj client/RawCampaignCursor.cpp/.h, 225553df).
// 사용: new RawCampaignCursor(data, game).setEffects(effects); take/cancel/update/drop/handleInput으로 같은 현재 상태를 조작한다.
// data는 cpppj CampaignCursorData의 소유자 참조다. playerKnown은 현재 {value} 상자, windType/rangeProcessType은 상자 또는 함수이며 cameraView는 현재 pixels 두 칸을 빌린다.
// game과 effects의 함수 묶음은 값 사본이다. UI/소리/에너지/전체 MayPlace/Builder/마나·사정거리는 선택된 순간 실제 소비자를 요구한다.
// 웹 제외인 편집 객체 TakeObject는 노출하지 않는다. Kernel 정리 뒤 상위 Session이 참조를 반납한다. (2026-11-16 WB04-E569)
import { ClientCursorGlobals } from './client-cursor-globals.js';
import { RawCursorPlacement, bindCursorPlacementAssets, bindCursorPlacement } from './raw-cursor-placement.js';
import { RawCursorPreviews, bindCursorPreviewPlacement, bindCursorPreviewWorld, bindCursorPreviews } from './raw-cursor-previews.js';
import { RawCursorLifecycle, bindCursorLifecycleAssets } from './raw-cursor-lifecycle.js';
import { RawCursorPosition, makeCursorPositionHooks } from './raw-cursor-position.js';
import { CursorReturnPieceState, RawCursorReturnPiece, makeReturnPieceHooks } from './raw-cursor-return-piece.js';
import { ConstructionLocalState, RawConstructionLocal } from './raw-construction-local.js';
import { RawCursorDrop } from './raw-cursor-drop.js';
import { RawCursorActions } from './raw-cursor-actions.js';
import { RawCampaignCursorDisplay } from '../render/raw-campaign-cursor-display.js';
import { makeOwnerDispatch } from './raw-bridge-connect.js';
import { SquidForms } from './squid-forms.js';
import { RegularProcess } from './squid-process.js';
import { writeConstructionCommand } from './raw-construction-command.js';
import { InputCode } from './input-queue.js';

// 도달한 현재 장치가 비었으면 원문의 선택 위치에서 거부한다.
function need(callback, name) { if (typeof callback !== 'function') throw new Error(`캠페인 커서 실제 경계 미연결: ${name}`); return callback; }
// 교체된 effects의 다음 호출을 읽는다. 생성 시점의 빈 함수나 이전 장치를 캡처하지 않는다.
function current(read, name) { return (...args) => need(read(), name)(...args); }
// 원문의 값형 서비스와 내부 함수 묶음을 복사하되 현재 게임 저장소는 빌린다.
function copyServices(source) { return { ...source, lifecycle: { ...source?.lifecycle }, placement: { ...source?.placement }, drop: { ...source?.drop } }; }

// 고정된 게임 참조와 교체 가능한 장치를 구별하여 실제 하위 몸체를 한 번 소유한다.
export class RawCampaignCursor {
    // 같은 월드/자료/게임 함수 사본과 현재 교체 장치다.
    #data; #core; #game; #effects = copyServices({});
    // Client 장기 전역 또는 명시 독립 fallback, 같은 현재 두 상태와 snap DWORD다.
    #cursorState; #life; #placementState; #snap;
    // 반환 설정과 로컬 예측 상태는 커서 수명이며 원문 Prepare가 현재 입력을 공급한다.
    #returnState = new CursorReturnPieceState(); #localState = new ConstructionLocalState(); #pendingTimer = null;
    // 동일 풀의 실제 수명·위치·미리보기·반환·배치·입력·표시 몸체다.
    #placement; #previews; #lifecycle; #position; #returned; #local; #actions; #drop; #display;

    // 서로 다른 풀과 필수 게임 준비·Builder·소리를 첫 하위 생성 전에 거부한다.
    constructor(data, game) {
        this.#data = { ...data }; data = this.#data; this.#core = data.world.core(); this.#game = copyServices(game);
        const c = this.#core; const g = this.#game;
        if (data.forms?.pool !== c.pool || data.contained?.pool !== c.pool || data.reward?.pool !== c.pool || data.construction?.pool !== c.pool ||
            typeof g.prepare !== 'function' || typeof g.moveBuilder !== 'function' || typeof g.playDrop !== 'function') throw new TypeError('실제 캠페인 커서 단일 월드/게임 연결 오류');
        this.#cursorState = data.currentState ?? new ClientCursorGlobals(); this.#life = this.#cursorState.lifecycle(); this.#placementState = this.#cursorState.placement(); this.#snap = this.#cursorState.snapToGrid();
        // 건설 미리보기·비용 표시는 카메라 픽셀 원점(+28/+32)을 [0]/[1] 로 읽는다. CameraView.pixels 접근자는 `{x, y}` 값 사본이라 공유 포인터로 쓰면 원점이 항상 0 이 되어
        // 미리보기가 카메라 위치만큼 어긋나 그려졌다. 카메라 바이트의 같은 8바이트를 가리키는 살아 있는 Int32Array 를 쓴다(client-cursor-state-cleanup.js 와 같은 방식). (2026-11-21 WB04-E1135)
        // 시험 대역처럼 bytes 가 없는 보기는 pixels 가 이미 Int32Array 인 경우만 그대로 쓴다.
        if (data.cameraView) this.#placementState.sharedScreenOrigin = data.cameraView.bytes instanceof Uint8Array ? new Int32Array(data.cameraView.bytes.buffer, data.cameraView.bytes.byteOffset + 28, 2) : data.cameraView.pixels;
        const placementHooks = { ...g.placement };
        bindCursorPlacementAssets(placementHooks, data.assets, c.types, data.geometry);
        placementHooks.gumpAt = current(() => this.#effects.placement.gumpAt, 'Gump 위치 조회');
        placementHooks.accept = current(() => this.#effects.placement.accept, 'Gump 수신 판정');
        placementHooks.parent = current(() => this.#effects.placement.parent, 'Gump 부모 조회');
        placementHooks.invalidate = current(() => this.#effects.placement.invalidate, '화면 dirty');
        this.#placement = new RawCursorPlacement(c.pool, c.types, c.hash.view(0), data.drop, this.#life, this.#placementState, placementHooks);
        const previewHooks = {};
        bindCursorPreviewPlacement(previewHooks, c.pool, this.#placement, placementHooks);
        bindCursorPreviewWorld(previewHooks, c.pool, c.factory, data.reward, data.world.bridgeConnections, c.frames, c.bridgeState, (sid) => c.building.notifySurface(sid));
        previewHooks.setOwner = makeOwnerDispatch(c.pool, c.bridgeState, (sid, owner) => c.owner.set(sid, owner));
        this.#previews = new RawCursorPreviews(c.pool, c.types, data.drop, this.#life, c.bridgeState, previewHooks);
        const lifecycleHooks = { ...g.lifecycle }; const dropHooks = {};
        lifecycleHooks.highlight = dropHooks.highlight = current(() => this.#effects.lifecycle.highlight, '강조 해제');
        lifecycleHooks.selectCursor = dropHooks.selectCursor = current(() => this.#effects.lifecycle.selectCursor, '하드웨어 커서 선택');
        lifecycleHooks.closePopups = current(() => this.#effects.lifecycle.closePopups, '팝업 닫기');
        lifecycleHooks.translate = current(() => this.#effects.lifecycle.translate, '현재 번역');
        lifecycleHooks.measure = current(() => this.#effects.lifecycle.measure, '현재 글자 측정');
        lifecycleHooks.mouse = current(() => this.#effects.lifecycle.mouse, '현재 마우스');
        bindCursorLifecycleAssets(lifecycleHooks, data.assets, c.types); bindCursorPlacement(lifecycleHooks, dropHooks, this.#placement); bindCursorPreviews(lifecycleHooks, this.#previews);
        lifecycleHooks.updatePosition = (x, y) => this.#position.update(x, y);
        this.#lifecycle = new RawCursorLifecycle(c.pool, c.types, data.drop, this.#life, lifecycleHooks);
        this.#position = new RawCursorPosition(data.drop, this.#placementState, this.#snap, data.camera, makeCursorPositionHooks(this.#lifecycle, this.#previews, dropHooks.selectCursor));
        const returnHooks = {
            // JS 예약 토큰은 생성/부착 사이에만 살며 native 힙 실패를 가짜 관측으로 보고하지 않는다.
            allocateTimer: (bytes) => { if (bytes !== 0x28) throw new Error('조각 반환 원본 타이머 할당 크기 오류'); this.#pendingTimer = {}; return true; },
            // 원문의 재구성된 Regular를 같은 실제 Host에 부착하고 현재 시각으로 재예약한다.
            createTimer: (parent, event, duration) => { if (!this.#pendingTimer) throw new Error('조각 반환 실제 타이머 예약 누락'); this.#pendingTimer = null; const process = c.processes.attachRegular(new RegularProcess(event, duration), parent); if (!process) throw new Error('조각 반환 실제 Regular 부착 실패'); process.resetTask(event, c.processState.now); },
            productTimers: g.productTimers, unitRate: g.unitRate, fortMode: () => c.ownerMode.fort ? 1 : 0,
        };
        this.#returned = new RawCursorReturnPiece(data.contained, this.#returnState, data.random, returnHooks);
        let localHooks = makeReturnPieceHooks(c.pool, this.#returned, {
            previewSource: (sid) => new SquidForms(c.pool).parent(sid), destroyPreview: (sid, flags) => this.#destroy(sid, flags), moveBuilder: g.moveBuilder,
            // 원문3e 생성자 헤더는 매 송신 때 같은 장기 GameAux를 읽는다.
            send: (destination, message) => { const packet = new Uint8Array(25); const aux = data.world.storage.gameAux; writeConstructionCommand(message, packet, { message: aux.constructionRequestHeader[0], protocol: aux.constructionRequestHeader[1], length: aux.constructionRequestLength }); current(() => this.#effects.send, '로컬 건설 송신')(destination, packet); },
        });
        this.#localState.decoderDiagnostics = data.geometry.decoderDiagnostics;
        localHooks = data.construction.bindLocal(localHooks);
        this.#local = new RawConstructionLocal(c.pool, c.types, data.frames, c.spots, this.#localState, localHooks);
        dropHooks.playDrop = g.playDrop; dropHooks.previewSource = (sid) => new SquidForms(c.pool).parent(sid);
        dropHooks.destroy = (sid, flags) => this.#destroy(sid, flags); dropHooks.pop = (sid, x, y, flags) => data.world.pop(sid, x, y, flags);
        dropHooks.record = current(() => this.#effects.drop.record, '선택 기록기');
        dropHooks.placeLocal = (request) => { this.#returnState.type = data.drop.type; this.#local.place(request); };
        dropHooks.addLocalSp = g.lifecycle.addLocalSp;
        dropHooks.showSp = current(() => this.#effects.drop.showSp, '비용 표시'); dropHooks.battleRefresh = current(() => this.#effects.drop.battleRefresh, '전투 표시 갱신');
        dropHooks.invalidGui = () => this.#lifecycle.cancel();
        dropHooks.guiFlags = current(() => this.#effects.drop.guiFlags, 'Gump 허용 DWORD'); dropHooks.guiParent = current(() => this.#effects.drop.guiParent, 'Gump 부착 부모');
        dropHooks.detach = (sid, flags) => need(g.lifecycle.detach, '실제 Form 분리')(sid, flags);
        dropHooks.attach = (sid, parent, flags) => { if (!parent || parent >= c.pool.capacity || parent > 65535) throw new RangeError('커서 Gump 부모 SID 범위'); data.forms.attach(sid, parent, flags); };
        dropHooks.guiReceive = current(() => this.#effects.drop.guiReceive, 'Gump 조각 수신');
        dropHooks.returnPiece = (flags, source, quality) => { this.#returnState.type = data.drop.type; this.#returned.returnPiece(flags, source, quality); };
        dropHooks.activatePreview = (force) => this.#lifecycle.activate(force); dropHooks.refresh = current(() => this.#effects.drop.refresh, '커서 변경 갱신');
        // 현재 프로세스의 가상+1c를 사용하며 effects.drop.closeGump로 바꾸지 않는다.
        dropHooks.closeGump = current(() => g.lifecycle.processEvent, '현재 커서 프로세스 통지');
        this.#drop = new RawCursorDrop(c.pool, c.types, data.drop, dropHooks);
        this.#actions = new RawCursorActions(c.types, data.drop, this.#life, data.windType, data.rangeProcessType, {
            activate: (force) => this.#lifecycle.activate(force), createPreviews: () => this.#previews.create(), selectCursor: current(() => this.#effects.lifecycle.selectCursor, '하드웨어 커서 선택'),
            sound: g.lifecycle.sound, removeRange: g.removeRange, createRange: g.lifecycle.createRange,
        });
        this.#display = new RawCampaignCursorDisplay(c, { pool: c.pool, drop: data.drop, lifecycle: this.#life, placement: this.#placementState, surfaces: c.bridgeState, grid: data.grid }, data.assets, data.sp, data.playerKnown);
    }

    // 가상 삭제 거부를 임시 성공으로 바꾸지 않는다.
    #destroy(sid, flags) { if (!this.#data.world.destroy(sid, flags)) throw new Error('실제 커서 가상 삭제 거부'); }
    // 공개 명령의 현재 게임 전역을 준비하고 값형 지역 입력만 갱신한다.
    #prepare() { const d = this.#data; this.#game.prepare(); this.#localState.localPlayer = d.drop.localPlayer; this.#localState.patternTypes = Array.from(d.geometry.patternTypes); this.#returnState.type = d.drop.type; this.#returnState.bridgeType = this.#core.bridgeState.bridgeType; this.#returnState.fort = this.#core.ownerMode.fort; }
    // 이미 만든 하위 실행기는 다음 호출부터 새 함수 사본을 조회한다.
    setEffects(effects) { this.#effects = copyServices(effects); }
    // 파일 팔레트와 같은 현재 경고 흰색/가격 노랑을 Take에 공급한다.
    setPalette(palette, logical = [], maps = null) { this.#display.setPalette(palette, logical, maps); const colors = this.#display.textColors(); this.#life.warningFont = colors[0]; this.#life.costFont = colors[1]; }
    // 공유 카메라 원점이 있으면 덮어쓰지 않고 전체 viewport만 갱신한다.
    setView(view) { if (!this.#placementState.sharedScreenOrigin) { this.#placementState.screenOrigin.x = view.cameraX; this.#placementState.screenOrigin.y = view.cameraY; } this.#display.setView(view); }
    // 메뉴 Form과 전체 DWORD flags를 실제 취득 몸체에 전달한다.
    take(form, flags) { this.#prepare(); this.#lifecycle.take(form, flags); }
    // State 정리는 외곽 Prepare 효과를 추가하지 않고 Native leaf만 실행한다.
    cancelForState() { this.#lifecycle.cancel(); }
    // 일반 취소는 현재 게임 준비를 먼저 실행한다.
    cancel() { this.#prepare(); this.#lifecycle.cancel(); }
    // 같은 카메라 float비트와 현재 snap을 사용한다.
    update(x, y) { this.#prepare(); this.#position.update(x, y); }
    // 정지 마우스의 UI 교체도 현재 Gump/dirty를 다시 수집한다.
    gumpsChanged() { const d = this.#data.drop; if (d.lockDepth || !d.preview) return; this.#prepare(); this.#placement.prepare(); this.#placement.query(); this.#placement.prepare(); current(() => this.#effects.lifecycle.selectCursor, '하드웨어 커서 선택')(d.legalGump ? 3 : 2); }
    // 실제 중첩 잠금과 처음 진입의 커서 선택이다.
    lock(skipCursor) { this.#prepare(); this.#actions.lock(skipCursor); }
    // 깊이0 전이에서만 같은 미리보기 목록을 만든다.
    unlock() { this.#prepare(); this.#actions.unlock(); }
    // signed 방향과 실제 Range 수명을 같은 몸체에서 처리한다.
    rotate(step) { this.#prepare(); this.#actions.rotate(step); }
    // 활성 커서의 원문 입력 필터다. outside는 이 위치에서 별도 거부하지 않는다.
    handleInput(event, rotateMode) {
        if (this.#data.drop.lockDepth || (event.code & InputCode.release)) return false;
        switch (event.code & 0x03ffffff) {
        case InputCode.leftButton: this.drop(); return true;
        case InputCode.rightButton: this.rotate(rotateMode ? -2 : 2); return true;
        case 0x250000: this.rotate(-2); return true;
        case 0x270000: this.rotate(2); return true;
        case 0x1b: this.cancel(); current(() => this.#effects.lifecycle.closePopups, '팝업 닫기')(); return true;
        default: return false;
        }
    }
    // 비용·송신·임시 객체 정리는 같은 실제 Drop에 위임한다.
    drop() { this.#prepare(); this.#drop.drop(); }
    // AI 직접 예측 배치는 취득이나 상위 비용 차감을 추가하지 않는다.
    placeLocal(request) { this.#prepare(); this.#local.place({ ...request }); }
    // 사제 메뉴의 취득만 같은 단일 커서에 제자리 연결한다.
    bind(hooks) { hooks.takePreview = (form, flags) => this.take(form, flags); }
    // 같은 표시기의 장기 주소를 빌린다.
    get display() { return this.#display; }
    // 현재30DWORD 목록과 취득/문자 측정 전역을 빌린다.
    get lifecycle() { return this.#life; }
    // 같은 허용 판정·Gump·dirty/원점 상태를 빌린다.
    get placement() { return this.#placementState; }
    // 실제 생성/정렬 부착에 사용하는 내용물 몸체를 빌린다.
    get contents() { return this.#data.contained; }
    // 현재 반환 TYPE·생산 유지/소멸 설정을 빌린다.
    get returnSettings() { return this.#returnState; }
    // 다음 위치 갱신이 읽을 같은 snap DWORD 상자를 빌린다.
    get snapToGrid() { return this.#snap; }
}
