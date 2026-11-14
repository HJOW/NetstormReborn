// 원문 4cf730 의 현재 Tell 이름 기록과 세 단계 섹션 조회(원본 10.78 858행)와 실제 Config 등록·적재 수명·전체 공장 호출 경계를 검사한다. 원본게임을 실행하지 않는다. (2026-11-14 WB04-E461)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { readDataMap } from './helpers.mjs';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { Config } from '../www/assets/config.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { ClientHtmlWindowGlobals } from '../www/core/client-html-window-globals.js';
import { parseTellParameters } from '../www/ui/tell-parameters.js';
import { TELL_CURRENT_BYTES, createTellSectionState, findTellSection, findTellSectionInConfig, showTellInConfig } from '../www/ui/tell-section.js';

// 원문 BYTE 문자열을 hex 로 기록한다(UTF-8 변환 없이 BYTE 순서만).
const hex = (value) => Buffer.from(value, 'latin1').toString('hex');
// hex 를 원문 BYTE 문자열로 되돌린다.
const unhex = (value) => Buffer.from(value, 'hex').toString('latin1');
// 현재 256BYTE 저장소의 첫 NUL 앞 BYTE 문자열이다.
const currentName = (state) => { const end = state.current.indexOf(0); return Buffer.from(state.current.subarray(0, end < 0 ? state.current.length : end)).toString('latin1'); };
// 전체 256BYTE 의 순서를 기록하는 독립 FNV-1a 32비트 지문이다(원문 투영과 같은 계산).
function fingerprint(state) {
    let value = 2166136261;
    for (const byte of state.current) value = Math.imul(value ^ byte, 16777619) >>> 0;
    return value.toString(16).padStart(8, '0');
}

// 원문 관측과 같은 입력/하위 장치만 준비한다: 현재 상태와 선택 장치의 존재 여부(mask)만 입력하고 호출 기록을 남긴다.
class SelectionScene {
    state = createTellSectionState(); mask = 0; available = false; trace = [];
    // 선택된 실제 하위 효과의 순서를 기록한다.
    event(text) { this.trace.push(text); }
    // 없음(null)과 빈 문자열 본문을 구별한다.
    body(present, name) { return present ? (this.mask & 32 ? '' : name) : null; }
    // 같은 원문 입력과 현재 256BYTE 로 공개 제품 함수를 실행해 독립 투영 문자열을 돌려준다.
    run(target, prior, flags) {
        this.mask = flags; this.available = (flags & 4) !== 0; this.trace = [];
        this.state.current.fill(0x5a); for (let index = 0; index < prior.length; index += 1) this.state.current[index] = prior.charCodeAt(index); this.state.current[prior.length] = 0;
        const hooks = {
            prior: (name) => { this.event(`P,${hex(name)}`); if (this.mask & 64) { this.available = !this.available; for (const [index, character] of [...'changed'].entries()) this.state.current[index] = character.charCodeAt(0); this.state.current[7] = 0; } },
            named: (prefix, section) => { this.event(`N,${hex(prefix)},${hex(section)}`); return this.body((this.mask & 3) === 3, 'named'); },
            mission: (name) => { if (!this.available) return null; this.event(`M,${hex(name)}`); return this.body((this.mask & 8) !== 0, 'mission'); },
            fallback: (name) => { this.event(`F,${hex(name)}`); return this.body((this.mask & 16) !== 0, 'fallback'); },
        };
        const result = findTellSection(this.state, target, hooks);
        return `${hex(currentName(this.state))}|${fingerprint(this.state)}|${result === null ? '-' : hex(result)}|${this.trace.join(';')}`;
    }
}

test('TellSection 원본 858행: 현재 256BYTE 이름·지문·본문/없음·호출 순서가 전부 일치한다', () => {
    const rows = fs.readFileSync(new URL('../../cpppj/tests/fixtures/tell-section-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t'));
    assert.equal(rows.length, 858);
    // 매 행의 현재 상태를 독립 초기화한다.
    for (const row of rows) { assert.equal(row.length, 5, row[0]); assert.equal(new SelectionScene().run(unhex(row[1]), unhex(row[2]), Number(row[3])), row[4], `${row[0]} mask ${row[3]}`); }
});

