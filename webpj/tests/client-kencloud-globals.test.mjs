// 구름 전역 소유자 www/core/client-kencloud-globals.js 를 cpppj `ClientKencloudGlobalsTests.cpp` 8건과 웹 계약으로 검사한다.
//  - cpppj 이식: PE 최초값·인접 별칭·주소 수명, 실제 512 GIF 와 seed 400 난수(0x89abcdef → 4201191824, 간격 110 — cpppj 가 원본 x86 관측 v2 행으로 확정한 값), 없는 파일/512 아닌 GIF fallback,
//    fury 0 의 부분 효과, 설정 옵션 해석(Client 의 즉시 callback 부분은 Client 본체 몫이라 readOptions 직접 호출로 대신), 팔레트 전환의 장치 순서·같은 이름 자연 반환, 갱신/dirty,
//    느슨한 파일 우선·잘린 GIF 오류 전파(cpppj 의 PathSpec 이름 고리 10칸 색인은 웹 설정에 없는 host 구조라 제외).
//  - 실제 자료: originals 의 netstorm.tarc·d/setup.cfg·d/Gifcloud.gif·d/Defeat.gif 를 읽기만 한다(없으면 해당 검사를 건너뛴다).
//  - 웹 계약: 배경 GIF 치수/정규화 해독, 주소 산술·소진 경계, 기억 조회 오류, 옵션 상자 별칭, 실제 Screen 출력(타일 위치·부분 행·parallax 거부), 대소문자 무시 비교 경계, Renderer 토큰.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E396)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { GameFiles } from '../www/assets/game-files.js';
import { decodeBackgroundGif, decodeGif, gifFileDimensions } from '../www/assets/gif.js';
import { ClientRandomGlobals, GameRandom } from '../www/core/random.js';
import { KencloudDomainError } from '../www/core/raw-kencloud.js';
import { ClientKencloudGlobals, KencloudOption } from '../www/core/client-kencloud-globals.js';
import { Renderer } from '../www/render/renderer.js';
import { Screen, ScreenMode } from '../www/render/screen.js';

// 원본 보호 자료 폴더와 존재 여부이다(읽기만 한다). (2026-11-13 WB04-E396)
const ORIGINALS = path.join(REPO_ROOT, 'originals');
const present = ['netstorm.tarc', 'd/setup.cfg', 'd/Gifcloud.gif', 'd/Defeat.gif'].every((name) => fs.existsSync(path.join(ORIGINALS, name)));
// cpppj 시험의 난수 시작 상태와 seed 400·파일 경로 생성 뒤의 기대 난수 상태·간격이다. (2026-11-13 WB04-E396)
const SEED_STATE = 0x89abcdef;
const AFTER_STATE = 4201191824;
const AFTER_SPACER = 110;
// 512×512 캐시 바이트 수와 첫 host 주소 토큰이다. (2026-11-13 WB04-E396)
const CACHE_BYTES = 262144;
const FIRST = 0x36000000;

// 원본 자료 한 파일을 읽는다. (2026-11-13 WB04-E396)
const original = (name) => fs.readFileSync(path.join(ORIGINALS, name));
// 동작이 던진 오류의 문구이다(던지지 않으면 빈 문자열). (2026-11-13 WB04-E396)
const messageOf = (action) => { try { action(); } catch (error) { return error.message; } return ''; };
// 문자열을 NUL 종료 바이트로 만든다. (2026-11-13 WB04-E396)
const cbytes = (text) => Uint8Array.from([...text].map((character) => character.charCodeAt(0)).concat(0));

// 원본 setup.cfg·아카이브와 구름 두 GIF 를 느슨한 파일로 둔 게임 데이터이다. name 이 있으면 cpppj 시험처럼 CloudFileSpec 을 그 경로로 바꾼다. (2026-11-13 WB04-E396)
function cloudData(name = null) {
    const files = new Map([[ARCHIVE_PATH, original('netstorm.tarc')], [SETUP_PATH, original('d/setup.cfg')], ['d/Gifcloud.gif', original('d/Gifcloud.gif')], ['d/Defeat.gif', original('d/Defeat.gif')]]);
    const data = GameData.build({ files });
    if (name !== null) data.set('CloudFileSpec', name);
    return data;
}

// 한 색 GIF89a 를 만든다(전역 팔레트 2색·최소 코드 크기 8·250 코드마다 clear 로 9비트 유지). 논리 화면·이미지 위치·앞 확장 블록·LZW 생략을 고를 수 있다. (2026-11-13 WB04-E396)
function makeGif(width, height, color, { screenWidth = width, screenHeight = height, left = 0, top = 0, extension = false, emptyData = false } = {}) {
    const codes = [256];
    // 픽셀마다 리터럴 코드를 넣고 250 개마다 사전을 비운다(빈 데이터 요청이면 픽셀을 넣지 않는다).
    for (let index = 0; !emptyData && index < width * height; index += 1) {
        if (index > 0 && index % 250 === 0) codes.push(256);
        codes.push(color);
    }
    codes.push(257);
    const packed = [];
    let accumulator = 0;
    let bits = 0;
    // 9비트 코드를 낮은 비트부터 바이트로 채운다.
    for (const code of codes) {
        accumulator |= code << bits;
        bits += 9;
        // 찬 바이트를 내보낸다.
        while (bits >= 8) {
            packed.push(accumulator & 0xff);
            accumulator >>>= 8;
            bits -= 8;
        }
    }
    if (bits > 0) packed.push(accumulator & 0xff);
    // 16비트 값을 리틀 엔디언 두 바이트로 나눈다.
    const word = (value) => [value & 0xff, (value >> 8) & 0xff];
    const bytes = [0x47, 0x49, 0x46, 0x38, 0x39, 0x61, ...word(screenWidth), ...word(screenHeight), 0x80, 0, 0, 0, 0, 0, 255, 255, 255];
    if (extension) bytes.push(0x21, 0xf9, 4, 0, 0, 0, 0, 0);
    bytes.push(0x2c, ...word(left), ...word(top), ...word(width), ...word(height), 0, 8);
    // 255 바이트 이하 서브 블록으로 나눈다.
    for (let offset = 0; offset < packed.length; offset += 255) {
        const part = packed.slice(offset, offset + 255);
        bytes.push(part.length, ...part);
    }
    bytes.push(0, 0x3b);
    return Uint8Array.from(bytes);
}

