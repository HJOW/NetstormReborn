// 팔레트 이름 전환 raw-fort-palette-transition.js(004b6100 전환·00470c30 캐시 해제·00471190 구름 이름·004973c0 dirty)를 원본 10.78 관측과 cpppj `FortPaletteTransitionTests` 계약으로 대조한다.
//  - 원본 관측 fort-palette-transition-x86.tsv 676행(두 FPCW 관측 × 4개 진입점 입력): 반환/물리 중단 원인·현재 11개 필드·이름 두 버퍼와 입력 두 개와 경로 두 개의 전체 물리(raw)·하위 호출 trace 를
//    callback 변이(인자 변수 교체, source 내용 변경, 캐시·횟수·렌더러·dirty 변경, 경로 조회 반환 교체, 중단)까지 포함해 한 바이트도 다르지 않게 대조한다. 중단 34행 포함.
//  - 계약: 같은 이름의 자연 반환은 하위 장치를 부르지 않고, 도달한 필수 장치(loadPalette) 누락은 원래 부분 효과를 남긴 채 거부한다.
// 실제 파일·GUI 는 열지 않는다. (2026-11-10 WB04-E227: 팔레트 전환 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { markFortPaletteDirty, releaseFortPaletteCloudCache, setFortPaletteCloudName, transitionFortPalette } from '../www/core/raw-fort-palette-transition.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(6칸: 대조용 FPCW 열·입력·반환/중단·11개 필드·raw·trace). (2026-11-10 WB04-E227)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fort-palette-transition-x86.tsv'));

// 원문 고정 이름 주소를 trace 에만 표현한다. Kencloud 이름은 팔레트 이름과 별개 주소이다. 격리 장치의 두 512 BYTE 입력·두 256 BYTE 경로 위치와 cache 토큰 시작 주소이다. (2026-11-10 WB04-E227)
const PALETTE_NAME = 0x5c8910;
const CLOUD_NAME = 0x56b7a0;
const INPUT = 0x12400000;
const PATH = INPUT + 0x1000;
const POOL = 0x12000000;

// 소문자 hex 로 표시한다(NUL 뒤 BYTE 도 생략하지 않는다). (2026-11-10 WB04-E227)
const hex = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');

// 명시 하위 callback 또는 물리 접근 중단만 나타내며 부모의 정상 반환과 구별한다. (2026-11-10 WB04-E227)
class PaletteStop {
    constructor(cause) { this.cause = cause; }
}

// 장치 입력/버퍼 오류(모듈의 RangeError 와 구별하려고 일반 Error 를 쓴다). (2026-11-10 WB04-E227)
function deviceError(message) { return new Error(message); }

// NUL 까지 쓰되 주변 BYTE 는 유지하여 alias/남은 물리를 검증한다. (2026-11-10 WB04-E227)
function putString(target, value) {
    if (value.length + 1 > target.length) throw deviceError('검사 장치 입력 길이');
    for (let index = 0; index < value.length; index += 1) target[index] = value.charCodeAt(index);
    target[value.length] = 0;
}

// 읽기 범위 안에 NUL 이 없는 경우 실제 하위 접근 실패를 명시 중단으로 돌린다. 반환은 NUL 앞까지의 부분이다. (2026-11-10 WB04-E227)
function cString(bytes, cause) {
    const found = bytes ? bytes.indexOf(0) : -1;
    if (!bytes || bytes.length === 0 || found === -1) throw new PaletteStop(cause);
    return bytes.subarray(0, found);
}

