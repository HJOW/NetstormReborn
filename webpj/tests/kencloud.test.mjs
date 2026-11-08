// 구름 캐시 raw-kencloud.js(loadKencloudFile/stepKencloud/buildKencloud)를 원본 10.78 관측 246개와 cpppj 계약으로 대조한다.
//  - 원본 관측: kencloud-v2-x86.tsv — 진입점(build/step/file)·초기 캐시/이미지 토큰·치수·exists·할당·색·speed·fury·seed 수·phase·난수 상태·end·변화(mutation)·경로 사례의 입력으로
//    중단 원인(return 또는 물리/나눗셈/명시 중단), 12개 현재 필드(파일 반환·난수·색·speed·fury·count·간격·호출 횟수), 인접 전역 8KB·세 512×512 할당 전체·파일 scratch 의 SHA-256, 호출 순서(trace)가 같다.
//  - 계약: 캐시가 있으면 하위 훅을 요구하지 않고, 도달한 필수 훅 누락은 부분 효과 뒤 거부하며, 같은 GameRandom 참조를 소비한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E118: RawKencloud 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { KencloudDomainError, buildKencloud, loadKencloudFile, stepKencloud } from '../www/core/raw-kencloud.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E118)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'kencloud-v2-x86.tsv')) ?? [];

// 인접 전역(0x2018바이트)과 세 512×512 할당의 격리 원문 주소이다. (2026-11-08 WB04-E118)
const STATE = 0x5697f0;
const STATE_BYTES = 0x2018;
const BYTES = 0x40000;
const CACHE = 0x569ffc;
const END = 0x569ff0;
const TOP = 0x569ff8;
const BOTTOM = 0x569ff4;
const PHASE = 0x56b804;
const ALL = 0x56b7fc;
const PART = 0x56b800;
const DIRTY = 0x56a000;
const SEEDS = 0x56a800;
const BUFFERS = [0x12001000, 0x12051000, 0x120a1000];
const ROWS = [0, 8, 4, 12, 1, 11, 5, 15, 3, 10, 2, 7, 14, 9, 6, 13];

// 명시 외부 장치 중단의 원인을 그대로 보존하는 표식이다. (2026-11-08 WB04-E118)
class Stop {
    // 중단한 경계 이름이다.
    constructor(cause) { this.cause = cause; }
}

// SHA-256 16진 문자열이다. (2026-11-08 WB04-E118)
const sha = (bytes) => crypto.createHash('sha256').update(bytes).digest('hex');

// 십진 숫자 목록을 쉼표로 잇는다. (2026-11-08 WB04-E118)
const numbers = (values) => values.join(',');

// 제품의 부모 계산은 포함하지 않는 명시 파일/할당/관측 장치이다. (2026-11-08 WB04-E118)
class Device {
    // 현재 입력 행(16칸)으로 모든 현재 상태/물리를 새로 초기화한다.
    constructor(input) {
        assert.equal(input.length, 16);
        // 실제 인접 전역의 고정 물리와 독립 소유한 세 할당, 원문 PathSpec 이 돌려주는 NUL 물리이다.
        this.state = new Uint32Array(STATE_BYTES / 4);
        this.buffers = [0, 1, 2].map(() => new Uint8Array(BYTES));
        this.pathSource = new Uint8Array(1024);
        this.rows = ROWS.slice();
        this.spacer = 29;
        this.fileReturn = 0;
        this.calls = { file: 0, alloc: 0, free: 0, random: 0, move: 0 };
        this.events = [];
        this.scratch = null;
        this.fileSnapshot = '-';
        const num = (text) => Number(text) >>> 0;
        this.kind = input[0];
        this.image = Device.token(num(input[2]));
        this.width = num(input[3]) | 0;
        this.height = num(input[4]) | 0;
        this.exists = num(input[5]);
        this.allocate = num(input[6]);
        this.color = num(input[7]);
        this.speed = num(input[8]);
        this.fury = num(input[9]);
        this.count = num(input[10]);
        this.mutation = input[14];
        this.random = new GameRandom(0);
        this.random.setState(num(input[12]));
        this.state.fill(0xcdcdcdcd);
        const cache = Device.token(num(input[1]));
        const end = num(input[13]);
        this.word(CACHE, cache);
        this.word(END, cache + end);
        this.word(TOP, cache + 512);
        this.word(BOTTOM, cache + end - 512);
        this.word(PHASE, num(input[11]));
        this.word(ALL, 17);
        this.word(PART, 19);
        // 이전 dirty 표/현재 dirty 표/seed 표의 전체 미사용 DWORD 도 입력으로 보존한다.
        for (let index = 0; index < 512; index += 1) {
            this.word(STATE + index * 4, 0x80000000 + index);
            this.word(DIRTY + index * 4, 0x90000000 + index);
        }
        for (let index = 0; index < 1000; index += 1) this.word(SEEDS + index * 4, 0x70000000 + index);
        // 서로 다른 전체 할당 입력이다.
        this.buffers.forEach((buffer, b) => {
            for (let byte = 0; byte < buffer.length; byte += 1) buffer[byte] = (byte * 13 + b * 31) & 0xff;
        });
        this.pathSource.fill(0xde);
        if (input[15] === '2') {
            this.pathSource.fill(0x51, 0, 256);
            this.pathSource[256] = 0;
        } else Device.put(this.pathSource, input[15] === '0' ? 'Cloud/File.gif' : '');
    }