// 모드가 정해진 640×480 창 DIB 화면이다(render/screen.js, 표시 훅은 기록하지 않음). (2026-11-13 WB04-E396)
function makeScreen() {
    const screen = new Screen(640, 480, { devices: { present: () => {}, presentFullScreen: () => {}, paletteChanged: () => {}, waitVerticalBlank: () => {}, cursor: () => {} } });
    screen.init();
    screen.initDibSection();
    assert.equal(screen.setMode(ScreenMode.fallbackWindowed), true);
    return screen;
}

// 화면 DIB 의 현재 픽셀 사본과 줄 간격이다. (2026-11-13 WB04-E396)
function screenPixels(screen) {
    const pixels = screen.lock().slice();
    screen.unlock();
    return { pixels, pitch: screen.pitch };
}

test('ClientKencloudGlobals_PhysicalGlobalsAndAddressLifetime: PE 최초값·인접 별칭·살아 있는 블록/내부/one-past·반납 뒤 거부', () => {
    const cloud = new ClientKencloudGlobals((bytes) => bytes.fill(0x71));
    const globals = cloud.globals();
    assert.equal(cloud.state().length, 2054);
    assert.deepEqual([globals.cache, globals.phase, globals.lowColor, globals.speed, globals.fury, globals.seedCount, globals.spacer, cloud.allocations()], [0, 0, 228, 2, 10, 400, 32, 0]);
    assert.deepEqual([cloud.phaseRows()[1], cloud.phaseRows()[15], cloud.cloudName().length], [8, 13, 104]);
    // 보호 PE 00540a98 의 고정 16행 순서 전체이다(원본 x86 구름 관측 시험의 행 표와 같다).
    assert.deepEqual(Array.from(cloud.phaseRows()), [0, 8, 4, 12, 1, 11, 5, 15, 3, 10, 2, 7, 14, 9, 6, 13]);
    assert.deepEqual([0, 1, 2, 3, 4, 5, 6, 7].map((option) => cloud.value(option).value), [72, 2, 10, 32, 99999, 400, 228, 245]);
    // 이름/표는 같은 인접 물리를 빌리며 복사하지 않는다.
    cloud.cloudName()[84] = 7;
    assert.equal(cloud.state()[2049], 7);
    globals.dirtyRows[3] = 29;
    assert.equal(cloud.state()[519], 29);
    globals.seeds[999] = 31;
    assert.equal(cloud.state()[2027], 31);
    const first = cloud.allocate(36);
    const second = cloud.allocate(262144);
    assert.ok(second > first + 36);
    assert.deepEqual([first, second], [FIRST, FIRST + 0x2000]);
    assert.deepEqual([cloud.memory(first).length, cloud.memory(first + 35).length, cloud.memory(first + 36).length], [36, 1, 0]);
    assert.equal(cloud.memory(second)[0], 0x71);
    cloud.memory(second + 512)[0] = 19;
    assert.equal(cloud.memory(second)[512], 19);
    cloud.release(first);
    assert.throws(() => cloud.memory(first), RangeError);
    assert.equal(cloud.allocations(), 1);
    cloud.release(0);
    cloud.release(second);
    assert.equal(cloud.allocations(), 0);
});

test('ClientKencloudGlobals_ActualFileAndNative400SeedRandom: 실제 512 GIF 픽셀 전체·경계·같은 난수와 캐시 재조회 자연 반환', { skip: !present }, () => {
    // 원본 setup.cfg 의 CloudFileSpec(\D\gifcloud.gif)을 그대로 쓴다.
    const data = cloudData();
    const random = new GameRandom(0);
    random.setState(SEED_STATE);
    const pixels = decodeBackgroundGif(original('d/Gifcloud.gif'));
    assert.deepEqual([pixels.width, pixels.height], [512, 512]);
    const sizes = [];
    const cloud = new ClientKencloudGlobals((bytes) => { sizes.push(bytes.length); bytes.fill(0x5a); });
    cloud.build(data.files, data, random);
    const globals = cloud.globals();
    assert.deepEqual(sizes, [264, CACHE_BYTES]);
    assert.equal(globals.cache, FIRST);
    assert.deepEqual([globals.end, globals.nextRow, globals.lastRow, cloud.allocations()], [FIRST + CACHE_BYTES, FIRST + 512, FIRST + CACHE_BYTES - 512, 1]);
    assert.ok(Buffer.from(cloud.memory(globals.cache)).equals(Buffer.from(pixels.indices)));
    assert.deepEqual([random.state, globals.spacer, globals.allDirty, globals.partialDirty], [AFTER_STATE, AFTER_SPACER, 1, 0]);
    // 같은 캐시의 원문 재조회/행 이동은 별도 난수를 쓰지 않는다.
    const token = globals.cache;
    const before = random.state;
    const byte = cloud.memory(token)[37];
    cloud.build(data.files, data, random);
    assert.deepEqual([globals.cache, random.state, cloud.allocations(), cloud.memory(token)[37]], [token, before, 1, byte]);
    cloud.step();
    assert.deepEqual([globals.phase, random.state, globals.dirtyRows[8]], [1, before, 1]);
});