// 동일한 실제 버퍼를 빌리고 하위 효과만 기록한다. 부모의 제어 흐름은 이 장치가 계산하지 않는다. (2026-11-10 WB04-E227)
class PaletteDevice {
    constructor(values) {
        assert.equal(values.length, 9);
        this.palette = new Uint8Array(257); // 고정 이름 앞 1 BYTE 와 뒤 256 BYTE 전체 물리이다.
        this.cloud = new Uint8Array(257);
        this.inputs = [new Uint8Array(512), new Uint8Array(512)]; // 입력 인자 변수 변경과 같은 입력 내용 변경을 구별한다.
        this.paths = [new Uint8Array(256), new Uint8Array(256)]; // 경로 조회 반환 주소의 실제 물리이다.
        this.calls = { free: 0, build: 0, step: 0, load: 0, maps: 0 };
        this.trace = [];
        [this.kind] = values;
        this.nameCase = Number(values[1]);
        this.cache = PaletteDevice.token(Number(values[2]));
        this.count = Number(values[3]);
        this.renderer = Number(values[4]);
        this.dirty = Number(values[5]);
        this.built = PaletteDevice.token(Number(values[6]));
        this.pathIndex = Number(values[7]);
        this.mutation = values[8];
        this.palette.fill(0xa1);
        this.palette[0] = 'Z'.charCodeAt(0);
        this.cloud.fill(0xb2);
        this.cloud[0] = 'Y'.charCodeAt(0);
        putString(this.palette.subarray(1), this.nameCase === 1 || this.nameCase === 2 ? 'gifcloud' : 'OldPalette');
        putString(this.cloud.subarray(1), 'LegacySort');
        // 각 입력의 NUL 뒤 바이트까지 서로 다른 초기값을 보존한다.
        for (let input = 0; input < this.inputs.length; input += 1) for (let byte = 0; byte < this.inputs[input].length; byte += 1) this.inputs[input][byte] = (0xc0 + input * 7 + byte) & 0xff;
        let name = this.nameCase === 1 ? 'GIFCLOUD' : this.nameCase === 3 ? '' : 'gifcloud'; // 일반 입력의 실제 C 문자열이다.
        if (this.nameCase === 6) name = 'LongPaletteName'.repeat(14); // 210자 입력도 원문 길이대로 복사한다.
        putString(this.inputs[0], name);
        putString(this.inputs[1], 'SecondPalette');
        if (this.nameCase === 8) this.inputs[0].fill('Q'.charCodeAt(0));
        this.argument = this.inputs[0];
        if (this.nameCase === 4) this.argument = this.palette.subarray(2);
        else if (this.nameCase === 5) this.argument = this.cloud.subarray(2);
        else if (this.nameCase === 7) this.argument = this.palette;
        else if (this.nameCase === 9) this.argument = this.cloud.subarray(1);
        else if (this.nameCase === 10) this.argument = this.cloud;
        this.captured = this.argument;
        // 실제 경로 조회 반환 둘의 주소와 내용도 서로 구별한다.
        for (let index = 0; index < this.paths.length; index += 1) {
            for (let byte = 0; byte < this.paths[index].length; byte += 1) this.paths[index][byte] = (0xd0 + index * 5 + byte) & 0xff;
            putString(this.paths[index], index === 0 ? 'Work/First.col' : 'Work/Second.col');
        }
    }

    // 논리 cache 입력을 격리 원문과 같은 주소 토큰으로 바꾼다. (2026-11-10 WB04-E227)
    static token(logical) { return logical === 0 ? 0 : POOL + Math.trunc(logical / 11) * 0x1000; }

    // 물리 영역과 원문 토큰의 대응(각 실제 소유 버퍼와 시작 주소)이다. (2026-11-10 WB04-E227)
    regions() {
        return [[this.palette, PALETTE_NAME - 1], [this.cloud, CLOUD_NAME - 1], [this.inputs[0], INPUT], [this.inputs[1], INPUT + 0x400], [this.paths[0], PATH], [this.paths[1], PATH + 0x200]];
    }

