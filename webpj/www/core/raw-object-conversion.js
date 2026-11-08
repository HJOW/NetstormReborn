// 비석(Buried)·주문(Spell) 객체의 Command 0x030000ce 전환(41e400/462290)을 실제 공간/타입 변경 하위와 조립하는 모듈이다(cpppj `o/RawObjectConversion.cpp/.h`, 10.78 전용).
// 원본 객체의 가상 Input 과 타입 변경을 GUI 수명에서 분리한다. 같은 SID 의 슬롯은 각 효과 뒤 다시 읽는다.
//  - objectConversionBody(vtable): 실제 가상 6c 가 41e400(주문 16종 vtable 표)/462290(비석 vtable 0x50a748)인 표만 해당 본문 번호를 반환하고 다른 클래스는 0 이다.
//  - RawObjectConversion.command(sid, code, body): 선택한 실제 6c 몸체를 실행한다. 명령 코드의 상위 입력 비트(0xfc000000)를 제거해 0x030000ce 가 아니면 미처리 명령으로 보고 부모(base)의 반환을 그대로 돌려준다.
//    맞으면 Unpop(flags 0) 뒤 본문별로 다음을 하고 Repop(flags 0) 뒤 1 을 반환한다. 비석 본문(462290): Unpop 뒤 **현재 QA WORD**(+12)로 타입 변경(retype) → 현재 TYPE 의 기본 프레임으로 프레임 DWORD(+36) 쓰기.
//    주문 본문(41e400): Unpop 뒤 현재 TYPE 을 보관해 두고 비석 타입(buriedType 의 현재 값)으로 타입 변경 → 프레임 0 → 보관한 이전 TYPE 을 QA WORD 로 쓰기. 선택하지 않은 경계는 요구하지 않고 도달한 누락은 앞선 쓰기를 보존한 채 거부한다.
//  - makeObjectConversionHooks(pool, factory, hooks, assertion): 같은 풀의 실제 SquidFactory.retype·프레임 DWORD(+36)·QA WORD(+12) 쓰기를 hooks 에 연결한다. 타입 변경 진단은 0x22d(isVoid)·0x22e(footX 불일치)·0x22f(footY 불일치)만 허용하며
//    assertion(식, 파일, 행)이 복귀하면 전환을 계속한다. 공간(unpop/repop)/부모(base)/기본 프레임(defaultFrame)은 입력 장치를 보존한다.
// hooks = { unpop(sid, flags), repop(sid, flags), retype(sid, type), defaultFrame(type)→DWORD, frame(sid, dword), qa(sid, word), base(sid, code)→DWORD }, buriedType 은 현재 비석 타입(getter 또는 `{ value }`).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E106: 객체 전환 이식으로 추가)

// 실제 6c 가 41e400 인 주문 생성자 vtable 16종과 462290 인 비석 vtable, 두 본문 번호, 변환 명령 코드와 상위 입력 비트 마스크이다. (2026-11-08 WB04-E106)
const SPELL_TABLES = Object.freeze([0x502370, 0x502448, 0x502510, 0x5025d8, 0x5026a0, 0x502778, 0x502840, 0x502908, 0x5029d0, 0x502aa8, 0x502b70, 0x502e68, 0x502f30, 0x502ff8, 0x5030c0, 0x503188]);
const BURIED_TABLE = 0x50a748;
const SPELL_BODY = 0x41e400;
const BURIED_BODY = 0x462290;
const CONVERT_COMMAND = 0x030000ce;
const COMMAND_MASK = 0x03ffffff;
// 004b1620 의 진단 0x22d..0x22f 가 보고하는 실제 식과 파일, 슬롯의 TYPE(+10)·QA WORD(+12)·프레임 DWORD(+36) 위치이다. (2026-11-08 WB04-E106)
const RETYPE_ASSERTIONS = Object.freeze(['isVoid()', '::getType(typeNum)->footX == ::getType(newType)->footX', '::getType(typeNum)->footY == ::getType(newType)->footY']);
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const FIRST_LINE = 0x22d;
const LAST_LINE = 0x22f;
const TYPE_AT = 10;
const QA_AT = 12;
const FRAME_AT = 36;

// 도달한 효과가 없으면 앞선 쓰기를 보존하고 중단한다. (2026-11-08 WB04-E106)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('객체 전환의 실제 하위 미연결');
    return hook;
}