// 실제 공개 자료의 GameData 를 설정 인터페이스로 쓴다(영어 아카이브).
const FILES = readDataMap([ARCHIVE_PATH, SETUP_PATH]);
// 빈 읽기 장치/파일 목록으로 설정 인터페이스를 만든다. loadFile 은 시험이 바꿔 끼운다.
function openConfig(assign) { const data = GameData.build({ files: FILES, language: 'english' }); data.configuration.assign(assign); return data; }
// 실제 Config 파일 공급자의 평문 입력을 읽는다.
const loadText = (text) => (config) => config.loadBytes(Buffer.from(text, 'utf8'), decodeOriginalText);

test('TellSection 실제 등록부: 등록 객체 섹션 우선·미등록 접두어의 미션 폴백·존재하는 빈 섹션', () => {
    const reads = []; const data = openConfig('missionSpec=d/{local.1}.english\n');
    data.loadFile = (config, path) => { reads.push(path); loadText('[file.part]\nFallback\n[missing.part]\nFallback missing\n[END]\n')(config); };
    const file = new Config(data.registry, 'file'); file.assign('[part]\nNamed\n[empty]\n[END]\n'); const state = createTellSectionState(); const missionCalls = [];
    // 실제 현재 미션 조회의 호출 인자도 검증한다.
    const mission = (name) => { missionCalls.push(name); return name === 'missing.part' ? 'Mission' : null; };
    assert.equal(findTellSectionInConfig(state, 'file.part', data, mission), 'Named\n'); assert.deepEqual([missionCalls, reads], [[], []]);
    const empty = findTellSectionInConfig(state, 'file.empty', data, mission); assert.equal(empty, ''); assert.deepEqual([missionCalls, reads], [[], []]);
    assert.equal(findTellSectionInConfig(state, 'missing.part', data, mission), 'Mission'); assert.deepEqual([missionCalls, reads], [['missing.part'], []]);
    assert.equal(data.registry.findByName('missing'), null); assert.equal(data.substitute('{global.priorTell}'), 'file.empty');
});

test('TellSection 실제 fallback: 매번 새 임시 Config 로 파일을 재적재하고 실패한 이름도 다음 priorTell 에 남긴다', () => {
    let version = 0; let reads = 0; let duringObjects = 0; const data = openConfig('missionSpec=d/{local.1}.english\n');
    data.loadFile = (config, path) => { reads += 1; assert.equal(path, 'd/tell.english'); duringObjects = data.registry.objects.length; assert.notEqual(data.registry.findByName('tell'), null); loadText(version ? '[About]\nSecond\n[END]\n' : '[About]\nFirst\n[END]\n')(config); };
    const existing = new Config(data.registry, 'tell'); existing.assign('[About]\nCached\n[END]\n'); const before = data.registry.objects.length; const state = createTellSectionState();
    assert.equal(findTellSectionInConfig(state, 'About', data, null), 'First\n'); assert.equal(reads, 1); assert.equal(duringObjects, before + 1); assert.equal(data.registry.objects.length, before); assert.equal(data.registry.findByName('tell'), existing);
    version = 1; assert.equal(findTellSectionInConfig(state, 'ABOUT', data, null), 'Second\n'); assert.equal(reads, 2); assert.equal(data.substitute('{global.priorTell|none}'), '');
    assert.equal(findTellSectionInConfig(state, 'missing.part', data, null), null); assert.equal(reads, 3); assert.equal(data.substitute('{global.priorTell}'), 'ABOUT');
    assert.equal(findTellSectionInConfig(state, 'About', data, null), 'Second\n'); assert.equal(data.substitute('{global.priorTell}'), 'missing.part'); assert.equal(data.registry.objects.length, before);
});

test('TellSection 실제 param 범위: 첫 등록 객체를 조회하고 값 치환은 현재 지역 param 을 쓰며 장기 상태는 Client 수명이다', () => {
    const data = openConfig('missionSpec=d/{local.1}.english\nwidth=800\n'); const registry = data.registry; const outer = new Config(registry, 'param'); outer.assign('1=outer\n');
    const older = new Config(registry, 'file'); older.assign('[part]\n{param.1}\n[END]\n'); const owner = new ClientHtmlWindowGlobals();
    {
        const latest = new Config(registry, 'file'); latest.assign('[part]\nNew\n[END]\n'); const parameters = new Config(registry, 'param');
        const name = parseTellParameters('file.part({width})', { substitute: (text) => data.substitute(text), append: (line) => parameters.append(line) });
        const body = findTellSectionInConfig(owner.tell, name, data, null);
        assert.equal(registry.findByName('file'), older); assert.equal(body, '{param.1}\n'); assert.equal(data.substitute(body), '800\n'); owner.window.instances = 3; // 실제 Config 치환은 본문의 LF 를 보존한다.
        latest.dispose(); parameters.dispose();
    }
    const body = findTellSectionInConfig(owner.tell, 'file.part', data, null);
    assert.equal(data.substitute('{param.1}'), 'outer'); assert.equal(body, '{param.1}\n'); assert.equal(data.substitute(body), 'outer\n'); assert.equal(currentName(owner.tell), 'file.part'); assert.equal(owner.window.instances, 3);
});

