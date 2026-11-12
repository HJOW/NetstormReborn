// 미션 기술 허용 목록 적용(mission-technology.js)을 원본 10.78 관측과 cpppj 시험, 웹 계약으로 확인한다.
//  - 원본 관측: mission-technology-x86.tsv 55행(목록 문자열·호출 도중 바뀌는 현재 목록·초기 표 시드 × deny/allow/all/이름 항목, 빈 항목 종료, 미등록 이름 -1 쓰기)의 257DWORD 표 전체와 호출 순서를 대조한다.
//  - cpppj 시험 이식: MissionTechnologyTests 의 Native55. 나머지 `MissionTechnology_ProtectedElevenAndInvalidStorage` 는 실제 MissionScript(Mission 단위, 4-D B 구획)가 필요해 그 단위에서 옮긴다.
//  - 웹 계약: 키워드 대소문자·공백 비정리·빈 항목 종료와 그 뒤 비조회, 매 항목 재조회, 도달한 효과만 요구, 미등록 이름 -1 전달, 스크립트 래퍼(실제 RawTotalMade·전역 금지 표)의 목록 조회·512BYTE 범위·Windows-1252 부호화·NUL 절단·미등록 이름 거부 시 무변경.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E338)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { TypeTable } from '../www/assets/type-table.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { configListItem } from '../www/core/raw-mission-ai-settings.js';
import { RawTotalMade } from '../www/core/raw-total-made.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { applyMissionTechnology, applyMissionTechnologyToRestrictions, applyMissionTechnologyToTotalMade } from '../www/core/mission-technology.js';

// 원본 관측 행이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-12 WB04-E338)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-technology-x86.tsv'));

// 한 번만 읽는 실제 보호 타입 표이다. (2026-11-12 WB04-E338)
let cachedTable = null;
// 실제 netstorm.tarc 에서 이름 검색 가능한 타입 표를 만든다. (2026-11-12 WB04-E338)
function typeTable() {
    cachedTable ??= TypeTable.fromArchive(new TaffArchive(readData('netstorm.tarc')), decodeOriginalText);
    return cachedTable;
}

// TSV 의 16진 바이트를 한 글자 한 바이트 문자열로 복원한다(`-` 는 빈 값). (2026-11-12 WB04-E338)
function technologyBytes(encoded) {
    if (encoded === '-') return '';
    let text = '';
    // 두 16진수 글자를 원본 한 바이트로 돌린다.
    for (let index = 0; index < encoded.length; index += 2) text += String.fromCharCode(Number.parseInt(encoded.slice(index, index + 2), 16));
    return text;
}

// 예외가 나는지 본다. (2026-11-12 WB04-E338)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

// 실제 255칸 쓰기와 경계 두 칸을 관찰하는 독립 장면이다. cpppj TechnologyScene 에 대응한다. (2026-11-12 WB04-E338)
class TechnologyScene {
    // 보호 타입 표와 변하지 않아야 하는 장부, 앞 관측 칸 + 물리 256칸이다.
    constructor(row) {
        this.types = typeTable();
        this.counts = new SquidPostPopState();
        this.table = new Uint32Array(257);
        this.total = new RawTotalMade(realPlayerAssets().types, this.counts, this.table.subarray(1));
        this.text = technologyBytes(row[0]);
        this.changed = technologyBytes(row[1]);
        this.trace = '';
        this.lookups = 0;
        const seed = Number(row[2]);
        // 앞/끝 경계도 서로 다른 입력 값으로 둔다.
        for (let index = 0; index < this.table.length; index += 1) this.table[index] = (Math.imul(index, 7) ^ seed) >>> 0;
    }

    // 호출 효과 순서를 이어 붙인다.
    add(value) {
        if (this.trace) this.trace += '|';
        this.trace += value;
    }

    // 원본 목록 처리와 실제 표를 같은 현재 입력으로 실행한다.
    run() {
        const hooks = {
            item: (index) => {
                this.add(`G,${index}`);
                const current = this.changed !== '' && this.lookups > 0 ? this.changed : this.text;
                this.lookups += 1;
                const nul = current.indexOf('\0');
                return configListItem(nul < 0 ? current : current.slice(0, nul), index);
            },
            findType: (name) => this.types.find(name),
            allowAll: (value) => {
                this.add(`A,${value}`);
                this.total.allowAll(value);
            },
            allow: (type, value) => {
                this.add(`T,${type >>> 0},${value}`);
                // 원본의 표 앞 쓰기(-1)를 이 검사에서만 격리해 관측한다. 실제 저장소는 아래 계약처럼 거부한다.
                if (type === -1) this.table[0] = value === 0 ? 1 : 0;
                else this.total.allow(type, value);
            },
        };
        applyMissionTechnology(hooks);
    }
}

// 257DWORD 를 리틀 엔디언 16진 문자열로 만든다. (2026-11-12 WB04-E338)
function tableHex(table) {
    let text = '';
    // 값마다 낮은 바이트부터 적는다.
    for (const value of table) for (let index = 0; index < 4; index += 1) text += ((value >>> (index * 8)) & 0xff).toString(16).padStart(2, '0');
    return text;
}

