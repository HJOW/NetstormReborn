// 색표 캐시 부모 raw-color-map-cache.js(prepareColorMapCache·initializeColorMapCache·regenerateColorMapCache·closeColorMapCacheFile)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - colorsort-cache-parent-x86.tsv 의 130행(진입점 prefix·parent·catch·cleanupR/cleanupW × 입력 프로필/초기값/읽기 길이/쓰기 제한/하위 변경): 완료 방식(return·prefix·stop·other·unable)·독립 정리 횟수·
//    typed dispatch 횟수·현재 소스 선택·소유자 DWORD 32바이트·두 소유자 표 4096바이트·아홉 표 2304바이트·두 파일 객체 48바이트·쓴 바이트·장치 호출 순서가 모두 같다.
//    장치(CacheDevice)는 cpppj ColorMapCacheTests 의 명시 입력/효과 장치를 그대로 옮긴 것으로 표 계산이나 부모 출력 기대를 만들지 않는다.
//  - 계약: 선택되지 않은 재생성 장치는 요구하지 않고(hit) 정확한 marker 에서만 누락이 드러남, 미지원 raw 소스 물리는 첫 solid/identity 쓰기 뒤 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E375: 5-A RawColorMapCache 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ColorCacheUnableToOpen, closeColorMapCacheFile, initializeColorMapCache, prepareColorMapCache, regenerateColorMapCache } from '../www/core/raw-color-map-cache.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E375)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'colorsort-cache-parent-x86.tsv'));

// 부모의 장치 경계에서만 발생시키는 명시 중단과, 파일 열기 형과 다른 오류(fallback catch-all 검출용)이다. (2026-11-13 WB04-E375)
class CacheStop extends Error {}
class CacheOther extends Error {}

// 바이트 배열의 소문자 16진 문자열이다(NUL 절단 없이 전체). (2026-11-13 WB04-E375)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 최초 Native 와 같은 입력/장치 효과만 제공한다. 표 분기나 부모 출력 기대는 만들지 않는다. (2026-11-13 WB04-E375)
class CacheDevice {
    constructor(fields) {
        assert.equal(fields.length, 6);
        this.entry = fields[0];
        this.mutation = fields[5];
        this.pending = '';
        const profile = Number(fields[1]);
        const initial = Number(fields[2]);
        this.writeLimit = Number(fields[4]);
        this.sources = [new Uint8Array(0x3800), new Uint8Array(0x3800)];
        this.current = 0;
        this.owners = [84, 249, 184, 140, 150, 112, 156, 222];
        this.highlight = new Uint8Array(2048);
        this.solid = new Uint8Array(2048);
        this.maps = Array.from({ length: 9 }, () => new Uint8Array(256));
        this.reader = new Uint8Array(24);
        this.writer = new Uint8Array(24);
        this.path = new Uint8Array(256);
        this.output = [];
        this.position = 0;
        this.cleanups = 0;
        this.dispatches = 0;
        this.calls = {};
        this.trace = [];
        this.directCleanup = false;
        CacheDevice.source(this.sources[0], profile, 0);
        CacheDevice.source(this.sources[1], profile, 1);
        this.name('!color.dat');
        this.reader.fill(initial ? 0xa5 : 0x3c);
        this.writer.set(this.reader);
        // 출력 잔여는 원시 초기 입력이다.
        for (let byte = 0; byte < 2048; byte += 1) {
            this.highlight[byte] = (initial * 31 + byte * 11) & 0xff;
            this.solid[byte] = (initial * 31 + 7 + byte * 11) & 0xff;
        }
        // 짧은 read 가 덮지 않는 BYTE 이다.
        for (let index = 0; index < 9; index += 1) for (let byte = 0; byte < 256; byte += 1) this.maps[index][byte] = (initial * 13 + index * 19 + byte * 5) & 0xff;
        this.input = new Uint8Array(Number(fields[3]));
        for (let byte = 0; byte < this.input.length; byte += 1) this.input[byte] = (19 + byte * 7) & 0xff;
    }

