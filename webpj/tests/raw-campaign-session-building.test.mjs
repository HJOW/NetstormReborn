// 실제 기존 owner에 건설/캡처/수신을 연결한다(WB04-E639). 전체 미션 Load/Client 앱 성공과 구별한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { get, put } from './raw-support.mjs';
import { drawShape } from '../www/render/vfx-draw.js';
import { RawColorSortPrimary, COLOR_SORT_PRIMARY_BYTES } from '../www/core/raw-color-sort-primary.js';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { ConstructionProcess } from '../www/core/raw-construction-process.js';

// 화면/음향/선택의 외부 입력만 공급하고 실제 Impl 생성 순서와 모든 World/Host 부품을 사용한다.
function scene(extra = {}, bind = true) {
    const calls = [], o = campaignOwners({ ...extra, deletion: { selected: () => 0, ...extra.deletion },
        construction: { refresh: () => calls.push('refresh'), ...extra.construction },
        sound: { play: (sound) => { calls.push(['play', sound]); return sound; }, playOnceAt: (x, y, sound) => { calls.push(['once', x, y, sound]); return sound; }, playLoopAt: (_x, _y, current, sound) => current || sound, stop: (sound) => calls.push(['stop', sound]), isPlaying: () => 0, ...extra.sound },
    });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }); o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction();
        if (bind) o.connectBuilding(); return { o, calls };
    } catch (error) { o.dispose(); throw error; }
}
// 같은 실제 World에 명시 abstract bit1 입력으로 배치한다. 색상 abstract bit16/미배치 VOID와 구별한다.
function parent(o, abstract = true) {
    const sid = o.world.create(75); if (abstract) o.core.pool.allocated(sid)[40] |= 1;
    o.core.owner.set(sid, 1); o.world.pop(sid, 32, 32, 0); return sid;
}
// 원문 경과 시각을 준비한 뒤 실제30/47과 같은 Form/Kernel 객체를 조회한다. remove=false 조회다.
function construction(o, sid) {
    o.core.processState.now = 12.5; o.assetModules.prepareBuilding(12.5); o.building.start(sid, 10);
    const form = new RawContainedLookup(o.core.pool, () => { throw new Error('조회 중 삭제 금지'); }).find(sid, 30);
    const process = o.core.processes.fromForm(form); assert.ok(process instanceof ConstructionProcess); return { process, form };
}

