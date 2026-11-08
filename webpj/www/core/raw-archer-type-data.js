// 타입 초기화(.type 콜백)가 아처/크로스보우 소비자에게 공급하는 방향 수와 물리 프레임 수 읽기(원본 TYPE+110 / TYPE+114)를 옮긴 모듈이다(cpppj `o/RawArcherTypeData.cpp/.h`, 10.78 전용).
//  - readRawArcherTypeData(definition): TYPE+110(방향 수)은 초기값 0 을 유지하며 `orientations` 속성이 명시된 경우에만 숫자 AST 의 값을 float 폭으로 줄여 0 쪽으로 절삭해 덮어쓴다
//    (속성이 있는데 숫자가 아니면 — 예: 문자열 — 원본 계약 오류로 거부하고, 값이 비유한이거나 int32 범위를 벗어나면 CRT 정상 정수 범위 오류로 거부한다).
//    TYPE+114(물리 프레임 수)는 .type 클러스터 개수이다. 반환은 { directions, frameCount } 이다. 소비자(RawArcherAnimation.rotate / RawCrossbowAnimation.frame)의 `animationTypes[type]` 이 이 값이다.
// definition 은 parseTypeDefinition(assets/type-table.js)의 결과({ properties[{name,value}], clusters[] })이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E127: RawArcherTypeData 이식으로 추가)

// 속성 이름(대소문자 무시)이다. (2026-11-08 WB04-E127)
const ORIENTATIONS = 'orientations';
const INT32_MIN = -2147483648;
const INT32_MAX = 2147483647;

// 타입 정의에서 방향 수와 물리 프레임 수를 읽는다. 사용: const data = readRawArcherTypeData(parseTypeDefinition(text)); animationTypes[71] = data. (2026-11-08 WB04-E127)
export function readRawArcherTypeData(definition) {
    const matches = definition.properties.filter((property) => property.name.toLowerCase() === ORIENTATIONS);
    // 같은 속성이 여러 번 나오면 마지막 값이 이긴다(원본이 순서대로 덮어씀).
    const last = matches.length ? matches[matches.length - 1] : null;
    // 원본은 orientations 가 명시된 경우 숫자 AST 만 허용한다.
    if (last && typeof last.value !== 'number') throw new TypeError('Archer directions 숫자 AST 원본 계약 오류');
    // 누락된 방향 수는 원본 초기 0 이며 숫자는 원본 AST 의 float 폭으로 읽는다.
    const value = last ? Math.fround(last.value) : 0;
    if (!Number.isFinite(value) || value < INT32_MIN || value > INT32_MAX) throw new RangeError('Archer 방향수 CRT 정상 정수 범위 오류');
    if (definition.clusters.length > INT32_MAX) throw new RangeError('Archer 물리프레임 정수 범위 오류');
    return { directions: Math.trunc(value) | 0, frameCount: definition.clusters.length };
}
