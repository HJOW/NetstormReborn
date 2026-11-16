// 실제 캠페인 자산 연결(RawCampaignAssetBindings) 검사가 쓰는 설치 장면이다(cpppj `tests/CampaignInstallSupport.h` 의 InstallScene 중 자산 연결에 닿는 부분).
// 보호 10.78 TYPE 188칸·실제 프레임 코드·SHP 셀 크기로 실제 RawSurfaceWorld 를 만들고 같은 풀에 섬 목록·9 Player 기록·Carrier Damageable·자산 상태를 조립한 뒤
// RawCampaignAssetBindings 를 설치하고 월드 runtime(Regular 처리기·Gun 전송)을 연결한다. 외부 UI/소리/전송 요청은 `events` 에 문자열로 기록하는 관찰 장치이며
// 선택하지 않은 하위 효과는 성공으로 덮지 않고 `forbidden` 오류로 보고한다. 사제/피해/전투(RawPriestWorldBindings·RawCampaignHitBindings·RawCampaignArcherBindings)는 아직 이 장면에 없다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E561: RawCampaignAssetBindings 이식과 함께 추가)
import { realCaptureAssets } from './capture-assets-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { put } from './raw-support.mjs';
import { FIRST_ASSET_TYPE_NUMBER } from '../www/assets/type-table.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AssetPostPopState } from '../www/core/raw-asset-post-pop-state.js';
import { AssetRegularState } from '../www/core/raw-asset-regular.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { RawCampaignAssetBindings } from '../www/core/raw-campaign-asset-bindings.js';
import { CarrierPostPopState, RawCarrierPostPop, makeCarrierPostPopHooks } from '../www/core/raw-carrier-post-pop.js';
import { GunProcessState } from '../www/core/raw-gun-process.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { PlayerWorldState } from '../www/core/raw-player-world.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { GameRandom } from '../www/core/random.js';

// Player 한 칸 폭과 첫 지역·동맹 마스크 위치, 지면 TYPE 이다. (2026-11-15 WB04-E561)
export const PLAYER_STRIDE = 0xb4;
const PLAYER_REGION = 0x78;
const PLAYER_ALLIES = 0x54;
export const TERRAIN_TYPE = 97;

// 선택되지 않은 하위 효과는 성공으로 덮지 않고 필요한 단계를 보고한다. (2026-11-15 WB04-E561)
export const forbidden = (name) => () => { throw new Error(`초기 캠페인 검사에서 미연결 하위 효과 요청: ${name}`); };

// 실제 자료에서 만든 월드 입력(TYPE·프레임 코드·실제 SHP 물리 프레임 표)이다. 한 번만 읽는다. 셰이프는 그림자/abstract 층까지 포함한 물리 프레임 순서(squidRendererShapes)이다. (2026-11-15 WB04-E561)
let inputCache = null;
function worldInputs() {
    if (inputCache) return inputCache;
    const real = realPlayerAssets();
    const capture = realCaptureAssets();
    // 번호마다 `.type` 정의의 프레임 코드 목록을 만든다(내장 타입은 빈 목록).
    const codes = real.types.map((_, number) => {
        const definition = number >= FIRST_ASSET_TYPE_NUMBER ? capture.table.definitions[number - FIRST_ASSET_TYPE_NUMBER] : null;
        return definition ? definition.clusters.map((cluster) => cluster.code) : [];
    });
    inputCache = { real, codes, shapes: capture.shapes, table: capture.table };
    return inputCache;
}