test('ClientKencloudGlobals_MissingAndWrongSizeUseActualFallback: 없는 파일과 512 가 아닌 실제 GIF 는 반납 뒤 같은 난수/채움 fallback 을 쓴다', { skip: !present }, () => {
    const defeat = gifFileDimensions(original('d/Defeat.gif'));
    const decoded = decodeBackgroundGif(original('d/Defeat.gif'));
    assert.deepEqual([decoded.width, decoded.height], [defeat.width, defeat.height]);
    assert.notDeepEqual([defeat.width, defeat.height], [512, 512]);
    const defeatBytes = defeat.width * defeat.height;
    // 존재 실패와 다른 치수의 파일을 각각 부모가 처리한다.
    for (const [name, expectedSizes, cache] of [
        ['d/no-kencloud-424.gif', [264, CACHE_BYTES], FIRST],
        ['d/Defeat.gif', [264, defeatBytes, CACHE_BYTES], FIRST + Math.ceil(defeatBytes / 0x1000) * 0x1000 + 0x1000],
    ]) {
        const data = cloudData(name);
        const sizes = [];
        const cloud = new ClientKencloudGlobals((bytes) => sizes.push(bytes.length));
        const random = new GameRandom(0);
        random.setState(SEED_STATE);
        cloud.build(data.files, data, random);
        const globals = cloud.globals();
        assert.deepEqual(sizes, expectedSizes, name);
        assert.deepEqual([cloud.allocations(), globals.cache, globals.end, random.state, globals.spacer], [1, cache, cache + CACHE_BYTES, AFTER_STATE, AFTER_SPACER], name);
        assert.ok(cloud.memory(globals.cache).every((value) => value === 228), name);
    }
});

test('ClientKencloudGlobals_FallbackDivideFaultPreservesPrefix: fury 0 은 할당/seed 효과를 남기고 정상 dirty 재설정에 도달하지 않는다', { skip: !present }, () => {
    const data = cloudData('d/no-kencloud-424.gif');
    const cloud = new ClientKencloudGlobals();
    cloud.value(KencloudOption.fury).value = 0;
    const random = new GameRandom(0);
    random.setState(SEED_STATE);
    assert.throws(() => cloud.build(data.files, data, random), KencloudDomainError);
    const globals = cloud.globals();
    assert.deepEqual([globals.cache, cloud.allocations(), globals.end, globals.allDirty, random.state], [FIRST, 1, FIRST + CACHE_BYTES, 0, AFTER_STATE]);
});

test('ClientKencloudGlobals_ActualConfigOptions: 설정 옵션은 signed DWORD 로 적용되고 누락 키·다른 소유자 상태·난수/파일은 그대로이다', { skip: !present }, () => {
    const randoms = new ClientRandomGlobals();
    const cloud = new ClientKencloudGlobals();
    assert.deepEqual([randoms.game.state, cloud.allocations()], [55, 0]);
    const data = cloudData();
    data.setInt('cloudSpeed', -1);
    data.setInt('cloudFury', -17);
    data.setInt('cloudiness', 13);
    data.setInt('cloudAlgo', 3);
    data.setInt('kenCloud', 1);
    data.setInt('readOnlyForts', 1);
    cloud.readOptions(data);
    assert.deepEqual([cloud.value(KencloudOption.speed).value, cloud.value(KencloudOption.fury).value, cloud.value(KencloudOption.cloudiness).value, cloud.state()[2050], cloud.state()[2049]], [0xffffffff, 0xffffffef, 13, 3, 1]);
    // setup.cfg 에 없는 cloudSpacer/cloudRecenter 와 읽지 않는 seed 수·색은 최초값 그대로이다.
    assert.deepEqual([KencloudOption.spacer, KencloudOption.recenter, KencloudOption.seedCount, KencloudOption.lowColor, KencloudOption.highColor].map((option) => cloud.value(option).value), [32, 99999, 400, 228, 245]);
    assert.deepEqual([cloud.allocations(), randoms.game.state], [0, 55]);
    // 같은 값이면 저장하지 않고, 남은 다섯 키도 같은 규칙이다.
    data.setInt('cloudSpacer', 77);
    data.setInt('cloudRecenter', -5);
    cloud.value(KencloudOption.speed).value = 9;
    data.setInt('cloudSpeed', 9);
    cloud.readOptions(data);
    assert.deepEqual([cloud.value(KencloudOption.spacer).value, cloud.value(KencloudOption.recenter).value, cloud.value(KencloudOption.speed).value], [77, 0xfffffffb, 9]);
});

