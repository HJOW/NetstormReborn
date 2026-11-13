// PATCH 0043c4e0 의 고정 소스/소유자 표/`!color.dat` 캐시 부모를 옮긴 모듈이다(cpppj `o/RawColorMapCache.cpp/.h`, 10.78 전용). 실제 File/TYPE 표/9표 계산은 필수 하위(hooks)이고 이 모듈은 호출 순서와 부분 효과만 소유한다.
//  - prepareColorMapCache(globals, hooks): 경로 장치(00459c60)가 돌려준 NUL 경로를 받은 뒤 **현재** ColorSort 소스(primary)를 한 번 빌리고, 소유자 8행 × 색 256칸을 행마다 현재 색 DWORD 의 하위 BYTE 로
//    ownerSolid(고정색 표) → ownerHighlight(항등) → 소스 기록 56바이트 복사 → 명도(+8 double) > 0.800000011920929 이면 ownerHighlight 를 소유자 색으로 바꾸는 순서로 쓰고, 마지막에 소유자 표 등록(hooks.owners)을 한다. 경로를 돌려준다.
//  - initializeColorMapCache(globals, hooks, reader, writer): 위 접두 → reader 마지막 8바이트만 0 → reader 열기(경로, 모드 0, 1)·아홉 표 읽기(파일 순서 6038/7738/6438/6238/6138/6338/5f38/5e38/7838)·닫기.
//    정확한 ColorCacheUnableToOpen 만 재생성(regenerateColorMapCache: 9표 계산 → writer 열기(모드 1, 1)·아홉 쓰기·닫기)으로 들어가고 다른 오류는 정리(생성이 끝난 파일만 닫음) 뒤 전파한다.
//  - closeColorMapCacheFile(file, hooks): 독립 정리 funclet 004f9c20/004f9c28 의 고정 this 전달과 같은 경계이다.
// globals = { primary()→현재 raw 기록 Uint8Array, ownerColors(8 DWORD), ownerHighlight(Uint8Array 2048), ownerSolid(Uint8Array 2048), maps(9개 Uint8Array 256, 파일 순서) }, 파일 객체는 24바이트 Uint8Array(호출자가 정의한 서로 다른 지역 물리).
// hooks = { path(), owners(), buildMaps(), openRead(file, name, mode, create), openWrite(…), read(file, target, limit)→읽은 길이, write(file, source, limit)→쓴 길이, close(file) } 는 도달할 때만 요구한다(없으면 Error).
// 이 모듈은 실제 OS 파일 장치를 복원하지 않으며 웹은 메모리 저장소(core/color-map-files.js)를 hooks 로 연결한다. DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E375: 5-A RawColorMapCache 이식으로 추가, 기준선 뒤 cpppj 변경 없음)

// 파일 접근의 "열 수 없음" 만 나타낸다(원문 Handler 0052221c 의 EX_UNABLE_TO_OPEN_FILE). 일반 오류 전체를 이 형으로 바꾸지 않는다. (2026-11-13 WB04-E375)
export class ColorCacheUnableToOpen extends Error {
    // 원문 형식 분류용이며 호스트 오류 메시지로 catch 대상을 추측하지 않는다.
    constructor() {
        super('EX_UNABLE_TO_OPEN_FILE');
        this.name = 'ColorCacheUnableToOpen';
    }
}

// 소유자 행 수, 행의 색 칸 수, ColorSort 기록 크기(56)와 명도 위치(+8), 명도 임계(00506710 의 double), 읽기/쓰기 제한 인자이다. (2026-11-13 WB04-E375)
const OWNER_ROWS = 8;
const COLORS = 256;
const RECORD_BYTES = 56;
const LIGHT_AT = 8;
const HIGHLIGHT_THRESHOLD = 0.800000011920929;
const NO_LIMIT = 0xffffffff;
// 읽기 객체에서 0 으로 쓰는 마지막 8바이트의 시작 위치이다. (2026-11-13 WB04-E375)
const READER_TAIL_AT = 16;

// 원문이 선택한 필수 하위만 그 호출 시점에 검사한다. (2026-11-13 WB04-E375)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('ColorMap cache의 실제 하위 장치 미연결');
    return hook;
}

