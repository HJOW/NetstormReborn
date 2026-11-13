// 실제 건물/자산의 kind0 수집·+78/+a4·일반 SpriteDraw와200픽셀 선택을 같은 월드에 연결한다(cpppj client/RawCampaignFactoryDisplay.cpp/.h).
// 사용: new RawCampaignFactoryDisplay(RawSurfaceWorld.core(), assets, templeDraw?, patchDiagnosticBox?). 같은 자산/풀/Kernel보다 먼저 해제한다.
// 기준선1f00a25a 뒤 a0342fae의 메타 선택/추가Draw 장치도 반영한다. 실제 공중3종의 새 생성/전용 검사는 수행하지 않는다. (2026-11-14 WB04-E409)
import { OBJECT_HIT_VTABLES } from './object-hit-vtables.js';
import { ObjectCollection } from './object-collection.js';
import { SpriteDraw, SpriteDrawState, SpriteShapeMode, SpritePixelTarget, makeSpriteAssetViews, makeSpriteAssetShapes, makeSpriteSurfaceLookup } from './sprite-draw.js';
import { DRAW_SUBMISSION_CAPACITY } from './draw-submission.js';
import { drawShape } from './vfx-draw.js';
import { cursorDisplayLogicalPalette } from './raw-campaign-cursor-display.js';
import { RawFactoryDraw } from '../core/raw-factory-draw.js';
import { RawObjectHit } from '../core/raw-object-hit.js';
import { suppressSpellHelpFrame } from '../core/spell-frame-visibility.js';
import { PATCH_DAIS_EXTRA_TYPE, isPatchDais, drawDaisAdditional } from '../core/raw-dais-draw.js';
import { drawManaAdditional } from '../core/raw-mana-draw.js';
import { drawScaledMetadisplay, drawAbstractAdditional } from '../core/raw-auxiliary-draw.js';
import { isPatchTemple, patchTempleMainFrame } from '../core/raw-temple-draw.js';
import { isThunderCannonCapture, thunderCannonCaptureMainFrame, drawThunderCannonCapture } from '../core/raw-thunder-cannon-capture.js';
import { isGeyserCapture, drawGeyserCapture } from '../core/raw-geyser-capture.js';
import { isSunAviaryCapture, sunAviaryCaptureMainFrame, drawSunAviaryCapture } from '../core/raw-sun-aviary-capture.js';
import { isBurningArcherCapture, drawBurningArcherCapture } from '../core/raw-burning-lifetime-draw.js';
import { isPlainAssetDraw } from '../core/raw-plain-asset-draw.js';
import { findAttachedProcess } from '../core/raw-sun-aviary-process.js';
import { FlyerProcess } from '../core/raw-flyer-process.js';
import { BombEffectTimerProcess } from '../core/raw-bomb-effect-timer.js';
import { SquidProcess } from '../core/squid-process.js';
import { RawContainedFinder, ContainedFinderState } from '../core/raw-contained-finder.js';
import { buildStandardOwnerColorMaps, buildSpecialOwnerColorMaps } from '../assets/owner-colors.js';
import { buildSpriteColorMaps } from '../core/sprite-color-maps.js';
import { buildPaletteColorTable } from '../core/palette-colors.js';
import { ClientColorSortInput, colorSortLogicalInput } from '../core/client-color-sort-input.js';

// 보호PE의 실제 분배 키이며 호스트 포인터로 실행하지 않는다.
const VTABLES = new Map(OBJECT_HIT_VTABLES.map((entry) => [entry.table, entry]));
// 일반12종과 전용5종의 최초 소유자 표 대상이다.
const STANDARD = Object.freeze([82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126]);
const SPECIAL = Object.freeze([111, 141, 137, 94, 97]);
// 비정렬 DWORD/WORD의 현재 비트만 읽는다.
const read = (raw, at, width = 4) => { const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); return width === 2 ? view.getUint16(at, true) : view.getUint32(at, true); };
// 확인된83VT만 실제+78/+a4 분배에 허용한다.
function hitVtable(raw) { const entry = VTABLES.get(read(raw, 0)); if (!entry) throw new Error('픽셀 선택의 실제 가상 표 미복원'); return entry; }
// 그림 요청은 TYPE/프레임/상대점과 원문 색 모드를 값으로 전달한다.
const request = (type, frame, point, mode) => ({ type: type >>> 0, frame: frame >>> 0, position: { x: point.x, y: point.y }, mode });