test('ClientKencloudGlobals_ActualTransitionUsesSameGlobalsAndDeviceOrder: 실제 파일/원문 부모·장치 변화 재조회와 같은 이름 자연 반환', { skip: !present }, () => {
    const data = cloudData('d/Gifcloud.gif');
    data.set('GamePalSpec', '$1.COL');
    const cloud = new ClientKencloudGlobals();
    cloud.value(KencloudOption.speed).value = 2;
    const random = new ClientRandomGlobals().game;
    let trace = '';
    let loads = 0;
    let maps = 0;
    const paths = [];
    const name = cbytes('Gifcloud');
    const renderer = new Renderer(640, 480);
    cloud.bindRenderer(renderer);
    const effects = {
        log: (level, text) => { assert.deepEqual([level, text], [29, 'init palette\n']); trace += 'L'; },
        loadPalette: (file, clear) => { assert.equal(clear, 1); paths.push(file); loads += 1; trace += 'P'; },
        buildMaps: () => { maps += 1; trace += 'M'; cloud.bindRenderer(null); },
    };
    cloud.transition(name, data.files, data, random, effects);
    assert.deepEqual([trace, loads, maps, cloud.globals().cache, cloud.globals().phase, cloud.worldRedraw().value], ['LPM', 1, 1, FIRST, 2, 0]);
    assert.deepEqual(Array.from(paths[0]), Array.from(cbytes(data.pathSpec('GamePalSpec', 'Gifcloud'))));
    assert.deepEqual(Array.from(cloud.paletteName().subarray(0, 9)), Array.from(name));
    assert.deepEqual(Array.from(cloud.cloudName().subarray(0, 9)), Array.from(name));
    // 호출자 장치 묶음은 바꾸지 않는다.
    assert.deepEqual(Object.keys(effects), ['log', 'loadPalette', 'buildMaps']);
    // 같은 이름(대소문자 무시)은 실제 cache/Screen/maps/dirty 를 건드리지 않는다.
    const token = cloud.globals().cache;
    const state = random.state;
    name[0] = 0x67;
    trace = '';
    cloud.transition(name, data.files, data, random, effects);
    assert.deepEqual([trace, loads, maps, cloud.globals().cache, random.state, cloud.allocations()], ['L', 1, 1, token, state, 1]);
    // 다른 이름이면 이전 캐시를 반납하고 새로 만들며, Renderer 가 남아 있으면 같은 0059a8b0 을 1 로 둔다.
    const kept = new ClientKencloudGlobals();
    kept.bindRenderer(renderer);
    kept.transition(cbytes('Gifcloud'), data.files, data, random, { ...effects, buildMaps: () => { trace += 'M'; } });
    const firstCache = kept.globals().cache;
    kept.transition(cbytes('Gifcloud2'), data.files, data, random, { ...effects, buildMaps: () => {} });
    assert.deepEqual([kept.worldRedraw().value, kept.paletteGlobals().dirty === kept.worldRedraw(), kept.allocations(), kept.globals().cache], [1, true, 1, firstCache + CACHE_BYTES + 0x1000]);
    assert.throws(() => kept.memory(firstCache), RangeError);
});

test('ClientKencloudGlobals_ActualUpdateAndDirtyUseSameMemory: 갱신 부모는 한 번만 생성하고 현재/이전 행을 같은 물리에서 정리한다', { skip: !present }, () => {
    const data = cloudData('d/Gifcloud.gif');
    const cloud = new ClientKencloudGlobals();
    const random = new GameRandom(0);
    random.setState(SEED_STATE);
    cloud.update(data.files, data, random);
    const globals = cloud.globals();
    const token = globals.cache;
    assert.deepEqual([token, globals.phase, random.state], [FIRST, 2, AFTER_STATE]);
    cloud.update(data.files, data, random);
    assert.deepEqual([globals.phase, globals.cache, random.state, cloud.allocations()], [4, token, AFTER_STATE, 1]);
    cloud.markDirty();
    assert.deepEqual([globals.allDirty, globals.partialDirty, cloud.rowDirty(0xffffffff, 0)], [1, 0, 1]);
    cloud.state()[8] = 1;
    globals.dirtyRows[8] = 1;
    globals.dirtyRows[13] = 1;
    cloud.cleanDirty();
    // 이전에 표시하지 않은 13행은 보존한다.
    assert.deepEqual([cloud.state()[8], globals.dirtyRows[8], globals.dirtyRows[13], globals.allDirty, globals.partialDirty], [0, 0, 1, 0, 0]);
    // 전체 요청이 꺼지면 행 조회는 현재 행 표만 본다(음수 합은 앞선 같은 물리 DWORD).
    assert.deepEqual([cloud.rowDirty(10, 3), cloud.rowDirty(10, 4), cloud.rowDirty(0xffffffff, 0)], [1, 0, 1]);
});

test('ClientKencloudGlobals_LooseFileBeforeArchiveAndPropagatesTruncation: 느슨한 파일이 아카이브보다 먼저이고 잘린 GIF 오류를 미발견으로 숨기지 않는다', { skip: !present }, () => {
    const data = cloudData('d/probe.gif');
    const bad = Uint8Array.from([0x62, 0x61, 0x64]);
    // 아카이브에는 잘린 내용만 있다.
    const archive = { read: (name) => (name === '\\d\\probe.gif' ? bad : null), find: () => [] };
    const cloud = new ClientKencloudGlobals();
    const random = new GameRandom(0);
    cloud.build(new GameFiles({ loose: new Map([['d/probe.gif', makeGif(512, 512, 7)]]), archives: [archive] }), data, random);
    const globals = cloud.globals();
    const token = globals.cache;
    assert.deepEqual([token, cloud.allocations(), cloud.memory(token).length], [FIRST, 1, CACHE_BYTES]);
    assert.ok(cloud.memory(token).every((value) => value === 7));
    cloud.release(token);
    globals.cache = 0;
    assert.throws(() => cloud.build(new GameFiles({ loose: new Map([['d/probe.gif', bad]]), archives: [archive] }), data, random), /GIF 치수 헤더의 실제 물리 범위/);
    assert.equal(cloud.allocations(), 0);
    data.set('CloudFileSpec', 'd/missing.gif');
    cloud.build(new GameFiles({ loose: new Map(), archives: [archive] }), data, random);
    assert.deepEqual([cloud.allocations(), globals.cache !== 0], [1, true]);
});

