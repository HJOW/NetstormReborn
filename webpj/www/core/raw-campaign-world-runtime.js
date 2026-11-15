// 실제 캠페인 부모와 일반 지형·저장 객체를 동일 raw 월드에 설치하는 client 연결이다(cpppj `client/RawCampaignWorldRuntime.cpp/.h`, 10.78 전용).
// 사용:
//   const runtime = new RawCampaignWorldRuntime(world, chunks, islands, player, normal, random, hp, ownership, finish, contained, objectState, globals, puzzlePiece, noRegion, data, effects);
//   runtime.load() 는 월드 BeginLoad → 깊이 1 → 실제 캠페인 부모(`RawCampaignWorldLoad.load`) → Graph(false) 뒤 종료(깊이 0)를 한 번 실행한다. 두 번째 호출/실패 뒤 재실행은 거부한다.
//   runtime.loadExistingFort(beforeWorld, alternate = 0) 는 일반 Fort 의 실제 네 child(Objects·Money·Badges·Technology)를 같은 월드로 읽는다(`data.existingFort` 필요).
//   runtime.finishMissionPriests() · runtime.pool · runtime.objects · runtime.readTerritory(slot, output120).
// data = { templates?, territories?, headers(180 BYTE), handle(NUL 포함), templateAt?(slot), territoryAt?(slot), existingFort?(objects), campaignObjects?(objects, slot) } 는 상위 로그인/Template 계층이 소유하는 현재 저장소를 그대로 빌린다(사본 없음).
// effects = { translateYou, setConfig, refreshWindowTitle, resetCamera, builderAssertion, territoryAssertion, notifySurface, setState, virtualCount, decoderDiagnostics?, preparePriests?, populateIsland? }
//   는 Core 밖의 UI/번역/가상 상태/진단 경계만 공급한다. 게임 효과는 월드·소유권·섬·Player 의 실제 모듈이 처리한다.
// 구성: 저장 객체 로더(`FortObjectLoad`)의 훅은 소유권(SetOwner)·월드(Pop/Destroy/로딩 깊이)·청크 지도·섬 목록·finish 후처리·생성/HP/내용물 팩토리 순으로 쌓고,
//   Fort 부모(`RawFortWorldLoad`)와 캠페인 부모(`RawCampaignWorldLoad`)는 같은 정상 지형 상위(`normal.generateOriginal`)와 Graph 재구성을 쓴다. 일반 Fort 가 주어지면 Money/Meta/Object child 도 기존 완료 몸체를 쓴다.
// 이 연결은 Kernel/Player/PostPop 등록의 수명을 소유하지 않으며 전투 프레임/Ready/경제 스크립트 부모를 대신하지 않는다. 캠페인이 쓰지 않는 generic Fort 후처리(Money/Badges/Technology)는 명시 오류로 둔다.
// 기준선 뒤 cpppj 변경(+2줄, 주석/표기 수준, 2026-11-15 확인)은 웹 결과에 영향이 없다(3.6절 ①). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E519: 4-D C 구획 RawCampaignWorldRuntime 이식)
import { FortObjectLoad, makeFortObjectLoadHooks } from './fort-object-load.js';
import { RawCampaignWorldLoad } from './raw-campaign-world-load.js';
import { makeFortFinishOwnerHooks } from './raw-fort-finish-owner.js';
import { RawFortWorldLoad } from './raw-fort-world-load.js';
import { makeFortWorldLoadBinding } from './raw-fort-world-load-binding.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// Template 칸 수, 칸 하나의 header 폭, 전체 header 크기이다. (2026-11-15 WB04-E519)
const TEMPLATE_COUNT = 9;
const TEMPLATE_STRIDE = 20;
const HEADER_BYTES = TEMPLATE_COUNT * TEMPLATE_STRIDE;
// 상위 설치가 반드시 공급해야 하는 Core 밖 효과 이름이다. 누락은 최초 등록/쓰기 전에 거부한다. (2026-11-15 WB04-E519)
const REQUIRED_EFFECTS = Object.freeze(['translateYou', 'setConfig', 'refreshWindowTitle', 'resetCamera', 'builderAssertion', 'territoryAssertion', 'notifySurface', 'setState', 'virtualCount']);

