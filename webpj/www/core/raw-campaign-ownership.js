// 실제 캠페인의 신전 Owner → IslandList → 지역 투표/도장을 같은 월드 Core 에 조립하는 client 소비자이다(cpppj `client/RawCampaignOwnership.cpp/.h`, 10.78 전용).
// 사용:
//   const ownership = new RawCampaignOwnership(world, islands, assets, players, frames, themes, random, paintState, voteState, vortexRevision, 97, 121,
//       { otherOwner(sid, owner), refreshProduction(), themeMode(), missingIsland(sid, island, owner) });
//   ownership.setOwner(sid, owner) 는 신전의 실제 가상 표만 신전 Owner(RawVortexOwner)로 보내고 다른 파생은 otherOwner 에 넘긴다.
//   ownership.islandNumber(x, y) 는 float 좌표를 먼저 CRT 절삭한 뒤 현재 hash0 의 지역을 조회한다. ownership.updateRegion(x, y)/updateRegionNative(x, y) 는 지역 투표를 요청한다.
//   RawCampaignOwnership.native(owner, otherOwner) 는 Client 장기 본체(`ClientConstructionGlobals`)를 빌리는 두 번째 생성 경로이다.
// 구성: 지면 getter(RawCanonPlacementTerrain, 지역 조회만 사용) → 도장(RawRegionPaint) → 투표(RawRegionOwnership) → 신전 Owner(RawVortexOwner)를 기존 완료 모듈로 한 번 만들며,
//   월드 Core 의 풀/해시/TYPE/프레임/공통 Owner 를 그대로 빌려 쓴다(사본 없음). 섬 런타임에는 같은 9 Player 기록을 연결한다.
// 상위가 공급하는 것: 신전이 아닌 가상 소유자 분배기(otherOwner)·생산 갱신·themeMode 전역·신전 진단(missingIsland). 이 모듈은 이 경계를 빈 성공으로 채우지 않는다.
// Native 경로: cpppj 는 `ClientConstructionGlobals`가 보유한 장기 섬/투표/신전 본체를 빌린다. 웹은 그 소유자가 아직 없어(4-D 이후 Client 조립) 같은 형태의 덕타입
//   { vortexOwner(), islandNumber(x, y), regions() { updateNative(x, y) } } 을 받으며 실제 `ClientConstructionGlobals` 이식 때 같은 접근자를 구현한다. 이 모듈은 DOM 을 쓰지 않는다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 의 소스가 같음, 2026-11-15 확인).
// (2026-11-15 WB04-E518: 4-D C 구획 RawCampaignOwnership 이식)
import { createCanonPlacementTerrainState, RawCanonPlacementTerrain } from './raw-canon-placement-terrain.js';
import { RawRegionPaint, makeRegionPaintOwnershipHooks } from './raw-region-paint.js';
import { RawRegionOwnership, makeTerrainRegionOwnershipHooks } from './raw-region-ownership.js';
import { RawVortexOwner } from './raw-vortex-owner.js';
import { snapRegionCoordinate } from './raw-region-ownership.js';

// 신전 Owner 생성자가 요구하는 Player 한 칸 폭과 9칸 수이다. 9 Player 기록 전체 크기 검사는 RawVortexOwner 와 RawIslandRuntime 이 수행한다. (2026-11-15 WB04-E518)
const REQUIRED_EFFECTS = Object.freeze(['otherOwner', 'refreshProduction', 'themeMode', 'missingIsland']);

// 원본 plain ftol 처럼 float 좌표를 0 쪽으로 절삭한다. 비유한 값과 int32 밖 값은 거부한다. 투표용 0.99999 편향 snap 은 투표 어댑터가 따로 수행한다. (2026-11-15 WB04-E518)
function truncateCoordinate(value) {
    const result = Math.trunc(Math.fround(value));
    if (!Number.isFinite(result) || result < -0x80000000 || result > 0x7fffffff) throw new RangeError('캠페인 소유권 현재 지면 좌표 범위 오류');
    return result;
}

// Native 경로 생성을 독립 조립과 구분하는 내부 표식이다. 모듈 밖에서는 `RawCampaignOwnership.native()` 만 쓴다. (2026-11-15 WB04-E518)
const NATIVE_MODE = Symbol('RawCampaignOwnership.native');

