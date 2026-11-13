// 원본 Renderer 의 한 유효 dirty 사각형 안에서 표시 목록 병합과 kind 0~4 분배를 옮긴 모듈이다(cpppj `client/DrawSubmission.cpp/.h`, 10.78 004994b0, 10.78 전용. CD 004575e0 경로는 범위 밖).
// 좌표 환산·dirty 순회·배경·장치 잠금은 호출자의 책임이다. UI 내부는 GumpCollection 으로 연결할 수 있다.
//  - 항목은 16바이트(x float32 · y float32 · token DWORD · depth int16 · kind BYTE · tail BYTE)이고 목록은 항목 n 개의 Uint8Array(16×n)이다 — `core/raw-render-collection.js` 가 쓰는 같은 형식이다.
//    묶음 저장소의 개수는 원본의 개수 포인터에 해당하는 `{ value }` 상자라 그리기 중 바뀐 값을 매번 다시 읽는다.
//  - collectDrawEntries(request, work, hooks): 용량을 5,000 이하로 제한해 UI → 건설 표시 → 묶음 → 일반 자산/효과 → 미리보기 순으로 병합한다. 수집기는 남은 칸(부분 배열)과 한도만 받고 실제 기록 수를 돌려준다
//    (음수·남은 용량 초과·정수가 아닌 반환은 예외, 그 앞 기록은 남을 수 있다). 비어 있는 수집기는 건너뛰고 연결된 수집기는 남은 용량 0 에서도 원본 순서대로 부른다. 요청 dirtyFlags 의 비트 2 는 UI 만 수집한다.
//    묶음은 첫 항목의 x·y·depth 와 헤더 토큰을 새 항목으로 복사하고 kind 만 1 로 바꾸며 마지막 BYTE(tail) 는 쓰지 않는다. 빈 묶음 저장소·개수 상자 누락·남은 용량 초과는 예외로 거부한다.
//  - dispatchDrawEntries(entries, state, hooks): 정렬된 상위 목록을 종류별로 제출한다 — kind 0 은 sprite(항목 전체 보기), 1 은 묶음 내부를 매 차례 현재 개수로 sprite 에, 2·3 은 ui·construction(토큰), 4 는 preview(항목 전체),
//    알 수 없는 kind 는 그리기만 건너뛴다. state.debugAfterEach 이면 상위 항목마다 한 번(알 수 없는 kind 포함, 묶음 내부 항목마다가 아님) afterEach 를 부른다. hideGroups·debugAfterEach 는 매 항목에서 현재 값을 읽는다.
//    사용하는 종류의 필수 콜백이나 묶음 저장소가 없으면 예외로 거부한다(쓰지 않는 종류는 요구하지 않는다). 정렬은 호출자가 별도로 수행한다(sortRenderEntries).
//  - sortRenderEntries(entries): 16바이트 목록 전체를 원본 표시 비교(`draw-sort.js` 의 CRT qsort/shortsort 복원)로 제자리 정렬한다. 항목 전체 16바이트가 함께 움직이므로 kind·tail 이 보존된다.
//    유한 float32 좌표와 signed WORD 깊이만 받으며 아니면 교환 전에 거부한다.
// 요청 구조: { screen: { left, top, right, bottom }(정수 화면 좌표), visible, unclipped: { left, top, right, bottom }(월드 float), dirtyFlags, groupRowLimit }. 수집기 훅: ui(screen, output, limit)·construction(visible, output, limit)·objects(visible, output, limit)·
//    preview(unclipped, output, limit) → 기록 수, groups(visible, groupRowLimit, covered{ value }) → [{ token, entries(Uint8Array), count({ value }) }]. 분배 훅: sprite(entry)·group(token)→묶음 보기·ui(token)·construction(token)·preview(entry)·afterEach().
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E382: 5-A 렌더러 프레임 본체의 순수 수집·분배 단위로 추가, 기준선 뒤 cpppj 변경 없음)
import { RENDER_ENTRY_BYTES } from '../core/raw-render-collection.js';
import { sortDrawEntries } from './draw-sort.js';

// 원본 공통 작업 배열은 16바이트 표시 항목 5,000개이다. (2026-11-13 WB04-E382)
export const DRAW_SUBMISSION_CAPACITY = 5000;

// 항목 안의 바이트 위치: x(0)·y(4)·token(8)·depth(12)·kind(14)·tail(15). (2026-11-13 WB04-E382)
const TOKEN = 8;
const DEPTH = 12;
const KIND = 14;
// 상위 항목 종류: 0 sprite·1 묶음·2 UI·3 건설 표시·4 미리보기. (2026-11-13 WB04-E382)
const KIND_SPRITE = 0;
const KIND_GROUP = 1;
const KIND_UI = 2;
const KIND_CONSTRUCTION = 3;
const KIND_PREVIEW = 4;
// 묶음 항목의 앞 8바이트(x·y)와 depth 2바이트 위치이다. (2026-11-13 WB04-E382)
const POSITION_BYTES = 8;

// 목록에 담긴 항목 수이다(16바이트 미만 꼬리는 항목이 아니다). (2026-11-13 WB04-E382)
const entryCount = (bytes) => Math.floor(bytes.length / RENDER_ENTRY_BYTES);

// 연결된 종류의 그리기 경계가 빠졌다면 빈 함수를 호출하기 전에 계약 오류를 알려 준다(C++ Required). (2026-11-13 WB04-E382)
function required(hook) {
    if (typeof hook !== 'function') throw new TypeError('공통 표시 분배 콜백 누락');
    return hook;
}

