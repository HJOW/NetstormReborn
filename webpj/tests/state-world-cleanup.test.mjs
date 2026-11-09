// State 세계 정리 부모 raw-state-world-cleanup.js(004b2df0)와 두 작은 정리 몸체(004328a0 경고 표·004ab940 spot 지도)를 원본 10.78 관측과 cpppj `StateWorldCleanupTests`/`StateFlatReset` 계약으로 대조한다.
//  - 원본 관측 state-world-cleanup-x86.tsv 294행(정상 194·명시 중단 100, 두 FPCW 관측): 입력 15칸(seed·reason·모드·Template header 존재 마스크·정지 깊이·전역 값·하위 경계에서 현재 값을 바꾸는 변이)에서
//    하위 호출 순서·인자, 직접 쓴 전역(61곳의 DWORD/QWORD/10 BYTE 캐시)의 전체 결과, 세 Player 물리 전체, 현재 Core 포인터 정체성, TYPE+c4 float, 9×20 BYTE Template header 가 한 바이트도 다르지 않다.
//    Core 선택(0048f3b0)은 E3 `raw-player-core-data.js` 의 `selectPlayerCoreData` 를 같은 전역/캐시/Player 뷰에 연결한다.
//  - 작은 정리 몸체 state-flat-reset-x86.tsv 16행(두 x87 상태 × 경고/spot × 네 패턴): 전체 Adler-32 와 앞뒤 인접 16 BYTE 로 정확한 쓰기 범위를 확인한다.
//  - 계약: 선택된 Core 정리 장치가 없으면 앞선 효과만 남고 그 뒤의 가짜 정상 진행을 하지 않는다. 짧은 물리 거부 후 입력 불변.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E228: State 세계 정리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { adler32, floatToBits } from './raw-support.mjs';
import { selectPlayerCoreData } from '../www/core/raw-player-core-data.js';
import { StateWorldCleanupEffect, cleanupStateWorld, resetStatePlayerNoMakeWarnings, resetStateSpotMap } from '../www/core/raw-state-world-cleanup.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다. (2026-11-10 WB04-E228)
const worldFile = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'state-world-cleanup-x86.tsv');
const flatFile = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'state-flat-reset-x86.tsv');
const worldLines = fs.existsSync(worldFile) ? fs.readFileSync(worldFile, 'utf8').split(/\r?\n/) : null;
const flatLines = fs.existsSync(flatFile) ? fs.readFileSync(flatFile, 'utf8').split(/\r?\n/) : null;

// 격리 PE 의 세 실제 Player 물리 식별자이다. (2026-11-10 WB04-E228)
const PLAYER_TOKENS = [0x12200000, 0x12202000, 0x12204000];

// 소문자 hex 로 표시한다(NUL 절단 없이 전체 BYTE). (2026-11-10 WB04-E228)
const hex = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');
// 두 뷰가 같은 원문 포인터(같은 buffer 의 같은 byteOffset)인지 본다. (2026-11-10 WB04-E228)
const same = (first, second) => first !== null && second !== null && first.buffer === second.buffer && first.byteOffset === second.byteOffset;

// 원문 하위에서 명시 중단한 입력만 표시한다. (2026-11-10 WB04-E228)
class CleanupStop {}

