// 미션 장면 음악 구동기(원소 곡 순환·곡 길이 표·미리 받기·버퍼 놓기)를 대역 재생기·적재기·시계로 확인한다. 게임과 브라우저를 실행하지 않는다. (2026-11-20 WB04-E1119)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT } from './helpers.mjs';
import { MENU_TRACK, MUSIC_DATA_BYTES, SceneMusicDriver, musicSeconds } from '../www/audio/scene-music-driver.js';
import { TRACKS } from '../www/audio/scene-music.js';

// 대역 장치 묶음을 만든다. prepare 는 호출을 기록하고 finish(이름) 으로 받기를 끝내거나 fail(이름) 으로 실패시킨다.
// random 은 values 를 차례로 돌려주며 시계는 clock.now 를 읽는다.
function fixture({ values = [0], menuLoaded = true } = {}) {
    const audio = { music: new Map(), closed: false, played: [], playMusic(name) { this.played.push(name); return true; } };
    if (menuLoaded) audio.music.set(MENU_TRACK, {});
    const clock = { now: 100 }, prepared = [], released = [], weather = [], errors = [], waiting = new Map();
    const driver = new SceneMusicDriver({
        audio, wallSeconds: () => clock.now, random: { next: (limit) => { assert.equal(limit, 4000); return values.shift() ?? 0; } },
        prepare: (name) => { prepared.push(name); return new Promise((resolve, reject) => waiting.set(name, { resolve: () => { audio.music.set(name.toLowerCase(), {}); resolve({}); }, reject })); },
        release: (name) => { released.push(name); audio.music.delete(name); }, weatherSound: (name) => weather.push(name), onError: (error, name) => errors.push([error.message, name]),
    });
    // 받기 하나를 끝내고 이어지는 비동기 정리가 돌도록 마이크로태스크를 비운다.
    const finish = async (name) => { waiting.get(name).resolve(); waiting.delete(name); for (let index = 0; index < 5; index += 1) await Promise.resolve(); };
    return { audio, clock, prepared, released, weather, errors, waiting, driver, finish };
}

test('곡 길이 표: 모든 곡이 공개 자산 파일 머리의 data 크기·원본 식과 같다', () => {
    const directory = path.join(DATA_ROOT, 'music'), files = fs.readdirSync(directory).filter((name) => name.endsWith('.mus')).sort();
    assert.deepEqual(Object.keys(MUSIC_DATA_BYTES).sort(), files);
    for (const name of files) {
        // RIFF/WAVE 청크를 읽어 fmt(채널·빈도·비트)와 data 크기를 얻는다.
        const bytes = fs.readFileSync(path.join(directory, name)); let offset = 12, channels = 0, rate = 0, bits = 0, length = -1;
        while (offset + 8 <= bytes.length) {
            const id = bytes.toString('latin1', offset, offset + 4), size = bytes.readUInt32LE(offset + 4);
            if (id === 'fmt ') { channels = bytes.readUInt16LE(offset + 10); rate = bytes.readUInt32LE(offset + 12); bits = bytes.readUInt16LE(offset + 22); }
            if (id === 'data') { length = size; break; }
            offset += 8 + size + (size & 1);
        }
        assert.equal(MUSIC_DATA_BYTES[name], length, name);
        // 원본 SoundMusic 의 길이 식(길이 / (비트×0.125) / 빈도 / 채널)과 같은 순서로 계산한다.
        assert.equal(musicSeconds(name), length / (bits * 0.125) / rate / channels, name);
        assert.equal(musicSeconds(name.toUpperCase()), musicSeconds(name));
    }
    assert.equal(musicSeconds('없는.mus'), 0);
    // 30초 기준은 결과 곡만 짧다(fanfare 21.56초). 원소 곡은 모두 200초를 넘는다.
    assert.ok(musicSeconds('fanfare.mus') < 30); for (const name of TRACKS) assert.ok(musicSeconds(name) > 200);
});

