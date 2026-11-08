// 원본 10.78 SquidHash 의 4단계 버킷 머리 배열·버킷 주소·객체 해시 단계 선택을 옮긴 공간 해시 모듈이다.
// 버킷 값은 객체 SID 체인의 머리(WORD)이며 체인의 다음 번호는 SID 풀 슬롯 +4 WORD 에 있다(RawSquidFinder 가 따라간다).
// 전체 발자국 점유(spot)·체인 삽입/제거·Pop 등록은 이 모듈 범위 밖이며 WB04-E 에서 같은 저장소에 연결한다.
// 근거: cpppj `o/SquidHash.cpp/.h`(원본 004b28f0 초기화, 004b29a0 At, 004b2a90 float 버킷, 004b2b00 정수 버킷, 004ace40 단계 선택),
//       읽기 전용 원본 관측 `cpppj/tests/fixtures/hash-x86.tsv`(초기화4·주소807·ObjectLevel605). CD 10.72 배치는 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-B: 실제 월드 공간 해시 이식으로 추가)
import { TypeFlag2 } from '../assets/type-flags.js';
import { WORLD_CELLS } from '../assets/terrain-mask.js';

// 단계별 버킷 한 변이 담당하는 월드 칸 수이다(원본 패치 005424f8). 0단계는 표면(섬/다리), 1~3단계는 일반 객체이다. (2026-11-05 WB04-B)
export const SQUID_HASH_SCALES = Object.freeze([1, 2, 4, 16]);

// 해시 단계 수이다. (2026-11-05 WB04-B)
export const SQUID_HASH_LEVELS = SQUID_HASH_SCALES.length;

// 단계 번호를 검사한다. 원본 일부 호출은 잘못된 단계를 포인터 산술로 진행하지만 웹은 오류로 거부한다(cpppj 와 같은 보호). (2026-11-05 WB04-B)
function checkLevel(level) {
    if (!Number.isInteger(level) || level < 0 || level >= SQUID_HASH_LEVELS) throw new RangeError('Squid hash 단계 범위 오류');
    return level;
}

// 원본 4단계 버킷 머리 배열을 소유하는 공간 해시이다. 각 단계 배열은 한 변 (256/scale) 의 정사각형이며 y 바깥·x 안쪽 순서(y*side+x)이다.
// JS 에서는 참조를 돌려줄 수 없으므로 읽기(cell/bucket/at)와 쓰기(setCell/setBucket/setAt)를 나눴다. entries(level) 는 읽기 사본이다.
// 사용: const hash = new SquidHash(); hash.setBucket(0, 20.25, 20.75, sid); hash.bucket(0, 20.9, 20.1) === sid. (2026-11-05 WB04-B)
export class SquidHash {
    // 단계별 WORD 머리 배열이다(0→3 단계 순서).
    #heads;
    // 현재 선택 단계이다(원본 초기화/탐색기의 단계 선택 대입).
    #selected = 0;

    // 네 단계 배열을 0 으로 확보하고 선택 단계를 정한다(원본 004b28f0 의 이미 할당된 배열 초기화와 같은 결과).
    constructor(selected = 0) {
        this.select(selected);
        this.#heads = SQUID_HASH_SCALES.map((_, level) => new Uint16Array(SquidHash.side(level) ** 2));
    }

