// 실제 저장 Fort 부모의 현재 Template 재조회·Territory 120BYTE·섬 배치/생성/후처리 순서를 한 곳에서 공유하는 모듈이다(cpppj `o/RawFortWorldLoad.cpp/.h`, 원본 004b5f70 Load · 004bd660 ReadTerritory, 10.78 전용).
//  - load(alternate)(004b5f70): builder(owner 1·청크 경계 1..15·실제 world 주소)를 만들고 현재 localOwner 의 Territory 120BYTE 를 읽어(현재 pos 에서 120BYTE 복사, pos += 120) PlaceAll(0, 20) 로 영역 20개를 놓은 뒤 현재 noRegion 으로 정상 지형을 생성한다.
//    alternate 가 0 이면 저장 Fort 후처리를 objects → money → badges → technology 순으로 부르되 **매 단계 직전에 현재 localOwner 와 그 owner 의 저장 Template 를 다시 조회**한다(앞 child 가 owner 를 바꿔도 다음 child 는 새 owner 를 쓴다).
//    alternate 가 0 이 아니면 현재 섬 0 이 존재할 때만(Exists 비영) populateIsland(state, 0)을 요청한다. 마지막은 항상 그래프 재구성(rebuildGraph(false))이다.
//  - readTerritory(templateSlot, output120)(004bd660): 현재 Template 의 첫 DWORD 가 0 이면 진단(MissingTerritoryTemplate) 뒤 계속 → Territory stream(lookup 은 pos 를 0 으로 되돌림)에서 현재 pos 의 120BYTE 를 복사(물리 120BYTE 가 없으면 거부, pos 불변)하고
//    pos 에 120 을 더한 뒤 signed pos > end 이면 진단(TerritoryPastEnd). 새 전체 parse 로 세지 않으며 없는 데이터의 가짜 0 을 주지 않는다.
//  - makeFortWorldChildrenHooks(pool, objects, meta, selectMeta, metaStream, hooks): 같은 풀의 FortObjectLoad.load(fort, owner, 0, 20)와 RawFortMetaLoad 의 Badges/Technology 선택 읽기를 훅에 연결한다(Money·섹션 선택·TypeNames·UI 는 호출자 경계로 남김).
// 10.78 전용이라 CD 의 Territory 깊이 증감(loadingDepthChanged)은 옮기지 않는다. puzzlePiece/localOwner/noRegion 은 현재 DWORD 를 읽는 `{ value }` 상자이며 child 직전마다 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-11 WB04-E286: 4-B Fort 월드 적재 부모 이식)
import { RawIslandBuilder } from './raw-island-builder.js';

// 보고 뒤 계속하는 Template/stream 진단 두 가지이다. (2026-11-11 WB04-E286)
export const FORT_WORLD_LOAD_ASSERTION = Object.freeze({ missingTerritoryTemplate: 'MissingTerritoryTemplate', territoryPastEnd: 'TerritoryPastEnd' });

// Territory 섹션 크기(6BYTE × 20)와 영역 수, 10.78 타입 수이다. (2026-11-11 WB04-E286)
const TERRITORY_BYTES = 120;
const TERRITORY_COUNT = 20;
const TYPE_COUNT = 188;

// 필수 child 훅 이름이다. (2026-11-11 WB04-E286)
const REQUIRED_HOOKS = ['templatePresent', 'territory', 'currentTemplate', 'assertion', 'builderAssertion', 'generate', 'objects', 'money', 'badges', 'technology', 'populateIsland', 'rebuildGraph'];