// 실제 현재 저장소와 명시 하위 장치만 구성하고 부모 분기/기대를 계산하지 않는다. (2026-11-10 WB04-E228)
class CleanupDevice {
    constructor(input, fields) {
        assert.equal(input.length, 15);
        this.fields = fields; // TSV 메타데이터의 관측 물리 폭이며 기대 효과가 아니다.
        this.words = new Map(); // 실제 DWORD 저장소이며 Core 자식도 같은 참조를 직접 쓴다.
        this.wides = new Map(); // 원문 double 필드의 그대로인 64비트 입력이다.
        this.playerBytes = [0, 1, 2].map(() => new Uint8Array(1620)); // 바뀐 base 와 예전 current 대상을 구별하는 세 물리이다.
        this.headers = new Uint8Array(180); // 실제 9×20 BYTE Template 표이다.
        this.cached = new Uint8Array(10); // 실제 lastKnown 캐시이다.
        this.external = new Uint8Array(10); // 외부 Core 입력이며 사본 동기화하지 않는다.
        this.typeDefault = floatBitsToNumber(0x3f123456); // Native TYPE+c4 의 실제 단일 float 저장소이다.
        this.calls = new Map();
        this.trace = [];
        const seed = Number(input[0]);
        this.reason = Number(input[1]);
        this.mutation = input[14];
        this.pause = Number(input[11]);
        for (const [address, width] of fields) { // 기록의 폭만 참조해 같은 초기 물리를 구성한다.
            if (width === 4) this.words.set(address, Number(CleanupDevice.inputBits(address, 0, 4, seed)));
            else if (width === 8) this.wides.set(address, CleanupDevice.inputBits(address, 0, 8, seed));
            else if (width === 10) { assert.equal(address, 0x5953f0); for (let byte = 0; byte < 10; byte += 1) this.cached[byte] = (seed + address + byte * 13) & 0xff; }
            else if (width === 88) { for (let offset = 0; offset < 88; offset += 4) this.words.set(address + offset, Number(CleanupDevice.inputBits(address, offset, 4, seed))); }
            else throw new Error('State 원문 미지원 관측 폭');
        }
        for (let index = 0; index < 3; index += 1) for (let byte = 0; byte < 1620; byte += 1) this.playerBytes[index][byte] = (seed + index * 47 + byte * 17) & 0xff; // 모든 Player BYTE 를 정의한다.
        for (let byte = 0; byte < 10; byte += 1) this.external[byte] = (seed + byte * 7 + 31) & 0xff;
        this.players = this.playerBytes[0];
        this.current = this.players.subarray(180 + 108, 180 + 118);
        const modes = Number(input[3]); // 각 모드의 독립 입력 비트이다.
        this.write(0x5c85c0, Number(input[2]));
        this.write(0x594fb8, modes & 1);
        this.write(0x594fbc, (modes >> 1) & 1);
        this.write(0x594fc8, (modes >> 2) & 1);
        // 실제 DWORD 와 원시 입력 칸의 대응이다(음수 defaultSp 도 원문 DWORD 비트 그대로 쓴다).
        const numbers = [[0x5c85a4, 4], [0x594fa4, 5], [0x5c8758, 6], [0x595344, 7], [0x540c70, 8], [0x540cac, 9], [0x594fa0, 13], [0x540bdc, 12]];
        for (const [address, index] of numbers) this.write(address, Number(BigInt.asUintN(32, BigInt(input[index]))));
        this.write(0x541240, 152);
        this.write(0x5949d4, seed ? 0xffffffff : 7);
        for (let byte = 0; byte < this.headers.length; byte += 1) this.headers[byte] = (seed + byte * 3) & 0xff; // 전체 header 인접 필드를 보존한다.
        const mask = Number(input[10]); // 현재 각 header 의 존재 입력이다.
        for (let owner = 0; owner < 9; owner += 1) {
            const value = (mask & (1 << owner)) ? 0x11110000 + owner : 0;
            for (let byte = 0; byte < 4; byte += 1) this.headers[owner * 20 + byte] = (value >>> (byte * 8)) & 0xff; // 첫 DWORD 만 실제 manager 로 설정한다.
        }
    }

    // little endian 원시 입력의 DWORD/QWORD 를 구성한다(부모 출력 기대는 계산하지 않는다). (2026-11-10 WB04-E228)
    static inputBits(address, offset, width, seed) {
        let value = 0n;
        for (let byte = 0; byte < width; byte += 1) value |= BigInt((seed + address + (offset + byte) * 13) & 0xff) << BigInt(byte * 8);
        return value;
    }

    // 실제 Player 뷰의 주소를 격리 PE 의 opaque 주소 식별자로 정규화한다. (2026-11-10 WB04-E228)
    playerToken() {
        for (let index = 0; index < 3; index += 1) if (same(this.players, this.playerBytes[index])) return PLAYER_TOKENS[index];
        throw new Error('State 검사 현재 Player 뷰 미등록');
    }

