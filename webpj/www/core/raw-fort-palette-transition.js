// 팔레트 이름 전환(004b6100)과 세 하위 부모의 현재 전역·부분 효과를 옮긴 모듈이다(cpppj `o/RawFortPaletteTransition.cpp/.h`, 10.78 전용).
//  - transitionFortPalette(argument, globals, hooks)(004b6100): 로그(29, "init palette\n") 복귀 뒤에 인자(포인터 변수)를 캡처하고, 현재 이름과 같으면(대소문자 무시 비교가 0) 자연 반환한다.
//    다르면 이름을 NUL 까지 순방향 복사 → 구름 캐시 해제 → 구름 이름 설정 → 경로 조회("GamePalSpec") → 팔레트 적재(둘째 인자 1) → 추가 표 계산 → 렌더러가 있으면 dirty 를 1 로 만든다.
//  - releaseFortPaletteCloudCache(globals, hooks)(00470c30): 비영 현재 캐시만 해제하고 정상 복귀한 뒤 무조건 같은 DWORD 를 0 으로 쓴다.
//  - setFortPaletteCloudName(source, globals, hooks)(00471190): 같은 source 주소에서 NUL 까지 순방향 복사하고, 캐시가 0 이면 구름 계산을 한 뒤 현재 횟수를 한 번 캡처해 그만큼 행을 갱신한다.
//  - markFortPaletteDirty(globals)(004973c0): 현재 렌더러 DWORD 가 비영일 때만 dirty = 1 로 만든다.
// 팔레트 계산 자체는 다시 쓰지 않고 이름·캐시·하위 호출·dirty 의 순서만 소유한다. 이름 복사는 길이를 미리 재지 않고 BYTE 하나씩 읽고 써서 겹치는 버퍼와 물리 중단 전 효과를 보존한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E227: 팔레트 전환 이식으로 추가)

// 고정 주소의 이름 버퍼와 같은 Client 수명의 실제 DWORD 를 빌리는 전역 묶음이다.
//  globals = { paletteName: Uint8Array(005c8910 물리 버퍼), cloudName: Uint8Array(0056b7a0 물리 버퍼, paletteName 과 별개), cache: { value }(00569ffc Kencloud 할당 주소/장치 토큰),
//              count: () => 정수(00540a80 현재 횟수, build 뒤 한 번 읽음), renderer: () => 정수(0059a8bc 현재 렌더러 존재 DWORD, maps 뒤에 읽음), dirty: { value }(0059a8b0) }.
// hooks = { log(level, text), compare(a, b) → 정수, release(cache), buildCloud(), stepCloud(), pathSpec(key, source, first, second) → Uint8Array|null, loadPalette(path, clear), buildMaps() } 이며
// 도달한 필수 장치 누락은 성공으로 가장하지 않고 그 위치에서 TypeError 로 거부해 앞선 효과를 남긴다. (2026-11-10 WB04-E227)

// 필수 하위 장치를 도달한 시점에 꺼낸다. 누락은 기본 성공값 대신 TypeError 이다. (2026-11-10 WB04-E227)
function need(hooks, name) {
    if (typeof hooks?.[name] !== 'function') throw new TypeError(`팔레트 전환 하위 장치 ${name} 누락`);
    return hooks[name];
}

// 전체 길이를 미리 계산하지 않고 BYTE 하나씩 읽고 써서 겹치는 버퍼와 물리 중단 전 효과를 보존한다. 원문에는 길이 절단이나 memcpy 가 없으며 NUL 을 쓴 직후에만 끝난다.
// 입력/출력 물리 범위를 벗어나면 RangeError 이다(메시지로 입력/출력을 구별). (2026-11-10 WB04-E227)
function copyNameForward(target, source) {
    for (let index = 0; ; index += 1) { // NUL 을 쓴 직후에만 끝난다.
        if (index >= source.length) throw new RangeError('팔레트 이름 입력 물리 범위');
        const value = source[index]; // 같은 source 주소의 현재 BYTE 이며 앞선 alias 쓰기를 관측한다.
        if (index >= target.length) throw new RangeError('팔레트 이름 출력 물리 범위');
        target[index] = value;
        if (value === 0) return;
    }
}

// 00470c30: 같은 현재 구름 할당을 해제한 뒤에만 전역 토큰을 0 으로 쓰며 실패 전 효과를 보존한다. (2026-11-10 WB04-E227)
export function releaseFortPaletteCloudCache(globals, hooks) {
    const current = globals.cache.value; // 검사와 해제 인자에 함께 쓰는 현재 주소이다.
    if (current !== 0) {
        need(hooks, 'release')(current);
        globals.cache.value = 0;
    }
}

// 00471190: 실제 구름 이름을 복사하고 필요한 생성 뒤에 한 번 읽은 횟수만큼 행을 갱신한다. (2026-11-10 WB04-E227)
export function setFortPaletteCloudName(source, globals, hooks) {
    copyNameForward(globals.cloudName, source);
    if (globals.cache.value === 0) need(hooks, 'buildCloud')();
    let remaining = globals.count(); // build 가 바꾼 현재 횟수까지 반영한 뒤 한 번 캡처한다.
    // 각 step 이 count/cache 를 바꾸어도 남은 횟수는 캡처된 값을 쓴다.
    while (remaining !== 0) {
        need(hooks, 'stepCloud')();
        remaining -= 1;
    }
}

// 004973c0: 팔레트 적용 뒤 현재 렌더러가 존재하는 경우에만 dirty 를 설정한다. (2026-11-10 WB04-E227)
export function markFortPaletteDirty(globals) {
    if (globals.renderer() !== 0) globals.dirty.value = 1;
}

// 004b6100: 같은 이름 자연 반환과 로그·이름·구름·경로·장치·색표·dirty 의 실제 순서를 실행한다. argument 는 `() => Uint8Array` 로 로그 복귀 뒤에 읽는다(스택의 인자 변수가 로그가 바꾼 값을 보게 함). (2026-11-10 WB04-E227)
export function transitionFortPalette(argument, globals, hooks) {
    need(hooks, 'log')(29, 'init palette\n');
    const source = argument(); // 004b610d 는 log 복귀 뒤 인자 포인터를 캡처한다.
    if (need(hooks, 'compare')(globals.paletteName, source) === 0) return;
    copyNameForward(globals.paletteName, source);
    releaseFortPaletteCloudCache(globals, hooks);
    setFortPaletteCloudName(source, globals, hooks);
    const path = need(hooks, 'pathSpec')('GamePalSpec', source, 0, 0); // 원문 push 1 은 PathSpec 인자가 아니라 다음 loadPalette 의 둘째 인자이다.
    need(hooks, 'loadPalette')(path, 1);
    need(hooks, 'buildMaps')();
    markFortPaletteDirty(globals);
}
