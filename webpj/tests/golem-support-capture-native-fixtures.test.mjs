// 골렘(TYPE 133, 가상 표 0x50e568)과 받침(TYPE 94 윗면·95 하단)의 ShapeToBuffer 캡처를 원본 10.78 관측(cpppj `golem-capture-x86.tsv` 66행·`support-color-capture-x86.tsv` 198행)과 대조한다.
// altar-capture 시험(E1276)과 같은 방식이다: 원문 입력(프레임·소유자·색표·추가 비트)을 raw 슬롯에 그대로 두고 웹 RawShapeToBuffer 가 같은 메타데이터 16바이트와 전체 픽셀을 내는지 본다.
//  - 골렘: 소유자 0·extra 128·TYPE 기준점 (8,2)·단위 배율. 프레임 0..65 66행 모두(cpppj 게임 월드 시험은 보행 64프레임만 쓰지만 캡처 자체는 66행 전부 원본이다).
//  - 받침: TYPE 기준점 (-2,3)·소유자별 현재 색 전역(`ownerColors[owner]`)이 소유자 번호와 다른 경우 포함. 행 열은 [TYPE, 프레임, 소유자, 현재 색 번호, 메타데이터, 픽셀].
//    색표는 웹이 미션에서 쓰는 그대로이다: 윗면 94 는 명도 전용 표(`buildSpecialOwnerColorMaps`, 실제 게임 팔레트 GIFCLOUD.COL 의 명도 기반 '독립 HSL 색표'), 하단 95 는 일반 띠 표(`buildStandardOwnerColorMaps`).
//    두 가설을 바꿔 보면(94 를 일반 띠·95 를 명도 전용) 각 타입 99행 중 일부만 맞아 어느 표가 맞는지 이 시험이 가른다.
// 기대 픽셀은 제품의 색 계산으로 만들지 않고 원본 관측만 쓴다. (2026-11-24 WB04-E1312: 5-A 'golem-capture·support-color-capture' 대조)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { buildSpecialOwnerColorMaps, buildStandardOwnerColorMaps } from '../www/assets/owner-colors.js';
import { GamePalette } from '../www/assets/palette.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';
import { put } from './raw-support.mjs';
import { clientPool, hexOrDash as hex, realCaptureAssets } from './capture-assets-support.mjs';
import { readData } from './helpers.mjs';

// 원본 fixture 의 데이터 행(주석·빈 줄 제외)을 탭으로 나눠 읽는다.
function rows(name) {
    return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t'));
}

// 메타데이터 16바이트(폭·높이·기준점 x·y 의 리틀 엔디언 int32)를 만든다.
function metadataBytes(image) {
    const metadata = Buffer.alloc(16);
    [image.width, image.height, image.hotspotX, image.hotspotY].forEach((value, position) => metadata.writeInt32LE(value, position * 4));
    return metadata;
}

test('golem-capture 원본 66행: 골렘(TYPE 133) 프레임 0..65 캡처의 메타데이터·전체 픽셀이 같다', () => {
    const data = rows('golem-capture-x86.tsv');
    assert.equal(data.length, 66);
    const GOLEM = 133, VTABLE = 0x50e568, assets = realCaptureAssets(), types = assets.types.slice();
    types[GOLEM] = { ...types[GOLEM], hotspotX: 8, hotspotY: 2 };
    const pool = clientPool(), failures = []; let count = 0;
    for (const [index, row] of data.entries()) {
        assert.equal(row.length, 5, `행 ${index}`);
        const frame = Number(row[0]);
        const raw = pool.allocated(50); raw.fill(0); put(raw, 0, VTABLE); raw[10] = GOLEM; raw[34] = 0; raw[40] = 128; put(raw, 36, frame);
        const state = createShapeToBufferState({ zoom: 65536, addTypeHotspot: true, ownerTypeMaps: new Uint32Array(188), ownerColors: () => { throw new Error('중립 골렘은 색표를 부르지 않는다'); }, mainFrame: () => frame, additional: () => {} });
        const before = Uint8Array.from(raw);
        let image;
        try { image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50); } catch (error) { failures.push(`${index}: ${error.message}`); continue; }
        const [width, height, hotX, hotY] = row[1].split(',').map(Number);
        if (Buffer.compare(metadataBytes(image), Buffer.from(Int32Array.from([width, height, hotX, hotY]).buffer)) !== 0 || hex(image.pixels) !== row[2] || Buffer.compare(Buffer.from(before), Buffer.from(raw)) !== 0) failures.push(`${index} frame${frame}`);
        else count += 1;
    }
    assert.deepEqual(failures.slice(0, 12), []);
    assert.equal(count, 66);
});

test('support-color-capture 원본 198행: 받침 윗면(94)·하단(95)의 소유자·현재 색 번호별 캡처가 같다', () => {
    const data = rows('support-color-capture-x86.tsv');
    assert.equal(data.length, 198);
    const assets = realCaptureAssets(), types = assets.types.slice();
    for (const type of [94, 95]) types[type] = { ...types[type], hotspotX: -2, hotspotY: 3 };
    const pool = clientPool(), failures = [], logical = GamePalette.parse(readData('d/GIFCLOUD.COL')).logical; let count = 0;
    // 타입별 소유자 색표 9행이다: 94 는 명도 전용, 95 는 일반 띠 표.
    const colorMaps = { 94: buildSpecialOwnerColorMaps(logical, 94), 95: buildStandardOwnerColorMaps(95, 82) };
    for (const [index, row] of data.entries()) {
        assert.equal(row.length, 6, `행 ${index}`);
        const [type, frame, owner, color] = [Number(row[0]), Number(row[1]), Number(row[2]), Number(row[3])];
        const raw = pool.allocated(50); raw.fill(0); put(raw, 0, 0x5076b8); raw[10] = type; raw[34] = owner; raw[40] = 128; put(raw, 36, frame);
        const maps = colorMaps[type], tokens = new Uint32Array(188); tokens[type] = 1;
        const state = createShapeToBufferState({ zoom: 65536, addTypeHotspot: true, ownerTypeMaps: tokens, ownerColors: (requested, who) => { assert.deepEqual([requested, who], [type, owner]); return maps[color]; }, mainFrame: () => frame, additional: () => {} });
        let image;
        try { image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50); } catch (error) { failures.push(`${index}: ${error.message}`); continue; }
        const [width, height, hotX, hotY] = row[4].split(',').map(Number);
        if (Buffer.compare(metadataBytes(image), Buffer.from(Int32Array.from([width, height, hotX, hotY]).buffer)) !== 0 || hex(image.pixels) !== row[5]) failures.push(`${index} type${type} frame${frame} owner${owner} color${color}`);
        else count += 1;
    }
    assert.deepEqual(failures.slice(0, 12), []);
    assert.equal(count, 198);
});
