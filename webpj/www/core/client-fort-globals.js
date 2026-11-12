// 현재/예약(pending) Fort 이름 두 개와 파일 키·목록 dirty·선택 모드를 Client 수명으로 한 곳에 보관하는 모듈이다(cpppj `client/ClientFortGlobals.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 변경 없음).
// 원문: 현재 이름 버퍼 0055b490 이 가리키는 256BYTE, 예약 이름 0055b494 의 별개 256BYTE(서로 복사하지 않음), 파일 키 005327ac 의 9BYTE(기본 "fortSpec"+NUL, "SOLOSpec"+NUL 로 바뀔 수 있음)와 선택 포인터 005327e0,
//   목록 캐시 dirty 00542678(최초 1), 선택 모드 005cae84(최초 0). 메뉴와 짧은 Session 이 같은 이름/키/목록 물리를 **빌린다**: 값 사본을 만들지 않고 임의 초기화하지도 않는다.
// 사용: const fort = new ClientFortGlobals(); fort.currentBytes().set(bytes); fort.current(); fort.fileSpec().selectSolo(); fort.selectFileSpec(); fort.selectedFileSpec(); fort.selectionMode().value = 1;
// 이름 읽기는 첫 NUL 까지이고 NUL 뒤 미작성 바이트는 읽지 않으며 NUL 이 없으면 물리 범위 오류이다(미종료 버퍼를 빈 문자열로 만들지 않음). 이름은 원문 ANSI 바이트 그대로 한 글자 = 한 바이트(latin1)로 읽는다.
// 웹 범위: 목록 캐시 `Catalog()` 의 50행 목록(`SoloMapListState`, 단독/Edit 맵 목록 — `exclude-edit` 인 `SoloMapCatalog`/`RawSoloMapList` 소유)은 옮기지 않고, 이름·설정과 같은 dirty DWORD 한 칸만
//   `catalogDirty()` 상자로 둔다(최초 1, 쓰기는 DWORD 로 감김). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E331: 4-D ClientFortGlobals 이식)
import { DwordBox } from './client-subscriber-globals.js';

// 현재/예약 Fort 이름 버퍼의 원문 크기(BYTE)이다. (2026-11-12 WB04-E331)
export const FORT_NAME_BYTES = 256;

// 파일 키 버퍼의 원문 크기(8문자 + NUL)이다. (2026-11-12 WB04-E331)
export const FILE_SPEC_BYTES = 9;

// 원본 005327ac 의 같은 9BYTE 를 덮어쓰는 파일 키 저장소이다. Client 가 소유하면 모든 목록 경계가 같은 버퍼를 빌린다.
// 사용: spec.current() → "fortSpec"; spec.selectSolo(); spec.current() → "SOLOSpec"; spec.bytes 는 키가 바뀌어도 같은 배열이다. (2026-11-12 WB04-E331)
export class SoloMapFileSpec {
    // 기본 원본 키 "fortSpec" 과 마지막 NUL 을 같은 고정 할당에 둔다.
    #bytes = Uint8Array.from([0x66, 0x6f, 0x72, 0x74, 0x53, 0x70, 0x65, 0x63, 0x00]);

