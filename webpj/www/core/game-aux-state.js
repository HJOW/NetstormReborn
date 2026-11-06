// 원본 10.78 의 게임 보조 전역(현재 TYPE 번호·모드·장부·기한 등)을 클라이언트 수명으로 한 번만 소유하는 모듈이다.
// 원본은 각 값이 .data/BSS 의 고정 DWORD/float/double 이고, 생성·Pop·보상·HP·지형 등 여러 소비자가 같은 주소를 직접 읽고 쓴다.
// 웹도 소비자마다 사본을 만들지 않고 이 객체 하나를 빌린다. 세션/미션 생성·반납으로 다시 만들지 않으며, 원본 일반 월드 준비에 해당하는 일부 값만 resetForGeneralWorld 가 되돌린다.
// 근거: cpppj `o/RawGameAuxState.h`(커밋 886f2d23). 초기값은 보호 원본 originals/Netstorm.exe 의 .data(초기화 영역)·BSS(0)와 같다(tests/game-aux-state.test.mjs 가 PE 를 읽어 대조).
// 범위 밖: cpppj 의 RawSquidTransmissionState(공통 전송 버퍼/압축 전역)는 실제 객체 송신 소비자와 함께 판단한다. CD 10.72 주소는 다루지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-C2: 세션 장기 상태의 현재 TYPE/모드 전역 이식으로 추가)

// 필드 폭 종류이다: u32(부호 없는 DWORD), i32(signed DWORD), f32(float), f64(double), bool(원본 0/1 바이트). (2026-11-05 WB04-C2)
const KIND_CHECKS = Object.freeze({
    // 0~0xffffffff 정수만 받는다.
    u32: (value) => {
        if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new RangeError('부호 없는 DWORD 범위 오류');
        return value;
    },
    // signed 32비트 정수만 받는다.
    i32: (value) => {
        if (!Number.isInteger(value) || value < -0x80000000 || value > 0x7fffffff) throw new RangeError('signed DWORD 범위 오류');
        return value;
    },
    // 수를 float32 로 반올림해 저장한다.
    f32: (value) => {
        if (typeof value !== 'number') throw new TypeError('float 값은 수여야 합니다.');
        return Math.fround(value);
    },
    // double 은 수를 그대로 저장한다.
    f64: (value) => {
        if (typeof value !== 'number') throw new TypeError('double 값은 수여야 합니다.');
        return value;
    },
    // 원본 0/1 값이며 boolean 만 받는다.
    bool: (value) => {
        if (typeof value !== 'boolean') throw new TypeError('bool 값은 boolean 이어야 합니다.');
        return value;
    },
});