// 현재 월드의 같은 raw/프로세스/표시 저장소를 빌리는 건물 출력기다.
export class RawCampaignFactoryDisplay {
    // 현재 core/자산과 두 실제 표시 몸체다.
    #core; #assets; #tables; #factory; #collection; #views;
    // 표시 상태/현재Kernel 순서의 상태 슬롯·기본 프레임 표다.
    #state = new SpriteDrawState(); #statusSlots = []; #templeBaseFrames;
    // 팔레트 전 Hit도 원문처럼0으로 초기화된 세 표/강조 표를 읽는다.
    #colors = { bright: new Uint8Array(256), dead: new Uint8Array(256), shadow: new Uint8Array(256) }; #intense = new Uint8Array(256);
    // 독립9색 표와 실제 Client의 공통 현재 표다.
    #ownerColors = new Map(); #sharedOwner = null;
    // 실제 Session 신전 Draw와 Client 메타 선택 장치, 파일 팔레트 준비다.
    #templeDraw; #metaPixels = null; #ready = false;

    // 참조만 연결하며 실제 객체/게임 진행을 생성하지 않는다.
    constructor(core, assets, templeDraw = null, patchDiagnostic = null) {
        this.#core = { ...core }; this.#assets = assets; this.#templeDraw = templeDraw;
        this.#tables = core.frames.map((type) => type.frames ?? type); this.#factory = new RawFactoryDraw(core.pool, this.#tables);
        this.#collection = new ObjectCollection(core.hash, core.pool, core.types, core.shapes); this.#views = makeSpriteAssetViews(assets); this.#templeBaseFrames = new Int32Array(core.types.length).fill(-1);
        // 실제 타입 정의의 기본 프레임은 내장70칸을 건너뛰고 같은 번호에 쓴다.
        for (const type of assets.types()) this.#templeBaseFrames[type.block + 70] = type.definition.baseFrame;
        Object.assign(this.#state, { filteredFlags: 2421574144, wideOwnerType: 164, unfilteredType: 84, shadowAliasType: 158, shadowAliasPinned: true,
            markerEnabled: true, markerExcludeType: 157, markerType: 96, statusType: 118, abstractStatusType: 117, timerProcessType: 31, statusProcessType: 57,
            costType: 122, timerTypes: [138, 144, 73], selectionColors: [84, 84, 249, 184, 140, 150, 112, 156, 222], ownerMaps: new Array(core.types.length).fill(0), sharedPatchDiagnostic: patchDiagnostic });
        // 일반9표는 즉시 준비하며 전용5표의 명도는 파일 팔레트에서 만든다.
        for (const type of STANDARD) { this.#state.ownerMaps[type] = 1; this.#ownerColors.set(type, buildStandardOwnerColorMaps(type, 82)); }
        // 전용 표의 원본 존재 여부는 팔레트 전부터 유지한다.
        for (const type of SPECIAL) this.#state.ownerMaps[type] = 2;
    }
    // 가상/출력 뒤 현재 raw를 다시 읽을 수 있는 같은50BYTE 보기다.
    #raw(sid) { return this.#core.pool.physical(sid); }
    // 실제 자산 TYPE만 현재 정의와 연결한다.
    #definition(type) { if (type < 70 || type - 70 >= this.#assets.types().length) throw new RangeError('표시 자산 TYPE 범위'); return this.#assets.types()[type - 70].definition; }
    // 플레어/제네레이터/마나볼트/폭탄은 원문 TYPE/VT 조합으로만 구별한다.
    #flare(sid) { const raw = this.#raw(sid); return raw[10] === 106 && read(raw, 0) === 0x5096a0; }
    // 세 제네레이터 생성자가 설치하는 같은 실제 VT다.
    #generator(sid) { return read(this.#raw(sid), 0) === 0x501ea0; }
    // 마나볼트105의 빈 본체/전용 추가 그림 VT다.
    #manaBolt(sid) { const raw = this.#raw(sid); return raw[10] === 105 && read(raw, 0) === 0x509810; }
    // 전용1.6배 표를 쓰는 VT만 선택하며 임의 TYPE를 대입하지 않는다.
    #intenseBomb(sid) { return read(this.#raw(sid), 0) === 0x509b98; }
    // 여섯 주문의 공통+a0/+a4 조합이다.
    #bomb(sid) {
        const raw = this.#raw(sid); const table = read(raw, 0);
        return (raw[10] === 117 && table === 0x502510) || (raw[10] === 118 && table === 0x502ff8) || (raw[10] === 168 && table === 0x502370)
            || (raw[10] === 171 && table === 0x5025d8) || (raw[10] === 172 && table === 0x502e68) || (raw[10] === 173 && table === 0x502f30);
    }
    // 원래 별도SID의 두 번째 물리 프레임 묶음이며 위치/부모를 바꾸지 않는다.
    #flyerShadow(sid) { const raw = this.#raw(sid); const type = raw[10]; return (this.#core.types[type].flags1 & 0x400000) !== 0 && read(raw, 36) >= this.#core.shapes[type].frameCount; }
    // 검증된 원본 파생과 일반 자산만 기존 월드 그림에서 맡아 온다.
    handles(sid) {
        return this.#factory.handles(sid) || this.#bomb(sid) || this.#flare(sid) || this.#generator(sid) || isPatchTemple(this.#raw(sid)) || isPatchDais(this.#raw(sid))
            || isThunderCannonCapture(this.#raw(sid)) || isGeyserCapture(this.#raw(sid)) || this.#manaBolt(sid) || this.#intenseBomb(sid)
            || isBurningArcherCapture(this.#raw(sid)) || isSunAviaryCapture(this.#raw(sid)) || isPlainAssetDraw(this.#raw(sid)) || this.#flyerShadow(sid);
    }
    // 실제 현재 소유자 색 번호와 같은 공통 표 또는 독립9표를 읽는다.
    #color(mode, type, owner) {
        if (mode === SpriteShapeMode.bright160) return this.#intense;
        if (mode === SpriteShapeMode.abstract) return this.#colors.bright;
        if (mode === SpriteShapeMode.dead) return this.#colors.dead;
        if (mode === SpriteShapeMode.shadow) return this.#colors.shadow;
        if (mode !== SpriteShapeMode.owner || owner >= this.#core.bridgeState.ownerColors.length) throw new Error('실제 자산 소유자 색표 분기 오류');
        const index = this.#core.bridgeState.ownerColors[owner]; if (index < 0 || index > 8) throw new RangeError('실제 자산 현재 소유자 색 번호');
        if (this.#sharedOwner) return this.#sharedOwner.color(type, index);
        const maps = this.#ownerColors.get(type); if (!maps) throw new Error('실제 자산 현재 소유자 색표 미연결'); return maps[index];
    }
    // 실제497280의 현재 추상/로컬 소유자 관문을 각 그림마다 적용한다.
    #currentShape(sid, shape, type, frame, x, y) {
        const raw = this.#raw(sid);
        if (!(raw[40] & 0x10)) shape(request(type, frame, { x, y }, SpriteShapeMode.plain));
        else if (raw[34] === (this.#state.localPlayer >>> 0)) shape(request(type, frame, { x, y }, SpriteShapeMode.abstract));
    }
    // 일반 화면과 픽셀 선택이 같은 실제+a4를 사용한다. 도달한 미연결 장치는 명시 실패다.
    #additional(sid, point, shape, bridge = null, meta = null) {
        const raw = this.#raw(sid); const dispatch = hitVtable(raw); const address = dispatch.additional;
        if (address === 0x41c100) return;
        if (address === 0x478d10) { if (typeof meta !== 'function') throw new Error('메타 섬 실제 추가Draw 장치 미연결'); meta(sid, point.x, point.y); return; }
        if (address === 0x45bcd0) { if (raw[11] & 2) return; const frame = read(raw, 36); shape(request(raw[10], frame, point, SpriteShapeMode.bright160)); return; }
        if (address === 0x479340 || address === 0x485f90) {
            const hooks = { shape: (type) => type, draw: (value) => { const item = request(value.shape, value.frame, { x: value.x, y: value.y }, value.abstract ? SpriteShapeMode.abstract : SpriteShapeMode.plain); item.scaled = value.scale !== 0; item.scaleX = value.scale; item.scaleY = value.scale; shape(item); } };
            if (address === 0x479340) drawScaledMetadisplay(raw, point.x, point.y, hooks); else drawAbstractAdditional(raw, point.x, point.y, hooks); return;
        }
        if (address === 0x473710) { drawManaAdditional(raw, point.x, point.y, { shape: (type) => type, draw: (value) => shape(request(value.shape, value.frame, { x: value.x, y: value.y }, value.shadow ? SpriteShapeMode.shadow : SpriteShapeMode.plain)) }); return; }
        if (this.#manaBolt(sid)) { const frame = read(raw, 36); shape(request(105, frame, { x: (point.x + 10) | 0, y: (point.y + 10) | 0 }, SpriteShapeMode.shadow)); shape(request(105, frame, point, SpriteShapeMode.plain)); return; }
        if (address === 0x4223c0) { if (typeof bridge !== 'function') throw new Error('선택용 다리의 실제 추가Draw 미연결'); bridge(sid, point.x, point.y); return; }
        if (address === 0x41e3d0) {
            const frame = read(raw, 36); if (suppressSpellHelpFrame(this.#core.types[raw[10]], this.#definition(raw[10]), frame)) return;
            this.#currentShape(sid, shape, raw[10], frame, point.x, point.y); return;
        }
        if (isPlainAssetDraw(raw)) return;
        const current = (type, frame, x, y) => this.#currentShape(sid, shape, type, frame, x, y);
        if (isSunAviaryCapture(raw) || isBurningArcherCapture(raw)) {
            if (isSunAviaryCapture(raw)) drawSunAviaryCapture(raw, current, point.x, point.y);
            else { const header = this.#core.shapes[raw[10]].frames[read(raw, 36)]; if (!header) throw new RangeError('아처 현재 프레임 헤더'); drawBurningArcherCapture(raw, this.#definition(raw[10]).baseFrame, header.muzzleX, header.muzzleY, current, point.x, point.y); }
            return;
        }
        if (isPatchDais(this.#raw(sid))) { drawDaisAdditional(this.#raw(sid), this.#core.shapes[PATCH_DAIS_EXTRA_TYPE].frameCount, (type, frame) => { if (frame < 0) throw new RangeError('다이스 추가 그림 음수 프레임'); current(type, frame, point.x, point.y); }); return; }
        if (this.#flare(sid) || this.#generator(sid)) return;
        if (isGeyserCapture(this.#raw(sid))) { drawGeyserCapture(this.#raw(sid), current, point.x, point.y); return; }
        if (isThunderCannonCapture(this.#raw(sid))) { drawThunderCannonCapture(this.#raw(sid), current, point.x, point.y); return; }
        if (isPatchTemple(this.#raw(sid))) {
            if (typeof this.#templeDraw !== 'function') throw new Error('실제 템플 전용Draw 미연결'); const type = this.#raw(sid)[10];
            this.#templeDraw(sid, (frame) => { if (frame < 0) throw new RangeError('템플 추가 그림 음수 프레임'); current(type, frame, point.x, point.y); }); return;
        }
        if (!this.#factory.handles(sid)) throw new Error(`픽셀 선택의 파생 추가Draw 미복원 TYPE=${raw[10]}`);
        this.#factory.additional(sid, point.x, point.y, (x, y, type, frame) => { if (frame < 0) throw new RangeError('Factory 추가 그림 음수 프레임'); shape(request(type, frame, { x, y }, SpriteShapeMode.plain)); });
    }
    // 원문 일반 Draw와 픽셀 선택 모두 같은 실제+78을 조회한다.
    #hitFrame(sid) {
        const raw = this.#raw(sid);
        switch (hitVtable(raw).frame) {
            case 0x4acfe0: return this.#definition(raw[10]).baseFrame;
            case 0x4222f0: return read(raw, 36) | 0;
            case 0x454cd0: return this.#factory.mainFrame(sid);
            case 0x45b530: return sunAviaryCaptureMainFrame(raw);
            case 0x46ae10: return thunderCannonCaptureMainFrame(raw, this.#tables[raw[10]]);
            default: throw new Error('픽셀 선택의 실제+78 미복원');
        }
    }
    // 고정200픽셀 선택에서는 소유자 색/배율을 기본 그림에 적용하지 않는다.
    hit(sid, x, y, camera, localPlayer, bridgeDraw = null) {
        this.#state.localPlayer = localPlayer; const hooks = { frame: (object) => this.#hitFrame(object),
            plain: (type, frame, px, py, pixels) => { if (frame < 0) throw new RangeError('픽셀 선택의 plain 음수 프레임'); drawShape(pixels, 200, 200, this.#assets.shapes, type - 70, frame, px, py, { left: 0, top: 0, right: 199, bottom: 199 }); },
            additional: (object, px, py, pixels) => {
                const target = new SpritePixelTarget(pixels, 200, 200, { left: 0, top: 0, right: 200, bottom: 200 }, makeSpriteAssetShapes(this.#assets, this.#state), (mode, type, owner) => this.#color(mode, type, owner));
                const meta = (current, mx, my) => { if (typeof this.#metaPixels !== 'function') throw new Error('메타 선택 캡처의 실제 픽셀 장치 미연결'); this.#metaPixels(current, { x: mx, y: my }, pixels, 200, 200, { left: 0, top: 0, right: 200, bottom: 200 }); };
                this.#additional(object, { x: px, y: py }, target.hooks().shape, bridgeDraw, meta);
            } };
        return new RawObjectHit(this.#core.pool, this.#core.types, this.#core.shapes, hooks).hit(sid, x, y, camera);
    }
    // 실제 가상 Draw용 현재 상태를 선택한 SpriteDraw에 전달한다.
    #draw(entry, pixels, pitch, height, clip, native = false, bridge = null, meta = null) {
        const sid = this.#collection.object(new DataView(entry.buffer, entry.byteOffset, entry.byteLength).getUint32(8, true)); if (!native && !this.handles(sid)) return;
        if (!this.#ready) throw new Error('Factory 파일 팔레트 미설치'); const core = this.#core; const state = this.#state;
        state.now = core.processState.now; state.ownerPalettes = core.bridgeState.ownerColors.slice(); state.markerVisible = core.ownerMode.battle;
        const target = new SpritePixelTarget(pixels, pitch, height, clip, makeSpriteAssetShapes(this.#assets, state), (mode, type, owner) => this.#color(mode, type, owner));
        const rules = {
            typeHitPoints: (number) => { if (number >= core.types.length) throw new RangeError('표시 HP의 선택 TYPE 범위'); return core.factory.readSelectedHitPoints(core.types[number], core.types); },
            virtualFrame: (object) => {
                if (native) return this.#hitFrame(object);
                if (this.#bomb(object) || this.#manaBolt(object)) return -1;
                if (isPatchTemple(this.#raw(object))) return patchTempleMainFrame(this.#raw(object), this.#templeBaseFrames);
                if (isThunderCannonCapture(this.#raw(object))) return thunderCannonCaptureMainFrame(this.#raw(object), this.#tables[this.#raw(object)[10]]);
                if (isSunAviaryCapture(this.#raw(object))) return sunAviaryCaptureMainFrame(this.#raw(object));
                if (isBurningArcherCapture(this.#raw(object)) || isPlainAssetDraw(this.#raw(object))) return this.#definition(this.#raw(object)[10]).baseFrame;
                return this.#flare(object) || this.#generator(object) || isGeyserCapture(this.#raw(object)) || this.#intenseBomb(object) || isPatchDais(this.#raw(object)) ? this.#definition(this.#raw(object)[10]).baseFrame : this.#factory.mainFrame(object);
            },
            surface: makeSpriteSurfaceLookup(core.hash), constructionActive: (object) => core.processes.findEvent(object, 0x25d) !== null,
            timers: (object, type) => {
                const form = findAttachedProcess(core.pool, object, type); if (!form) return null; const process = core.processes.fromForm(form);
                if (!(process instanceof FlyerProcess)) throw new Error('비행 선택 타이머의 현재 프로세스 미연결'); const bytes = process.fields.bytes; const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
                return [view.getFloat64(72, true), view.getFloat64(104, true), view.getFloat64(40, true)];
            },
            resource: (object) => {
                const cast = core.processes.findEvent(object, 0x493); if (!cast) return null; const current = new ContainedFinderState(); current.boss = core.processState.boss; const finder = new RawContainedFinder(core.pool, current); let content = 0;
                // 원문처럼 첫 mask1의 실제 내용물을 고르며 추가 상태 필터를 넣지 않는다.
                for (let candidate = finder.begin(object); candidate; candidate = finder.next()) if (read(this.#raw(candidate), 20, 2) & 1) { content = candidate; break; }
                return { capacityType: read(this.#raw(content), 18, 2), remaining: cast.count | 0 };
            },
            statusProcesses: () => {
                this.#statusSlots.length = 0; this.#statusSlots.push({});
                // 원본의 첫0슬롯을 유지하고 실제 현재 PID 순서를 그대로 따른다.
                for (let pid = 1; pid <= core.kernel.capacity; pid += 1) {
                    const process = core.kernel.get(pid); if (!(process instanceof SquidProcess) || this.#raw(process.form)[10] !== state.statusProcessType) continue;
                    if (!(process instanceof BombEffectTimerProcess)) throw new Error('상태57의 실제 타이머 필드 미연결'); const fields = process.fields;
                    this.#statusSlots.push({ type: state.statusProcessType, flags: fields.flags, members: fields.members, endTime: fields.endTime });
                }
                return this.#statusSlots;
            },
        };
        const hooks = target.hooks(rules); const shape = hooks.shape; hooks.virtualDraw = (object, point) => this.#additional(object, point, shape, bridge, meta);
        new SpriteDraw(core.pool, core.types, this.#views, state, hooks).draw(entry);
    }
    // 현재 메타 장치만 설치하며 설치 자체는 그림/선택을 실행하지 않는다.
    bindMetaPixels(draw) { if (typeof draw !== 'function') throw new TypeError('현재 메타 선택 픽셀 장치 누락'); this.#metaPixels = draw; }
    // 통합kind0/지형 목록은 같은 현재 카메라/로컬소유자/메뉴값과 실제 추가Draw 장치를 사용한다.
    drawEntry(entry, pixels, pitch, height, clip, camera, localPlayer, hideBuildings, bridge, meta = null) {
        Object.assign(this.#state, { cameraX: camera[0], cameraY: camera[1], localPlayer, specialMode: hideBuildings }); this.#draw(entry, pixels, pitch, height, clip, true, bridge, meta);
    }
    // 같은 Client의 현재557940/배너 DWORD를 빌리고 독립 색표는 반납한다.
    bindOwnerColors(owner) {
        if (!owner?.ready) throw new TypeError('월드 공통 소유자 색표 최초 등록 누락'); this.#sharedOwner = owner; this.#ownerColors.clear(); this.#state.sharedOwnerMaps = owner.tokens;
        this.#state.sharedUseBanners = { get value() { return owner.enabled; } };
    }
    // 현재 파일 명명색과 고정 lookup 입력/실제 캐시9표의 용도를 분리한다.
    setPalette(palette, colorSortLogical = [], colorMaps = null) {
        const logical = cursorDisplayLogicalPalette(palette); const named = buildPaletteColorTable(logical); const fixed = () => colorSortLogicalInput(colorSortLogical, logical);
        this.#colors = colorMaps ? { bright: colorMaps[4].slice(), dead: colorMaps[6].slice(), shadow: colorMaps[7].slice() } : buildSpriteColorMaps(fixed()); this.#state.barBackground = named.basic[0];
        if (colorMaps) this.#intense = colorMaps[0].slice();
        else { const independent = new ClientColorSortInput(); independent.capture(fixed()); this.#intense = independent.rebuild(named.basic[1])[0].slice(); }
        if (!this.#sharedOwner) {
            // 실제 Client 공통 표가 없는 명시 독립 입력만 전용 명도 표를 다시 만든다.
            for (const type of SPECIAL) this.#ownerColors.set(type, buildSpecialOwnerColorMaps(fixed(), type));
        }
        this.#state.hitPointColors = [named.named[10].index, named.basic[5], named.basic[1]]; this.#ready = true;
    }
    // 실제 해시 수집 순서와 그림자 표식/객체 신원을 유지하며 외부 Renderer 정렬에 맡긴다.
    sprites(clip, cameraX, cameraY, localPlayer, hideBuildings = 0) {
        Object.assign(this.#state, { cameraX, cameraY, localPlayer, specialMode: hideBuildings });
        const view = { left: Math.fround((clip.left + cameraX) / 16), top: Math.fround((clip.top + cameraY) / 11), right: Math.fround((clip.right + cameraX) / 16), bottom: Math.fround((clip.bottom + cameraY) / 11) };
        const entries = new Uint8Array(DRAW_SUBMISSION_CAPACITY * 16); const count = this.#collection.collect(view, entries, DRAW_SUBMISSION_CAPACITY); const result = [];
        // 지원된 실제 VT만 옮기며 다른 타입을 휴리스틱 성공으로 그리지 않는다.
        for (let index = 0; index < count; index += 1) {
            const entry = entries.slice(index * 16, (index + 1) * 16); const data = new DataView(entry.buffer); const sid = this.#collection.object(data.getUint32(8, true)); if (!this.handles(sid)) continue;
            result.push({ database: null, clip: { ...clip }, order: { x: data.getFloat32(0, true), y: data.getFloat32(4, true), depth: data.getInt16(12, true) }, shadow: (entry[15] & 4) !== 0, sourceSid: sid,
                procedure: { bounds: { ...clip }, draw: (pixels, pitch, height, cut) => this.#draw(entry, pixels, pitch, height, cut) } });
        }
        return result;
    }
}