test('TellSection 선택 장치: 선택하지 않은 장치는 요구하지 않고 물리 범위 밖 복사만 거부한다', () => {
    const state = createTellSectionState(); const hooks = { prior: () => {}, named: () => '' };
    assert.equal(findTellSection(state, 'file.', hooks), '');
    // 점이 없으면 미션 장치가 필요하지만 이름은 먼저 기록돼 있다.
    assert.throws(() => findTellSection(state, 'next', hooks), { name: 'Error', message: /선택된 실제 조회 미연결/ }); assert.equal(currentName(state), 'next');
    assert.throws(() => findTellSection(state, 'A'.repeat(256), hooks), RangeError); assert.equal(currentName(state), 'next');
    state.current.fill(0x41); assert.throws(() => findTellSection(state, 'next', hooks), RangeError); assert.equal(state.current.every((byte) => byte === 0x41), true);
});

// 원본 관측이 닿지 않는 경계(기본 상태/BYTE 보존/해독 경계/전체 공장 호출/지역 param 반납)를 독립 입력으로 고정한다. (2026-11-14 WB04-E461)
test('TellSection 상태: 256BYTE 0 으로 시작하고 이름 기록은 NUL 뒤 기존 바이트를 지우지 않으며 첫 NUL 앞만 쓴다', () => {
    const state = createTellSectionState(); assert.equal(TELL_CURRENT_BYTES, 256); assert.equal(state.current.length, 256); assert.ok(state.current.every((byte) => byte === 0));
    const calls = []; const hooks = { prior: (name) => calls.push(['prior', name]), named: () => null, mission: (name) => { calls.push(['mission', name]); return null; }, fallback: (name) => { calls.push(['fallback', name]); return null; } };
    state.current.set(Buffer.from('abcdef\0gh')); assert.equal(findTellSection(state, 'xy\0zz', hooks), null);
    assert.deepEqual(Array.from(state.current.subarray(0, 9)), [...Buffer.from('xy\0def\0gh')]); assert.deepEqual(calls, [['prior', 'abcdef'], ['mission', 'xy'], ['fallback', 'xy']]);
    // 같은 이름(대소문자 무시)은 이전 이름 기록을 부르지 않고 새 대소문자를 기록한다.
    calls.length = 0; assert.equal(findTellSection(state, 'XY', hooks), null); assert.deepEqual(calls, [['mission', 'XY'], ['fallback', 'XY']]); assert.equal(currentName(state), 'XY');
    // 255BYTE 이름은 마지막 NUL 자리가 남아 허용하고 256BYTE 는 거부한다.
    assert.equal(findTellSection(state, 'k'.repeat(255), hooks), null); assert.equal(state.current[254], 0x6b); assert.equal(state.current[255], 0);
    assert.throws(() => findTellSection(state, 'k'.repeat(256), hooks), /256BYTE 지역 범위/);
    // 255 를 넘는 글자는 조용히 잘라 쓰지 않고 아무것도 쓰기 전에 거부한다.
    const before = state.current.slice(); assert.throws(() => findTellSection(state, 'aĀ', hooks), /BYTE 문자열/); assert.deepEqual(state.current, before);
    assert.throws(() => findTellSection(state, 'Āb', hooks), /BYTE 문자열/); assert.deepEqual(state.current, before);
    assert.equal(findTellSection(state, 'aÿ', hooks), null); assert.equal(state.current[1], 0xff);
    // 첫 글자가 NUL 이면 빈 이름이다(이름 저장소 첫 바이트만 0 이 되고 뒤 바이트는 남는다).
    calls.length = 0; assert.equal(findTellSection(state, '\0abc', hooks), null); assert.equal(state.current[0], 0); assert.equal(state.current[1], 0xff); assert.deepEqual(calls.at(-1), ['fallback', '']);
    // 현재 이름 저장소에 NUL 이 없으면 아무것도 쓰기 전에 거부한다.
    state.current.fill(0x41); assert.throws(() => findTellSection(state, 'b', hooks), /NUL 범위/); assert.ok(state.current.every((byte) => byte === 0x41));
});