test('Session Building 설치: 생성 순서·동일 Pool/Host·색표·미션 미설치·반복/반납', () => {
    const p = campaignOwners(); try { assert.throws(() => p.connectBuilding(), /초기화 전/); } finally { p.dispose(); }
    const { o } = scene({}, false);
    try {
        const bytes = o.core.pool.bytes(), seed = o.random.state; o.connectBuilding();
        assert.equal(o.buildingModules.ready, true); assert.equal(o.building.pool, o.core.pool); assert.ok(o.buildingData); assert.ok(o.buildingEffects);
        assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.equal(o.loaded, false);
        assert.deepEqual([o.buildingMapTokens[75], o.buildingMapTokens[133], o.buildingMapTokens[82], o.buildingMapTokens[97]], [0, 1, 2, 2]);
        assert.equal(o.buildingCaptureState.addTypeHotspot, true); assert.equal(o.buildingCaptureState.ownerTypeMaps, o.buildingMapTokens);
        assert.throws(() => o.connectBuilding(), /반복/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectBuilding(), /반납한 Session/); assert.throws(() => o.buildingCaptureState.mainFrame(1), /반납\/미연결/);
});

// 주 캡처의 plain 건물 분기를 실제 SHP decoder의 독립 버퍼와 전 픽셀 비교한다.
test('Session Building 캡처: 실제 plain SHP·기준점·일반 건물의 추가 그림 ret8', () => {
    const { o } = scene();
    try {
        const sid = o.world.create(75), raw = o.core.pool.allocated(sid), definition = o.assets.game.types()[5].definition;
        const current = Math.min(1, o.core.shapes[75].frameCount - 1); put(raw, 36, current);
        const image = o.buildingCapture.capture(sid), expected = new Uint8Array(image.pixels.length).fill(255);
        drawShape(expected, image.width, image.height, o.assets.game.shapes, 5, current,
            image.hotspotX - (o.core.types[75].hotspotX ?? 0), image.hotspotY - (o.core.types[75].hotspotY ?? 0), { left: 0, top: 0, right: image.width - 1, bottom: image.height - 1 });
        assert.deepEqual(image.pixels, expected); assert.ok(image.pixels.some((pixel) => pixel !== 255));
        // 이 실제 타입은 baseframe 지정이 없다(-1). extra2 분기는 기본 그림 생략+빈 추가 그림이므로 전부 투명이다.
        assert.equal(definition.baseFrame, -1); raw[40] |= 2; assert.equal(o.buildingCaptureState.mainFrame(sid), -1);
        assert.ok(o.buildingCapture.capture(sid).pixels.every((pixel) => pixel === 255)); assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 저장 primary 한 번 선택/지연 표 캐시와 같은 공통 토큰/색표 참조를 검사한다.
test('Session Building 색표: 현재 소유자 색·공통 ownerTables·고정 primary·선택 누락', () => {
    const primary = new RawColorSortPrimary(new Uint8Array(COLOR_SORT_PRIMARY_BYTES).fill(0x7c));
    const { o } = scene({}, false); let reads = 0;
    try {
        const logical = o.assets.game.palette.logical; primary.construct(new Uint8Array(logical.buffer, logical.byteOffset, logical.byteLength));
        o.services.colorSortInput = { get primary() { reads++; return primary; } }; o.connectBuilding();
        o.core.bridgeState.ownerColors[1] = 8; assert.equal(o.buildingCaptureState.ownerColors(133, 1), o.buildingOwnerColors[8]);
        const first = o.buildingCaptureState.ownerColors(97, 1); assert.equal(first.length, 256); assert.equal(reads, 1);
        o.buildingCaptureState.ownerColors(97, 2); assert.equal(reads, 1); assert.equal(o.buildingSpecialOwnerColors.size, 1);
        assert.throws(() => o.buildingCaptureState.ownerColors(82, 1), /전용 소유자 색표 미복원/);
        assert.throws(() => o.buildingCaptureState.ownerColors(133, 9), /소유자 번호/);
    } finally { o.dispose(); }
    const tokens = new Uint32Array(188), table = new Uint8Array(256), calls = [];
    const shared = scene({ ownerColorTables: { tokens, color: (type, color) => { calls.push([type, color]); return table; } } });
    try { shared.o.core.bridgeState.ownerColors[2] = 7; assert.equal(shared.o.buildingCaptureState.ownerTypeMaps, tokens); assert.equal(shared.o.buildingCaptureState.ownerColors(133, 2), table); assert.deepEqual(calls, [[133, 7]]); }
    finally { shared.o.dispose(); }
    const absent = scene({ colorSortInput: { get primary() { throw new Error('명시 primary 미연결'); } } });
    try { assert.throws(() => absent.o.buildingCaptureState.ownerColors(97, 1), /명시 primary/); assert.equal(absent.o.buildingSpecialOwnerColors.size, 0); }
    finally { absent.o.dispose(); }
});

// 값 대입 뒤에도 처음 복사한 shared pointer는 유지한다. 수신 때에는 그 시점의 지역 값을 다시 복사한다.
test('Session Building checking: RawCheckingFlag 복사와 간헐천 수신의 현재값/동일 packet', () => {
    const { o } = scene({}, false);
    try {
        const debug = o.gameplayGlobals.aiDebug().checking(); debug.value = 3; o.connectBuilding();
        o.state.gunState.checking = 7; debug.value = 9; assert.equal(o.state.buildingState.read('checking'), 9);
        const seen = [], packet = Uint8Array.of(1, 2), original = o.geyserMined;
        o.geyserMined = { handlePacket: (value) => { assert.equal(value, packet); seen.push(o.state.geyserMinedState.checking); } };
        try {
            o.receiveGeyserMined(packet); debug.value = 12; assert.equal(o.state.geyserMinedState.checking, 7);
            o.state.gunState.checking = 17; o.receiveGeyserMined(packet); assert.deepEqual(seen, [7, 17]);
        } finally { o.geyserMined = original; }
    } finally { o.dispose(); }
});

// 실제30/47·표시 입자와 부모 HP/재등록을 같은 World에서 실행한다. 미션/프레임 루프 전체는 아니다.
test('Session Building 과정: 실제 Pop·Form30·Sound47·반/완료 시각과 Host 정리', () => {
    const { o, calls } = scene(); const kernel = o.core.kernel;
    try {
        const sid = parent(o), { process, form } = construction(o, sid), maximum = o.hp.maxHitPoints(sid);
        assert.equal(process.fields.start, 12.5); assert.equal(process.fields.end, 22.5); assert.equal(process.fields.sound, o.soundNames.lookup('jimBuild.wav'));
        const lookup = new RawContainedLookup(o.core.pool, () => { throw new Error('조회 중 삭제 금지'); }); assert.ok(lookup.find(sid, 47));
        o.core.processState.now = 17.5; o.assetModules.prepareBuilding(17.5); process.runFrame();
        assert.equal(get(o.core.pool.slot(sid), 26, 2), Math.trunc(maximum / 2)); assert.equal(o.core.processes.fromForm(form), process);
        o.core.processState.now = 22.5; o.assetModules.prepareBuilding(22.5); process.runFrame();
        assert.equal(get(o.core.pool.slot(sid), 26, 2), maximum); assert.equal(o.core.pool.slot(sid)[40] & 1, 0); assert.equal(o.core.pool.slot(form)[11] & 1, 1); assert.equal(lookup.find(sid, 47), 0);
        assert.ok(calls.filter((call) => call === 'refresh').length >= 2); assert.equal(o.core.bookkeeping.depth, 0); assert.equal(o.loaded, false);
    } finally { o.dispose(); }
    assert.equal(kernel.size, 0);
});

// 같은 실제 수신 decoder가 기존 Form의 Kernel 객체를 다시 고른다. 독립 packet의 end만 바꾼다.
test('Session Building Data: 같은 실제 Form/PID·decoder의 시각 부분 수신', () => {
    const { o } = scene();
    try {
        const sid = parent(o), { process, form } = construction(o, sid), packet = new Uint8Array(17); packet[0] = 1; packet[1] = 30; put(packet, 2, 17, 2); put(packet, 4, form); packet[8] = 4;
        new DataView(packet.buffer).setFloat64(9, 99.5, true); const start = process.fields.start, sound = process.fields.sound;
        o.buildingData.receive(packet); assert.equal(o.core.processes.fromForm(form), process); assert.equal(process.fields.end, 99.5); assert.equal(process.fields.start, start); assert.equal(process.fields.sound, sound);
    } finally { o.dispose(); }
});

// 다이스의 기본 그림 -1/전용169 추가 그림과 색상 abstract 거부를 실제 SHP/현재 raw에 연결한다.
test('Session Building 다이스 캡처: 실제 추가 그림·선택된 abstract 오류', () => {
    const { o } = scene();
    try {
        const sid = o.world.create(165), raw = o.core.pool.allocated(sid); raw[40] |= 2;
        const image = o.buildingCapture.capture(sid); assert.equal(o.buildingCaptureState.mainFrame(sid), -1); assert.ok(image.width > 0 && image.height > 0); assert.ok(image.pixels.some((pixel) => pixel !== 255));
        raw[40] |= 16; assert.throws(() => o.buildingCapture.capture(sid), /추상 다이스/); assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 신전123/124/125의 실제 정의/가상표를 공통 Renderer/캡처 함수에 공급한다. 의식/게임 상승은 실행하지 않는다.
test('Session Building 공통 신전 그림: 실제 세 타입과 레인 조건부 음향', () => {
    const { o, calls } = scene();
    try {
        // 세 원본 타입의 초기 프레임0/상태0을 같은 현재 Pool에서 조회한다.
        for (const [type, expected] of [[123, [0]], [124, [0, 0]], [125, [0]]]) {
            const sid = o.world.create(type), frames = []; o.drawTempleFrames(sid, (frame) => frames.push(frame)); assert.deepEqual(frames, expected);
        }
        assert.equal(calls.length, 0);
        const rain = o.world.create(124), raw = o.core.pool.allocated(rain), frames = []; put(raw, 12, 1, 2);
        o.drawTempleFrames(rain, (frame) => frames.push(frame)); assert.deepEqual(frames, [0, 0, o.core.frames[124].run(0x43).first]);
        assert.deepEqual(calls, [['play', o.soundNames.lookup('templeAscend.wav')]]);
        assert.throws(() => o.drawTempleFrames(o.world.create(75), () => {}), /가상표 신전/); assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});