// 부호 있는 BYTE 로 읽은 청크 섬 번호이다. (2026-11-15 WB04-E519)
const signedByte = (value) => (value << 24) >> 24;

// 일반 Fort 가 아닌 캠페인 설치에서 도달하면 안 되는 generic Fort 부모 후처리이다. 성공으로 대신하지 않는다. (2026-11-15 WB04-E519)
function genericFortParent() {
    throw new Error('캠페인 설치에서 generic Fort 부모 후처리를 실행할 수 없습니다');
}

// 같은 Core 의 모든 하위 모듈과 상위 저장소의 참조를 보관하는 고정 구현이다. 값/배열 사본은 만들지 않는다. (2026-11-15 WB04-E519)
class WorldRuntimeBody {
    world;
    core;
    chunks;
    islands;
    player;
    normal;
    ownership;
    objectState;
    data;
    effects;
    objects;
    territory;
    campaign;
    existing = null;
    started = false;

    // 실제 저장소 동일성과 필수 경계를 최초 등록/쓰기 전에 검사한 뒤 하위 로더 세 개를 구성한다. (2026-11-15 WB04-E519)
    constructor(world, chunks, islands, player, normal, random, hp, ownership, finish, contained, objectState, globals, puzzlePiece, noRegion, data, effects) {
        this.world = world;
        this.core = world.core();
        this.chunks = chunks;
        this.islands = islands;
        this.player = player;
        this.normal = normal;
        this.ownership = ownership;
        this.objectState = objectState;
        this.data = data;
        this.effects = effects;
        const core = this.core;
        const vortex = ownership?.vortex?.();
        if (islands?.pool !== core.pool || player?.pool !== core.pool || normal?.pool !== core.pool || hp?.pool !== core.pool || hp.bookkeeping !== core.bookkeeping
            || vortex?.pool !== core.pool || finish?.pool !== core.pool || contained?.pool !== core.pool || vortex.records !== player.records
            || (data?.templates?.length !== TEMPLATE_COUNT && typeof data?.templateAt !== 'function') || (data.territories?.length !== TEMPLATE_COUNT && typeof data.territoryAt !== 'function')
            || data.headers?.length !== HEADER_BYTES || !(data.handle?.length > 0) || REQUIRED_EFFECTS.some((name) => typeof effects?.[name] !== 'function')) {
            throw new TypeError('캠페인 설치의 동일 Core/Player/9 Template/실제 경계 누락');
        }
        finish.bindPlayerRecords(player.records);
        // 저장 객체 로더 훅: 소유권 → 월드 → 청크/섬 → finish 후처리 → 생성/HP/내용물 순으로 쌓는다.
        let objectHooks = {
            notifySurface: effects.notifySurface,
            setState: effects.setState,
            virtualCount: effects.virtualCount,
            setOwner: (sid, owner) => { ownership.setOwner(sid, owner); },
            pop: (sid, x, y, flags) => { world.pop(sid, x, y, flags); },
            destroy: (sid, flags) => { world.destroy(sid, flags); },
            chunkRegion: (x, y) => signedByte(chunks.at(x, y).island),
            territoryAllowed: (region, owner, section) => islands.matches(region, owner | 0, section),
            loadingDepthChanged: (depth) => { world.setLoadingDepth(depth); },
        };
        objectHooks = makeFortFinishOwnerHooks(core.pool, finish, objectHooks);
        objectHooks = makeFortObjectLoadHooks(core.pool, core.factory, hp, contained, objectHooks);
        this.objects = new FortObjectLoad({
            pool: core.pool, finder: new RawSquidFinder(core.pool, core.hash, core.types), types: core.types, frames: core.frames, random, state: objectState, hooks: objectHooks,
        });
        // Fort 부모 훅: Template 현재 칸 검사와 Territory stream 을 매 호출 다시 선택한다. 현재 localOwner 는 Player 상태의 templateOwner 이다.
        let territoryHooks = {
            decoderDiagnostics: effects.decoderDiagnostics ?? null,
            templatePresent: (slot) => {
                this.checkSlot(slot);
                const record = data.headers.subarray(slot * TEMPLATE_STRIDE, slot * TEMPLATE_STRIDE + 4);
                return (record[0] | record[1] | record[2] | record[3]) !== 0;
            },
            territory: (slot) => {
                this.checkSlot(slot);
                const stream = data.territoryAt ? data.territoryAt(slot) : data.territories[slot];
                stream.position = 0;
                return stream;
            },
            currentTemplate: (slot) => this.template(slot),
            assertion: effects.territoryAssertion,
            builderAssertion: effects.builderAssertion,
            loadingDepthChanged: (depth) => { world.setLoadingDepth(depth); },
            generate: (builder, region) => { this.generate(builder, region); },
            objects: (input, slot) => { this.objects.load(input, slot | 0); },
            // 캠페인 부모가 사용하지 않는 generic Fort 분기는 실제 미복원 경계로 남긴다.
            money: genericFortParent,
            badges: genericFortParent,
            technology: genericFortParent,
            // Session 이 실제 섬 채우기를 연결한 경우에만 alternate Fort populate 를 실행한다(M6-n).
            populateIsland: effects.populateIsland ?? (() => { throw new Error('캠페인 설치의 alternate Fort populate 미연결'); }),
            rebuildGraph: (mode) => { core.graph.rebuild(mode); },
        };
        if (data.existingFort) {
            this.existing = data.existingFort(this.objects);
            const existing = this.existing;
            if (!existing?.money || !existing.meta || typeof existing.selectMoney !== 'function') throw new TypeError('일반Fort 실제Money/Meta몸체미연결');
            // Object/Generate/Graph 도 기존 같은 완료 몸체를 쓴다.
            territoryHooks = makeFortWorldLoadBinding(world, normal, this.objects, existing.money, existing.meta, existing.moneyStream, existing.selectMeta, existing.metaStream, territoryHooks);
            const objectLoad = territoryHooks.objects;
            territoryHooks.objects = (fort, owner) => {
                // 실제 Session 은 raw 스트림을 사용하며 독립 값형 장치만 기존 레코드 로더를 유지한다.
                if (existing.loadObjects) existing.loadObjects(owner);
                else objectLoad(fort, owner);
            };
            const moneyLoad = territoryHooks.money;
            territoryHooks.money = (fort, owner) => {
                // 실제 stream 선택은 여전히 LoadSelected 의 깊이/진단 뒤이다.
                existing.selectMoney(fort, owner);
                moneyLoad(fort, owner);
            };
        }
        // C++ 은 player.State().templateOwner 의 참조를 전달한다. JS 는 현재 값을 읽고 쓰는 상자로 같은 DWORD 를 빌린다.
        // 섬 Builder 는 타입별 프레임 코드 배열을 읽는다. 표(FrameCodeTable)의 같은 codes 배열을 빌려 사본을 만들지 않는다.
        const frameCodes = core.frames.map((table) => table.codes);
        const state = player.state;
        const templateOwnerBox = { get value() { return state.templateOwner; }, set value(next) { state.templateOwner = next; } };
        this.territory = new RawFortWorldLoad(core.pool, chunks, islands, frameCodes, puzzlePiece, templateOwnerBox, noRegion, territoryHooks);
        const campaignHooks = {
            translateYou: effects.translateYou,
            setConfig: effects.setConfig,
            refreshWindowTitle: effects.refreshWindowTitle,
            builderAssertion: effects.builderAssertion,
            generate: (builder, region) => { this.generate(builder, region); },
            currentTemplate: (slot) => this.template(slot),
            // 실제 캠페인 Session 은 원문의 004be610 을 같은 현재 stream 으로 실행한다.
            objects: (input, slot) => {
                if (data.campaignObjects) data.campaignObjects(this.objects, slot);
                else this.objects.load(input, slot | 0);
            },
            rebuildGraph: (mode) => { core.graph.rebuild(mode); },
            resetCamera: effects.resetCamera,
            decoderDiagnostics: effects.decoderDiagnostics ?? null,
            preparePriests: effects.preparePriests,
        };
        this.campaign = new RawCampaignWorldLoad(chunks, islands, frameCodes, puzzlePiece, noRegion, this.territory, player, globals, data.headers, data.handle, campaignHooks);
    }