    // 실제 double 비트와 나머지 56BYTE 를 명시하며 원문 생성자의 출력을 계산하지 않는다. (매 기록 +8 만 명도 입력으로 덮는다)
    static source(bytes, profile, alternate) {
        for (let byte = 0; byte < bytes.length; byte += 1) bytes[byte] = (0x37 + byte * 13 + alternate * 29) & 0xff;
        const values = [0.8, 0.800000011920929, 0.80000002, Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY, -1.0, 1.0];
        const view = new DataView(bytes.buffer);
        for (let index = 0; index < 256; index += 1) view.setFloat64(index * 56 + 8, profile ? values[(index + alternate) % values.length] : index / 255.0, true);
    }

    // 선택 path 의 같은 물리만 바꾸며 뒤 파일 호출이 현재 NUL 이름을 보게 한다.
    name(text) {
        this.path.fill(0);
        this.path.set(Buffer.from(text, 'latin1'));
    }

    // 하위 File 이 읽는 현재 C 문자열을 관측한다(NUL 이 없으면 거부).
    static nameHex(bytes) {
        const end = bytes.indexOf(0);
        if (end < 0) throw new RangeError('ColorCache 장치 경로 NUL 물리');
        return hex(bytes.subarray(0, end));
    }

    // 장치의 실제 호출 인자를 순서대로 기록하고 종류별 번호가 붙은 경계 이름을 돌려준다.
    event(kind, text) {
        this.trace.push(text);
        this.calls[kind] = (this.calls[kind] ?? 0) + 1;
        return `${kind}${this.calls[kind]}`;
    }

    // 선택된 단 하나의 하위 효과에서 현재 전역/경로를 바꾸거나 명시 예외를 던진다.
    mutate(boundary) {
        const dash = this.mutation.indexOf('-');
        if (dash < 0) return;
        const trigger = this.mutation.slice(0, dash);
        const action = this.mutation.slice(dash + 1);
        const kind = boundary.replace(/[0-9]+$/u, '');
        if (boundary !== trigger && kind !== trigger) return;
        if (action === 'sourceB') this.current = 1;
        else if (action === 'owner7') this.owners[0] = 0x12345607;
        else if (action === 'pathChanged') this.name('changed-color.dat');
        else if (action === 'stop') { this.pending = action; throw new CacheStop(); } else if (action === 'other') { this.pending = action; throw new CacheOther(); } else if (action === 'unable') { this.pending = action; throw new ColorCacheUnableToOpen(); } else throw new Error('ColorCache 미해석 변경');
    }

    // 같은 실제 배열/현재 source getter 만 빌린다.
    globals() {
        return { primary: () => this.sources[this.current], ownerColors: this.owners, ownerHighlight: this.highlight, ownerSolid: this.solid, maps: this.maps };
    }

    // 같은 source/파일/표 주소만 식별하며 내용으로 파일 역할을 추측하지 않는다.
    role(file) {
        if (file === this.reader) return 'R';
        if (file === this.writer) return 'W';
        throw new Error('ColorCache 미등록 File 물리');
    }

    // 24BYTE 객체의 명시 생성 효과이며 부모가 반환값을 사용했다고 가장하지 않는다.
    open(file, name, mode, flag) {
        const role = this.role(file);
        assert.ok(mode === (role === 'R' ? 0 : 1) && flag === 1);
        const boundary = this.event(`open${role}`, `O:${role}:${hex(file)}:${CacheDevice.nameHex(name)}`);
        // ctor 부분 출력도 예외 전에 실제 물리에 쓴다.
        for (let byte = 0; byte < 24; byte += 1) file[byte] = 0x10 + (role === 'W' ? 0x30 : 0) + byte;
        this.mutate(boundary);
    }

    // 표 주소로 어느 표인지 식별한다(없으면 거부).
    tableIndex(target, message) {
        const selected = this.maps.findIndex((map) => map === target);
        if (selected < 0) throw new Error(message);
        return selected;
    }