test('begin: 난수로 시작 원소 곡을 고르고 받은 뒤에 재생하며 다음 곡을 미리 받는다', async () => {
    // 난수 2500 → 색인 2 → next() 가 색인을 올려 3 → sun22.mus 이다. 그 다음 곡은 wind22.mus 이다.
    const f = fixture({ values: [2500] });
    assert.equal(f.driver.active, false); f.driver.begin();
    assert.equal(f.driver.active, true); assert.equal(f.driver.selected, 'sun22.mus'); assert.deepEqual(f.prepared, ['sun22.mus']); assert.deepEqual(f.audio.played, []);
    await f.finish('sun22.mus');
    assert.deepEqual(f.audio.played, ['sun22.mus']); assert.deepEqual(f.prepared, ['sun22.mus', 'wind22.mus']);
    // 두 번째 begin 은 난수도 곡도 바꾸지 않는다.
    f.driver.begin(); assert.deepEqual(f.prepared, ['sun22.mus', 'wind22.mus']); assert.equal(f.driver.selected, 'sun22.mus');
});

test('begin: 천둥 곡이 시작될 때만 천둥 효과음을 내고 곡마다 색인 순환을 지킨다', () => {
    // 난수 1500 → 색인 1 → 2 → thu22.mus 이며 날씨 효과음이 난다. 난수 0 → 1 → rain22.mus 는 효과음이 없다.
    const thunder = fixture({ values: [1500] }); thunder.driver.begin();
    assert.equal(thunder.driver.selected, 'thu22.mus'); assert.deepEqual(thunder.weather, ['thunderCrack.wav']);
    const rain = fixture({ values: [0] }); rain.driver.begin(); assert.equal(rain.driver.selected, 'rain22.mus'); assert.deepEqual(rain.weather, []);
    // 난수 3999 → 3 → (3+1)%4=0 → wind22.mus 로 넘어 돈다.
    const wrap = fixture({ values: [3999] }); wrap.driver.begin(); assert.equal(wrap.driver.selected, 'wind22.mus');
});

test('frame: 곡 기한 전에는 그대로 두고 기한이 지나면 미리 받은 다음 곡을 바로 틀며 지난 곡은 놓는다', async () => {
    const f = fixture({ values: [2500] }); f.driver.begin(); await f.finish('sun22.mus'); await f.finish('wind22.mus');
    // 시작 100초 + sun22.mus 길이 232.83초 = 기한 332.83초 이다.
    const end = 100 + musicSeconds('sun22.mus'); f.clock.now = end - 0.01; f.driver.frame();
    assert.equal(f.driver.selected, 'sun22.mus'); assert.deepEqual(f.audio.played, ['sun22.mus']);
    f.clock.now = end; f.driver.frame();
    assert.equal(f.driver.selected, 'wind22.mus'); assert.deepEqual(f.audio.played, ['sun22.mus', 'wind22.mus']);
    // 지난 sun22.mus 는 놓고 다음 rain22.mus 를 미리 받는다. 메뉴 곡은 유지한다.
    assert.deepEqual(f.released, ['sun22.mus']); assert.ok(f.audio.music.has(MENU_TRACK)); assert.equal(f.prepared.at(-1), 'rain22.mus');
    // 같은 곡의 다음 기한은 wind22.mus 길이를 따른다(곡마다 길이가 다르다).
    f.clock.now = end + musicSeconds('wind22.mus') - 0.01; f.driver.frame(); assert.equal(f.driver.selected, 'wind22.mus');
    f.clock.now = end + musicSeconds('wind22.mus'); f.driver.frame(); assert.equal(f.driver.selected, 'rain22.mus');
});

test('frame: 미션 장면이 아니면 아무 일도 하지 않는다', () => {
    const f = fixture(); f.clock.now = 1e9; f.driver.frame(); assert.deepEqual(f.prepared, []); assert.deepEqual(f.audio.played, []); assert.equal(f.driver.selected, MENU_TRACK);
});

