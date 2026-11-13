// 같은 Client 수명의 구름(Kencloud) 전역·주소 공간·팔레트 이름을 메뉴/화면/Session 보다 오래 소유하는 모듈이다(cpppj `client/ClientKencloudGlobals.cpp/.h`, 10.78 전용, 기준선 뒤 변경 없음).
// 구름 계산은 이미 옮긴 원문 본체(raw-kencloud.js·raw-kencloud-render.js·raw-fort-palette-transition.js)를 그대로 부르고, 이 모듈은 메모리/파일/설정/화면 장치만 연결한다(별도 난수·구름 사본을 만들지 않는다).
//  - 물리: 연속 전역 2054 DWORD(`state()`: 이전 행 0..511·end 512·lastRow 513·nextRow 514·cache 515·현재 행 516..1027·seed 1028..2027·이름 바이트 8112..8215(=DWORD 2028..2053: kenCloud 2049·cloudAlgo 2050·전체 2051·부분 2052·phase 2053)),
//    옵션 8 DWORD(`value(option)`: cloudiness 72·speed 2·fury 10·spacer 32·recenter 99999·seedCount 400·lowColor 228·highColor 245), 고정 16행 순서표, 팔레트 이름 128바이트(별도 물리).
//    `globals()`/`paletteGlobals()`/`renderGlobals()` 는 세 본체가 요구하는 전역 묶음을 **같은 물리의 getter/setter 와 살아 있는 보기**로 빌려 준다(필드 사본이 없다).
//  - 주소 공간: `allocate(bytes)` 는 0x36000000 부터 겹치지 않는 32비트 주소 토큰과 정확한 길이의 블록을 만들고(현재 할당 입력 훅으로 내용을 채움), `memory(address)` 는 내부 주소부터 남은 물리(one-past 는 길이 0)를,
//    `release(token)` 는 정확한 생존 base 만 반납한다(0 은 무시). 해제·미등록·블록 사이 주소는 RangeError 이다.
//  - 파일 부모(웹 적응): "CloudFileSpec" 경로를 NUL 포함 바이트로 빌려 주고, 존재는 `files.has`(원문 Probe 의 위치 코드 대신 1/0), 해독은 `files.read` → `gifFileDimensions`(치수 선행 쓰기) → 크기 0/16MB 초과 거부 →
//    `allocate` → `decodeBackgroundGif`(치수 불일치 Error) → 같은 블록에 색 번호 복사이다. 264바이트 지역 scratch 도 현재 할당 입력으로 채운다. 512×512 판정·반납·fallback 은 원문 본체가 한다.
//  - 설정: `readOptions(config)` 는 cloudiness/cloudSpeed/cloudFury/cloudSpacer/cloudRecenter/cloudAlgo/kenCloud 를 현재 값을 기본 입력으로 `config.readInt` 해 바뀐 경우에만 저장한다(signed → DWORD). 파일·난수를 쓰지 않는다.
//  - 화면: `draw(screen, area, …)` 는 현재 Screen 의 clip/setClip/lock/unlock/blitBuffer 를 그대로 빌린다. alternate(원문 parallax DirectDraw 잠금 004a15d0/004a1650)는 장치 미복원이라 Error 로 거부한다.
//  - 팔레트 전환: `transition(name, …, devices)` 는 호출자 장치(log/loadPalette/buildMaps)에 대소문자 무시 비교(CRT _stricmp)·반납·생성·이동·"GamePalSpec" 경로를 더해 원문 004b6100 을 실행한다.
//  - Renderer 연결: `bindRenderer(renderer)` 는 실제 객체를 소유하지 않고 0x3b000000 부터 0x1000 간격의 opaque32 토큰을 붙인다(null 이면 0). `worldRedraw()` 는 0059a8b0 `{ value }` 상자이다.
// 오류: 물리 범위 = RangeError(cpppj out_of_range), 주소/토큰 소진 = RangeError(cpppj overflow_error), 장치 계약·미복원 = Error(cpppj logic_error), 0 나눗셈은 본체의 KencloudDomainError 이다.
// Client 의 설정 변경 즉시 반영 callback·메뉴 구름 배경 연결(4절 37054픽셀 차이)은 Client 본체·ClientRendererFrame 단위 몫이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E396: 5-A ClientRendererFrame 선행 구름 전역 소유자로 추가)
import { currentAllocationBytes } from './current-allocation-bytes.js';
import { buildKencloud, loadKencloudFile, stepKencloud } from './raw-kencloud.js';
import { cleanKencloudDirty, drawKencloud, kencloudRowDirty, markKencloudDirty, updateKencloud } from './raw-kencloud-render.js';
import { transitionFortPalette } from './raw-fort-palette-transition.js';
import { decodeBackgroundGif, gifFileDimensions } from '../assets/gif.js';
import { decodeWindows1252, encodeWindows1252 } from '../assets/text-encoding.js';

