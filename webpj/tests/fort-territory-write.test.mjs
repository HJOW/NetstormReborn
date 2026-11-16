// 004bd600의 원본 writer200관측과 Session 실제 Template9/일반 MemStream 결선을 대조한다(WB04-E588).
// 같은 fixture의 회복774행은 ExistingEditor 전용004b5ec0이라 이 웹 검사 범위에서 제외한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put } from './raw-support.mjs';
import { writeFortTerritoryRecords } from '../www/core/raw-fort-territory-write.js';
import { writeCampaignTerritory } from '../www/core/raw-campaign-territory-write.js';
import { writeMemStream, MEM_STREAM_ASSERTION } from '../www/core/raw-mem-stream.js';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { SECTION_NAMES } from '../www/assets/fort.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 원본 실행기의 세 실제 주소 집합이다. manager0도 기본 manager로 대체하지 않는다.
const MANAGERS = [0, 0x12001000, 0x12002000]; const STREAMS = [0x12003000, 0x12003020, 0x12003040]; const BUFFERS = [0x12004000, 0x12004200, 0x12004400];
// 관측은 입력 구성 후에만 읽는다. 같은974행 중 writer200행을 명시 선택한다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-territory-recover-x86.tsv'));
// 주변 NUL/미선택 바이트도 모두 비교하는 소문자 hex다.
function hex(bytes) { return Buffer.from(bytes).toString('hex'); }
// 명시 callback/0주소 물리 중단을 예외 유형과 구별한다.
class FixtureStop extends Error { code; constructor(code) { super(code); this.code = code; } }
// 현재 입력의20×6BYTE 원문이다. 본문의 쓰기/진단 분기는 이 함수가 계산하지 않는다.
function recordsFor(owner, pattern) {
    const bytes = Uint8Array.from({ length: 120 }, (_, n) => n * 13 + 7 + owner * 11);
    // flags 외 다섯 바이트는 입력 패턴 그대로 둔다.
    for (let n = 0; n < 20; ++n) { const at = n * 6 + 1; bytes[at] &= 0xfb; if (pattern === 4) bytes[at] = 255; else if (pattern === 1 || (pattern === 2 && n % 2 === 0) || (pattern === 3 && n === 19)) bytes[at] |= 4; }
    return bytes;
}
// cpppj FortTerritoryRecoverTests의 외부 장치 입력만 옮긴다. 일반 쓰기 몸체는 실제 writeMemStream이다.
class WriterDevices {
    // 입력의 고정 owner/현재 manager·stream·주변 전체 물리와 관측 호출수/trace다.
    owner; fixed; other; mutation; nullInput; headers; managers; streams; buffers; states; input; calls = new Uint32Array(6); trace = []; outputStream = 0;
    // 기대값 없이 입력10칸만으로 원문의 메모리 배치를 구성한다.
    constructor(description) {
        const f = description.split(','); assert.equal(f.length, 10); const [kind, owner, manager, pattern, state, position, end, capacity, nullInput] = f; assert.equal(kind, 'write');
        this.owner = this.fixed = Number(owner); this.other = (this.owner + 1) % 9; this.mutation = f[9]; this.nullInput = Number(nullInput);
        this.headers = Uint8Array.from({ length: 180 }, (_, n) => 0x70 + n); this.states = Array.from({ length: 9 }, (_, slot) => { const bytes = Uint8Array.from({ length: 28 }, (_, n) => 0x80 + slot + n * 7); bytes[0] = Number(state); return bytes; });
        // 현재 owner와 다른 owner 이외의 manager DWORD는0이다.
        for (let slot = 0; slot < 9; ++slot) put(this.headers, slot * 20, 0);
        put(this.headers, this.other * 20, MANAGERS[2]); put(this.headers, this.owner * 20, MANAGERS[Number(manager) / 11]);
        this.managers = Array.from({ length: 2 }, (_, n) => { const bytes = Uint8Array.from({ length: 16 }, (_, at) => 0x50 + (n + 1) * 11 + at); put(bytes, 8, STREAMS[n]); return bytes; });
        this.streams = Array.from({ length: 3 }, (_, n) => { const bytes = new Uint8Array(16); put(bytes, 0, BUFFERS[n]); put(bytes, 4, Number(position)); put(bytes, 8, Number(end)); put(bytes, 12, Number(capacity)); return bytes; });
        this.buffers = Array.from({ length: 3 }, (_, n) => Uint8Array.from({ length: 256 }, (_, at) => 0xb0 + n * 7 + at)); this.input = recordsFor(this.owner, Number(pattern));
    }
    // 관측용 주소 번호만 조회한다. 미등록 주소는 정상0으로 바꾸지 않는다.
    index(tokens, value) { const n = tokens.indexOf(value); if (n < 0) throw new RangeError('Territory 관측 토큰'); return n; }
    // writer200행의 유일한 외부 변이는 첫 두 진단의 manager 교체/명시 중단이다.
    assertion(line) { ++this.calls[5]; this.trace.push(`A:${line}`); if (this.mutation === `assert${line}-manager22`) put(this.headers, this.fixed * 20, MANAGERS[2]); else if (this.mutation === `assert${line}-stop`) throw new FixtureStop(`assert${line}`); }
    // 원본 현재manager/Create/current/일반쓰기 장치를 공급한다.
    hooks() { return {
        manager: (slot) => { const value = get(this.headers, slot * 20); this.trace.push(`M:${slot}:${this.index(MANAGERS, value) * 11}`); return value; },
        assertion: (line) => this.assertion(line),
        create: (manager, name) => { assert.equal(name, 'Territory'); ++this.calls[2]; this.trace.push(`K:${this.index(MANAGERS, manager) * 11}:Territory`); },
        current: (manager) => { const n = this.index(MANAGERS, manager); if (!n) throw new FixtureStop('manager0'); const stream = get(this.managers[n - 1], 8); this.trace.push(`C:${n * 11}:${this.index(STREAMS, stream)}`); return stream; },
        write: (token, records, length) => {
            assert.equal(length, 120); this.outputStream = this.index(STREAMS, token); ++this.calls[3]; this.trace.push(`W:${this.outputStream}:120:${records ? hex(records) : 'null'}`);
            try { writeMemStream(this.streams[this.outputStream], records ?? new Uint8Array(0), length, { buffer: (pointer) => this.buffers[this.index(BUFFERS, pointer)], assertion: (kind) => { assert.equal(kind, MEM_STREAM_ASSERTION.writeCapacity); this.assertion(36); } }); } catch (error) { if (error instanceof RangeError && !records) throw new FixtureStop('source0'); throw error; }
        },
    }; }
    // 고정 this 슬롯을 기록한 뒤 실제 제품 writer만 실행한다.
    run() { this.trace.push(`F:${this.owner}`); writeFortTerritoryRecords(this.owner, this.nullInput ? null : this.input, this.hooks()); }
    // 현재owner와 여섯 호출수를 원문 순서로 관측한다.
    state() { return [this.owner, ...this.calls].join(','); }
    // 미선택 manager/stream/버퍼·State까지 전부 비교한다.
    raw() { return [hex(this.headers), ...this.managers.map(hex), ...this.streams.map(hex), ...this.buffers.map(hex), this.states.map(hex).join(''), hex(this.input), '-'].join(':'); }
}

