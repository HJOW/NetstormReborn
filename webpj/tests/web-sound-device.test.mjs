// 실제 SoundPlayer와 Web Audio 장치 훅의 버퍼·복제·좌우·수명을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { AudioSystem } from '../www/audio/audio-system.js';
import { MissionAudioAssets } from '../www/audio/mission-audio-assets.js';
import { SoundList, SOUND_FIELD } from '../www/audio/sound-list.js';
import { createSoundState, SILENT_SOUND_BUFFER, SoundPlayer } from '../www/audio/sound-player.js';
import { WebSoundDevice } from '../www/audio/web-sound-device.js';

// 연결·해제와 실제 gain 값을 관찰할 수 있는 Web Audio API 장치다.
function contextFixture() {
    const gains = [], sources = [], nodes = [];
    // 각 노드의 연결과 반납은 독립적으로 기록한다.
    const node = () => { const item = { links: [], disconnected: false, connect(...args) { this.links.push(args); }, disconnect() { this.disconnected = true; } }; nodes.push(item); return item; };
    const context = { destination: {}, createGain: () => { const gain = { ...node(), gain: { value: 0 } }; gains.push(gain); return gain; },
        createChannelSplitter: () => node(), createChannelMerger: () => node(),
        createBufferSource: () => { const source = { ...node(), onended: null, started: false, stopped: false,
            start() { this.started = true; }, stop() { this.stopped = true; } }; sources.push(source); return source; },
        decodeAudioData: async (data) => ({ bytes: data.byteLength }) };
    return { context, gains, sources, nodes };
}

// 준비된 원본 감쇠 파일의 동일 해독본에서 독립 버퍼를 만들고 현재 재생 수명을 추적한다.
test('웹 소리 장치: 실제 SoundPlayer의 복제·감쇠·좌우·자연 종료', async () => {
    const scene = contextFixture(), audio = new AudioSystem(scene.context);
    const loader = { manifest: { files: [{ path: 'sound/test-500.wav' }] }, loadFile: async () => Uint8Array.of(1, 2, 3) };
    const assets = new MissionAudioAssets(loader, audio); await assets.loadSound('test.wav');
    const device = new WebSoundDevice(audio, assets), hooks = device.hooks(), list = new SoundList(), state = createSoundState();
    list.initialize(); state.initialized = state.device = state.enabled = true;
    const player = new SoundPlayer(list, state, hooks), sound = list.lookup('test.wav');
    // 공통 출력 노드 개수와 무관하게 이 재생에서 실제 추가한 세 음량 노드만 확인한다.
    const firstVoice = scene.gains.length;
    try {
        assert.equal(player.play(sound, 0, -1000, 2000, 0, 0), sound); assert.equal(list.field(sound, SOUND_FIELD.attenuation), 500);
        assert.equal(scene.sources[0].started, true); assert.equal(player.isPlaying(sound), 1);
        assert.ok(Math.abs(scene.gains[firstVoice].gain.value - 10 ** (-1500 / 2000)) < 1e-12);
        assert.ok(Math.abs(scene.gains[firstVoice + 1].gain.value - 0.1) < 1e-12); assert.equal(scene.gains[firstVoice + 2].gain.value, 1);
        const copy = player.play(sound, 1, 0, -10000, 0, 0); assert.notEqual(copy, sound); assert.equal(scene.sources[1].loop, true);
        assert.equal(scene.gains[firstVoice + 4].gain.value, 1); assert.equal(scene.gains[firstVoice + 5].gain.value, 0);
        // 원본 장치도 음악/일반 효과음과 같은1배 버스로 합쳐지고 음량은 다시 곱하지 않는다.
        assert.equal(audio.output.gain.value, 1); assert.deepEqual(audio.output.links, [[scene.context.destination]]);
        assert.deepEqual(audio.soundGain.links, [[audio.output]]); assert.deepEqual(audio.musicGain.links, [[audio.output]]);
        assert.equal(scene.nodes.filter(node => node.links.some(link => link[0] === audio.output)).length, 4);
        scene.sources[0].onended(); assert.equal(player.isPlaying(sound), 0); assert.equal(state.playing, 2);
        assert.equal(player.recount(), 1); player.stop(copy); assert.equal(scene.sources[1].stopped, true); assert.equal(state.playing, 0);
        assert.equal(hooks.load('missing.wav').buffer, SILENT_SOUND_BUFFER);
    } finally { device.dispose(); assets.dispose(); }
    assert.throws(() => hooks.status(1), /반납한 웹 소리 장치/);
});

// 음원은 비동기로 미리 준비해야 하며 잘못된 음량·좌우는 장치 실패로 남긴다.
test('웹 소리 장치: 준비·범위·반납 경계', () => {
    const scene = contextFixture(), audio = new AudioSystem(scene.context);
    const assets = new MissionAudioAssets({ manifest: { files: [{ path: 'sound/a.wav' }] }, loadFile: async () => Uint8Array.of(0) }, audio);
    const device = new WebSoundDevice(audio, assets), hooks = device.hooks();
    assert.throws(() => hooks.load('a.wav'), /준비되지 않은/); audio.sounds.set('a.wav', {});
    const { buffer } = hooks.load('a.wav'); assert.equal(hooks.setVolume(buffer, 1), -1); assert.equal(hooks.setPan(buffer, -10001), -1);
    assert.throws(() => hooks.setPosition(buffer, 1), /0이 아닌 위치/);
    hooks.play(buffer, 1); assert.equal(hooks.status(buffer), 5); const oldEnded = scene.sources[0].onended;
    hooks.stop(buffer); hooks.play(buffer, 0); oldEnded(); assert.equal(hooks.status(buffer), 1);
    device.dispose(); assert.equal(scene.sources[1].stopped, true); device.dispose(); assert.throws(() => hooks.status(buffer), /반납/);
});