    // 테스트 입력의 0/11/22/33 을 native 주소로 바꾼다. (2026-11-08 WB04-E118)
    static token(logical) {
        if (logical === 0) return 0;
        return BUFFERS[Math.floor(logical / 11) - 1];
    }

    // 명시 하위가 C 문자열을 바꾸되 나머지 물리는 보존한다. (2026-11-08 WB04-E118)
    static put(target, text) {
        if (text.length + 1 > target.length) throw new RangeError('검사 이름 쓰기');
        for (let index = 0; index < text.length; index += 1) target[index] = text.charCodeAt(index);
        target[text.length] = 0;
    }

    // 이름의 현재 NUL 까지 읽어 명시 파일 장치에 전달한다. (2026-11-08 WB04-E118)
    static cstring(bytes) {
        const end = bytes.indexOf(0);
        if (end < 0) throw new RangeError('검사 CString 물리');
        return bytes.subarray(0, end);
    }

    // 실제 전역 주소의 같은 DWORD 를 읽거나 쓴다. (2026-11-08 WB04-E118)
    word(address, value) {
        const index = (address - STATE) / 4;
        if (value === undefined) return this.state[index];
        this.state[index] = value >>> 0;
        return value;
    }

    // 도달한 실제 할당 주소부터 남은 물리를 빌린다. one-past 는 길이 0 이동을 위해 유효하다. (2026-11-08 WB04-E118)
    memory(address) {
        for (let index = 0; index < BUFFERS.length; index += 1) {
            if (address >= BUFFERS[index] && address - BUFFERS[index] <= BYTES) return this.buffers[index].subarray(address - BUFFERS[index]);
        }
        throw new RangeError('검사 미등록 물리 주소');
    }

    // 같은 현재 전역/표의 참조만 제공한다. 필드 사본의 후동기화는 없다. (2026-11-08 WB04-E118)
    globals() {
        const device = this;
        const accessor = (address) => ({ get: () => device.word(address), set: (value) => device.word(address, value), enumerable: true });
        return Object.defineProperties({
            phaseRows: this.rows,
            dirtyRows: this.state.subarray((DIRTY - STATE) / 4, (DIRTY - STATE) / 4 + 512),
            seeds: this.state.subarray((SEEDS - STATE) / 4, (SEEDS - STATE) / 4 + 1000)
        }, {
            cache: accessor(CACHE),
            end: accessor(END),
            nextRow: accessor(TOP),
            lastRow: accessor(BOTTOM),
            phase: accessor(PHASE),
            allDirty: accessor(ALL),
            partialDirty: accessor(PART),
            spacer: { get: () => device.spacer, set: (value) => { device.spacer = value >>> 0; }, enumerable: true },
            lowColor: { get: () => device.color, enumerable: true },
            speed: { get: () => device.speed, enumerable: true },
            fury: { get: () => device.fury, enumerable: true },
            seedCount: { get: () => device.count, enumerable: true }
        });
    }

