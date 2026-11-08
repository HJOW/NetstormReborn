// 윈드 소환 생산기의 원주 좌표 계산(41e020 / 특별 Wind 생산 12단계의 원주 위치)을 옮긴 모듈이다(cpppj `o/RawWindCircle.cpp/.h` 의 windCirclePoint, 10.78 전용).
//  - windCirclePoint(x, y, count, index): 원주를 count 등분(각 간격 = 360/count 정수 나눗셈)한 index+1 번째(index 는 DWORD 감김) 점이다. 각도는 `(360/count) × (index+1) × 3.141593 × 0.0055555556900799274(float)` 이며,
//    결과는 `(cos(각) × count + x, sin(각) × count + y)` 를 각각 float 로 저장한 두 값이다. count 가 0 이면 나눗셈 오류로 거부한다. 표준 pi 나 VFX 높이로 치환하지 않는다.
//  - windCirclePosition(pool, types, parent, flyer, fields, readHeader): 특별 Wind 생산 12단계의 실제 원주 위치 — 소유자 callback 뒤 현재 생성 비행체의 SHP 헤더(`readHeader(flyer, 추상 인수, 프레임 13)` → `{ walkStepY(+14 float), height(+1a), hotspotY(+1e) }`)를 먼저 읽고,
//    부모의 현재 중심(발자국 절반 보정)에서 원문 count(+0x44)·index(+0x48)의 원주 점을 구해 13번 높이(walkStepY × 0.09090909361839294)와 현재 발자국을 보정한다(288행 원주 프레임 관측과 함께 검증).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E121: RawWindCircle 의 원주 점 이식으로 추가, 2026-11-09 WB04-E185: 원주 위치 windCirclePosition 추가)

// 원본 502328 의 double pi 근사와 502320 의 float 도 변환 계수(float 로 저장된 값)이다. 원본 float 저장 경계마다 먼저 반올림한다. (2026-11-08 WB04-E121)
const PI = 3.141593;
const DEGREE_SCALE = Math.fround(0.0055555556900799274);

// 원주 위의 한 점을 구한다. 사용: const [px, py] = windCirclePoint(x, y, 6, 2). 입력 x,y 는 float 값이고 count/index 는 int32 이다. (2026-11-08 WB04-E121)
export function windCirclePoint(x, y, count, index) {
    if (!count) throw new RangeError('Wind 원주 개수 0 나눗셈');
    // 원본 add 의 하위 DWORD 감김이다.
    const next = ((index >>> 0) + 1) | 0;
    const angle = Math.trunc(360 / (count | 0)) * next * PI * DEGREE_SCALE;
    return [Math.fround(Math.cos(angle) * count + x), Math.fround(Math.sin(angle) * count + y)];
}

// 원본 50a380/504194 의 두 float 보정(생성 비행체 x 오프셋·높이 배율)이다. (2026-11-09 WB04-E185)
const FLYER_OFFSET = Math.fround(1.399999976158142);
const HEIGHT_SCALE = Math.fround(0.09090909361839294);

// 비정렬 DWORD 를 읽는다. (2026-11-09 WB04-E185)
function dword(raw, at) {
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E185)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 큰 효과 뒤 현재 종류의 발자국을 다시 조회한다. (2026-11-09 WB04-E185)
function currentType(pool, types, sid) {
    const kind = pool.slot(sid)[10];
    if (kind >= types.length) throw new RangeError('Wind 원주 현재 TYPE 범위');
    return types[kind];
}

// 헤더 뒤 부모의 현재 중심/원문 count·index 를 읽고 실제 13번 높이·현재 발자국을 보정한다. 사용: windCirclePosition(pool, types, 50, 60, fields, (sid, abstract, frame) => header). (2026-11-09 WB04-E185)
export function windCirclePosition(pool, types, parent, flyer, fields, readHeader) {
    if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || typeof readHeader !== 'function') throw new TypeError('Wind 원주 PATCH/188TYPE/현재 헤더 필요');
    // owner callback 뒤 현재 생성 비행체를 조회한다.
    const header = readHeader(flyer, pool.slot(flyer)[40] & 1, 13);
    const type = currentType(pool, types, parent);
    const raw = pool.slot(parent);
    const x = Math.fround(bitsFloat(dword(raw, 14)) - type.footX * 0.5);
    const y = Math.fround(bitsFloat(dword(raw, 18)) - type.footY * 0.5);
    const centerX = Math.fround(x + 1.0);
    const centerY = Math.fround(y + Math.fround(((header.height | 0) - (header.hotspotY | 0) + 1) | 0));
    const point = windCirclePoint(centerX, centerY, dword(fields.bytes, 68) | 0, dword(fields.bytes, 72) | 0);
    return [Math.fround(point[0] - FLYER_OFFSET), Math.fround(header.walkStepY * HEIGHT_SCALE - currentType(pool, types, parent).footY + point[1])];
}
