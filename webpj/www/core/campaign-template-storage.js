// Client 와 Session 이 함께 쓰는 Template9(캠페인/미션 템플릿 9칸)의 실제 manager 이미지·State cursor 수명 소유자를 옮긴 모듈이다(cpppj `client/CampaignTemplateStorage.cpp/.h`, 10.78).
//  이 파일은 **1부(WB04-E345)**: 9개 20BYTE 헤더, manager 이미지 표, 빈 manager 할당/생성/초기화, 설치·빌림·반납, 섹션 노드 목록(60BYTE 노드·MemStream 16BYTE·다음 포인터)의 생성/검색/이름 교체/최소 확장/쓰기,
//  State·Territory·Money·Badges·Technology 의 typed cursor 공개와 선택, 구독자 이름 읽기, State 확장 복사, **2부(WB04-E346)**: 파일 읽기(ReadFile·BindFileStreams)와 원문 Load 부모(LoadFile)·InstallFile. 로그인 이미지 복제·저장은 3부(E347)에서 같은 클래스에 더한다.
//  - 모델: manager 는 276BYTE(`+0` 가상표·`+4` head·`+8` current·`+12` 파일명 256BYTE·`+0x10c` 서명 판정·`+0x110` FE 판정)이고, 섹션은 60BYTE 노드(`+0` 버퍼 토큰·`+4` pos·`+8` end·`+12` 할당·`+16` 이름 40BYTE·`+0x38` next)이다.
//    노드 토큰·manager 핸들은 호스트 포인터가 아닌 불투명 번호다(버퍼 토큰 = 0x10000000 + 노드 핸들, 임시 할당 토큰 = 0x20000000 + 핸들). 표준 35개 이름은 `fort.sections` 의 위치를 쓰고 그 밖의 이름(Temp 등)은 노드가 소유한 별도 버퍼를 쓴다.
//  - 버퍼는 `ByteVector`(std::vector<uint8_t> 의 size/capacity 의미: resize 는 늘릴 때 새 칸을 0 으로 채우고 용량 안에서는 같은 저장소를 쓰며, 이동/교환은 원본을 비움)로 소유하고, 표준 위치의 변화는 `fort.sections[i]` 에 현재 논리 크기의 뷰로 반영한다.
//  - 표기: 반환 `TemplateWriteStream` = `{ manager, section, generation }`(같은 이름 재생성 뒤 이전 참조로 쓰는 것을 거부하는 생성 수명). cursor 는 State/Badges/Technology `{ bytes, position, end, present }`, Territory `{ bytes, position, end }`, Money `{ position, end }`.
//  - 제외: `BindCurrentName`(Editor 전용 `RawFortCurrentName`)은 옮기지 않는다. 기준선 뒤 변경(+41/−8줄 .cpp, +18줄 .h: 원격 수신 `ReadImage` 와 파일 읽기의 이미지 장치 공용화)은 3.6절 ①(결과 동일)/MP 이며 웹은 기준선 판을 옮긴다.
//  이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E345: 4-D B 구획 CampaignTemplateStorage 1부 이식)
import { SECTION_NAMES } from '../assets/fort.js';
import { FortSerializationGlobals } from './fort-serialization-globals.js';
import { ClientTemplateSectionNames } from './client-template-section-names.js';
import { compareDataManagerSectionNames, findDataManagerSection, selectDataManagerSection } from './raw-data-manager-selection.js';
import { createDataManagerStream, ensureDataManagerStreamSize, renameDataManagerStream } from './raw-data-manager-resize.js';
import { destroyDataManager } from './raw-data-manager-lifetime.js';
import { initializeDataManager } from './raw-data-manager-initialize.js';
import { compactMemStream, writeMemStream } from './raw-mem-stream.js';
import { readDataManagerFile, readTemplateFileSignature } from './raw-data-manager-image.js';
import { constructTemplateDataManager, loadTemplateFile, templateDataManagerFileName } from './raw-template-load.js';
import { clearTemplateHeaders, initializeTemplateHeader, releaseTemplateHeader } from './raw-template-lifetime.js';

// 고정 크기와 위치 상수이다. (2026-11-12 WB04-E345)
const HEADER_BYTES = 20; // Template 헤더 한 칸.
const SLOT_COUNT = 9; // Template 칸 수.
const MANAGER_BYTES = 276; // manager 이미지 크기.
const NODE_BYTES = 60; // 섹션 노드 크기.
const SECTION_COUNT = SECTION_NAMES.length; // 표준 섹션 수(35).
const BUFFER_TOKEN_BASE = 0x10000000; // 노드 버퍼 토큰 기준.
const FILE_TOKEN = 0x30000; // 파일 읽기 부모가 보유한 읽은 파일 할당의 불투명 토큰.
const TEMPORARY_TOKEN_BASE = 0x20000000; // 임시 할당 토큰 기준.
const NAME_OFFSET = 16; // 노드 안 이름 위치.
const NAME_BYTES = 40; // 노드 안 이름 영역 크기.
const NEXT_OFFSET = 0x38; // 노드 안 next 위치.
const STATE_BYTES_MINIMUM = 4; // 구독자 ID 한 DWORD.
const SIGNED_LENGTH_LIMIT = 0x8000; // 구독자 이름 길이의 signed 한도.
const SUBSCRIBER_NAME_LIMIT = 256; // 구독자 이름 출력 한도(NUL 자리 포함).

// 비정렬 DWORD(리틀 엔디언)를 같은 저장소에서 읽는다. (2026-11-12 WB04-E345)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 지정 DWORD 의 네 BYTE 에만 쓴다(인접 필드 보존). (2026-11-12 WB04-E345)
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// manager 의 현재 파일 이름(+12, 첫 NUL 앞)을 한 글자 한 바이트 문자열로 읽는다. (2026-11-13 WB04-E346)
function nameOfManager(manager) {
    return String.fromCharCode(...templateDataManagerFileName(manager));
}

// 선택된 파일 장치 같은 효과를 호출한 자리에서만 요구한다. (2026-11-12 WB04-E345)
function need(effect, message) {
    if (typeof effect !== 'function') throw new Error(message);
    return effect;
}

// std::vector<uint8_t> 의 size/capacity/move/swap 의미를 가진 소유 버퍼이다(시험이 resize/take 의미를 직접 확인하도록 내보낸다). `bytes` 는 현재 논리 크기(size)의 살아 있는 뷰이다.
// 용량 안의 resize 는 같은 저장소를 쓰며(늘린 칸은 0), 용량을 넘으면 새 저장소로 옮긴다(용량 = 새 크기). onChange 는 표준 위치의 `fort.sections[i]` 뷰를 새로 맞추는 선택 콜백이다. (2026-11-12 WB04-E345)
export class ByteVector {
    // 용량만큼의 저장소와 현재 논리 크기, 변경 통지이다.
    #store;
    #size;
    #onChange;

    // 초기 내용과 변경 통지로 만든다. adopt 이면 같은 저장소를 그대로 인수하고(이동 의미: 기존 `fort.sections[i]` 와 같은 메모리), 아니면 복사한다. 초기 내용이 없으면 빈 벡터이다. (2026-11-12 WB04-E345)
    constructor(initial = null, onChange = null, adopt = false) {
        this.#store = initial ? (adopt ? initial : Uint8Array.from(initial)) : new Uint8Array(0);
        this.#size = this.#store.length;
        this.#onChange = onChange;
    }

