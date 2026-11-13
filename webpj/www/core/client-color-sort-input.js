// 실제 ColorSort 가 최초 논리 RGB 를 복사하는 수명을 소유하고 기존 색표 계산을 같은 고정 입력에 합성하는 Client 소유자이다(cpppj `client/ClientColorSortInput.cpp/.h`, 10.78 전용).
//  - capture(logical)/captureWithPrimary(logical, allocationBytes): 439de0 의 명시 생성 위치에서만 전체 RGBX 256개를 복사한다(일반 palette 변경은 부르지 않음). 후자는 같은 입력으로
//    RawColorSortPrimary(정확 0x3800 초기 물리)를 만들어 construct 한 뒤에만 발행하고, 실패하면 부분 raw 를 성공 입력으로 주지 않는다.
//  - primary: 누락/미완성은 거부한다(RGB/COL 에서 즉석 대체하지 않음). logical: 미캡처는 거부. ownerTables: 현재 배너/소유자 표 저장소(OwnerColorTableState)를 모든 소비자에게 같은 객체로 빌려 준다.
//  - rebuild(currentRed): 43c010 의 현재 빨강과 같은 최초 입력으로 기본 53비트 9표를 갱신한다. 순서: [0] RGB 1.6배 L1 최근접(첫 표, 228..245·254·255 항등) · [1] 파괴 · [2] 불법 · [3] 항등 · [4] 밝음 ·
//    [5] 글자 그림자 · [6] 건설 · [7] 그림자 · [8] HLS 명도 최근접((명도+0.2)/3, 254·255 항등). 64비트 별도 출력은 섞지 않는다. maps: 완료 뒤 같은 표 배열을 빌린다.
// colorSortLogicalInput(fixed, independentCurrent): 실제 Client 는 고정 초기 RGB(256)를 필수로 주고, 소유자 없는 독립 표시 검사만 명시한 현재 입력으로 되돌아간다(길이가 256 이 아니면 거부).
//  - initializeCache(files, path, currentRed, initializeOwners): 최초 43c4e0 — 소유자 표 두 개·TYPE 등록(initializeOwners)과 소유자 8행 ownerHighlight/ownerSolid 출력을 만들고 현재 파일(files)에서 아홉 표를 읽는다(hit).
//    파일이 없으면(ColorCacheUnableToOpen) rebuild(currentRed)로 계산해 아홉 표를 파일 순서로 써서 남긴다(miss). 짧은 파일은 첫 목적 표의 접두만 바꾸고 나머지 표는 재생성하거나 파일을 고치지 않는다.
//    파일 순서는 주소 순서와 달리 maps[2]·[7]·[6]·[4]·[3]·[5]·[1]·[0]·[8] 이다. ownerHighlight/ownerSolid 는 캐시 파일에 포함하지 않는 소유자 8행 출력(명도 > 0.8 이면 소유자 색)이다.
//    files 는 core/color-map-files.js 의 웹 적응 저장소(read(path)·createWriter(path))이다(cpppj BaseFileSystem 대응, adapt-file).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E374: 5-A ClientColorSortInput 이식으로 추가, 2026-11-13 WB04-E375: initializeCache·소유자 8행 출력 추가, 기준선 뒤 cpppj 변경 없음)
import { RawColorSortPrimary, COLOR_SORT_PRIMARY_BYTES } from './raw-color-sort-primary.js';
import { OwnerColorTableState } from './owner-color-table-state.js';
import { buildSpriteColorMaps } from './sprite-color-maps.js';
import { buildPreviewColorMaps } from './preview-color-maps.js';
import { currentAllocationBytes } from './current-allocation-bytes.js';
import { ColorCacheUnableToOpen, initializeColorMapCache } from './raw-color-map-cache.js';

// 팔레트 크기, 표 개수, 원본 503608/5066f8/5066e0/506558 의 double 상수(정규화·1.6배·명도 보정·1/3)와 초기 최소 거리이다. (2026-11-13 WB04-E374)
const COLORS = 256;
const MAP_COUNT = 9;
const NORMALIZE = 1.0 / 255.0;
const SCALE = 1.600000023841858;
const OFFSET = 0.20000000298023224;
const THIRD = 0.3333333333333333;
const INITIAL_DISTANCE = Math.fround(1e6);
const LIGHT_INITIAL = 1e6;
// 43c010 꼬리가 덮는 예약 구간(228..245)과 마지막 두 번호이다. (2026-11-13 WB04-E374)
const IDENTITY_FIRST = 228;
const IDENTITY_LAST = 245;
// 원문 005319c8 의 현재 초기 소유자 색 8 DWORD, 캐시 파일의 표 순서(주소 순서 인덱스), 소유자 8행 출력 크기, 한 표의 바이트 수이다. (2026-11-13 WB04-E375)
const OWNER_COLORS = Object.freeze([84, 249, 184, 140, 150, 112, 156, 222]);
const FILE_ORDER = Object.freeze([2, 7, 6, 4, 3, 5, 1, 0, 8]);
const OWNER_TABLE_BYTES = 2048;
const TABLE_BYTES = 256;