    // JS 뷰를 격리 PE 의 실제 주소와 대응시킨다(같은 buffer 안의 위치). 널 포인터는 0 이다. (2026-11-10 WB04-E227)
    address(view) {
        if (view === null || view === undefined) return 0;
        for (const [region, base] of this.regions()) if (view.buffer === region.buffer) return base + (view.byteOffset - region.byteOffset);
        throw deviceError('검사 장치 주소 대응 누락');
    }

    // 하위 효과가 같은 captured source 의 내용을 바꾸도록 실제 가변 물리를 찾는다. (2026-11-10 WB04-E227)
    changeSource() {
        const address = this.address(this.captured); // 입력 변수 변경과 달리 현재 캡처 주소를 쓴다.
        for (const [region, base] of this.regions()) {
            if (address >= base && address - base < region.length) { putString(region.subarray(address - base), 'ChangedByChild'); return; }
        }
        throw deviceError('검사 source 쓰기 주소 누락');
    }

    // 부모 밖에서만 발생하도록 지정한 callback 변화/중단을 적용한다. (2026-11-10 WB04-E227)
    mutate(boundary) {
        const mutation = this.mutation;
        if (mutation === `${boundary}-argumentB`) this.argument = this.inputs[1];
        else if (mutation === `${boundary}-source`) this.changeSource();
        else if (mutation === `${boundary}-palette`) putString(this.palette.subarray(1), 'ChangedPalette');
        else if (mutation === `${boundary}-cache11`) this.cache = PaletteDevice.token(11);
        else if (mutation === `${boundary}-cache33`) this.cache = PaletteDevice.token(33);
        else if (mutation === `${boundary}-cache0`) this.cache = 0;
        else if (mutation === `${boundary}-count0`) this.count = 0;
        else if (mutation === `${boundary}-count3`) this.count = 3;
        else if (mutation === `${boundary}-renderer0`) this.renderer = 0;
        else if (mutation === `${boundary}-renderer1`) this.renderer = 1;
        else if (mutation === `${boundary}-dirty77`) this.dirty = 77;
        else if (mutation === 'path-return1' && boundary === 'path') this.pathIndex = 1;
        else if (mutation === 'path-return0' && boundary === 'path') this.pathIndex = -1;
        else if (mutation === `${boundary}-stop`) throw new PaletteStop(boundary);
    }

    // 실제 DWORD/고정 버퍼의 같은 수명 참조를 새 부모에 전달한다(상자는 장치 필드의 접근자이다). (2026-11-10 WB04-E227)
    globals() {
        const device = this;
        return {
            paletteName: this.palette.subarray(1),
            cloudName: this.cloud.subarray(1),
            cache: { get value() { return device.cache; }, set value(next) { device.cache = next >>> 0; } },
            count: () => device.count,
            renderer: () => device.renderer,
            dirty: { get value() { return device.dirty; }, set value(next) { device.dirty = next >>> 0; } },
        };
    }

