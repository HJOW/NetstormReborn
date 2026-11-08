// 영역(Territory) 배치 모듈이다: `.fort` 의 `Territory` 레코드(6바이트 × 20)를 해석해 영역마다 월드 청크 격자(16 × 16)의 어느 청크를 차지하는지 정하고,
// `TerrNN` 섹션에 저장된 청크 레코드를 그 월드 청크에 짝짓는다. 지형 생성·오브젝트 그리기가 월드 좌표를 얻으려면 이 배치가 먼저 필요하다.
// 복원 기준: cpppj `o/CanonDecoder.cpp`(패턴 풀기: 원본 00425c20·00425860·00425700·00425c00), `o/ChunkMap.cpp`(청크 지도·섬 목록: 00432180·00432210·0046ee60·0046f200),
// `o/Islandbuilder.cpp`(0046dba0 패턴 놓기·0046dd70 전체 놓기·004be2c0 영역 청크 순회)의 10.78 패치판 경로이고 근거 문서는 docs/exe/territory-layout.md 이다.
// 범위: 미션·요새를 읽는 경로(저장된 위치를 그대로 쓰며 기준 사각형의 시작점이 (1,1))이다. 일반 전투의 플레이어별 재배치, 요새 편집 경로(시작점 (6,6)), 섬 목록의
// 나머지 필드는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// 규칙 요약:
//  - 레코드: [0] 하위 6비트 모양 번호·상위 2비트 방향 값, [1] 플래그(bit 0 이 켜지고 bit 1 이 꺼져야 놓음), [2] 청크 위치(하위 4비트 x, 상위 4비트 y), [3~5] 보존만 한다.
//  - 패턴(TERRITORY_PATTERNS)의 칸마다 방향 글자('.' 은 빈 칸)·변형 숫자·번호 글자가 있고, 놓을 때 회전(방향 값 ÷ 2)에 따라 글자를 바꿔 `puzzlePiece` 타입의 프레임 코드에서
//    (글자, 변형 'P', 변형 숫자)와 같은 프레임을 찾는다. 찾지 못한 칸은 원본처럼 건너뛴다.
//  - 영역은 번호순으로 놓으며 나중에 놓은 영역이 겹친 청크를 차지한다. 저장 청크 레코드는 월드를 y 바깥·x 안쪽으로 훑을 때 그 영역에 속한 청크의 순서와 같다.
// (2026-11-01 WB03-V: 정적 맵 화면(M2) 2단계로 추가)
import { TERRITORY_PATTERNS } from './territory-patterns.js';
import { TERRITORY_COUNT, WORLD_CHUNKS_PER_SIDE, CHUNK_CELLS } from './fort.js';
import { TYPE_LOAD_ORDER } from './type-table.js';

// `Territory` 섹션의 레코드 하나의 바이트 수이다(원본 004bd660). (2026-11-01 WB03-V)
export const TERRITORY_RECORD_BYTES = 6;

// 섬이 없는 청크의 섬 번호이다(원본 DAT_0050c038 = 0x7f). (2026-11-01 WB03-V)
export const NO_ISLAND = 0x7f;

// 영역을 놓을 수 있는 청크 범위의 기본값이다: (1,1)~(15,15)(원본 FUN_00431fc0 이 돌려주는 값 그대로). 영역 위치 바이트는 이 사각형의 왼쪽 위(left, top)에서부터 센다. (2026-11-01 WB03-V)
export const DEFAULT_CHUNK_RECT = Object.freeze({ left: 1, top: 1, right: 15, bottom: 15 });

// 레코드 첫 바이트에서 모양 번호를 뽑는 마스크(하위 6비트)와 방향 값을 뽑는 시프트(상위 2비트)이다. (2026-11-01 WB03-V)
const SHAPE_MASK = 0x3f;
const DIRECTION_SHIFT = 6;

// 레코드 둘째 바이트의 플래그: bit 0 이 켜지고 bit 1 이 꺼져야 영역을 놓는다. (2026-11-01 WB03-V)
const ACTIVE_FLAG = 0x01;
const SKIP_FLAG = 0x02;

// 패턴 칸의 글자 규칙이다: 빈 칸 글자, 방향 글자의 범위, 번호·변형 글자의 기준, 변형 숫자의 최댓값(원본 assert "0 <= variation && variation <= 10"). (2026-11-01 WB03-V)
const EMPTY_SIDE = '.';
const FIRST_SIDE = 'A';
const LAST_SIDE = 'P';
const LABEL_BASE = 0x61;
const VARIATION_BASE = 0x30;
const MAX_VARIATION = 10;

// 회전표의 방향 글자 열 수이다. 열 15(글자 'P' 자리)는 프레임을 찾을 때 변형 글자로 쓴다(원본 `(&DAT_005315cc)[회전 * 0x40]` = 16번째 글자). (2026-11-01 WB03-V)
const VARIANT_COLUMN = 15;