// 실제 Client 의 고정 초기 입력(있으면)과 소유자 없는 독립 검사의 현재 입력 중 쓸 256색 배열을 고른다. (2026-11-13 WB04-E374)
export function colorSortLogicalInput(fixed, independentCurrent) {
    if (!fixed || fixed.length === 0) return independentCurrent;
    if (fixed.length !== COLORS) throw new TypeError('ColorSort 고정 초기 RGB 256색 계약');
    return fixed;
}

// 같은 raw 0x3800 과 고정 RGB/기존 9표를 소유한다. 사용: const input = new ClientColorSortInput(); input.captureWithPrimary(logical, allocation); input.rebuild(189). (2026-11-13 WB04-E374)
export class ClientColorSortInput {
    // 주소가 변하지 않는 최초 ColorSort 입력(미캡처 0 은 유효 입력이 아님), 발행한 primary, 현재 소유자 색표 저장소, 9표, 준비 표식이다.
    #logical = new Uint32Array(COLORS);
    #primary = null;
    #ownerTables = new OwnerColorTableState();
    #maps = Array.from({ length: MAP_COUNT }, () => new Uint8Array(COLORS));
    #ownerHighlight = new Uint8Array(OWNER_TABLE_BYTES);
    #ownerSolid = new Uint8Array(OWNER_TABLE_BYTES);
    #captured = false;
    #built = false;

    // 기존 독립 adapter 의 logical 캡처이다: 실제 primary 가 있다고 가장하지 않고 이전 raw 연결을 끊는다.
    capture(logical) {
        this.#primary = null;
        this.#copy(logical);
        this.#captured = true;
        this.#built = false;
    }

