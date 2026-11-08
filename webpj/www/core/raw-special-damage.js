// 신전(Vortex)의 건설 중 피해 분배(4dba40)와 윈드 타워의 방향 방어(48ef70)를 옮긴 모듈이다(cpppj `o/RawSpecialDamage.cpp/.h`, 10.78 전용).
//  - rawVortexDamage(pool, victim, damage, owner, attacker, hooks): 피해자 extra bit0(건설 중)이면 현재 건설 프로세스(30) 전역을 호출 시 읽어 같은 건설 Form 을 찾고(find(피해자, 건설 타입, 0, false)) 찾았을 때만 끝 시각을
//    피해 비트(DWORD)로 바꾸고(resetEnd) 같은 신전의 표시를 갱신(refresh)한다. 찾지 못해도 건설 중이면 항상 true 이다(피해는 지연되며 효과 없음). 건설이 끝난 신전은 기존 일반 피해(general)의 반환을 그대로 돌려준다.
//  - rawWindTowerDefense(pool, types, frames, victim, attacker, arcSpireType = 149): 공격자(0 도 현재 풀 슬롯의 TYPE/genus 로 읽음)의 flags2 에 0x100100(flyer/balloon 계열)이 있거나 공격자 TYPE 이 아크 스파이어면 방어하지 않는다.
//    아니면 피해자와 공격자의 발자국 중심(좌표 − 발자국/2, float 저장)을 비교한 네 방향(0 북/2 동/4 남/6 서; 동률은 세로 우선)이 피해자의 현재 프레임 코드의 방향 글자(L/M/N/O)와 일치할 때만 방어한다.
//    공격자 제외를 먼저 검사하며 제외되면 피해자 프레임을 읽지 않는다.
// hooks(신전) = { constructionType(), find(parent, type, index, remove)→sid, resetEnd(form, damageBits), refresh(sid), general(victim, damage, owner, attacker)→bool }, frames[type] 은 { codes: [{ side }] } 모양의 프레임 표이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E104: 신전/윈드 타워 특수 피해 이식으로 추가)
import { RawSquidNeighbors } from './raw-squid-neighbors.js';

// 슬롯의 TYPE(+10)·extra(+40)·프레임(+36)·좌표(+14/+18) 위치와 건설 중 extra 비트, 윈드 타워가 방어하지 않는 공격자 genus, 아크 스파이어 기본 TYPE 이다. (2026-11-08 WB04-E104)
const TYPE_AT = 10;
const EXTRA_AT = 40;
const FRAME_AT = 36;
const X_AT = 14;
const Y_AT = 18;
const CONSTRUCTING = 1;
const EXCLUDED_GENUS = 0x100100;
const ARC_SPIRE = 149;
// 윈드 타워 방어 방향 글자 L/M/N/O 의 문자 코드이다. (2026-11-08 WB04-E104)
const SIDE_NORTH = 0x4c;
const SIDE_EAST = 0x4d;
const SIDE_SOUTH = 0x4e;
const SIDE_WEST = 0x4f;
const fround = Math.fround;

// PATCH 전용 몸체에 다른 판본이나 무효 SID 를 섞지 않는다. (2026-11-08 WB04-E104)
function slot(pool, sid) {
    if (pool?.edition !== 'patch1078' || !sid || sid >= pool.capacity) throw new RangeError('특수 피해 판본/SID 오류');
    return pool.slot(sid);
}

// 비정렬 little endian DWORD 를 읽는다. (2026-11-08 WB04-E104)
const read = (raw, offset) => (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;

// 정렬되지 않은 단정도 좌표를 읽는다. (2026-11-08 WB04-E104)
function coordinate(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
}

// PATCH 4dba40: 건설 Form 이 없으면 원본처럼 효과 없이 true 이며, 찾은 뒤에만 끝 시각/표시를 차례로 호출한다. (2026-11-08 WB04-E104)
export function rawVortexDamage(pool, victim, damage, owner, attacker, hooks) {
    if (slot(pool, victim)[EXTRA_AT] & CONSTRUCTING) {
        if (!hooks.constructionType || !hooks.find) throw new Error('신전 피해 현재 건설 조회 미연결');
        const form = hooks.find(victim, hooks.constructionType(), 0, false);
        if (form) {
            if (!hooks.resetEnd || !hooks.refresh) throw new Error('신전 피해 건설 끝 시각/표시 미연결');
            hooks.resetEnd(form, damage >>> 0);
            hooks.refresh(victim);
        }
        return true;
    }
    if (!hooks.general) throw new Error('완공 신전 실제 일반 피해 미연결');
    return hooks.general(victim, damage | 0, owner, attacker);
}

// PATCH 48ef70: 현재 L/M/N/O 와 공격자 중심의 4방향을 비교하며 flyer/balloon/arcSpire 는 방어하지 않는다. (2026-11-08 WB04-E104)
export function rawWindTowerDefense(pool, types, frames, victim, attacker, arcSpireType = ARC_SPIRE) {
    if (pool?.edition !== 'patch1078' || attacker >= pool.capacity) throw new RangeError('윈드 타워 공격자 판본/SID 오류');
    const source = pool.slot(attacker);
    const type = source[TYPE_AT];
    if (type >= types.length) throw new RangeError('윈드 타워 공격자 TYPE 범위 오류');
    if ((types[type].flags2 & EXCLUDED_GENUS) || type === arcSpireType) return false;
    const target = slot(pool, victim);
    const targetType = target[TYPE_AT];
    if (targetType >= types.length || targetType >= frames.length) throw new RangeError('윈드 타워 방어자 TYPE 범위 오류');
    // 실제 4adea0 처럼 발자국 절반을 넓게 계산한 뒤 각 중심 성분을 float 로 저장한다.
    const center = (raw) => {
        const record = types[raw[TYPE_AT]];
        return [fround(coordinate(raw, X_AT) - record.footX * 0.5), fround(coordinate(raw, Y_AT) - record.footY * 0.5)];
    };
    const from = center(target);
    const to = center(source);
    const direction = RawSquidNeighbors.direction(from[0], from[1], to[0], to[1]);
    const number = read(target, FRAME_AT);
    const codes = frames[targetType].codes;
    if (number >= codes.length) throw new RangeError('윈드 타워 현재 프레임 범위 오류');
    const side = codes[number].side;
    return (side === SIDE_NORTH && direction === 0) || (side === SIDE_EAST && direction === 2) || (side === SIDE_SOUTH && direction === 4) || (side === SIDE_WEST && direction === 6);
}
