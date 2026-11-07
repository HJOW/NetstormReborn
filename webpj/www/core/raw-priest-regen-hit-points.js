// 사제 회복 HP 부모(4945ee)의 읽기 순서와 signed ADD 감김을 옮긴 모듈이다(cpppj `o/RawPriestRegenHitPoints.cpp/.h`, 10.78 경로).
// 순서: 현재 HP(current) → 현재 최대 HP(maximum) 읽기, 초기 HP 가 최대 이상이면 회복 없음(null) → 아니면 증분 계산(priestType 번호 읽기 → 현재 HP 재읽기 → 번호의 TYPE 최대 HP typeMaximum 조회 → 현재 HP + trunc(typeHp / 6) 를 32비트로 감김)을
//   한 번 해 후보를 만들고 현재 최대를 다시 읽어 후보가 크면 그 최대를 한 번 더 읽어 반환, 아니면 증분 계산을 한 번 더 해 그 값을 반환한다(Setter 에 넘길 signed DWORD).
// Setter/표시/공간 효과는 실제 회복 처리기가 이어서 실행한다. 회복이 없는 분기는 TYPE 장치를 요구하지 않고 장치 누락은 그 위치에서 거부한다. 10.78 경로만 옮겼다(CD 의 HP/번호 읽기 순서는 다름). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E15: 사제 회복 HP 부모 이식으로 추가)

// 선택된 실제 장치가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E15)
function need(device) {
    if (typeof device !== 'function') throw new Error('회복 HP 부모의 선택된 실제 장치 미연결');
    return device;
}

// hooks = { current() → signed HP, maximum() → signed 최대 HP, priestType() → 번호, typeMaximum(번호) → TYPE 최대 HP }. 반환은 signed 32비트 수 또는 회복 없음(null)이다. (2026-11-08 WB04-E15)
export function readPriestRegenHitPoints(hooks) {
    const initial = need(hooks.current)() | 0;
    const maximum = need(hooks.maximum)() | 0;
    if (initial >= maximum) return null;
    // 두 분기 모두 번호를 재조회하며 번호 자체는 HP 읽기 전에 고정한다.
    const increment = () => {
        const number = need(hooks.priestType)();
        const current = need(hooks.current)() | 0;
        const typeHp = need(hooks.typeMaximum)(number) | 0;
        // 원문 ADD 의 하위 32비트이다.
        return (current + Math.trunc(typeHp / 6)) | 0;
    };
    const candidate = increment();
    const currentMaximum = need(hooks.maximum)() | 0;
    if (candidate > currentMaximum) return need(hooks.maximum)() | 0;
    return increment();
}
