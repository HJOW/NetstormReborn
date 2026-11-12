// 부모 TYPE 에서 이동 경로 프로세스 종류(23/54/39/40/42/24/41/38)를 고르는 몸체를 옮긴 모듈이다(cpppj `client/RawPriestWorldBindings.cpp` 의 `RawMovementProcessTypes`·`SelectMovementProcessType`, 원본 00488fe0, 10.78 전용).
// RawPathStart 의 `processType` 훅과 사제 월드 바인딩이 같은 mutable 선택 상태를 읽도록 분리했다. 그 파일의 나머지(사제 월드 바인딩 본체)는 아직 옮기지 않았다.
//  - RawMovementProcessTypes: 현재 자산 TYPE 전역(sunWalker 133·empty 187·windWalker 137·rainWalker 156·teleport 134·bulf 135·dude 70·priest 158·manaBolt 105)과 내장 프로세스 번호
//    (worm 40·walker 38·balloon 42·teleportProcess 23·bulfProcess 41·priestProcess 54·flyProcess 39·dudeProcess 24)를 가진다. 실제 Session 은 장기 소유자의 현재 값을 빌린다:
//    `new RawMovementProcessTypes({ priest: 소유자(priestType), roles: 소유자(templeProductType·emptyCarrierType·rainWalkerType·bulfType·dudePathType) })` 이면 해당 칸은 매번 소유자의 현재 값을 읽고 쓰며,
//    소유자를 주지 않으면 독립 지역 값을 쓴다(값 복사 `copy()` 는 독립 저장소를 만든다).
//  - selectMovementProcessType(types, selection, type): 타입 표 범위(188) 확인 뒤 sunWalker/empty → 웜 40, windWalker/rainWalker → Walker 38, 타입 genus 0x20000(벌룬) → 42, teleport → 23, bulf → 41, dude → dudeProcess,
//    priest → 54, manaBolt → 비행 39, 그 밖은 웜 40 이다(10.78 은 미등록 타입도 웜). 우선순위와 타입 표 조회 시점은 원본 그대로다.
// 10.72(CD) 분기(worm 특수 반환이 표 조회 앞·미지원 타입 assert)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// 근거: cpppj `client/RawPriestWorldBindings.cpp/.h`, 원본 관측 pathprocesstype-x86.tsv 10.78 359행. (2026-11-13 WB04-E351: 6-P RawPathStart 이식과 함께 추가)

// 타입 표 길이(10.78)와 벌룬 genus 비트이다. (2026-11-13 WB04-E351)
const TYPE_COUNT = 188;
const BALLOON_GENUS = 0x20000;

// 소유자 객체의 칸 이름 → 지역 초기값이다. (2026-11-13 WB04-E351)
const ROLE_DEFAULTS = Object.freeze({ templeProductType: 133, emptyCarrierType: 187, rainWalkerType: 156, bulfType: 135, dudePathType: 24 });

// 현재 TYPE/프로세스 전역이다. 초기 번호는 두 보호 PE 의 DWORD 와 같다. 게임 중 전역을 바꿔도 다음 조회가 이 상태를 다시 읽는다. (2026-11-13 WB04-E351)
export class RawMovementProcessTypes {
    // 독립 입력의 지역 값이다(소유자를 빌리지 않은 칸만 쓴다).
    #local = { ...ROLE_DEFAULTS, priestType: 158 };
    // 빌린 장기 소유자(사제 TYPE 소유자, 역할 TYPE 소유자)이다. 없으면 null.
    #priest;
    #roles;

    // 소유자 두 개를 선택적으로 빌린다. 객체 형식이 아니면 거부한다. (2026-11-13 WB04-E351)
    constructor({ priest = null, roles = null } = {}) {
        if (priest !== null && (typeof priest !== 'object' || typeof priest.priestType !== 'number')) throw new TypeError('사제 TYPE 소유자는 priestType 을 가진 객체여야 합니다.');
        if (roles !== null && (typeof roles !== 'object' || Object.keys(ROLE_DEFAULTS).some((name) => typeof roles[name] !== 'number'))) throw new TypeError('역할 TYPE 소유자의 다섯 칸이 필요합니다.');
        this.#priest = priest;
        this.#roles = roles;
        // 이번 소유자 묶음과 별개인 기존 이동 TYPE 입력: windWalker·teleport·dude·manaBolt.
        this.windWalker = 137;
        this.teleport = 134;
        this.dude = 70;
        this.manaBolt = 105;
        // 내장 프로세스 번호: worm·simpleWalker·balloon·teleport·bulf·priest·simpleFly.
        this.wormProcess = 40;
        this.walkerProcess = 38;
        this.balloonProcess = 42;
        this.teleportProcess = 23;
        this.bulfProcess = 41;
        this.priestProcess = 54;
        this.flyProcess = 39;
    }

