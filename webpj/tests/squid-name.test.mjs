// 객체 이름 raw-squid-name.js(RawSquidName·squidNameEntry·squidLevelName·formatSuffix)를 cpppj 계약 검사 4개와 같은 입력으로 대조한다.
//  - 계약: SquidName_BaseLanguageReadOrderAndSubstitution(언어 조회 횟수·번역 선택)·SquidName_FactorySuffixAfterBaseAndCurrentLevel(base→패턴→레벨 순서)·
//    SquidName_CurrentTypeFieldAndSelectedBounds(TYPE 선택 시점·NUL·80바이트 경계)·SquidName_EntriesRomanSentinelAndCrtSuffix(83표·로마숫자·CRT 서식).
//  - 표 대조: squid-name-vtables.js 83칸을 cpppj `SquidNameVtables.inc` 와 행 단위로 비교한다(파일이 없으면 건너뜀).
// 원본 PE 관측 fixture 는 이 파일에 없다(cpppj 도 계약 검사만 둔다). 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E58: 객체 이름 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawSquidName, squidNameEntry, squidLevelName, formatSuffix } from '../www/core/raw-squid-name.js';
import { SQUID_NAME_ENTRIES } from '../www/core/squid-name-vtables.js';

// 예외가 나는지 본다. (2026-11-08 WB04-E58)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 이름 장면이다: 부모 SID 하나(가상 표 0x501dd0·TYPE 85)·188칸 설명 표·언어/번역/치환/레벨 장치와 호출 추적이다. (2026-11-08 WB04-E58)
function nameScene() {
    const pool = new SidPool(32768, { server: true });
    const types = Array.from({ length: 188 }, () => ({ description: '' }));
    const parent = pool.allocate(2);
    const raw = pool.allocated(parent);
    raw.fill(0);
    put(raw, 0, 0x501dd0);
    raw[10] = 85;
    types[85].description = 'raw';
    const scene = { pool, types, parent, language: 1, trace: [] };
    scene.hooks = {
        language: () => { scene.trace.push('language'); return scene.language; },
        translate: (text) => { scene.trace.push(`translate:${text}`); return text === ' Level %s' ? text : `translated:${text}`; },
        substitute: (text) => { scene.trace.push('substitute'); return `cfg:${text}`; },
        level: (sid) => { assert.equal(sid, parent); scene.trace.push('level'); return pool.slot(sid)[11] >> 5; }
    };
    scene.make = () => new RawSquidName(pool, types, scene.hooks);
    return scene;
}

test('SquidName: 언어 0은 한 번, 1은 두 번 조회하고 다른 언어일 때만 설명 번역을 선택한다', () => {
    const scene = nameScene();
    scene.language = 0;
    assert.equal(scene.make().name(scene.parent), 'cfg:raw');
    assert.deepEqual(scene.trace, ['language', 'substitute']);
    scene.trace.length = 0;
    scene.language = 1;
    assert.equal(scene.make().name(scene.parent), 'cfg:raw');
    assert.deepEqual(scene.trace, ['language', 'language', 'substitute']);
    scene.trace.length = 0;
    scene.language = 2;
    assert.equal(scene.make().name(scene.parent), 'cfg:translated:raw');
    assert.deepEqual(scene.trace, ['language', 'language', 'translate:raw', 'substitute']);
});

test('SquidName: Factory 는 base 치환 뒤 패턴을 항상 번역하고 그 효과 뒤 현재 레벨을 읽는다', () => {
    const scene = nameScene();
    put(scene.pool.allocated(scene.parent), 0, 0x509148);
    scene.hooks.translate = (text) => {
        assert.equal(text, ' Level %s');
        scene.trace.push('pattern');
        scene.pool.allocated(scene.parent)[11] = 64;
        return text;
    };
    assert.equal(scene.make().name(scene.parent), 'cfg:raw Level III');
    assert.deepEqual(scene.trace, ['language', 'language', 'substitute', 'pattern', 'level']);
});

test('SquidName: TYPE 는 언어 조회 전에 선택하지만 설명 바이트는 그 효과 뒤 현재 값이며 NUL·버퍼·누락 오류를 분리한다', () => {
    const scene = nameScene();
    scene.hooks.language = () => {
        scene.types[85].description = 'current\0ignored';
        scene.pool.allocated(scene.parent)[10] = 86;
        return 0;
    };
    scene.types[86].description = 'other';
    assert.equal(scene.make().name(scene.parent), 'cfg:current');
    scene.hooks.substitute = () => 'x'.repeat(80);
    assert.ok(throws(() => scene.make().name(scene.parent)));
    put(scene.pool.allocated(scene.parent), 0, 0);
    scene.hooks = {};
    assert.ok(throws(() => scene.make().name(scene.parent)));
    assert.ok(throws(() => new RawSquidName(scene.pool, scene.types.slice(0, 187), {})));
});

test('SquidName: 83개 가상 표, 로마숫자와 0xff sentinel, CRT 서식(%%·폭·%s)과 256바이트 경계를 보존한다', () => {
    assert.equal(squidNameEntry(0x501dd0), 0x4ace20);
    assert.equal(squidNameEntry(0x509148), 0x454d30);
    assert.equal(squidNameEntry(0x509210), 0x454d30);
    assert.ok(throws(() => squidNameEntry(0)));
    assert.equal(squidLevelName(0), 'I');
    assert.equal(squidLevelName(9), 'X');
    assert.equal(squidLevelName(10), String.fromCharCode(0xff));
    const scene = nameScene();
    put(scene.pool.allocated(scene.parent), 0, 0x509148);
    scene.hooks.translate = () => ' %% [%4s]';
    assert.equal(scene.make().name(scene.parent), 'cfg:raw % [   I]');
    scene.hooks.translate = () => 'x'.repeat(256);
    assert.ok(throws(() => scene.make().name(scene.parent)));
    assert.equal(formatSuffix('[%-4s|%.1s]', 'IV'), '[IV  |I]');
    assert.ok(throws(() => formatSuffix('%d', 'I')));
});

test('SquidName: 가상 표 83칸이 cpppj SquidNameVtables.inc 와 행 단위로 같다', () => {
    const file = path.join(REPO_ROOT, 'cpppj', 'src', 'o', 'SquidNameVtables.inc');
    if (!fs.existsSync(file)) return;
    // 한 줄 `{0x…U,0x…U},` 형태의 행만 읽는다.
    const rows = fs.readFileSync(file, 'utf8').split(/\r?\n/)
        .map((line) => /^\s*\{(0x[0-9a-fA-F]+)U,(0x[0-9a-fA-F]+)U\},?\s*$/.exec(line))
        .filter(Boolean)
        .map((match) => [Number.parseInt(match[1], 16), Number.parseInt(match[2], 16)]);
    assert.equal(rows.length, 83);
    assert.deepEqual(SQUID_NAME_ENTRIES.map((row) => [...row]), rows);
});
