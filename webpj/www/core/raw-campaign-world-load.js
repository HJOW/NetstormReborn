// 실제 미션 새 login 의 저장 월드 부모를 generic Fort 메뉴 부모(RawFortWorldLoad)와 별도 몸체로 연결한 모듈이다(cpppj `o/RawCampaignWorldLoad.cpp/.h`, 원본 4b5300, 10.78 전용).
// 이 부모는 Money/Badges/Technology 를 호출하지 않는다. load() 순서: ① 현재 subscriber 를 login subscriber 로 맞춘다 → ② 현재 Template owner 의 Template header(+8 DWORD)를 1 로 쓴다 → ③ 번역키 "You" 의 NUL 포함 결과를 현재 mySubHandle 저장소에 복사하고
//   `fort.mySubHandle` 설정(NUL 포함 현재 handle)과 창 제목 갱신을 요청한다 → ④ Player 를 만든다(현재 battleOwner) → ⑤ builder(owner 1·경계 1..15)를 만들고 편집(editor) 전역이 켜져 있으면 현재 Player 기록의 첫 지역 DWORD(+0x78)를 0 으로 쓴다 →
//   ⑥ 현재 Template owner 의 Territory 120BYTE 를 읽어 PlaceAll(0, 20) → Generate(현재 noRegion) → 현재 Template owner 를 다시 읽어 Objects → Graph 재구성(false) → 미션 후처리(missionPost 가 0 일 때만 사제 준비 부모) → 카메라 재설정 →
//   현재 battleOwner 의 Player 기록 +0xc 에 현재 handle 이름(NUL 포함)을 복사한다. 각 단계 직전에 현재 두 owner(template/battle)를 다시 읽고 9기록 밖은 거부한다.
// CampaignWorldGlobals 는 이 부모와 미션 후처리가 읽는 현재 전역이다: mySubscriber(00540c74)·loginSubscriber(0054265c)는 외부 DWORD 상자를 빌릴 수 있고(빌리면 초기화 쓰기를 하지 않음), editor(005c85a4)·missionPost(00594fa4, 기본 true)는 값이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E287: 4-B 캠페인 월드 적재 부모 이식)
import { RawIslandBuilder } from './raw-island-builder.js';

// Player 기록의 판본 폭과 첫 지역 DWORD 위치, 이름 복사 위치, Template header 간격/오프셋, Territory 크기이다. (2026-11-11 WB04-E287)
const PLAYER_STRIDE = 0xb4;
const PLAYER_COUNT = 9;
const FIRST_REGION_OFFSET = 0x78;
const NAME_OFFSET = 0xc;
const TEMPLATE_STRIDE = 20;
const TEMPLATE_FLAG_OFFSET = 8;
const TEMPLATE_HEADER_BYTES = 180;
const TERRITORY_BYTES = 120;
const TERRITORY_COUNT = 20;
const TYPE_COUNT = 188;

// 필수 하위 경계 이름이다(미션 후처리의 preparePriests 는 missionPost 가 0 일 때만 요구). (2026-11-11 WB04-E287)
const REQUIRED_HOOKS = ['translateYou', 'setConfig', 'refreshWindowTitle', 'builderAssertion', 'generate', 'currentTemplate', 'objects', 'rebuildGraph', 'resetCamera'];

// 부호 없는 DWORD 상자 또는 값으로 읽고 쓰는 현재 전역 묶음이다. 사용: new CampaignWorldGlobals({ my: 0xabcdef01, login: 3, editor: 0, missionPost: true }) 또는 CampaignWorldGlobals.borrow({ my: box, login: box }). (2026-11-11 WB04-E287)
export class CampaignWorldGlobals {
    // 소유자 없는 독립 값(기본 0/0)이다.
    #localMy = 0;
    #localLogin = 0;
    // 외부 owner 의 { value } 상자이다(없으면 null 로 독립 값을 쓴다).
    #myBox = null;
    #loginBox = null;
    // 005c85a4 현재 Player 의 residence/island DWORD 를 0 으로 쓰는 편집 전역과 후처리 두 분기를 모두 생략하는 실제 미션 전역(기본 true)이다.
    editor = 0;
    missionPost = true;

    constructor({ my = 0, login = 0, editor = 0, missionPost = true } = {}) {
        this.#localMy = my >>> 0;
        this.#localLogin = login >>> 0;
        this.editor = editor >>> 0;
        this.missionPost = Boolean(missionPost);
    }

    // 같은 두 DWORD 상자를 직접 빌리며 외부 값에 초기화 쓰기를 하지 않는다. 생략한 쪽은 독립 값(0)을 쓴다. 외부 저장소는 이 객체보다 오래 살아야 한다. (2026-11-11 WB04-E287)
    static borrow({ my = null, login = null, editor = 0, missionPost = true } = {}) {
        for (const box of [my, login]) if (box !== null && typeof box?.value !== 'number') throw new TypeError('Subscriber 전역은 { value } 상자여야 합니다.');
        const globals = new CampaignWorldGlobals({ editor, missionPost });
        globals.#myBox = my;
        globals.#loginBox = login;
        return globals;
    }

