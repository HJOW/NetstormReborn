// 실제 TYPE 정의와 SHP 헤더 표에서 투사체 기하 입력 표(RawProjectileGeometry 의 geometry[n])를 만드는 연결 모듈이다
// (cpppj 세션이 자산 TYPE 마다 `projectileTypes[type]` 을 채우는 루프: `client/RawCampaignSession.cpp`·`tests/CampaignInstallSupport.h`, 10.78 전용).
//  - buildProjectileGeometryTypes(definitions, shapes, typeCount = 188): 자산 TYPE 은 70 번부터 정의 순서대로(TYPE 70 + 블록 번호) 이어지며 각 칸은
//    { defaultFrame, frameCount, directions, shotOffsets, heightExtension, range } 이다.
//      · defaultFrame = 정의의 기본 프레임(`defaultFrame`).
//      · frameCount / directions = TYPE+114 클러스터 수 / TYPE+110 방향 수(`readRawArcherTypeData`, 표시 hotspot 이나 SHP 프레임 수와 혼용하지 않음).
//      · shotOffsets = 그 TYPE 의 모든 물리 SHP 프레임 헤더의 마지막 두 signed WORD(+0x20/+0x22, 포구 픽셀)이다(`shapes[type].frames[i].muzzleX/muzzleY`; 표시 기준점 hotspot 이 아님).
//      · heightExtension = 정의의 숫자 `height` 를 0 쪽으로 절삭한 int32 이다(없거나 문자열이면 0, int32 밖은 x87 정수 불확정값 −2^31).
//      · range = 정의의 숫자 `range` 를 float 로 줄인 값이다(없거나 문자열이면 0). 방향 불일치 사정선 보정(`cannonRayEnd`)이 읽는 현재 사정거리다.
//    속성 이름은 대소문자를 구별하지 않고 같은 이름이 여러 번 나오면 마지막 값이 이긴다. 문자열 속성은 숫자로 바꾸지 않는다(원본 `Number()` 계약).
//    정의 개수가 typeCount − 70 을 넘으면 거부하고, 대응하는 shapes 칸이 없으면 거부한다. 정의가 없는 TYPE(0~69 와 뒤쪽)은 모두 0 인 빈 칸이다.
// definitions 는 `TypeTable.definitions`(parseTypeDefinition 결과 배열), shapes 는 `squidRendererShapes(table, database)` 결과이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E362: 7-P2 RawProjectileGeometry 의 실제 SHP 헤더 연결로 추가)
import { readRawArcherTypeData } from './raw-archer-type-data.js';

// 자산 TYPE 번호의 시작(정의 블록 0 = TYPE 70)과 10.78 의 TYPE 표 길이, x87 정수 변환의 불확정값이다. (2026-11-13 WB04-E362)
const ASSET_TYPE_BASE = 70;
const TYPE_COUNT = 188;
const INTEGER_INDEFINITE = -2147483648;

// 원본 TypeDefinition::Number: 대소문자 무시, 마지막 일치 속성, 숫자 AST 만 허용한다(없거나 문자열이면 undefined). (2026-11-13 WB04-E362)
function numberProperty(definition, name) {
    const wanted = name.toLowerCase();
    // 같은 속성의 마지막 지정값이 이기므로 뒤에서부터 찾는다.
    for (let index = definition.properties.length - 1; index >= 0; index -= 1) {
        const property = definition.properties[index];
        if (property.name.toLowerCase() === wanted) return typeof property.value === 'number' ? property.value : undefined;
    }
    return undefined;
}

// double 을 0 쪽으로 절삭한 int32 로 바꾼다(범위 밖·비유한은 x87 정수 불확정값). (2026-11-13 WB04-E362)
function truncateInteger(value) {
    if (!Number.isFinite(value) || value <= -2147483649 || value >= 2147483648) return INTEGER_INDEFINITE;
    return Math.trunc(value) | 0;
}

// 정의/SHP 표로 투사체 기하 표(길이 typeCount)를 만든다. 사용: new RawProjectileGeometry(pool, types, buildProjectileGeometryTypes(table.definitions, squidRendererShapes(table, database))). (2026-11-13 WB04-E362)
export function buildProjectileGeometryTypes(definitions, shapes, typeCount = TYPE_COUNT) {
    if (definitions.length > typeCount - ASSET_TYPE_BASE) throw new RangeError('투사체 기하 정의 개수가 TYPE 표를 넘습니다');
    const geometry = Array.from({ length: typeCount }, () => ({ defaultFrame: 0, frameCount: 0, directions: 0, shotOffsets: [], heightExtension: 0, range: 0 }));
    // 정의 블록마다 자기 TYPE(70 + 블록)의 칸을 채운다.
    for (let block = 0; block < definitions.length; block += 1) {
        const type = ASSET_TYPE_BASE + block;
        const definition = definitions[block];
        const shape = shapes[type];
        if (!shape || !Array.isArray(shape.frames)) throw new TypeError(`투사체 기하 SHP 표가 없습니다: TYPE ${type}`);
        const archer = readRawArcherTypeData(definition);
        geometry[type] = {
            defaultFrame: definition.defaultFrame,
            frameCount: archer.frameCount,
            directions: archer.directions,
            // 각 물리 프레임 헤더의 마지막 두 signed WORD 이다.
            shotOffsets: shape.frames.map((frame) => [frame.muzzleX, frame.muzzleY]),
            heightExtension: truncateInteger(numberProperty(definition, 'height') ?? 0),
            range: Math.fround(numberProperty(definition, 'range') ?? 0),
        };
    }
    return geometry;
}
