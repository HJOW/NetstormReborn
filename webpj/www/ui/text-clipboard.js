// 원문 004dd810/004dd870 의 CF_TEXT 읽기/쓰기와 GlobalAlloc/Lock 순서를 명시한 장치에 연결한다(cpppj client/TextClipboard.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 공통 TextEntry 가 빌리는 clipboard 의 원문 부모다. CF_UNICODETEXT 변환이나 새 줄/문자열 정규화를 더하지 않고, 실제 OS 반환을 성공으로 바꾸지 않으며 원문의 검사/무시/Close 위치를 유지한다.
// 웹 적응: cpppj 의 Win32TextClipboardDevices(User32/Kernel32 직접 호출)는 이식하지 않는다. 브라우저 clipboard 는 비동기이므로 호스트가 붙여넣기 이벤트에서 얻은 문자열 스냅숏과 쓰기 함수만 주면
// createWebTextClipboardDevices 가 같은 아홉 장치 호출 순서(형식/Open/Get/Lock/Unlock/Empty/Alloc/Set/Close)로 에뮬레이트한다. 글자는 Windows-1252 바이트이며 전용 PE fixture 는 없다.
// 사용: const clipboard = new TextClipboard(createWebTextClipboardDevices({ snapshot: () => pasteText, store: (text) => navigator.clipboard.writeText(text) })); bindTextClipboard(context, clipboard).
// (2026-11-14 WB04-E439: RawMenuControls 선행 clipboard 이식)
import { decodeWindows1252, encodeWindows1252 } from '../assets/text-encoding.js';

// CF_TEXT 형식 번호이며 UTF-8/Unicode 변환을 요청하지 않는다.
const TEXT_FORMAT = 1;
// 원문 GlobalAlloc 의 GMEM_DDESHARE 비트다. MOVEABLE 을 새로 더하지 않는다.
const ALLOCATE_FLAGS = 0x2000;
// 호스트 할당/출력 크기의 상한(32비트 부호 없는 수)이다.
const MAX_DWORD = 0xffffffff;

// 선택된 장치 호출을 도달한 위치에서만 요구한다(누락은 Error, logic_error).
function need(device) {
    if (typeof device !== 'function') throw new Error('TextClipboard의 선택된 실제 OS 장치 미연결');
    return device;
}

// 원문 clipboard 부모다. devices 의 아홉 호출은 실제 OS 반환을 원문이 검사하는 위치에서만 해석한다:
// formatAvailable(1)→비영, open(0)→비영(원문 HWND 는 고정 NULL), get(1)→HANDLE, lock(handle)→Uint8Array|null, unlock(handle)·empty()는 반환 무시, allocate(0x2000, 길이+1)→HANDLE, set(1, handle)은 반환 무시, close().
export class TextClipboard {
    // 창이나 Config 보다 독립적인 실제 OS/검사 장치의 수명이다.
    #devices;