// 사용: new RawFortWorldLoad(pool, world, islands, frames, puzzlePiece, localOwner, noRegion, hooks).load(0). hooks = { templatePresent(slot)→bool, territory(slot)→{bytes,position,end}, currentTemplate(slot)→Template, assertion(종류), builderAssertion(종류),
//  generate(state, region), objects/money/badges/technology(template, owner), populateIsland(state, region), rebuildGraph(reset), decoderDiagnostics? }. (2026-11-11 WB04-E286)
export class RawFortWorldLoad {
    // world/Runtime/frames/현재 원본 전역은 부모보다 오래 살아야 한다. builder 의 world 주소 동일성을 보존한다. 판본/표 크기/필수 child 훅을 부모 효과 전에 확인한다.
    constructor(pool, world, islands, frames, puzzlePiece, localOwner, noRegion, hooks) {
        if (pool?.edition !== 'patch1078' || islands?.pool !== pool || !Array.isArray(frames) || frames.length !== TYPE_COUNT
            || typeof puzzlePiece?.value !== 'number' || typeof localOwner?.value !== 'number' || typeof noRegion?.value !== 'number' || !world
            || REQUIRED_HOOKS.some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('Fort World 실제 자료/필수 child/진단 연결 누락');
        this.pool = pool;
        this.world = world;
        this.islands = islands;
        this.frames = frames;
        this.puzzlePiece = puzzlePiece;
        this.localOwner = localOwner;
        this.noRegion = noRegion;
        this.hooks = hooks;
    }

    // 004bd660: 현재 pointer 의 120BYTE 를 직접 읽는다. output 은 정확히 120BYTE 이다. (2026-11-11 WB04-E286)
    readTerritory(templateSlot, output) {
        if (!(output instanceof Uint8Array) || output.length !== TERRITORY_BYTES) throw new RangeError('Fort Territory 출력은 120BYTE 여야 합니다.');
        if (!this.hooks.templatePresent(templateSlot)) this.hooks.assertion(FORT_WORLD_LOAD_ASSERTION.missingTerritoryTemplate);
        const stream = this.hooks.territory(templateSlot);
        const position = stream.position >>> 0;
        if (position > stream.bytes.length || stream.bytes.length - position < TERRITORY_BYTES) throw new RangeError('Fort Territory 현재 물리 120BYTE 누락');
        // 실제 DWORD 30개 복사와 동일한 120BYTE 만 읽으며 사용하지 않는 꼬리는 보존한다.
        for (let index = 0; index < TERRITORY_BYTES; index += 1) output[index] = stream.bytes[position + index];
        stream.position = (position + TERRITORY_BYTES) >>> 0;
        if ((stream.end | 0) < (stream.position | 0)) this.hooks.assertion(FORT_WORLD_LOAD_ASSERTION.territoryPastEnd);
    }

    // 004b5f70: ctor/attach → zero120 → Territory → PlaceAll → Generate → 모드별 후처리 → Graph 0 이다. alternate 는 signed DWORD 이다. (2026-11-11 WB04-E286)
    load(alternate) {
        const hooks = this.hooks;
        const state = { bounds: { left: 1, top: 1, right: 15, bottom: 15 }, owner: 1, decoderDiagnostics: hooks.decoderDiagnostics ?? null };
        const builder = new RawIslandBuilder(this.world, this.world, this.islands, this.frames, () => this.puzzlePiece.value >>> 0, state, hooks.builderAssertion);
        const records = new Uint8Array(TERRITORY_BYTES);
        this.readTerritory(this.localOwner.value >>> 0, records);
        builder.placeAll(records, 0, TERRITORY_COUNT);
        hooks.generate(state, this.noRegion.value | 0);
        if (!(alternate | 0)) {
            // 매 child 직전에 현재 localOwner 와 그 owner 의 저장 Template 를 다시 조회한다.
            for (const name of ['objects', 'money', 'badges', 'technology']) {
                const owner = this.localOwner.value >>> 0;
                hooks[name](hooks.currentTemplate(owner), owner);
            }
        } else if (this.islands.exists(0)) hooks.populateIsland(state, 0);
        hooks.rebuildGraph(false);
    }
}

// 같은 풀의 완료 child 를 재사용하고 기존 Money/변환/전체 populate 경계는 그대로 보존한다. selectMeta(fort, owner, technology) 는 매 child 의 현재 Template 칸/포인터/owner 를 MetaState 에 동기화하고,
// metaStream(fort, technology) 은 각 호출이 실제 Badges/Technology 섹션을 선택해 현재 cursor 를 돌려준다. 사용: makeFortWorldChildrenHooks(pool, objectLoad, metaLoad, selectMeta, metaStream, hooks). (2026-11-11 WB04-E286)
export function makeFortWorldChildrenHooks(pool, objects, meta, selectMeta, metaStream, hooks) {
    if (objects?.pool !== pool || meta?.pool !== pool || typeof selectMeta !== 'function' || typeof metaStream !== 'function') throw new TypeError('Fort World child 실제 풀/현재 전역/섹션 조회 불일치');
    return {
        ...hooks,
        objects: (fort, owner) => objects.load(fort, owner | 0, 0, TERRITORY_COUNT),
        // 실제 섹션 getter 는 각 wrapper 의 Template 진단/기술 부모 뒤에 실행하고 현재 Template 입력을 유지한다.
        badges: (fort, owner) => {
            selectMeta(fort, owner, false);
            meta.loadBadgesSelected(() => metaStream(fort, false));
        },
        technology: (fort, owner) => {
            selectMeta(fort, owner, true);
            meta.loadTechnologySelected(() => metaStream(fort, true));
        },
    };
}
