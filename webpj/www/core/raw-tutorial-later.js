// Early Missions 3~6(Capture the Priest·Tactical Combat·Subtle Defense·Raw Power)의 현재 단계 조건(PATCH 004c3f00/004c40f0/004c42b0/004c45e0)을 실제 게이트·장부·raw 객체에 연결하는 모듈이다(cpppj `o/RawTutorialLater.cpp/.h`, 10.78 전용).
//  - handle(tutorial, event): 입력 DWORD 3칸(event[0] 은 명령 종류 비교에서만 modifier 상위 비트를 `0x03ffffff` 로 지움, event[1]·event[2] 는 SID/type 인자를 보존)으로 미션 번호 3~6 의 한 switch 분기만 실행하며 같은 호출 중 다음 단계로 넘어가지 않는다. 반환값은 항상 0.
//    3(Capture the Priest): A 사거리 표시 명령이면 Advance(옵션 BYTE 2 = 2), B 적/자기 사제의 Graph 가 같거나 사제가 없을 때 Advance, C/D 생산 목록 조건, E 배치 완료 명령의 지면 지역·주변 부류, F 적 사제 없음/포획, G 내용물 통지.
//    4(Tactical Combat): 허용 표 쓰기와 전역/로컬 제작 수 비교(원본 cmp 동일/감긴 sub 부호), 내용물 통지·포획·상승 명령. 5(Subtle Defense): switch 전에 두 일회성 경고(`VortexDestroyed`·`FactoryDestroyed`, 각각 현재 단계/생산을 다시 읽음)와 선택 5초 double 지연.
//    6(Raw Power): 전역 제작 수·기술 알림·생산·사건.
//  - 모든 현재 TYPE 번호(원본 전역 주소 값)는 `state.type(주소)` 로 읽는 위치마다 다시 조회하고, 하위(Tell/정지/선택)가 바꾼 현재 단계/타이머/owner 를 재사용한다. Advance 가 거부돼도 선택 해제 두 효과는 원본대로 실행한다.
// state = { battleOptions(0052f7e0 의 같은 옵션, BYTE 2 만 수정), localOwner(00540c70, 현재 값을 읽는 속성), type(address) → 현재 TYPE 번호, rangeCommand·placedCommand·containedCommand·technologyCommand·captureCommand·ascendCommand }, players = 같은 9×180 Player 표(+0x78 지역),
// hooks = { findPriest(owner), graphFor(sid), productionContains(type), indicateTechnology(type), regionFor(x, y) → 정수, nearby(x, y, mask), priestCaptured(sid), selected(), gameSeconds(), resetSelection(), clearSelection(), tell(섹션) → boolean, closeTransientGumps() } (모두 필수),
// made = E237 RawTotalMade, gate = E231 RawMissionFrameGate, mission = E234 createMissionStartState() 상태(+c0/+c4 일회성 알림). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E239: 튜토리얼 3~6 단계 판정기 이식으로 추가)

// 명령 종류 비교에서만 제거하는 원본 modifier 비트와 선택 안내 지연(ROM 005071e0 의 double 5.0), 현재 TYPE 부류 비트, 단계 문자이다. (2026-11-10 WB04-E239)
const EVENT_MASK = 0x03ffffff;
const SELECTION_DELAY = 5.0;
const STAGE = Object.freeze({ A: 0x41, B: 0x42, C: 0x43, D: 0x44, E: 0x45, F: 0x46, G: 0x47, H: 0x48 });
// 필수 하위 경계 이름이다. (2026-11-10 WB04-E239)
const REQUIRED_HOOKS = Object.freeze(['findPriest', 'graphFor', 'productionContains', 'indicateTechnology', 'regionFor', 'nearby', 'priestCaptured', 'selected', 'gameSeconds', 'resetSelection', 'clearSelection', 'tell', 'closeTransientGumps']);

// 원본 비정렬 DWORD 필드를 해당 4바이트만 읽는다. (2026-11-10 WB04-E239)
const readDword = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
// DWORD 비트를 단정도 수로 읽는다. (2026-11-10 WB04-E239)
const view = new DataView(new ArrayBuffer(4));
function floatOf(bits) { view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); }