// 회전 r(0~3, 시계 방향 90도 단위)마다 패턴을 읽는 규칙이다(원본 0x531530~0x531580). 바깥 반복(줄)이 끝날 때 패턴 좌표에 더하는 값, 안쪽 반복(칸)마다 더하는 값,
// 시작 패턴 좌표를 정하는 배율(시작 x = (폭 - 1) × START_X, 시작 y = (높이 - 1) × START_Y)이다. (2026-11-01 WB03-V)
const OUTER_STEP_X = Object.freeze([0, 1, 0, -1]);
const OUTER_STEP_Y = Object.freeze([1, 0, -1, 0]);
const INNER_STEP_X = Object.freeze([1, 0, -1, 0]);
const INNER_STEP_Y = Object.freeze([0, -1, 0, 1]);
const START_X = Object.freeze([0, 0, 1, 1]);
const START_Y = Object.freeze([0, 1, 1, 0]);

// 회전 r 에서 패턴의 방향 글자 A~P 가 바뀌는 글자이다(원본 0x531590, 16글자씩 4줄). (2026-11-01 WB03-V)
export const ROTATED_SIDES = Object.freeze(['ABCDEFGHIJKLMNOP', 'ACDEBGHIFKJMNOLP', 'ADEBCHIFGJKNOLMP', 'AEBCDIFGHKJOLMNP']);

// 프레임 코드 목록에서 (방향 글자, 변형 글자, 번호)가 모두 같은 첫 프레임을 찾는다. 없으면 -1 이다(원본 0049a9a0). codes 는 { side, variant, number } 배열이다.
// 사용: findFrameNumber(pieceFrameCodes(table), 0x41, 0x50, 1). (2026-11-01 WB03-V)
export function findFrameNumber(codes, side, variant, number) {
    // 첫 일치를 돌려주므로 중복 프레임이 있으면 앞의 것이 이긴다.
    for (let i = 0; i < codes.length; i += 1) {
        const code = codes[i];
        if (code.side === side && code.variant === variant && code.number === number) return i;
    }
    return -1;
}

// 타입 표에서 영역 패턴 타입(`puzzlePiece`)의 프레임 코드 목록을 만든다. 영역 패턴을 풀 때 각 칸의 프레임을 이 목록에서 찾는다. 정의를 읽지 못했으면 오류이다.
// 사용: pieceFrameCodes(TypeTable.fromArchive(archive, decodeOriginalText)). (2026-11-01 WB03-V)
export function pieceFrameCodes(typeTable) {
    const definition = typeTable.definitions[TYPE_LOAD_ORDER.indexOf('puzzlePiece')];
    if (!definition) throw new Error('puzzlePiece 타입 정의를 읽지 못했습니다.');
    return definition.clusters.map((cluster) => cluster.code);
}

