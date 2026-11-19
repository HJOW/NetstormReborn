// 앱 소유 효과음 준비와 실제 미션 서비스·설정 사이의 수명을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { AudioSystem } from '../www/audio/audio-system.js';
import { Settings } from '../www/platform/settings.js';

// 연결과 실제 소스·음량을 관찰할 수 있는 Web Audio 경계다.
function contextFixture() {
    const gains = [], sources = [];
    // 각 노드는 독립적인 연결 수명을 가진다.
    const node = () => ({ connect() {}, disconnect() {} });
    const context = { destination: {}, createGain: () => { const gain = { ...node(), gain: { value: 0 } }; gains.push(gain); return gain; },
        createChannelSplitter: () => node(), createChannelMerger: () => node(),
        createBufferSource: () => { const source = { ...node(), onended: null, start() { this.started = true; }, stop() { this.stopped = true; } }; sources.push(source); return source; },
        decodeAudioData: async (bytes) => ({ bytes: bytes.byteLength }) };
    return { context, gains, sources };
}

// 앱이 동일한 이름표·원본 감쇠·현재 옵션을 미션 서비스에 전달한다.
test('앱 미션 효과음: 준비 공유와 실제 재생·옵션 변경', async () => {
    const app = new Netstorm(), scene = contextFixture(); app._initializeClientRuntime(false);
    app._settings = new Settings({ onChange: (key) => app._onSettingChange(key) }); app._gameData = { readInt: (_key, fallback) => ({ value: fallback }), set() {} };
    app._clientPaletteScreen = { width: 640, height: 480 }; app._audio = new AudioSystem(scene.context);
    let reads = 0; app._assetLoader = { manifest: { files: [{ path: 'sound/test-500.wav' }] }, loadFile: async () => { reads += 1; return Uint8Array.of(1, 2); } };
    try {
        const [player, same] = await Promise.all([app._prepareCampaignAudio(), app._prepareCampaignAudio()]);
        assert.equal(player, same); assert.equal(reads, 1); assert.equal(app._clientSoundState.maxPlaying, 8);
        const services = app._campaignGameplayServices(), sound = app._clientSoundNames.lookup('test.wav');
        assert.equal(services.sound.play(sound, 0, 0, 0, 0, 0), sound); assert.equal(services.sound.isPlaying(sound), 1);
        assert.equal(scene.sources[0].started, true); assert.ok(Math.abs(scene.gains[2].gain.value - 10 ** (-500 / 2000)) < 1e-12);
        app._settings.set('soundVolume', 3); app._settings.set('swapLeftRightSpeakers', 1);
        assert.equal(app._clientSoundState.swapSpeakers, true); assert.ok(Math.abs(scene.gains[2].gain.value - 10 ** (-1500 / 2000)) < 1e-12);
        services.assets.postPop.playImpactSound(sound); assert.equal(scene.sources.length, 2);
        services.sound.stop(sound); assert.equal(scene.sources[0].stopped, true);
        app._settings.set('sound', 0); assert.equal(services.sound.play(sound, 0, 0, 0, 0, 0), 0); assert.equal(scene.sources.length, 2);
    } finally { app._clientSoundDevice?.dispose(); app._missionAudioAssets?.dispose(); }
});

// 오디오 미지원에서도 원본 비활성 장치처럼 효과음 요청이 안전하게 무시된다.
test('앱 미션 효과음: 오디오 미사용 장치', async () => {
    const app = new Netstorm({ audio: false }); app._initializeClientRuntime(false); app._settings = new Settings();
    const player = await app._prepareCampaignAudio(), services = app._campaignGameplayServices();
    const sound = app._clientSoundNames.lookup('test.wav');
    assert.equal(player, app._clientSoundPlayer); assert.equal(app._clientSoundState.device, false);
    assert.equal(services.sound.play(sound, 0, 0, 0, 0, 0), 0); assert.equal(services.sound.isPlaying(sound), 0);
});

// 대기 중 반납하면 오래된 해독 결과로 재생기를 만들지 않는다.
test('앱 미션 효과음: 준비 중 자산 반납', async () => {
    const app = new Netstorm(), scene = contextFixture(); app._initializeClientRuntime(false); app._settings = new Settings(); app._audio = new AudioSystem(scene.context);
    let resume; app._assetLoader = { manifest: { files: [{ path: 'sound/test.wav' }] }, loadFile: () => new Promise((resolve) => { resume = resolve; }) };
    const pending = app._prepareCampaignAudio(); app._missionAudioAssets.dispose(); resume(Uint8Array.of(1));
    await assert.rejects(pending, /반납한 미션 음향/); assert.equal(app._clientSoundPlayer, null); assert.equal(app._clientSoundPrepare, null);
});