// 같은 풀의 실제 월드·섬·Player·Carrier·자산 상태와 자산 연결을 한 번 구성한다. options.effects 로 개별 효과 묶음을 덮어쓰고 options.bind=false 면 연결을 만들지 않는다. (2026-11-15 WB04-E561)
export class AssetScene {
    constructor(options = {}) {
        const { real, codes, shapes, table } = worldInputs();
        this.table = table;
        this.types = real.types.map((record) => ({ ...record }));
        this.frames = codes.map((list) => new FrameCodeTable(list));
        const links = new BridgeConnectState();
        Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true, mission: true });
        this.world = new RawSurfaceWorld(this.types, this.frames, shapes, links, TERRAIN_TYPE);
        this.core = this.world.core();
        this.random = new GameRandom(0x12345678);
        this.events = [];
        // 실제 목록은 원본 크기(100칸)로 두고 서버·boss 권한은 월드의 권한 소유자가 맡는다.
        this.assets = new AssetPostPopState();
        this.assets.inBattle = true;
        this.assets.localOwner = 1;
        for (const list of [this.assets.residences, this.assets.temples, this.assets.outposts, this.assets.geysers]) list.entries.length = 100;
        this.finishState = createFortFinishOwnerState();
        this.finishState.chaffMode = true;
        this.islandBytes = createRawIslandStorage();
        this.players = new Uint8Array(9 * PLAYER_STRIDE);
        // 원본 Player 의 첫 지역 sentinel 과 자기 동맹 비트를 9기록에 직접 쓴다.
        for (let owner = 0; owner < 9; owner += 1) {
            put(this.players, owner * PLAYER_STRIDE + PLAYER_REGION, 127);
            put(this.players, owner * PLAYER_STRIDE + PLAYER_ALLIES, 1 << owner);
        }
        this.playerState = new PlayerWorldState();
        this.playerState.playerKnown = 1;
        this.owners = [];
        this.islands = new RawIslandRuntime(this.core.pool, this.core.hash, this.islandBytes, this.finishState, this.assets.residences, { value: 0 }, { value: 0 }, {
            setResidenceOwner: (sid, owner) => { this.core.owner.set(sid, owner); },
            assertion: forbidden('섬 목록 진단'),
        });
        this.islands.reset();
        this.carrierState = new CarrierPostPopState();
        this.carrier = new RawCarrierPostPop(this.core.pool, this.core.types, this.carrierState,
            makeCarrierPostPopHooks(this.core.pool, this.core.postPop, (sid) => { this.events.push(`carrier:${sid}`); }, (x, y, owner) => { this.world.setSupportOwner(x, y, owner); }));
        this.regularState = new AssetRegularState();
        this.gunState = new GunProcessState();
        // 자기 자신만 동맹인 방향별 표이다.
        for (let owner = 0; owner < 9; owner += 1) this.gunState.allies[owner][owner] = true;
        this.elements = new Array(this.types.length).fill(0);
        this.archerCommand = { value: 0x030000b0 };
        this.globals = { regular: this.regularState, gun: this.gunState, elements: this.elements, archerNoticeCommand: this.archerCommand };
        this.effects = this.makeEffects(options.effects ?? {});
        this.bindings = null;
        if (options.bind !== false) this.bind();
    }

    // 자산 연결의 외부 효과 묶음이다. 관찰 장치는 요청을 events 에 기록하고, 고르지 않은 효과는 비워 두어 연결의 거부 경계가 드러나게 한다. (2026-11-15 WB04-E561)
    makeEffects(overrides = {}) {
        return {
            factory: { ...(overrides.factory ?? {}) },
            levelAssertion: overrides.levelAssertion,
            postPop: { reserveRegular: () => true, ...(overrides.postPop ?? {}) },
            regular: { ...(overrides.regular ?? {}) },
            gun: { ...(overrides.gun ?? {}) },
            queueCommand: 'queueCommand' in overrides ? overrides.queueCommand : (command, sid, argument) => { this.events.push(`queue:${command.toString(16)}:${sid}:${argument}`); },
            sendGunPacket: 'sendGunPacket' in overrides ? overrides.sendGunPacket : (packet) => { this.events.push(`gun:${Buffer.from(packet).toString('hex')}`); },
        };
    }

    // 같은 입력으로 자산 연결을 만든다. 거부 시험은 개별 입력을 덮어쓴다. (2026-11-15 WB04-E561)
    make(input = {}) {
        const use = { world: this.world, damageable: this.carrier, assets: this.assets, random: this.random, islands: this.islands, players: this.players, playerState: this.playerState,
            globals: this.globals, effects: this.effects, ...input };
        return new RawCampaignAssetBindings(use.world, use.damageable, use.assets, use.random, use.islands, use.players, use.playerState, use.globals, use.effects);
    }

    // 자산 연결을 만들고 공통 후처리에 등록한 뒤 월드 runtime 을 연결한다(첫 객체 생성 전 한 번). (2026-11-15 WB04-E561)
    bind() {
        this.bindings = this.make();
        this.bindings.activatePostPop();
        this.core.postPop.setBody('carrier', this.carrier);
        // 실제 공용 Squid writer의 패킷을 별도로 관측하며 기존 Form/효과 로그는 그 계약대로 유지한다.
        this.squidPackets = [];
        this.world.bindRuntime({
            squidPacket: (packet) => { this.squidPackets.push(Uint8Array.from(packet)); },
            assets: { emit: (event) => { this.events.push(`delete:${event.effect}:${event.sid}:${event.flags}`); }, selected: () => 0 },
            // 파생 가상 삭제 요청은 대상과 flags 만 관찰하고 처리한 것으로 돌려준다(실제 삭제 수명은 피해/수명 연결의 몫이다).
            destroy: (sid, flags) => { this.events.push(`destroy:${sid}:${flags.toString(16)}`); return true; },
            regular: this.bindings.handler((sid, event) => { throw new Error(`다른 실제 Regular 부모 미연결: TYPE${this.core.pool.slot(sid)[10]} event${event}`); }),
            transmit: this.bindings.transmission(),
            walkerFall: forbidden('보행자 추락'),
            loadingDepth: (depth) => {
                this.carrierState.loadingDepth = depth;
                this.bindings.setLoadingDepth(depth);
            },
            bridgeEffect: forbidden('다리 파편/소리'),
            // 서버 Form 전송 요청은 검사 외부 경계에서 현재 타입/flags 를 관찰한다. 실제 네트워크 구현을 뜻하지 않는다.
            formTransmit: (form, flags) => { this.events.push(`form:${this.core.pool.slot(form)[10]}:${flags.toString(16)}`); },
        });
        return this.bindings;
    }

    // 사각형 영역에 owner 의 지면(TYPE 97, 프레임 0)을 깔고 초기 로드를 마친다(월드당 한 번). (2026-11-15 WB04-E561)
    ground(left, top, right, bottom, owner = 1) {
        const seeds = [];
        // 행 우선으로 지면 한 칸씩 등록한다.
        for (let y = top; y <= bottom; y += 1) {
            for (let x = left; x <= right; x += 1) seeds.push({ type: TERRAIN_TYPE, owner, frame: 0, x, y });
        }
        this.world.load(seeds);
        return seeds.length;
    }

    // 각 점을 중심으로 한 (2×radius+1) 정사각 지면 조각만 깔고 초기 로드를 마친다(월드당 한 번). 넓은 지면은 로드가 느려 객체 자리만 깐다. (2026-11-15 WB04-E561)
    patches(points, owner = 1, radius = 1) {
        const seeds = [];
        const seen = new Set();
        // 점마다 주변 칸을 중복 없이 모은다.
        for (const [centerX, centerY] of points) {
            for (let y = centerY - radius; y <= centerY + radius; y += 1) {
                for (let x = centerX - radius; x <= centerX + radius; x += 1) {
                    const key = y * 256 + x;
                    if (seen.has(key)) continue;
                    seen.add(key);
                    seeds.push({ type: TERRAIN_TYPE, owner, frame: 0, x, y });
                }
            }
        }
        this.world.load(seeds);
        return seeds.length;
    }

    // (x, y) 칸의 지면 객체에 섬 번호 WORD(+8)를 쓴다(초기 로드의 지면은 번호 127 이다). 반환은 그 지면 SID 이다. (2026-11-15 WB04-E561)
    setIsland(x, y, island) {
        const sid = this.core.hash.cell(0, x, y);
        put(this.core.pool.allocated(sid), 8, island, 2);
        return sid;
    }

    // 지정 타입을 owner 로 만들고 (x, y) 에 Pop 한다. 반환은 새 SID 이다. (2026-11-15 WB04-E561)
    pop(type, x, y, flags = 3, owner = 1) {
        const sid = this.world.create(type, 0);
        this.core.pool.allocated(sid)[34] = owner;
        this.world.pop(sid, x, y, flags);
        return sid;
    }

    // 기록한 외부 요청을 꺼내고 비운다. (2026-11-15 WB04-E561)
    take() { return this.events.splice(0); }
}