// 모체·자산 이름이나 Bomb genus 만으로 서로 다른 원문 가상 입력을 합치지 않는다. (2026-11-08 WB04-E106)
export function objectConversionBody(vtable) {
    if (vtable === BURIED_TABLE) return BURIED_BODY;
    // 현재 실제 표에 처음 일치하면 끝낸다.
    for (const table of SPELL_TABLES) if (table === vtable) return SPELL_BODY;
    return 0;
}

// 실제 타입 변경의 진단을 선택한 위치에서만 요구하며 원문의 쓰기 폭을 유지한다. (2026-11-08 WB04-E106)
export function makeObjectConversionHooks(pool, factory, hooks, assertion) {
    if (factory.pool !== pool || pool?.edition !== 'patch1078') throw new Error('객체 전환 실제 Factory/풀/판본 불일치');
    return {
        ...hooks,
        retype: (sid, type) => {
            factory.retype(sid, type, (line) => {
                if (line < FIRST_LINE || line > LAST_LINE) throw new RangeError('객체 전환 원문 타입 변경 진단 행 범위');
                if (typeof assertion !== 'function') throw new Error('객체 전환 실제 타입 변경 진단 장치 미연결');
                assertion(RETYPE_ASSERTIONS[line - FIRST_LINE], SQUID_CPP, line);
            });
        },
        // 현재 프레임 DWORD 만 쓰며 인접 extra/level 은 보존한다.
        frame: (sid, frame) => {
            const raw = pool.allocated(sid);
            for (let byte = 0; byte < 4; byte += 1) raw[FRAME_AT + byte] = (frame >>> (8 * byte)) & 0xff;
        },
        // QA 는 원문 4ac220 처럼 하위 WORD 만 기록한다.
        qa: (sid, value) => {
            const raw = pool.allocated(sid);
            raw[QA_AT] = value & 0xff;
            raw[QA_AT + 1] = (value >>> 8) & 0xff;
        }
    };
}

// 원문 Command 의 상위 입력 비트 제거와 Unpop 뒤 snapshot 위치를 유지하는 전환 처리기이다. (2026-11-08 WB04-E106)
export class RawObjectConversion {
    // 실제 현재 TYPE/QA 를 읽는 유일한 저장소이다. (2026-11-08 WB04-E106)
    #pool;
    // Spell Unpop 뒤 읽는 현재 비석 타입의 소유자(getter 또는 { value })이다. (2026-11-08 WB04-E106)
    #buriedType;
    // 선택된 실제 하위만 요구하는 장치 경계이다. (2026-11-08 WB04-E106)
    #hooks;

    // 현재 10.78 풀과 00541218 의 장기 값을 빌린다. 생성 중 외부 효과나 원문 객체 변경은 실행하지 않는다. (2026-11-08 WB04-E106)
    constructor(pool, buriedType, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new Error('객체 전환은 10.78만 복원했습니다');
        this.#pool = pool;
        this.#buriedType = buriedType;
        this.#hooks = hooks;
    }

    // 현재 비석 타입이다. (2026-11-08 WB04-E106)
    #buried() { return typeof this.#buriedType === 'function' ? this.#buriedType() : this.#buriedType.value; }

    // 선택한 실제 6c 몸체를 실행하고 그 부모 반환을 보존한다. (2026-11-08 WB04-E106)
    command(sid, code, body) {
        const hooks = this.#hooks;
        if (body !== BURIED_BODY && body !== SPELL_BODY) throw new Error('선택한 객체 전환의 실제 가상 입력 미복원');
        if ((code & COMMAND_MASK) !== CONVERT_COMMAND) return need(hooks.base)(sid, code);
        need(hooks.unpop)(sid, 0);
        if (body === BURIED_BODY) {
            const raw = this.#pool.slot(sid);
            const type = raw[QA_AT] | (raw[QA_AT + 1] << 8);
            need(hooks.retype)(sid, type);
            const frame = need(hooks.defaultFrame)(this.#pool.slot(sid)[TYPE_AT]);
            need(hooks.frame)(sid, frame);
        } else {
            const previous = this.#pool.slot(sid)[TYPE_AT];
            need(hooks.retype)(sid, this.#buried());
            need(hooks.frame)(sid, 0);
            need(hooks.qa)(sid, previous);
        }
        need(hooks.repop)(sid, 0);
        return 1;
    }
}