    get mySubscriber() { return (this.#myBox ? this.#myBox.value : this.#localMy) >>> 0; }

    set mySubscriber(value) {
        if (this.#myBox) this.#myBox.value = value >>> 0;
        else this.#localMy = value >>> 0;
    }

    get loginSubscriber() { return (this.#loginBox ? this.#loginBox.value : this.#localLogin) >>> 0; }

    set loginSubscriber(value) {
        if (this.#loginBox) this.#loginBox.value = value >>> 0;
        else this.#localLogin = value >>> 0;
    }
}

// 사용: new RawCampaignWorldLoad(world, islands, frames, puzzlePiece, noRegion, territory, player, globals, templateHeaders180, currentHandle, hooks).load().
// 모든 현재 자료는 부모보다 오래 살고 같은 pool·지도·현재 Player 9기록 저장소를 써야 한다. puzzlePiece/noRegion 은 `{ value }` 상자이다. (2026-11-11 WB04-E287)
export class RawCampaignWorldLoad {
    constructor(world, islands, frames, puzzlePiece, noRegion, territory, player, globals, templateHeaders, currentHandle, hooks) {
        const pool = player?.pool;
        if (!pool || pool.edition !== 'patch1078' || islands?.pool !== pool || territory?.pool !== pool || !Array.isArray(frames) || frames.length !== TYPE_COUNT
            || typeof puzzlePiece?.value !== 'number' || typeof noRegion?.value !== 'number' || !(globals instanceof CampaignWorldGlobals) || !world
            || !(templateHeaders instanceof Uint8Array) || templateHeaders.length !== TEMPLATE_HEADER_BYTES || !(currentHandle instanceof Uint8Array) || currentHandle.length === 0
            || REQUIRED_HOOKS.some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('실제 캠페인 부모의 동일 Pool/현재 저장소/필수 하위 경계 누락');
        this.world = world;
        this.islands = islands;
        this.frames = frames;
        this.puzzlePiece = puzzlePiece;
        this.noRegion = noRegion;
        this.territory = territory;
        this.player = player;
        this.globals = globals;
        this.templates = templateHeaders;
        this.handle = currentHandle;
        this.hooks = hooks;
    }

    // NUL 포함 현재 좁은 문자 입력을 실제 저장소에 복사하며 물리 메모리 밖 실행을 거부한다(원본 strcpy 형태: 같은 NUL BYTE 까지 쓰고 나머지 stale 은 보존). (2026-11-11 WB04-E287)
    static #copyName(target, source) {
        const end = source.indexOf(0);
        if (end < 0) throw new RangeError('캠페인 실제 NUL 문자열 물리 바이트 누락');
        const count = end + 1;
        if (count > target.length) throw new RangeError('캠페인 실제 이름 대상 물리 바이트 범위 초과');
        target.set(source.subarray(0, count));
    }

    // 현재 battleOwner 의 실제 Player 9기록을 직접 빌린다(두 owner 를 합치지 않음). 9기록 밖은 거부한다. (2026-11-11 WB04-E287)
    #currentPlayer() {
        const owner = this.player.state.battleOwner >>> 0;
        if (owner >= PLAYER_COUNT) throw new RangeError('캠페인 현재 battleOwner 의 실제 9기록 범위 오류');
        return this.player.records.subarray(owner * PLAYER_STRIDE, (owner + 1) * PLAYER_STRIDE);
    }

    // mission 1 은 자연 반환하고 그 밖에는 실제 현재 기본 기술/지역 사제 부모를 선택한다(0x923e0). (2026-11-11 WB04-E287)
    finishMissionPriests() {
        if (!this.globals.missionPost) {
            if (typeof this.hooks.preparePriests !== 'function') throw new Error('Fort 실제 기술/사제 준비 부모 미연결');
            this.hooks.preparePriests();
        }
    }

    // 본문 순서를 보존하고 기존 하위 body 를 다시 구현하지 않는다. (2026-11-11 WB04-E287)
    load() {
        const hooks = this.hooks;
        const globals = this.globals;
        globals.mySubscriber = globals.loginSubscriber;
        const owner = this.player.state.templateOwner >>> 0;
        if (owner >= PLAYER_COUNT) throw new RangeError('캠페인 현재 Template owner 의 실제 9기록 범위 오류');
        const header = this.templates.subarray(owner * TEMPLATE_STRIDE + TEMPLATE_FLAG_OFFSET, owner * TEMPLATE_STRIDE + TEMPLATE_FLAG_OFFSET + 4);
        header[0] = 1;
        header[1] = header[2] = header[3] = 0; // 최초 현재 Template 의 +8 DWORD 만 1 로 저장한다.
        RawCampaignWorldLoad.#copyName(this.handle, hooks.translateYou());
        const handleEnd = this.handle.indexOf(0); // config 에 전달하는 현재 NUL 포함 handle 이다.
        if (handleEnd < 0) throw new RangeError('캠페인 현재 handle 의 실제 NUL 누락');
        hooks.setConfig('fort.mySubHandle', this.handle.subarray(0, handleEnd + 1));
        hooks.refreshWindowTitle();
        this.player.create(this.player.state.battleOwner | 0);
        const state = { bounds: { left: 1, top: 1, right: 15, bottom: 15 }, owner: 1, decoderDiagnostics: hooks.decoderDiagnostics ?? null };
        const builder = new RawIslandBuilder(this.world, this.world, this.islands, this.frames, () => this.puzzlePiece.value >>> 0, state, hooks.builderAssertion);
        if (globals.editor) this.#currentPlayer().fill(0, FIRST_REGION_OFFSET, FIRST_REGION_OFFSET + 4);
        const records = new Uint8Array(TERRITORY_BYTES);
        this.territory.readTerritory(this.player.state.templateOwner >>> 0, records);
        builder.placeAll(records, 0, TERRITORY_COUNT);
        hooks.generate(state, this.noRegion.value | 0);
        const current = this.player.state.templateOwner >>> 0;
        hooks.objects(hooks.currentTemplate(current), current);
        hooks.rebuildGraph(false);
        this.finishMissionPriests();
        hooks.resetCamera();
        RawCampaignWorldLoad.#copyName(this.#currentPlayer().subarray(NAME_OFFSET), this.handle);
    }
}