// 연속 전역의 DWORD 수(8216바이트)와 구름 이름이 시작하는 바이트 위치·길이이다. (2026-11-13 WB04-E396)
const STATE_WORDS = 2054;
const NAME_OFFSET = 8112;
const NAME_BYTES = 104;
// 연속 전역 안의 실제 DWORD 위치이다: end·lastRow·nextRow·cache·현재 행 표·seed 표·cloudAlgo·kenCloud·전체/부분 요청·phase. (2026-11-13 WB04-E396)
const END = 512;
const LAST_ROW = 513;
const NEXT_ROW = 514;
const CACHE = 515;
const ROWS = 516;
const SEEDS = 1028;
const SEED_COUNT = 1000;
const KEN_CLOUD = 2049;
const ALGORITHM = 2050;
const ALL = 2051;
const PART = 2052;
const PHASE = 2053;
// 보호 PE 00540a7c..90 의 옵션 최초값과 00540a98 의 고정 행 순서이다. (2026-11-13 WB04-E396)
const OPTION_DEFAULTS = Object.freeze([72, 2, 10, 32, 99999, 400, 228, 245]);
const PHASE_ROWS = Object.freeze([0, 8, 4, 12, 1, 11, 5, 15, 3, 10, 2, 7, 14, 9, 6, 13]);
// 005c8910 팔레트 이름 버퍼의 바이트 수이다. (2026-11-13 WB04-E396)
const PALETTE_NAME_BYTES = 128;
// host 주소 토큰의 시작·블록 정렬 단위·블록 사이 간격, Renderer 토큰의 시작·간격·상한이다. (2026-11-13 WB04-E396)
const FIRST_ADDRESS = 0x36000000;
const PAGE = 0x1000;
const FIRST_RENDERER = 0x3b000000;
const RENDERER_LIMIT = 0xffffe000;
// 원문 지역 파일 scratch 의 크기(height 4 + width 4 + 경로 256)와 구름 GIF 할당 상한(16MB)이다. (2026-11-13 WB04-E396)
const FILE_SCRATCH_BYTES = 264;
const GIF_LIMIT = 16 * 1024 * 1024;

// 원문 옵션 DWORD 의 순서이다. 사용: cloud.value(KencloudOption.speed).value = 3. (2026-11-13 WB04-E396)
export const KencloudOption = Object.freeze({ cloudiness: 0, speed: 1, fury: 2, spacer: 3, recenter: 4, seedCount: 5, lowColor: 6, highColor: 7 });

// 현재 NUL 까지의 C 문자열을 문자열로 읽는다. 물리 안에 NUL 이 없으면 RangeError 이다(입력을 복제하지 않고 같은 바이트를 읽는다). (2026-11-13 WB04-E396)
function cstringText(bytes) {
    const end = bytes.indexOf(0);
    if (end < 0) throw new RangeError('구름 현재 C문자열 물리 범위');
    return decodeWindows1252(bytes.subarray(0, end));
}