test('FortTerritory writer: 원본200행의 정상/중단·주변 전체 raw·현재 manager/stream·trace가 같다', () => {
    assert.ok(rows); assert.equal(rows.length, 974); const selected = rows.filter((row) => row[1].startsWith('write,')); assert.equal(selected.length, 200); assert.equal(rows.filter((row) => row[1].startsWith('recover,')).length, 774); let stopped = 0;
    // 각 입력 뒤에만 기대값을 읽어 함수의 전체 결과를 대조한다.
    for (const row of selected) { const d = new WriterDevices(row[1]); let result = '1'; try { d.run(); } catch (error) { if (!(error instanceof FixtureStop)) throw error; result = error.code; ++stopped; } assert.deepEqual([result, d.state(), d.raw(), d.trace.join(';')], row.slice(2), row[1]); }
    assert.equal(stopped, 134);
});

// 실제 manager 초기화가 읽을35개 빈 섹션이다. 파일 저장/편집기 데이터는 만들지 않는다.
function emptyFort() { return { sections: Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)) }; }

test('Session Territory: 실제 Template9에120BYTE를 쓰고 current/cursor/노드 수명을 보존한다', () => {
    const storage = new CampaignTemplateStorage(); storage.install(1, emptyFort(), 'mission.fort'); const before = storage.createSection(1, 'Territory'); const records = recordsFor(1, 3);
    writeCampaignTerritory(storage, 1, records); assert.equal(storage.currentSection(1), 'Territory'); const stream = storage.currentWriteStream(1); assert.notEqual(stream.generation, before.generation); assert.throws(() => storage.accessSection(before, () => {}), /수명/);
    const territory = storage.selectTerritory(1); assert.deepEqual(territory.bytes, records); assert.equal(territory.position, 0); assert.equal(territory.end, 120); assert.deepEqual(storage.selectTerritory(1).bytes, records);
});