// 영역 패턴 하나를 회전(방향 값 ÷ 2)을 반영해 한 칸씩 풀어 준다(원본 CanonDecoder 00425c20·00425860). 유효한 칸마다 { frame, x, y, label, side } 를 낸다:
// 칸 좌표는 (originX + 안쪽 번호, originY + 바깥 번호), side 는 그 프레임 코드의 방향 글자(회전이 반영된 글자), label 은 번호 글자에서 'a' 를 뺀 값(없으면 -0x61)이다.
// 빈 칸('.')과 프레임을 찾지 못한 칸은 건너뛰며 후자는 onMissingFrame() 으로 알린다. 방향 값이 회전 0~3 을 벗어나면 오류이다.
// 사용: for (const cell of decodeTerritoryPattern(TERRITORY_PATTERNS[7], 0, 10, 6, codes)) …. (2026-11-01 WB03-V)
export function* decodeTerritoryPattern(pattern, direction, originX, originY, frameCodes, onMissingFrame = null) {
    const rotation = Math.trunc(direction / 2);
    if (!(rotation >= 0 && rotation <= 3)) throw new RangeError('CanonDecoder rotation');
    const { width, height } = pattern;
    // 회전이 홀수면 폭과 높이를 바꿔 센다.
    const outerCount = (rotation & 1) !== 0 ? width : height;
    const innerCount = (rotation & 1) !== 0 ? height : width;
    let patternX = (width - 1) * START_X[rotation];
    let patternY = (height - 1) * START_Y[rotation];
    let outer = 0;
    let inner = 0;
    // 줄(바깥 반복)과 칸(안쪽 반복)을 끝까지 훑는다.
    for (;;) {
        if (inner >= innerCount) {
            // 한 줄이 끝났다: 패턴 좌표를 바깥 방향으로 한 칸 옮기고 다음 줄로 간다.
            if (OUTER_STEP_X[rotation] !== 0) patternX = (OUTER_STEP_X[rotation] + outerCount + patternX) % outerCount;
            if (OUTER_STEP_Y[rotation] !== 0) patternY = (OUTER_STEP_Y[rotation] + outerCount + patternY) % outerCount;
            outer += 1;
            if (outer >= outerCount) return;
            inner = 0;
        }
        // 셀 해석(원본 00425700): 줄 우선 배열에서 (패턴 x, 패턴 y)의 칸.
        const index = width * patternY + patternX;
        if (index < 0 || index >= pattern.sides.length) throw new RangeError('패턴 칸 번호가 범위를 벗어났습니다.');
        const labelChar = pattern.labels[index];
        const labelCode = labelChar === '-' ? 0 : labelChar.charCodeAt(0);
        if (labelCode !== 0 && (labelCode < LABEL_BASE || labelCode > 0x7a)) throw new Error("num==0 || ('a' <= num && num <= 'z')");
        const orientation = pattern.sides[index];
        let frame = -1;
        if (orientation !== EMPTY_SIDE) {
            if (orientation < FIRST_SIDE || orientation > LAST_SIDE) throw new Error("'A' <= orientation && orientation <= 'P'");
            const variation = pattern.variations.charCodeAt(index) - VARIATION_BASE;
            if (variation < 0 || variation > MAX_VARIATION) throw new Error('0 <= variation && variation <= 10');
            // 회전에 따라 방향 글자를 바꿔 그 방향·변형·번호의 프레임을 찾는다(원본 FUN_0049a940).
            const rotated = ROTATED_SIDES[rotation];
            frame = findFrameNumber(frameCodes, rotated.charCodeAt(orientation.charCodeAt(0) - FIRST_SIDE.charCodeAt(0)), rotated.charCodeAt(VARIANT_COLUMN), variation);
            if (frame === -1 && onMissingFrame) onMissingFrame();
        }
        const x = originX + inner;
        const y = originY + outer;
        // 패턴 좌표를 안쪽 방향으로 한 칸 옮긴다.
        if (INNER_STEP_X[rotation] !== 0) patternX = (INNER_STEP_X[rotation] + innerCount + patternX) % innerCount;
        if (INNER_STEP_Y[rotation] !== 0) patternY = (INNER_STEP_Y[rotation] + innerCount + patternY) % innerCount;
        inner += 1;
        if (frame !== -1) yield { frame, x, y, label: labelCode - LABEL_BASE, side: String.fromCharCode(frameCodes[frame].side) };
    }
}

// 금 간 프레임 코드 비트(FF_CRACKED)와 그 원문 진단 위치이다(10.78 Canondecoder.cpp 0x325). (2026-11-08 WB04-E57)
const CRACKED_FLAG = 0x20;
const CANON_FILE = '\\Ns\\O\\Canondecoder.cpp';
const CRACKED_EXPRESSION = 'frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)';
const CRACKED_LINE = 0x325;
// 반복자 사본을 만드는 내부 토큰이다(원본 C++ 의 값 복사: 생성자의 첫 진행을 다시 하지 않고 현재 상태를 그대로 복사). (2026-11-09 WB04-E138)
const CLONE = Symbol('CanonDecoder.clone');
// 패턴 없는 한 칸 경로에서 명시 프레임이 프레임 표 밖일 때의 원문 진단(10.78 Canondecoder.cpp 0x305)이다. (2026-11-08 WB04-E136)
const SINGLE_FRAME_EXPRESSION = '0 <= frame && frame < ::getType(type)->numFrames';
const SINGLE_FRAME_LINE = 0x305;

// 원본 CanonDecoder 반복자 객체(00425c20 생성·00425860 다음 칸·00425b90 범위)의 영역/다리 패턴 경로이다(cpppj `o/CanonDecoder` 의 패턴 생성자·Advance·Bounds, 10.78).
// 생성자가 첫 유효 칸까지 한 번 진행한다(그 앞 칸의 프레임 누락만 onMissingFrame 으로 알린다). checking 이 켜졌으면 고른 프레임 코드의 금 간 비트를 진단(0x325)하고 복귀하면 계속한다.
// bounds(footX, footY) 는 현재 좌표의 절삭에서 순회 개수 × 현재 발자국을 뺀 [left, top, right, bottom] 정수이다(유효 칸이 없어도 마지막 좌표로 계산).
// frameCodes 는 { side, variant, number, flags } 배열, diagnostics 는 { checking() → DWORD, assertion(식, 파일, 줄) } 이다(없으면 진단하지 않음).
// 사용: new CanonDecoder(codes, TERRITORY_PATTERNS[3], 0, 0, 0, () => missing += 1).bounds(1, 1). (2026-11-08 WB04-E57: Player 간헐천 범위의 패턴 폭/높이 계산과 함께 추가)
// 패턴이 null 이고 마지막 인자에 { defaultFrame, argument, explicitFrame } 을 주면 패턴 없는 한 칸 경로(비패턴 타입)이다. 타입 전역으로 패턴을 고르는 생성은 core/raw-canon-type.js 의 decodeCanonType 이다. (2026-11-08 WB04-E136)
export class CanonDecoder {
    // 그 타입의 프레임 코드 표이다.
    #codes;
    // 칸별 방향·변형·라벨을 펼칠 패턴이다.
    #pattern;
    // 패턴 회전(방향 값 ÷ 2, 0~3)이다.
    #rotation;
    // 프레임을 찾지 못한 칸을 알리는 장치(없으면 null)이다.
    #onMissingFrame;
    // 현재 checking getter 와 원문 진단 장치 묶음(없으면 null)이다.
    #diagnostics;
    // 첫 칸의 좌표(float)이다.
    #originX;
    #originY;
    // 바깥/안쪽 반복의 칸 개수와 현재 패턴 좌표·반복 번호이다.
    #outerCount;
    #innerCount;
    #patternX;
    #patternY;
    #outer = 0;
    #inner = 0;
    // 현재 칸의 프레임 번호(-1=없음)·남은 칸 여부·라벨·좌표(float)이다.
    #frame = -1;
    #valid = true;
    #label = 0;
    #x = 0;
    #y = 0;

