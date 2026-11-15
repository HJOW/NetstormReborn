// 장식·가장자리 팜의 단계별 HP 감소와 마지막 삭제를 원문 462420 대로 옮긴 모듈이다(cpppj `o/RawDecorationRegular.cpp/.h`, 10.78 전용). 일반 간헐천이 아닌 장식/가장자리 팜의 수명 감소이다.
// 사용: const next = runDecorationRegular(pool, boss, parent, event, count, payload, { maxHitPoints(sid) → signed int32, hitPoints(sid, amount, owner0, attacker0), destroy(sid, flags0), damageable(sid, event, count, payload) → float });
//   event 0 일 때: signed count 가 3 이상이면 서버(boss)일 때만 현재 가상 +10 삭제(flags 0)를 요청하고 −1.0 을 반환한다. 아니면 최대 HP(4adcb0 의 현재 TYPE/제단 역할 조회)를 먼저 얻은 뒤 현재 슬롯의 signed HP(+26 DWORD)를 읽고,
//   최대 HP × (3 − count) 를 32비트로 감아 곱한 값의 0 방향 정수 /3 을 한계로 삼아 한계가 현재 HP 보다 작으면 가상 +98(amount = 현재 − 한계, owner 0, attacker 0)을 요청한다. 그 뒤(event 가 0 이 아니어도) 나머지는 실제 44b900 인 damageable 의 반환 payload 를 그대로 전달한다.
//   boss 는 호출 시점의 현재 서버 권한이며 boolean 또는 현재 값을 돌려주는 함수이다. 선택된 가상 하위는 도달한 위치에서만 요구하고 미연결이면 명시 거부한다(권한 없는 종료와 비0 사건은 최대 HP/삭제/피격을 앞당기지 않는다).
// 호출자는 10.78 SidPool 만 넘겨야 하며 CD 10.72 풀은 거부한다. 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E527: 4-D 구획 RawDecorationRegular 이식)

// 선택된 가상 하위만 요구한다. 콜백 중 교체해도 현재 함수 사본의 수명을 유지한다. (2026-11-15 WB04-E527)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('장식 Regular의 선택된 실제 하위 미연결');
    return hook;
}

// 원문의 현재 signed DWORD HP(+26)를 비정렬 raw 에서 읽는다. (2026-11-15 WB04-E527)
function health(pool, sid) {
    const raw = pool.slot(sid);
    return (raw[26] | (raw[27] << 8) | (raw[28] << 16) | (raw[29] << 24)) | 0;
}

// 호출 시점의 현재 서버 권한을 읽는다. (2026-11-15 WB04-E527)
const isBoss = (boss) => (typeof boss === 'function' ? Boolean(boss()) : Boolean(boss));

// 462420: signed count·DWORD 곱/감김·0 방향 나눗셈·현재 HP 읽기 순서를 보존한다. 반환은 float 이다. (2026-11-15 WB04-E527)
export function runDecorationRegular(pool, boss, parent, event, count, payload, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('장식462420은 PATCH10.78 본문입니다');
    if (!event) {
        if ((count | 0) >= 3) {
            if (isBoss(boss)) need(hooks.destroy)(parent, 0);
            return -1.0;
        }
        // 최대 HP 의 실제 효과 뒤 현재 HP/VT 를 읽는다.
        const maximum = need(hooks.maxHitPoints)(parent) | 0;
        const product = Math.imul(maximum, (3 - count) | 0) | 0;
        // 원문 multiply-high/부호 보정은 signed /3 의 0 방향 절삭이다.
        const threshold = Math.trunc(product / 3) | 0;
        const current = health(pool, parent);
        if (threshold < current) need(hooks.hitPoints)(parent, (current - threshold) | 0, 0, 0);
    }
    return need(hooks.damageable)(parent, event, count, payload);
}
