// 공통 객체 명령 004ae010 과 Form 부모 004ac9b0 의 분기 및 가상 인자를 옮긴 모듈이다(cpppj `o/RawObjectCommand.cpp/.h`, 10.78 전용).
//  - command(parent, code): 명령 코드의 상위 6비트를 제거(`code & 0x03ffffff`)해 다음을 처리하고 처리 1 / 미처리 0 을 반환한다. 소유자/생존 필터는 추가하지 않는다.
//    소유자 명령(기본 0x030000af)은 가상 +74 에 현재 로컬 소유자 전체 DWORD 를 전달하고, 피해 명령(0x030000e2)은 편집기이거나 미션 모드일 때만(단락 평가 순서 유지) 슬롯 +1a 의 HP DWORD 를 1 감소(32비트 감김)한 값으로
//    가상 +98 에 피해(소유자 0, 공격자 0)를 전달하며 제한된 모드에서도 처리 1 을 반환한다. 삭제 명령(0x03000015)은 Form 가상 +10 을 flags 0 으로, 진단 명령(0x0300006e)은 Form 가상 +70 진단을 부르며 알 수 없는 명령만 0 이다.
// state = { localOwner(), editor(), mission(), ownerCommand, hurtCommand, destroyCommand, describeCommand }(모두 호출 시점에 읽음, 명령 토큰은 생략하면 위 기본값), hooks = { owner(sid, owner), hit(sid, amount, owner, attacker), destroy(sid, flags), describe(sid) } 는 도달한 지점에서만 요구한다.
// CD 10.72 의 WORD HP/무조건 피해는 이식하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E123: RawObjectCommand 이식으로 추가)

// 원본 데이터 표(5429b0/542a7c)와 Form 부모(54274c/5428b0)의 기본 명령 토큰, 코드의 하위 26비트 마스크, HP DWORD 위치이다. (2026-11-08 WB04-E123)
const OWNER_COMMAND = 0x030000af;
const HURT_COMMAND = 0x030000e2;
const DESTROY_COMMAND = 0x03000015;
const DESCRIBE_COMMAND = 0x0300006e;
const COMMAND_MASK = 0x03ffffff;
const HP_AT = 26;

// 선택된 실제 하위만 요구하며 미처리 명령을 예외나 성공으로 바꾸지 않는다. (2026-11-08 WB04-E123)
function need(fn) {
    if (typeof fn !== 'function') throw new Error('공통 객체 명령의 실제 가상 효과 미연결');
    return fn;
}

// 10.78 의 객체 명령 처리기이다. 사용: new RawObjectCommand(pool, state, hooks). (2026-11-08 WB04-E123)
export class RawObjectCommand {
    // 현재 HP DWORD 를 읽는 같은 물리 슬롯 저장소, 실제 모드/로컬 소유자와 명령 토큰의 현재 조회, 같은 월드의 가상 소유자/피해/삭제와 별도 진단 경계이다.
    #pool;
    #state;
    #hooks;

    // 10.78 의 HP DWORD 배치를 다른 판본에 적용하지 않는다. 생성할 때 외부 효과를 호출하지 않는다. (2026-11-08 WB04-E123)
    constructor(pool, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new Error('공통 객체 명령은 10.78 전용');
        this.#pool = pool;
        this.#state = state ?? {};
        this.#hooks = hooks;
    }

    // 원본은 피해 명령이 제한된 모드에서도 처리 1 을 반환하고, 알 수 없는 명령만 Form 부모에서 0 을 반환한다. 사용: command(parentSid, code) → 1 또는 0. (2026-11-08 WB04-E123)
    command(parent, code) {
        const state = this.#state;
        const hooks = this.#hooks;
        const command = (code & COMMAND_MASK) >>> 0;
        if (command === ((state.ownerCommand ?? OWNER_COMMAND) >>> 0)) {
            const owner = need(hooks.owner);
            owner(parent, need(state.localOwner)() >>> 0);
            return 1;
        }
        if (command === ((state.hurtCommand ?? HURT_COMMAND) >>> 0)) {
            if (need(state.editor)() || need(state.mission)()) {
                const raw = this.#pool.slot(parent);
                // 원본 +1a 의 비정렬 DWORD 를 읽고 signed overflow 없이 DEC 의 32비트 감김을 보존한다.
                const hp = (raw[HP_AT] | (raw[HP_AT + 1] << 8) | (raw[HP_AT + 2] << 16) | (raw[HP_AT + 3] << 24)) >>> 0;
                need(hooks.hit)(parent, (hp - 1) | 0, 0, 0);
            }
            return 1;
        }
        if (command === ((state.destroyCommand ?? DESTROY_COMMAND) >>> 0)) {
            need(hooks.destroy)(parent, 0);
            return 1;
        }
        if (command === ((state.describeCommand ?? DESCRIBE_COMMAND) >>> 0)) {
            need(hooks.describe)(parent);
            return 1;
        }
        return 0;
    }
}