    // 현재 논리 크기이다.
    get size() { return this.#size; }

    // 할당된 용량이다(비면 0).
    get capacity() { return this.#store.length; }

    // 현재 논리 크기의 뷰(같은 저장소)이다.
    get bytes() { return this.#store.subarray(0, this.#size); }

    // 크기를 바꾼다: 용량 안이면 같은 저장소에서 늘린 칸을 0 으로 채우고, 넘으면 새 저장소로 옮긴다. (2026-11-12 WB04-E345)
    resize(size) {
        if (size <= this.#store.length) {
            if (size > this.#size) this.#store.fill(0, this.#size, size);
            this.#size = size;
        } else {
            const next = new Uint8Array(size);
            next.set(this.#store.subarray(0, this.#size));
            this.#store = next;
            this.#size = size;
        }
        this.#onChange?.();
    }

    // std::vector{}.swap(v): 저장소를 모두 놓는다. (2026-11-12 WB04-E345)
    release() {
        this.#store = new Uint8Array(0);
        this.#size = 0;
        this.#onChange?.();
    }

    // 이동 대입: source 의 저장소와 크기를 가져오고 source 는 비운다. (2026-11-12 WB04-E345)
    take(source) {
        this.#store = source.#store;
        this.#size = source.#size;
        source.#store = new Uint8Array(0);
        source.#size = 0;
        source.#onChange?.();
        this.#onChange?.();
    }
}

// 섹션 노드 하나이다: 60BYTE 노드 물리, 표준 위치(없으면 동적), 동적 이름의 소유 버퍼, 생성 수명, 인라인 쓰기 빌림 표시. (2026-11-12 WB04-E345)
function createStream(canonical, generation) {
    return { bytes: new Uint8Array(NODE_BYTES), canonical, extra: new ByteVector(), generation, accessing: false };
}

// 표준 35개 이름 중 같은 이름(ASCII 대소문자 무시)의 위치를 찾는다. 없으면 null. (2026-11-12 WB04-E345)
function canonicalSection(name) {
    // 이름표 순서대로 비교한다.
    for (let index = 0; index < SECTION_NAMES.length; index += 1) if (compareDataManagerSectionNames(SECTION_NAMES[index], name) === 0) return index;
    return null;
}

// 표준 이름의 위치를 찾고 없으면 거부한다. (2026-11-12 WB04-E345)
function requiredSection(name) {
    const index = canonicalSection(name);
    if (index === null) throw new Error(`Template 실제 ${name} 섹션 이름 누락`);
    return index;
}

// 하나의 manager 를 빌린 여러 헤더도 이 실제 이미지/커서를 공유한다. (2026-11-12 WB04-E345)
class Image {
    // 표준 위치의 ByteVector 들이다(위치 → 벡터).
    #vectors = [];
    // 생성 수명 번호와 노드 핸들의 단조 증가 값이다.
    nextStreamGeneration = 0;
    nextStreamHandle = 1;
    // 외부 파싱 자료의 각 표준 위치를 최초 목록에 한 번만 연결했는지이다.
    seeded = new Array(SECTION_COUNT).fill(false);
    // Client 와 같은 lazy 이름표이다.
    sectionNames;
    // 원문 생성자가 끝난 실제 이미지인지이다.
    constructed = false;
    // 파싱한 요새(`sections` 포함)와 같은 State/지형/경제/메타 cursor 이다.
    fort = { sections: [] };
    state = { bytes: new Uint8Array(0), position: 0, end: 0, present: false };
    territory = { bytes: new Uint8Array(0), position: 0, end: 0 };
    money = { position: 0, end: 0 };
    badges = { bytes: new Uint8Array(0), position: 0, end: 0, present: false };
    technology = { bytes: new Uint8Array(0), position: 0, end: 0, present: false };
    // 공개한 Money/Badges/Technology cursor 의 유효 비트(1/2/4)이다.
    loadCursors = 0;
    // 현재 typed cursor 를 소유한 노드 핸들이다.
    stateHandle = 0;
    territoryHandle = 0;
    moneyHandle = 0;
    badgesHandle = 0;
    technologyHandle = 0;
    // manager+12 의 현재 이름을 읽기 투영한 문자열이다.
    name = '';
    // State 확장 중 복사가 끝날 때까지 보유하는 이전 State 할당이다.
    retiredState = new ByteVector();
    // 원본 Template manager 276BYTE 물리이다.
    manager = new Uint8Array(MANAGER_BYTES);
    // 노드 핸들 → 노드, 그리고 원본 추가/교체 뒤 목록 순서이다.
    streams = new Map();
    streamOrder = [];

    // 빈 이미지(new276 만 한 상태)이다: 원문 ctor/목록 생성은 아직 하지 않고 선행 노드 투영도 막는다. 파싱된 요새가 있으면 State/Territory cursor 를 처음 공개한다. (2026-11-12 WB04-E345)
    constructor(names, fort = null, name = '', version = null, fileImage = false) {
        this.sectionNames = names;
        if (fort === null) {
            this.seeded.fill(true);
            return;
        }
        this.fort = fort;
        this.fort.sections ??= [];
        this.name = String(name);
        if (this.name.length >= 256 || this.name.includes('\0')) throw new TypeError('Template 실제 파일명256BYTE 계약 위반');
        // 실제 DWORD 객체의 BYTE 뷰를 만들어 생성자가 0 을 쓴 값을 판본 전역으로 되돌린다.
        constructTemplateDataManager(this.manager, this.name, version.view);
        version.commit();
        this.constructed = true;
        if (fileImage) return;
        const at = requiredSection('State');
        const stateBytes = at < this.fort.sections.length ? this.fort.sections[at] : new Uint8Array(0);
        Object.assign(this.state, { bytes: stateBytes, position: 0, end: stateBytes.length, present: stateBytes.length !== 0 });
        const territoryAt = requiredSection('Territory');
        const territoryBytes = territoryAt < this.fort.sections.length ? this.fort.sections[territoryAt] : new Uint8Array(0);
        Object.assign(this.territory, { bytes: territoryBytes, position: 0, end: territoryBytes.length });
        this.syncStreams();
    }

    // 원문 ctor 는 같은 생성 수명의 실제 manager 와 Client version 을 바꾼다. 이름표 할당은 하지 않는다. (2026-11-12 WB04-E345)
    constructName(file, version) {
        if (this.constructed || this.streams.size !== 0 || file.length >= 256 || file.includes('\0')) throw new TypeError('Template 빈Image 실제ctor계약');
        constructTemplateDataManager(this.manager, file, version.view);
        version.commit();
        this.name = file;
        this.constructed = true;
    }

    // 노드 핸들의 버퍼 토큰이다. 이름이 바뀌어도 노드가 같으면 같다. (2026-11-12 WB04-E345)
    static bufferToken(handle) {
        return BUFFER_TOKEN_BASE + handle;
    }

    // 같은 노드의 현재 실제 할당을 표준 위치 또는 동적 벡터에서 빌린다. (2026-11-12 WB04-E345)
    buffer(handle) {
        const stream = this.streams.get(handle);
        if (!stream) throw new RangeError('Template 실제 노드 핸들 범위');
        if (stream.canonical === null) return stream.extra;
        // 표준 위치가 아직 없으면 빈 칸으로 채워 위치를 보존한다(원문 `sections.resize(canonical+1)`).
        while (this.fort.sections.length <= stream.canonical) this.fort.sections.push(new Uint8Array(0));
        return this.#vector(stream.canonical);
    }

    // 표준 위치의 벡터를 만들거나 돌려준다. 변경은 `fort.sections[i]` 뷰에 반영된다. (2026-11-12 WB04-E345)
    #vector(index) {
        if (!this.#vectors[index]) {
            const initial = index < this.fort.sections.length ? this.fort.sections[index] : null;
            this.#vectors[index] = new ByteVector(initial, () => { this.#publishSection(index); }, true);
        }
        return this.#vectors[index];
    }

    // 벡터의 현재 논리 뷰를 `fort.sections[i]` 에 맞춘다(없는 칸은 빈 뷰로 채움). (2026-11-12 WB04-E345)
    #publishSection(index) {
        // 앞쪽 빈 칸을 빈 배열로 채워 위치를 보존한다.
        while (this.fort.sections.length <= index) this.fort.sections.push(new Uint8Array(0));
        this.fort.sections[index] = this.#vectors[index].bytes;
    }

    // 원본 60BYTE 에 저장된 현재 이름을 읽는다. handle 값으로 표준 종류를 추정하지 않는다. (2026-11-12 WB04-E345)
    streamName(handle) {
        const bytes = this.streams.get(handle).bytes.subarray(NAME_OFFSET, NAME_OFFSET + NAME_BYTES);
        const end = bytes.indexOf(0);
        if (end < 0) throw new Error('Template 실제 섹션 이름 NUL 누락');
        return String.fromCharCode(...bytes.subarray(0, end));
    }

    // 실제 60BYTE 객체를 할당하고 같은 생성 수명을 부여한다. 원본 생성자는 호출자가 별도로 수행한다. (2026-11-12 WB04-E345)
    allocateStream(canonical = null) {
        if (!this.nextStreamHandle || this.nextStreamHandle >= BUFFER_TOKEN_BASE || !(this.nextStreamGeneration + 1)) throw new RangeError('Template 동적 노드/생성 수명 소진');
        const handle = this.nextStreamHandle;
        this.nextStreamHandle += 1;
        this.nextStreamGeneration += 1;
        const stream = createStream(canonical, this.nextStreamGeneration);
        if (canonical !== null) this.seeded[canonical] = true;
        this.streams.set(handle, stream);
        return handle;
    }

    // 저장 중인 raw 빌림의 포인터/노드를 구조 변경으로 반납하지 않는다. (2026-11-12 WB04-E345)
    requireNoAccess() {
        // 현재 manager 의 모든 실제 노드를 검사한다.
        for (const stream of this.streams.values()) if (stream.accessing) throw new Error('Template raw 쓰기 빌림 중 섹션 구조 변경 금지');
    }

    // 같은 typed cursor 가 반납되거나 다른 이름으로 바뀌면 이전 위치/뷰를 새 노드에 쓰지 않는다. cursor 객체는 C++ 멤버처럼 같은 객체에 값을 대입해 빌린 참조가 살아 있다. (2026-11-12 WB04-E345)
    retireCursor(handle) {
        if (this.stateHandle === handle) { this.stateHandle = 0; Object.assign(this.state, { bytes: new Uint8Array(0), position: 0, end: 0, present: false }); }
        if (this.territoryHandle === handle) { this.territoryHandle = 0; Object.assign(this.territory, { bytes: new Uint8Array(0), position: 0, end: 0 }); }
        if (this.moneyHandle === handle) { this.moneyHandle = 0; Object.assign(this.money, { position: 0, end: 0 }); this.loadCursors &= ~1; }
        if (this.badgesHandle === handle) { this.badgesHandle = 0; Object.assign(this.badges, { bytes: new Uint8Array(0), position: 0, end: 0, present: false }); this.loadCursors &= ~2; }
        if (this.technologyHandle === handle) { this.technologyHandle = 0; Object.assign(this.technology, { bytes: new Uint8Array(0), position: 0, end: 0, present: false }); this.loadCursors &= ~4; }
    }

    // MemStream 소멸에서 요청한 같은 실제 raw 섹션 버퍼를 반납한다. (2026-11-12 WB04-E345)
    releaseBuffer(token) {
        if (token <= BUFFER_TOKEN_BASE || !this.streams.has(token - BUFFER_TOKEN_BASE)) throw new RangeError('Template 실제 버퍼 핸들 범위');
        this.buffer(token - BUFFER_TOKEN_BASE).release();
    }

    // 현재 노드의 data/pos/end 와 같은 raw 버퍼를 typed cursor 들에 다시 연결한다(같은 cursor 객체의 필드를 갱신). (2026-11-12 WB04-E345)
    publishCursor(handle) {
        const bytes = this.buffer(handle).bytes;
        const node = this.streams.get(handle).bytes;
        const sectionName = this.streamName(handle);
        const position = read(node, 4);
        const end = read(node, 8);
        const present = read(node, 0) !== 0;
        if (compareDataManagerSectionNames(sectionName, 'State') === 0) { this.stateHandle = handle; Object.assign(this.state, { bytes, position, end, present }); }
        if (compareDataManagerSectionNames(sectionName, 'Territory') === 0) { this.territoryHandle = handle; Object.assign(this.territory, { bytes, position, end }); }
        if (compareDataManagerSectionNames(sectionName, 'Money') === 0) { this.moneyHandle = handle; Object.assign(this.money, { position, end }); this.loadCursors |= 1; }
        if (compareDataManagerSectionNames(sectionName, 'Badges') === 0) { this.badgesHandle = handle; Object.assign(this.badges, { bytes, position, end, present }); this.loadCursors |= 2; }
        if (compareDataManagerSectionNames(sectionName, 'Technology') === 0) { this.technologyHandle = handle; Object.assign(this.technology, { bytes, position, end, present }); this.loadCursors |= 4; }
    }

    // 실제 임시 할당을 원본 포인터 토큰으로 빌린 뒤 완료한 같은 버퍼를 raw 섹션에 넘긴다. operation 은 MemStream hooks 를 받는다. (2026-11-12 WB04-E345)
    withMemory(handle, operation) {
        const allocation = new ByteVector();
        let allocated = false;
        const temporary = TEMPORARY_TOKEN_BASE + handle;
        const node = this.streams.get(handle).bytes;
        const hooks = {
            buffer: (pointer) => {
                if (pointer === temporary && allocated) return allocation.bytes;
                if (pointer <= BUFFER_TOKEN_BASE || !this.streams.has(pointer - BUFFER_TOKEN_BASE)) throw new RangeError('Template MemStream 실제 버퍼 핸들 범위');
                return this.buffer(pointer - BUFFER_TOKEN_BASE).bytes;
            }, // 옛 버퍼는 free 전까지 같은 실제 소유 벡터에서 읽는다.
            allocate: (size) => {
                if (allocated) throw new Error('Template MemStream 한 작업의 중복 할당');
                allocation.resize(size);
                allocated = true;
                return temporary;
            }, // 실제 임시 할당은 원본 본문이 0 으로 채운다.
            release: (pointer) => { if (pointer) this.releaseBuffer(pointer); }, // 같은 원시 섹션의 옛 할당만 반납한다.
            assertion: () => { throw new Error('Template 실제 MemStream 진단'); }, // 잘못된 저장 버퍼를 성공으로 대신하지 않는다.
        };
        operation(hooks);
        const buffer = this.buffer(handle);
        if (read(node, 0) === temporary && allocated) {
            buffer.take(allocation);
            put(node, 0, Image.bufferToken(handle));
        }
        const end = read(node, 8);
        if (end > buffer.size) throw new RangeError('Template MemStream 완료 뒤 실제 end 범위');
        buffer.resize(end);
        this.publishCursor(handle);
    }

    // 같은 stream 의 물리 할당을 노출하고 원본 쓰기 뒤 논리 end 만 원시 섹션에 남긴다. (2026-11-12 WB04-E345)
    writeStream(handle, input) {
        this.accessSection(handle, (fields, hooks) => { writeMemStream(fields, input, input.length, hooks); });
    }

    // 원본 ce10 이 새 노드를 붙이기 전에 마지막 MemStream 의 end 에 맞춰 할당을 줄인다. (2026-11-12 WB04-E345)
    compactTail(handle) {
        const node = this.streams.get(handle).bytes;
        if (read(node, 8) === read(node, 12)) return;
        this.withMemory(handle, (hooks) => { compactMemStream(this.streams.get(handle).bytes, hooks); });
    }

    // 실제 노드/버퍼 반납과 마지막 압축을 같은 원본 목록 처리기에 공급한다. (2026-11-12 WB04-E345)
    listEffects() {
        return {
            lifetime: {
                stream: (handle) => this.streams.get(handle).bytes, // 같은 현재 60BYTE 노드를 빌린다.
                releaseBuffer: (pointer) => { this.releaseBuffer(pointer); }, // 실제 raw 섹션 할당이다.
                releaseStream: (handle) => {
                    this.retireCursor(handle);
                    if (!this.streams.delete(handle)) throw new Error('Template 실제 노드 중복 반납');
                    this.streamOrder = this.streamOrder.filter((value) => value !== handle);
                }, // 버퍼 소멸을 마친 노드와 그 typed cursor 만 반납한다.
            },
            compact: (handle) => { this.compactTail(handle); },
            missingPredecessor: () => { throw new Error('Template 실제 선행 노드 진단'); },
        };
    }

    // 같은 이름의 기존 노드 unlink/버퍼 반납 뒤 이름과 독립적인 새 마지막 노드를 붙인다. (2026-11-12 WB04-E345)
    createSection(sectionName) {
        if (this.retiredState.size !== 0) throw new Error('State 확장 복사 중 새 섹션 생성 불가');
        this.requireNoAccess();
        this.syncStreams();
        let handle = 0;
        const allocation = new ByteVector();
        let allocated = false;
        const hooks = {
            selection: { stream: (stream) => this.streams.get(stream).bytes },
            list: this.listEffects(), // 검색/기존 제거/새 추가도 같은 실제 부모에서 수행한다.
            allocateStream: (size) => {
                if (size !== NODE_BYTES || handle) throw new Error('Template 새 섹션 실제 노드 할당 계약 위반');
                handle = this.allocateStream(canonicalSection(sectionName));
                return handle;
            },
            memory: {
                buffer: (pointer) => {
                    if (pointer === TEMPORARY_TOKEN_BASE + handle) return allocation.bytes;
                    if (pointer <= BUFFER_TOKEN_BASE || !this.streams.has(pointer - BUFFER_TOKEN_BASE)) throw new RangeError('Template 생성 부모의 실제 버퍼 범위');
                    return this.buffer(pointer - BUFFER_TOKEN_BASE).bytes;
                }, // 새 할당은 완료 전까지 별도 소유하고 기존 tail 압축은 원래 실제 버퍼를 쓴다.
                allocate: (size) => {
                    if (allocated) throw new Error('Template 생성 부모의 중복 할당');
                    allocation.resize(size);
                    allocated = true;
                    return TEMPORARY_TOKEN_BASE + handle;
                },
                assertion: () => { throw new Error('Template 생성 부모의 실제 MemStream 진단'); },
            },
        };
        const current = createDataManagerStream(this.manager, sectionName, hooks);
        const node = this.streams.get(handle).bytes;
        if (current !== handle || !allocated || read(node, 0) !== TEMPORARY_TOKEN_BASE + handle) throw new Error('Template 생성 부모의 실제 현재 섹션 불일치');
        put(node, 0, Image.bufferToken(handle));
        allocation.resize(read(node, 8));
        this.buffer(handle).take(allocation);
        this.streamOrder.push(handle);
        this.publishCursor(handle);
        return handle;
    }

    // 기존 State 생성 호출도 같은 일반 생성/원본 쓰기 부모를 사용한다. (2026-11-12 WB04-E345)
    initializeState(input) {
        const handle = this.createSection('State');
        this.writeStream(handle, input);
    }

    // 읽기 부모가 진행한 typed cursor 의 현재 위치를 같은 실제 노드에 반영한다. 생성 수명/current 선택은 바꾸지 않는다. (2026-11-12 WB04-E345)
    syncCursorPosition(handle) {
        const stream = this.streams.get(handle);
        if (stream.accessing) return; // 바깥 인라인 쓰기가 진행한 raw 위치를 아직 공개하지 않은 typed 위치로 되돌리지 않는다.
        const node = stream.bytes;
        if (handle === this.stateHandle) put(node, 4, this.state.position);
        if (handle === this.territoryHandle) put(node, 4, this.territory.position);
        if (handle === this.moneyHandle && (this.loadCursors & 1)) put(node, 4, this.money.position);
        if (handle === this.badgesHandle && (this.loadCursors & 2)) put(node, 4, this.badges.position);
        if (handle === this.technologyHandle && (this.loadCursors & 4)) put(node, 4, this.technology.position);
    }

    // 현재 선택을 바꾸지 않고 명시 노드에 인라인 쓰기를 적용한 뒤 논리 끝만 원시 섹션에 공개한다. operation(필드 16BYTE, MemStream hooks). (2026-11-12 WB04-E345)
    accessSection(handle, operation) {
        if (typeof operation !== 'function') throw new TypeError('Template 인라인 쓰기 장치 누락');
        const stream = this.streams.get(handle);
        if (stream.accessing) throw new Error('Template 같은 stream의 중첩 쓰기 빌림 금지');
        this.syncCursorPosition(handle);
        const buffer = this.buffer(handle);
        const node = stream.bytes;
        const allocation = read(node, 12);
        if (buffer.size < allocation) buffer.resize(allocation);
        stream.accessing = true;
        try {
            this.withMemory(handle, (hooks) => { operation(node.subarray(0, 16), hooks); });
        } catch (error) {
            stream.accessing = false;
            const end = read(node, 8);
            if (end <= buffer.size) buffer.resize(end);
            this.publishCursor(handle);
            throw error;
        } // 예외 전의 pos/end/부분 쓰기를 보존하며 물리 할당을 논리 자료로 잘못 공개하지 않는다.
        stream.accessing = false;
    }

    // 실제 이름 교체가 끝난 같은 노드의 할당을 새 표준 위치/동적 저장소로 이동한다. 바이트/할당을 복사하지 않는다. (2026-11-12 WB04-E345)
    bindRenamedBuffer(handle) {
        const stream = this.streams.get(handle);
        const destination = canonicalSection(this.streamName(handle));
        if (destination === stream.canonical) { this.publishCursor(handle); return; }
        const moved = new ByteVector();
        moved.take(this.buffer(handle));
        this.retireCursor(handle);
        stream.canonical = destination;
        if (destination !== null) this.seeded[destination] = true;
        this.buffer(handle).take(moved);
        this.publishCursor(handle);
    }

    // 이름 검색은 원문처럼 현재 선택을 갱신하되 현재 위치와 목록을 바꾸지 않는다. (2026-11-12 WB04-E345)
    findSection(sectionName) {
        this.syncStreams();
        return findDataManagerSection(this.manager, sectionName, { stream: (handle) => this.streams.get(handle).bytes });
    }

    // 원본 Rename 이 대상 제거/현재 source 재검색/이름 쓰기를 끝낸 뒤 버퍼의 호스트 위치만 매핑한다. (2026-11-12 WB04-E345)
    renameSection(destination, source) {
        this.requireNoAccess();
        this.syncStreams();
        const hooks = { selection: { stream: (handle) => this.streams.get(handle).bytes }, list: this.listEffects() };
        const current = renameDataManagerStream(this.manager, destination, source, hooks);
        this.bindRenamedBuffer(current);
        return current;
    }

    // 실제 임시 노드가 살아 있는 동안 원본 부모를 실행하고 완료한 같은 할당을 State 로 넘긴다. (2026-11-12 WB04-E345)
    ensureStateSize(minimum) {
        if (this.retiredState.size !== 0) throw new Error('분리 State 복사 중 원본 최소 확장 불가');
        this.requireNoAccess();
        this.syncStreams();
        let temporary = 0;
        const buffer = new ByteVector();
        let allocated = false;
        const hooks = {
            selection: { stream: (handle) => this.streams.get(handle).bytes }, // 원문과 임시 노드 모두 같은 실제 저장소에 둔다.
            list: this.listEffects(),
            memory: {
                buffer: (pointer) => {
                    if (pointer === TEMPORARY_TOKEN_BASE + temporary) return buffer.bytes;
                    if (pointer <= BUFFER_TOKEN_BASE || !this.streams.has(pointer - BUFFER_TOKEN_BASE)) throw new RangeError('Template 확장의 실제 원문 버퍼 핸들 범위');
                    return this.buffer(pointer - BUFFER_TOKEN_BASE).bytes;
                }, // 복사는 원본 부모가 요청하는 현재 data/end 를 사용한다.
                allocate: (size) => {
                    if (allocated) throw new Error('Template 확장 임시 버퍼 중복 할당');
                    buffer.resize(size);
                    allocated = true;
                    return TEMPORARY_TOKEN_BASE + temporary;
                },
                release: (pointer) => { if (pointer) this.releaseBuffer(pointer); },
                assertion: () => { throw new Error('Template 원본 확장의 MemStream 진단'); },
            },
            allocateStream: (size) => {
                if (size !== NODE_BYTES || temporary) throw new Error('Template 실제 임시 노드 할당 계약 위반');
                temporary = this.allocateStream();
                return temporary;
            },
            missingSource: () => { throw new Error('Template 원본 State 확장의 원문 섹션 누락'); },
        };
        const current = ensureDataManagerStreamSize(this.manager, 'State', minimum, hooks);
        if (!temporary) { this.publishCursor(current); return; }
        const node = this.streams.get(current).bytes;
        if (current !== temporary || !allocated || read(node, 0) !== TEMPORARY_TOKEN_BASE + temporary) throw new Error('Template 원본 확장의 임시 소유권 계약 위반');
        const end = read(node, 8);
        if (end > buffer.size) throw new RangeError('Template 원본 확장 결과의 실제 end 범위');
        // 원문 Rename 의 현재/head/next 와 임시 node handle 을 그대로 유지하고 같은 벡터 할당만 인계한다.
        put(node, 0, Image.bufferToken(current));
        buffer.resize(end);
        this.buffer(current).take(buffer);
        this.streamOrder.push(current);
        this.bindRenamedBuffer(current);
    }

    // 공개 진단은 보조 순서 벡터 대신 실제 head/next 와 이름을 읽는다. (2026-11-12 WB04-E345)
    sectionOrder() {
        const result = [];
        let current = read(this.manager, 4);
        // 실제 노드 수보다 긴 사슬은 손상/순환으로 거부한다.
        while (current) {
            if (result.length >= this.streams.size) throw new Error('Template 실제 섹션 목록 순환');
            const node = this.streams.get(current).bytes;
            const name = node.subarray(NAME_OFFSET, NAME_OFFSET + NAME_BYTES);
            const end = name.indexOf(0);
            if (end < 0) throw new Error('Template 섹션 이름 NUL 누락');
            result.push(String.fromCharCode(...name.subarray(0, end)));
            current = read(node, NEXT_OFFSET);
        }
        return result;
    }

    // 실제 원시 State 와 같은 커서를 노드에 반영한 뒤 원본 선택을 실행한다. (2026-11-12 WB04-E345)
    selectState() {
        this.selectLoadSection('State', 1);
    }

    // 004bd4d0/004bcb60 처럼 현재 원시 Subscriber 를 선택하고 같은 노드의 cursor 를 진행한다. (2026-11-12 WB04-E345)
    readSubscriberName() {
        this.syncStreams();
        const hooks = { stream: (node) => this.streams.get(node).bytes, assertion: () => { throw new Error('Template Subscriber 실제 읽기 섹션 선택 진단'); } };
        const selected = selectDataManagerSection(this.manager, 'Subscriber', 0, hooks);
        const node = this.streams.get(selected).bytes;
        const bytes = this.buffer(selected).bytes;
        put(node, 4, read(node, 4) + STATE_BYTES_MINIMUM); // 원본 DWORD ID 는 반환하지 않고 cursor 만 네 BYTE 진행한다.
        let position = read(node, 4);
        const end = read(node, 8);
        if (position > end || position > bytes.length || bytes.length - position < 2) throw new RangeError('Template Subscriber 길이 읽기의 실제 원문 범위');
        const length = bytes[position] | (bytes[position + 1] << 8);
        put(node, 4, position + 2);
        position += 2;
        if (position > end || length >= SIGNED_LENGTH_LIMIT || length >= SUBSCRIBER_NAME_LIMIT || length > end - position || length > bytes.length - position) throw new RangeError('Template Subscriber signed 길이/목록256BYTE 출력 범위');
        let result = String.fromCharCode(...bytes.subarray(position, position + length));
        put(node, 4, position + length);
        const nul = result.indexOf('\0');
        if (nul >= 0) result = result.slice(0, nul);
        return result;
    }

    // Territory120BYTE 읽기는 비어 있지 않은 섹션 선택을 먼저 수행한다. (2026-11-12 WB04-E345)
    selectTerritory() {
        this.selectLoadSection('Territory', 0);
    }

    // 현재 노드가 없는 읽기/빈 읽기는 원본 선택의 진단으로 남기고, 메타의 빈 쓰기 허용은 그대로 유지한다. (2026-11-12 WB04-E345)
    selectLoadSection(sectionName, writable) {
        this.syncStreams();
        const hooks = {
            stream: (node) => this.streams.get(node).bytes, // 같은 manager 의 실제 목록 노드를 선택한다.
            assertion: () => { throw new Error('Template 경제/메타 실제 섹션 선택 진단'); }, // 누락/읽기 불가를 가짜 빈 성공으로 바꾸지 않는다.
        };
        const selected = selectDataManagerSection(this.manager, sectionName, writable, hooks);
        this.publishCursor(selected);
        return selected;
    }

    // 선택 없이 typed cursor 를 처음 공개할 때도 같은 원시 노드에서 현재 상태를 읽는다. (2026-11-12 WB04-E345)
    loadCursor(sectionName) {
        this.syncStreams();
        let handle = 0;
        // getter 는 current 를 바꾸지 않고 현재 이름의 실제 노드를 빌린다.
        for (const candidate of this.streams.keys()) {
            if (compareDataManagerSectionNames(this.streamName(candidate), sectionName) === 0) { handle = candidate; break; }
        }
        if (!handle) throw new Error('Template 경제/메타 현재 노드 누락');
        this.publishCursor(handle);
    }

    // 원본을 옮겨 보존한 뒤 같은 Fort State 표준 위치에 대체 할당(0 으로 채운 size BYTE)을 만든다. 이전 할당은 commit 전까지 retiredState 가 보유한다. (2026-11-12 WB04-E345)
    growState(size) {
        if (this.retiredState.size !== 0) throw new Error('Template State 확장 복사 미완료');
        const at = requiredSection('State');
        // 표준 위치가 아직 없으면 빈 칸으로 채워 위치를 보존한다.
        while (this.fort.sections.length <= at) this.fort.sections.push(new Uint8Array(0));
        const vector = this.#vector(at);
        this.retiredState.take(vector);
        vector.resize(size);
        return vector.bytes;
    }

    // 선택된 노드는 소멸과 동일한 핸들/이름 저장소를 사용한다. 선택이 없으면 빈 문자열이다. (2026-11-12 WB04-E345)
    currentSection() {
        const handle = read(this.manager, 8);
        if (!handle) return '';
        const bytes = this.streams.get(handle).bytes.subarray(NAME_OFFSET, NAME_OFFSET + NAME_BYTES);
        const end = bytes.indexOf(0);
        if (end < 0) throw new Error('Template 현재 섹션 이름 NUL 누락');
        return String.fromCharCode(...bytes.subarray(0, end));
    }

    // 공개된 Fort/State 의 현재 버퍼를 같은 목록에 연결하며 파일 읽기 구현 자체를 대신하지 않는다. (2026-11-12 WB04-E345)
    syncStreams() {
        if (this.fort.sections.length > SECTION_COUNT) throw new RangeError('Template 실제 섹션35칸 범위');
        const count = this.fort.sections.length;
        // 파싱 자료/외부 추가 표준 위치만 최초 연결한다(원문 생성/rename 으로 제거된 이름을 이름표 기준으로 되살리지 않는다).
        for (let index = 0; index < count; index += 1) {
            if (this.seeded[index]) continue;
            const handle = this.allocateStream(index);
            const node = this.streams.get(handle).bytes;
            // 표준 이름을 노드 이름 영역에 쓴다.
            for (let at = 0; at < SECTION_NAMES[index].length; at += 1) node[NAME_OFFSET + at] = SECTION_NAMES[index].charCodeAt(at) & 255;
            let tail = read(this.manager, 4);
            if (!tail) put(this.manager, 4, handle);
            else {
                // 마지막 노드를 찾아 새 노드를 붙인다(파싱/외부 자료의 최초 연결은 current 를 바꾸지 않는다).
                for (let next = read(this.streams.get(tail).bytes, NEXT_OFFSET); next; next = read(this.streams.get(tail).bytes, NEXT_OFFSET)) tail = next;
                put(this.streams.get(tail).bytes, NEXT_OFFSET, handle);
            }
            this.streamOrder.push(handle);
        }
        // 실제 목록/head/current/next 는 보존하고 현재 외부 버퍼와 공개 cursor 만 동기화한다.
        for (const [handle, stream] of this.streams) {
            if (stream.accessing) continue;
            const buffer = this.buffer(handle);
            put(stream.bytes, 0, buffer.capacity ? Image.bufferToken(handle) : 0);
            put(stream.bytes, 8, buffer.size);
            put(stream.bytes, 12, buffer.capacity);
            this.syncCursorPosition(handle);
            this.publishCursor(handle);
        }
    }

    // 실제 head/next 의 같은 버퍼만 표준 위치로 옮기고 current/pos/생성 수명은 재설정하지 않는다. 원문이 이미 붙인 노드만 같은 순서로 공개하며 미완료 임시 노드를 붙이지 않는다. (2026-11-13 WB04-E346)
    bindFileStreams() {
        this.streamOrder = [];
        let current = read(this.manager, 4);
        // head 부터 next 를 따라 붙은 노드를 차례로 공개한다.
        while (current) {
            if (this.streamOrder.length >= this.streams.size) throw new Error('Template 파일 읽기 뒤 실제 목록 순환');
            const node = this.streams.get(current).bytes;
            this.streamOrder.push(current);
            this.bindRenamedBuffer(current);
            const buffer = this.buffer(current);
            const end = read(node, 8);
            if (end > buffer.size) throw new RangeError('Template 파일 읽기 뒤 실제 end 범위');
            buffer.resize(end);
            this.publishCursor(current);
            current = read(node, NEXT_OFFSET);
        }
    }

    // 빈 이미지가 처음 파일을 읽기 전에 요새 섹션 35칸과 표준 위치 벡터, typed cursor 를 빈 상태로 되돌린다. (2026-11-13 WB04-E346)
    resetForFileRead() {
        this.fort.sections.length = 0;
        this.#vectors = [];
        // 표준 35칸을 빈 섹션으로 채운다.
        for (let index = 0; index < SECTION_COUNT; index += 1) this.fort.sections.push(new Uint8Array(0));
        this.seeded.fill(true);
        Object.assign(this.state, { bytes: new Uint8Array(0), position: 0, end: 0, present: false });
        Object.assign(this.territory, { bytes: new Uint8Array(0), position: 0, end: 0 });
    }

    // 같은 raw reader 의 bool 을 유지한다. 새 임시 버퍼는 기존 같은 이름을 제거하기 전까지 별개로 소유한다. fileEffects = TemplateFileLoadHooks
    //  { read(이름) → Uint8Array, readWhole?(이름) → { storage, length } | null, typeCount(), tooManyTypes(), rejectFile(이름) } 이며 선택된 호출만 연결을 요구한다. (2026-11-13 WB04-E346)
    readFile(fileEffects) {
        this.requireNoAccess();
        if (this.streams.size === 0 && !read(this.manager, 4)) this.resetForFileRead();
        else this.syncStreams();
        let source = new Uint8Array(0);
        let wholeSource = null;
        let allocating = 0;
        let created = 0; // 파일 할당은 지역 소유지만 이름표는 같은 Client 에서 빌린다.
        const fileBuffer = (pointer) => {
            if (pointer === FILE_TOKEN) return wholeSource ? wholeSource.storage : source;
            if (pointer <= BUFFER_TOKEN_BASE || !this.streams.has(pointer - BUFFER_TOKEN_BASE)) throw new RangeError('Template 파일 이미지의 실제 버퍼 토큰 범위');
            return this.buffer(pointer - BUFFER_TOKEN_BASE).bytes;
        }; // 입력과 새 임시 할당 및 기존 노드 모두 현재 실제 버퍼를 빌린다.
        const imageHooks = {
            names: () => this.sectionNames.names(),
            assertion: () => { throw new Error('Template 실제 파일 이미지의 원본 진단'); },
            streams: {
                selection: { stream: (handle) => this.streams.get(handle).bytes },
                list: this.listEffects(),
                allocateStream: (size) => {
                    if (size !== NODE_BYTES || created >= SECTION_COUNT) throw new Error('Template 파일 이미지의 새 노드 계약 위반');
                    created += 1;
                    allocating = this.allocateStream();
                    return allocating;
                }, // 기존 canonical 버퍼는 원문 제거 전까지 덮어쓰지 않는다.
                memory: {
                    buffer: fileBuffer,
                    allocate: (size) => {
                        const buffer = this.buffer(allocating);
                        if (buffer.size !== 0) throw new Error('Template 파일 이미지의 새 버퍼 중복 할당');
                        buffer.resize(size);
                        return Image.bufferToken(allocating);
                    },
                    assertion: () => { throw new Error('Template 파일 이미지의 실제 MemStream 진단'); },
                },
            },
        };
        const currentFileName = () => nameOfManager(this.manager);
        const file = {
            image: imageHooks,
            fileName: currentFileName,
            readFile: (fileName, mode, length, flags) => {
                if (mode !== 1 || flags || length.length !== 4) throw new Error('Template 파일 읽기의 실제 요청 인수 불일치');
                if (typeof fileEffects.readWhole === 'function') { // 실제 Native 만 할당 존재와 논리 길이를 분리한다. 기존 snapshot/Editor namespace 의 값형 read 는 아래에 유지한다.
                    wholeSource = fileEffects.readWhole(fileName);
                    if (!wholeSource) return 0;
                    if (wholeSource.length < 0 || wholeSource.storage.length < 4 || wholeSource.length > wholeSource.storage.length) throw new RangeError('Template Native 파일의 실제 할당/논리 길이 불일치');
                    put(length, 0, wholeSource.length);
                    return FILE_TOKEN;
                }
                source = need(fileEffects.read, 'Template 실제 파일 Load 장치 미연결')(fileName);
                put(length, 0, source.length);
                return source.length === 0 ? 0 : FILE_TOKEN;
            }, // manager/header 가 공개되고 readonly/+16 을 쓴 뒤에만 실제 파일 장치를 호출한다.
            signature: (cursor) => readTemplateFileSignature(this.manager, cursor, {
                buffer: fileBuffer,
                typeCount: () => need(fileEffects.typeCount, 'Template 실제 파일 Load 장치 미연결')(),
                fileName: currentFileName,
                tooManyTypes: () => { need(fileEffects.tooManyTypes, 'Template 실제 파일 Load 장치 미연결')(); },
                rejectFile: (fileName) => { need(fileEffects.rejectFile, 'Template 실제 파일 Load 장치 미연결')(fileName); },
            }),
            releaseFile: (pointer) => {
                if (pointer !== FILE_TOKEN) throw new Error('Template 파일 읽기 부모의 실제 할당 반납 불일치');
                wholeSource = null;
                source = new Uint8Array(0);
            }, // 기존 원문 reader 가 성공/거부/예외에서 한 번만 반납한다.
        };
        try {
            const success = readDataManagerFile(this.manager, file);
            this.bindFileStreams();
            if (success && created !== SECTION_COUNT) throw new Error('Template 실제 파일의 전체 이름 표 불일치');
            return success;
        } catch (error) {
            this.bindFileStreams();
            throw error;
        } // host 예외 전에 원문이 이미 붙인 노드/부분 pos 를 숨기거나 rollback 하지 않는다.
    }

    // 기존 Campaign/목록 교체 API 는 이전과 같이 빈 manager 에 읽고 정상 false 를 예외로 바꾼다. (2026-11-13 WB04-E346)
    readImage(input, typeCount) {
        if (this.streams.size !== 0 || read(this.manager, 4) || input.length > 0xffffffff) throw new TypeError('Template 실제 파일 이미지/빈 manager 계약 위반');
        const copy = Uint8Array.from(input);
        const effects = {
            read: () => Uint8Array.from(copy),
            typeCount: () => typeCount,
            tooManyTypes: () => { throw new Error('Template 파일의 numTypes<256 진단'); },
            rejectFile: () => { throw new Error('Template 잘못된 서명의 원본 파일 제거 요청: 보호 원문은 삭제하지 않습니다'); },
        };
        if (!this.readFile(effects)) throw new Error('Template 실제 파일 열기/서명 거부');
    }

    // 현재 각 섹션의 버퍼를 먼저 반납하고 같은 노드의 실제 할당을 뒤에 해제한다. (2026-11-12 WB04-E345)
    destroy() {
        this.syncStreams();
        destroyDataManager(this.manager, {
            stream: (handle) => this.streams.get(handle).bytes, // 현재 같은 60BYTE 노드만 빌린다.
            releaseBuffer: (pointer) => { this.releaseBuffer(pointer); }, // 이름 변경 뒤에도 같은 data 토큰의 현재 할당만 반납한다.
            releaseStream: (handle) => { if (!this.streams.delete(handle)) throw new Error('DataManager 같은 섹션 중복 반납'); }, // 버퍼 소멸이 끝난 노드만 반납한다.
        });
    }
}

// 판본 전역(DWORD 한 칸)을 생성자가 쓰는 4BYTE 뷰로 빌리고 결과를 되돌려 쓰는 작은 어댑터이다. (2026-11-12 WB04-E345)
function versionBorrow(globals) {
    const view = new Uint8Array(4);
    put(view, 0, globals.version);
    return { view, commit() { globals.version = read(view, 0); } };
}

// Client 와 Session 이 같은 Template9/실제 manager 이미지·State cursor 수명을 공유하는 소유자이다. (2026-11-12 WB04-E345)
export class CampaignTemplateStorage {
    // 모든 Image/외부 Session 보다 먼저 확보하는 실제 공유 DWORD 소유권과 같은 Client 의 이름표 소유자이다.
    #serializationGlobals;
    #sectionNames;
    // 실제 Client level29 진단 출력이다(지역 송신 이미지 덤프에서 쓴다).
    #diagnosticLog = null;
    // Client 보다 짧은 Session 이 사라져도 유지되는 실제 9개 헤더(180BYTE)이다.
    #headers = new Uint8Array(HEADER_BYTES * SLOT_COUNT);
    // 호스트 포인터를 DWORD 로 자르지 않는 manager 핸들 → 실제 소유 이미지 표와 다음 핸들이다.
    #images = new Map();
    #nextHandle = 1;

    // 원본 9개 헤더를 초기화하고 같은 Client 전역을 빌린다. 생략하면 독립 전역을 새로 소유한다. (2026-11-12 WB04-E345)
    constructor(serializationGlobals = null, sectionNames = null) {
        this.#serializationGlobals = serializationGlobals ?? new FortSerializationGlobals();
        this.#sectionNames = sectionNames ?? new ClientTemplateSectionNames();
        // 아홉 개의 처음 20BYTE 만 원본 생성 몸체로 초기화한다.
        for (let slot = 0; slot < SLOT_COUNT; slot += 1) initializeTemplateHeader(this.#header(slot));
    }

    // 같은 고정 물리 표(180BYTE)를 빌린다. (2026-11-12 WB04-E345)
    headers() { return this.#headers; }

    // 유효 슬롯의 현재 20BYTE 기록(살아 있는 뷰)이다. (2026-11-12 WB04-E345)
    #header(slot) {
        if (!Number.isInteger(slot) || slot < 0 || slot >= SLOT_COUNT) throw new RangeError('Template 실제9칸 범위');
        return this.#headers.subarray(slot * HEADER_BYTES, (slot + 1) * HEADER_BYTES);
    }

    // 헤더가 빌린 같은 handle 도 같은 이미지/State cursor 를 찾는다. 없거나 소유자보다 오래 산 빌림은 거부한다. (2026-11-12 WB04-E345)
    #current(slot) {
        const handle = read(this.#header(slot), 0);
        const image = this.#images.get(handle);
        if (!handle || !image) throw new Error('Template 현재 manager가 없거나 소유자보다 오래 산 빌림');
        return image;
    }

    // 원문 new276 만 실행한다: header/ctor/이름표는 나중에 선택된 부모가 진행한다. 호스트 핸들(비영) 또는 0 을 돌려준다. (2026-11-12 WB04-E345)
    allocateEmptyManager(bytes) {
        if (bytes !== MANAGER_BYTES || !this.#nextHandle) throw new TypeError('Template 실제빈Image276할당계약');
        const token = this.#nextHandle;
        this.#nextHandle += 1;
        this.#images.set(token, new Image(this.#sectionNames));
        return token;
    }

    // 존재하는 같은 미생성 할당만 원문 ctor 로 전환한다. (2026-11-12 WB04-E345)
    constructEmptyManager(token, name) {
        this.#images.get(token).constructName(String(name), versionBorrow(this.#serializationGlobals));
        return token;
    }

    // 고정 token 의 물리는 header 교체 후에도 같은 Image 이다. (2026-11-12 WB04-E345)
    managerBytes(token) { return this.#images.get(token).manager; }

    // 진단 뒤 이름표를 한 번 받고 같은 Image 의 기존 raw Create 를 매 이름마다 실행한다. assertion 이 없으면 진단에서 거부한다. (2026-11-12 WB04-E345)
    initializeEmptyManager(token, assertion = null) {
        const image = this.#images.get(token);
        if (!image.constructed) throw new Error('Template 실제ctor이전초기화');
        initializeDataManager(image.manager, {
            assertion,
            names: () => image.sectionNames.names(),
            create: (name) => {
                const end = name.indexOf(0);
                if (end < 0) throw new RangeError('Template 실제Create이름NUL범위');
                image.createSection(String.fromCharCode(...name.subarray(0, end)));
            },
        });
    }

    // 같은 Image 의 소멸/할당 반납을 원문 바깥 부모에 빌린다: { destroy(handle), release(handle) }. (2026-11-12 WB04-E345)
    managerLifetime() { return this.#effects(); }

    // 다른 Storage 도 동일 Client 이름표를 명시 공유할 수 있다. (2026-11-12 WB04-E345)
    sectionNamesOwner() { return this.#sectionNames; }

    // 같은 Client 의 판본 전역 묶음이다(생성자/객체 로더/저장 부모가 직접 같이 쓰는 005c89c0 DWORD 를 복사하지 않는다). (2026-11-12 WB04-E345)
    get serializationGlobals() { return this.#serializationGlobals; }

    // 같은 DWORD 의 현재 값이다. 조회/Storage 생성자에서 공유값을 재초기화하지 않는다. (2026-11-12 WB04-E345)
    get serializationVersion() { return this.#serializationGlobals.version; }
    set serializationVersion(value) { this.#serializationGlobals.version = value; }

    // 실제 Client level29 로그 장치를 빌린다. 지역 송신 이미지의 덤프가 같은 공유 formatter/FILE/Chat 을 쓴다. (2026-11-12 WB04-E345)
    bindDiagnosticLog(log) { this.#diagnosticLog = log; }

    // 실제 manager 소멸과 객체 할당 반납을 분리한다. (2026-11-12 WB04-E345)
    #effects() {
        return {
            destroy: (handle) => {
                const image = this.#images.get(handle);
                image.destroy();
                image.fort = { sections: [] };
                Object.assign(image.state, { bytes: new Uint8Array(0), position: 0, end: 0, present: false });
                Object.assign(image.territory, { bytes: new Uint8Array(0), position: 0, end: 0 });
                image.retiredState.release();
            },
            release: (handle) => { if (!this.#images.delete(handle)) throw new Error('Template 같은 manager 중복 반납'); },
        };
    }

    // 기존 한 manager 를 정확히 해제한 뒤 새 실제 객체를 소유한다. (2026-11-12 WB04-E345)
    install(slot, fort, name) {
        this.clear(slot);
        if (!this.#nextHandle) throw new RangeError('Template 호스트 manager 핸들 소진');
        const handle = this.#nextHandle;
        this.#nextHandle += 1;
        this.#images.set(handle, new Image(this.#sectionNames, fort, name, versionBorrow(this.#serializationGlobals)));
        put(this.#header(slot), 0, handle);
    }

    // 파싱된 보조 자료와 같은 실제 파일 원문을 새 manager 에 읽으며 임의 빈 섹션 목록을 먼저 만들지 않는다. typeCount 의 기본 188 은 10.78 독립 호출용이다. (2026-11-13 WB04-E346)
    installFile(slot, fort, bytes, name, typeCount = 188) {
        this.clear(slot);
        if (!this.#nextHandle) throw new RangeError('Template 파일 manager 핸들 소진');
        const image = new Image(this.#sectionNames, fort, name, versionBorrow(this.#serializationGlobals), true);
        image.readImage(bytes, typeCount);
        const handle = this.#nextHandle;
        this.#nextHandle += 1;
        this.#images.set(handle, image);
        put(this.#header(slot), 0, handle);
    }

    // 실제 원문 부모(004bf550)가 설치/읽기/정상 false 정리를 지휘하며 기존 같은 manager 는 건드리지 않는다. fort 는 이미 해석된 보조 metadata 이고
    // file = TemplateFileLoadHooks { read | readWhole, sameName, wrongName, readOnly, typeCount, tooManyTypes, rejectFile, currentSlot, refreshTitle } 이다. (2026-11-13 WB04-E346)
    loadFile(slot, fort, name, file) {
        const hooks = {
            fileName: (manager) => nameOfManager(this.#images.get(manager).manager), // 항상 전달받은 현재 handle 의 실제 +12 를 빌린다.
            sameName: (first, second) => need(file.sameName, 'Template 실제 파일 Load 장치 미연결')(first, second),
            wrongName: () => { need(file.wrongName, 'Template 실제 파일 Load 장치 미연결')(); },
            allocate: (bytes) => {
                if (bytes !== MANAGER_BYTES || !this.#nextHandle) throw new RangeError('Template Load의 실제276BYTE/호스트handle 소진');
                const handle = this.#nextHandle;
                this.#nextHandle += 1;
                return handle;
            },
            construct: (manager, fileName) => {
                const image = new Image(this.#sectionNames, fort, String(fileName), versionBorrow(this.#serializationGlobals), true);
                image.fort.sections.length = 0;
                if (this.#images.has(manager)) throw new Error('Template Load manager의 같은 할당 중복');
                this.#images.set(manager, image);
                return manager;
            }, // 생성자의 공유 version 0 과 실제 Image 소유를 끝낸 뒤 원문 부모가 header.ptr 를 공개한다.
            readOnly: (fileName) => need(file.readOnly, 'Template 실제 파일 Load 장치 미연결')(fileName),
            readFile: (manager) => this.#images.get(manager).readFile(file), // readonly callback 뒤 header 에서 재조회한 manager 를 그대로 읽는다.
            isCurrent: () => slot === need(file.currentSlot, 'Template 실제 파일 Load 장치 미연결')(),
            refreshTitle: () => { need(file.refreshTitle, 'Template 실제 파일 Load 장치 미연결')(); },
            lifetime: this.#effects(),
        };
        loadTemplateFile(this.#header(slot), name, hooks);
    }

    // 빌린 헤더를 반납해도 원래 소유자의 실제 버퍼/커서는 유지된다. (2026-11-12 WB04-E345)
    borrow(slot, source) {
        if (slot === source) throw new TypeError('Template 자기 manager 빌림 계약');
        this.#current(source);
        const handle = read(this.#header(source), 0);
        const target = this.#header(slot);
        if (read(target, 0) === handle && !read(target, 4)) throw new TypeError('Template 원래 소유자를 반납하는 같은 manager 빌림');
        this.clear(slot);
        put(this.#header(slot), 0, handle);
        put(this.#header(slot), 4, 1);
    }

    // 비영 포인터만 정리하는 원본 전역 몸체를 그대로 사용한다. (2026-11-12 WB04-E345)
    clearAll() { clearTemplateHeaders(this.#headers, this.#effects()); }

    // 포인터 0 이어도 단일 기록은 0 으로 비운다. (2026-11-12 WB04-E345)
    clear(slot) { releaseTemplateHeader(this.#header(slot), this.#effects()); }

    // 모든 소비자는 같은 현재 manager 를 다시 조회한다. 파싱한 요새(`sections` 포함)를 돌려준다. (2026-11-12 WB04-E345)
    template(slot) { return this.#current(slot).fort; }

    // 한 manager 를 빌린 헤더끼리도 같은 cursor 객체를 반환한다. (2026-11-12 WB04-E345)
    state(slot) { return this.#current(slot).state; }

    // 실제 Player State getter 가 같은 목록/current 를 선택한 뒤 같은 cursor 를 돌려받는다. (2026-11-12 WB04-E345)
    selectState(slot) {
        const image = this.#current(slot);
        image.selectState();
        return image.state;
    }

    // 파싱 시점의 편의 사본이 아니라 실제 현재 섹션/노드를 읽는다. (2026-11-12 WB04-E345)
    readSubscriberName(slot) { return this.#current(slot).readSubscriberName(); }

    // 미정 바이트를 포함한 완성 28BYTE 를 실제 새 섹션에 쓰고 같은 현재 cursor 를 반환한다. (2026-11-12 WB04-E345)
    initializeState(slot, bytes) {
        if (!(bytes instanceof Uint8Array) || bytes.length !== 28) throw new RangeError('Template State 초기화는 정확히 28BYTE 여야 합니다');
        const image = this.#current(slot);
        image.initializeState(bytes);
        return image.state;
    }

    // 쓰기 참조를 만든다: 생성한 manager/노드/수명을 고정하고 이후 current 변경과 분리한다. (2026-11-12 WB04-E345)
    #reference(manager, image, section) {
        return { manager, section, generation: image.streams.get(section).generation };
    }

    // 원본 생성 부모로 이름의 기존 노드를 교체하고 새 마지막 노드의 참조를 반환한다. Temp 같은 동적 이름도 허용한다. (2026-11-12 WB04-E345)
    createSection(slot, name) {
        const manager = read(this.#header(slot), 0);
        const image = this.#current(slot);
        return this.#reference(manager, image, image.createSection(name));
    }

    // 원문이 이미 선택한 현재 manager 를 고정해 섹션을 만든다. 뒤 callback 의 header 교체로 다른 manager 를 재선택하지 않는다. (2026-11-12 WB04-E345)
    createManagerSection(manager, name) {
        const image = this.#images.get(manager);
        if (!manager || !image) throw new RangeError('섹션 생성의 실제 manager 수명 누락');
        return this.#reference(manager, image, image.createSection(name));
    }

    // 검색 실패도 원문 current 0 을 유지하며 참조 유효성과 빈 결과를 구별한다. 없으면 null 이다. (2026-11-12 WB04-E345)
    findSection(slot, name) {
        const manager = read(this.#header(slot), 0);
        const image = this.#current(slot);
        const section = image.findSection(name);
        return section ? this.#reference(manager, image, section) : null;
    }

    // 선택 부모의 실제 pos 0/읽기 진단을 끝낸 같은 노드의 생성 수명을 반환한다. (2026-11-12 WB04-E345)
    selectSection(slot, name, writable) {
        const manager = read(this.#header(slot), 0);
        const image = this.#current(slot);
        return this.#reference(manager, image, image.selectLoadSection(name, writable));
    }

    // 실제 Rename 부모가 source 노드를 그대로 남긴 결과를 사용하며 current/head/next 를 편의 재기록하지 않는다. (2026-11-12 WB04-E345)
    renameSection(slot, destination, source) {
        const manager = read(this.#header(slot), 0);
        const image = this.#current(slot);
        return this.#reference(manager, image, image.renameSection(destination, source));
    }

    // Create 반환값과 독립적으로 현재 선택을 보관한다. 과거 파일에서 읽은 노드에도 첫 빌림 수명을 부여한다. (2026-11-12 WB04-E345)
    currentWriteStream(slot) {
        const manager = read(this.#header(slot), 0);
        const image = this.#current(slot);
        const section = read(image.manager, 8);
        if (!section || !image.streams.has(section)) throw new Error('Template 현재 쓰기 stream의 실제 노드 누락');
        image.syncCursorPosition(section);
        return this.#reference(manager, image, section);
    }

    // 반납된 manager 와 재생성된 동일 이름의 새 노드를 이전 쓰기 대상으로 취급하지 않는다. (2026-11-12 WB04-E345)
    #writable(stream) {
        const image = this.#images.get(stream?.manager);
        const node = image?.streams.get(stream.section);
        if (!image || !stream.section || !stream.generation || !node || node.generation !== stream.generation) throw new Error('Template 쓰기 참조의 해제된 manager/섹션 수명');
        return image;
    }

    // 원본 일반 MemStream 쓰기는 보관한 노드의 현재 cursor 에 적용한다. (2026-11-12 WB04-E345)
    writeSection(stream, bytes) { this.#writable(stream).writeStream(stream.section, bytes); }

    // TypeNames 처럼 current 가 다른 섹션으로 바뀐 경우에도 보관한 실제 노드만 빌린다. operation(필드 16BYTE, MemStream hooks). (2026-11-12 WB04-E345)
    accessSection(stream, operation) { this.#writable(stream).accessSection(stream.section, operation); }

    // 같은 manager 의 전체 최소 확장 부모가 만든 실제 현재 State 를 빌린다. (2026-11-12 WB04-E345)
    ensureStateSize(slot, minimum) {
        const image = this.#current(slot);
        image.ensureStateSize(minimum);
        return image.state;
    }

    // 빌린 헤더도 같은 manager 의 실제 연결 순서를 확인한다. (2026-11-12 WB04-E345)
    sectionOrder(slot) { return this.#current(slot).sectionOrder(); }

    // 빌린 헤더도 같은 manager 의 현재 섹션을 확인한다. (2026-11-12 WB04-E345)
    currentSection(slot) { return this.#current(slot).currentSection(); }

    // 지형 cursor 도 같은 manager 의 원문을 빌린다. (2026-11-12 WB04-E345)
    territory(slot) { return this.#current(slot).territory; }

    // 월드 로더가 같은 manager 에서 원본 읽기 선택을 한 뒤 같은 지형 cursor 를 사용한다. (2026-11-12 WB04-E345)
    selectTerritory(slot) {
        const image = this.#current(slot);
        image.selectTerritory();
        return image.territory;
    }

    // 같은 manager 의 현재 Money 노드를 선택 효과 없이 빌린다. (2026-11-12 WB04-E345)
    money(slot) {
        const image = this.#current(slot);
        image.loadCursor('Money');
        return image.money;
    }

    // Money 는 원본 wrapper 의 읽기 전용 선택을 적용한다. (2026-11-12 WB04-E345)
    selectMoney(slot) {
        const image = this.#current(slot);
        image.selectLoadSection('Money', 0);
        return image.money;
    }

    // 같은 manager 의 Badges 노드와 실제 버퍼를 빌린다. (2026-11-12 WB04-E345)
    badges(slot) {
        const image = this.#current(slot);
        image.loadCursor('Badges');
        return image.badges;
    }

    // 빈 Badges 도 허용하는 원본 선택 모드를 유지한다. (2026-11-12 WB04-E345)
    selectBadges(slot) {
        const image = this.#current(slot);
        image.selectLoadSection('Badges', 1);
        return image.badges;
    }

    // 같은 manager 의 Technology 노드와 실제 버퍼를 빌린다. (2026-11-12 WB04-E345)
    technology(slot) {
        const image = this.#current(slot);
        image.loadCursor('Technology');
        return image.technology;
    }

    // 원본 기술 로더가 빈 섹션의 부모/UI 처리 여부를 직접 결정하게 한다. (2026-11-12 WB04-E345)
    selectTechnology(slot) {
        const image = this.#current(slot);
        image.selectLoadSection('Technology', 1);
        return image.technology;
    }

    // 원문 선택 때 쓴 같은 파일 이름(manager+c 의 256BYTE 물리)을 반환한다. (2026-11-12 WB04-E345)
    name(slot) {
        const image = this.#current(slot);
        const bytes = image.manager.subarray(12, 12 + 256);
        const end = bytes.indexOf(0);
        if (end < 0) throw new RangeError('Template 현재 manager 파일명의 NUL 범위');
        const current = String.fromCharCode(...bytes.subarray(0, end));
        if (image.name !== current) image.name = current;
        return image.name;
    }

    // 빌린 헤더도 원래 소유자의 같은 manager 꼬리 DWORD 를 읽는다: [서명 판정(+0x10c), FE 판정(+0x110)]. (2026-11-12 WB04-E345)
    fileHeader(slot) {
        const { manager } = this.#current(slot);
        return [read(manager, 0x10c), read(manager, 0x110)];
    }

    // 원본을 옮겨 보존한 뒤 같은 Fort State 섹션에 대체 할당을 만든다. 새 버퍼(0 으로 채운 size BYTE)를 돌려준다. (2026-11-12 WB04-E345)
    growState(slot, size) { return this.#current(slot).growState(size); }

    // 복사 뒤에만 이전 실제 버퍼를 반납한다. (2026-11-12 WB04-E345)
    commitStateGrowth(slot) { this.#current(slot).retiredState.release(); }

    // 현재 실제 manager 할당 수를 읽기만 한다. (2026-11-12 WB04-E345)
    ownedImages() { return this.#images.size; }
}