    // 소유만 넘기며 아직 OS 효과를 만들지 않는다. 선택한 하위만 호출할 때 요구한다.
    constructor(devices) { this.#devices = devices; }

    // 원문 최대 256바이트 호출 외에도 같은 명시 capacity 를 받는다. output[0] 을 먼저 0 으로 쓰고, 형식/Open 실패면 그대로 돌아오며(Close 도 하지 않음), 데이터가 있으면 strncpy 처럼 첫 NUL 앞만 복사하고 남은 capacity 는 0 으로 채운다.
    // capacity 가 비었거나 2^32 이상이면 RangeError 다. 가득 찬 비 NUL 은 마지막 바이트를 따로 덮지 않는다. Open 에 성공한 정상 경로는 항상 Close 로 끝난다.
    read(output) {
        if (output.length === 0 || output.length > MAX_DWORD) throw new RangeError('TextClipboard 읽기의 실제 출력 물리 범위');
        output[0] = 0;
        if (!need(this.#devices.formatAvailable)(TEXT_FORMAT)) return;
        if (!need(this.#devices.open)(0)) return;
        const handle = need(this.#devices.get)(TEXT_FORMAT);
        if (handle) {
            const source = need(this.#devices.lock)(handle);
            if (source) {
                let index = 0;
                // strncpy 와 같은 첫 NUL 뒤 0 padding 이며 capacity 이하 바이트만 복사한다(source 가 짧으면 NUL 로 본다).
                for (; index < output.length && source[index]; index += 1) output[index] = source[index];
                for (; index < output.length; index += 1) output[index] = 0;
                need(this.#devices.unlock)(handle);
            }
        }
        need(this.#devices.close)();
    }

    // 선택한 본문 길이에 첫 NUL 절단을 더하지 않고 명시 길이를 복사한 뒤 별도 마지막 NUL 을 한 바이트 쓴다. Open 실패면 아무 효과 없이 돌아오고, 할당 실패면 Close 만 하고 돌아오며,
    // Lock 이 NULL 이면 원문이 그 위치에서 접근 실패하므로 RangeError 다(성공/Close 로 위장하지 않는다). 성공은 Unlock→Set(1, handle)→Close 순서다. input 은 바이트 배열이다.
    write(input) {
        if (input.length >= MAX_DWORD) throw new RangeError('TextClipboard 쓰기의 호스트 할당 물리 범위');
        if (!need(this.#devices.open)(0)) return;
        need(this.#devices.empty)();
        const handle = need(this.#devices.allocate)(ALLOCATE_FLAGS, input.length + 1);
        if (!handle) { need(this.#devices.close)(); return; }
        const target = need(this.#devices.lock)(handle);
        if (!target) throw new RangeError('원문 TextClipboard 쓰기의 NULL GlobalLock 물리 접근');
        if (input.length) target.set(input);
        target[input.length] = 0;
        need(this.#devices.unlock)(handle); need(this.#devices.set)(TEXT_FORMAT, handle); need(this.#devices.close)();
    }
}

// 브라우저용 아홉 장치 에뮬레이터다. host = { snapshot(): 문자열|null (읽을 수 있는 clipboard 문자열 스냅숏, 없으면 null), store(text: 문자열) (쓰기 완료된 문자열 전달) }.
// Open/Close 는 항상 성공하고, Get 은 스냅숏이 있으면 핸들 1 을 주며 Lock 은 Windows-1252 바이트 + NUL 이다. Allocate 는 새 핸들(2부터)과 길이 버퍼를 만들고 Set 은 그 NUL 앞 바이트를 문자열로 해독해 store 로 넘긴다.
export function createWebTextClipboardDevices(host) {
    // 확보한 쓰기 버퍼(핸들 → Uint8Array)와 다음 핸들 번호다.
    const buffers = new Map(); let next = 2;
    return {
        formatAvailable: () => { const text = host.snapshot(); return text === null || text === undefined ? 0 : 1; },
        open: () => 1,
        get: () => 1,
        lock: (handle) => {
            if (handle === 1) { const text = host.snapshot(); return Uint8Array.from([...encodeWindows1252(text ?? ''), 0]); }
            return buffers.get(handle) ?? null;
        },
        unlock: () => 0,
        empty: () => 1,
        allocate: (flags, length) => { const handle = next; next += 1; buffers.set(handle, new Uint8Array(length)); return handle; },
        set: (format, handle) => {
            const bytes = buffers.get(handle); buffers.delete(handle);
            if (bytes) { const end = bytes.indexOf(0); host.store(decodeWindows1252(bytes.subarray(0, end < 0 ? bytes.length : end))); }
            return handle;
        },
        close: () => 1,
    };
}

// 편집 UI 문맥에 같은 실제 장치 수명을 공유하는 두 callback 을 설치한다(다른 now/Console callback 은 보존). clipboard 가 없으면 TypeError 다.
export function bindTextClipboard(context, clipboard) {
    if (!clipboard) throw new TypeError('TextEntry clipboard의 실제 수명 소유자 누락');
    context.clipboardRead = (bytes) => clipboard.read(bytes);
    context.clipboardWrite = (bytes) => clipboard.write(bytes);
    return context;
}