test('배경 GIF: 치수는 첫 descriptor 고정 위치에서 읽고, 해독은 그 치수로 논리 화면을 정규화하며 각 물리 경계를 거부한다', () => {
    // 논리 화면 8×8 안 (3,5) 위치의 4×2 이미지이다.
    const offset = makeGif(4, 2, 9, { screenWidth: 8, screenHeight: 8, left: 3, top: 5 });
    assert.deepEqual(gifFileDimensions(offset), { width: 4, height: 2 });
    const background = decodeBackgroundGif(offset);
    assert.deepEqual([background.width, background.height, Array.from(background.indices)], [4, 2, [9, 9, 9, 9, 9, 9, 9, 9]]);
    assert.equal(decodeGif(offset).width, 8);
    // 원본 바이트는 바꾸지 않는다.
    assert.deepEqual(Array.from(offset.subarray(6, 10)), [8, 0, 8, 0]);
    // 확장 블록이 앞에 있으면 치수는 고정 위치의 다른 바이트이고, 해독은 descriptor 표식이 없어 거부한다.
    const extended = makeGif(4, 2, 9, { extension: true });
    assert.deepEqual(gifFileDimensions(extended), { width: 0, height: 0x2c00 });
    assert.throws(() => decodeBackgroundGif(extended), /BackgroundGif first descriptor required/);
    // 전역 팔레트 크기(2^(k+1) 색 × 3)가 descriptor 위치를 정한다.
    const wide = makeGif(3, 1, 4);
    wide[10] = 0x81;
    const shifted = Uint8Array.from([...wide.subarray(0, 19), 1, 2, 3, 4, 5, 6, ...wide.subarray(19)]);
    assert.deepEqual(gifFileDimensions(shifted), { width: 3, height: 1 });
    assert.equal(decodeBackgroundGif(shifted).indices.length, 3);
    const noPalette = Uint8Array.from([...wide.subarray(0, 13), ...wide.subarray(19)]);
    noPalette[10] = 0x07;
    assert.deepEqual(gifFileDimensions(noPalette), { width: 3, height: 1 });
    // 헤더 13바이트, descriptor 뒤 9바이트(치수)·10바이트(해독) 경계이다.
    assert.throws(() => gifFileDimensions(offset.subarray(0, 12)), /GIF 치수 헤더/);
    assert.throws(() => decodeBackgroundGif(offset.subarray(0, 12)), /Truncated BackgroundGif header/);
    assert.deepEqual(gifFileDimensions(offset.subarray(0, 19 + 9)), { width: 4, height: 2 });
    assert.throws(() => gifFileDimensions(offset.subarray(0, 19 + 8)), /GIF 치수 descriptor/);
    assert.throws(() => decodeBackgroundGif(offset.subarray(0, 19 + 9)), /BackgroundGif first descriptor required/);
    // descriptor 10바이트가 있으면 관문은 통과하고 그 뒤 해독기가 잘린 파일을 거부한다.
    const truncated = messageOf(() => decodeBackgroundGif(offset.subarray(0, 19 + 10)));
    assert.match(truncated, /GIF/);
    assert.doesNotMatch(truncated, /first descriptor/);
    // 전역 팔레트가 파일보다 길면 두 함수 모두 거부한다.
    const tooShort = Uint8Array.from(offset.subarray(0, 13));
    tooShort[10] = 0x87;
    assert.throws(() => gifFileDimensions(tooShort), /GIF 치수 descriptor/);
    assert.throws(() => decodeBackgroundGif(tooShort), /BackgroundGif first descriptor required/);
});

test('구름 파일 부모: 경로·존재·해독 장치 계약, 크기 0/16MB 경계와 해독 실패 때 할당 보존, 경로 길이 한계', () => {
    const random = new GameRandom(0);
    const calls = [];
    // 경로 조회와 정수 읽기만 가진 설정이다.
    const config = (spec) => ({ pathSpec: (...args) => { calls.push(args); return spec; }, readInt: (key, current) => ({ changed: false, value: current }) });
    // 존재/읽기를 기록하는 파일 계층이다.
    const files = (bytes, exists = bytes !== null) => ({ has: (name) => { calls.push(['has', name]); return exists; }, read: (name) => { calls.push(['read', name]); return bytes; } });
    const zero = makeGif(0, 7, 1, { emptyData: true });
    const cloud = new ClientKencloudGlobals();
    assert.throws(() => cloud.build(files(zero), config('d/zero.gif'), random), /구름 GIF 실제 할당 폭/);
    assert.deepEqual(calls, [['CloudFileSpec'], ['has', 'd/zero.gif'], ['read', 'd/zero.gif']]);
    assert.equal(cloud.allocations(), 0);
    // 4097×4096 은 16MB 를 넘어 할당 전에 거부한다.
    assert.throws(() => cloud.build(files(makeGif(4097, 4096, 1, { emptyData: true })), config('d/huge.gif'), random), /구름 GIF 실제 할당 폭/);
    assert.equal(cloud.allocations(), 0);
    // 곱이 2^31 이상인 최대 치수도 uint32 크기로 보고 같은 문구로 거부한다.
    assert.throws(() => cloud.build(files(makeGif(65535, 65535, 1, { emptyData: true })), config('d/max.gif'), random), /구름 GIF 실제 할당 폭/);
    assert.equal(cloud.allocations(), 0);
    // 정확히 16MB 는 할당한 뒤 해독 실패(픽셀 수)로 끝나며 할당은 반납하지 않는다.
    assert.throws(() => cloud.build(files(makeGif(4096, 4096, 1, { emptyData: true })), config('d/exact.gif'), random), /GIF 픽셀 수/);
    assert.deepEqual([cloud.allocations(), cloud.memory(FIRST).length, cloud.globals().cache], [1, 16 * 1024 * 1024, 0]);
    // 존재한다고 했지만 읽을 수 없으면 원문 열기 진단(0xfb)이다.
    const opened = new ClientKencloudGlobals();
    assert.throws(() => opened.build(files(null, true), config('d/gone.gif'), random), /원문 진단 251/);
    assert.equal(opened.allocations(), 0);
    // 존재하지 않으면 해독하지 않고 fallback 이다.
    calls.length = 0;
    const missing = new ClientKencloudGlobals();
    missing.build(files(null), config('d/none.gif'), random);
    assert.deepEqual([calls.map((call) => call[0]), missing.allocations()], [['CloudFileSpec', 'has'], 1]);
    // 경로는 NUL 포함 256바이트까지만 지역 버퍼에 복사된다.
    const long = new ClientKencloudGlobals();
    assert.throws(() => long.build(files(null), config('x'.repeat(256)), random), /Kencloud file-path-write/);
    assert.equal(long.allocations(), 0);
    const fits = new ClientKencloudGlobals();
    calls.length = 0;
    fits.build(files(null), config('y'.repeat(255)), random);
    assert.deepEqual(calls[1], ['has', 'y'.repeat(255)]);
    // 지역 scratch 와 블록은 현재 할당 입력으로 채운다.
    const sizes = [];
    const filled = new ClientKencloudGlobals((bytes) => { sizes.push(bytes.length); bytes.fill(3); });
    filled.build(files(makeGif(512, 512, 5)), config('d/fill.gif'), random);
    assert.deepEqual([sizes, filled.memory(FIRST)[262143]], [[264, CACHE_BYTES], 5]);
});