// 보조 전역 필드 표이다: [이름, 원본 10.78 주소, 폭, 초기값, 대응 타입 이름(TYPE 번호 칸만), 설명].
// 초기값은 PE .data 의 값이고 BSS 는 0 이다. 대응 타입 이름은 웹 타입 표(TYPE_LOAD_ORDER·PROCESS_TYPE_NAMES·내장 Form)와 맞춰 본 이름이다. (2026-11-05 WB04-C2)
export const GAME_AUX_FIELDS = Object.freeze([
    ['ordinaryGeysers', 0x005950ac, 'u32', 0, null, 'Start 가 moreGeysers 의 부정을 쓴다'],
    ['geyserFrameFlag', 0x005950b0, 'u32', 0, null, '486bc0 의 미복원 프레임 입력'],
    ['savedGeyserCount', 0x00595090, 'u32', 0, null, '저장 상태 읽기 tail[0] 의 출력 저장소'],
    ['geyserDeadline', 0x005950b8, 'f64', 0, null, '간헐천 게임 시각 기한'],
    ['cleanupDeadline', 0x005950c0, 'f64', 0, null, '원문 정리가 쓰는 별도 기한(의미 미확정)'],
    ['secondaryGeyserDeadline', 0x005950c8, 'f64', 0, null, 'b8 과 별개인 게임 프레임 기한'],
    ['scheduledDrawDeadline', 0x00595358, 'f64', 0, null, 'Draw/Dais 부모가 읽는 게임 기한'],
    ['totalCost', 0x0059508c, 'i32', 0, null, 'Pop/삭제 장부의 signed 비용'],
    ['resourcePool', 0x00595088, 'i32', 0, null, '보상/지역 간헐천이 공유하는 signed 자원'],
    ['refundPercent', 0x005424bc, 'u32', 25, null, '보상/로그인/전투 옵션이 공유하는 환불 비율'],
    ['allowAnyCapture', 0x005453e4, 'i32', 0, null, 'atol 결과와 pickup 비0 관문(raw)'],
    ['archipelago', 0x005c8bb0, 'u32', 0, null, 'FarmCluster 군도 모드'],
    ['terrainEnabled', 0x00540a2c, 'u32', 1, null, '생성/Zone 보정 DWORD'],
    ['archipelagoMinimum', 0x0054264c, 'i32', 15, null, '군도 밀도 하한'],
    ['archipelagoMaximum', 0x00542650, 'i32', 35, null, '군도 밀도 상한'],
    ['archipelagoPercent', 0x005c8bb4, 'u32', 0, null, '강제 군도 밀도'],
    ['lostX', 0x005c848c, 'f32', 0, null, '공통 삭제가 소리 뒤 기록하는 최근 손실 X'],
    ['lostY', 0x005c8490, 'f32', 0, null, '최근 손실 Y'],
    ['defaultSp', 0x00540bdc, 'i32', 2000, null, '기본 SP(TYPE.cost 에는 float 로 변환)'],
    ['geyserType', 0x00541240, 'u32', 122, 'geyser', '현재 간헐천 TYPE'],
    ['fieryTailProcessType', 0x0054109c, 'u32', 11, 'fieryTailSystemProcessType', '불꽃 꼬리 생성자의 부착 TYPE'],
    ['streakProcessType', 0x005410a0, 'u32', 12, 'streakSystemProcessType', 'Teleport 생성·표시 부착·도착 삭제 TYPE'],
    ['regularProcessType', 0x00541128, 'u32', 46, 'regularProcessType', '일반 예약(Regular) 생성과 종속 조회 TYPE'],
    ['processFormType', 0x00541078, 'u32', 2, 'ProcessForm', '공통 Process 부모 Create/Take TYPE'],
    ['dependFormType', 0x00541074, 'u32', 1, 'DependForm', '감시 쌍 Create 와 검색 TYPE'],
    // 아래 세 칸은 공통 Form 층·contained 해제가 호출마다 읽는 종속 전역이다(squid-forms.js squidFormGlobals). (2026-11-06 WB04-D3 추가)
    ['gumpFormType', 0x0054107c, 'u32', 3, 'GumpForm', 'contained 해제 UI 검색이 종속 raw TYPE BYTE 와 비교하는 현재 Form TYPE'],
    ['contentFormType', 0x00541088, 'u32', 6, 'ContentForm', '공통 부착 꼬리 진단·contained 부모 통지의 현재 내용물 Form TYPE'],
    ['contentUnpopEvent', 0x005427dc, 'u32', 0x02000039, null, 'contained 해제가 큐에 넣는 내용물 변경(UpdateContents) 사건(Player 통지 005427bc 와 별개)'],
    ['sharedRegularProcessType', 0x00541164, 'u32', 61, 'sharedRegularProcessType', '공유 예약 TYPE(일반 예약과 별개)'],
    ['processTypeEnd', 0x0054116c, 'u32', 63, null, '프로세스 TYPE 상한(원문 signed 비교, 10+53)'],
    ['evilRayProcessType', 0x00541098, 'u32', 10, 'evilRaySystemProcessType', '악마 광선 생성자 부착 TYPE'],
    ['particleSource', 0x00595128, 'u32', 0, null, '현재 파편 원천 DWORD(SID 로 줄이지 않음)'],
    ['flyerProcessType', 0x005410ec, 'u32', 31, 'baseFlyerProcessType', '비행 프로세스 생성/홈 조회 TYPE'],
    ['altarType', 0x005412e8, 'u32', 164, 'altar', '현재 제단 TYPE'],
    ['daisType', 0x005412ec, 'u32', 165, 'dais', '현재 다이스 TYPE'],
    ['priestType', 0x005412d0, 'u32', 158, 'priest', '현재 사제 TYPE'],
    ['fakeSurfaceType', 0x005412e0, 'u32', 162, 'fakeThreeByThreeSurface', 'Create 입구 진단이 읽는 TYPE'],
    ['runeType', 0x005412f0, 'u32', 166, 'rune', '현재 룬 TYPE'],
    ['rainBlockerType', 0x005412a4, 'u32', 147, 'rainBlocker', '현재 레인 바리케이드 TYPE'],
    ['growingRainBlockerType', 0x005412b0, 'u32', 150, 'growingRainBlocker', '성장형 레인 바리케이드 TYPE'],
    ['nuggetType', 0x005412c0, 'u32', 154, 'nugget', '현재 너겟 TYPE'],
    ['supportTypeWas', 0x0054d4e4, 'u32', 0, null, '받침 상세가 읽는 이전 TYPE'],
    ['firstTimerFactoryType', 0x005411ac, 'u32', 85, 'sunFactory', 'Factory 최초 event2 가 읽는 TYPE'],
    ['secondTimerFactoryType', 0x005411b4, 'u32', 87, 'rainFactory', 'Factory 최초 event1 이 읽는 TYPE'],
    ['outpostType', 0x00541250, 'u32', 126, 'outpost', 'Factory 프레임·연구·업그레이드의 제외 TYPE'],
    ['terrainType', 0x005411dc, 'u32', 97, 'isle', '현재 지면 TYPE'],
    ['edgeFarmType', 0x0054123c, 'u32', 121, 'edgeFarm', '가장자리 농장 TYPE'],
    ['worldGeyserMargin', 0x00542654, 'i32', 2, null, '전체/분할 간헐천 범위의 여백'],
    ['subscriberMarker', 0x005467bc, 'u32', 0, null, 'Subscriber 별도 표시 DWORD'],
    ['priestQuarterHp', 0x005ca8f0, 'bool', false, null, '사제 1/4 HP 0/1(생성·HP·연소·정리 공유)'],
].map(([name, address, kind, initial, typeName, note]) => Object.freeze({ name, address, kind, initial, typeName, note })));