test('원본 관측 55행: 목록 분할·이름 검색·257DWORD 표·호출 순서가 모두 일치한다', { skip: ROWS === null }, () => {
    assert.equal(ROWS.length, 55);
    // 각 독립 입력은 새 장부/표/현재 목록을 갖는다.
    for (const row of ROWS) {
        assert.equal(row.length, 5);
        const scene = new TechnologyScene(row);
        const before = [scene.counts.localCounts().slice(), scene.counts.localSecondaryCounts().slice(), scene.counts.globalCounts().slice()];
        scene.run();
        assert.equal(tableHex(scene.table), row[3]);
        assert.equal(scene.trace, row[4]);
        assert.deepEqual([scene.counts.localCounts(), scene.counts.localSecondaryCounts(), scene.counts.globalCounts()], before);
    }
});

// 목록과 효과를 기록하는 단순 장면이다. (2026-11-12 WB04-E338)
function recorder(items, find = (name) => name.length) {
    const log = [];
    return {
        log,
        hooks: {
            item: (index) => { log.push(`G${index}`); return items[index] ?? ''; },
            findType: (name) => { log.push(`F:${name}`); return find(name); },
            allowAll: (value) => log.push(`A${value}`),
            allow: (type, value) => log.push(`T${type},${value}`),
        },
    };
}

test('키워드는 ASCII 대소문자를 무시하고 허용 상태만 바꾸며 이름은 원문 그대로 찾는다', () => {
    const scene = recorder(['DENY', 'All', 'Allow', 'aLL', 'deny', 'Ab', 'ALLOW', ' all']);
    applyMissionTechnology(scene.hooks);
    // deny → all(0) → allow → all(1) → deny → 이름(0) → allow → 앞 공백 이름(1)이며 공백 항목은 새로 다듬지 않고 이름은 대소문자도 그대로 찾는다.
    assert.deepEqual(scene.log, ['G0', 'G1', 'A0', 'G2', 'G3', 'A1', 'G4', 'G5', 'F:Ab', 'T2,0', 'G6', 'G7', 'F: all', 'T4,1', 'G8']);
});

test('첫 빈 항목에서 끝나고 그 뒤는 조회하지 않으며 NUL 뒤 바이트는 보지 않는다', () => {
    const scene = recorder(['x', 'y\0hidden', '', 'z']);
    applyMissionTechnology(scene.hooks);
    assert.deepEqual(scene.log, ['G0', 'F:x', 'T1,1', 'G1', 'F:y', 'T1,1', 'G2']);
    const onlyNul = recorder(['\0zzz', 'a']);
    applyMissionTechnology(onlyNul.hooks);
    assert.deepEqual(onlyNul.log, ['G0']);
});

test('매 항목마다 현재 목록을 다시 조회하고 인덱스는 0 부터 하나씩 늘어난다', () => {
    const seen = [];
    let list = 'a;b;c';
    applyMissionTechnology({
        item: (index) => {
            seen.push(index);
            // 둘째 조회부터 다른 목록을 돌려준다.
            if (index === 1) list = 'a;B;stop';
            return configListItem(list, index);
        },
        findType: (name) => name.charCodeAt(0),
        allowAll: () => { throw new Error('all 은 쓰이지 않아야 함'); },
        allow: () => {},
    });
    assert.deepEqual(seen, [0, 1, 2, 3]);
});

test('도달한 효과의 누락만 거부한다', () => {
    // all 이 없는 목록은 allowAll 없이 끝난다.
    applyMissionTechnology({ item: (index) => (index === 0 ? 'x' : ''), findType: () => 5, allow: () => {} });
    // 이름 항목에서 allow/findType 이 없으면 거부한다.
    assert.ok(throws(() => applyMissionTechnology({ item: () => 'x', findType: () => 5 })));
    assert.ok(throws(() => applyMissionTechnology({ item: () => 'x', allow: () => {} })));
    // all 에서 allowAll 이 없으면 거부하고 item 이 없으면 처음부터 거부한다.
    assert.ok(throws(() => applyMissionTechnology({ item: () => 'all' })));
    assert.ok(throws(() => applyMissionTechnology({})));
    assert.ok(throws(() => applyMissionTechnology(undefined)));
    // 빈 목록은 쓰기 효과를 요구하지 않는다.
    applyMissionTechnology({ item: () => '' });
});

test('미등록 이름의 -1 도 그대로 쓰기 몸체에 전달한다', () => {
    const scene = recorder(['nothing'], () => -1);
    scene.hooks.item = (index) => { scene.log.push(`G${index}`); return index === 0 ? 'nothing' : ''; };
    applyMissionTechnology(scene.hooks);
    assert.deepEqual(scene.log, ['G0', 'F:nothing', 'T-1,1', 'G1']);
});

// 작은 스크립트 대역이다: get('techAllowed') 만 쓴다. (2026-11-12 WB04-E338)
const fakeScript = (value) => ({ get: (key) => (key === 'techAllowed' ? value : undefined) });