    // 주소 역할의 current 는 별도 DWORD 사본 없이 같은 뷰의 주소로 관측한다. (2026-11-10 WB04-E228)
    pointer() {
        if (this.current === null) return '0';
        if (same(this.current, this.cached)) return 'CACHED';
        if (same(this.current, this.external)) return 'EXTERNAL';
        for (let index = 0; index < 3; index += 1) for (let owner = 0; owner < 9; owner += 1) {
            if (this.current.buffer === this.playerBytes[index].buffer && this.current.byteOffset === owner * 180 + 108) return `P${index}:${owner}`; // 내용으로 현재 포인터를 추정하지 않는다.
        }
        throw new Error('State 검사 현재 Core 뷰 미등록');
    }

    // 같은 현재 DWORD 를 조회한다. Player base 는 실제 뷰 슬롯을 opaque 식별자로 표현하는 장치 경계이다. (2026-11-10 WB04-E228)
    read(address) {
        if (address === 0x59534c) return this.playerToken();
        if (!this.words.has(address)) throw new Error(`State 검사 미등록 전역 ${address.toString(16)}`);
        return this.words.get(address);
    }

    // 같은 실제 필드/현재 Player 뷰 슬롯에만 쓰며 범위 밖 새 전역을 만들지 않는다. (2026-11-10 WB04-E228)
    write(address, value) {
        if (address === 0x59534c) {
            for (let index = 0; index < 3; index += 1) if (value === PLAYER_TOKENS[index]) { this.players = this.playerBytes[index]; return; }
            throw new Error('State 검사 Player 토큰 미등록');
        }
        if (!this.words.has(address)) throw new Error(`State 검사 미등록 전역 ${address.toString(16)}`);
        this.words.set(address, value >>> 0);
    }

    // 각 하위의 진입 인자를 기록하고 같은 주소별 호출 번호를 반환한다. (2026-11-10 WB04-E228)
    event(address, args = []) {
        const key = address.toString(16);
        this.trace.push(`C:${key}:${args.join(',')}`);
        const count = (this.calls.get(key) ?? 0) + 1;
        this.calls.set(key, count);
        return `${key}.${count}`;
    }

    // 명시 하위에서만 현재 값을 바꾸며 부모 전역 접근에 숨은 callback 을 추가하지 않는다. (2026-11-10 WB04-E228)
    mutate(boundary) {
        const separator = this.mutation.indexOf('-');
        if (separator === -1 || this.mutation.slice(0, separator) !== boundary) return; // 선택한 한 경계만 바꾼다.
        const action = this.mutation.slice(separator + 1); // 외부 장치의 실제 효과이다.
        if (action === 'stop') throw new CleanupStop();
        if (action === 'client1') this.write(0x5c85c0, 1);
        else if (action === 'template2') this.write(0x540cac, 2);
        else if (action === 'battle1') this.write(0x594fbc, 1);
        else if (action === 'editor1') this.write(0x5c85a4, 1);
        else if (action === 'owner8') this.write(0x540c70, 8);
        else if (action === 'playerB') this.players = this.playerBytes[1];
        else if (action === 'known0') this.write(0x595344, 0);
        else if (action === 'known1') this.write(0x595344, 1);
        else if (action === 'knownEditor') { this.write(0x595344, 1); this.write(0x5c85a4, 1); }
        else if (action === 'defaultNegative') this.write(0x540bdc, 0x80000000);
        else if (action === 'state29') this.write(0x5c8758, 29);
        else if (action === 'coreExternal') this.current = this.external;
        else if (action === 'counterMax') this.write(0x5949d4, 0xffffffff);
        else throw new Error('State 원문 미해석 변경');
    }

