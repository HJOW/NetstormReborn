// 원본 Kencloud(구름 배경 캐시)의 파일 부모 00470bb0, 생성 00471040, 행 이동 00470b20 을 같은 현재 물리 메모리·GameRandom 에 옮긴 모듈이다(cpppj `o/RawKencloud.cpp/.h`, 10.78 전용).
// 512×512바이트 캐시(0x40000)를 주소 토큰(uint32)으로 다루며, 메모리 소유권·파일 장치는 훅으로 빌린다. 이 모듈은 난수나 캐시를 새로 소유하지 않고 호출자의 전역(globals)과 GameRandom 을 직접 읽고 쓴다.
//  - loadKencloudFile(scratch, hooks): 004410f0 "CloudFileSpec" 경로 NUL 물리를 256바이트 scratch 에 NUL 포함 한 바이트씩 복사하고(출력 물리를 넘으면 오류) exists 가 비영일 때만 decode(path, scratch)를 부르며,
//    치수가 512×512 가 아니면 반환 주소(0 도)를 release 한 뒤 0 을 돌려준다. scratch = { height, width, path(Uint8Array 256) }.
//  - stepKencloud(globals, hooks): 캐시 첫 바이트를 읽고(실패면 phase 는 아직 변하지 않음) phase 를 (phase+1)&15 로 올린 뒤 end[-1] 에 그 바이트를 쓰고, 현재 위상의 행 번호(phaseRows[phase])부터 16행 간격으로 행을 한 칸씩 memmove 한다
//    (매 행 현재 캐시/end 를 다시 읽고 끝 주소를 end-1 로 제한). 난수를 쓰지 않는다.
//  - buildKencloud(globals, random, hooks): 캐시가 비영이면 표시 요청(전체 1·부분 0)만 하고 끝낸다. 아니면 파일 부모로 읽고, 없으면 0x40000 을 할당해 lowColor 로 채운 뒤 시드(seedCount > 0 이면 행 1..510·열 0..511 난수 두 번씩)와
//    간격 난수(256)+5 를 정하고, 파일 대신 생성한 경우에만 fury 기반 횟수(10000/fury, 부호 있는 나눗셈; 0 이면 예외)만큼 speed 번의 step 을 되풀이하는 warm-up 을 한다. 캐시가 0 이면 재귀로 다시 만든다.
// globals 는 호출 중 계속 읽고 쓰는 살아 있는 객체이다: { cache, end, nextRow, lastRow, phase, allDirty, partialDirty, spacer, lowColor, speed, fury, seedCount, phaseRows[16], dirtyRows(Uint32Array 512), seeds(Uint32Array) }.
// 훅 { loadFile(), allocate(bytes), memory(address)→Uint8Array(주소부터 남은 물리; 미등록 주소는 RangeError), afterMove(target, source, length), afterRandom(limit, before, result) } 는 도달한 지점에서만 요구한다.
// 물리 범위 오류는 `RangeError('Kencloud <원인>')`, 0 나눗셈은 `KencloudDomainError('Kencloud divide-zero')` 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E118: RawKencloud 이식으로 추가)

// 원문의 512×512 할당 크기, 행 바이트, 행별 이동 간격(0x3dd)과 16행 간격 묶음(0x3dd0)이다. (2026-11-08 WB04-E118)
const BYTES = 0x40000;
const ROW_BYTES = 512;
const STRIDE = 0x3dd;
const STRIDE_BATCH = 0x3dd0;

// 0 나눗셈을 성공으로 바꾸지 않는 명시 예외이다. (2026-11-08 WB04-E118)
export class KencloudDomainError extends Error {
    // 원문 메시지를 그대로 보존한다. (2026-11-08 WB04-E118)
    constructor(message = 'Kencloud divide-zero') {
        super(message);
        this.name = 'KencloudDomainError';
    }
}