// 내부 콜백이 주소를 빌리는 같은 월드의 하위 모듈 묶음이다. 소유자 객체보다 오래 살지 않으며 Native 경로에서는 만들지 않는다. (2026-11-15 WB04-E518)
class OwnershipBody {
    // 같은 월드·Core·섬 런타임·효과, 지면 getter 의 현재 sentinel 상태와 하위 몸체 네 개이다.
    world;
    core;
    islands;
    effects;
    getterState = createCanonPlacementTerrainState();
    getter;
    paint;
    ownership;
    vortex;

    // 자료 동일성을 먼저 확인한 뒤 실제 하위 몸체를 구성한다. 어떤 거부도 공유 상태를 쓰기 전에 일어난다. (2026-11-15 WB04-E518)
    constructor(world, islands, assets, players, frames, themes, random, paintState, ownershipState, revision, terrainType, edgeFarmType, effects) {
        this.world = world;
        this.core = world.core();
        this.islands = islands;
        this.effects = effects;
        const core = this.core;
        if (islands.pool !== core.pool || frames.length !== core.frames.length || themes.length !== core.types.length
            || REQUIRED_EFFECTS.some((name) => typeof effects?.[name] !== 'function')) {
            throw new TypeError('캠페인 소유권의 같은 Core/현재 표/필수 효과 누락');
        }
        // 이 객체는 지역 조회(regionAt)만 사용한다. 배치 순회는 상위 실제 PlacementPipeline 으로 분리한다.
        const guards = {
            candidatePermission: () => { throw new Error('소유권 getter의 별도 배치 후보 경계'); },
            shapePermission: () => { throw new Error('소유권 getter의 별도 배치 모양 경계'); },
            finishRelations: () => { throw new Error('소유권 getter의 별도 배치 완료 경계'); },
        };
        // 현재 hash0 머리 표는 사본이 아니라 살아 있는 참조(view)를 써야 이후 표면 변경을 읽는다.
        this.getter = new RawCanonPlacementTerrain(core.pool, core.types, frames, core.hash.view(0), this.getterState, guards);
        this.paint = new RawRegionPaint(core.pool, core.frames, core.hash.view(0), terrainType, edgeFarmType, random, paintState, ownershipState, {
            setOwner: effects.otherOwner,
            boundaryAt: (x, y, type) => world.find(x, y, type),
            regionAt: (x, y) => this.islandNumber(x, y) >>> 0,
        });
        let voteHooks = { themeMode: effects.themeMode };
        voteHooks = makeRegionPaintOwnershipHooks(core.pool, this.paint, voteHooks);
        voteHooks = makeTerrainRegionOwnershipHooks(core.pool, this.getter, voteHooks);
        // 원본 455380 은 0055a70c(아웃포스트)와 005e189c(신전)를 투표한다. 0055a4d4 의 생산 목록에는 이 시점의 삭제 중인 신전/일반 건물이 남아 섬 색/테마가 재점유된다.
        this.ownership = new RawRegionOwnership(core.pool, assets.currentOutposts(), assets.currentTemples(), themes, ownershipState, voteHooks);
        this.vortex = new RawVortexOwner(core.pool, core.owner, players, assets.currentOutposts(), revision, {
            islandNumber: (x, y) => this.islandNumber(x, y),
            islandExists: (island) => islands.exists(island) !== 0,
            setIslandOwner: (island, owner) => islands.setOwner(island, owner),
            regionOwnership: (x, y) => this.ownership.update(x, y),
            refreshProduction: effects.refreshProduction,
            missingIsland: effects.missingIsland,
        });
        islands.bindPlayerRecords(players);
    }

    // plain getter 는 float 를 먼저 원본 정수로 변환하고 현재 hash0 를 조회한다. 결과는 signed 32비트이다. (2026-11-15 WB04-E518)
    islandNumber(x, y) {
        // 원문 0046f530 은 좌표에 0041d7a0 의 편향 snap(0.99999 를 더해 절삭)을 먼저 적용한 뒤 plain 절삭으로 지형 칸을 읽는다. (2026-11-22 WB04-E1219: 이전에는 snap 이 빠져 소수 좌표의 칸이 한 칸 달랐다)
        return this.getter.regionAt(truncateCoordinate(snapRegionCoordinate(x)), truncateCoordinate(snapRegionCoordinate(y))) | 0;
    }
}