test('TellSection 조회 순서: 점 이름은 등록 객체→미션→fallback 순이고 빈 본문/undefined 는 각각 있음/없음이다', () => {
    const trace = []; const bodies = { named: undefined, mission: undefined, fallback: undefined };
    const hooks = { prior: () => {}, named: (prefix, section) => { trace.push(['named', prefix, section]); return bodies.named; }, mission: (name) => { trace.push(['mission', name]); return bodies.mission; }, fallback: (name) => { trace.push(['fallback', name]); return bodies.fallback; } };
    const state = createTellSectionState();
    assert.equal(findTellSection(state, 'a.b.c', hooks), null); assert.deepEqual(trace.map((entry) => entry[0]), ['named', 'mission', 'fallback']); assert.deepEqual(trace[0], ['named', 'a', 'b.c']);
    trace.length = 0; assert.equal(findTellSection(state, '.x', hooks), null); assert.deepEqual(trace[0], ['named', '', 'x']);
    trace.length = 0; bodies.named = ''; assert.equal(findTellSection(state, 'a.b', hooks), ''); assert.deepEqual(trace.map((entry) => entry[0]), ['named']);
    trace.length = 0; bodies.named = null; bodies.mission = ''; assert.equal(findTellSection(state, 'a.b', hooks), ''); assert.deepEqual(trace.map((entry) => entry[0]), ['named', 'mission']);
    trace.length = 0; bodies.mission = null; bodies.fallback = ''; assert.equal(findTellSection(state, 'plain', hooks), ''); assert.deepEqual(trace.map((entry) => entry[0]), ['mission', 'fallback']);
    bodies.fallback = undefined; assert.equal(findTellSection(state, 'plain', hooks), null);
});

test('TellSection 설정 경계: 이름/이전 이름은 Windows-1252 BYTE 로 해독하고 priorTell 쓰기는 저장 통지를 내지 않는다', () => {
    const data = openConfig('missionSpec=d/{local.1}.english\n'); const notified = []; data.onChange = (key, value) => notified.push([key, value]);
    const book = new Config(data.registry, 'book'); book.assign('[café]\nCoffee\n[END]\n'); const state = createTellSectionState();
    // 0xe9 BYTE 는 유효한 UTF-8 이 아니므로 Windows-1252 로 해독돼 등록 객체의 'café' 섹션을 찾는다.
    assert.equal(findTellSectionInConfig(state, 'book.café', data, null), 'Coffee\n'); assert.equal(currentName(state), 'book.café');
    assert.equal(findTellSectionInConfig(state, 'next', data, () => ''), ''); assert.equal(data.substitute('{global.priorTell}'), 'book.café'); assert.deepEqual(notified, []);
    // 이미 해독된 문자열(255 초과 글자)은 그대로 둔다. 미션 조회에는 해독한 이름을 넘긴다.
    const names = []; findTellSectionInConfig(createTellSectionState(), 'mé', data, (name) => { names.push(name); return null; }); assert.deepEqual(names, ['mé']);
});

test('TellSection 전체 공장: 지역 param 은 조회와 공장 동안 등록되고 mode/폭높이 0 과 serial DWORD 를 전달한다', () => {
    for (const mode of [0, 1, 0xffffffff]) {
        const data = openConfig('missionSpec=d/{local.1}.english\nwidth=800\n'); const outer = new Config(data.registry, 'param'); outer.assign('1=outer\n');
        const book = new Config(data.registry, 'book'); book.assign('[entry]\n{param.1}\n[END]\n'); const objects = data.registry.objects.length; const state = createTellSectionState(); const calls = [];
        const result = showTellInConfig(state, 'book.entry({width})', mode, data, null, (body, suppress, width, height) => { calls.push([body, suppress, width, height, data.substitute('{param.1}'), data.registry.objects.length]); return -1; });
        assert.deepEqual(result, { found: true, serial: 0xffffffff }); assert.deepEqual(calls, [['{param.1}\n', mode, 0, 0, '800', objects + 1]]);
        assert.equal(data.substitute('{param.1}'), 'outer'); assert.equal(data.registry.objects.length, objects); assert.equal(currentName(state), 'book.entry');
    }
});