    // 실제 저장소에 접근하는 순수 provider 와 선택된 현재 TYPE 참조를 공급한다. (2026-11-10 WB04-E228)
    globals() {
        const device = this;
        return {
            read32: (address) => device.read(address),
            write32: (address, value) => device.write(address, value),
            write16: (address, value) => device.write(address, ((device.read(address) & 0xffff0000) | value) >>> 0),
            write64Bits: (address, bits) => { if (!device.wides.has(address)) throw new Error('State 검사 미등록 QWORD'); device.wides.set(address, bits); },
            players: () => device.players,
            templateManager: (slot) => {
                if (slot >= 9) throw new RangeError('State 검사 header 물리');
                return (device.headers[slot * 20] | (device.headers[slot * 20 + 1] << 8) | (device.headers[slot * 20 + 2] << 16) | (device.headers[slot * 20 + 3] << 24)) >>> 0;
            },
            typeDefault: (type) => {
                const boundary = device.event(0x49a840, [type]);
                device.mutate(boundary);
                return { get value() { return device.typeDefault; }, set value(next) { device.typeDefault = Math.fround(next); } };
            },
        };
    }

    // 모든 미복원 자식은 명시 장치다. Core 선택만 E3 의 실제 JS 자식으로 합성한다. (2026-11-10 WB04-E228)
    hooks() {
        const device = this;
        return {
            effect: (effect) => {
                if (effect === StateWorldCleanupEffect.ResumeGame) device.pause -= 1;
                if (effect === StateWorldCleanupEffect.InitializePlayers) { device.trace.push(`BEFOREPLAYER:${device.pointer()}`); device.players = device.playerBytes[2]; } // 명시 하위 장치의 현재 base 교체이며 실제 할당기 복원 주장이 아니다.
                const boundary = effect === StateWorldCleanupEffect.Reset568b60 ? device.event(effect, [0x568b60])
                    : effect === StateWorldCleanupEffect.Reset54c8a0 ? device.event(effect, [0x54c8a0])
                        : effect === StateWorldCleanupEffect.ResetZone ? device.event(effect, [0x5c86c0]) : device.event(effect);
                device.mutate(boundary);
            },
            disconnect: (owner) => { device.mutate(device.event(0x4e2940, [owner])); },
            assertion: (line) => { device.mutate(device.event(0x4e0620, [line])); },
            pauseDepth: () => { const value = device.pause; device.mutate(device.event(0x460de0, [value])); return value; },
            saveFort: (slot) => { device.mutate(device.event(0x4bf9a0, [slot])); },
            saveBattle: (slot) => { device.mutate(device.event(0x4bf880, [slot])); },
            resetAi: (owner) => { device.mutate(device.event(0x413ae0, [owner])); },
            resetPriest: (owner) => { device.mutate(device.event(0x492240, [owner])); },
            resetOtherPlayer: (owner, value) => { device.mutate(device.event(0x447810, [owner, value])); },
            setSp: (owner, value) => { device.mutate(device.event(0x40edb0, [owner, floatToBits(value)])); },
            selectCore: (owner, selected) => {
                device.mutate(device.event(0x48f3b0, [owner, selected]));
                // 같은 실제 전역/캐시/Player 뷰를 동결 자식이 직접 쓴다.
                const coreGlobals = {
                    get templateOwner() { return device.words.get(0x540cac); }, set templateOwner(value) { device.words.set(0x540cac, value >>> 0); },
                    get battleOwner() { return device.words.get(0x540c70); }, set battleOwner(value) { device.words.set(0x540c70, value >>> 0); },
                    get playerKnown() { return device.words.get(0x595344); }, set playerKnown(value) { device.words.set(0x595344, value >>> 0); },
                    get active() { return device.words.get(0x595348); }, set active(value) { device.words.set(0x595348, value >>> 0); },
                    get metamap() { return device.words.get(0x594fc8); },
                    get current() { return device.current; }, set current(value) { device.current = value; },
                    cached: device.cached,
                    get players() { return device.players; },
                };
                selectPlayerCoreData(coreGlobals, owner, selected);
            },
            resetPool: (size) => { device.mutate(device.event(0x4af170, [size])); },
            wallSeconds: () => { device.mutate(device.event(0x460d70)); return 123.5; },
        };
    }