// 외부 소유자가 빌려 주는 주소의 물리 길이를 확인하고 원문의 해당 접근 시점에만 실패한다. 길이 0 도 one-past 를 허용하는 소유자에 조회한다. (2026-11-08 WB04-E118)
function bytes(hooks, address, length, cause) {
    let view;
    try {
        view = hooks.memory(address >>> 0);
    } catch (error) {
        if (error instanceof RangeError) throw new RangeError(`Kencloud ${cause}`);
        throw error;
    }
    if (view.length < length) throw new RangeError(`Kencloud ${cause}`);
    return view.subarray(0, length);
}

// fury 의 signed 해석과 매회 IDIV 를 보존한다. 음수는 그대로 음수 몫이며 0 만 명시 예외이다. (2026-11-08 WB04-E118)
function warmCount(globals) {
    const divisor = globals.fury | 0;
    if (divisor === 0) throw new KencloudDomainError();
    return (10000 / divisor) | 0;
}

// 기존 실제 난수를 정확히 한 번 소비하고 선택 관측 뒤 반환값은 그대로 유지한다. (2026-11-08 WB04-E118)
function next(random, limit, hooks) {
    const before = random.state;
    const result = random.next(limit);
    if (hooks.afterRandom) hooks.afterRandom(limit, before, result);
    return result;
}

// 00470bb0: 현재 256바이트 복사 → exists → decode → 현재 치수 검사/free 를 실행한다. 모든 scratch 바이트는 호출자 입력이다. 반환은 512×512 이미지의 주소 토큰(아니면 0)이다. 사용: loadKencloudFile(scratch, hooks). (2026-11-08 WB04-E118)
export function loadKencloudFile(scratch, hooks) {
    // 호출 뒤 반환된 같은 NUL 물리를 캡처한다.
    const source = hooks.pathSpec('CloudFileSpec', 0, 0, 0);
    // 전체 길이 선계산/문자열 사본 없이 원문처럼 NUL 포함한 바이트씩 복사한다.
    for (let index = 0; ; index += 1) {
        if (index >= source.length) throw new RangeError('Kencloud file-path-read');
        const value = source[index];
        if (index >= scratch.path.length) throw new RangeError('Kencloud file-path-write');
        scratch.path[index] = value;
        if (value === 0) break;
    }
    if (hooks.exists(scratch.path) !== 0) {
        // 중단해도 현재 치수와 이름의 부분 쓰기는 남는다.
        const image = hooks.decode(scratch.path, scratch);
        if ((scratch.width | 0) === 512 && (scratch.height | 0) === 512) return image >>> 0;
        hooks.release(image);
    }
    return 0;
}

// 00470b20: cache 첫 바이트 읽기 뒤 phase 쓰기/end[-1] 쓰기와 현재 base/end 재조회를 보존한다. 난수를 소비하지 않는다. 사용: stepKencloud(globals, hooks). (2026-11-08 WB04-E118)
export function stepKencloud(globals, hooks) {
    // 원문 첫 load 이며 물리 read 실패면 phase 는 아직 변하지 않는다.
    const previousPhase = globals.phase;
    const first = bytes(hooks, globals.cache, 1, 'step-read')[0];
    // end 도 phase 쓰기 앞에 현재 값을 캡처한다.
    const end = globals.end;
    const phase = (previousPhase + 1) & 15;
    globals.phase = phase;
    bytes(hooks, (end - 1) >>> 0, 1, 'step-tail')[0] = first;
    // 위상 쓰기 뒤 현재 행 표를 읽는다.
    const start = globals.phaseRows[phase] | 0;
    if (start >= 512) return;
    // 원문의 고정 dirty 포인터를 행 offset 으로 표현한다.
    let row = start >>> 0;
    let offset = Math.imul(row, STRIDE) >>> 0;
    // 각 CRT 호출 복귀 뒤 다음 행의 현재 cache/end 를 다시 읽는다.
    do {
        let destination = (globals.cache + offset) >>> 0;
        const currentEnd = globals.end;
        if (destination >= currentEnd) destination = (currentEnd - 1) >>> 0;
        let last = (destination + STRIDE) >>> 0;
        if (last >= currentEnd) last = (currentEnd - 1) >>> 0;
        const length = (last - destination) >>> 0;
        const target = bytes(hooks, destination, length, 'move-physical');
        const source = bytes(hooks, (destination + 1) >>> 0, length, 'move-physical');
        // 같은 버퍼의 겹침은 실제 memmove(앞쪽 사본 뒤 복사)로 보존한다.
        if (length !== 0) target.set(source.slice());
        if (hooks.afterMove) hooks.afterMove(destination, (destination + 1) >>> 0, length);
        if (row >= globals.dirtyRows.length) throw new RangeError('Kencloud dirty-physical');
        globals.dirtyRows[row] = 1;
        row = (row + 16) >>> 0;
        offset = (offset + STRIDE_BATCH) >>> 0;
    } while ((row | 0) < 512);
}

