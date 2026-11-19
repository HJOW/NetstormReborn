// 실제 브라우저 앱의 게임 시계 생성·프레임 갱신·반납·재초기화를 확인한다.
const { test, expect } = require('@playwright/test');

test('게임 시계는 앱 프레임과 같은 수명으로 생성되고 재시작된다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(String(error)));
  await page.goto('/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart' && window.netstorm?._gameClock?.time().number >= 2);
  const initial = await page.evaluate(() => {
    const app = window.netstorm; window.previousGameClock = app._gameClock; window.previousLocal = app._clientLocal; window.previousRandom = app._clientRandom; window.previousSp = app._clientSp; window.previousPlayers = app._clientPlayers; window.previousTemplates = app._clientTemplates; window.previousCamera = app._clientGameplay.camera(); window.previousPaletteScreen = app._clientPaletteScreen; window.previousColorSort = app._clientColorSort;
    const camera = app._clientGameplay.camera();
    return { frames: app._gameClock.time().number, sameTicks: app._clientLocal.gameTicks().value, spInitialized: app._clientSp.store().initialized, playersInitialized: app._clientPlayers.initialized() && app._clientCoreData.players === app._clientPlayers.records(), surface: Boolean(app._surfaceStorage && app._surfaceStorage.source === app._gameAssets && app._surfaceStorage.processes.usesKernel(app._clientKernel)), shared: app._campaignSharedServices().randomGlobals === app._clientRandom && app._clientSp.randomGlobals() === app._clientRandom,
      camera: [...camera.target().limits, app._layout.logicalWidth, app._layout.logicalHeight, app._clientKencloud.worldRedraw().value, app._clientGameplay.mapRevision().value], palette: [app._clientPaletteScreen.paletteLoaded.value, app._clientColorSort.captured, app._clientColorSort.primary.constructed, app._clientColorSort.logical.every((color, index) => color === app._clientPaletteScreen.logicalPalette()[index]), app._clientColorSort.mapsReady, app._clientColorSort.ownerTables.ready, app._clientColorMapFiles.read('cache/!color.dat').length] };
  });
  expect(initial.frames).toBeGreaterThanOrEqual(2); expect(initial.sameTicks).toBe(0); expect(initial.spInitialized).toBe(true); expect(initial.playersInitialized).toBe(true); expect(initial.surface).toBe(true); expect(initial.shared).toBe(true);
  // Renderer 초기화 뒤 소유자 색표 등록이 같은 redraw 값을 한 번 더 증가시킨다.
  expect(initial.camera).toEqual([4096 - initial.camera[2], 2816 - initial.camera[3], initial.camera[2], initial.camera[3], 2, 1]);
  expect(initial.palette).toEqual([1, true, true, true, true, true, 2304]);
  // 화면 팔레트 변경이 실제 표시기의 같은 색 번호에 전달되는지 확인한 뒤 원래 색을 복구한다.
  const presentedColor = await page.evaluate(async () => {
    const app = window.netstorm, { GamePalette } = await import('/assets/palette.js');
    const rgb = app._gameAssets.palette.rgb.slice(); rgb.set([17, 33, 97], 37 * 3);
    app._clientPaletteScreen.loadPalette(new GamePalette(rgb), 0);
    const bytes = new Uint8Array(app._presenter.lookup.buffer).slice(37 * 4, 37 * 4 + 4);
    app._clientPaletteScreen.loadPalette(app._gameAssets.palette, 0); return [...bytes];
  });
  expect(presentedColor).toEqual([17, 33, 97, 255]);
  // clear=1은 DIB 픽셀을 실제 색 번호 표면과 Canvas에 즉시 내보낸다.
  const cleared = await page.evaluate(() => {
    const app = window.netstorm; app._ensureSurface(); app._surface.fill(9);
    const screen = app._clientPaletteScreen; screen.lock().fill(9); screen.unlock();
    screen.loadPalette(app._gameAssets.palette, 1);
    return [app._surface.pixels[0], ...app._ctx.getImageData(0, 0, 1, 1).data];
  });
  expect(cleared).toEqual([0, 0, 0, 0, 255]);
  // 화면비를 바꿔도 현재 팔레트와 Client 색표 수명은 유지하고 DIB 크기만 바뀐다.
  const resized = await page.evaluate(() => {
    const app = window.netstorm, before = app._clientPaletteScreen, colors = app._clientColorSort, loaded = before.paletteLoaded, palette = [...before.palette()];
    app.setAspect('4:3');
    const narrow = app._clientPaletteScreen, result = [narrow.width, narrow.height, narrow !== before, narrow.paletteLoaded === loaded,
      app._clientColorSort === colors, palette.every((value, index) => narrow.palette()[index] === value)];
    app.setAspect('16:10'); result.push(app._clientPaletteScreen.width, app._clientPaletteScreen.height, app._clientColorSort === colors);
    app.setAspect('16:9'); result.push(app._clientPaletteScreen.width, app._clientPaletteScreen.height, app._clientColorSort === colors);
    return result;
  });
  expect(resized).toEqual([1024, 768, true, true, true, true, 1228, 768, true, 1366, 768, true]);
  // 실제 설정 쓰기는 같은 구름 전역을 즉시 갱신하고 검사 뒤 원래 값을 복구한다.
  const cloudSpeed = await page.evaluate(() => {
    const app = window.netstorm, cloud = app._clientKencloud, previous = cloud.value(1).value;
    app._gameData.setInt('cloudSpeed', 7); const result = cloud.value(1).value;
    app._gameData.set('cloudSpeed', previous); return result;
  });
  expect(cloudSpeed).toBe(7);
  // 준비는 실제 공개 자산에서 스크립트가 지정한 Fort를 읽고 사용 뒤 등록 객체를 반납한다.
  const mission = await page.evaluate(() => {
    const prepared = window.netstorm._prepareCampaignMission('Tutorial1');
    try { return [prepared.script.loaded, prepared.script.fortName()]; } finally { prepared.dispose(); }
  });
  expect(mission).toEqual([true, 'BridgeTheGap']);
  // 실제 앱 Guide의 미션 생성과 해제가 같은 설정·월드 전역에 반영된다.
  const guide = await page.evaluate(async () => {
    const app = window.netstorm; app._clientGuide.begin('Tutorial1');
    const title = app._clientGuide.current().script.get('title'); app._clientGuide.run(); app._clientGuide.run();
    const begun = [app._clientGuide.current().script.fortName(), app._surfaceStorage.links.mission, app._gameData.get('global.inMission'),
      app._clientFort.current(), document.title === app._gameData.pathSpec('missionWindowName', title), app._clientGuide.fortRequest().next[0], app._clientGuide.pending().pending()];
    const { SoundList } = await import('/audio/sound-list.js');
    const unused = () => { throw new Error('검사의 미사용 외부 효과'); };
    const world = app._installCampaignWorld({ localLogin: {}, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: unused, playOnceAt: unused, playLoopAt: unused, stop: unused, isPlaying: () => false } }, { x: 1024, y: 768 });
    const shared = world.session().world().storage === app._surfaceStorage; app._releaseCampaignWorld();
    app._clientGuide.close(); return [begun, app._surfaceStorage.links.mission, app._gameData.get('global.inMission'), shared, app._campaignWorld === null];
  });
  expect(guide).toEqual([['BridgeTheGap', true, '1', 'BridgeTheGap', true, 7, true], false, '0', true, true]);
  await page.evaluate(() => window.netstorm.destroy());
  expect(await page.evaluate(() => [window.netstorm._gameClock, window.netstorm._clientLocal, window.netstorm._clientRandom, window.netstorm._clientSp, window.netstorm._clientPlayers, window.netstorm._clientTemplates, window.netstorm._clientPaletteScreen, window.netstorm._clientColorSort])).toEqual([null, null, null, null, null, null, null, null]);
  await page.evaluate(() => window.netstorm.init(document.getElementById('netstorm_root')));
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart' && window.netstorm?._gameClock?.time().number >= 2);
  expect(await page.evaluate(() => window.netstorm._gameClock !== window.previousGameClock && window.netstorm._clientLocal !== window.previousLocal && window.netstorm._clientRandom !== window.previousRandom && window.netstorm._clientSp !== window.previousSp && window.netstorm._clientPlayers !== window.previousPlayers && window.netstorm._clientTemplates !== window.previousTemplates && window.netstorm._clientGameplay.camera() !== window.previousCamera && window.netstorm._clientGameplay.mapRevision().value === 1)).toBe(true);
  expect(await page.evaluate(() => window.netstorm._clientPaletteScreen !== window.previousPaletteScreen && window.netstorm._clientColorSort !== window.previousColorSort && window.netstorm._clientColorSort.captured && window.netstorm._clientColorSort.mapsReady && window.netstorm._clientColorSort.ownerTables.ready)).toBe(true);
  expect(errors).toEqual([]);
});
