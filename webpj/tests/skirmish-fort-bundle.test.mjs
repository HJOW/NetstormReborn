// 12차 스커미시 시작 섬 후보 fort 12개가 웹 자료 묶음('skirmish')에 원본 그대로 들어 있고 후보 표와 맞는지 검사한다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1103)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { SKIRMISH_START_ISLANDS, makeSkirmishEmptyWorldFort } from '../www/core/skirmish-setup.js';
import { DATA_ROOT, WEBPJ_ROOT } from './helpers.mjs';

const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const entries = manifest.files.filter((entry) => entry.bundle === 'skirmish');
const REPO_ROOT = path.resolve(WEBPJ_ROOT, '..');

test('스커미시 묶음: 후보 표의 fort 이름 12개가 빠짐없이 한 번씩 들어 있다', () => {
    const wanted = [...new Set(SKIRMISH_START_ISLANDS.map((island) => island.fort))].sort();
    assert.equal(wanted.length, 12);
    assert.deepEqual(entries.map((entry) => entry.path).sort(), wanted.map((fort) => `d/${fort}.fort`).sort());
    assert.ok(Object.hasOwn(manifest.bundles, 'skirmish'));
    // 메인 메뉴 필수 묶음에는 섞이지 않는다(시작할 때만 받는다).
    assert.ok(!manifest.files.some((entry) => entry.bundle === 'menu' && entry.path.endsWith('.fort')));
});

test('스커미시 묶음: 복사본은 공용 원본과 바이트·크기·SHA-256 이 같고 서명과 35개 섹션이 맞다', () => {
    for (const entry of entries) {
        const copy = fs.readFileSync(path.join(DATA_ROOT, ...entry.path.split('/'))), original = fs.readFileSync(path.join(REPO_ROOT, ...entry.source.split('/')));
        assert.ok(copy.equals(original), entry.path); assert.equal(entry.size, copy.length, entry.path);
        assert.equal(entry.sha256, crypto.createHash('sha256').update(copy).digest('hex'), entry.path);
        assert.equal(copy[0], 0x46, `${entry.path} 서명 F`);
        // 빈 월드 fort 를 만들 수 있다(섹션 35개가 길이 표대로 끝까지 읽힌다).
        const world = makeSkirmishEmptyWorldFort(new Uint8Array(copy));
        assert.ok(world.length > 0 && world.length < copy.length, `${entry.path} 빈 월드`);
    }
});

test('스커미시 묶음: 후보 표의 영역 번호가 각 fort 의 영역 표(120바이트 = 20×6)에서 비어 있지 않은 기록이다', () => {
    for (const island of SKIRMISH_START_ISLANDS) {
        assert.ok(island.territory >= 0 && island.territory < 20, `${island.fort} 영역 번호`);
        const bytes = new Uint8Array(fs.readFileSync(path.join(DATA_ROOT, 'd', `${island.fort}.fort`)));
        // 35개 섹션을 읽어 영역 표(6번째, 0 시작)를 찾는다.
        let position = 2;
        const sections = [];
        while (sections.length < 35) { const length = bytes[position] | (bytes[position + 1] << 8); sections.push(bytes.subarray(position + 2, position + length)); position += length; }
        assert.equal(sections[6].length, 120, `${island.fort} 영역 표 크기`);
        const record = sections[6].subarray(island.territory * 6, island.territory * 6 + 6);
        assert.ok(record.some((byte) => byte !== 0), `${island.fort} 영역 ${island.territory} 기록이 비어 있음`);
        assert.ok((record[0] & 63) < 68, `${island.fort} 영역 ${island.territory} 패턴 번호`);
    }
});
