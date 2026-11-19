// 원본 x86 Fort 이름 관측 876행의 입력·중단·전체 물리와 외부 효과 순서를 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { setCurrentFortName, setFortManagerFileName } from '../www/core/raw-fort-current-name.js';
import { REPO_ROOT } from './helpers.mjs';

// 고정 원본 manager 주소를 관측 장치의 두 저장소에 대응한다.
const TOKENS = [0, 0x12001000, 0x12002000];
// NUL 앞 문자열만 읽고 미종료 배열은 명시적으로 거부한다.
function text(bytes) { const end = bytes.indexOf(0); if (end < 0) throw new RangeError('관측 장치 NUL 범위'); return Buffer.from(bytes.subarray(0, end)); }
// 전체 물리 바이트를 관측 표와 같은 소문자 문자열로 변환한다.
function hex(bytes) { return Buffer.from(bytes).toString('hex'); }
// NUL을 포함한 문자열만 덮고 뒤의 물리 바이트는 유지한다.
function input(target, value) {
    const bytes = Buffer.from(value, 'latin1'); if (bytes.length >= target.length) throw new RangeError('관측 입력 범위');
    target.set(bytes); target[bytes.length] = 0;
}
// 고정 헤더의 비정렬 DWORD를 읽고 쓴다.
function read(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset + offset, 4).getUint32(0, true); }
// 선택한 DWORD만 쓰고 헤더의 다른 바이트는 유지한다.
function put(bytes, offset, value) { new DataView(bytes.buffer, bytes.byteOffset + offset, 4).setUint32(0, value >>> 0, true); }
// ASCII 대문자만 소문자 바이트로 바꾼다.
function foldAscii(value) { return value >= 65 && value <= 90 ? value + 32 : value; }
// 원본 C 로케일 경로 비교처럼 ASCII 대문자만 소문자로 비교한다.
function sameAscii(first, second) {
    if (first.length !== second.length) return false;
    // 원본 경로를 수정하지 않고 각 바이트를 독립적으로 비교한다.
    for (let index = 0; index < first.length; index += 1) {
        if (foldAscii(first[index]) !== foldAscii(second[index])) return false;
    }
    return true;
}
// 관측 장치가 선택한 중단과 구현의 예외를 구별한다.
class ObservationStop extends Error { constructor(cause) { super(cause); this.cause = cause; } }