// PathSpec 반환 문자열을 동기 부모가 끝날 때까지 쓰는 NUL 포함 바이트로 만든다. (2026-11-13 WB04-E396)
function cstringBytes(text) {
    const encoded = encodeWindows1252(text);
    const bytes = new Uint8Array(encoded.length + 1);
    bytes.set(encoded);
    return bytes;
}

// CRT _stricmp: 두 NUL 종료 문자열을 ASCII 대문자만 소문자로 바꿔 비교하고 첫 차이(소문자 값의 차)를 돌려준다. 두 입력 모두 물리 안에 NUL 이 있어야 한다. (2026-11-13 WB04-E396)
function compareIgnoringCase(first, second) {
    cstringText(first);
    cstringText(second);
    // 같은 위치의 바이트를 소문자로 바꿔 차이 또는 공통 NUL 까지 진행한다.
    for (let index = 0; ; index += 1) {
        const left = first[index] >= 0x41 && first[index] <= 0x5a ? first[index] + 0x20 : first[index];
        const right = second[index] >= 0x41 && second[index] <= 0x5a ? second[index] + 0x20 : second[index];
        if (left !== right || left === 0) return left - right;
    }
}

// 같은 Client 의 구름 전역 소유자이다. 사용: const cloud = new ClientKencloudGlobals(); cloud.readOptions(data); cloud.build(data.files, data, random); cloud.draw(screen, [0, 0, 640, 480, 0, 0], data.files, data, random).
// allocation 은 새 블록·지역 scratch 의 현재 할당 입력 훅이며(생략/null 이면 currentAllocationBytes), 게임 계산을 하지 않는다. (2026-11-13 WB04-E396)
export class ClientKencloudGlobals {
    // 연속 전역 2054 DWORD 와 그 버퍼의 이름 보기, 옵션 8 DWORD, 고정 행 순서, 팔레트 이름이다.
    #state = new Uint32Array(STATE_WORDS);
    #cloudName = new Uint8Array(this.#state.buffer, NAME_OFFSET, NAME_BYTES);
    #options = Uint32Array.from(OPTION_DEFAULTS);
    #rows = Uint32Array.from(PHASE_ROWS);
    #paletteName = new Uint8Array(PALETTE_NAME_BYTES);
    // 생존 주소 → 블록(주소 오름차순으로 들어간다)과 다음 host 주소 토큰, 현재 할당 입력 훅이다.
    #blocks = new Map();
    #nextAddress = FIRST_ADDRESS;
    #allocation;
    // 각 PathSpec 반환을 동기 부모 동안 유지하는 NUL 포함 바이트이다.
    #filePath = new Uint8Array(0);
    #palettePath = new Uint8Array(0);
    // 연결한 Renderer(비소유)와 그 opaque32 토큰, 다음 토큰, 0059a8b0 dirty DWORD 와 그 상자이다.
    #renderer = null;
    #rendererToken = 0;
    #nextRendererToken = FIRST_RENDERER;
    #redraw = 0;
    #redrawBox;