test('TellSection 전체 공장: 없는 섹션은 공장을 부르지 않고 빈 본문은 있는 것이며 실패해도 지역 param 을 내린다', () => {
    const data = openConfig('missionSpec=d/{local.1}.english\n'); data.loadFile = loadText('[About]\nFallback\n[END]\n'); const book = new Config(data.registry, 'book'); book.assign('[empty]\n[throw]\nBody\n[END]\n');
    const objects = data.registry.objects.length; const state = createTellSectionState(); let calls = 0;
    const show = (body) => { calls += 1; return body === '' ? 0 : 5; };
    assert.deepEqual(showTellInConfig(state, 'missing(arg)', 0, data, null, undefined), { found: false, serial: 0 }); assert.equal(calls, 0); assert.equal(data.registry.objects.length, objects);
    assert.deepEqual(showTellInConfig(state, 'book.empty(arg)', 0, data, null, show), { found: true, serial: 0 }); assert.equal(calls, 1);
    assert.deepEqual(showTellInConfig(state, 'About', 0, data, null, show), { found: true, serial: 5 }); assert.equal(calls, 2); assert.equal(data.registry.objects.length, objects);
    // 공장이 던져도 param 이 내려가고 예외는 그대로 전파된다.
    assert.throws(() => showTellInConfig(state, 'book.throw(arg)', 0, data, null, () => { assert.equal(data.substitute('{param.1}'), 'arg'); throw new Error('공장 경계의 명시 실패'); }), /공장 경계/);
    assert.equal(data.registry.findByName('param'), null); assert.equal(data.registry.objects.length, objects);
    // 선택된 공장이 연결되지 않았으면 존재하는 섹션에서만 요구한다.
    assert.throws(() => showTellInConfig(state, 'book.empty', 0, data, null, undefined), /선택된 실제 조회 미연결/); assert.equal(data.registry.findByName('param'), null);
    // 인자 치환 결과가 Windows-1252 로 나타낼 수 없으면 조회 전에 거부하고 param 을 내린다.
    data.configuration.assign('missionSpec=d/{local.1}.english\nhangul=한\n'); const before = currentName(state);
    assert.throws(() => showTellInConfig(state, 'book.empty({hangul})', 0, data, null, show), RangeError); assert.equal(currentName(state), before); assert.equal(data.registry.findByName('param'), null);
});

test('TellSection 설정 경계: 1252 전용 BYTE 와 UTF-8 BYTE 열을 접두·섹션·미션·이전 이름·param 값/줄 모두에서 해독한다', () => {
    const data = openConfig('missionSpec=d/{local.1}.english\n'); const book = new Config(data.registry, 'book'); book.assign('[’x]\nBody\n[café]\nCoffee\n[END]\n');
    const state = createTellSectionState(); const names = [];
    // BYTE 0x92 는 Windows-1252 의 ’(U+2019)이고 BYTE C3 A9 는 UTF-8 의 é 다.
    assert.equal(findTellSectionInConfig(state, 'book.\x92x', data, null), 'Body\n'); assert.equal(findTellSectionInConfig(state, 'book.caf\xc3\xa9', data, null), 'Coffee\n');
    const odd = new Config(data.registry, '’obj'); odd.assign('[s]\nOdd\n[END]\n'); assert.equal(findTellSectionInConfig(state, '\x92obj.s', data, null), 'Odd\n');
    // 현재 미션 조회에는 해독한 이름을 넘기고, 다음 이름 기록은 이전 이름을 해독해 priorTell 에 쓴다.
    findTellSectionInConfig(state, 'm\x92', data, (name) => { names.push(name); return ''; }); assert.deepEqual(names, ['m’']);
    findTellSectionInConfig(state, 'z', data, () => ''); assert.equal(data.substitute('{global.priorTell}'), 'm’');
    // 지역 param 의 인자 값은 해독→치환→원본 BYTE 로 되돌림→줄로 덧붙일 때 다시 해독된다.
    let seen = null; assert.deepEqual(showTellInConfig(state, 'book.\x92x(a\x92)', 0, data, null, (body) => { seen = [body, data.substitute('{param.1}')]; return 1; }), { found: true, serial: 1 }); assert.deepEqual(seen, ['Body\n', 'a’']);
});

test('TellSection fallback: 임시 Config 는 tell 이름으로 등록돼 적재 중 조회에 보인다', () => {
    const data = openConfig('missionSpec=d/{local.1}.english\n'); let seen = null; const state = createTellSectionState();
    data.loadFile = (config) => { seen = data.registry.findByName('tell') === config; loadText('[A]\nx\n[\u2019y]\nz\n[END]\n')(config); };
    assert.equal(findTellSectionInConfig(state, 'A', data, null), 'x\n'); assert.equal(seen, true); assert.equal(findTellSectionInConfig(state, '\x92y', data, null), 'z\n'); assert.equal(data.registry.findByName('tell'), null);
});