// 00471040: 현재 캐시/파일/할당/fill/seed/재귀 warm-up 을 실행하며 받은 실제 GameRandom 참조만 소비한다. 사용: buildKencloud(globals, random, hooks). (2026-11-08 WB04-E118)
export function buildKencloud(globals, random, hooks) {
    if (globals.cache !== 0) {
        globals.partialDirty = 0;
        globals.allDirty = 1;
        return;
    }
    // 파일 부모의 반환 주소를 캡처하고 같은 전역에 쓴다.
    const image = hooks.loadFile() >>> 0;
    globals.cache = image;
    // 파일 실패 fallback 에서만 warm-up 을 실행하는 원문 EBX 이다.
    let generated = false;
    if (image === 0) {
        // 할당 callback 뒤 현재 lowColor 를 읽는다.
        const allocated = hooks.allocate(BYTES) >>> 0;
        const color = globals.lowColor & 0xff;
        globals.cache = allocated;
        generated = true;
        // 실패 전 cache 대입은 이미 발생했다.
        bytes(hooks, allocated, BYTES, 'fill-physical').fill(color);
    }
    // fallback rep 종료 뒤의 현재 base 에서 상·하부 행 뷰를 구성한다.
    const base = globals.cache;
    const end = (base + BYTES) >>> 0;
    globals.nextRow = (base + ROW_BYTES) >>> 0;
    // 첫 signed 관문에 사용한다.
    const initialCount = globals.seedCount | 0;
    globals.end = end;
    globals.lastRow = (end - ROW_BYTES) >>> 0;
    if (initialCount > 0) {
        // 원문 EDI 는 0 부터 시작하며 각 callback 뒤 현재 count 와 비교한다.
        let index = 0;
        // 첫 양수 관문 뒤에는 현재 count 가 줄어도 현재 한 seed 의 두 난수와 쓰기를 먼저 완료한다.
        for (;;) {
            const y = (next(random, 510, hooks) + 1) >>> 0;
            const x = next(random, 512, hooks);
            if (index >= globals.seeds.length) throw new RangeError('Kencloud seed-physical');
            globals.seeds[index] = (globals.cache + y * ROW_BYTES + x) >>> 0;
            // 원문은 저장 뒤 다시 읽고 EDI 를 증가시킨다.
            const count = globals.seedCount | 0;
            index += 1;
            if (index >= count) break;
        }
    }
    globals.spacer = (next(random, 256, hooks) + 5) >>> 0;
    if (generated && warmCount(globals) > 0) {
        // 현재 signed 몫과 비교할 원문 EDI 이다.
        let iteration = 0;
        // fury 는 각 batch 복귀 뒤 재조회하며 재귀 생성 뒤 현재 speed 를 캡처한다.
        for (;;) {
            if (globals.cache === 0) buildKencloud(globals, random, hooks);
            // 재귀 복귀 뒤의 현재 DWORD 를 한 번만 캡처한다. step callback 이 speed 를 바꾸어도 현재 batch 의 남은 횟수는 변하지 않는다.
            let remaining = globals.speed >>> 0;
            while (remaining !== 0) {
                stepKencloud(globals, hooks);
                remaining = (remaining - 1) >>> 0;
            }
            const limit = warmCount(globals);
            iteration += 1;
            if (iteration >= limit) break;
        }
    }
    globals.partialDirty = 0;
    globals.allDirty = 1;
}