    // PE 최초값과 BSS 0 만 구성하며 파일/난수/구름 생성을 실행하지 않는다. allocation 이 함수가 아니면(생략/null 제외) TypeError 이다.
    constructor(allocation = null) {
        if (allocation !== null && allocation !== undefined && typeof allocation !== 'function') throw new TypeError('구름 현재 할당 입력은 함수여야 합니다.');
        this.#allocation = allocation ?? currentAllocationBytes;
        const owner = this;
        this.#redrawBox = {
            // 같은 0059a8b0 DWORD 를 읽는다.
            get value() { return owner.#redraw; },
            // 같은 DWORD 에 uint32 로 쓴다.
            set value(next) { owner.#redraw = next >>> 0; },
        };
    }

    // 005697f0..0056b807 의 같은 물리와 옵션을 raw-kencloud.js 전역 묶음으로 빌려 준다(getter/setter 와 살아 있는 표 보기, 필드 사본 없음). lowColor·speed·fury·seedCount 는 읽기 전용이다.
    globals() {
        const state = this.#state;
        const options = this.#options;
        return {
            // 00569ffc 현재 512×512 캐시 주소 토큰이다.
            get cache() { return state[CACHE]; },
            set cache(value) { state[CACHE] = value; },
            // 00569ff0 캐시 끝 주소이다(cache 변경 때 자동 동기화하지 않는다).
            get end() { return state[END]; },
            set end(value) { state[END] = value; },
            // 00569ff8 생성 당시 cache+512 이다.
            get nextRow() { return state[NEXT_ROW]; },
            set nextRow(value) { state[NEXT_ROW] = value; },
            // 00569ff4 생성 당시 end−512 이다.
            get lastRow() { return state[LAST_ROW]; },
            set lastRow(value) { state[LAST_ROW] = value; },
            // 0056b804 현재 위상이다.
            get phase() { return state[PHASE]; },
            set phase(value) { state[PHASE] = value; },
            // 0056b7fc 전체 표시 요청이다.
            get allDirty() { return state[ALL]; },
            set allDirty(value) { state[ALL] = value; },
            // 0056b800 부분 표시 요청이다.
            get partialDirty() { return state[PART]; },
            set partialDirty(value) { state[PART] = value; },
            // 00540a88 간격(난수 256 + 5)이다.
            get spacer() { return options[KencloudOption.spacer]; },
            set spacer(value) { options[KencloudOption.spacer] = value; },
            // 00540a34 fallback 채움 색이다.
            get lowColor() { return options[KencloudOption.lowColor]; },
            // 00540a80 cloudSpeed 이다.
            get speed() { return options[KencloudOption.speed]; },
            // 00540a84 cloudFury(signed 로 10000 을 나눔)이다.
            get fury() { return options[KencloudOption.fury]; },
            // 00540a90 seed 개수이다.
            get seedCount() { return options[KencloudOption.seedCount]; },
            phaseRows: this.#rows,
            dirtyRows: state.subarray(ROWS, ROWS + 512),
            seeds: state.subarray(SEEDS, SEEDS + SEED_COUNT),
        };
    }

    // raw-fort-palette-transition.js 전역 묶음이다: 별도 팔레트 이름, 같은 구름 이름 물리, cache·speed·Renderer 토큰·0059a8b0 을 살아 있는 값으로 빌려 준다.
    paletteGlobals() {
        const state = this.#state;
        return {
            paletteName: this.#paletteName,
            cloudName: this.#cloudName,
            cache: {
                // 같은 00569ffc DWORD 를 읽는다.
                get value() { return state[CACHE]; },
                // 같은 DWORD 에 쓴다.
                set value(next) { state[CACHE] = next; },
            },
            count: () => this.#options[KencloudOption.speed],
            renderer: () => this.#rendererToken,
            dirty: this.#redrawBox,
        };
    }

    // 실제 연속 전역 2054 DWORD 이며 인접 별칭(이름 보기·표 보기)과 같은 배열이다.
    state() {
        return this.#state;
    }

    // 옵션 한 DWORD 를 `{ value }` 상자로 빌려 준다(원문 참조). option 이 0..7 정수가 아니면 RangeError 이다.
    value(option) {
        if (!Number.isInteger(option) || option < 0 || option >= this.#options.length) throw new RangeError('구름 옵션 번호 범위');
        const options = this.#options;
        return {
            // 같은 옵션 DWORD 를 읽는다.
            get value() { return options[option]; },
            // 같은 옵션 DWORD 에 uint32 로 쓴다.
            set value(next) { options[option] = next; },
        };
    }

    // 보호 PE 의 고정 16행 순서표(같은 배열)이다. 호출자는 쓰지 않는다.
    phaseRows() {
        return this.#rows;
    }

    // 005c8910 팔레트 이름의 같은 128바이트 물리이다(Session 교체로 지우지 않는다).
    paletteName() {
        return this.#paletteName;
    }

    // 0056b7a0 구름 이름 물리이다. 연속 전역 바이트 8112 부터 104바이트로, 뒤의 kenCloud/algorithm/dirty/phase DWORD 와 같은 물리이다.
    cloudName() {
        return this.#cloudName;
    }

    // 현재 값을 기본 입력으로 설정을 읽어 "찾았고 값이 다른" 경우에만 저장한다(signed 해석 → DWORD). 누락 키는 현재 값을 보존하고 파일·난수를 쓰지 않는다.
    // config 는 `readInt(key, current) → { changed, value }` 를 가진 설정 인터페이스(GameData)이다.
    readOptions(config) {
        // 한 키를 읽어 성공한 store 만 같은 배열 칸에 적용한다.
        const read = (key, array, index) => {
            const { changed, value } = config.readInt(key, array[index] | 0);
            if (changed) array[index] = value;
        };
        read('cloudiness', this.#options, KencloudOption.cloudiness);
        read('cloudSpeed', this.#options, KencloudOption.speed);
        read('cloudFury', this.#options, KencloudOption.fury);
        read('cloudSpacer', this.#options, KencloudOption.spacer);
        read('cloudRecenter', this.#options, KencloudOption.recenter);
        read('cloudAlgo', this.#state, ALGORITHM);
        read('kenCloud', this.#state, KEN_CLOUD);
    }

    // 겹치지 않는 32비트 주소 토큰과 정확한 길이 블록을 만들고 현재 할당 입력으로 채운다. bytes 는 uint32 로 해석하며 0 이면 RangeError, 주소 공간이 모자라면 RangeError 이다.
    allocate(bytes) {
        const size = bytes >>> 0;
        if (size === 0) throw new RangeError('구름 할당의 실제 물리 크기0');
        const extent = Math.ceil(size / PAGE) * PAGE;
        if (this.#nextAddress + extent + PAGE > 0xffffffff) throw new RangeError('구름 DWORD 주소 공간 소진');
        const block = new Uint8Array(size);
        this.#allocation(block);
        const token = this.#nextAddress;
        this.#blocks.set(token, block);
        this.#nextAddress = token + extent + PAGE;
        return token;
    }

    // 내부 주소부터 같은 블록의 남은 물리를 빌린다(끝 주소는 길이 0). 생존 블록 밖·해제된 주소는 RangeError 이다.
    memory(address) {
        const target = address >>> 0;
        let base = -1;
        // 주소 오름차순 블록 중 target 이하의 가장 큰 base 를 찾는다.
        for (const token of this.#blocks.keys()) {
            if (token > target) break;
            base = token;
        }
        if (base < 0) throw new RangeError('구름 현재 주소 물리 누락');
        const block = this.#blocks.get(base);
        const offset = target - base;
        if (offset > block.length) throw new RangeError('구름 현재 주소 물리 범위');
        return block.subarray(offset);
    }

    // 정확한 생존 base 만 반납한다. 0 은 무시하고, 없는 base 는 RangeError 이다. 원문 부모의 cache 0 쓰기는 대신하지 않는다.
    release(token) {
        if (!token) return;
        if (!this.#blocks.delete(token >>> 0)) throw new RangeError('구름 실제 반납 base 누락');
    }

    // 현재 생존 블록 수이다.
    allocations() {
        return this.#blocks.size;
    }

    // 원문 파일 부모(00470bb0)에 실제 경로·존재·해독·반납 장치를 연결한다. 지역 scratch 264바이트는 현재 할당 입력으로 채운다.
    #loadFile(files, config) {
        const raw = new Uint8Array(FILE_SCRATCH_BYTES);
        this.#allocation(raw);
        const view = new DataView(raw.buffer);
        const scratch = {
            // 원문 local+0 의 현재 signed DWORD 이다.
            get height() { return view.getInt32(0, true); },
            set height(value) { view.setInt32(0, value, true); },
            // 원문 local+4 의 현재 signed DWORD 이다.
            get width() { return view.getInt32(4, true); },
            set width(value) { view.setInt32(4, value, true); },
            path: raw.subarray(8),
        };
        const hooks = {
            // 004410f0("CloudFileSpec", 0, 0, 0) 의 NUL 물리를 동기 부모 끝까지 유지한다.
            pathSpec: (key, first, second, third) => {
                if (first || second || third) throw new Error('구름 실제 FileSpec 인자 계약');
                this.#filePath = cstringBytes(config.pathSpec(key));
                return this.#filePath;
            },
            // 0041ab10 의 존재 조회이다(웹은 위치 코드 대신 1/0).
            exists: (path) => (files.has(cstringText(path)) ? 1 : 0),
            // 004dae60 의 치수 선행 쓰기 → 할당 → 해독 → 같은 블록 복사이다.
            decode: (path, current) => this.#decodeFile(files, cstringText(path), current),
            // 512×512 가 아닌 이미지(0 포함)를 그대로 반납한다.
            release: (token) => this.release(token),
        };
        return loadKencloudFile(scratch, hooks);
    }

    // GIF 파일 부모의 웹 적응이다: 파일이 없으면 원문 진단 0xfb, 첫 descriptor 치수를 먼저 쓰고 크기 0/16MB 초과를 거부한 뒤 할당·해독·복사한다. 예외 때 이미 한 할당은 반납하지 않는다.
    #decodeFile(files, name, scratch) {
        const bytes = files.read(name);
        if (!bytes) throw new Error('GIF 파일 부모의 원문 진단 251');
        const { width, height } = gifFileDimensions(bytes);
        scratch.width = width;
        scratch.height = height;
        const size = Math.imul(width, height) >>> 0;
        if (size === 0 || size > GIF_LIMIT) throw new RangeError('구름 GIF 실제 할당 폭');
        const token = this.allocate(size);
        const image = decodeBackgroundGif(bytes);
        if (image.width !== width || image.height !== height || image.indices.length !== size) throw new Error('구름 Native descriptor/decoder 치수 불일치');
        this.memory(token).set(image.indices);
        return token;
    }

    // 원문 생성 부모가 쓰는 파일·할당·메모리 장치이다(같은 동기 호출보다 오래 빌리지 않는다).
    #devices(files, config) {
        return {
            loadFile: () => this.#loadFile(files, config),
            allocate: (bytes) => this.allocate(bytes),
            memory: (address) => this.memory(address),
        };
    }

    // 같은 실제 난수와 파일 부모로 원문 생성(00471040)을 실행한다. 캐시가 있으면 표시 요청만 한다. random 은 `next(limit)`/`state` 를 가진 같은 GameRandom 이다.
    build(files, config, random) {
        buildKencloud(this.globals(), random, this.#devices(files, config));
    }

    // 같은 현재 캐시/행/dirty 에 원문 행 이동(00470b20)을 실행한다. 난수를 쓰지 않는다.
    step() {
        stepKencloud(this.globals(), { memory: (address) => this.memory(address) });
    }

    // raw-kencloud-render.js 전역 묶음이다: 같은 2054 DWORD 와 현재 speed 를 빌려 준다.
    renderGlobals() {
        const options = this.#options;
        return {
            state: this.#state,
            // 00540a80 현재 speed 를 읽는다.
            get speed() { return options[KencloudOption.speed]; },
        };
    }

    // 00470c50: 캐시가 없으면 생성한 뒤 speed 를 다시 읽고, 캡처한 횟수만큼 행 이동을 실행한다.
    update(files, config, random) {
        updateKencloud(this.renderGlobals(), { build: () => this.build(files, config, random), step: () => this.step() });
    }

    // 00470c80: 전체 1 → 부분 0 순서의 표시 요청이다.
    markDirty() {
        markKencloudDirty(this.#state);
    }

    // 00470ca0: 실제 이전 표시 행만 지우고 남은 부분 요청을 정리한다.
    cleanDirty() {
        cleanKencloudDirty(this.#state);
    }

    // 00470ce0: 합의 signed 나머지 행(음수는 앞선 같은 물리)이나 전체 요청이 켜져 있으면 1 이다.
    rowDirty(first, second) {
        return kencloudRowDirty(this.#state, first, second);
    }

    // 004711d0/00471410: 현재 Screen 의 clip/잠금/DIB 에 두 표시 부모를 실행한다. area = [left, top, right, bottom, x, y]. alternate 잠금은 장치 미복원이라 Error 이다.
    // screen 은 `clip()`→{left,top,right,bottom}·`setClip(l,t,r,b)`·`lock()`·`unlock()`·`blitBuffer(x, y, bytes, pitch, rect)` 를 가진 화면 장치(render/screen.js)이다.
    draw(screen, area, files, config, random, alternate = false) {
        const hooks = {
            build: () => this.build(files, config, random),
            // 현재 Screen 의 같은 네 값을 DWORD 로 읽는다.
            clip: () => {
                const clip = screen.clip();
                return [clip.left >>> 0, clip.top >>> 0, clip.right >>> 0, clip.bottom >>> 0];
            },
            setClip: (clip) => screen.setClip(clip[0] | 0, clip[1] | 0, clip[2] | 0, clip[3] | 0),
            // 두 잠금 장치를 같은 GDI 잠금으로 속이지 않는다.
            lock: (special) => {
                if (special) throw new Error('구름 parallax DirectDraw 004a15d0 장치 미복원');
                screen.lock();
            },
            unlock: (special) => {
                if (special) throw new Error('구름 parallax DirectDraw 004a1650 장치 미복원');
                screen.unlock();
            },
            // 현재 cache 의 같은 생존 바이트를 복사본 없이 읽는다.
            blit: (args) => screen.blitBuffer(args[0] | 0, args[1] | 0, this.memory(args[2]), args[3] | 0, { left: args[4] | 0, top: args[5] | 0, right: args[6] | 0, bottom: args[7] | 0 }),
        };
        drawKencloud(this.renderGlobals(), area, hooks, alternate);
    }

    // 004b6100: 호출자 장치(log/loadPalette/buildMaps 등)에 비교·반납·생성·이동·"GamePalSpec" 경로를 더해 팔레트 이름 전환을 실행한다. 호출자 객체는 바꾸지 않는다.
    // name 은 NUL 종료 Uint8Array 이며 log 복귀 뒤에 읽힌다.
    transition(name, files, config, random, devices) {
        const hooks = {
            ...devices,
            compare: (first, second) => compareIgnoringCase(first, second),
            release: (token) => this.release(token),
            buildCloud: () => this.build(files, config, random),
            stepCloud: () => this.step(),
            // 팔레트 이름을 local.1 로 넣은 경로를 동기 부모 끝까지 유지한다.
            pathSpec: (key, input, second, third) => {
                if (second || third) throw new Error('팔레트 실제 FileSpec 인자 계약');
                this.#palettePath = cstringBytes(config.pathSpec(key, cstringText(input)));
                return this.#palettePath;
            },
        };
        transitionFortPalette(() => name, this.paletteGlobals(), hooks);
    }

    // 현재 Renderer 를 소유하지 않고 연결한다. 같은 객체면 그대로, null 이면 토큰 0, 새 객체면 0x3b000000 부터 0x1000 간격의 다음 토큰이다(0xffffe000 초과는 RangeError).
    bindRenderer(renderer) {
        if (renderer === this.#renderer) return;
        this.#renderer = renderer ?? null;
        if (!renderer) {
            this.#rendererToken = 0;
            return;
        }
        if (this.#nextRendererToken > RENDERER_LIMIT) throw new RangeError('현재 Renderer 주소 토큰 소진');
        this.#rendererToken = this.#nextRendererToken;
        this.#nextRendererToken += PAGE;
    }

    // 현재 Renderer 존재 토큰(0059a8bc)이다.
    rendererToken() {
        return this.#rendererToken;
    }

    // 0059a8b0 world redraw DWORD 의 `{ value }` 상자이다(paletteGlobals().dirty 와 같은 상자).
    worldRedraw() {
        return this.#redrawBox;
    }
}
