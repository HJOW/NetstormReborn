// 미션이 시작되면 메인 메뉴 곡 대신 원소 곡(wind22·rain22·thu22·sun22)이 재생되고, 다음 곡을 미리 받아 두며, 미션을 떠나면 메뉴 곡으로 돌아가는지 실제 Chromium 에서 확인한다.
// 소리를 들을 수는 없으므로 재생기의 현재 곡·해독 표·구동기 상태를 읽는다. (2026-11-20 WB04-E1119)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('미션 장면 음악: 원소 곡 재생·다음 곡 미리 받기·떠나면 메뉴 곡 복귀', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => { localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 1, music: 1 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1' } })); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // 메뉴 곡이 먼저 재생된다.
  await page.waitForFunction(() => window.netstorm._audio?.currentMusic === 'ser22.mus', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  expect(first).toBeTruthy(); await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  // 원소 곡을 받아 해독해 재생하면 메뉴 곡 대신 그 곡이 현재 곡이다.
  const tracks = ['wind22.mus', 'rain22.mus', 'thu22.mus', 'sun22.mus'];
  await page.waitForFunction((list) => window.netstorm._sceneMusic?.active && list.includes(window.netstorm._audio.currentMusic), tracks, { timeout: 90000 });
  const playing = await page.evaluate(() => ({ current: window.netstorm._audio.currentMusic, selected: window.netstorm._sceneMusic.selected, state: window.netstorm._audio.context.state }));
  expect(playing.current).toBe(playing.selected); expect(playing.state).toBe('running');
  // 다음 원소 곡을 미리 받아 두면 해독 표에 메뉴 곡과 현재·다음 곡만 남는다(쓰지 않는 곡은 놓는다).
  const next = tracks[(tracks.indexOf(playing.current) + 1) % 4];
  await page.waitForFunction((name) => window.netstorm._audio.music.has(name), next, { timeout: 90000 });
  expect(await page.evaluate(() => [...window.netstorm._audio.music.keys()].sort())).toEqual(['ser22.mus', playing.current, next].sort());
  // 벽시계를 곡 길이보다 크게 앞으로 보내면 곡 기한이 지나 다음 원소 곡으로 넘어가고, 지난 곡은 놓으며 그다음 곡을 미리 받는다.
  await page.evaluate(() => { const clock = window.netstorm._gameClock, original = clock.wallSeconds.bind(clock); window.__restoreWall = () => { delete clock.wallSeconds; }; clock.wallSeconds = () => original() + 1000; });
  await page.waitForFunction((name) => window.netstorm._audio.currentMusic === name, next, { timeout: 30000 });
  await page.evaluate(() => window.__restoreWall());
  const afterNext = tracks[(tracks.indexOf(next) + 1) % 4];
  await page.waitForFunction((name) => window.netstorm._audio.music.has(name), afterNext, { timeout: 90000 });
  expect(await page.evaluate(() => [...window.netstorm._audio.music.keys()].sort())).toEqual(['ser22.mus', next, afterNext].sort());
  playing.current = next;
  // 희생 의식이 쓰는 서비스(requestMusic)로 sacrifice.mus 를 요청하면 받아 해독해 그 곡이 현재 곡이 되고, 원소 곡 버퍼는 놓는다(E1120).
  await page.evaluate(() => window.netstorm._campaignGameplayServices().requestMusic('sacrifice.mus'));
  await page.waitForFunction(() => window.netstorm._audio.currentMusic === 'sacrifice.mus', null, { timeout: 60000 });
  expect(await page.evaluate(() => [...window.netstorm._audio.music.keys()].sort())).toEqual(['sacrifice.mus', 'ser22.mus']);
  playing.current = 'sacrifice.mus';
  // 미션 중 음악 설정을 껐다 켜도 메뉴 곡이 끼어들지 않는다.
  await page.evaluate(() => { window.netstorm._settings.set('music', 0); window.netstorm._settings.set('music', 1); });
  await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._audio.currentMusic)).toBe(playing.current);
  // 미션 중 소리(Sound On)를 끄면 원본은 장치를 닫아 음악도 무음이 되고, 다시 켜면 현재 곡을 처음부터 재생한다(E1239).
  await page.evaluate(() => window.netstorm._settings.set('sound', 0));
  await page.waitForFunction(() => window.netstorm._audio.currentMusic === null, null, { timeout: 10000 });
  expect(await page.evaluate(() => window.netstorm._audio.wantedMusic)).toBe(playing.current);
  await page.evaluate(() => window.netstorm._settings.set('sound', 1));
  await page.waitForFunction((name) => window.netstorm._audio.currentMusic === name, playing.current, { timeout: 10000 });
  // 천둥 곡이 시작되면 날씨 효과음은 audio.playSound 가 아니라 미션 SoundPlayer 의 playByName(이름,0,0,0,0,0)을 거친다(게임 메뉴 음소거·동시 재생 한도 적용, E1239).
  // 난수 0.3 → 시작 색인 1 → 다음 곡 색인 2(thu22.mus)이다.
  const thunder = await page.evaluate(() => {
    const app = window.netstorm, player = app._clientSoundPlayer, calls = [], direct = [], play = player.playByName.bind(player), raw = app._audio.playSound.bind(app._audio), random = Math.random;
    player.playByName = (...args) => { calls.push(args); return play(...args); };
    app._audio.playSound = (...args) => { direct.push(args[0]); return raw(...args); };
    Math.random = () => 0.3;
    try { app._beginSceneMusic(); } finally { Math.random = random; delete player.playByName; delete app._audio.playSound; }
    return { calls, direct, selected: app._sceneMusic.selected };
  });
  expect(thunder.selected).toBe('thu22.mus'); expect(thunder.calls).toEqual([['thunderCrack.wav', 0, 0, 0, 0, 0]]); expect(thunder.direct).toEqual([]);
  // 미션을 떠나면 메뉴 곡으로 돌아가고 미션 곡은 놓는다.
  await page.evaluate(() => window.netstorm._leaveCampaignMission());
  await page.waitForFunction(() => window.netstorm._audio.currentMusic === 'ser22.mus' && !window.netstorm._sceneMusic, null, { timeout: 30000 });
  expect(await page.evaluate(() => [...window.netstorm._audio.music.keys()])).toEqual(['ser22.mus']);
  expect(errors).toEqual([]);
});
