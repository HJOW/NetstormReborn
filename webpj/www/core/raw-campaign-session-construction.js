// 생성자1652/ConnectConstruction2731..2774의 독립 typed 건설 결선이다(225553df, WB04-E603).
// 사용: connectFlare 뒤 bind. 같은 현재 World/최근5칸/방향표를 빌리고 건물·Graph·이동은 호출 때 실제 후설치 몸체를 선택한다.
import { RawCampaignConstructionBindings, bindConstructionRuntime, makePlayerGeyserBoundsState } from './raw-campaign-construction-bindings.js';
import { currentBuildReplayIsLabel, notifyCurrentBuildReplay } from './client-build-replay.js';
import { writeConstructionMoneyPacket, writeConstructionRejectPacket } from './raw-construction-failure-packets.js';
import { writeRemoteTellPacket } from './raw-remote-tell-packet.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RawPriestLookup } from './raw-priest-lookup.js';
import { RawBoardApproach } from './raw-board-approach.js';
import { RawCanonPlacementPipeline } from './raw-canon-placement-pipeline.js';
import { RawPriestPlacement } from './raw-priest-placement.js';
import { RawPriestPlacementCollision, makePriestPlacementCollisionHooks } from './raw-priest-placement-collision.js';
import { RawPriestPlacementGeometry, makePriestGeometryCollisionHooks, makePriestGeometryPreviewHooks } from './raw-priest-placement-geometry.js';
import { RawPriestPlacementPreview, makePriestPlacementPreviewHooks } from './raw-priest-placement-preview.js';
import { RawPriestPlacementShape, makePriestShapePlacementHooks } from './raw-priest-placement-shape.js';
import { RawPriestPlacementTerrain, createPriestPlacementTerrainState, makePriestTerrainCollisionHooks, makePriestTerrainGeometryHooks } from './raw-priest-placement-terrain.js';
import { decodeOriginalText } from '../assets/text-encoding.js';
import { formatWarningMessage } from '../ui/message-gump.js';

// 선택된 실제 후속 부모만 요구하며 앞선 부분 효과를 되돌리지 않는다.
function need(value, name) { if (!value) throw new Error(name); return value; }
// 원문 DWORD 비트를 float로 해석한다.
function float(bits) { const raw = new DataView(new ArrayBuffer(4)); raw.setUint32(0, bits, true); return raw.getFloat32(0, true); }
// 원문의 값형 effects/presentation만 복사하고 실제 함수가 빌리는 owner는 유지한다.
export function copyCampaignConstructionEffects(value) { return { ...value, presentation: { ...value?.presentation } }; }

// 같은 실제 건설/간헐천 본체와 현재 외부 효과의 수명 구간이다.
export class RawCampaignSessionConstruction {
    // 한 번 설치/부분 실패와 반납 뒤 closure의 접근을 구별한다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 기존 본체가 값으로 받는 외부/내부 효과 표와 현재 PlayerBounds 입력이다.
    effects = null; gameplay = null; boundsState = null;

