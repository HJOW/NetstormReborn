// 자산의 가상 소유자 지정(+74) 재정의 중 생산 변경 접두와 고정 중립 소유자를 실제 현재 가상 표로 분배하는 함수(패치 00454cb0 과 00462580)를 옮긴 모듈이다(cpppj `o/RawAssetOwner.cpp/.h`, 10.78 전용).
// 순서(호출 때 읽은 현재 가상 표 기록값으로 고정): 고정 중립 자산(0x50a8e8)이면 현재 소유자 BYTE(+34)를 0 으로 쓰고 끝 → 생산 자산 두 종류(0x509148, 0x509210)이면 extra(+40)의 비트 9(0x9)가 모두 꺼져 있을 때 생산 목록 변경 카운터를 먼저 하나 올린 뒤
//   → 기존 섬 받침/종유석 소유자 분배(makeOwnerDispatch)와 공통 부모를 같은 순서로 부른다. 카운터를 먼저 올리므로 공통 소유자가 진단/목록 처리에서 중단돼도 그 변경은 남는다. 신전/사제는 상위 전용 분배기가 먼저 처리한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E71: 자산 소유자 분배 이식으로 추가)

import { makeOwnerDispatch } from './raw-bridge-connect.js';

// 가상 표 기록값(고정 중립 자산, 생산 자산 둘)과 슬롯의 소유자/extra 위치, 생산 변경 제외 비트이다. (2026-11-08 WB04-E71)
const NEUTRAL_TABLE = 0x50a8e8;
const PRODUCTION_TABLES = Object.freeze([0x509148, 0x509210]);
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const PRODUCTION_SKIP_BITS = 9;

// 정렬되지 않은 슬롯의 첫 DWORD 를 리틀엔디언으로 읽는다. (2026-11-08 WB04-E71)
function tableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// pool 은 10.78 SidPool, state 는 BridgeConnectState, productionDirty 는 실제 생산 목록 DWORD 를 빌리는 { value } holder(반환 함수보다 오래 살아야 함), base 는 공통 부모 (sid, owner) 이다. 반환은 (sid, owner) 분배 함수이다. (2026-11-08 WB04-E71)
export function makeAssetOwnerDispatch(pool, state, productionDirty, base) {
    // 기존 섬 받침/종유석과 공통 부모의 동일 순서를 재사용한다.
    const surfaces = makeOwnerDispatch(pool, state, base);
    return (sid, owner) => {
        // 호출할 때의 가상 표와 extra 를 읽고 생성 당시 타입으로 추정하지 않는다.
        const raw = pool.slot(sid);
        const table = tableOf(raw);
        if (table === NEUTRAL_TABLE) {
            pool.physical(sid)[OWNER_OFFSET] = 0;
            return;
        }
        if (PRODUCTION_TABLES.includes(table) && !(raw[EXTRA_OFFSET] & PRODUCTION_SKIP_BITS)) productionDirty.value = (productionDirty.value + 1) >>> 0;
        surfaces(sid, owner);
    };
}