    // 원문이 기록한 각 범위의 전체 실제 BYTE 를 little endian 으로 출력한다. (2026-11-10 WB04-E228)
    rawGlobals() {
        let text = '';
        for (const [address, width] of this.fields) {
            if (width === 10) { text += hex(this.cached); continue; }
            for (let offset = 0; offset < width;) { // 연속 22 DWORD 도 같은 개별 현재 필드를 읽는다.
                const count = width === 8 ? 8 : 4;
                const value = width === 8 ? this.wides.get(address) : BigInt(this.read(address + offset));
                const bytes = [];
                for (let byte = 0; byte < count; byte += 1) bytes.push(Number((value >> BigInt(byte * 8)) & 0xffn));
                text += hex(bytes);
                offset += count;
            }
        }
        return text;
    }

    // 실제 하위 진입 순서를 그대로 원시 표 형식으로 반환한다. (2026-11-10 WB04-E228)
    traceText() { return this.trace.join(';'); }
}

// float 비트를 수로 바꾼다. (2026-11-10 WB04-E228)
function floatBitsToNumber(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}
// float32 를 4 BYTE 로 직렬화한다. (2026-11-10 WB04-E228)
function floatBytes(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return new Uint8Array(view.buffer);
}

// 고정 fixture 의 관측 범위 선언(# fields=주소.폭,...)만 해석한다. 원문 효과 기대는 계산하지 않는다. (2026-11-10 WB04-E228)
function cleanupFields(line) {
    assert.ok(line.startsWith('# fields='));
    return line.slice(9).split(',').map((field) => {
        const [address, width] = field.split('.');
        return [parseInt(address, 16), Number(width)];
    });
}

test('State 세계 정리: 원본 10.78 294행(정상 194·중단 100)의 직접 전역·Player·TYPE·Core 포인터·header·하위 호출 순서가 모두 같다', { skip: !worldLines && '원본 fixture 없음' }, () => {
    const fields = cleanupFields(worldLines[0]);
    let rows = 0;
    const statuses = { normal: 0, stop: 0 };
    for (const line of worldLines.slice(1)) {
        if (line.length === 0 || line[0] === '#') continue;
        const values = line.split('\t');
        assert.equal(values.length, 13);
        assert.ok(values[0] === '639' || values[0] === '895');
        const device = new CleanupDevice(values[1].split(','), fields);
        let status = 'normal';
        try { cleanupStateWorld(device.reason, device.globals(), device.hooks()); } catch (error) { if (!(error instanceof CleanupStop)) throw error; status = 'stop'; }
        rows += 1;
        statuses[status] += 1;
        const label = `행 ${rows} 입력 ${values[1]}`;
        assert.equal(status, values[2], `${label} status`);
        assert.equal(String(device.pause), values[3], `${label} pause`);
        assert.equal(device.pointer(), values[4], `${label} current`);
        assert.equal(device.rawGlobals(), values[5], `${label} globals`);
        assert.equal(hex(device.playerBytes[0]), values[6], `${label} playerA`);
        assert.equal(hex(device.playerBytes[1]), values[7], `${label} playerB`);
        assert.equal(hex(device.playerBytes[2]), values[8], `${label} playerC`);
        assert.equal(hex(floatBytes(device.typeDefault)), values[9], `${label} type+c4`);
        assert.equal(hex(device.headers), values[10], `${label} headers`);
        assert.equal(device.traceText(), values[11], `${label} trace`);
    }
    assert.equal(rows, 294);
    assert.deepEqual(statuses, { normal: 194, stop: 100 });
});

test('State 세계 정리 계약: 선택된 Core 정리/Player 재생성 경계가 없으면 앞선 효과만 남고 그 뒤의 가짜 정상 진행을 하지 않는다', { skip: !worldLines && '원본 fixture 없음' }, () => {
    const device = new CleanupDevice('165,1,0,0,0,0,14,1,1,1,6,2,10000,1,none'.split(','), cleanupFields(worldLines[0]));
    const hooks = { ...device.hooks(), selectCore: undefined }; // 실제 도달 경계만 누락한다.
    assert.throws(() => cleanupStateWorld(device.reason, device.globals(), hooks), TypeError);
    assert.equal(device.read(0x595344), 0);
    assert.equal(device.read(0x5c89b4) & 0xffff, 0);
    assert.equal(device.pointer(), 'P0:1');
    assert.ok((device.calls.get('4910a0') ?? 0) === 0 && (device.calls.get('4af170') ?? 0) === 0);
    assert.ok(device.traceText().endsWith('C:43b540:'));
});