    // 패턴 없는 타입의 한 칸 경로 입력이다: { defaultFrame, argument, explicitFrame }. 패턴 경로에서는 null 이다. (2026-11-08 WB04-E136)
    #single = null;

    // 패턴 경로의 반복자를 만들고 첫 유효 칸으로 진행한다. 회전이 0~3 밖이면 오류이다.
    // pattern 이 null 이면 패턴 없는 한 칸 경로(00425c20 의 비패턴 분기)이다: single = { defaultFrame, argument, explicitFrame } 이 필요하며 회전은 0, 폭·높이는 1이고,
    // 명시 프레임이 아니면 기본 프레임을, 명시 프레임이면 argument 를 프레임으로 쓴다(프레임 표 밖이면 원문 진단 0x305). 좌표가 유한하지 않으면 거부한다. (2026-11-08 WB04-E136: 비패턴 경로 추가)
    constructor(frameCodes, pattern, direction, x, y, onMissingFrame = null, diagnostics = null, single = null) {
        if (frameCodes === CLONE) {
            // pattern 인자에 원본 반복자를 받아 모든 상태를 복사한다. 진단·콜백 참조는 같은 것을 가리킨다.
            const source = pattern;
            this.#codes = source.#codes;
            this.#pattern = source.#pattern;
            this.#single = source.#single;
            this.#rotation = source.#rotation;
            this.#onMissingFrame = source.#onMissingFrame;
            this.#diagnostics = source.#diagnostics;
            this.#originX = source.#originX;
            this.#originY = source.#originY;
            this.#outerCount = source.#outerCount;
            this.#innerCount = source.#innerCount;
            this.#patternX = source.#patternX;
            this.#patternY = source.#patternY;
            this.#outer = source.#outer;
            this.#inner = source.#inner;
            this.#frame = source.#frame;
            this.#valid = source.#valid;
            this.#label = source.#label;
            this.#x = source.#x;
            this.#y = source.#y;
            return;
        }
        if (pattern === null) {
            if (!single || !Number.isFinite(x) || !Number.isFinite(y)) throw new TypeError('패턴 없는 CanonDecoder 좌표 오류');
            this.#single = { defaultFrame: single.defaultFrame | 0, argument: single.argument | 0, explicitFrame: Boolean(single.explicitFrame) };
            this.#rotation = 0;
            this.#codes = frameCodes;
            this.#pattern = null;
            this.#onMissingFrame = onMissingFrame;
            this.#diagnostics = diagnostics;
            this.#originX = Math.fround(x);
            this.#originY = Math.fround(y);
            this.#outerCount = 1;
            this.#innerCount = 1;
            this.#patternX = 0;
            this.#patternY = 0;
            this.advance();
            return;
        }
        this.#rotation = Math.trunc(direction / 2);
        if (!(this.#rotation >= 0 && this.#rotation <= 3)) throw new RangeError('CanonDecoder rotation');
        this.#codes = frameCodes;
        this.#pattern = pattern;
        this.#onMissingFrame = onMissingFrame;
        this.#diagnostics = diagnostics;
        this.#originX = Math.fround(x);
        this.#originY = Math.fround(y);
        const { width, height } = pattern;
        // 회전이 홀수면 폭과 높이를 바꿔 센다.
        this.#outerCount = (this.#rotation & 1) !== 0 ? width : height;
        this.#innerCount = (this.#rotation & 1) !== 0 ? height : width;
        this.#patternX = (width - 1) * START_X[this.#rotation];
        this.#patternY = (height - 1) * START_Y[this.#rotation];
        this.advance();
    }

    // 00425b90: 현재 좌표 절삭에서 순회 개수 × 발자국을 뺀 미리보기 사각형이다. 정수 범위를 벗어나면 오류이다.
    bounds(footX, footY) {
        const x = Math.trunc(this.#x);
        const y = Math.trunc(this.#y);
        const left = x - this.#innerCount * footX;
        const top = y - this.#outerCount * footY;
        // 원본 int 산술/CRT 범위를 벗어나는 입력은 별도 계약으로 거부한다.
        for (const value of [left, top, x, y]) if (!Number.isFinite(value) || value < -2147483648 || value > 2147483647) throw new RangeError('CanonDecoder 범위 산술 오류');
        return [left | 0, top | 0, x | 0, y | 0];
    }

    // 현재 상태를 그대로 복사한 반복자를 만든다(C++ 값 복사와 같음: 생성자의 첫 진행과 진단을 다시 하지 않는다). 남은 칸 수를 세어 보는 등 원 반복자를 건드리지 않는 순회에 쓴다. (2026-11-09 WB04-E138)
    clone() { return new CanonDecoder(CLONE, this); }

    // 아직 칸이 남았는가이다.
    get valid() { return this.#valid; }

    // 현재 칸의 프레임 번호이다.
    get frame() { return this.#frame; }

    // 현재 칸의 좌표(float)이다.
    get x() { return this.#x; }
    get y() { return this.#y; }

    // 현재 칸의 번호 글자에서 'a' 를 뺀 값이다(번호 없으면 -0x61).
    get label() { return this.#label; }

    // 현재 프레임 코드의 방향 글자(코드)이다. 물리 표 밖은 오류이다.
    get side() {
        if (this.#frame < 0 || this.#frame >= this.#codes.length) throw new RangeError('CanonDecoder 방향 조회 물리 프레임 범위 오류');
        return this.#codes[this.#frame].side;
    }

    // 선택한 원문 진단을 보고한다. 장치가 없으면 그 위치에서 오류이며 복귀를 성공으로 합성하지 않는다.
    #diagnose(expression, line) {
        const assertion = this.#diagnostics?.assertion;
        if (typeof assertion !== 'function') throw new RangeError(`CanonDecoder 원문 진단 장치 미연결 ${line}`);
        assertion(expression, CANON_FILE, line);
    }

    // 00425860: 다음 유효 칸으로 간다. 빈 칸과 프레임을 찾지 못한 칸은 건너뛴다.
    advance() {
        const rotation = this.#rotation;
        const pattern = this.#pattern;
        // 프레임이 있는 칸을 만날 때까지 간다.
        for (;;) {
            if (this.#inner >= this.#innerCount) {
                // 한 줄이 끝났다: 패턴 좌표를 바깥 방향으로 한 칸 옮기고 다음 줄로 간다.
                if (OUTER_STEP_X[rotation] !== 0) this.#patternX = (OUTER_STEP_X[rotation] + this.#outerCount + this.#patternX) % this.#outerCount;
                if (OUTER_STEP_Y[rotation] !== 0) this.#patternY = (OUTER_STEP_Y[rotation] + this.#outerCount + this.#patternY) % this.#outerCount;
                this.#outer += 1;
                if (this.#outer >= this.#outerCount) {
                    this.#valid = false;
                    return;
                }
                this.#inner = 0;
            }
            this.#frame = -1;
            if (pattern === null) {
                // 패턴 없는 한 칸 경로: 기본 프레임 또는 명시 프레임이다. 명시 프레임이 표 밖이면 원문 진단(복귀하면 그 값으로 계속, 물리 표 밖 조회는 side 에서 거부).
                this.#frame = this.#single.explicitFrame ? this.#single.argument : this.#single.defaultFrame;
                if (this.#single.explicitFrame && (this.#frame < 0 || this.#frame >= this.#codes.length)) this.#diagnose(SINGLE_FRAME_EXPRESSION, SINGLE_FRAME_LINE);
            } else {
            // 셀 해석(00425700): 줄 우선 배열에서 (패턴 x, 패턴 y)의 칸.
            const index = pattern.width * this.#patternY + this.#patternX;
            if (index < 0 || index >= pattern.sides.length) throw new RangeError('패턴 칸 번호가 범위를 벗어났습니다.');
            const labelChar = pattern.labels[index];
            const labelCode = labelChar === '-' ? 0 : labelChar.charCodeAt(0);
            if (labelCode !== 0 && (labelCode < LABEL_BASE || labelCode > 0x7a)) throw new Error("num==0 || ('a' <= num && num <= 'z')");
            this.#label = labelCode - LABEL_BASE;
            const orientation = pattern.sides[index];
            const variation = pattern.variations.charCodeAt(index) - VARIATION_BASE;
            if (orientation !== EMPTY_SIDE) {
                if (orientation < FIRST_SIDE || orientation > LAST_SIDE) throw new Error("'A' <= orientation && orientation <= 'P'");
                if (variation < 0 || variation > MAX_VARIATION) throw new Error('0 <= variation && variation <= 10');
                // 회전에 따라 방향 글자를 바꿔 그 방향·변형·번호의 프레임을 찾는다(원본 FUN_0049a940).
                const rotated = ROTATED_SIDES[rotation];
                this.#frame = findFrameNumber(this.#codes, rotated.charCodeAt(orientation.charCodeAt(0) - FIRST_SIDE.charCodeAt(0)), rotated.charCodeAt(VARIANT_COLUMN), variation);
                // 원본 assert lookup 은 보고한 뒤 -1 을 반환하며 같은 진행이 그 칸을 건너뛴다.
                if (this.#frame === -1 && typeof this.#onMissingFrame === 'function') this.#onMissingFrame();
            }
            }
            // 10.78 은 현재 checking 에서 현재 프레임 코드의 금 간 비트를 읽는다.
            if (this.#diagnostics && (this.#diagnostics.checking?.() >>> 0) && this.#frame !== -1) {
                if (this.#frame < 0 || this.#frame >= this.#codes.length) throw new RangeError('CanonDecoder 진단 복귀 뒤 물리 프레임 코드 범위 오류');
                if ((this.#codes[this.#frame].flags ?? 0) & CRACKED_FLAG) this.#diagnose(CRACKED_EXPRESSION, CRACKED_LINE);
            }
            this.#x = Math.fround(this.#inner + this.#originX);
            this.#y = Math.fround(this.#outer + this.#originY);
            // 패턴 좌표를 안쪽 방향으로 한 칸 옮긴다.
            if (INNER_STEP_X[rotation] !== 0) this.#patternX = (INNER_STEP_X[rotation] + this.#innerCount + this.#patternX) % this.#innerCount;
            if (INNER_STEP_Y[rotation] !== 0) this.#patternY = (INNER_STEP_Y[rotation] + this.#innerCount + this.#patternY) % this.#innerCount;
            this.#inner += 1;
            if (this.#frame !== -1) return;
        }
    }
}

// 청크 지도의 항목 하나이다(원본 12바이트): 섬 목록 번호·영역 패턴 칸의 방향 글자·`Territory` 레코드 6바이트·영역 안에서 몇 번째 칸인가. (2026-11-01 WB03-V)
function emptyEntry() {
    return { island: NO_ISLAND, side: '', record: new Uint8Array(TERRITORY_RECORD_BYTES), ordinal: 0 };
}

// 범위 안으로 당긴 값을 돌려준다. (2026-11-01 WB03-V)
function clamp(value, low, high) {
    return Math.min(Math.max(value, low), high);
}

// 섬(영역) 목록이다(원본 DAT_00568b60, Islandlist.cpp 중 배치에 쓰는 필드): 등록 순서가 섬 번호이고 소유자와 영역 번호를 보관한다.
// 사용: const islands = new IslandList(); islands.add(1, 7) → 0. (2026-11-01 WB03-V)
export class IslandList {
    constructor() {
        // 등록 순서의 섬 목록 [{ owner, territory }] 이다.
        this.islands = [];
    }

    // 끝에 더하고 그 번호를 돌려준다(원본 0046ee60).
    add(owner, territory) {
        this.islands.push({ owner, territory });
        return this.islands.length - 1;
    }

    // 그 번호의 섬이 있는가(원본 0046ee30).
    exists(island) {
        return Number.isInteger(island) && island >= 0 && island < this.islands.length;
    }

    // 그 섬이 owner 의 territory 번 영역인가(원본 0046f200 의 일반 경로).
    matches(island, owner, territory) {
        if (!this.exists(island)) return false;
        const info = this.islands[island];
        return info.owner === owner && info.territory === territory;
    }
}

// 월드를 청크(16 × 16칸) 격자로 나눈 지도이다(원본 ChunkMap.cpp): 청크마다 어느 섬(영역)에 속하는지를 담는다.
// 항목은 (x, y) → entries[y × 16 + x] 이다(원본은 x × 16 + y 순서이지만 밖으로 드러나지 않는다).
// 사용: const map = new ChunkMap(); map.at(11, 6).island. (2026-11-01 WB03-V)
export class ChunkMap {
    // 가로·세로 청크 수(미션 맵은 16 × 16)이다. 1~16 범위를 벗어나면 오류이다.
    constructor(xLength = WORLD_CHUNKS_PER_SIDE, yLength = WORLD_CHUNKS_PER_SIDE) {
        if (!(xLength >= 1 && yLength >= 1 && xLength <= WORLD_CHUNKS_PER_SIDE && yLength <= WORLD_CHUNKS_PER_SIDE)) throw new RangeError('Chunk map size');
        // 가로 청크 수이다.
        this.xLength = xLength;
        // 세로 청크 수이다.
        this.yLength = yLength;
        // 모든 청크를 "섬 없음"으로 둔 항목 목록이다(고정 256칸).
        this.entries = Array.from({ length: WORLD_CHUNKS_PER_SIDE * WORLD_CHUNKS_PER_SIDE }, emptyEntry);
    }

    // 좌표가 현재 지도 안인가.
    contains(x, y) {
        return x >= 0 && y >= 0 && x < this.xLength && y < this.yLength;
    }

    // 그 청크의 항목이다. 범위를 벗어난 좌표는 원본처럼 가장자리로 당긴다(원본은 그 전에 assert 한다; 당겨졌는지는 contains 로 먼저 확인한다).
    at(x, y) {
        return this.entries[clamp(y, 0, this.yLength - 1) * WORLD_CHUNKS_PER_SIDE + clamp(x, 0, this.xLength - 1)];
    }

    // 그 청크에 섬이 있는가(원본 00432300).
    hasIsland(x, y) {
        return this.at(x, y).island !== NO_ISLAND;
    }

    // 그 섬이 차지한 청크를 모두 감싸는 사각형 { left, top, right, bottom } 이다(right·bottom 은 포함하지 않는 경계, 원본 00432210). 찾지 못하면 (16, 16, 1, 1) 이다(원본 그대로).
    bounds(island) {
        let left = WORLD_CHUNKS_PER_SIDE;
        let top = WORLD_CHUNKS_PER_SIDE;
        let right = 0;
        let bottom = 0;
        // 지도 전체를 y 바깥, x 안쪽으로 훑는다.
        for (let y = 0; y < this.yLength; y += 1) {
            for (let x = 0; x < this.xLength; x += 1) {
                if (this.at(x, y).island !== island) continue;
                left = Math.min(left, x);
                top = Math.min(top, y);
                right = Math.max(right, x);
                bottom = Math.max(bottom, y);
            }
        }
        return { left, top, right: right + 1, bottom: bottom + 1 };
    }
}

// 영역을 청크 지도에 놓는 도구이다(원본 Islandbuilder.cpp 0046dba0·0046dd70). 놓을 때 지도 밖이라 가장자리로 당겨진 칸 수(clampedCells)와
// 프레임을 찾지 못해 건너뛴 칸 수(missingFrames)를 세어 둔다(원본 파일에서는 둘 다 0 이어야 한다).
// 사용: const builder = new IslandBuilder(map, islands, pieceFrameCodes(table), 1); builder.placeAll(fort.territory). (2026-11-01 WB03-V)
export class IslandBuilder {
    // map·islands 는 놓을 곳, frameCodes 는 pieceFrameCodes 의 결과, owner 는 섬의 소유자(미션 읽기는 1), rect 는 영역 위치의 기준 사각형이다.
    constructor(map, islands, frameCodes, owner, rect = DEFAULT_CHUNK_RECT) {
        this.map = map;
        this.islands = islands;
        this.frameCodes = frameCodes;
        this.owner = owner;
        this.rect = rect;
        // 지도 밖이라 가장자리로 당겨진 칸의 수이다.
        this.clampedCells = 0;
        // 프레임을 찾지 못해 건너뛴 칸의 수이다.
        this.missingFrames = 0;
    }

    // 레코드 하나를 놓고 섬 번호를 돌려준다(원본 0046dba0). territory 는 영역 번호, ignorePosition 이 참이면 위치 바이트를 무시해 기준 사각형의 왼쪽 위에 놓는다.
    // 레코드가 6바이트보다 짧거나 모양 번호가 표 밖이면 오류이다.
    place(record, territory, ignorePosition = false) {
        if (record.length < TERRITORY_RECORD_BYTES) throw new RangeError('Territory record');
        // 위치 바이트: 하위 니블 x, 상위 니블 y(오브젝트 위치 바이트와 반대다).
        const offsetX = ignorePosition ? 0 : record[2] & 0xf;
        const offsetY = ignorePosition ? 0 : record[2] >> 4;
        const island = this.islands.add(this.owner, territory);
        const shape = record[0] & SHAPE_MASK;
        if (shape >= TERRITORY_PATTERNS.length) throw new RangeError('Territory shape');
        const cells = decodeTerritoryPattern(TERRITORY_PATTERNS[shape], record[0] >> DIRECTION_SHIFT, this.rect.left + offsetX, this.rect.top + offsetY, this.frameCodes,
            () => { this.missingFrames += 1; });
        let ordinal = 0;
        // 유효한 칸마다 청크 항목을 채운다.
        for (const cell of cells) {
            if (!this.map.contains(cell.x, cell.y)) this.clampedCells += 1;
            const entry = this.map.at(cell.x, cell.y);
            entry.island = island;
            // 레코드 6바이트를 그대로 보관한다.
            entry.record.set(record.subarray(0, TERRITORY_RECORD_BYTES));
            entry.ordinal = ordinal;
            entry.side = cell.side;
            ordinal += 1;
        }
        return island;
    }

    // 영역 레코드 120바이트에서 start 이상 end 미만 번호의 레코드를 번호순으로 놓는다(원본 0046dd70). 활성 플래그가 켜지고 건너뜀 플래그가 꺼진 레코드만 놓는다. start 와 end 가 같으면 오류이다.
    placeAll(territories, start = 0, end = TERRITORY_COUNT) {
        if (start === end) throw new RangeError('iStart != iEnd');
        // 레코드를 번호순으로 본다.
        for (let index = start; index < end; index += 1) {
            const offset = index * TERRITORY_RECORD_BYTES;
            if (offset + TERRITORY_RECORD_BYTES > territories.length) break;
            const flags = territories[offset + 1];
            if ((flags & ACTIVE_FLAG) !== 0 && (flags & SKIP_FLAG) === 0) this.place(territories.subarray(offset, offset + TERRITORY_RECORD_BYTES), index);
        }
    }
}

// 소유자와 영역 번호가 맞는 청크의 좌표 목록 [{ x, y }] 이다. 월드를 y 바깥·x 안쪽으로 훑는 순서이며 이 순서가 `TerrNN` 섹션의 청크 레코드 순서이다(원본 004be2c0). (2026-11-01 WB03-V)
export function territoryChunks(map, islands, owner, territory) {
    const chunks = [];
    // y 바깥, x 안쪽.
    for (let y = 0; y < map.yLength; y += 1) {
        for (let x = 0; x < map.xLength; x += 1) {
            if (islands.matches(map.at(x, y).island, owner, territory)) chunks.push({ x, y });
        }
    }
    return chunks;
}

// 영역 레코드 하나를 읽기 쉬운 객체로 풀어 준다: { shape, direction, flags, active, positionX, positionY, raw }. 놓지 않는 레코드도 값은 풀어 둔다. (2026-11-01 WB03-V)
export function readTerritoryRecord(territory, index) {
    const offset = index * TERRITORY_RECORD_BYTES;
    const raw = territory.slice(offset, offset + TERRITORY_RECORD_BYTES);
    return {
        shape: raw[0] & SHAPE_MASK,
        direction: raw[0] >> DIRECTION_SHIFT,
        flags: raw[1],
        active: (raw[1] & ACTIVE_FLAG) !== 0 && (raw[1] & SKIP_FLAG) === 0,
        positionX: raw[2] & 0xf,
        positionY: raw[2] >> 4,
        raw,
    };
}

// 오브젝트의 월드 칸 좌표를 돌려준다: 청크 좌표 × 16 + 청크 안 칸 좌표(위치 바이트의 상위 4비트 x·하위 4비트 y). (2026-11-01 WB03-V)
export function worldCell(chunkX, chunkY, object) {
    return { x: chunkX * CHUNK_CELLS + object.cellX, y: chunkY * CHUNK_CELLS + object.cellY };
}

// `.fort` 한 개의 영역 배치를 만든다. fort 는 parseFort 의 결과, frameCodes 는 pieceFrameCodes 의 결과이다.
// 반환: { map, islands, builder, chaff[], regions[] }.
//  - chaff: 월드 청크 256개 [{ x, y, chunk }] (Chaff 섹션의 청크 i 는 월드 청크 (i % 16, i ÷ 16), 섹션이 없으면 빈 배열).
//  - regions: 영역 20개 [{ territory, record, chunks[{ x, y, side, ordinal, stored }], storedCount, matches }]. 놓이지 않은 영역은 chunks 가 비어 있다.
//    stored 는 그 청크에 짝지어진 `TerrNN` 의 청크 레코드({ objects[] }, 저장된 것이 모자라면 null), matches 는 놓인 청크 수와 저장된 청크 레코드 수가 같은가이다.
// 사용: const layout = layoutFort(parseFort(bytes, types), pieceFrameCodes(table)); layout.regions[7].chunks. (2026-11-01 WB03-V)
export function layoutFort(fort, frameCodes, owner = 1) {
    const map = new ChunkMap();
    const islands = new IslandList();
    const builder = new IslandBuilder(map, islands, frameCodes, owner);
    if (fort.territory) builder.placeAll(fort.territory);
    // Chaff 의 청크 레코드는 월드를 y 바깥·x 안쪽으로 훑은 순서이다.
    const chaff = fort.chaff.chunks.map((chunk, i) => ({ x: i % WORLD_CHUNKS_PER_SIDE, y: Math.trunc(i / WORLD_CHUNKS_PER_SIDE), chunk }));
    const regions = [];
    // 영역마다 놓인 청크와 저장된 청크 레코드를 짝짓는다.
    for (let territory = 0; territory < TERRITORY_COUNT; territory += 1) {
        const stored = fort.territories[territory].chunks;
        const chunks = territoryChunks(map, islands, owner, territory).map(({ x, y }, k) => {
            const entry = map.at(x, y);
            return { x, y, side: entry.side, ordinal: entry.ordinal, stored: stored[k] ?? null };
        });
        regions.push({ territory, record: fort.territory ? readTerritoryRecord(fort.territory, territory) : null, chunks, storedCount: stored.length, matches: chunks.length === stored.length });
    }
    return { map, islands, builder, chaff, regions };
}