// Native REP MOVSD 14 의 각 DWORD 물리를 읽어 56바이트 임시 기록에 복사하고 기록+8 의 little endian double 명도를 돌려준다. 실패 전에 쓴 출력은 되돌리지 않는다. (2026-11-13 WB04-E375)
function currentLight(primary, offset) {
    const record = new Uint8Array(RECORD_BYTES);
    // 각 4바이트 복사 직전에만 현재 source 의 물리 범위를 확인한다.
    for (let word = 0; word < 14; word += 1) {
        const source = offset + word * 4;
        if (source > primary.length || 4 > primary.length - source) throw new RangeError('ColorMap cache raw ColorSort 기록 물리범위');
        record.set(primary.subarray(source, source + 4), word * 4);
    }
    return new DataView(record.buffer).getFloat64(LIGHT_AT, true);
}

// 경로 → 현재 primary 한 번 → 두 8행 표 → TYPE 표까지이다. 반환 경로의 같은 물리는 이후 파일 단계도 빌린다. 사용: const path = prepareColorMapCache(globals, hooks). (2026-11-13 WB04-E375)
export function prepareColorMapCache(globals, hooks) {
    const path = need(hooks.path)();
    const primary = need(globals.primary)();
    // 원문 8행의 순서이며 이후 owners 훅에서 다시 생성하지 않는다.
    for (let row = 0; row < OWNER_ROWS; row += 1) {
        const selected = globals.ownerColors[row] & 0xff;
        // 원문은 solid → identity → 56바이트 복사 → 명도 조건 쓰기 순서이다.
        for (let color = 0; color < COLORS; color += 1) {
            const offset = row * COLORS + color;
            globals.ownerSolid[offset] = selected;
            globals.ownerHighlight[offset] = color;
            if (currentLight(primary, color * RECORD_BYTES) > HIGHLIGHT_THRESHOLD) globals.ownerHighlight[offset] = selected;
        }
    }
    need(hooks.owners)();
    return path;
}

// 실제 close 는 같은 물리/장치가 소유한다. 정상 close 전에 부모는 constructed 관문을 먼저 내린다. (2026-11-13 WB04-E375)
export function closeColorMapCacheFile(file, hooks) {
    need(hooks.close)(file);
}

// 독립 catch 0043c96b: 9표 재계산 → writer → 9 write → close 이다. writer 는 reader 와 다른 지역 객체이며 ctor 직전의 미정 24바이트를 부모가 초기화하지 않는다. 같은 reader try 를 재진입하지 않는다. (2026-11-13 WB04-E375)
export function regenerateColorMapCache(path, globals, hooks, writer) {
    need(hooks.buildMaps)();
    let constructed = false;
    try {
        need(hooks.openWrite)(writer, path, 1, 1);
        constructed = true;
        // 아홉 반환 길이를 검사하거나 짧음으로 분기하지 않는다.
        for (const table of globals.maps) need(hooks.write)(writer, table, NO_LIMIT);
        constructed = false;
        closeColorMapCacheFile(writer, hooks);
    } catch (error) {
        if (constructed) {
            constructed = false;
            closeColorMapCacheFile(writer, hooks);
        }
        throw error;
    }
}

// 경로/TYPE 표 부모는 try 앞이다(그 단계의 marker 를 reader cache miss 로 잘못 잡지 않는다). reader/writer 는 caller 가 명시한 서로 다른 24바이트 지역 물리이며 reader 마지막 8바이트만 0 으로 쓴다.
// 정확한 ColorCacheUnableToOpen 만 cache 재생성에 들어간다. 장치는 unwind 중 close 에서 두 번째 예외를 던지지 않아야 한다. (2026-11-13 WB04-E375)
export function initializeColorMapCache(globals, hooks, reader, writer) {
    const path = prepareColorMapCache(globals, hooks);
    reader.fill(0, READER_TAIL_AT);
    let constructed = false;
    try {
        try {
            need(hooks.openRead)(reader, path, 0, 1);
            constructed = true;
            // 같은 고정 아홉 목적 물리에 부분 읽기를 남긴다.
            for (const table of globals.maps) need(hooks.read)(reader, table, NO_LIMIT);
            constructed = false;
            closeColorMapCacheFile(reader, hooks);
        } catch (error) {
            if (constructed) {
                constructed = false;
                closeColorMapCacheFile(reader, hooks);
            }
            throw error;
        }
    } catch (error) {
        if (!(error instanceof ColorCacheUnableToOpen)) throw error;
        regenerateColorMapCache(path, globals, hooks, writer);
    }
}