test('스크립트 래퍼(실제 RawTotalMade): deny/allow/all/이름이 같은 허용 표를 쓰고 장부는 건드리지 않는다', () => {
    const types = typeTable();
    const counts = new SquidPostPopState();
    const restrictions = new Uint32Array(256);
    restrictions[255] = 0x7391;
    const total = new RawTotalMade(realPlayerAssets().types, counts, restrictions);
    const wind = types.find('windVortex');
    const rain = types.find('rainCannon');
    assert.ok(wind > 0 && rain > 0);
    applyMissionTechnologyToTotalMade(fakeScript('deny;all;allow;windVortex'), types, total);
    // all(allow 전 deny=0) → 첫 255칸 금지 후 windVortex 만 허용이며 256번째 칸은 보존된다.
    assert.equal(total.allowed(wind), true);
    assert.equal(total.allowed(rain), false);
    assert.equal(restrictions[255], 0x7391);
    assert.deepEqual(Array.from(counts.localCounts()).filter((value) => value !== 0), []);
    // deny 뒤 이름은 그 타입만 금지한다(allow 값 0 이 개별 쓰기까지 전달된다).
    applyMissionTechnologyToTotalMade(fakeScript('allow;all;deny;windVortex'), types, total);
    assert.equal(total.allowed(wind), false);
    assert.equal(total.allowed(rain), true);
    // 속성이 없으면 빈 목록이라 표를 바꾸지 않는다.
    const before = restrictions.slice();
    applyMissionTechnologyToTotalMade(fakeScript(undefined), types, total);
    applyMissionTechnologyToTotalMade({ get: () => null }, types, total);
    assert.deepEqual(restrictions, before);
});

test('스크립트 래퍼(전역 금지 표): 같은 쓰기를 Uint32Array 에 하고 미등록 이름은 쓰기 전에 거부한다', () => {
    const types = typeTable();
    const restrictions = new Uint32Array(256).fill(7);
    restrictions[255] = 0x1234;
    const wind = types.find('windVortex');
    applyMissionTechnologyToRestrictions(fakeScript('deny;all;allow;windVortex'), types, restrictions);
    assert.equal(restrictions[wind], 0);
    assert.equal(restrictions[wind + 1], 1);
    assert.equal(restrictions[255], 0x1234);
    // deny 뒤 이름은 그 칸만 금지(1)로 쓴다.
    applyMissionTechnologyToRestrictions(fakeScript('allow;all;deny;windVortex'), types, restrictions);
    assert.equal(restrictions[wind], 1);
    assert.equal(restrictions[wind + 1], 0);
    assert.equal(restrictions[0], 0);
    assert.equal(restrictions[254], 0);
    // 이름이 없으면 -1 이 실제 저장소에서 범위 오류로 거부되고 표는 그대로이다.
    const before = restrictions.slice();
    assert.ok(throws(() => applyMissionTechnologyToRestrictions(fakeScript('allow;missingType'), types, restrictions), RangeError));
    assert.deepEqual(restrictions, before);
});

test('스크립트 래퍼의 목록 처리: NUL 절단·Windows-1252 부호화·512BYTE 범위', () => {
    const types = typeTable();
    const wind = types.find('windVortex');
    const restrictions = new Uint32Array(256).fill(1);
    // NUL 뒤의 항목은 보이지 않는다.
    applyMissionTechnologyToRestrictions(fakeScript('windVortex\0;rainCannon'), types, restrictions);
    assert.equal(restrictions[wind], 0);
    assert.equal(restrictions[types.find('rainCannon')], 1);
    // 511글자는 허용되고(뒤의 빈 항목은 첫 빈 항목에서 끝남) 512글자는 임시 버퍼 범위 오류이다.
    const fits = new Uint32Array(256).fill(1);
    applyMissionTechnologyToRestrictions(fakeScript(`windVortex${';'.repeat(501)}`), types, fits);
    assert.equal(fits[wind], 0);
    const tooLong = new Uint32Array(256).fill(1);
    assert.ok(throws(() => applyMissionTechnologyToRestrictions(fakeScript(`windVortex${';'.repeat(502)}`), types, tooLong), RangeError));
    assert.deepEqual(tooLong, new Uint32Array(256).fill(1));
    // 표현할 수 없는 문자는 부호화에서 거부한다.
    assert.ok(throws(() => applyMissionTechnologyToRestrictions(fakeScript('한글'), types, restrictions), RangeError));
});

test('스크립트 래퍼는 Windows-1252 바이트를 한 글자씩 보존해 이름 검색에 넘긴다', () => {
    const names = [];
    const types = { find: (name) => { names.push(name); return 3; } };
    const restrictions = new Uint32Array(256).fill(1);
    applyMissionTechnologyToRestrictions(fakeScript('café;caf€'), types, restrictions);
    // é 는 0xe9 한 바이트, 유로 기호는 0x80 한 바이트로 부호화되어 그 코드 그대로의 글자가 된다.
    assert.deepEqual(names, ['café', 'caf']);
    assert.equal(restrictions[3], 0);
});