    // 제품 본체와 독립인 명시 CRT/계산/파일/Screen 장치이다. 실제 GUI 나 파일은 열지 않는다. (2026-11-10 WB04-E227)
    hooks() {
        const text = (bytes) => hex(bytes);
        return {
            log: (level, message) => {
                assert.equal(level, 29);
                assert.equal(message, 'init palette\n');
                this.trace.push(`LOG:${level}:${text(Buffer.from(message, 'latin1'))}`);
                this.mutate('log');
            },
            compare: (first, second) => {
                this.captured = second;
                this.trace.push(`COMPARE:${this.address(first)}:${this.address(second)}`);
                const left = cString(first, 'compare-physical');
                const right = cString(second, 'compare-physical'); // callback 전에 비교 결과를 확정한다.
                const lower = (value) => (value >= 65 && value <= 90 ? value + 32 : value); // 입력 전체가 ASCII 인 명시 CRT 장치이다.
                const equal = left.length === right.length && left.every((value, index) => lower(value) === lower(right[index]));
                this.mutate('compare');
                return equal ? 0 : 1;
            },
            release: (selected) => { this.calls.free += 1; this.trace.push(`FREE:${selected}`); this.mutate('free'); },
            buildCloud: () => { this.calls.build += 1; this.trace.push(`BUILD:${this.cache}:${this.count}`); this.cache = this.built; this.mutate('build'); },
            stepCloud: () => { this.calls.step += 1; const index = this.calls.step; this.trace.push(`STEP:${index}:${this.cache}:${this.count}`); this.mutate(`step${index}`); },
            pathSpec: (key, source, first, second) => {
                assert.ok(key === 'GamePalSpec' && first === 0 && second === 0);
                this.trace.push(`PATH:${this.address(source)}:${first}:${second}`);
                this.trace.push(`PATH-NAME:${text(cString(source, 'path-physical'))}`);
                this.mutate('path');
                return this.pathIndex < 0 ? null : this.paths[this.pathIndex];
            },
            loadPalette: (loadPath, clear) => {
                this.calls.load += 1;
                assert.equal(clear, 1);
                this.trace.push(`LOAD:${this.address(loadPath)}:${clear}`);
                this.trace.push(`LOAD-PATH:${text(cString(loadPath, 'load-physical'))}`);
                this.mutate('load');
            },
            buildMaps: () => { this.calls.maps += 1; this.trace.push(`MAPS:${this.renderer}:${this.dirty}`); this.mutate('maps'); },
        };
    }

    // 네 선택 진입점만 실행하고 명시 중단/물리 예외를 정직하게 반환한다. (2026-11-10 WB04-E227)
    run() {
        const globals = this.globals();
        const hooks = this.hooks(); // 함수 도중 같은 실제 필드를 계속 빌린다.
        try {
            if (this.kind === 'transition') transitionFortPalette(() => this.argument, globals, hooks);
            else if (this.kind === 'cache') releaseFortPaletteCloudCache(globals, hooks);
            else if (this.kind === 'sort') setFortPaletteCloudName(this.argument, globals, hooks); // 최초 fixture 의 역사적 진입 식별자는 API 명칭 정정과 별개로 보존한다.
            else if (this.kind === 'dirty') markFortPaletteDirty(globals);
            else throw deviceError('palette fixture 진입점');
        } catch (error) {
            if (error instanceof PaletteStop) return error.cause;
            if (!(error instanceof RangeError) || error.message !== '팔레트 이름 출력 물리 범위') throw error;
            return this.kind === 'transition' && this.nameCase === 7 ? 'palette-write' : 'sort-write'; // 최초 물리 중단 레이블은 변경하지 않는다.
        }
        return '1';
    }

    // 독립 원문과 같은 11개 현재 필드의 최종/중단 시점을 표시한다. (2026-11-10 WB04-E227)
    values() {
        return [this.cache, this.count, this.renderer, this.dirty, this.address(this.argument), this.pathIndex, this.calls.free, this.calls.build, this.calls.step, this.calls.load, this.calls.maps].join(',');
    }

    // 이름 앞 BYTE·NUL 뒤 잔여·입력 두 개·경로 두 개의 모든 물리를 비교한다. (2026-11-10 WB04-E227)
    raw() { return [this.palette, this.cloud, this.inputs[0], this.inputs[1], this.paths[0], this.paths[1]].map(hex).join(':'); }

    // trace 를 원문과 같은 순서로 한 줄에 결합한다. (2026-11-10 WB04-E227)
    traceText() { return this.trace.join(';'); }
}

// 입력 문자열(쉼표 구분 9칸)로 장치를 만든다. (2026-11-10 WB04-E227)
const deviceFor = (input) => new PaletteDevice(input.split(','));