    // 역할 칸을 소유자 또는 지역 저장소에서 읽는다. (2026-11-13 WB04-E351)
    #role(name) { return this.#roles ? this.#roles[name] : this.#local[name]; }

    // 역할 칸을 소유자 또는 지역 저장소에 쓴다. (2026-11-13 WB04-E351)
    #setRole(name, value) {
        if (this.#roles) this.#roles[name] = value >>> 0;
        else this.#local[name] = value >>> 0;
    }

    // 0054126c: 현재 썬 워커(신전 제품) TYPE.
    get sunWalker() { return this.#role('templeProductType'); }
    set sunWalker(value) { this.#setRole('templeProductType', value); }

    // 00541344: 추가 빈(empty) TYPE.
    get empty() { return this.#role('emptyCarrierType'); }
    set empty(value) { this.#setRole('emptyCarrierType', value); }

    // 005412c8: 크리스탈 크랩(레인 보행) TYPE.
    get rainWalker() { return this.#role('rainWalkerType'); }
    set rainWalker(value) { this.#setRole('rainWalkerType', value); }

    // 00541274: 벌프 TYPE.
    get bulf() { return this.#role('bulfType'); }
    set bulf(value) { this.#setRole('bulfType', value); }

    // 005410d0: Dude 경로 프로세스 번호(24).
    get dudeProcess() { return this.#role('dudePathType'); }
    set dudeProcess(value) { this.#setRole('dudePathType', value); }

    // 005412d0: 현재 사제 TYPE.
    get priest() { return this.#priest ? this.#priest.priestType : this.#local.priestType; }
    set priest(value) {
        if (this.#priest) this.#priest.priestType = value >>> 0;
        else this.#local.priestType = value >>> 0;
    }

    // 값 사본은 독립 저장소를 가지며 원래 소유자 참조를 남기지 않는다. (2026-11-13 WB04-E351)
    copy() {
        const clone = new RawMovementProcessTypes();
        // 모든 칸의 현재 값을 복사한다.
        for (const name of ['sunWalker', 'empty', 'rainWalker', 'bulf', 'dudeProcess', 'priest', 'windWalker', 'teleport', 'dude', 'manaBolt', 'wormProcess', 'walkerProcess', 'balloonProcess',
            'teleportProcess', 'bulfProcess', 'priestProcess', 'flyProcess']) clone[name] = this[name];
        return clone;
    }
}

// 00488fe0 의 기존 몸체이다. types 는 188칸 TYPE 표(flags2), selection 은 RawMovementProcessTypes, type 은 부모 TYPE 이다.
// 타입 표 범위를 먼저 확인하고(표 밖은 거부), 우선순위는 웜 → Walker → 벌룬 → 텔레포트 → 벌프 → Dude → 사제 → 마나 볼트 → (그 밖은 웜)이다. (2026-11-13 WB04-E351)
export function selectMovementProcessType(types, selection, type) {
    if (types.length !== TYPE_COUNT) throw new RangeError('이동 프로세스 선택 판본/타입 표 크기 오류');
    if (!Number.isInteger(type) || type < 0 || type >= types.length) throw new RangeError('이동 프로세스 선택 타입 범위 오류');
    if (type === selection.sunWalker || type === selection.empty) return selection.wormProcess;
    if (type === selection.windWalker || type === selection.rainWalker) return selection.walkerProcess;
    if ((types[type].flags2 & BALLOON_GENUS) !== 0) return selection.balloonProcess;
    if (type === selection.teleport) return selection.teleportProcess;
    if (type === selection.bulf) return selection.bulfProcess;
    if (type === selection.dude) return selection.dudeProcess;
    if (type === selection.priest) return selection.priestProcess;
    if (type === selection.manaBolt) return selection.flyProcess;
    return selection.wormProcess;
}