test('구름 주소 공간: 블록 크기별 주소 간격·소진 경계, 조회/반납 오류와 생성자 입력 검사', () => {
    const cloud = new ClientKencloudGlobals();
    const tokens = [1, 0x1000, 0x1001, 262144, 1].map((size) => cloud.allocate(size));
    assert.deepEqual(tokens, [FIRST, FIRST + 0x2000, FIRST + 0x4000, FIRST + 0x7000, FIRST + 0x48000]);
    assert.equal(cloud.memory(FIRST + 0x4000 + 0x1001).length, 0);
    assert.throws(() => cloud.memory(FIRST + 0x4000 + 0x1002), /구름 현재 주소 물리 범위/);
    assert.throws(() => cloud.memory(FIRST - 1), /구름 현재 주소 물리 누락/);
    assert.throws(() => cloud.memory(-1), /구름 현재 주소 물리 범위/);
    // 가운데 블록을 반납하면 그 안의 주소는 앞 블록 범위 밖이다.
    cloud.release(FIRST + 0x2000);
    assert.throws(() => cloud.memory(FIRST + 0x2000), /구름 현재 주소 물리 범위/);
    assert.throws(() => cloud.release(FIRST + 0x2000), /구름 실제 반납 base 누락/);
    assert.throws(() => cloud.release(FIRST + 1), /구름 실제 반납 base 누락/);
    assert.equal(cloud.memory(FIRST + 0x4000).length, 0x1001);
    assert.throws(() => cloud.allocate(0), /구름 할당의 실제 물리 크기0/);
    assert.throws(() => cloud.allocate(2 ** 32), /구름 할당의 실제 물리 크기0/);
    assert.throws(() => cloud.allocate(-1), /구름 DWORD 주소 공간 소진/);
    // 현재 다음 base 는 FIRST + 0x4a000 이라 허용 최대 크기는 0xc9fb4000 이고, 한 바이트 더 크면 할당 전에 거부한다.
    assert.throws(() => cloud.allocate(0xc9fb4001), /구름 DWORD 주소 공간 소진/);
    assert.equal(cloud.allocations(), 4);
    // 1바이트 블록은 주소 0x2000 씩을 쓰며, 0xffffe000 을 넘는 다음 base 는 거부한다.
    const full = new ClientKencloudGlobals();
    let count = 0;
    let failure = '';
    // 주소 공간이 바닥날 때까지(기대 개수보다 한 번 더까지만) 1바이트 블록을 만든다.
    while (count <= 413695) {
        try { full.allocate(1); } catch (error) { failure = error.message; break; }
        count += 1;
    }
    assert.equal(count, 413695);
    assert.match(failure, /구름 DWORD 주소 공간 소진/);
    assert.equal(full.memory(0xffffc000).length, 1);
    // 0x80000000 이상의 토큰도 같은 base 로 반납되고, 반납 뒤 그 주소는 앞 블록 범위 밖이다.
    full.release(0xffffc000);
    assert.equal(full.allocations(), 413694);
    assert.throws(() => full.memory(0xffffc000), /구름 현재 주소 물리 범위/);
    // 생성자 입력: 함수가 아니면 거부하고 null 은 기본(새 할당의 0)이다.
    assert.throws(() => new ClientKencloudGlobals(5), TypeError);
    const plain = new ClientKencloudGlobals(null);
    assert.equal(plain.memory(plain.allocate(8)).every((value) => value === 0), true);
});