    // 실제 malloc 초기 BYTE(allocationBytes, 생략하면 현재 힙 입력 대응)를 읽으며 정상 constructor 반환 뒤에만 새 primary 를 발행한다. 실패 때 부분 raw 를 성공 입력으로 주지 않는다.
    captureWithPrimary(logical, allocationBytes = null) {
        this.#primary = null;
        this.#captured = false;
        this.#built = false;
        const allocation = allocationBytes ?? new Uint8Array(COLOR_SORT_PRIMARY_BYTES);
        if (allocationBytes === null) currentAllocationBytes(allocation);
        this.#copy(logical);
        const primary = new RawColorSortPrimary(allocation);
        primary.construct(new Uint8Array(this.#logical.buffer, this.#logical.byteOffset, this.#logical.byteLength));
        this.#primary = primary;
        this.#captured = true;
    }

    // 입력 256색을 복사한다(호출자 배열의 이후 변경은 전파하지 않는다).
    #copy(logical) {
        if (!logical || logical.length !== COLORS) throw new TypeError('ColorSort 논리 RGB 는 256색이어야 합니다.');
        for (let index = 0; index < COLORS; index += 1) this.#logical[index] = logical[index] >>> 0;
    }

    // 같은 현재 슬롯만 빌린다. 누락/미완성은 명시 거부한다.
    get primary() {
        if (!this.#primary || !this.#primary.constructed) throw new Error('ColorSort 실제 primary 미연결');
        return this.#primary;
    }

    // 현재 배너/소유자 표를 모든 표시 소비자에게 같은 주소로 빌려 준다.
    get ownerTables() { return this.#ownerTables; }

    // 파일 hit 또는 실제 9표 생성의 같은 준비 표식이다.
    get mapsReady() { return this.#built; }

    // 호스트 입력 계약의 준비 상태이다(원문 paletteLoaded 와 별개).
    get captured() { return this.#captured; }

    // 초기 입력의 같은 배열을 빌린다. 미캡처는 명시 오류이다.
    get logical() {
        if (!this.#captured) throw new Error('ColorSort 최초 논리 RGB 미캡처');
        return this.#logical;
    }

    // 완료 뒤 같은 현재 9표 배열을 빌린다. 미생성은 명시 오류이다.
    get maps() {
        if (!this.#built) throw new Error('ColorSort 실제 9표 미생성');
        return this.#maps;
    }

    // 같은 43c4e0 의 8행 명도 치환 출력(명도 > 0.8 이면 소유자 색)이다. 초기화 전에는 거부한다.
    get ownerHighlight() {
        void this.maps;
        return this.#ownerHighlight;
    }

    // 같은 43c4e0 의 8행 고정색 출력이며 캐시 hit 에서도 먼저 작성한다. 초기화 전에는 거부한다.
    get ownerSolid() {
        void this.maps;
        return this.#ownerSolid;
    }

    // 최초 43c4e0: 소유자 두 표/등록(initializeOwners)과 소유자 8행 출력 뒤 현재 파일 9read(hit) 또는 계산 9write(miss)를 같은 저장소에 연결한다. path 는 문자열이고 files 는 색표 캐시 저장소이다.
    initializeCache(files, path, currentRed, initializeOwners) {
        void this.primary;
        if (this.#built || typeof initializeOwners !== 'function') throw new Error('색표 캐시 최초 초기화 계약');
        const name = Uint8Array.from([...new TextEncoder().encode(path), 0]);
        const readerObject = new Uint8Array(24);
        const writerObject = new Uint8Array(24);
        let source = null;
        let position = 0;
        let sink = null;
        const globals = {
            primary: () => this.primary.bytes,
            ownerColors: OWNER_COLORS,
            ownerHighlight: this.#ownerHighlight,
            ownerSolid: this.#ownerSolid,
            maps: FILE_ORDER.map((index) => this.#maps[index]),
        };
        const hooks = {
            path: () => name,
            owners: initializeOwners,
            buildMaps: () => { this.rebuild(currentRed); },
            // 파일이 없으면 열 수 없음으로 재생성에 들어가고 있으면 현재 전체 내용을 읽기용으로 연다.
            openRead: () => {
                source = files.read(path);
                if (source === null) throw new ColorCacheUnableToOpen();
                position = 0;
            },
            openWrite: () => { sink = files.createWriter(path); },
            // 짧은 읽기는 목적 표의 접두만 바꾸고 반환 길이를 재시도 조건으로 쓰지 않는다.
            read: (file, table) => {
                const count = Math.min(TABLE_BYTES, source.length - position);
                table.set(source.subarray(position, position + count));
                position += count;
                return count;
            },
            write: (file, table) => { sink.write(table); return TABLE_BYTES; },
            close: (file) => { if (file === writerObject) sink?.close(); },
        };
        initializeColorMapCache(globals, hooks, readerObject, writerObject);
        this.#built = true;
    }

    // 43c010: 현재 빨강과 같은 초기 입력으로 기본 53비트 9표를 갱신해 돌려준다.
    rebuild(currentRed) {
        const logical = this.logical;
        const sprite = buildSpriteColorMaps(logical);
        const preview = buildPreviewColorMaps(logical, currentRed);
        const maps = this.#maps;
        maps[1].set(preview.destroy);
        maps[2].set(preview.illegal);
        maps[4].set(sprite.bright);
        maps[5].set(preview.labelShadow);
        maps[6].set(sprite.dead);
        maps[7].set(sprite.shadow);
        const rgb = new Float64Array(COLORS * 3);
        const light = new Float64Array(COLORS);
        // 고정 초기 RGB 의 정규화와 명도를 원본 번호 순서로 작성한다.
        for (let index = 0; index < COLORS; index += 1) {
            const red = (logical[index] & 255) * NORMALIZE;
            const green = ((logical[index] >>> 8) & 255) * NORMALIZE;
            const blue = ((logical[index] >>> 16) & 255) * NORMALIZE;
            rgb[index * 3] = red;
            rgb[index * 3 + 1] = green;
            rgb[index * 3 + 2] = blue;
            light[index] = (Math.max(red, green, blue) + Math.min(red, green, blue)) * 0.5;
        }
        // 두 미노출 표와 항등 표의 전체 256바이트를 같은 초기 입력에서 작성한다.
        for (let index = 0; index < COLORS; index += 1) {
            const targetRed = rgb[index * 3] * SCALE;
            const targetGreen = rgb[index * 3 + 1] * SCALE;
            const targetBlue = rgb[index * 3 + 2] * SCALE;
            const targetLight = (light[index] + OFFSET) * THIRD;
            let minimum = INITIAL_DISTANCE;
            let lightMinimum = LIGHT_INITIAL;
            let selected = 0;
            // 동률/색 번호 순서를 보존한 현재 후보 검색이다(RGB 최솟값만 float32 로 좁힌다).
            for (let candidate = 0; candidate < COLORS; candidate += 1) {
                const distance = Math.abs(targetRed - rgb[candidate * 3]) + Math.abs(targetGreen - rgb[candidate * 3 + 1]) + Math.abs(targetBlue - rgb[candidate * 3 + 2]);
                if (distance < minimum) {
                    minimum = Math.fround(distance);
                    maps[0][index] = candidate;
                }
                const lightDistance = Math.abs(targetLight - light[candidate]);
                if (lightDistance < lightMinimum) {
                    lightMinimum = lightDistance;
                    selected = candidate;
                }
            }
            maps[8][index] = selected;
            maps[3][index] = index;
        }
        // 43c010 꼬리의 첫 표 예약 번호 덮어쓰기이다.
        for (let index = IDENTITY_FIRST; index <= IDENTITY_LAST; index += 1) maps[0][index] = index;
        maps[0][254] = 254;
        maps[0][255] = 255;
        maps[8][254] = 254;
        maps[8][255] = 255;
        this.#built = true;
        return maps;
    }
}