    // 같은 버퍼의 읽기/쓰기 배열이다(키가 바뀌어도 주소가 같음). (2026-11-12 WB04-E331)
    get bytes() { return this.#bytes; }

    // 현재 8문자 키를 읽는다(첫 NUL 앞까지). (2026-11-12 WB04-E331)
    current() { return nameOf(this.#bytes); }

    // 같은 버퍼를 "fortSpec"+NUL 로 복구한다. 목록 dirty 는 별도 원본 호출자가 설정한다. (2026-11-12 WB04-E331)
    selectFort() { this.#write('fortSpec'); }

    // 같은 버퍼를 "SOLOSpec"+NUL 로 바꾼다. 캐시를 자동 무효화하지 않는다. (2026-11-12 WB04-E331)
    selectSolo() { this.#write('SOLOSpec'); }

    // 8문자 키와 NUL 을 같은 버퍼에 쓴다. (2026-11-12 WB04-E331)
    #write(key) {
        // 키 문자를 한 바이트씩 쓴다.
        for (let index = 0; index < key.length; index += 1) this.#bytes[index] = key.charCodeAt(index);
        this.#bytes[key.length] = 0;
    }
}

// 첫 NUL 앞까지를 한 글자 = 한 바이트로 읽는다. NUL 이 없으면 물리 범위 오류이다. (2026-11-12 WB04-E331)
function nameOf(bytes) {
    const end = bytes.indexOf(0);
    if (end < 0) throw new RangeError('Client Fort 이름의 NUL 물리 범위');
    return String.fromCharCode(...bytes.subarray(0, end));
}

// 원문 두 이름 버퍼와 파일 키 포인터·목록 dirty·모드의 단일 소유자이다. 접근자는 호출마다 같은 객체를 돌려준다(주소 고정). (2026-11-12 WB04-E331)
export class ClientFortGlobals {
    // 0055b490 이 가리키는 현재 이름 256BYTE 이다. 원문 두 할당은 각 첫 BYTE 만 쓰므로 나머지는 미작성이다(웹은 0).
    #current = new Uint8Array(FORT_NAME_BYTES);
    // 0055b494 의 예약 이름 256BYTE 이다. 현재 이름에 복사하지 않는다.
    #pending = new Uint8Array(FORT_NAME_BYTES);
    // 005327ac 의 파일 키와 005327e0 의 선택 포인터(웹은 키가 하나라 항상 이 키)이다.
    #fileSpec = new SoloMapFileSpec();
    #selectedFileSpec = this.#fileSpec;
    // 00542678 의 목록/설정 dirty(보호 PE 최초 1)와 005cae84 의 선택 모드(최초 0)이다.
    #catalogDirty = new DwordBox(1);
    #selectionMode = new DwordBox(0);

    // 원문 setter 의 같은 목적 주소(현재 이름 쓰기 버퍼)이다. (2026-11-12 WB04-E331)
    currentBytes() { return this.#current; }

    // 다음 상태가 나중에 읽는 별개 목적 주소(예약 이름 버퍼)이다. (2026-11-12 WB04-E331)
    pendingBytes() { return this.#pending; }

    // 현재 이름을 NUL 까지 읽는다(미종료는 범위 오류). (2026-11-12 WB04-E331)
    current() { return nameOf(this.#current); }

    // 예약 이름을 NUL 까지 읽는다. (2026-11-12 WB04-E331)
    pending() { return nameOf(this.#pending); }

    // 005327ac 의 같은 9BYTE 키이다. SelectSolo/SelectFort 로 내용이 바뀌어도 같은 객체/배열이다. (2026-11-12 WB04-E331)
    fileSpec() { return this.#fileSpec; }

    // 005327e0 을 같은 고정 키로 지정한다(포인터 선택과 키 내용 변경을 구별, 내용 보존). (2026-11-12 WB04-E331)
    selectFileSpec() { this.#selectedFileSpec = this.#fileSpec; }

    // 현재 선택 포인터의 키를 경로 작성 위치에서 다시 읽는다. (2026-11-12 WB04-E331)
    selectedFileSpec() { return this.#selectedFileSpec.current(); }

    // 목록과 이름/설정이 함께 쓰는 00542678 dirty 상자이다(최초 1). 50행 목록 캐시는 Edit 맵 목록 소유라 웹에 없다. (2026-11-12 WB04-E331)
    catalogDirty() { return this.#catalogDirty; }

    // 005cae84 의 현재 ReadFort/AskEditFort 모드 상자이다. (2026-11-12 WB04-E331)
    selectionMode() { return this.#selectionMode; }
}
