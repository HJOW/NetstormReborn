// 원본 10.78 의 여덟 파생 객체 메뉴(가상 +a8 본체)를 공통 객체/회수 계층 앞에 작성하는 모듈이다(cpppj `client/RawObjectMenuVariants.cpp/.h`, 10.78 전용).
// 사용:
//   const body = new RawObjectMenuVariants(pool, hooks);   // pool = SidPool(10.78). 생성 중에는 메뉴 효과를 실행하지 않는다.
//   body.open(sid, originalBody)   // 현재 +a8 본체 주소(0041c210 Meltdown·00422000 상태 변경·00462390 Dig Up/Turn To Spell·00462550 도움말·00485600 Storm Power·0048eca0 회수·004dcca0 AI·00420b30 Detonate/Obelisk)에 맞는 전체 파생을 작성한다.
//   hooks = { global(주소) → 원본 모드·로컬 소유자·명령 전역의 현재 값, localOwner(), maximumHp(sid), eventZero(sid), hasBrain(sid), count(sid), aiMenu(owner), translate(문자열), format(패턴, int32),
//             helpWord(), item(문자열, 명령, 인자, include, enabled), header(문자열), separator(), object(sid), damageable(sid), salvage(sid, 타입, 개수) }.
//   objectMenuVariantCommandValue(address) → 확인한 원문 전역 주소의 초기 명령 ID 또는 null / isObjectMenuVariantCommand(code) → 같은 표의 값이면 true.
// 번역/모드/소유자는 원문 접근 순서대로 다시 읽고(번역·가상 효과 뒤 재조회), 없는 파생 본체를 기본 메뉴로 바꾸지 않는다(미연결 Error). 슬롯은 접근 시점마다 현재 값을 읽는다.
// 원본 관측: `object-menu-variants-x86.tsv` 1,024입력(여덟 본체의 전체 외부 효과·인자·순서와 변경 조합)을 tests/raw-object-menu-variants.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E542: 파생 객체 메뉴 RawObjectMenuVariants 이식)

// 보호 PE 의 실제 초기 명령 DWORD(주소 식별자→ID)이며 호스트 함수 주소가 아니다. 조회는 첫 일치만 사용한다. (2026-11-15 WB04-E542)
const COMMANDS = Object.freeze([
    [0x542ad4, 0x030000f8], [0x542b5c, 0x02000125], [0x542b60, 0x02000126], [0x542b64, 0x02000127],
    [0x542a2c, 0x030000ce], [0x5427ac, 0x0200002d], [0x5428c8, 0x03000074], [0x542754, 0x02000017], [0x542974, 0x020000a0],
]);

// 선택된 필수 경계를 빈 성공으로 바꾸지 않는다. (2026-11-15 WB04-E542)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('파생 객체 메뉴 실제 하위 미연결');
    return hook;
}

// 확인한 원문 주소만 동일 초기 ID 로 돌려준다(게임 상태·통신 성공을 공급하지 않는다). (2026-11-15 WB04-E542)
export function objectMenuVariantCommandValue(address) {
    // 실제 첫 일치만 사용한다.
    for (const [source, code] of COMMANDS) if (source === (address >>> 0)) return code;
    return null;
}

// UI 명령 검증과 미연결 실행 진단에서 같은 원본 ID 표를 사용한다(주소는 비교하지 않는다). (2026-11-15 WB04-E542)
export function isObjectMenuVariantCommand(code) {
    // 같은 값의 주소 항목이 여럿이어도 허용 결과는 같다.
    for (const [, value] of COMMANDS) if (value === (code >>> 0)) return true;
    return false;
}

// 파생 가상 주소별 원문 분기를 보존하며 없는 파생을 기본 메뉴로 바꾸지 않는 처리기이다. (2026-11-15 WB04-E542)
export class RawObjectMenuVariants {
    // 같은 10.78 풀을 빌리며 판본을 혼합하거나 별도 슬롯 사본을 만들지 않는다.
    constructor(pool, hooks) {
        if (pool.edition !== 'patch1078') throw new TypeError('파생 객체 메뉴 10.78 풀 필요');
        this.pool = pool;
        this.hooks = hooks;
    }