    // callback 복귀 직전 명시 변화만 적용한다. 난수도 같은 실제 객체를 직접 변경한다. (2026-11-08 WB04-E118)
    mutate(boundary) {
        const mutation = this.mutation;
        if (mutation === `${boundary}-stop`) throw new Stop(boundary);
        if (mutation === `${boundary}-cache33`) this.word(CACHE, Device.token(33));
        else if (mutation === `${boundary}-cache22`) this.word(CACHE, Device.token(22));
        else if (mutation === `${boundary}-cache0`) this.word(CACHE, 0);
        else if (mutation === `${boundary}-count0`) this.count = 0;
        else if (mutation === `${boundary}-count4`) this.count = 4;
        else if (mutation === `${boundary}-state17`) this.random.setState(17);
        else if (mutation === `${boundary}-fury0`) this.fury = 0;
        else if (mutation === `${boundary}-fury20000`) this.fury = 20000;
        else if (mutation === `${boundary}-color17`) this.color = 17;
        else if (mutation === `${boundary}-end17`) this.word(END, this.word(CACHE) + 17);
        else if (mutation === `${boundary}-source`) Device.put(this.pathSource, 'ChangedSource');
        else if (mutation === `${boundary}-name` && this.scratch) Device.put(this.scratch.path, 'ChangedName');
    }

    // 현재 파일 stack 객체(높이·너비·256바이트 이름 264바이트)의 바이트 표현이다. (2026-11-08 WB04-E118)
    scratchBytes() {
        const bytes = new Uint8Array(264);
        const view = new DataView(bytes.buffer);
        view.setInt32(0, this.scratch.height, true);
        view.setInt32(4, this.scratch.width, true);
        bytes.set(this.scratch.path, 8);
        return bytes;
    }

    // 명시 파일 장치만 연결하며 부모 복사/치수 검사/free 결정은 제품이 실행한다. (2026-11-08 WB04-E118)
    file() {
        this.calls.file += 1;
        this.events.push(`FILE:${this.calls.file}`);
        // 원문의 초기 stack 입력이다. 임의 0 초기화가 아니다.
        this.scratch = { height: 0xcdcdcdcd | 0, width: 0xcdcdcdcd | 0, path: new Uint8Array(256).fill(0xcd) };
        const hex = (bytes) => Buffer.from(bytes).toString('hex');
        const hooks = {
            pathSpec: (key, a, b, c) => {
                assert.ok(key === 'CloudFileSpec' && a === 0 && b === 0 && c === 0);
                this.events.push('PATH:CloudFileSpec:0,0,0');
                this.mutate('path');
                return this.pathSource;
            },
            exists: (name) => { this.events.push(`EXISTS:${hex(Device.cstring(name))}`); this.mutate('exists'); return this.exists; },
            decode: (name, output) => {
                this.events.push(`DECODE:${hex(Device.cstring(name))}`);
                output.width = this.width;
                output.height = this.height;
                this.mutate('decode');
                return this.image;
            },
            release: (selected) => { this.calls.free += 1; this.events.push(`FREE:${selected}`); this.mutate('free'); }
        };
        try {
            const result = loadKencloudFile(this.scratch, hooks);
            this.fileSnapshot = sha(this.scratchBytes());
            return result;
        } catch (error) {
            this.fileSnapshot = sha(this.scratchBytes());
            throw error;
        }
    }

    // 할당/주소 조회와 실제 memmove·난수 뒤 관측만 공급한다. (2026-11-08 WB04-E118)
    hooks() {
        return {
            loadFile: () => this.file(),
            allocate: (bytes) => {
                assert.equal(bytes, BYTES);
                this.calls.alloc += 1;
                const result = this.allocate === 0 ? 0 : BUFFERS[Math.min(this.calls.alloc - 1, 2)];
                this.events.push(`ALLOC:${result}`);
                this.mutate('alloc');
                return result;
            },
            memory: (address) => this.memory(address),
            afterMove: (target, source, bytes) => {
                this.calls.move += 1;
                this.events.push(`MOVE:${numbers([this.calls.move, target, source, bytes])}`);
                this.mutate(`move${this.calls.move}`);
            },
            afterRandom: (limit, before, result) => {
                this.calls.random += 1;
                this.events.push(`RANDOM:${numbers([this.calls.random, limit, before, result, this.random.state])}`);
                this.mutate(`random${this.calls.random}`);
            }
        };
    }