// 신전 소유 변경과 지역 번호/투표를 같은 월드에서 제공하는 캠페인 소유권 연결이다. (2026-11-15 WB04-E518)
export class RawCampaignOwnership {
    // 독립 조립에서 만든 하위 몸체 묶음이다. Native 경로에서는 null 이다.
    #body = null;
    // Native 경로가 빌리는 Client 장기 본체와 현재 짧은 Session 의 가상 소유자 분배기이다. 독립 조립에서는 null 이다.
    #native = null;
    #otherOwner = null;

    // 표면/프레임/난수/9 Player/세 목록과 원본 두 도장 상태를 동일 Core 에 연결한다. frames 는 PriestPlainCanonType(`{ frames }`) 188칸, themes 는 TYPE 별 테마 188칸,
    // vortexRevision 은 `{ value }` 상자이다. 첫 인자가 NATIVE_MODE 이면 Native 경로(`native()`)의 내부 호출이다(JS 는 생성자 중복 정의가 없어 표식으로 구분한다). (2026-11-15 WB04-E518)
    constructor(world, islands, assets, players, frames, themes, random, paintState, ownershipState, vortexRevision, terrainType, edgeFarmType, effects) {
        if (world === NATIVE_MODE) {
            this.#native = islands;
            this.#otherOwner = assets;
            return;
        }
        this.#body = new OwnershipBody(world, islands, assets, players, frames, themes, random, paintState, ownershipState, vortexRevision, terrainType, edgeFarmType, effects);
    }

    // Native 경로: 실제 장기 본체를 한 번 빌리고 사제 분배만 현재 짧은 Session 에 남긴다. 기존 독립 조립 방식은 바꾸지 않는다. (2026-11-15 WB04-E518)
    static native(owner, otherOwner) {
        if (!owner || typeof owner.vortexOwner !== 'function' || typeof owner.islandNumber !== 'function' || typeof owner.regions !== 'function' || typeof otherOwner !== 'function') {
            throw new TypeError('Native 소유권의 같은 장기 owner/현재 파생 분배 필요');
        }
        return new RawCampaignOwnership(NATIVE_MODE, owner, otherOwner);
    }

    // 원본 신전 가상 재정의의 Player/섬/지역 효과를 공통 Owner 로 축약하지 않는다. 신전 네 가상 표만 신전 Owner 로 가고 그 밖은 상위 분배기이다. (2026-11-15 WB04-E518)
    setOwner(sid, owner) {
        if (this.#native) {
            // 선택한 실제 owner/함수는 외부 효과가 wrapper 를 반납해도 호출이 끝날 때까지 살아 있도록 지역 변수에 보관한다.
            const vortex = this.#native.vortexOwner();
            const other = this.#otherOwner;
            if (vortex.handles(sid)) vortex.set(sid, owner);
            else other(sid, owner);
            return;
        }
        const body = this.#body;
        if (body.vortex.handles(sid)) body.vortex.set(sid, owner);
        else body.effects.otherOwner(sid, owner);
    }

    // 현재 plain float 지면 getter 를 상위 같은 월드의 자산 조회에 제공한다. 결과는 signed 32비트이다. (2026-11-15 WB04-E518)
    islandNumber(x, y) {
        return this.#native ? this.#native.islandNumber(x, y) : this.#body.islandNumber(x, y);
    }

    // Factory/신전의 같은 mode 0 지역 갱신을 제공한다. Native 는 장기 투표의 Native 진입을 쓴다. (2026-11-15 WB04-E518)
    updateRegion(x, y) {
        if (this.#native) this.#native.regions().updateNative(x, y);
        else this.#body.ownership.update(x, y);
    }

    // Outpost 의 Native 삭제는 같은 투표/도장 owner 의 signed 현재 목록 진입을 쓴다. 독립 조립은 기존 계약을 유지한다. (2026-11-15 WB04-E518)
    updateRegionNative(x, y) {
        if (this.#native) this.#native.regions().updateNative(x, y);
        else this.#body.ownership.updateNative(x, y);
    }

    // 같은 Player 주소/풀과 원본 몸체를 읽기 전용으로 제공한다. (2026-11-15 WB04-E518)
    vortex() {
        return this.#native ? this.#native.vortexOwner() : this.#body.vortex;
    }
}