    // 구성은 요청/패킷/게임 난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 현재 자산 준비 뒤 실제 기본 효과·후설치 소비자를 원문 순서로 연결한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.flareModules?.ready || this.#started || o.construction) throw new Error('Session Construction의 초기화 전/반복 결선');
        this.#started = true; o.assetModules.prepareConstruction();
        const effects = copyCampaignConstructionEffects(o.services.construction); const p = effects.presentation;
        if (!p.sendMoney) p.sendMoney = (owner, value) => this.sendChatMutable(owner | 0, writeConstructionMoneyPacket(value));
        if (!p.sendReject) p.sendReject = (owner, value) => this.sendChatMutable(owner | 0, writeConstructionRejectPacket(value));
        if (!p.format) p.format = formatWarningMessage;
        if (!p.tellOthers) p.tellOthers = (owner, text) => this.sendRemoteTell(owner, text, true);
        if (!p.tellPlayer) p.tellPlayer = (owner, text) => this.sendRemoteTell(owner, text, false);
        if (!p.tell) p.tell = (text) => { this.#live(); need(o.services.factoryTell, '비용부족의 실제 현재Tell 미연결')(decodeOriginalText(Uint8Array.from(text, (letter) => letter.charCodeAt(0)))); };
        // 미선택 외부 경계를 빈 성공으로 바꾸지 않고 선택된 위치에서 오류로 드러낸다.
        for (const name of ['configInt', 'wallSeconds', 'refresh', 'negativeQuantity', 'boundsAssertion']) if (!effects[name]) effects[name] = () => { this.#live(); throw new Error(`건설 실제 외부 장치 미연결: ${name}`); };
        effects.islandExists = (region) => { this.#live(); return Boolean(o.islands.exists(region)); };
        const replay = o.services.buildReplay; o.state.constructionState.confirm.sharedRecorderEnabled = replay ? () => replay.playing : null;
        const gameplay = {
            confirm: {
                recorderActive: () => { this.#live(); return currentBuildReplayIsLabel(o.services.buildReplay); },
                record: (sid) => { this.#live(); notifyCurrentBuildReplay(o.services.buildReplay, sid); },
                findBuilder: (owner, type, x, y) => { this.#live(); const value = need(o.anchor, '일반 확정의 실제 기준점 미설치').locate(owner, type, x, y) >>> 0; if (value >= c.pool.capacity) throw new RangeError('일반 확정 기준점 SID 범위 오류'); return value & 0xffff; },
            },
            setOwner: (sid, owner) => { this.#live(); o.priests.setOwner(sid, owner); },
            request: {
                mayPlace: (query) => this.mayPlace(query),
                displace: (x, y, type, owner) => { this.#live(); const body = need(o.buildingEffects, '요청의 실제 건설 효과 미설치'); o.assetModules.prepareBuilding(c.processState.now); return body.displace(x, y, type, owner); },
                cancel: (x, y, flags, owner) => { this.#live(); const body = need(o.buildingEffects, '요청 실패의 실제 섬 정리 미설치'); o.assetModules.prepareBuilding(c.processState.now); body.dropIsland(x, y, flags, owner); },
            },
            notice: {
                moveBuilder: (owner, target, low, high) => { this.#live(); need(o.buildingMover, '일반 통지의 실제 Builder 미설치').move(owner, target, low, high); },
                findPriest: (owner) => { this.#live(); return new RawPriestLookup(c.pool, o.priestState.priests).find(owner) & 0xffff; },
                containedType: (type) => { this.#live(); return o.priests.movementProcessType(type); },
                removeContained: (parent, type) => { this.#live(); new RawContainedLookup(c.pool, (sid, flags) => o.world.destroy(sid, flags)).find(parent, type, 0, true); },
                immobile: (sid) => { this.#live(); return o.priests.priestState.immobile(sid); },
                approach: (sid, x, y, flags) => {
                    this.#live(); need(o.anchor, '일반 통지 접근점의 실제 Graph 미설치'); const r = o.gunTarget.footprint(sid, false);
                    const approach = new RawBoardApproach((px, py) => o.anchor.graphAt(px, py), () => { need(o.services.ai.assertion, 'Boardcoord.cpp 0x235 실제 진단장치 미연결')('0'); });
                    return approach.find([r.left, r.top, r.right, r.bottom], x, y, flags, () => o.gameplayGlobals.aiDebug().checking().value);
                },
                tryMove: (parent, x, y, target) => { this.#live(); return need(o.motion, '일반 통지의 실제54 미설치').paths.startPriest({ parent, destination: { x, y }, replaceStart: 0, helper: target, tailWord: 0, tailByte: 0, reserved: 0 }) !== null; },
                start: (parent, bits) => { this.#live(); const body = need(o.building, '일반 통지의 실제30 미설치'); o.assetModules.prepareBuilding(c.processState.now); body.start(parent, float(bits)); },
            },
        };
        this.effects = effects; this.gameplay = gameplay;
        this.boundsState = makePlayerGeyserBoundsState(c.ownerMode, { get mission() { return o.objectState.mission; }, get mapSize() { return o.battleOptions[6]; }, get puzzlePieceType() { return o.piece; }, get playerDirections() { return o.world.storage.geyserPlayerDirections; }, get decoderDiagnostics() { return o.metadata.geometry.decoderDiagnostics; } });
        o.construction = new RawCampaignConstructionBindings(bindConstructionRuntime(o.world), o.hp, o.sp, o.rewardState, o.metadata.frames, o.chunks, o.players, o.tables.broadcast, o.state.constructionState, this.boundsState, o.world.storage.recentGeysers, o.state.assetState.currentGeysers(), effects, gameplay);
        this.#ready = true;
    }

    // 일반 지형 파이프라인과 분리된 게임 배치 판정기를 같은 현재 월드에 설치한다.
    bindGameplayPlacement() {
        this.#live(); const o = this.#owner, c = o.core, state = o.state.gameplayPlacement;
        if (o.gameplayPipeline) throw new Error('실제 게임 배치 중복 연결');
        state.geometry = { ...o.metadata.geometry }; state.permission.useAlliances = 1;
        state.anchor.bindFullDeck(() => o.state.anchorState.currentIgnoreRestrictions);
        o.gameplayPipeline = new RawCanonPlacementPipeline(c.pool, c.hash, { types: c.types, frames: o.metadata.frames, shapes: c.shapes, hotspots: o.metadata.hotspots, spots: c.spots, islands: c.hash.view(0), regions: o.gameplayRegions }, c.bookkeeping, o.additional, o.contentFinder, state);
        this.#bindPriestPlacement();
    }

    // 사제 생성(건물이 파괴돼 붙잡힌 사제를 풀어 줄 때 등)의 배치 검사 MayPlace 를 같은 현재 월드에 설치한다. 원문은 사제와 일반 배치가 12×12 차단 배열·관계·충돌·기하·모양 전역을 공유하므로 일반 파이프라인과 같은 상태 묶음(`state`)을 빌리고,
    // 사제 전용 지형 지역 배열(`createPriestPlacementTerrainState`)만 따로 갖는다. 체인은 모양 → 지형 → 기하 → 충돌 → 미리보기 → 최종 접두 순서로 묶이며 후보 칸의 spot 이 0 이 아닐 때 `RawPriestSpawn.spawn` 이 이 판정을 부른다.
    // 일반 배치와 같은 전역(로컬 Player·클라이언트 모드·무시 genus)은 호출 때마다 현재 값으로 맞춘다. 이전에는 이 장치가 없어 그 지점에서 "사제 월드 배치 검사 몸체 미연결" 오류가 났다. (2026-11-23 WB04-E1269)
    #bindPriestPlacement() {
        const o = this.#owner, c = o.core, state = o.state.gameplayPlacement, surfaces = c.hash.view(0), frames = o.metadata.frames;
        const terrain = new RawPriestPlacementTerrain(c.pool, c.types, frames, surfaces, createPriestPlacementTerrainState());
        const geometry = new RawPriestPlacementGeometry(c.pool, c.types, frames, state.geometry, makePriestTerrainGeometryHooks(c.pool, terrain));
        const collision = new RawPriestPlacementCollision(c.pool, c.hash, c.types, state.collision, state.preview, makePriestGeometryCollisionHooks(c.pool, geometry, makePriestTerrainCollisionHooks(c.pool, terrain, {})));
        const preview = new RawPriestPlacementPreview(c.pool, c.types, c.spots, surfaces, state.preview, makePriestGeometryPreviewHooks(c.pool, geometry, makePriestPlacementCollisionHooks(c.pool, collision, {})));
        const shape = new RawPriestPlacementShape(c.pool, c.types, frames, c.shapes, o.metadata.hotspots, state.geometry, state.shape);
        const placement = new RawPriestPlacement(c.pool, c.types, state.placement, makePriestShapePlacementHooks(c.pool, shape, makePriestPlacementPreviewHooks(c.pool, preview, {})));
        o.priests.bindPlacement({ pool: placement.pool, mayPlace: (query) => {
            this.#live(); state.placement.localPlayer = o.playerState.battleOwner | 0; state.collision.clientMode = c.processState.server ? 1 : 0; state.collision.ignoredGenus = o.state.buildingState.read('pushGenus');
            return placement.mayPlace(query);
        } });
    }

    // 현재 실제 파이프라인에 Player/방향 동맹/Graph·섬 기록을 원문 소비 위치로 공급한다.
    mayPlace(query) {
        this.#live(); const o = this.#owner; const c = o.core; const pipeline = need(o.gameplayPipeline, '게임 요청의 전체 MayPlace 미설치'); const state = o.state.gameplayPlacement;
        state.placement.localPlayer = o.playerState.battleOwner | 0; state.permission.editor = o.globals.editor; state.permission.graphReady = o.state.anchorState.graphReady; state.permission.alliances = o.alliances; Object.assign(state.anchor, o.state.anchorState);
        state.collision.clientMode = c.processState.server ? 1 : 0; state.collision.ignoredGenus = o.state.buildingState.read('pushGenus'); state.geometry.patternTypes = o.state.constructionState.confirm.patternTypes; state.relations.islandCount = o.islands.count;
        const raw = new DataView(o.islandBytes.buffer, o.islandBytes.byteOffset, o.islandBytes.byteLength);
        // 앞선 효과 뒤 같은 현재32BYTE 섬 기록의 제한/존재만 순서대로 읽는다.
        for (let index = 0; index < o.gameplayRegions.length; index += 1) { o.gameplayRegions[index].restricted = raw.getUint32(4 + index * 32, true); o.gameplayRegions[index].exists = raw.getUint32(8 + index * 32, true); }
        return pipeline.mayPlace(query);
    }

    // 실제 설치 관문 뒤 현재 Login만 빌리며 동기 직접 수신으로 우회하지 않는다.
    sendChatMutable(target, packet) {
        this.#live(); const o = this.#owner; if (!o.loaded) throw new Error('실제 설치 전 mutable Chat 패킷 송신'); if (!o.login) throw new Error('실제 로컬 전송 없는 mutable Chat 송신'); o.login.sendMutable(target, packet);
    }

    // 다른 대상도 같은264BYTE 패킷을 사용하여 앞선 observer의 변경을 뒤 대상에 보존한다.
    sendRemoteTell(owner, text, others) {
        this.#live(); const o = this.#owner; const packet = writeRemoteTellPacket(text, (...args) => o.world.storage.assertDisplay(...args), others ? 0x4fb : 0x4ef);
        if (others) {
            // 원문의 Player1..8 순서로 자신만 제외하며 실제 연결 관문은 현재 전송에 맡긴다.
            for (let recipient = 1; recipient < 9; recipient += 1) if (recipient !== (owner >>> 0)) this.sendChatMutable(recipient, packet);
        } else this.sendChatMutable(owner | 0, packet);
    }

    // 반납한 생성 구간의 closure는 이전 World에 다시 들어갈 수 없다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Construction'); }
    // 실제 프로세스 종료 뒤 외부 대여만 닫고 장기 최근 목록/방향표는 초기화하지 않는다.
    dispose() { this.#disposed = true; }
    // 생성자1652 정상 복귀이며 World 게시/Load 완료는 아니다.
    get ready() { return this.#ready; }
}