    // 제품의 세 선택 진입점과 중단을 구별한다. 정상이면 'return', 아니면 중단 원인이다. (2026-11-08 WB04-E118)
    run() {
        const globals = this.globals();
        const hooks = this.hooks();
        try {
            if (this.kind === 'build') buildKencloud(globals, this.random, hooks);
            else if (this.kind === 'step') stepKencloud(globals, hooks);
            else if (this.kind === 'file') this.fileReturn = this.file();
            else throw new Error('Kencloud 진입점');
        } catch (error) {
            if (error instanceof Stop) return error.cause;
            if (error instanceof KencloudDomainError) {
                assert.equal(error.message, 'Kencloud divide-zero');
                return 'divide-zero';
            }
            if (error instanceof RangeError && error.message.startsWith('Kencloud ')) return error.message.slice(9);
            throw error;
        }
        return 'return';
    }

    // 정상/부분 중단 뒤 현재 12개 필드를 fixture 와 동일 순서로 기록한다. (2026-11-08 WB04-E118)
    values() {
        return numbers([this.fileReturn, this.random.state, this.color, this.speed, this.fury, this.count, this.spacer,
            this.calls.file, this.calls.alloc, this.calls.free, this.calls.random, this.calls.move]);
    }

    // 인접 전역 전체/세 할당 전체/현재 파일 stack 을 SHA-256 으로 요약한다. (2026-11-08 WB04-E118)
    raw() {
        let result = sha(Buffer.from(this.state.buffer));
        for (const buffer of this.buffers) result += `:${sha(buffer)}`;
        return `${result}:${this.fileSnapshot}`;
    }

    // 실제 기록한 호출 순서만 연결한다. (2026-11-08 WB04-E118)
    trace() { return this.events.join(';'); }
}

test('Kencloud: 원본 10.78 v2 관측 246개의 난수·전체 할당·인접 전역·파일 scratch·호출 순서가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    // 표준 해시 구현의 독립 확인이다.
    assert.equal(sha(new Uint8Array(0)), 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855');
    assert.equal(sha(Buffer.from('abc')), 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');
    let compared = 0;
    let normal = 0;
    // 양 정밀도도 매번 동일 독립 입력으로 시작한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const device = new Device(row[1].split(','));
        const cause = device.run();
        assert.equal(cause, row[2], `Kencloud 행 ${compared} 원인 ${row[1]}`);
        assert.equal(device.values(), row[3], `Kencloud 행 ${compared} 현재 필드 ${row[1]}`);
        assert.equal(device.raw(), row[4], `Kencloud 행 ${compared} 물리 해시 ${row[1]}`);
        assert.equal(device.trace(), row[5], `Kencloud 행 ${compared} 호출 순서 ${row[1]}`);
        compared += 1;
        if (cause === 'return') normal += 1;
    }
    assert.equal(compared, 246);
    assert.equal(normal, 208);
});

test('Kencloud: 자연 생략은 미연결 장치를 요구하지 않고 도달한 필수 경계는 부분 효과 뒤 예외를 낸다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    // 현재 캐시 비영 입력은 하위를 자연 생략한다.
    const current = new Device('build,11,11,512,512,1,11,228,1,10000,3,0,305419896,262144,none,0'.split(','));
    const before = current.random.state;
    buildKencloud(current.globals(), current.random, {});
    assert.equal(current.random.state, before);
    assert.equal(current.word(CACHE), BUFFERS[0]);
    assert.ok(current.word(PART) === 0 && current.word(ALL) === 1);
    // 같은 GameRandom 참조만 제공한 미연결 파일 경계는 부분 효과 없이 거부한다.
    const missing = new Device('build,0,11,512,512,1,11,228,1,10000,3,0,305419896,262144,none,0'.split(','));
    assert.throws(() => buildKencloud(missing.globals(), missing.random, {}), TypeError);
    assert.ok(missing.random.state === 305419896 && missing.word(CACHE) === 0 && missing.word(ALL) === 17 && missing.word(PART) === 19);
    // 호출자의 현재 stack 입력은 명시한다: exists 가 0 이면 높이/너비/이름의 부분 쓰기만 남긴다.
    const scratch = { height: 17, width: 19, path: new Uint8Array(256).fill(0xcd) };
    const name = Uint8Array.of(0x41, 0);
    const file = { pathSpec: () => name, exists: () => 0 };
    assert.equal(loadKencloudFile(scratch, file), 0);
    assert.ok(scratch.width === 19 && scratch.height === 17 && scratch.path[0] === 0x41 && scratch.path[1] === 0 && scratch.path[2] === 0xcd);
});
