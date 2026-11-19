// 실제 공개 자산과 AudioSystem을 사용해 미션 음향의 선별 적재와 반납을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { AssetLoader } from '../www/assets/asset-loader.js';
import { AudioSystem } from '../www/audio/audio-system.js';
import { MissionAudioAssets } from '../www/audio/mission-audio-assets.js';
import { Netstorm } from '../www/netstorm.js';
import { DATA_ROOT } from './helpers.mjs';

// 실제 파일과 같은 바이트를 돌려주는 요청기이며 요청 순서를 기록한다.
function diskLoader() {
    const requests = [];
    const loader = new AssetLoader({ baseUrl: 'http://localhost/data/', fetchFunction: async (url) => {
        const relative = decodeURIComponent(new URL(url).pathname.slice('/data/'.length)); requests.push(relative);
        const bytes = fs.readFileSync(path.join(DATA_ROOT, ...relative.split('/')));
        return new Response(bytes, { status: 200 });
    } });
    return { loader, requests };
}

// Web Audio 출력 노드와 해독 결과를 제공하되 실제 파일 바이트 길이를 관찰한다.
function audioFixture() {
    const decoded = [];
    const context = { destination: {}, createGain: () => ({ gain: { value: 0 }, connect: () => {} }),
        decodeAudioData: async (bytes) => { decoded.push(bytes.byteLength); return { byteLength: bytes.byteLength }; } };
    return { audio: new AudioSystem(context), decoded };
}

// 같은 효과음의 대소문자 요청은 한 번 받고, 음악은 선택된 한 곡만 받는다.
test('미션 음향: 실제 파일의 선별 다운로드·동시 요청·앱 소유', async () => {
    const { loader, requests } = diskLoader(), { audio, decoded } = audioFixture(); await loader.loadManifest(); requests.length = 0;
    const app = new Netstorm(); app._assetLoader = loader; app._audio = audio;
    const [first, second] = await Promise.all([app._prepareMissionSound('thunderCrack.wav'), app._prepareMissionSound('THUNDERCRACK.WAV')]);
    assert.equal(first, second); assert.deepEqual(requests, ['sound/thunderCrack.wav']); assert.equal(audio.sounds.get('thundercrack.wav'), first);
    assert.equal(await app._prepareMissionSound('thunderCrack.wav'), first); assert.equal(requests.length, 1);
    const music = await app._prepareMissionMusic('fanfare.mus'); assert.equal(audio.music.get('fanfare.mus'), music);
    assert.deepEqual(requests, ['sound/thunderCrack.wav', 'music/fanfare.mus']); assert.equal(decoded.length, 2);
    await assert.rejects(app._prepareMissionMusic('../fanfare.mus'), /파일 이름 오류/);
    await assert.rejects(app._prepareMissionSound('없는.wav'), /파일 누락/);
    app._missionAudioAssets.dispose(); await assert.rejects(app._prepareMissionSound('thunderCrack.wav'), /반납/);
});

// 실패한 요청은 성공으로 저장하지 않고 재시도하며, 반납 중인 해독 결과는 등록하지 않는다.
test('미션 음향: 실패 재시도와 비동기 반납', async () => {
    let attempts = 0, finishDecode;
    const loader = { manifest: { files: [{ path: 'sound/bridge.wav' }] }, loadFile: async () => {
        attempts += 1; if (attempts === 1) throw new Error('파일 실패'); return Uint8Array.of(1, 2);
    } };
    const audio = { sounds: new Map(), music: new Map(), closed: false, decode: () => new Promise((resolve) => { finishDecode = resolve; }) };
    const assets = new MissionAudioAssets(loader, audio);
    await assert.rejects(assets.loadSound('bridge.wav'), /파일 실패/); assert.equal(audio.sounds.size, 0);
    const pending = assets.loadSound('bridge.wav'); await Promise.resolve(); await Promise.resolve();
    assert.equal(typeof finishDecode, 'function'); assets.dispose(); finishDecode({});
    await assert.rejects(pending, /반납/); assert.equal(audio.sounds.size, 0); assert.equal(attempts, 2);
    assert.throws(() => new MissionAudioAssets({ ...loader, manifest: { files: [{ path: 'sound/A.wav' }, { path: 'sound/a.WAV' }] } }, audio), /중복 경로/);
});