test('Session Territory: Create 반환 대신 이후 현재stream을 선택하고 고정 슬롯의 빈 Territory는 유지한다', () => {
    const storage = new CampaignTemplateStorage(); storage.install(1, emptyFort(), 'mission.fort'); const records = recordsFor(1, 2); const create = storage.createSection.bind(storage); let ignored;
    storage.createSection = (slot, name) => { ignored = create(slot, name); create(slot, 'Temp'); return ignored; };
    writeCampaignTerritory(storage, 1, records); const current = storage.currentWriteStream(1); assert.notEqual(current.section, ignored.section); assert.equal(storage.currentSection(1), 'Temp'); storage.accessSection(current, (fields, memory) => { assert.equal(get(fields, 4), 120); assert.deepEqual(memory.buffer(get(fields, 0)).subarray(0, 120), records); }); assert.equal(storage.territory(1).end, 0); assert.throws(() => storage.selectTerritory(1), /실제 섹션 선택 진단/);
});

test('Session Territory: 누락 manager/NULL/범위·Create 뒤 실패는 선택된 위치와 앞선 효과를 유지한다', () => {
    const storage = new CampaignTemplateStorage(); assert.throws(() => writeCampaignTerritory(storage, 9, new Uint8Array(120)), /Template 범위/); assert.throws(() => writeCampaignTerritory(storage, 1, new Uint8Array(120)), /Territory 521/);
    storage.install(1, emptyFort(), 'mission.fort'); assert.throws(() => writeCampaignTerritory(storage, 1, null), /Territory 522/);
    const current = storage.currentWriteStream.bind(storage); storage.currentWriteStream = () => { throw new Error('current 중단'); }; assert.throws(() => writeCampaignTerritory(storage, 1, recordsFor(1, 0)), /current 중단/); storage.currentWriteStream = current; assert.equal(storage.currentSection(1), 'Territory'); assert.equal(storage.territory(1).end, 0);
});

test('Session owner Territory: 같은 실제 Template 저장소에 연결하고 반납 후 호출을 거부한다', () => {
    const data = realCaptureAssets(); const o = new RawCampaignSessionOwners(new GameAssets(data.table, data.database), { read() { throw new Error('선택되지 않은 파일 읽기'); } }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList() });
    try { o.templateStorage.install(1, emptyFort(), 'mission.fort'); const records = recordsFor(1, 1); o.writeTerritory(1, records); assert.deepEqual(o.templateStorage.selectTerritory(1).bytes, records); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.writeTerritory(1, new Uint8Array(120)), /반납/);
});