test('구름 옵션·전역 묶음 별칭: 옵션 상자·세 본체 묶음·팔레트 cache/dirty 가 같은 물리이고 잘못된 옵션 번호를 거부한다', () => {
    const cloud = new ClientKencloudGlobals();
    cloud.value(KencloudOption.speed).value = 5;
    assert.deepEqual([cloud.globals().speed, cloud.renderGlobals().speed, cloud.paletteGlobals().count()], [5, 5, 5]);
    assert.equal(cloud.renderGlobals().state, cloud.state());
    cloud.globals().spacer = 61;
    assert.equal(cloud.value(KencloudOption.spacer).value, 61);
    cloud.value(KencloudOption.lowColor).value = 0x1ff;
    cloud.value(KencloudOption.fury).value = -3;
    cloud.value(KencloudOption.seedCount).value = 17;
    assert.deepEqual([cloud.globals().lowColor, cloud.globals().fury, cloud.globals().seedCount], [0x1ff, 0xfffffffd, 17]);
    assert.throws(() => { cloud.globals().speed = 1; }, TypeError);
    const palette = cloud.paletteGlobals();
    palette.cache.value = 0x12345678;
    assert.deepEqual([cloud.globals().cache, cloud.state()[515]], [0x12345678, 0x12345678]);
    const globals = cloud.globals();
    // 각 DWORD 위치를 하나씩 확인한다.
    for (const [field, index] of [['cache', 515], ['end', 512], ['nextRow', 514], ['lastRow', 513], ['phase', 2053], ['allDirty', 2051], ['partialDirty', 2052]]) {
        globals[field] = 0x1000 + index;
        assert.equal(cloud.state()[index], 0x1000 + index, field);
        cloud.state()[index] = 0x2000 + index;
        assert.equal(globals[field], 0x2000 + index, field);
    }
    assert.equal(globals.dirtyRows.length, 512);
    assert.equal(globals.seeds.length, 1000);
    globals.dirtyRows[0] = 3;
    globals.dirtyRows[511] = 4;
    globals.seeds[0] = 5;
    assert.deepEqual([cloud.state()[516], cloud.state()[1027], cloud.state()[1028]], [3, 4, 5]);
    // 팔레트 이름은 구름 이름과 별개 물리이다.
    palette.paletteName[0] = 0x41;
    assert.deepEqual([cloud.paletteName()[0], cloud.cloudName()[0], cloud.paletteName().length], [0x41, 0, 128]);
    assert.equal(palette.cloudName, cloud.cloudName());
    palette.dirty.value = -1;
    assert.equal(cloud.worldRedraw().value, 0xffffffff);
    assert.equal(cloud.phaseRows(), cloud.globals().phaseRows);
    // 옵션 번호는 0..7 정수만 받는다.
    for (const option of [-1, 8, 1.5, '1', null]) assert.throws(() => cloud.value(option), /구름 옵션 번호 범위/);
    // 설정 정수 읽기는 현재 값을 signed 로 넘기고 원문 키 순서이다.
    const reads = [];
    cloud.value(KencloudOption.cloudiness).value = 0xffffffff;
    cloud.state()[2050] = 0x80000000;
    cloud.readOptions({ readInt: (key, current) => { reads.push([key, current]); return { changed: false, value: current }; } });
    assert.deepEqual(reads, [['cloudiness', -1], ['cloudSpeed', 5], ['cloudFury', -3], ['cloudSpacer', 61], ['cloudRecenter', 99999], ['cloudAlgo', -2147483648], ['kenCloud', 0]]);
});

test('구름 Screen 출력: 실제 화면 장치에 전체/부분 타일을 같은 캐시에서 복사하고 clip 을 복구하며 parallax 잠금은 거부한다', () => {
    const random = new GameRandom(0);
    const pattern = new Uint8Array(512 * 512);
    // 행·열이 모두 드러나는 서로 다른 색 번호이다.
    for (let index = 0; index < pattern.length; index += 1) pattern[index] = ((index >> 9) * 7 + (index & 511) * 3) & 0xff;
    const cloud = new ClientKencloudGlobals();
    const config = { pathSpec: () => 'd/cloud.gif', readInt: (key, current) => ({ changed: false, value: current }) };
    const files = { has: () => true, read: () => makeGif(512, 512, 1) };
    cloud.build(files, config, random);
    cloud.memory(cloud.globals().cache).set(pattern);
    const screen = makeScreen();
    // 출력 동안 바꾼 clip 은 원래 값(왼쪽/위가 0 이 아님)으로 복구되어야 한다.
    screen.setClip(5, 7, 600, 400);
    // 오프셋 (100, 38) 의 전체 출력이다(타일 좌표가 짝수가 되게 고름): 화면 (x, y) 는 캐시 ((y+38)%512, (x+100)%512) 이다.
    cloud.draw(screen, [0, 0, 640, 480, 100, 38], files, config, random);
    let { pixels, pitch } = screenPixels(screen);
    let mismatches = 0;
    // 모든 화면 픽셀을 독립 계산과 비교한다.
    for (let y = 0; y < 480; y += 1) for (let x = 0; x < 640; x += 1) if (pixels[y * pitch + x] !== pattern[((y + 38) % 512) * 512 + ((x + 100) % 512)]) mismatches += 1;
    assert.equal(mismatches, 0);
    assert.deepEqual(screen.clip(), { left: 5, top: 7, right: 600, bottom: 400 });
    assert.deepEqual([cloud.globals().partialDirty, cloud.globals().allDirty], [1, 1]);
    // 부분 출력: 전체 요청을 지운 뒤 현재 행 6 만 켜면 그 행만 화면 전체 폭으로 바뀐다(나머지 행은 0 그대로).
    cloud.cleanDirty();
    screen.setClip(0, 0, 640, 480);
    screen.lock().fill(0);
    screen.unlock();
    cloud.globals().dirtyRows[6] = 1;
    cloud.draw(screen, [0, 0, 640, 480, 0, 0], files, config, random);
    ({ pixels, pitch } = screenPixels(screen));
    mismatches = 0;
    // 행 6 은 캐시 행 6, 나머지는 0 인지 화면 전체를 비교한다.
    for (let y = 0; y < 480; y += 1) for (let x = 0; x < 640; x += 1) if (pixels[y * pitch + x] !== (y === 6 ? pattern[6 * 512 + (x % 512)] : 0)) mismatches += 1;
    assert.equal(mismatches, 0);
    assert.deepEqual([cloud.state()[6], cloud.state()[5]], [1, 0]);
    // 면적 0 은 장치도 부르지 않는다.
    cloud.draw({}, [5, 5, 5, 400, 0, 0], files, config, random);
    // parallax 잠금은 거부하며 이미 바꾼 clip 은 복구하지 않는다.
    screen.setClip(1, 2, 3, 4);
    assert.throws(() => cloud.draw(screen, [10, 20, 30, 40, 0, 0], files, config, random, true), /구름 parallax DirectDraw 004a15d0 장치 미복원/);
    assert.deepEqual(screen.clip(), { left: 10, top: 20, right: 30, bottom: 40 });
});