// 기대 출력이 아닌 입력 열과 고정 응답만 사용해 원본 장치의 상태를 구성한다.
class NameDevices {
    // 원문 콜백 입력과 물리 저장소의 초기 상태다.
    constructor(encoded) {
        const f = encoded.split(','); if (f.length !== 13) throw new Error('관측 입력 필드 수');
        this.mode = f[0]; this.owner = Number(f[1]); this.other = (this.owner + 1) % 9;
        const manager = Number(f[2]), readonly = Number(f[3]), duplicate = Number(f[4]), old = Number(f[5]);
        this.newKind = Number(f[6]); this.pathMode = Number(f[7]); this.fort = Number(f[8]); this.editor = Number(f[9]); this.mission = Number(f[10]); this.kind = Number(f[11]); this.mutation = f[12];
        this.current = 0; this.dirty = 0xfffffffe; this.lastHeader = 0; this.physical = ''; this.trace = [];
        this.specPointer = false; this.snapshots = 0; this.notifies = 0; this.paths = 0; this.deletes = 0; this.configs = 0;
        this.headers = Uint8Array.from({ length: 180 }, (_, i) => (0x70 + i) & 255);
        this.states = Array.from({ length: 9 }, () => Uint8Array.from({ length: 28 }, (_, i) => (0x80 + i) & 255));
        // 9개 고정 헤더와 State를 독립적인 원본 초기값으로 채운다.
        for (let slot = 0; slot < 9; slot += 1) { put(this.headers, slot * 20, 0); this.states[slot][17] = this.kind; }
        put(this.headers, this.other * 20, TOKENS[2]); put(this.headers, this.other * 20 + 4, 0); put(this.headers, this.other * 20 + 8, 0);
        put(this.headers, this.owner * 20, TOKENS[manager / 11]); put(this.headers, this.owner * 20 + 4, duplicate); put(this.headers, this.owner * 20 + 8, readonly);
        this.managers = Array.from({ length: 2 }, (_, i) => {
            const key = (i + 1) * 11, bytes = Uint8Array.from({ length: 276 }, (_, at) => (0x91 + at + key) & 255);
            input(bytes.subarray(12), `C:/prior${key}.fort`); return bytes;
        });
        this.names = [new Uint8Array(256).fill(0xa7), new Uint8Array(256).fill(0xa7)];
        input(this.names[0], ['Old', '', 'oLd'][old]); input(this.names[1], 'Other');
        this.argument = new Uint8Array(512).fill(0xb6);
        const names = ['New', 'Old', 'oLd', 'Bad.Name', '', 'M'.repeat(255), 'M'.repeat(256), 'New\0.after'];
        if (this.newKind === 7) this.argument.set(Buffer.from(names[7], 'latin1'));
        else if (this.newKind < 8) input(this.argument, names[this.newKind]);
        this.spec = new Uint8Array(9); input(this.spec, 'fortSpec'); this.scratch = new Uint8Array(256).fill(0xcd); input(this.scratch, 'Prior');
    }
    // 자기 참조 입력은 현재 이름 버퍼와 실제로 같은 배열이다.
    arg() { return this.newKind === 8 ? this.names[0] : this.argument; }
    // 현재 헤더의 주소 토큰을 원본 관측 번호로 변환한다.
    manager(slot) { const index = TOKENS.indexOf(read(this.headers, slot * 20)); if (index < 0) throw new Error('관측 manager 주소'); return index * 11; }
    // 선택된 콜백에만 원문 입력에서 지정한 상태 변경 또는 중단을 적용한다.
    mutate(boundary, fixed = 9) {
        const selected = this.owner, target = fixed < 9 ? fixed : selected, m = this.mutation;
        if (m === `${boundary}-owner`) this.owner = this.other;
        else if (m === `${boundary}-current1`) this.current = 1;
        else if (m === 'snapshot1-manager0' && boundary === 'snapshot1') put(this.headers, selected * 20, 0);
        else if (['snapshot1-readonly1', 'snapshot2-readonly1', 'notify-readonly1'].includes(m) && boundary === m.split('-')[0]) put(this.headers, target * 20 + 8, 1);
        else if (m === 'snapshot1-name' && boundary === 'snapshot1') input(this.arg(), 'Changed');
        else if (['notify-manager22', 'assert429-manager22', 'assert430-manager22', 'assert88-manager22'].includes(m) && boundary === m.split('-')[0]) put(this.headers, target * 20, TOKENS[2]);
        else if (['title-editor1', 'state-editor1', 'config1-editor1'].includes(m) && boundary === m.split('-')[0]) this.editor = 1;
        else if (m === 'title-dirty' && boundary === 'title') this.dirty = 0xffffffff;
        else if (m === 'state-mission1' && boundary === 'state') this.mission = 1;
        else if (m === 'path1-specsolo' && boundary === 'path1') input(this.spec, 'SOLOSpec');
        else if (m === 'path1-pointerother' && boundary === 'path1') this.specPointer = false;
        else if (m === `${boundary}-stop`) throw new ObservationStop(boundary);
    }
    // 구현이 매번 현재 값을 조회하고 동일 dirty 칸을 사용하게 한다.
    globals() {
        const d = this;
        return { owner: () => d.owner, fort: () => d.fort, editor: () => d.editor, mission: () => d.mission,
            get dirty() { return d.dirty; }, set dirty(value) { d.dirty = value; } };
    }
    // 실제 구현이 선택한 외부 효과만 기록하고 명시된 콜백 변이를 실행한다.
    hooks() {
        return {
            header: (slot) => { this.lastHeader = slot; if (slot >= 9) throw new RangeError('관측 헤더'); return this.headers.subarray(slot * 20, slot * 20 + 20); },
            currentName: () => { this.physical = 'current-size'; return this.names[this.current]; },
            managerName: (token) => { if (!token) throw new ObservationStop('manager0'); const index = TOKENS.indexOf(token) - 1; if (index < 0 || index >= 2) throw new Error('관측 manager'); this.physical = 'manager-name-size'; return this.managers[index].subarray(12, 268); },
            selectFileSpec: () => { this.specPointer = true; },
            pathSpec: (source) => {
                const name = text(source), key = text(this.spec); let result;
                if (this.pathMode === 1) result = Buffer.from('C:/fixed.fort');
                else if (this.pathMode === 3 || this.pathMode === 4) {
                    const length = this.pathMode === 3 ? 255 : 256;
                    result = Buffer.concat([Buffer.from('P'.repeat(Math.max(0, length - name.length - 5))), name, Buffer.from('.fort')]);
                } else result = Buffer.concat([Buffer.from(hex(key) === '666f727453706563' ? 'C:/work/d/' : 'C:/solo/d/'), name, Buffer.from('.fort')]);
                if (this.pathMode === 2) result = Buffer.concat([result, Buffer.from('\0TAIL', 'latin1')]);
                this.paths += 1; this.trace.push(`P:${hex(key)}:${hex(name)}:${hex(result)}`); this.physical = 'old-path-size'; this.mutate(`path${this.paths}`);
                return Uint8Array.from(Buffer.concat([result, Buffer.from([0])]));
            },
            assertion: (line) => { this.trace.push(`A:${line}`); this.mutate(`assert${line}`, line === 429 || line === 430 ? this.lastHeader : 9); },
            snapshot: () => { this.snapshots += 1; this.trace.push(`S:${this.owner}:${this.manager(this.owner)}`); this.mutate(`snapshot${this.snapshots}`); },
            notify: (slot) => { this.notifies += 1; const manager = this.manager(slot); this.trace.push(`N:${slot}:${manager}:${hex(text(this.managers[manager / 11 - 1].subarray(12)))}`); this.mutate('notify', slot); },
            samePath: (old, next) => { const a = text(old), b = text(next); this.trace.push(`E:${hex(a)}:${hex(b)}`); const same = sameAscii(a, b); this.trace.push(`Q:${same ? 1 : 0}`); return same; },
            deleteRequest: (old) => { this.deletes += 1; this.trace.push(`D:${hex(text(old))}`); this.mutate('delete'); },
            refreshTitle: () => { this.trace.push(`T:${hex(text(this.names[this.current]))}`); this.mutate('title'); },
            state: (slot) => { this.trace.push(`G:${slot}`); this.mutate('state'); return this.states[slot]; },
            config: (key, value) => { this.configs += 1; this.trace.push(`C:${key}:${hex(text(value))}`); this.mutate(`config${this.configs}`); },
        };
    }
    // 원본 관측의 전역 필드와 모든 물리 범위를 같은 순서로 직렬화한다.
    state() { return [this.owner, this.current, this.dirty, this.fort, this.editor, this.mission, Number(this.specPointer), this.snapshots, this.notifies, this.paths, this.deletes, this.configs].join(','); }
    raw() { return [this.headers, ...this.managers, ...this.names, this.argument, this.spec].map(hex).concat(this.mode === 'current' ? hex(this.scratch) : '-').join(':'); }
    // 제품 함수를 실행하고 원문이 지정한 물리 중단과 콜백 중단만 관측한다.
    run() {
        try {
            if (this.mode === 'current') setCurrentFortName(this.arg(), this.scratch, this.globals(), this.hooks());
            else setFortManagerFileName(this.owner, this.arg(), this.hooks());
            return '1';
        } catch (error) {
            if (error instanceof ObservationStop) return error.cause;
            if (error instanceof RangeError && this.physical) return this.physical;
            throw error;
        }
    }
}

// 동일 입력에서 두 정밀도와 모든 콜백 변이를 실행하고 독립 원본 출력을 비교한다.
test('Fort 이름: 원본 x86 관측 876행의 전체 물리와 호출 순서', () => {
    const file = path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-current-name-x86.tsv');
    const rows = fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#'));
    const unique = new Set(); let stopped = 0;
    // 각 원본 행을 새 장치에서 실행하고 전체 바이트와 중간 효과를 직접 대조한다.
    for (const line of rows) {
        const row = line.split('\t'); assert.equal(row.length, 6); const identity = `${row[0]}\t${row[1]}`;
        assert.equal(unique.has(identity), false, identity); unique.add(identity);
        const devices = new NameDevices(row[1]), completion = devices.run(); if (completion !== '1') stopped += 1;
        assert.equal(completion, row[2], identity); assert.equal(devices.state(), row[3], identity);
        assert.equal(devices.raw(), row[4], identity); assert.equal(devices.trace.join(';'), row[5], identity);
    }
    assert.equal(rows.length, 876); assert.equal(stopped, 156);
});