    // 현재 Template 번호를 매 하위 호출 직전에 실제 9칸 범위로 검사한다. (2026-11-15 WB04-E519)
    checkSlot(slot) {
        if (!(slot >= 0 && slot < TEMPLATE_COUNT)) throw new RangeError('캠페인 현재 Template의9칸 범위 오류');
    }

    // 현재 Template 선택은 부모의 callback 뒤 변경된 owner 도 반영한다. (2026-11-15 WB04-E519)
    template(slot) {
        this.checkSlot(slot);
        return this.data.templateAt ? this.data.templateAt(slot) : this.data.templates[slot];
    }

    // 원본 Builder 의 실제 owner/bounds 를 현재 옵션/모드로 분기하는 같은 지형 상위에 전달한다. (2026-11-15 WB04-E519)
    generate(builder, region) {
        this.normal.generateOriginal(builder.owner, builder.bounds, region);
    }
}

// 신전 소유권·정상 지형·저장 객체·캠페인 부모를 한 월드에 설치하는 연결이다. (2026-11-15 WB04-E519)
export class RawCampaignWorldRuntime {
    // 고정 주소 구현이다. 공급한 상태와 객체는 이 구현보다 오래 살아야 한다.
    #body;

    // 모든 자료와 실제 모듈은 이 연결보다 오래 살아야 한다. 다른 풀/Player 기록은 거부한다. (2026-11-15 WB04-E519)
    constructor(world, chunks, islands, player, normal, random, hp, ownership, finish, contained, objectState, globals, puzzlePiece, noRegion, data, effects) {
        this.#body = new WorldRuntimeBody(world, chunks, islands, player, normal, random, hp, ownership, finish, contained, objectState, globals, puzzlePiece, noRegion, data, effects);
    }

