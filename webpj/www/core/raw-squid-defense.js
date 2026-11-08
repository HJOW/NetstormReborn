// 원문 기본 방어(가상 +0x9c) 몸체 0043d9b0 의 무조건 false 반환을 옮기고, 현재 raw 가상 표에 따라 그 몸체만 분배하는 모듈이다(cpppj `o/RawSquidDefense.cpp/.h`, 10.78 전용).
// 원문 두 명령(xor eax,eax; ret 4)은 this 와 공격 SID 를 읽지 않고 false 를 돌려준다. 분배는 현재 victim 슬롯의 +0 DWORD(가상 표)와 검증된 진입 표를 비교해 진입이 기본 몸체일 때만 false 를 돌려주고,
//   다른 파생 방어 몸체는 기존 rejectDamage 경계(fallback)로 보낸다. 표에 없는 표를 묵시적으로 false 로 바꾸지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E58: 객체 방어 이식으로 추가)

// 10.78 기본 방어 몸체의 진입 주소이다(원본 PE 에서 5바이트 몸체를 독립 실행해 대조한 값). (2026-11-08 WB04-E58)
const PATCH_DEFENSE = 0x43d9b0;

// 10.78 실제 가상 표와 +0x9c 진입의 대응이다(Factory 가 만드는 캠페인71/128/사제/anim 표). 호스트에서 역참조하지 않는 두 32비트 값이다. (2026-11-08 WB04-E58)
export const SQUID_DEFENSE_ENTRIES = Object.freeze([
    Object.freeze({ vtable: 0x501dd0, address: 0x43d9b0 }),
    Object.freeze({ vtable: 0x50b888, address: 0x43d9b0 }),
    Object.freeze({ vtable: 0x50b9b8, address: 0x43d9b0 }),
    Object.freeze({ vtable: 0x50f210, address: 0x43d9b0 }),
    Object.freeze({ vtable: 0x509758, address: 0x43d9b0 })
]);

// 실제 기본 몸체의 인자 무관 반환이며 다른 파생 가상 호출의 기본값으로 쓰지 않는다. (2026-11-08 WB04-E58)
export function rawSquidDefaultDefense() {
    return false;
}

// 현재 vtable 대응표에 없는 파생 방어를 묵시적으로 false 로 바꾸지 않는 rejectDamage 훅을 만든다. pool 은 SidPool, entries 는 { vtable, address } 목록,
//   hooks 는 기존 피해 훅 묶음({ rejectDamage(victim, attacker) 등 })이다. 반환은 hooks 의 사본에 분배 rejectDamage 를 얹은 묶음이다. (2026-11-08 WB04-E58)
export function makeRawSquidDefenseHooks(pool, entries, hooks = {}) {
    if (!Array.isArray(entries) || entries.length === 0) throw new Error('실제 기본 방어 가상 진입 표 누락');
    const fallback = hooks.rejectDamage;
    return {
        ...hooks,
        rejectDamage(victim, attacker) {
            if (!victim || victim >= pool.capacity) throw new RangeError('실제 방어 현재 SID 범위 오류');
            const raw = pool.slot(victim);
            const table = (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
            let address = 0;
            // 현재 raw 가상 표와 일치하는 첫 진입 하나만 선택한다.
            for (const entry of entries) {
                if (entry.vtable === table) {
                    address = entry.address;
                    break;
                }
            }
            if (address === PATCH_DEFENSE) return rawSquidDefaultDefense(victim, attacker);
            if (typeof fallback !== 'function') throw new Error('다른 파생 실제 방어 몸체 미복원');
            return fallback(victim, attacker);
        }
    };
}