test('end: 메뉴 곡을 다시 요청하고 받아 둔 미션 곡을 모두 놓으며 두 번 불러도 안전하다', async () => {
    const f = fixture({ values: [2500] }); f.driver.begin(); await f.finish('sun22.mus'); await f.finish('wind22.mus');
    f.driver.end();
    assert.equal(f.driver.active, false); assert.equal(f.driver.selected, MENU_TRACK); assert.equal(f.audio.played.at(-1), MENU_TRACK);
    assert.deepEqual([...f.audio.music.keys()], [MENU_TRACK]); assert.deepEqual([...f.released].sort(), ['sun22.mus', 'wind22.mus']);
    // 미션이 아니면 frame 도 end 도 효과가 없다.
    const playedCount = f.audio.played.length; f.driver.end(); f.clock.now = 1e9; f.driver.frame(); assert.equal(f.audio.played.length, playedCount);
    // 다시 시작하면 새 곡 선택이 된다(난수가 비면 0 → rain22.mus).
    f.driver.begin(); assert.equal(f.driver.selected, 'rain22.mus');
});

test('받는 동안 미션을 떠나면 그 곡은 틀지 않고 받은 뒤에 놓는다', async () => {
    const f = fixture({ values: [2500] }); f.driver.begin(); assert.deepEqual(f.audio.played, []);
    f.driver.end(); assert.equal(f.audio.played.at(-1), MENU_TRACK); const before = f.audio.played.length;
    await f.finish('sun22.mus');
    assert.equal(f.audio.played.length, before, '떠난 뒤 받은 곡을 틀지 않는다'); assert.deepEqual(f.released, ['sun22.mus']); assert.deepEqual([...f.audio.music.keys()], [MENU_TRACK]);
});

test('재생기가 닫힌 뒤에는 받은 곡도 틀지 않는다', async () => {
    const f = fixture({ values: [0] }); f.driver.begin(); f.audio.closed = true; await f.finish('rain22.mus'); assert.deepEqual(f.audio.played, []);
});

test('곡 받기가 실패하면 오류와 곡 이름을 알리고 게임은 계속한다', async () => {
    const f = fixture({ values: [0] }); f.driver.begin();
    f.waiting.get('rain22.mus').reject(new Error('받기 실패')); for (let index = 0; index < 5; index += 1) await Promise.resolve();
    assert.deepEqual(f.errors, [['받기 실패', 'rain22.mus']]); assert.deepEqual(f.audio.played, []); assert.equal(f.driver.active, true);
    // 미리 받기 실패도 알리기만 한다. 곡이 바뀔 때 다시 받는다.
    f.clock.now = 100 + musicSeconds('rain22.mus'); f.driver.frame(); assert.equal(f.driver.selected, 'thu22.mus');
});

test('음악이 이미 해독돼 있으면 받지 않고 바로 틀며 다음 곡만 미리 받는다', () => {
    const f = fixture({ values: [0] }); f.audio.music.set('rain22.mus', {}); f.driver.begin();
    assert.deepEqual(f.audio.played, ['rain22.mus']); assert.deepEqual(f.prepared, ['thu22.mus']);
});

test('필수 장치가 하나라도 없으면 거부한다', () => {
    const base = { audio: { music: new Map(), playMusic: () => true }, prepare: () => Promise.resolve(), release: () => {}, wallSeconds: () => 0, random: { next: () => 0 } };
    assert.doesNotThrow(() => new SceneMusicDriver(base));
    for (const name of ['audio', 'prepare', 'release', 'wallSeconds', 'random']) assert.throws(() => new SceneMusicDriver({ ...base, [name]: undefined }), /장치 누락/u, name);
    assert.throws(() => new SceneMusicDriver({ ...base, audio: { music: [], playMusic: () => true } }), /장치 누락/u);
    assert.throws(() => new SceneMusicDriver({ ...base, weatherSound: 1 }), /장치 누락/u); assert.throws(() => new SceneMusicDriver(), /장치 누락/u);
});