    // 필수 장치 경계만 만든다. 계산/TYPE 표/File 전체 구현 완료를 주장하지 않는다.
    hooks() {
        return {
            path: () => {
                this.mutate(this.event('path', 'P'));
                return this.path;
            },
            owners: () => { this.mutate(this.event('owners', 'T')); },
            buildMaps: () => {
                // 제품 catch 에서 도달한 정확 marker 만 별도 분류한다.
                if (this.pending === 'unable' && this.entry === 'parent') { this.dispatches += 1; this.pending = ''; }
                for (let index = 0; index < 9; index += 1) for (let byte = 0; byte < 256; byte += 1) this.maps[index][byte] = (0x80 + index * 7 + byte * 3) & 0xff;
                this.mutate(this.event('maps', 'M'));
            },
            openRead: (file, name, mode, flag) => this.open(file, name, mode, flag),
            openWrite: (file, name, mode, flag) => this.open(file, name, mode, flag),
            read: (file, target, limit) => {
                assert.ok(this.role(file) === 'R' && limit === 0xffffffff);
                const selected = this.tableIndex(target, 'ColorCache read 표 주소');
                const count = Math.min(256, this.input.length - this.position);
                target.set(this.input.subarray(this.position, this.position + count));
                this.position += count;
                this.mutate(this.event('read', `R:${selected}:${count}`));
                return count;
            },
            write: (file, target, limit) => {
                assert.ok(this.role(file) === 'W' && limit === 0xffffffff);
                const selected = this.tableIndex(target, 'ColorCache write 표 주소');
                const count = Math.min(256, this.writeLimit);
                this.output.push(...target.subarray(0, count));
                this.mutate(this.event('write', `W:${selected}:${count}`));
                return count;
            },
            close: (file) => {
                // 전파 중 실제 close 와 독립 funclet entry 만 센다.
                if (this.pending !== '' || this.directCleanup) { this.cleanups += 1; this.directCleanup = false; }
                const role = this.role(file);
                const boundary = this.event(`close${role}`, `D:${role}:${hex(file)}`);
                for (let byte = 0; byte < 24; byte += 1) file[byte] = 0xd0 + (role === 'W' ? 7 : 0) + byte;
                this.mutate(boundary);
            },
        };
    }

    // 실제 장치 호출 순서를 정렬/종류별 묶음 없이 반환한다.
    traceText() { return this.trace.length ? this.trace.join(';') : '-'; }
}

test('ColorMapCache: 원본 10.78 부모/catch/cleanup 130행의 완료·정리·표·파일 객체·쓴 바이트·장치 호출 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 두 정밀도 자료를 제품의 같은 명시 입력으로 대조한다.
    for (const expected of rows) {
        assert.equal(expected.length, 13);
        const device = new CacheDevice(expected[1].split(','));
        const globals = device.globals();
        const hooks = device.hooks();
        let completion = 'return';
        try {
            if (device.entry === 'prefix') { prepareColorMapCache(globals, hooks); completion = 'prefix'; } else if (device.entry === 'parent') initializeColorMapCache(globals, hooks, device.reader, device.writer);
            else if (device.entry === 'catch') regenerateColorMapCache(device.path, globals, hooks, device.writer);
            else { device.directCleanup = true; closeColorMapCacheFile(device.entry === 'cleanupR' ? device.reader : device.writer, hooks); }
        } catch (error) {
            if (error instanceof CacheStop) completion = 'stop';
            else if (error instanceof CacheOther) completion = 'other';
            else if (error instanceof ColorCacheUnableToOpen) completion = 'unable';
            else throw error;
        }
        count += 1;
        const where = `행 ${count} (${expected[1]})`;
        assert.equal(completion, expected[2], `${where} completion`);
        assert.equal(String(device.cleanups), expected[3], `${where} cleanup`);
        assert.equal(String(device.dispatches), expected[4], `${where} dispatch`);
        assert.equal(String(device.current), expected[5], `${where} current`);
        const ownerBytes = new Uint8Array(32);
        // 원문 DWORD endian 만 표기한다.
        for (let owner = 0; owner < 8; owner += 1) for (let byte = 0; byte < 4; byte += 1) ownerBytes[owner * 4 + byte] = (device.owners[owner] >>> (byte * 8)) & 0xff;
        assert.equal(hex(ownerBytes), expected[6], `${where} ownerDwords`);
        assert.equal(hex(device.highlight) + hex(device.solid), expected[7], `${where} ownerTables`);
        assert.equal(device.maps.map(hex).join(''), expected[8], `${where} maps`);
        assert.equal(hex(device.reader) + hex(device.writer), expected[9], `${where} files`);
        assert.equal(device.output.length ? hex(device.output) : '-', expected[10], `${where} written`);
        assert.equal(device.traceText(), expected[11], `${where} trace`);
        assert.ok(Number(expected[12]) > 0);
    }
    assert.equal(count, 130);
});