    // 모든 단계의 머리를 0 으로 지운다. 선택 단계와 배열 저장소는 그대로 둔다(원본 초기화와 같은 결과).
    reset() {
        // 큰 표면 배열부터 작은 일반 공간 배열까지 모두 지운다.
        for (const heads of this.#heads) heads.fill(0);
    }

    // 현재 단계를 고른다. 유효한 0~3 만 받는다.
    select(level) {
        this.#selected = checkLevel(level);
    }

    // 현재 선택 단계이다.
    get selected() { return this.#selected; }

    // 단계 배열의 한 변(256/scale)이다. 사용: SquidHash.side(3) === 16.
    static side(level) {
        return WORLD_CELLS / SQUID_HASH_SCALES[checkLevel(level)];
    }

    // 단계 배열에서 버킷 칸 좌표(이미 scale 로 나눈 좌표)의 인덱스(원본 반환 주소 - 배열 시작, WORD 단위)이다. 배열 밖이면 RangeError.
    static cellIndex(level, x, y) {
        const side = SquidHash.side(level);
        if (!Number.isInteger(x) || !Number.isInteger(y) || x < 0 || y < 0 || x >= side || y >= side) throw new RangeError('Squid hash 버킷 칸 범위 오류');
        return y * side + x;
    }

    // 월드 좌표(float)를 0 방향으로 절삭한 뒤 단계 크기로 나눈 버킷 인덱스이다(원본 004b2a90, CRT _ftol). 입력은 float32 로 반올림해 원본 인자와 같게 한다.
    // 보드 밖·NaN·무한대는 RangeError 이다. 표면 조회의 +0.9999 보정은 이 함수에 넣지 않는다.
    static bucketIndex(level, x, y) {
        checkLevel(level);
        const fx = Math.fround(x);
        const fy = Math.fround(y);
        if (!Number.isFinite(fx) || !Number.isFinite(fy) || fx < 0 || fy < 0 || fx >= WORLD_CELLS || fy >= WORLD_CELLS) throw new RangeError('Squid hash 월드 좌표 범위 오류');
        const scale = SQUID_HASH_SCALES[level];
        return SquidHash.cellIndex(level, Math.trunc(Math.trunc(fx) / scale), Math.trunc(Math.trunc(fy) / scale));
    }

    // 지정 단계 버킷 칸의 머리 SID 를 읽는다(원본 004b2b00).
    cell(level, x, y) { return this.#heads[checkLevel(level)][SquidHash.cellIndex(level, x, y)]; }

    // 지정 단계 버킷 칸의 머리 SID 를 쓴다. value 는 WORD(0~65535) 정수여야 한다.
    setCell(level, x, y, value) { this.#heads[checkLevel(level)][SquidHash.cellIndex(level, x, y)] = word(value); }

    // 현재 선택 단계의 버킷 칸 머리를 읽는다(원본 004b29a0 At).
    at(x, y) { return this.cell(this.#selected, x, y); }

    // 현재 선택 단계의 버킷 칸 머리를 쓴다.
    setAt(x, y, value) { this.setCell(this.#selected, x, y, value); }

    // 월드 좌표가 속한 버킷의 머리를 읽는다(원본 004b2a90).
    bucket(level, x, y) { return this.#heads[checkLevel(level)][SquidHash.bucketIndex(level, x, y)]; }

    // 월드 좌표가 속한 버킷의 머리를 쓴다.
    setBucket(level, x, y, value) { this.#heads[checkLevel(level)][SquidHash.bucketIndex(level, x, y)] = word(value); }

    // 단계 배열의 읽기 사본이다. 반환값을 바꿔도 해시는 바뀌지 않는다(SID 풀의 slot 사본과 같은 정책).
    entries(level) { return this.#heads[checkLevel(level)].slice(); }

    // 단계 머리 배열의 살아 있는 읽기 전용 용도 참조이다(사본 아님). 원본의 표면 지도(0단계)처럼 같은 저장소를 직접 공유해야 하는 소비자(일반 배치 파이프라인의
    // 현재 표면·기준점 지도)에만 쓰며, 쓰기는 반드시 setCell/setBucket/setAt 으로 한다. (2026-11-09 WB04-E154: 일반 배치 파이프라인이 해시 0단계를 공유하도록 추가)
    view(level) { return this.#heads[checkLevel(level)]; }

    // 객체의 해시 단계를 고른다(원본 004ace40): 섬/다리(flags2 0x2|0x4)는 0, 그 밖은 현재 SHP 프레임 크기의 큰 변이 2 이하 1, 4 이하 2, 그보다 크면 3.
    // 프레임 크기는 float 입력이며 음수·NaN·무한대는 RangeError 이다. 사용: SquidHash.objectLevel(record.flags2, frame.width, frame.height).
    static objectLevel(flags2, frameWidth, frameHeight) {
        if (((flags2 >>> 0) & (TypeFlag2.island | TypeFlag2.bridge)) !== 0) return 0;
        const width = Math.fround(frameWidth);
        const height = Math.fround(frameHeight);
        if (!Number.isFinite(width) || !Number.isFinite(height) || width < 0 || height < 0) throw new RangeError('Squid hash 프레임 크기 범위 오류');
        const size = Math.max(width, height);
        return size <= 2 ? 1 : size <= 4 ? 2 : 3;
    }
}

// WORD 값을 검사한다. 체인 머리는 SID 번호(0 = 비어 있음)이다. (2026-11-05 WB04-B)
function word(value) {
    if (!Number.isInteger(value) || value < 0 || value > 0xffff) throw new RangeError('Squid hash 머리는 WORD 정수여야 합니다.');
    return value;
}
