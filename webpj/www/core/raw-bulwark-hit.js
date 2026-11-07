// 불워크의 48ef20 피해 관문과 일반 Damageable 호출(44c8e0)을 원본 순서로 연결하는 모듈이다(cpppj `o/RawBulwarkHit.cpp/.h`, 10.78 전용).
// 공격자가 0 이면 곧바로 일반 피해로 넘기고, 아니면 공격자 슬롯의 현재 TYPE(+10) 가 가진 flags2 0x100000(비행체 genus)이면 피해 없이 0 을 돌려준다(벌룬 20000 이나 genus 이름으로 대체하지 않음).
// 원본은 공격자의 free/dead/extra/owner 를 검사하지 않고 현재 TYPE 의 비행 속성만 읽는다. 관문 통과 뒤 일반 피해의 HP/방어/소유자 통지/삭제/연소 처리를 그대로 공유하며 반환 DWORD 를 좁히지 않는다.
// (2026-11-08 WB04-E11: 불워크 피해 관문 이식으로 추가)

// 비행체 genus 를 나타내는 TYPE flags2 비트이다. (2026-11-08 WB04-E11)
const FLYER_GENUS = 0x100000;

// pool 은 SidPool, types 는 188칸 TYPE 레코드 표({ flags2 }), general(victim, amount, owner, attacker) 는 같은 일반 피해(44c8e0)이다. amount 는 signed 32비트이다. (2026-11-08 WB04-E11)
export function rawBulwarkHit(pool, types, victim, amount, owner, attacker, general) {
    if (pool?.edition !== 'patch1078' || types.length !== 188) throw new TypeError('불워크 피해 판본/현재 TYPE 표 오류');
    if (attacker) {
        const type = pool.slot(attacker)[10];
        if (type >= types.length) throw new RangeError('불워크 현재 공격자 TYPE 오류');
        // 비행체 genus 이며 벌룬 20000 이나 genus 이름으로 대체하지 않는다.
        if ((types[type].flags2 >>> 0) & FLYER_GENUS) return 0;
    }
    if (typeof general !== 'function') throw new Error('불워크 실제 일반 피해 미연결');
    return general(victim, amount | 0, owner >>> 0, attacker) >>> 0;
}