test('ColorMapCache: 캐시 hit 는 선택되지 않은 재생성 장치를 요구하지 않고 누락은 정확한 marker 에서만 드러난다', () => {
    const hit = new CacheDevice(['parent', '0', '0', '2304', '256', 'none']);
    const hitHooks = hit.hooks();
    delete hitHooks.buildMaps;
    delete hitHooks.openWrite;
    delete hitHooks.write;
    initializeColorMapCache(hit.globals(), hitHooks, hit.reader, hit.writer);
    assert.ok(!hit.calls.maps && hit.calls.closeR === 1);
    // 열 수 없음 marker 가 나오면 재생성 장치(buildMaps)가 필요하고 없으면 Error 이다. reader 는 닫지 않고 writer 는 열지 않는다.
    const missing = new CacheDevice(['parent', '0', '0', '2304', '256', 'openR-unable']);
    const devices = missing.hooks();
    delete devices.buildMaps;
    assert.throws(() => initializeColorMapCache(missing.globals(), devices, missing.reader, missing.writer), { name: 'Error', message: /실제 하위 장치 미연결/u });
    assert.ok(!missing.calls.closeR && !missing.calls.openW);
});

test('ColorMapCache: 미지원 raw 소스 물리는 첫 solid/identity 쓰기 뒤 거부하고 owners 등록은 하지 않는다', () => {
    const device = new CacheDevice(['prefix', '0', '1', '2304', '256', 'none']);
    const globals = device.globals();
    globals.primary = () => new Uint8Array(0);
    const oldSecond = device.highlight[1];
    assert.throws(() => prepareColorMapCache(globals, device.hooks()), RangeError);
    assert.ok(device.solid[0] === 84 && device.highlight[0] === 0 && device.highlight[1] === oldSecond && !device.calls.owners);
});

test('ColorMapCache: 명도 임계는 엄격 초과이고 NaN/무한/음수는 교체하지 않으며 소유자 색은 행마다 하위 BYTE 로 읽는다', () => {
    const device = new CacheDevice(['prefix', '1', '0', '2304', '256', 'none']);
    device.owners = [0x1234, 0xff01, 7, 8, 9, 10, 11, 0x80000000 + 12];
    prepareColorMapCache(device.globals(), device.hooks());
    // 프로필 1 의 명도 값: [0.8, 0.800000011920929, 0.80000002, NaN, +Inf, -Inf, -1, 1] 의 반복. 0.8(double)은 임계 0.800000011920929 보다 작아 교체하지 않고, 임계와 같은 값도 교체하지 않으며,
    // 0.80000002 와 +Inf 와 1 만 교체한다.
    for (let row = 0; row < 8; row += 1) {
        const selected = device.owners[row] & 0xff;
        for (let color = 0; color < 256; color += 1) {
            const replaced = [false, false, true, false, true, false, false, true][color % 8];
            assert.equal(device.solid[row * 256 + color], selected, `solid ${row},${color}`);
            assert.equal(device.highlight[row * 256 + color], replaced ? selected : color, `highlight ${row},${color}`);
        }
    }
    assert.equal(device.calls.owners, 1);
    assert.equal(device.calls.path, 1);
});

test('ColorMapCache: 마지막 기록이 4바이트 단위로 모자라는 소스 물리는 쓰기 뒤 부분 단어에서 거부한다', () => {
    const device = new CacheDevice(['prefix', '0', '1', '2304', '256', 'none']);
    const globals = device.globals();
    // 마지막 기록(255번)의 마지막 단어(+52..55)가 한 바이트 모자란다: 앞의 255색은 처리되고 255번에서 거부한다.
    globals.primary = () => device.sources[0].subarray(0, 0x3800 - 1);
    assert.throws(() => prepareColorMapCache(globals, device.hooks()), { name: 'RangeError', message: /물리범위/u });
    // 첫 행의 255번 칸까지 solid/identity 가 이미 쓰였고(나머지 칸과 owners 등록은 없음) 정확히 맞는 길이는 통과한다.
    assert.equal(device.solid[255], 84);
    assert.equal(device.highlight[255], 255);
    assert.ok(!device.calls.owners);
    const exact = new CacheDevice(['prefix', '0', '1', '2304', '256', 'none']);
    const exactGlobals = exact.globals();
    exactGlobals.primary = () => exact.sources[0].subarray(0, 0x3800);
    prepareColorMapCache(exactGlobals, exact.hooks());
    assert.equal(exact.calls.owners, 1);
});