// 세계 장식 네 TYPE 의 원본 주소·초기값·이름이다(treeTwo·RUIN·treeThree·Monument 순서). 생성 부모는 반복 중 현재 Monument 를 다시 읽는다. (2026-11-05 WB04-C2)
export const WORLD_DECORATION_FIELDS = Object.freeze([
    [0x005411e8, 100, 'treeTwo'], [0x00541338, 184, 'RUIN'], [0x005411ec, 101, 'treeThree'], [0x0054133c, 185, 'Monument'],
].map(([address, initial, typeName]) => Object.freeze({ address, initial, typeName })));

// 원본 일반 월드 준비(기존 지역 Runtime 의 fresh 입력)에서 되돌리는 필드와 값이다. 원본 정리 전체가 아니다. (2026-11-05 WB04-C2)
const GENERAL_WORLD_RESET = Object.freeze({
    ordinaryGeysers: 0, totalCost: 0, resourcePool: 0, refundPercent: 25, allowAnyCapture: 0, archipelago: 0, lostX: 0, lostY: 0, geyserType: 122, priestQuarterHp: false,
});

// 정리(cleanup)가 손실 좌표에 쓰는 float 비트(0xc61c4000 = -10000.0)이다. (2026-11-05 WB04-C2)
const CLEANUP_LOST_POSITION = -10000;

// 표에서 만든 접근자가 비공개 저장소에 닿도록 클래스의 정적 초기화 블록이 채우는 읽기/쓰기 함수이다(클래스 밖에서 직접 쓰지 않는다). (2026-11-05 WB04-C2)
let readField;
let writeField;

// 게임 보조 전역 소유자이다. GAME_AUX_FIELDS 의 각 이름이 같은 이름의 읽기/쓰기 속성이며, 쓰기는 원본 폭을 검사한다(u32/i32 범위·f32 반올림·bool).
// 사용: const aux = new GameAuxState(); aux.priestType === 158; aux.totalCost = -5; aux.worldDecorationTypes[3]. (2026-11-05 WB04-C2)
export class GameAuxState {
    // 비공개 저장소에 닿는 공통 읽기/쓰기 함수를 만든다(접근자 생성용).
    static {
        readField = (instance, name) => instance.#read(name);
        writeField = (instance, field, value) => instance.#write(field, value);
    }

    // 필드 이름 → 현재 값이다.
    #values = new Map(GAME_AUX_FIELDS.map((field) => [field.name, field.initial]));
    // 세계 장식 네 TYPE 의 실제 DWORD 저장소이다(빌려 주는 같은 배열).
    #decorations = Uint32Array.from(WORLD_DECORATION_FIELDS, (field) => field.initial);

    // 필드 값을 읽는다(접근자 공통 구현).
    #read(name) { return this.#values.get(name); }

    // 필드 값을 폭 검사 뒤 쓴다(접근자 공통 구현).
    #write(field, value) { this.#values.set(field.name, KIND_CHECKS[field.kind](value)); }

    // 세계 장식 네 TYPE 의 실제 저장소(Uint32Array 4칸)를 빌려 준다. 원소 쓰기는 원본 메모리처럼 32비트로 감긴다.
    get worldDecorationTypes() { return this.#decorations; }

    // 자원 DWORD 를 unsigned 비트로 읽는다(로그인/폭탄의 unsigned 조회).
    get resourcePoolBits() { return this.#read('resourcePool') >>> 0; }

    // 자원 DWORD 에 unsigned 비트를 같은 DWORD 로 쓴다(signed 로 다시 해석해 저장).
    storeResourcePoolBits(value) {
        this.resourcePool = KIND_CHECKS.u32(value) | 0;
    }

    // 원본 일반 월드 준비의 fresh 입력만 되돌린다. 현재 TYPE 들(사제·제단 등)·Form/예약 번호·기한·군도 밀도는 그대로 둔다.
    resetForGeneralWorld() {
        // 되돌릴 필드를 같은 폭 검사로 쓴다.
        for (const [name, value] of Object.entries(GENERAL_WORLD_RESET)) this[name] = value;
    }

    // 원본 정리의 두 저장(Y 다음 X)만 -10000.0 으로 쓴다. 일반 fresh 의 +0 과 구별한다.
    resetLostPositionForCleanup() {
        this.lostY = CLEANUP_LOST_POSITION;
        this.lostX = CLEANUP_LOST_POSITION;
    }
}

// 필드 표의 각 이름에 읽기/쓰기 접근자를 붙인다. 필드가 많아 표에서 만들며, 모든 쓰기가 같은 폭 검사를 거친다. (2026-11-05 WB04-C2)
for (const field of GAME_AUX_FIELDS) {
    Object.defineProperty(GameAuxState.prototype, field.name, {
        // 현재 값을 읽는다.
        get() { return readField(this, field.name); },
        // 폭 검사 뒤 쓴다.
        set(value) { writeField(this, field, value); },
        enumerable: true,
        configurable: false,
    });
}