// PATCH 004c3f00/004c40f0/004c42b0/004c45e0 의 전체 단계 분기이다. (2026-11-10 WB04-E239)
export class RawTutorialLater {
    // 같은 Pool/TYPE/Player9/제작 장부와 gate/start 상태, 현재 전역과 실제 하위를 빌린다. 참조 대상은 더 오래 살아야 한다.
    #pool;
    #types;
    #players;
    #made;
    #gate;
    #mission;
    #state;
    #hooks;

    // 처음 실행 전에 같은 원본 저장소와 실제 하위의 물리/수명 전제를 확인한다. (2026-11-10 WB04-E239)
    constructor(pool, types, players, made, gate, mission, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || types?.length !== 188 || players?.length !== 9 * 180 || !(state?.battleOptions?.length >= 3) || typeof state.type !== 'function' ||
            REQUIRED_HOOKS.some((name) => typeof hooks[name] !== 'function')) throw new TypeError('튜토리얼3~6의 현재 원본 자료/하위 경계 누락');
        this.#pool = pool;
        this.#types = types;
        this.#players = players;
        this.#made = made;
        this.#gate = gate;
        this.#mission = mission;
        this.#state = state;
        this.#hooks = hooks;
    }

    // SID 0 도 원본 물리 슬롯을 읽을 수 있으나 host 범위 밖 번호는 명시 거부한다. (2026-11-10 WB04-E239)
    #object(sid) {
        if (sid >= this.#pool.capacity || sid > 0xffff) throw new RangeError('튜토리얼3~6의 현재 사건 SID 물리 범위');
        return this.#pool.slot(sid);
    }

    // 실제 004ac200 과 같은 현재 타입 BYTE 뒤 TYPE+ec(flags2)를 읽는다. (2026-11-10 WB04-E239)
    #genus(sid) {
        const type = this.#object(sid)[10];
        if (type >= this.#types.length) throw new RangeError('튜토리얼3~6 현재 부류 TYPE 범위');
        return this.#types[type].flags2 >>> 0;
    }

    // 현재 TYPE 번호(원본 전역 주소 값)를 읽는 위치마다 조회한다. (2026-11-10 WB04-E239)
    #type(address) { return this.#state.type(address) >>> 0; }

    // 원본의 cmp 동일/감긴 sub 결과 부호를 그대로 쓰며 수학적 signed 대소 비교로 바꾸지 않는다: 전역 제작 수 ≤ 로컬 제작 수. (2026-11-10 WB04-E239)
    #globalNotAboveLocal(address) {
        const total = this.#made.global(this.#type(address) | 0);
        const local = this.#made.local(this.#type(address) | 0);
        return total === local || ((total - local) | 0) < 0;
    }

    // Advance 가 실패해도 두 후속 선택 효과는 원본대로 실행한다. (2026-11-10 WB04-E239)
    #clearSelection() {
        this.#hooks.resetSelection();
        this.#hooks.clearSelection();
    }

    // 실제 창 생성 결과와 관계없이 원본 일회성 DWORD 를 1 로 쓴다(열렸으면 임시 Gump 정리 뒤 이 미션의 정지). (2026-11-10 WB04-E239)
    #tellOnce(name, key) {
        if (this.#hooks.tell(name)) { this.#hooks.closeTransientGumps(); this.#gate.pauseForDialog(); }
        this.#mission[key] = 1;
    }

    // 각 실제 판정기의 0 반환과 3 DWORD 인자를 같은 입력 큐에서 유지한다. (2026-11-10 WB04-E239)
    handle(tutorial, event) {
        if (!event || event.length < 3) throw new TypeError('튜토리얼 사건 DWORD 3개 누락');
        switch (tutorial) {
        case 3: this.#three(event); break;
        case 4: this.#four(event); break;
        case 5: this.#five(event); break;
        case 6: this.#six(event); break;
        default: throw new TypeError('튜토리얼3~6 밖 번호');
        }
        return 0;
    }

    // Capture the Priest: Graph 연결·배치 지역·내용물 사건은 실제 현재 객체에서 읽는다. (2026-11-10 WB04-E239)
    #three(event) {
        const command = (event[0] & EVENT_MASK) >>> 0; // switch 전에 한 번 읽는 실제 첫 입력 DWORD 이다.
        const state = this.#state;
        const hooks = this.#hooks;
        const advance = () => this.#gate.advance();
        switch (this.#gate.state.stage) {
        case STAGE.A:
            state.battleOptions[2] = 2;
            if (command === (state.rangeCommand >>> 0)) advance();
            break;
        case STAGE.B: {
            const enemy = hooks.findPriest((state.localOwner + 1) >>> 0);
            const own = hooks.findPriest(state.localOwner >>> 0); // 원본은 적 조회 후 자기 조회를 수행한다.
            if (enemy && own) {
                const ownGraph = hooks.graphFor(own);
                const enemyGraph = hooks.graphFor(enemy);
                if (enemyGraph !== ownGraph) return;
            }
            advance();
            break;
        }
        case STAGE.C:
            if (hooks.productionContains(this.#type(0x541174))) advance();
            break;
        case STAGE.D:
            hooks.indicateTechnology(this.#type(0x541184));
            if (hooks.productionContains(this.#type(0x541188)) || hooks.productionContains(this.#type(0x541184)) || hooks.productionContains(this.#type(0x54118c))) advance();
            break;
        case STAGE.E:
            if (command === (state.placedCommand >>> 0)) {
                const raw = this.#object(event[1]);
                const x = floatOf(readDword(raw, 14));
                const y = floatOf(readDword(raw, 18));
                const region = hooks.regionFor(x, y);
                if (raw[10] !== this.#type(0x541174)) return;
                if ((state.localOwner >>> 0) >= 9) throw new RangeError('튜토리얼3 현재 Player 지역 범위');
                if ((region >>> 0) === readDword(this.#players, (state.localOwner >>> 0) * 180 + 0x78)) return;
                if (!hooks.nearby(x, y, 0x1000000)) advance();
            }
            break;
        case STAGE.F: {
            const enemy = hooks.findPriest((state.localOwner + 1) >>> 0);
            if (!enemy || hooks.priestCaptured(enemy)) advance();
            break;
        }
        case STAGE.G:
            if (command === (state.containedCommand >>> 0) && (this.#genus(event[1]) & 0x200000) && !this.#object(event[1])[0x22]) advance();
            break;
        default: break;
        }
    }

    // Tactical Combat: 허용 표와 현재 전역/로컬 제작 비교는 같은 TotalMade 를 쓴다. (2026-11-10 WB04-E239)
    #four(event) {
        const command = (event[0] & EVENT_MASK) >>> 0; // 타입 사건이 아닌 명령 비교에서만 modifier 를 제거한다.
        const state = this.#state;
        const hooks = this.#hooks;
        const made = this.#made;
        const advance = () => this.#gate.advance();
        switch (this.#gate.state.stage) {
        case STAGE.A:
            made.allow(this.#type(0x5412ec) | 0, 0);
            state.battleOptions[2] = 2;
            if (hooks.productionContains(this.#type(0x541174)) && hooks.productionContains(this.#type(0x541184))) advance();
            break;
        case STAGE.B:
            if (this.#globalNotAboveLocal(0x541258)) advance();
            break;
        case STAGE.C:
            hooks.indicateTechnology(this.#type(0x541258));
            if (hooks.productionContains(this.#type(0x541258))) advance();
            break;
        case STAGE.D:
            if (this.#globalNotAboveLocal(0x541174)) advance();
            break;
        case STAGE.E:
            if (command === (state.containedCommand >>> 0) && (this.#genus(event[1]) & 0x200000) && !this.#object(event[1])[0x22] && this.#object(event[2])[10] === this.#type(0x54126c)) advance();
            break;
        case STAGE.F:
            made.allow(this.#type(0x5412ec) | 0, 1);
            if ((made.local(this.#type(0x5412ec) | 0) | 0) > 0) advance();
            break;
        case STAGE.G:
            if (command === (state.captureCommand >>> 0)) advance();
            break;
        case STAGE.H:
            if (command === (state.ascendCommand >>> 0)) advance();
            break;
        default: break;
        }
    }

    // Subtle Defense: 두 일회성 경고는 switch 전에 각각 현재 단계/생산을 다시 읽는다. (2026-11-10 WB04-E239)
    #five(event) {
        const command = (event[0] & EVENT_MASK) >>> 0;
        const state = this.#state;
        const hooks = this.#hooks;
        const made = this.#made;
        const mission = this.#mission;
        const advance = () => this.#gate.advance();
        if ((made.global(this.#type(0x541244) | 0) | 0) < 1 && !mission.fieldC0) this.#tellOnce('VortexDestroyed', 'fieldC0');
        if (this.#gate.state.stage > STAGE.B && (!hooks.productionContains(this.#type(0x541174)) || !hooks.productionContains(this.#type(0x541258)) || !hooks.productionContains(this.#type(0x541190)) || !hooks.productionContains(this.#type(0x541184))) && !mission.fieldC4) this.#tellOnce('FactoryDestroyed', 'fieldC4');
        const gate = this.#gate.state; // 하위 Tell/정지가 바꾼 현재 단계/타이머를 재사용한다.
        switch (gate.stage) {
        case STAGE.A:
            state.battleOptions[2] = 2;
            if (command === (state.technologyCommand >>> 0) && (event[1] >>> 0) === this.#type(0x541268) && (event[2] >>> 0) === this.#type(0x541190)) advance();
            break;
        case STAGE.B:
            if (hooks.productionContains(this.#type(0x541174)) && hooks.productionContains(this.#type(0x541258)) && hooks.productionContains(this.#type(0x541190)) && hooks.productionContains(this.#type(0x541184))) advance();
            break;
        case STAGE.C:
            if (gate.timerSeconds === 0) {
                let selectedType = 0; // 실제 첫 선택이 0 이면 두 번째 선택/raw 슬롯 조회를 생략한다.
                if (hooks.selected()) selectedType = this.#object(hooks.selected())[10];
                if (selectedType === this.#type(0x541178) || (this.#globalNotAboveLocal(0x541264) && this.#globalNotAboveLocal(0x541178))) gate.timerSeconds = hooks.gameSeconds() + SELECTION_DELAY;
            } else if (gate.timerSeconds <= hooks.gameSeconds()) { advance(); this.#clearSelection(); }
            break;
        case STAGE.D:
            if (this.#globalNotAboveLocal(0x541264) && this.#globalNotAboveLocal(0x541178)) advance();
            break;
        case STAGE.E:
            if (command === (state.containedCommand >>> 0) && (this.#genus(event[1]) & 0x100)) advance();
            break;
        case STAGE.F:
            if (command === (state.ascendCommand >>> 0)) advance();
            break;
        default: break;
        }
    }

    // Raw Power: 현재 전역 제작 수와 실제 기술 알림/생산/사건만으로 단계를 예약한다. (2026-11-10 WB04-E239)
    #six(event) {
        const state = this.#state;
        const hooks = this.#hooks;
        const made = this.#made;
        const advance = () => this.#gate.advance();
        switch (this.#gate.state.stage) {
        case STAGE.A:
            state.battleOptions[2] = 2;
            if ((made.global(this.#type(0x54128c) | 0) | 0) > 0 && (made.global(this.#type(0x541184) | 0) | 0) > 0) advance();
            break;
        case STAGE.B:
            if (((event[0] & EVENT_MASK) >>> 0) === (state.rangeCommand >>> 0)) advance();
            break;
        case STAGE.C:
            hooks.indicateTechnology(this.#type(0x541288));
            if (hooks.productionContains(this.#type(0x541288))) advance();
            break;
        case STAGE.D:
            if ((made.global(this.#type(0x541288) | 0) | 0) > 0) advance();
            break;
        case STAGE.E:
            if (!made.global(this.#type(0x541174) | 0) && !made.global(this.#type(0x541178) | 0) && !made.global(this.#type(0x541264) | 0)) advance();
            break;
        default: break;
        }
    }
}