    // BeginLoad → 실제 캠페인 부모 → Graph(false) 이후 종료를 한 번 실행한다. 실패를 로드 완료로 바꾸지 않는다. (2026-11-15 WB04-E519)
    load() {
        const state = this.#body;
        if (state.started) throw new Error('캠페인 동일 월드의 중복/실패 후 설치 재실행');
        state.started = true;
        state.world.beginLoad();
        state.objectState.loadingDepth = 1;
        state.campaign.load();
        state.world.finishLoadAfterGraph();
        state.objectState.loadingDepth = 0;
    }

    // 캠페인 전용 이름/readonly/Objects 부모를 우회 호출하지 않고 일반 Fort 의 실제 네 child 를 선택한다. beforeWorld 는 월드 읽기 앞의 외곽 준비이고,
    // alternate 1 은 새 Fort 의 원문 alternate(Objects/Money/Badges/Technology 읽기 생략)이다. (2026-11-15 WB04-E519)
    loadExistingFort(beforeWorld, alternate = 0) {
        const state = this.#body;
        if (state.started || !state.existing || typeof beforeWorld !== 'function') throw new Error('일반Fort실제자원/외곽준비누락또는재실행');
        state.started = true;
        state.world.beginLoad();
        state.objectState.loadingDepth = 1;
        beforeWorld();
        state.territory.load(alternate);
        // 부모의 Graph(false)를 중복 호출하지 않는다.
        state.world.finishLoadAfterGraph();
        state.objectState.loadingDepth = 0;
    }

    // 별도 사제 stub 을 만들지 않고 같은 실제 globals 를 읽는 기존 몸체로 전달한다. (2026-11-15 WB04-E519)
    finishMissionPriests() {
        this.#body.campaign.finishMissionPriests();
    }

    // 비교할 실제 Pool 이다. (2026-11-15 WB04-E519)
    get pool() { return this.#body.core.pool; }

    // 실제 저장 객체 로더이며 별도 사본을 만들지 않는다. (2026-11-15 WB04-E519)
    get objects() { return this.#body.objects; }

    // 새 로드/재파싱 없이 같은 현재 manager 의 Territory 읽기 부모를 실행한다. output 은 120BYTE 이다. (2026-11-15 WB04-E519)
    readTerritory(slot, output) {
        this.#body.territory.readTerritory(slot, output);
    }
}