    // free/dead 필터는 원본 파생 앞에 추가하지 않는다. 효과 뒤에도 현재 물리 슬롯을 다시 읽는다.
    raw(sid) {
        if (!sid || sid >= this.pool.capacity) throw new RangeError('파생 객체 메뉴 SID 범위');
        return this.pool.slot(sid);
    }

    // 정렬되지 않은 원본 바이트를 정확한 폭으로 결합한다(같은 현재 슬롯의 필요한 바이트만).
    read(sid, offset, width = 4) {
        const raw = this.raw(sid);
        let value = 0;
        // 낮은 바이트부터 조립한다.
        for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (index * 8);
        return value >>> 0;
    }

    // 선택한 분기에 필요한 현재 전역만 조회한다.
    global(address) { return need(this.hooks.global)(address) >>> 0; }

    // 원문 특수 접두 뒤 정확한 공통 부모만 실행한다. originalBody 는 현재 +a8 본체 주소이다.
    open(sid, originalBody) {
        const h = this.hooks;
        this.raw(sid);
        switch (originalBody >>> 0) {
        case 0x41c210:
            if (!this.global(0x594fa4) && this.raw(sid)[34] === (need(h.localOwner)() >>> 0)) {
                // HP 는 번역보다 먼저 보관한다.
                const hp = this.read(sid, 26) | 0;
                const label = need(h.translate)('Meltdown!');
                const maximum = need(h.maximumHp)(sid) | 0;
                const command = this.global(0x542ad4);
                need(h.item)(label, command, sid, 1, Math.trunc(maximum / 2) < hp ? 1 : 0);
            }
            need(h.damageable)(sid);
            return;
        case 0x422000:
            if (this.global(0x5c85a4)) {
                // 세 상태를 원문 번역/명령 조회 순서대로 작성한다.
                for (const [text, address] of [['Set as Cracked', 0x542b5c], ['Set as Normal', 0x542b60], ['Set as Hard', 0x542b64]]) {
                    const label = need(h.translate)(text);
                    const command = this.global(address);
                    need(h.item)(label, command, sid, 1, 1);
                }
            }
            need(h.object)(sid);
            return;
        case 0x462390:
            if (this.global(0x5c85a4)) need(h.item)('Turn To Spell!', this.global(0x542a2c), sid, 1, 1);
            {
                const include = this.global(0x55a480);
                const command = this.global(0x5427ac);
                need(h.item)('CHEAT Dig Up', command, sid, include, 1);
            }
            need(h.object)(sid);
            return;
        case 0x462550: {
            const type = this.raw(sid)[10];
            const command = this.global(0x542754);
            const label = need(h.helpWord)();
            need(h.item)(label, command, type, 1, 1);
            return;
        }
        case 0x485600: {
            const pattern = need(h.translate)('Storm Power: ~i%d~.\n');
            const text = need(h.format)(pattern, this.read(sid, 12, 2) | 0);
            need(h.header)(text);
            need(h.damageable)(sid);
            return;
        }
        case 0x48eca0: {
            const type = this.raw(sid)[10];
            const count = need(h.count)(sid);
            need(h.salvage)(sid, type, count);
            need(h.object)(sid);
            return;
        }
        case 0x4dcca0:
            if (need(h.hasBrain)(sid)) need(h.aiMenu)(this.raw(sid)[34]);
            if (this.raw(sid)[34] === this.global(0x540c70)) {
                const label = need(h.translate)('View Netstorm Knowledge');
                const command = this.global(0x542974);
                need(h.item)(label, command, 0, 1, 1);
            }
            need(h.separator)();
            need(h.damageable)(sid);
            return;
        case 0x420b30: {
            // 원본은 모드/소유자를 읽기 전에 예약 존재를 보관한다.
            const enabled = !need(h.eventZero)(sid);
            if (!this.global(0x55a480) && !this.global(0x5c85a4)) {
                const include = this.raw(sid)[34] === this.global(0x540c70);
                const command = this.global(0x5428c8);
                need(h.item)('Detonate!', command, sid, include ? 1 : 0, enabled ? 1 : 0);
            } else need(h.item)('Turn To Obelisk!', this.global(0x542a2c), sid, 1, enabled ? 1 : 0);
            need(h.object)(sid);
            return;
        }
        default:
            throw new Error(`현재 객체 메뉴의 실제 파생+a8 미연결: ${originalBody}`);
        }
    }
}