test('request: 미션 중 희생 곡을 요청하면 받은 뒤 틀고 다음 원소 곡은 기한 뒤 이어 간다', async () => {
    const f = fixture({ values: [2500] }); f.driver.begin(); await f.finish('sun22.mus'); await f.finish('wind22.mus');
    f.driver.request('sacrifice.mus');
    assert.equal(f.driver.selected, 'sacrifice.mus'); assert.equal(f.prepared.at(-1), 'sacrifice.mus'); assert.equal(f.audio.played.at(-1), 'sun22.mus', '받기 전에는 이전 곡이 계속된다');
    await f.finish('sacrifice.mus');
    assert.equal(f.audio.played.at(-1), 'sacrifice.mus');
    // 희생 곡은 원소 곡이 아니므로 다음 곡을 미리 받지 않고 지난 원소 곡 버퍼는 놓는다. 메뉴 곡은 유지한다.
    assert.deepEqual([...f.audio.music.keys()].sort(), [MENU_TRACK, 'sacrifice.mus'].sort());
    // 같은 곡을 다시 요청해도 아무 일도 없다.
    const count = f.prepared.length; f.driver.request('SACRIFICE.MUS'); assert.equal(f.prepared.length, count); assert.equal(f.audio.played.filter((name) => name === 'sacrifice.mus').length, 1);
    // 기한은 희생 곡 길이(139.92초)를 따르고, 지나면 색인 순환의 다음 원소 곡(wind22.mus)으로 돌아간다.
    f.clock.now = 100 + musicSeconds('sacrifice.mus') - 0.01; f.driver.frame(); assert.equal(f.driver.selected, 'sacrifice.mus');
    f.clock.now = 100 + musicSeconds('sacrifice.mus'); f.driver.frame(); assert.equal(f.driver.selected, 'wind22.mus');
});

test('request: 승리 곡은 결과 상태가 아니면 무시하고 미션 장면이 아니면 아무 일도 없다', () => {
    const idle = fixture(); idle.driver.request('sacrifice.mus'); assert.deepEqual(idle.prepared, []); assert.equal(idle.driver.selected, MENU_TRACK);
    const f = fixture({ values: [0] }); f.driver.begin(); f.driver.request('fanfare.mus'); assert.equal(f.driver.selected, 'rain22.mus', '결과 상태 1 이 아니면 승리 곡 요청을 거른다');
    f.driver.end(); const count = f.prepared.length; f.driver.request('sacrifice.mus'); assert.equal(f.prepared.length, count); assert.equal(f.driver.selected, MENU_TRACK);
});

// 원본 MusicChannelDuration 은 주 채널이 파일을 연 적이 없거나 닫았으면 0 을 돌려주고 감독은 길이 0 을 180초 뒤 재확인으로 처리한다. 소리·음악 옵션을 끈 동안의 곡 기한은 곡 길이 표(원소 곡 200초 넘음)가 아니라 180초이다. (2026-11-23 WB04-E1258)
test('곡 기한: 음악이나 소리를 끈 동안은 곡 길이 대신 180초 뒤에 다음 곡으로 넘어간다', async () => {
    const setup = async ({ musicEnabled, soundEnabled }) => {
        // 난수 0 → 색인 0 → next() 가 색인을 올려 1 → rain22.mus 이다.
        const f = fixture({ values: [0] }); f.audio.musicEnabled = musicEnabled; f.audio.soundEnabled = soundEnabled;
        f.driver.begin(); await f.finish('rain22.mus'); f.audio.played.length = 0; f.prepared.length = 0;
        return f;
    };
    // 둘 다 켜짐(또는 속성 없음): 곡 길이(200초 넘음)가 지나야 다음 곡으로 간다. 시작 시각 100 + 길이.
    const on = await setup({ musicEnabled: true, soundEnabled: true });
    on.clock.now = 100 + 181; on.driver.frame(); assert.deepEqual(on.audio.played, [], '180초가 지나도 곡이 이어진다');
    on.clock.now = 100 + musicSeconds('rain22.mus') + 1; on.driver.frame(); assert.deepEqual(on.prepared.concat(on.audio.played).length > 0, true, '곡 길이가 지나면 다음 곡');
    // 음악을 끔 / 소리를 끔: 길이 0 → 시작 시각 100 + 180 뒤에 다음 곡을 고른다.
    for (const [name, options] of [['음악 끔', { musicEnabled: false, soundEnabled: true }], ['소리 끔', { musicEnabled: true, soundEnabled: false }]]) {
        const off = await setup(options);
        off.clock.now = 100 + 179; off.driver.frame(); assert.deepEqual(off.prepared, [], `${name}: 179초에는 그대로`);
        off.clock.now = 100 + 181; off.driver.frame(); assert.ok(off.prepared.length > 0 || off.audio.played.length > 0, `${name}: 180초 뒤 다음 곡`);
    }
});
