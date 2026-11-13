// 원본 기본 프레임/빈 추가 그림(+a4 = 41c100)을 공유하는 유닛의 정확한 (TYPE, 가상표) 쌍을 판별하는 모듈이다(cpppj `o/RawPlainUnitCapture.cpp/.h`, 10.78 전용).
//  - isPlainUnitCapture(raw): 슬롯 길이가 50 이상이고 TYPE 바이트(+10)와 비정렬 little endian 가상표 DWORD(+0)가 표의 한 쌍과 둘 다 일치할 때만 참이다.
//    타입 번호만 맞고 가상 파생이 다른 슬롯은 이 빈 추가 그림으로 대신하지 않는다.
// 보호 PE 에서 +78=4acfe0, +a0=41c0f0, +a4=41c100 을 각각 확인한 표이며, 운반/비행/일반 유닛과 같은 빈 추가 그림인 애시드 145 의 실제 표를 포함한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E372: 5-A 착수의 첫 `o/` 단위로 추가)

// 일반 유닛 열세 개의 (TYPE, 원본 가상표) 쌍이다. 순서는 원본 표와 같다. (2026-11-13 WB04-E372)
const UNITS = Object.freeze([
    [70, 0x508cc8], [73, 0x50a1e8], [111, 0x5040c0], [133, 0x50e568], [135, 0x503e50],
    [137, 0x503d80], [138, 0x50a050], [141, 0x5040c0], [144, 0x50a388], [145, 0x50e738],
    [146, 0x504198], [156, 0x503f20], [158, 0x50f210],
]);

// 슬롯의 최소 길이(필드 위치 +10 까지와 기록 크기 확인)이다. (2026-11-13 WB04-E372)
const MIN_LENGTH = 50;

// 슬롯 앞 네 바이트를 비정렬 little endian 가상표 DWORD 로 읽는다(부호 없음). (2026-11-13 WB04-E372)
function vtableOf(raw) {
    let table = 0;
    // 낮은 바이트부터 현재 네 바이트만 합친다.
    for (let index = 0; index < 4; index += 1) table += raw[index] * 2 ** (index * 8);
    return table;
}

// 이 슬롯이 일반 유닛 캡처(공통 빈 추가 그림)를 쓰는 정확한 TYPE/가상표 쌍인지 판별한다. 사용: isPlainUnitCapture(pool.slot(sid)). (2026-11-13 WB04-E372)
export function isPlainUnitCapture(raw) {
    if (raw.length < MIN_LENGTH) return false;
    const table = vtableOf(raw);
    // 정확한 두 필드를 함께 비교한다.
    for (const [type, value] of UNITS) if (raw[10] === type && table === value) return true;
    return false;
}

export { vtableOf };
