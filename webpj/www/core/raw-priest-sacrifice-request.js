// 10.78 사제의 제단 자격 wrapper(4498e0)와 희생 요청(494eb0)을 같은 현재 월드에 연결하는 모듈이다(cpppj `o/RawPriestSacrificeRequest.cpp/.h`).
//  - allowed(dais, priest): 현재 Dais 의 실제 자격 판정(RawDaisRegular::CanBridge)을 그대로 전달한다.
//  - begin(priest, dais): 사제의 너겟 내용물 제거(4b26f0(priest, 현재 너겟 종류, 0, 1)) → 같은 Dais 의 의식 명령(BridgeCommand(dais, priest, 1), 다리 건설이 아님) → 사제의 현재 owner BYTE 가 현재 로컬 owner 와 같으면 sacrifice.mus 음악 요청 순서이다.
//    별도 자격 검사는 덧붙이지 않는다(명령이 현재 자격을 다시 확인). callback 뒤 raw owner 와 로컬 owner 를 다시 읽는다.
// nuggetType()/localOwner() 는 호출 위치마다 현재 값을 읽는 getter(원본 DAT5412c0 초기값 154, DAT540c70)이다. 10.78 만 지원하며 누락된 효과를 성공으로 대체하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E14: 사제 희생 요청 이식으로 추가)

// 풀·현재 값 getter·실제 하위 효과를 연결하는 처리기이다. hooks = { allowed(dais, priest) → bool, remove(priest, type, 0, 1), command(dais, priest, 1), music(name) }. (2026-11-08 WB04-E14)
export class RawPriestSacrificeRequest {
    #pool;
    #nuggetType;
    #localOwner;
    #hooks;

    // 패치 10.78 의 실제 원문만 검증했으므로 다른 판본이나 누락된 효과는 거부한다.
    constructor(pool, nuggetType, localOwner, hooks) {
        if (pool?.edition !== 'patch1078' || typeof nuggetType !== 'function' || typeof localOwner !== 'function' ||
            ['allowed', 'remove', 'command', 'music'].some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('사제 희생 요청 판본/필수 효과 오류');
        this.#pool = pool;
        this.#nuggetType = nuggetType;
        this.#localOwner = localOwner;
        this.#hooks = hooks;
    }

    // wrapper 는 같은 현재 Dais 자격 판정 결과를 전달한다.
    allowed(dais, priest) { return this.#hooks.allowed(dais, priest); }

    // 너겟 내용물 제거 → 의식 명령 → 현재 로컬 사제일 때 음악 순서이다.
    begin(priest, dais) {
        this.#hooks.remove(priest, this.#nuggetType() >>> 0, 0, 1);
        this.#hooks.command(dais, priest, 1);
        if (this.#pool.slot(priest)[34] === (this.#localOwner() >>> 0)) this.#hooks.music('sacrifice.mus');
    }
}
