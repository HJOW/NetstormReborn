// 실제 Chromium 오프라인 Web Audio에서 원본 감쇠와 좌우 채널을 확인한다.
const { test, expect } = require('@playwright/test');

test('웹 소리 장치는 실제 Web Audio 출력에 음량과 좌우 감쇠를 적용한다', async ({ page }) => {
  await page.goto('/debug.html');
  const result = await page.evaluate(async () => {
    const { AudioSystem } = await import('/audio/audio-system.js');
    const { SoundList } = await import('/audio/sound-list.js');
    const { SoundPlayer, createSoundState } = await import('/audio/sound-player.js');
    const { WebSoundDevice } = await import('/audio/web-sound-device.js');
    const context = new OfflineAudioContext(2, 256, 44100), audio = new AudioSystem(context);
    const buffer = context.createBuffer(1, 128, 44100); buffer.getChannelData(0).fill(0.5); audio.sounds.set('test.wav', buffer);
    const assets = { resolveSound: () => ({ entry: { path: 'sound/test-500.wav' }, attenuation: 500 }) };
    const device = new WebSoundDevice(audio, assets), list = new SoundList(), state = createSoundState(); list.initialize();
    state.initialized = state.device = state.enabled = true;
    const player = new SoundPlayer(list, state, device.hooks()), sound = list.lookup('test.wav');
    const played = player.play(sound, 0, -1000, 2000, 0, 0);
    const rendered = await context.startRendering();
    return { played: played === sound, left: rendered.getChannelData(0)[16], right: rendered.getChannelData(1)[16] };
  });
  expect(result.played).toBe(true);
  expect(result.left).toBeCloseTo(0.5 * 10 ** (-1500 / 2000) * 0.1, 5);
  expect(result.right).toBeCloseTo(0.5 * 10 ** (-1500 / 2000), 5);
});

// 출하된 효과음 전체가 Chromium에서 실제로 해독되며 음악은 내려받지 않는다.
test('미션 효과음은 제한된 요청으로 모두 해독된다', async ({ page }) => {
  test.setTimeout(120000);
  await page.goto('/debug.html');
  const result = await page.evaluate(async () => {
    const { AssetLoader } = await import('/assets/asset-loader.js');
    const { AudioSystem } = await import('/audio/audio-system.js');
    const { MissionAudioAssets } = await import('/audio/mission-audio-assets.js');
    const loader = new AssetLoader({ baseUrl: new URL('/data/', location.href), fetchFunction: fetch.bind(window) });
    await loader.loadManifest(); const audio = new AudioSystem(new OfflineAudioContext(2, 1, 22050));
    const assets = new MissionAudioAssets(loader, audio); const count = await assets.preloadSounds({ concurrency: 8 });
    return { count, sounds: audio.sounds.size, music: audio.music.size };
  });
  expect(result.count).toBe(218); expect(result.sounds).toBe(218); expect(result.music).toBe(0);
});