test('팔레트 전환: 원본 10.78 676관측의 반환·부분 효과·주소·전체 물리·하위 호출 순서가 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let observations = 0;
    let stops = 0;
    for (const fields of rows) {
        assert.equal(fields.length, 6);
        const device = deviceFor(fields[1]);
        const cause = device.run();
        const label = `행 ${observations} 입력 ${fields[1]}`;
        assert.equal(cause, fields[2], `${label} 반환/중단`);
        assert.equal(device.values(), fields[3], `${label} 필드`);
        assert.equal(device.raw(), fields[4], `${label} raw`);
        assert.equal(device.traceText(), fields[5], `${label} trace`);
        observations += 1;
        if (cause !== '1') stops += 1;
    }
    assert.equal(observations, 676);
    assert.equal(stops, 34);
});

test('팔레트 전환 계약: 같은 이름의 자연 반환은 하위 장치를 부르지 않고 도달한 필수 적재 장치의 누락은 이전 효과를 남긴 채 거부한다', () => {
    // 대소문자만 다른 같은 이름 입력이다. 로그·비교 외의 하위 장치가 없어도 자연 반환한다.
    const same = deviceFor('transition,1,11,3,1,42,22,0,none');
    const sameHooks = same.hooks();
    const bare = { log: sameHooks.log, compare: sameHooks.compare };
    transitionFortPalette(() => same.argument, same.globals(), bare);
    assert.ok(same.cache === PaletteDevice.token(11) && same.dirty === 42 && same.trace.length === 2);
    // 도달한 필수 장치(loadPalette) 누락은 성공으로 가장하지 않는다.
    const missing = deviceFor('transition,0,11,0,1,42,22,0,none');
    const missingHooks = { ...missing.hooks(), loadPalette: undefined };
    assert.throws(() => transitionFortPalette(() => missing.argument, missing.globals(), missingHooks), TypeError);
    assert.ok(missing.cache === PaletteDevice.token(22) && missing.dirty === 42 && missing.calls.free === 1 && missing.calls.build === 1 && missing.calls.maps === 0);
    assert.equal(hex(cString(missing.palette.subarray(1), '시험')), '676966636c6f7564');
});

test('팔레트 전환 계약: 이름 복사는 BYTE 를 그대로(고비트 포함) NUL 까지 옮기고 입력 범위가 출력 범위보다 먼저이며 출력 범위 오류 전 쓰기를 남긴다', () => {
    // 비 ASCII BYTE(0xe9·0x80·0xff)도 변형 없이 두 이름 버퍼에 복사된다.
    const device = deviceFor('transition,0,0,0,0,0,22,0,none');
    device.inputs[0].set([0xe9, 0x80, 0xff, 0x41, 0x00]);
    device.argument = device.inputs[0];
    assert.equal(device.run(), '1');
    assert.equal(hex(device.palette.subarray(1, 6)), 'e980ff4100');
    assert.equal(hex(device.cloud.subarray(1, 6)), 'e980ff4100');
    // 네 칸 출력 버퍼에 NUL 이 없는 네 칸 입력은 입력 범위 오류가 먼저이고 앞의 네 BYTE 는 이미 쓰였다.
    const names = { paletteName: new Uint8Array(4), cloudName: new Uint8Array(4), cache: { value: 0 }, count: () => 0, renderer: () => 0, dirty: { value: 0 } };
    const source = Uint8Array.from([65, 66, 67, 68]);
    assert.throws(() => setFortPaletteCloudName(source, names, {}), (error) => error instanceof RangeError && error.message === '팔레트 이름 입력 물리 범위');
    assert.equal(hex(names.cloudName), '41424344');
    // 더 긴 입력은 출력 범위 오류이며 출력 버퍼가 가득 찰 때까지는 쓰였다.
    const longer = Uint8Array.from([1, 2, 3, 4, 5, 0]);
    const small = { ...names, cloudName: new Uint8Array(3) };
    assert.throws(() => setFortPaletteCloudName(longer, small, {}), (error) => error instanceof RangeError && error.message === '팔레트 이름 출력 물리 범위');
    assert.equal(hex(small.cloudName), '010203');
});

