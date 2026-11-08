// 타입별 Canon 디코더 선택(원본 CanonDecoder 생성자 00425c20 의 타입 전역/패턴 번호 선택)과 다리 패턴 추첨(004257c0)을 옮긴 모듈이다(cpppj `o/CanonTypeDecoder.cpp/.h`·`CanonDecoder.cpp` 의 SelectBridgePattern, 10.78).
//  - decodeCanonType(frameCodes, defaultFrame, patternTypes, query, diagnostics): 타입 전역 8개(puzzlePiece, bridge, island, noIsland, thunderCannon, rainCannon, windArcher, windBlocker)를
//    앞 네 개부터 차례로 비교해(puzzlePiece → 영역 패턴 68개, bridge → 다리 패턴 26개, island → 섬 2개, noIsland → 받침 1개) 맞으면 그 패턴을, 뒤 네 개 중 하나와 같으면 공유 전투 패턴 1개를 고르고
//    query.argument 를 패턴 번호로 쓴다(0 미만이거나 표 크기 이상이면 거부). 어느 것도 아니면 패턴 없는 한 칸 경로이며 query.explicitFrame 이면 argument 를 프레임으로, 아니면 defaultFrame 을 쓴다.
//    패턴 타입에서는 explicitFrame 을 무시한다(타입 선택이 앞선다). 좌표가 유한하지 않으면 거부한다. 방향은 홀수도 받으며 회전은 방향÷2(0~3 밖이면 거부)이다.
//    반환은 territory.js 의 CanonDecoder 이며 생성자가 이미 첫 유효 칸으로 진행했다.
//  - canonSpecialPatterns(group): 섬(0, 2개)·받침(1, 1개)·공유 전투(2, 1개) 패턴 표이다. 다른 그룹은 거부한다.
//  - selectBridgePattern(random): 가중치 합(287)에 대한 signed 나머지를 누적 가중치와 `<=` 로 비교해 다리 패턴 번호를 고른다(나머지 0 또는 음수 합계 배수는 가중치 0 인 첫 항목).
// 10.72(CD) 경로(다리 표 306·홀수 방향 거부)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E136: 타입별 Canon 디코더 이식으로 추가)
import { CanonDecoder } from '../assets/territory.js';
import { TERRITORY_PATTERNS } from '../assets/territory-patterns.js';
import { BATTLE_PATTERNS, BRIDGE_PATTERNS, ISLAND_PATTERNS, SUPPORT_PATTERNS } from '../assets/canon-type-patterns.js';

// 타입 전역 순서에서 앞 네 개(영역/다리/섬/받침)의 자리와 전투 패턴을 고르는 뒤 네 개의 시작 자리, 전역 개수이다. (2026-11-08 WB04-E136)
const PUZZLE_PIECE = 0;
const BRIDGE = 1;
const ISLAND = 2;
const NO_ISLAND = 3;
const BATTLE_FIRST = 4;
const GLOBAL_COUNT = 8;
// 다리 패턴 가중치 합(10.78)이다. 생성 도구가 합을 확인한다. (2026-11-08 WB04-E136)
const BRIDGE_WEIGHT_TOTAL = 287;

// 특수 패턴 그룹 표이다(섬=0, 받침=1, 공유 전투=2). (2026-11-08 WB04-E136)
const SPECIAL_GROUPS = Object.freeze([ISLAND_PATTERNS, SUPPORT_PATTERNS, BATTLE_PATTERNS]);

// 복원된 특수 패턴 표를 돌려준다. 표 범위 밖 그룹은 원본의 잘못된 메모리 조회 대신 명시적으로 거부한다. (2026-11-08 WB04-E136)
export function canonSpecialPatterns(group) {
    if (!Number.isInteger(group) || group < 0 || group >= SPECIAL_GROUPS.length) throw new RangeError('CanonDecoder 특수 패턴 그룹 오류');
    return SPECIAL_GROUPS[group];
}

// 타입 번호와 타입 전역으로 패턴 표를 고른다. 비패턴 타입은 null 이다. 선행 비교 순서(앞 네 개 → 뒤 네 개)를 지킨다. (2026-11-08 WB04-E136)
function patternsFor(type, patternTypes) {
    if (type === patternTypes[PUZZLE_PIECE]) return TERRITORY_PATTERNS;
    if (type === patternTypes[BRIDGE]) return BRIDGE_PATTERNS;
    if (type === patternTypes[ISLAND]) return ISLAND_PATTERNS;
    if (type === patternTypes[NO_ISLAND]) return SUPPORT_PATTERNS;
    for (let index = BATTLE_FIRST; index < GLOBAL_COUNT; index += 1) {
        if (type === patternTypes[index]) return BATTLE_PATTERNS;
    }
    return null;
}

// 타입별 Canon 디코더를 만든다. frameCodes 는 그 타입의 프레임 코드 배열({ side, variant, number, flags }), patternTypes 는 타입 전역 8개의 현재 번호,
// query 는 { type, argument, direction, x, y, explicitFrame }, diagnostics 는 { checking() → DWORD, assertion(식, 파일, 줄) }(없으면 진단하지 않음)이다.
// 사용: const decoder = decodeCanonType(codes, 3, [107, 82, 94, 157, 131, 129, 140, 142], { type: 107, argument: 3, direction: 0, x: 20, y: 21 }); (2026-11-08 WB04-E136)
export function decodeCanonType(frameCodes, defaultFrame, patternTypes, query, diagnostics = null) {
    if (!Array.isArray(patternTypes) || patternTypes.length !== GLOBAL_COUNT) throw new TypeError('CanonDecoder 타입 전역은 8개여야 합니다.');
    if (!Number.isFinite(query.x) || !Number.isFinite(query.y)) throw new TypeError('CanonDecoder 좌표 오류');
    const patterns = patternsFor(query.type, patternTypes);
    if (patterns === null) {
        return new CanonDecoder(frameCodes, null, query.direction | 0, query.x, query.y, null, diagnostics,
            { defaultFrame, argument: query.argument, explicitFrame: Boolean(query.explicitFrame) });
    }
    const argument = query.argument | 0;
    if (argument < 0 || argument >= patterns.length) throw new RangeError('CanonDecoder 패턴 번호 오류');
    return new CanonDecoder(frameCodes, patterns[argument], query.direction | 0, query.x, query.y, null, diagnostics);
}

// 004257c0: 누적 가중치와 signed 나머지를 `<=` 로 비교한다. 원본은 r=0 에서 가중치 0 인 첫 항목을 고르며 음수 입력도 signed 나머지 그대로 첫 항목 쪽으로 간다. (2026-11-08 WB04-E136)
export function selectBridgePattern(random) {
    const value = (random | 0) % BRIDGE_WEIGHT_TOTAL;
    let accumulated = 0;
    // 누적 경계는 엄격한 < 가 아닌 <= 이다.
    for (let index = 0; index < BRIDGE_PATTERNS.length; index += 1) {
        accumulated += BRIDGE_PATTERNS[index].first;
        if (value <= accumulated) return index;
    }
    throw new Error('Bridge pattern weights');
}
