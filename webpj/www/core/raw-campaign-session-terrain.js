// Session 생성자1825..1865의 World 게시·일반 배치/지형·소유자 후처리 조립이다(225553df, WB04-E619).
// 사용: connectWorldTransmission 뒤 bind. Template/Load·전체 Client Session 공개는 후속 부모가 맡는다.
import { RawCanonPlacementPipeline } from './raw-canon-placement-pipeline.js';
import { FortRandomPlacementState, RawFortRandomPlacement, makeFortSinglePlacementHook } from './raw-fort-random-placement.js';
import { RawFortFinishOwner } from './raw-fort-finish-owner.js';
import { RawNormalTerrainWorldBinding, makeNormalTerrainFarmWorldHooks } from './raw-normal-terrain-world-binding.js';
import { TerrainContextGlobals } from './raw-terrain-context.js';
import { worldGeyserSpacing } from './raw-world-geyser-bounds.js';

// 같은 소유자의 칸을 현재 읽고 쓰는 참조다. CRT/TYPE/군도 값을 따로 복제하지 않는다.
function reference(owner, key) { return { get value() { return owner[key]; }, set value(value) { owner[key] = value; } }; }
// C++ SessionMissing처럼 선택된 진단 지점에서만 실패한다.
function missing(line) { return () => { throw new Error(`실제 세션 지형 진단 미연결: RawCampaignSession.cpp:${line}`); }; }

// 같은 Impl의 공개된 World와 지형 소비자를 한 번 구성하며 반납 후 진입을 거부한다.
export class RawCampaignSessionTerrain {
    // 실제 기반 소유자와 실패 접두/반복/반납 상태다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 지형 root가 빌리는 실제 현재 전역/진단 표다. 값형 함수 표는 구성 시 복사한다.
    generation = null; diagnostics = null;

    // 생성 자체로 World/난수/풀을 바꾸지 않는다.
    constructor(owner) { this.#owner = owner; }

    // World Runtime을 먼저 게시하고 같은 풀의 전체 배치·지형·finishOwner를 원문 순서로 만든다.
    bind() {
        const o = this.#owner; const c = o.core; const s = o.state;
        if (this.#disposed || !o.transmissionModules?.ready || this.#started) throw new Error('Session 지형의 초기화 전/반복 결선');
        this.#started = true;
        o.world.bindRuntime(o.worldHooks);
        // C++ geometry 값 복사이며 패턴/진단의 하위 참조는 현재 metadata를 빌린다.
        s.placement.geometry = { ...o.metadata.geometry }; s.placement.placement.localPlayer = 1; s.placement.permission.alliances.fill(1);
        o.pipeline = new RawCanonPlacementPipeline(c.pool, c.hash, { types: c.types, frames: o.metadata.frames, shapes: c.shapes, hotspots: o.metadata.hotspots, spots: c.spots, islands: c.hash.view(0), regions: [] }, c.bookkeeping, o.additional, o.contentFinder, s.placement);
        this.diagnostics = { context: missing(1828), stamp: missing(1828), region: missing(1828), farm: missing(1828), postpass: missing(1828), decoderDiagnostics: o.metadata.geometry.decoderDiagnostics };
        o.normal = new RawNormalTerrainWorldBinding(o.world, o.chunks, o.islands, o.random, reference(o.particleCrt, 'state'), o.terrainGlobals, o.stamp, o.randomPlacement, s.farm, reference(o, 'piece'), o.pipeline, o.hp, o.carrier, this.diagnostics);
        const aux = o.gameplayGlobals.gameAux(); const assertion = o.services.ai.assertion;
        this.generation = {
            enabled: () => { this.#live(); return aux.terrainEnabled; }, battle: () => { this.#live(); return c.ownerMode.battle; }, mission: () => { this.#live(); return c.bridgeState.mission; },
            layout: () => { this.#live(); return o.battleOptions[5]; }, spacing: () => { this.#live(); return worldGeyserSpacing(o.battleOptions[6]) | 0; },
            span: s.terrainSpan, minimum: reference(aux, 'archipelagoMinimum'), maximum: reference(aux, 'archipelagoMaximum'), percent: reference(aux, 'archipelagoPercent'), archipelago: s.terrainArchipelago,
            subscriber: (player) => {
                this.#live(); const current = o.coreDataGlobals.players;
                if (player < 1 || player > 8 || current.length < 9 * 180) throw new RangeError('지형 root의 현재 Player9 물리 범위');
                return new DataView(current.buffer, current.byteOffset, current.byteLength).getUint32(player * 180 + 0x2c, true);
            },
            placement: (player, record) => { this.#live(); if (!o.construction) throw new Error('지형 root의 실제 Player 배치 부모 미연결'); return o.construction.playerBounds.placement(player, record); },
            logPercent: (percent) => { this.#live(); if (typeof o.services.player.log !== 'function') throw new Error('지형 root의 실제 백분율 로그 미연결'); o.services.player.log(0x1d, `\nArchipelago Percentage: ${percent | 0}\n`); },
            growth: {
                checking: () => { this.#live(); return o.gameplayGlobals.aiDebug().checking().value; },
                assertion: (expression, file, line) => { this.#live(); if (typeof assertion !== 'function') throw new Error(`지형 실제 진단 장치 미연결 ${file}:${line}`); assertion(expression); },
            },
        };
        o.normal.bindGenerationInputs(this.generation);
        const single = makeNormalTerrainFarmWorldHooks(o.world, o.chunks, o.islands, o.pipeline, o.hp);
        single.mayPlace = (query) => { this.#live(); return o.constructionModules.mayPlace(query); };
        o.finishPlacement = new RawFortRandomPlacement(c.pool, c.hash, c.spots, c.types, c.frames, o.random, o.randomPlacement, single);
        const finish = {
            regionBounds: (region) => { this.#live(); const b = o.islands.bounds(region); return { left: b.left, top: b.top, right: b.right, bottom: b.bottom }; },
            wallSeconds: () => { this.#live(); if (typeof o.services.sound.wallSeconds !== 'function') throw new Error('소유자 후처리의 실제 벽시각 미연결'); return o.services.sound.wallSeconds(); },
            invalidType: missing(1862),
        };
        o.finish = new RawFortFinishOwner(c.pool, c.types, s.assetState.currentResidences(), o.finishState, makeFortSinglePlacementHook(c.pool, o.finishPlacement, finish));
        o.finish.bindPlayerRecords(o.players); this.#ready = true;
    }

    // 생성 완료 뒤 현재 지형 소비자만 사용한다. 전체 Session/Load 공개 여부는 별도다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 지형 소비자'); }
    // World/Host는 부모가 먼저 비우고 이 현재 효과의 대여를 무효화한다.
    dispose() { this.#disposed = true; }
    // 생성자 지형 구간만 완성됐음을 알린다.
    get ready() { return this.#ready; }
}

// Impl 선언부의 독립 typed 지형 상태다. 초기 문자는 원문97/98/127, 단일 배치도97/121을 쓴다.
export function createCampaignTerrainState() { return { terrainGlobals: new TerrainContextGlobals(), randomPlacement: new FortRandomPlacementState() }; }