test('State 세계 정리 계약: Fort 저장 장치가 Template 번호를 바꾸면 전투 저장은 바뀐 현재 번호를 받는다(Fort 저장 복귀 때만 다시 읽음)', { skip: !worldLines && '원본 fixture 없음' }, () => {
    const fields = cleanupFields(worldLines[0]);
    // Fort(모드 비트 1)와 전투(비트 2)가 함께 켜지고 State 14·Template header 1 존재 입력에서, Fort 저장 장치가 Template 을 2 로 바꾼다.
    const changed = new CleanupDevice('165,1,0,3,0,0,14,1,1,1,6,2,10000,1,4bf9a0.1-template2'.split(','), fields);
    cleanupStateWorld(changed.reason, changed.globals(), changed.hooks());
    assert.ok(changed.trace.includes('C:4bf9a0:1') && changed.trace.includes('C:4bf880:2') && !changed.trace.includes('C:4bf880:1'));
    // 변경이 없으면 같은 번호 1 을 그대로 받는다.
    const plain = new CleanupDevice('165,1,0,3,0,0,14,1,1,1,6,2,10000,1,none'.split(','), fields);
    cleanupStateWorld(plain.reason, plain.globals(), plain.hooks());
    assert.ok(plain.trace.includes('C:4bf9a0:1') && plain.trace.includes('C:4bf880:1'));
});

test('State 평탄 정리: 원본 10.78 16행의 전체 Adler-32 와 앞뒤 인접 16 BYTE 로 경고 표(255 DWORD)·spot 지도(65536 BYTE)의 쓰기 범위가 정확하다', { skip: !flatLines && '원본 fixture 없음' }, () => {
    let rows = 0;
    for (const line of flatLines) {
        if (line.length === 0 || line[0] === '#') continue;
        const row = line.split('\t');
        assert.equal(row.length, 9);
        const length = Number(row[3]);
        const seed = Number(row[1]);
        const buffer = new ArrayBuffer(Math.floor((length + 32) / 4) * 4); // DWORD 객체를 먼저 만들고 그 표현 바이트만 준비한다.
        const bytes = new Uint8Array(buffer);
        for (let index = 0; index < bytes.length; index += 1) bytes[index] = (seed + index * 17) & 0xff; // Native 의 명시 초기 패턴이며 기대 출력은 fixture 에서 읽는다.
        assert.equal(adler32(bytes), Number(row[4]));
        if (row[0] === 'warnings') resetStatePlayerNoMakeWarnings(new Uint32Array(buffer, 16, (buffer.byteLength - 16) / 4));
        else { assert.equal(row[0], 'spots'); resetStateSpotMap(bytes.subarray(16)); }
        assert.equal(adler32(bytes), Number(row[5]));
        assert.equal(hex(bytes.subarray(0, 16)) + hex(bytes.subarray(bytes.length - 16)), row[6]);
        assert.equal(Number(row[7]), length / 4);
        rows += 1;
    }
    assert.equal(rows, 16);
});

test('State 평탄 정리 계약: 지원한 호스트 물리 크기 밖에서는 기존 버퍼를 보존하고 정확한 범위만 0 으로 쓴다', () => {
    const warnings = new Uint32Array(256).fill(0xabcdef01);
    const before = Uint32Array.from(warnings);
    assert.throws(() => resetStatePlayerNoMakeWarnings(warnings.subarray(0, 254)), RangeError);
    assert.deepEqual(warnings, before);
    const spots = new Uint8Array(65537).fill(0xb3);
    const saved = Uint8Array.from(spots);
    assert.throws(() => resetStateSpotMap(spots.subarray(0, 65535)), RangeError);
    assert.deepEqual(spots, saved);
    resetStatePlayerNoMakeWarnings(warnings);
    assert.ok(warnings.subarray(0, 255).every((value) => value === 0) && warnings[255] === before[255]);
    resetStateSpotMap(spots);
    assert.ok(spots.subarray(0, 65536).every((value) => value === 0) && spots[65536] === 0xb3);
});