test('구름 팔레트 전환 비교·경로: CRT _stricmp 는 A~Z 만 접고, NUL 없는 이름·인자 계약을 거부하며 경로는 이름을 local.1 로 쓴다', () => {
    const random = new GameRandom(0);
    const specs = [];
    const config = { pathSpec: (key, ...args) => { specs.push([key, ...args]); return key === 'CloudFileSpec' ? 'd/none.gif' : `pal/${args[0]}.col`; }, readInt: (key, current) => ({ changed: false, value: current }) };
    const files = { has: () => false, read: () => null };
    const cloud = new ClientKencloudGlobals();
    const loads = [];
    const devices = { log: () => {}, loadPalette: (file) => loads.push(String.fromCharCode(...file)), buildMaps: () => {} };
    // 이름 쌍과 "같음" 판정이다.
    const same = (first, second) => {
        cloud.transition(cbytes(first), files, config, random, devices);
        const before = loads.length;
        cloud.transition(cbytes(second), files, config, random, devices);
        return loads.length === before;
    };
    assert.equal(same('ABCXYZ', 'abcxyz'), true);
    assert.equal(same('A@', 'A`'), false);
    assert.equal(same('A[', 'A{'), false);
    assert.equal(same('A{', 'A['), false);
    assert.equal(same('Za', 'zA'), true);
    // 첫 글자만 다른 이름과 새 이름 쪽의 'Z' 경계이다.
    assert.equal(same('Ab', 'Bb'), false);
    assert.equal(same('az', 'aZ'), true);
    assert.equal(same('pal', 'pal2'), false);
    assert.equal(same('pal2', 'pal'), false);
    assert.deepEqual(specs.at(-1), ['GamePalSpec', 'pal']);
    assert.equal(loads.at(-1), 'pal/pal.col\0');
    assert.deepEqual(specs.filter((spec) => spec[0] === 'CloudFileSpec')[0], ['CloudFileSpec']);
    // 이름에 NUL 이 없으면 로그 뒤 비교에서 물리 범위 오류이다.
    const trace = [];
    assert.throws(() => cloud.transition(Uint8Array.from([0x41, 0x42]), files, config, random, { ...devices, log: () => trace.push('L') }), /구름 현재 C문자열 물리 범위/);
    assert.deepEqual(trace, ['L']);
    // 현재 이름 쪽에 NUL 이 없어도 같은 오류이다.
    cloud.paletteName().fill(0x41);
    assert.throws(() => cloud.transition(cbytes('B'), files, config, random, devices), /구름 현재 C문자열 물리 범위/);
});

test('구름 Renderer 연결: 같은 객체는 그대로, 새 객체마다 0x1000 간격 토큰, null 은 0, 상한 넘는 다음 토큰은 거부한다', () => {
    const cloud = new ClientKencloudGlobals();
    const first = {};
    const second = {};
    assert.equal(cloud.rendererToken(), 0);
    cloud.bindRenderer(null);
    assert.equal(cloud.rendererToken(), 0);
    cloud.bindRenderer(first);
    assert.equal(cloud.rendererToken(), 0x3b000000);
    cloud.bindRenderer(first);
    assert.equal(cloud.rendererToken(), 0x3b000000);
    cloud.bindRenderer(second);
    assert.equal(cloud.rendererToken(), 0x3b001000);
    assert.equal(cloud.paletteGlobals().renderer(), 0x3b001000);
    cloud.bindRenderer(null);
    assert.equal(cloud.rendererToken(), 0);
    cloud.bindRenderer(undefined);
    assert.equal(cloud.rendererToken(), 0);
    cloud.bindRenderer(first);
    assert.equal(cloud.rendererToken(), 0x3b002000);
    // 남은 토큰을 모두 쓰면(마지막 0xffffe000) 다음 새 객체는 거부한다.
    const full = new ClientKencloudGlobals();
    let used = 0;
    let failure = '';
    // 두 객체를 번갈아 붙여 토큰을 하나씩 소비한다(기대 개수보다 한 번 더까지만).
    while (used <= 806911) {
        try { full.bindRenderer(used % 2 ? first : second); } catch (error) { failure = error.message; break; }
        used += 1;
    }
    assert.equal(used, 806911);
    assert.match(failure, /현재 Renderer 주소 토큰 소진/);
    assert.equal(full.rendererToken(), 0xffffe000);
    full.bindRenderer(null);
    assert.equal(full.rendererToken(), 0);
});