// 항목 하나를 부모 목록과 같은 저장소를 가리키는 16바이트 보기로 만든다(훅이 같은 항목인지 식별할 수 있다). (2026-11-13 WB04-E382)
const entryView = (bytes, index) => bytes.subarray(index * RENDER_ENTRY_BYTES, (index + 1) * RENDER_ENTRY_BYTES);

// 실제 수집기는 남은 칸만 전달받으며 음수/용량 밖 반환은 다음 수집기로 넘기지 않는다. 반환: { count, backgroundCovered }. 사용: collectDrawEntries(request, work, hooks). (2026-11-13 WB04-E382)
export function collectDrawEntries(request, work, hooks) {
    // 작업 배열은 5,000 항목까지만 쓴다.
    const capacity = Math.min(entryCount(work), DRAW_SUBMISSION_CAPACITY);
    const result = { count: 0, backgroundCovered: false };
    // 빈 수집기도 건너뛰되 연결된 수집기는 남은 용량 0 에서 호출되는 원본 순서를 유지한다.
    const append = (hook, rectangle) => {
        if (!hook) return;
        const remaining = capacity - result.count;
        const written = hook(rectangle, work.subarray(result.count * RENDER_ENTRY_BYTES, capacity * RENDER_ENTRY_BYTES), remaining);
        if (!Number.isInteger(written) || written < 0 || written > remaining) throw new RangeError('표시 수집 반환 개수 오류');
        result.count += written;
    };
    append(hooks.ui, request.screen);
    if (request.dirtyFlags & 4) return result;
    append(hooks.construction, request.visible);
    if (hooks.groups) {
        const covered = { value: false };
        const groups = hooks.groups(request.visible, request.groupRowLimit, covered);
        result.backgroundCovered = Boolean(covered.value);
        if (groups.length > capacity - result.count) throw new RangeError('표시 묶음 용량 초과');
        // 묶음의 첫 키(x·y·depth)와 헤더 토큰을 복사하며 사용하지 않는 마지막 바이트는 쓰지 않는다.
        for (const group of groups) {
            if (entryCount(group.entries) === 0 || !group.count) throw new TypeError('표시 묶음 저장소 누락');
            const base = result.count * RENDER_ENTRY_BYTES;
            work.set(group.entries.subarray(0, POSITION_BYTES), base);
            work.set(group.entries.subarray(DEPTH, DEPTH + 2), base + DEPTH);
            new DataView(work.buffer, work.byteOffset, work.length).setUint32(base + TOKEN, group.token >>> 0, true);
            work[base + KIND] = KIND_GROUP;
            result.count += 1;
        }
    }
    append(hooks.objects, request.visible);
    append(hooks.preview, request.unclipped);
    return result;
}

// 정렬된 상위 목록을 종류별로 제출한다. 알 수 없는 kind 는 원본 switch 처럼 그리기만 건너뛴다. 사용: dispatchDrawEntries(entries, state, hooks). (2026-11-13 WB04-E382)
export function dispatchDrawEntries(entries, state, hooks) {
    const view = new DataView(entries.buffer, entries.byteOffset, entries.length);
    const total = entryCount(entries);
    // 원본은 현재 목록의 항목 수만 처리하며 그리는 도중 새로 수집하지 않는다.
    for (let index = 0; index < total; index += 1) {
        const entry = entryView(entries, index);
        const token = view.getUint32(index * RENDER_ENTRY_BYTES + TOKEN, true);
        switch (entries[index * RENDER_ENTRY_BYTES + KIND]) {
        case KIND_SPRITE: required(hooks.sprite)(entry); break;
        case KIND_GROUP:
            if (!state.hideGroups) {
                const group = required(hooks.group)(token);
                if (!group.count) throw new TypeError('표시 묶음 개수 저장소 누락');
                // 그리기 콜백이 현재 개수를 줄이거나 늘릴 수 있다. 원래 배열 안에서만 계속 진행한다.
                for (let member = 0; member < group.count.value; member += 1) {
                    if (member >= entryCount(group.entries)) throw new RangeError('표시 묶음 개수 초과');
                    required(hooks.sprite)(entryView(group.entries, member));
                }
            }
            break;
        case KIND_UI: required(hooks.ui)(token); break;
        case KIND_CONSTRUCTION: required(hooks.construction)(token); break;
        case KIND_PREVIEW: required(hooks.preview)(entry); break;
        default: break;
        }
        if (state.debugAfterEach) required(hooks.afterEach)();
    }
}

// 16바이트 항목 목록을 원본 표시 비교로 제자리 정렬한다(CRT 피벗/교환 순서 포함). 항목 전체가 함께 움직여 kind·tail 이 보존된다. (2026-11-13 WB04-E382)
export function sortRenderEntries(entries) {
    const view = new DataView(entries.buffer, entries.byteOffset, entries.length);
    const total = entryCount(entries);
    // 항목마다 키(x·y·depth)와 원본 바이트 사본을 가진 정렬 대상으로 바꾼다.
    const items = Array.from({ length: total }, (_, index) => {
        const base = index * RENDER_ENTRY_BYTES;
        return { x: view.getFloat32(base, true), y: view.getFloat32(base + 4, true), depth: view.getInt16(base + DEPTH, true), raw: entries.slice(base, base + RENDER_ENTRY_BYTES) };
    });
    sortDrawEntries(items);
    // 정렬이 거부하면 위에서 예외가 나므로 목록은 바뀌지 않는다. 성공하면 정렬된 순서대로 되쓴다.
    items.forEach((item, index) => { entries.set(item.raw, index * RENDER_ENTRY_BYTES); });
    return entries;
}
