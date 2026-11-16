// 커서 조립 검사에 실제 보호 TYPE/SHP·World/Factory/Form 수명·건설 배치를 공급한다. 전체 Session/미션과 에너지·MayPlace/GUI의 게임 소비자 설치는 별도다. (WB04-E569)
import { realCaptureAssets } from './capture-assets-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { forbidden } from './campaign-asset-support.mjs';
import { floatToBits } from './raw-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { ChunkMap } from '../www/assets/territory.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { ContentFormHost } from '../www/core/content-form-host.js';
import { ContentLifetimeState } from '../www/core/content-form-lifetime.js';
import { RawCampaignLifetimeBindings } from '../www/core/raw-campaign-lifetime-bindings.js';
import { RawContainedCreate, makeContainedCreateHooks } from '../www/core/contained-create.js';
import { RawCampaignConstructionBindings, RawCampaignConstructionState, makePlayerGeyserBoundsState } from '../www/core/raw-campaign-construction-bindings.js';
import { RawCampaignCursor } from '../www/core/raw-campaign-cursor.js';
import { ClientCursorGlobals } from '../www/core/client-cursor-globals.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SquidForms } from '../www/core/squid-forms.js';

// 같은 물리 월드의 실제 커서와 선택한 외부 경계의 관찰 장치를 구성한다.
export class CampaignCursorScene {
    // 원문 조립 입력의 참조·값 사본을 변경할 수 있게 검사 수명 동안 보존한다.
    constructor() {
        const capture = realCaptureAssets(); this.assets = new GameAssets(capture.table, capture.database);
        this.types = realPlayerAssets().types.map((record) => ({ ...record })); this.metadata = new PriestPlacementAssets(capture.table, capture.database, this.types);
        const links = Object.assign(new BridgeConnectState(), { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true });
        this.world = new RawSurfaceWorld(this.types, this.metadata.frames.map((entry) => new FrameCodeTable(entry.frames)), this.metadata.shapes, links, 97); this.core = this.world.core();
        this.events = []; this.sent = []; this.mouse = { x: 0, y: 0 }; this.prepared = 0; this.timers = 0; this.rate = 0;
        this.drop = new CursorDropState(); this.owner = new ClientCursorGlobals(); this.known = { value: 1 }; this.wind = { value: 140 }; this.rangeType = { value: 51 };
        this.camera = new Uint32Array(2); this.cameraView = { pixels: new Int32Array(2) }; this.grid = new Uint8Array(144);
        this.random = new GameRandom(17); this.sp = new ScrambledSpStore(this.random, () => 1); this.tracking = { value: 0 }; this.sp.bindTrackingOffset(this.tracking); this.sp.initialize(); this.sp.set(1, floatToBits(5000));
        this.rewardState = new SquidRewardState(); this.hp = new SquidReward(this.core.pool, this.core.types, this.core.bookkeeping, this.rewardState, this.sp);
        this.world.bindDeletion(this.hp, () => 0, { emit: (event) => this.events.push(['delete-ui', event.effect, event.sid]), forward: (event) => this.events.push(['delete-wire', event.effect, event.sid]) });
        this.forms = new ContentFormHost(this.core.pool, this.core.types, (parent, form, reason) => this.events.push(['notice', parent, form, reason]));
        this.forms.bindPop((sid, flags) => this.core.factory.popForm(sid, flags)); this.forms.bindUnpop((sid) => this.core.factory.unpopForm(sid));
        this.lifeState = new ContentLifetimeState({ currentCursor: () => this.drop.preview });
        this.lifetime = new RawCampaignLifetimeBindings(this.world, this.forms, this.lifeState, { cancelCursor: () => this.cursor.cancelForState() }, this.hp, { pool: this.core.pool, notify: (p, f, r) => this.events.push(['player', p, f, r]) });
        this.contained = new RawContainedCreate(this.core.pool, { containedType: () => 6, bridgeType: () => this.core.bridgeState.bridgeType }, makeContainedCreateHooks(this.core.pool, this.core.factory, { notifySurface: (sid) => this.core.building.notifySurface(sid), attach: (sid, parent, flags) => this.forms.attach(sid, parent, flags) }));
        // 실제 수명과 공간을 사용하며 이 장면은 공통 미리보기 자산의 Pre/Post만 선택한다.
        // 실제 공용 Squid writer의 패킷을 별도로 관측하며 기존 Form/효과 로그는 그 계약대로 유지한다.
        this.squidPackets = [];
        this.world.bindRuntime({
            squidPacket: (packet) => { this.squidPackets.push(Uint8Array.from(packet)); }, assets: this.core.surfaces.hooks(), destroy: (sid, flags, hooks) => this.core.virtualDestroy.destroy(sid, flags, hooks), regular: forbidden('커서 외부 Regular 실행'),
            transmit: { writeData: forbidden('커서 외부 전송 자료'), transmit: forbidden('커서 외부 전송') }, walkerFall: forbidden('커서 보행자 추락'), bridgeEffect: forbidden('커서 다리 효과'), loadingDepth: () => {}, formTransmit: (sid, flags) => this.events.push(['form', sid, flags]) });
        this.constructionState = new RawCampaignConstructionState(); this.constructionState.place.localPlayer = this.constructionState.notice.localPlayer = 1;
        const recent = new SquidPostPopList(); recent.entries.length = 5; recent.entries.fill(0); const all = new SquidPostPopList(); all.entries.length = 100; all.entries.fill(0);
        this.construction = new RawCampaignConstructionBindings(this.world, this.hp, this.sp, this.rewardState, this.metadata.frames, new ChunkMap(), new Uint8Array(9 * 180), { sendMutable: forbidden('서버 건설 방송') }, this.constructionState,
            makePlayerGeyserBoundsState(this.core.ownerMode, { mission: false, mapSize: 0, puzzlePieceType: 107, playerDirections: new Array(9).fill(0) }), recent, all,
            { configInt: () => 1, wallSeconds: () => 1, refresh: () => this.events.push(['construction-refresh']), negativeQuantity: forbidden('간헐천 수량'), boundsAssertion: forbidden('간헐천 범위') });
        this.game = {
            prepare: () => { this.prepared += 1; this.drop.localPlayer = 1; }, moveBuilder: (...args) => this.events.push(['builder', ...args]), playDrop: () => this.events.push(['drop-sound']), productTimers: () => this.timers, unitRate: () => this.rate,
            lifecycle: { source: (sid) => new SquidForms(this.core.pool).parent(sid), sourceNotice: (p, f, r) => this.events.push(['source', p, f, r]), detach: (sid) => this.core.factory.unpopForm(sid), unpop: (sid, flags) => this.core.factory.popForm(sid, flags),
                destroy: (sid, flags) => this.world.destroy(sid, flags), addLocalSp: (delta) => { this.events.push(['charge', delta]); this.hp.addSp(1, delta); }, sound: (name) => this.events.push(['sound', name]),
                createMana: forbidden('실제 Session 마나 소비자'), createRange: forbidden('실제 Session 사정거리 소비자'), removeProcess: forbidden('실제 Session 프로세스 제거'), processEvent: (...args) => this.events.push(['process-event', ...args]) },
            // 바인딩의 외부 게임 경계만 명시 입력으로 둔다. 전체 에너지/공간 판정 통과나 실제 저장 미션 플레이로 세지 않는다.
            placement: { describe: () => 1, source: (sid) => new SquidForms(this.core.pool).parent(sid), region: () => 1, mayPlace: () => 1, processEvent: (...args) => this.events.push(['placement-event', ...args]), project: (x, y) => ({ x: Math.trunc(x * 16), y: Math.trunc(y * 11) }) },
            removeRange: forbidden('실제 Session 사정거리 제거'),
        };
        this.data = { world: this.world, assets: this.assets, geometry: this.metadata.geometry, frames: this.metadata.frames, forms: this.forms, contained: this.contained, reward: this.hp, random: this.random, construction: this.construction,
            drop: this.drop, grid: this.grid, sp: this.sp, playerKnown: this.known, windType: this.wind, rangeProcessType: this.rangeType, camera: this.camera, cameraView: this.cameraView, currentState: this.owner };
        this.cursor = this.make(); this.effects = this.ui(); this.cursor.setEffects(this.effects); this.cursor.snapToGrid.value = 1; this.world.load([]);
        this.parent = this.world.create(97, 2);
    }
    // 현재 구성에서 한 입력만 바꾸어 거부/수명 계약을 대조한다.
    make(data = {}, game = this.game) { return new RawCampaignCursor({ ...this.data, ...data }, game); }
    // 화면·번역·마우스·송신만 관찰하고 내부 계산/삭제/배치는 교체하지 않는다.
    ui(label = 'ui') { return { lifecycle: { highlight: (n) => this.events.push([label, 'highlight', n]), selectCursor: (n) => this.events.push([label, 'cursor', n]), closePopups: () => this.events.push([label, 'close']), translate: (s) => s, measure: (s) => [s.length * 8, 16], mouse: () => this.mouse },
        placement: { gumpAt: () => 0, invalidate: (...args) => this.events.push([label, 'dirty', ...args]) }, drop: { showSp: (...args) => this.events.push([label, 'show', ...args]), battleRefresh: () => this.events.push([label, 'battle']), refresh: (...args) => this.events.push([label, 'refresh', ...args]) },
        send: (target, packet) => { this.events.push([label, 'send', target]); this.sent.push(Uint8Array.from(packet)); } }; }
    // 실제 ContentForm을 원문 flags/품질로 생성한다. 생산 완료를 흉내 내는 코드가 아니다.
    form(type = 157, flags = 0, quality = 3) { return this.contained.create(this.parent, type, 0, flags, 0x10, quality); }
    // 현재 그림 치수로 정수 월드점에 해당하는 마우스를 공급한다.
    point(x = 20, y = 21) { const p = this.cursor.placement; this.mouse.x = x * 16 - this.cameraView.pixels[0] - p.hotspotX + Math.trunc(p.imageWidth / 2); this.mouse.y = y * 11 - this.cameraView.pixels[1] - p.hotspotY + Math.trunc(p.imageHeight / 2); this.cursor.update(this.mouse.x, this.mouse.y); }
}
