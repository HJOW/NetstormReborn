// 실제 브라우저 앱의 게임 시계 생성·프레임 갱신·반납·재초기화를 확인한다.
const { test, expect } = require('@playwright/test');

test('게임 시계는 앱 프레임과 같은 수명으로 생성되고 재시작된다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(String(error)));
  await page.goto('/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart' && window.netstorm?._gameClock?.time().number >= 2);
  const initial = await page.evaluate(() => {
    const app = window.netstorm; window.previousGameClock = app._gameClock; window.previousLocal = app._clientLocal; window.previousRandom = app._clientRandom; window.previousSp = app._clientSp; window.previousPlayers = app._clientPlayers; window.previousTemplates = app._clientTemplates; window.previousCamera = app._clientGameplay.camera(); window.previousPaletteScreen = app._clientPaletteScreen; window.previousColorSort = app._clientColorSort; window.previousGump = app._clientGump;
    const camera = app._clientGameplay.camera();
    return { frames: app._gameClock.time().number, sameTicks: app._clientLocal.gameTicks().value, spInitialized: app._clientSp.store().initialized, playersInitialized: app._clientPlayers.initialized() && app._clientCoreData.players === app._clientPlayers.records(), surface: Boolean(app._surfaceStorage && app._surfaceStorage.source === app._gameAssets && app._surfaceStorage.processes.usesKernel(app._clientKernel)), shared: app._campaignSharedServices().randomGlobals === app._clientRandom && app._clientSp.randomGlobals() === app._clientRandom,
      camera: [...camera.target().limits, app._layout.logicalWidth, app._layout.logicalHeight, app._clientKencloud.worldRedraw().value, app._clientGameplay.mapRevision().value], palette: [app._clientPaletteScreen.paletteLoaded.value, app._clientColorSort.captured, app._clientColorSort.primary.constructed, app._clientColorSort.logical.every((color, index) => color === app._clientPaletteScreen.logicalPalette()[index]), app._clientColorSort.mapsReady, app._clientColorSort.ownerTables.ready, app._clientColorMapFiles.read('cache/!color.dat').length] };
  });
  expect(initial.frames).toBeGreaterThanOrEqual(2); expect(initial.sameTicks).toBe(0); expect(initial.spInitialized).toBe(true); expect(initial.playersInitialized).toBe(true); expect(initial.surface).toBe(true); expect(initial.shared).toBe(true);
  // Renderer 초기화 뒤 소유자 색표 등록이 같은 redraw 값을 한 번 더 증가시키고, 설정의 useBanners 기본값 0(PE 초기값 1에서 바뀜)을 적용하는 소유자 표식 옵션이 한 번 더 증가시켜 3이다. (2026-11-21 WB04-E1134, 20261120-46 이후 기대값 갱신)
  expect(initial.camera).toEqual([4096 - initial.camera[2], 2816 - initial.camera[3], initial.camera[2], initial.camera[3], 3, 1]);
  expect(initial.palette).toEqual([1, true, true, true, true, true, 2304]);
  expect(await page.evaluate(() => [window.netstorm._clientGump.createState().count, window.netstorm._clientGump.root().width, window.netstorm._clientGump.root().height])).toEqual([1, initial.camera[2], initial.camera[3]]);
  // 실제 Client Gump 트리의 자식에 입력을 전달하고 지연 삭제한다.
  const gumpInput = await page.evaluate(() => {
    const app = window.netstorm, owner = app._clientGump, events = [];
    app._displayDirty = false; const token = owner.createBase(1, 20, 10, 0x401, (event) => { events.push(event); return 1; });
    owner.attach(token, 10, 20); const routed = owner.input().route(42, 0, 0); owner.remove(token);
    return [routed === token, events[0], owner.root().child, owner.createState().count, app._displayDirty];
  });
  expect(gumpInput).toEqual([true, 42, 0, 1, true]);
  // 배경 파생 생성기와 입력·소멸자는 같은 실제 노드 주소를 사용한다.
  const derivedGump = await page.evaluate(async () => {
    const { GumpBackgroundNode, GumpBackgroundCreate } = await import('/ui/gump-background-create.js');
    const owner = window.netstorm._clientGump, node = new GumpBackgroundNode(), calls = [];
    const token = owner.allocateDerived(node, (event) => { calls.push(event); return 1; }, (current, flags) => { calls.push('destroy'); return owner.lifetime().deleteScalar(current, flags); });
    const background = new GumpBackgroundCreate(owner.createState(), owner.factory(), () => 55, { node: (current) => owner.node(current) });
    background.construct(token, 20, 10, 0, 0x401, 0, 0); owner.attach(token, 10, 20);
    const result = [owner.node(token) === node, owner.input().route(43, 0, 0) === token]; owner.remove(token);
    return [...result, ...calls, owner.node(token) === null];
  });
  expect(derivedGump).toEqual([true, true, 43, 'destroy', true]);
  // 별도 Client 수명에서 실제 Uber 파생 루트를 공개하고 그 처리기에 입력한다.
  const replacedGump = await page.evaluate(async () => {
    const { ClientGumpRoot } = await import('/ui/client-gump-root.js'), { UberRootNode } = await import('/ui/uber-root-gump.js');
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), root = new UberRootNode();
    const token = owner.allocateDerived(root, () => 1, (value, flags) => owner.lifetime().deleteScalar(value, flags));
    owner.factory().construct(token, 1, 100, 80, 0x401); owner.createState().entries[root.id] = 0; root.id = 0; owner.replaceRoot(token);
    const result = [owner.root() === root, owner.createState().entries[0] === token, owner.input().route(42, 0, 0) === token]; owner.dispose(); return result;
  });
  expect(replacedGump).toEqual([true, true, true]);
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
    app.setAspect('16:10'); result.push(app._clientPaletteScreen.width, app._clientPaletteScreen.height, app._clientColorSort === colors, app._clientGump.root().width);
    app.setAspect('16:9'); result.push(app._clientPaletteScreen.width, app._clientPaletteScreen.height, app._clientColorSort === colors);
    return result;
  });
  expect(resized).toEqual([1024, 768, true, true, true, true, 1228, 768, true, 1228, 1366, 768, true]);
  // 실제 설정 쓰기는 같은 구름 전역을 즉시 갱신하고 검사 뒤 원래 값을 복구한다.
  const cloudSpeed = await page.evaluate(() => {
    const app = window.netstorm, cloud = app._clientKencloud, previous = cloud.value(1).value;
    app._gameData.setInt('cloudSpeed', 7); const result = cloud.value(1).value;
    app._gameData.set('cloudSpeed', previous); return result;
  });
  expect(cloudSpeed).toBe(7);
  // 실제 구름·파일·Screen 팔레트 전환과 같은 이름의 자연 반환을 브라우저에서 확인한다.
  const battlePalette = await page.evaluate(() => {
    const app = window.netstorm, screen = app._clientPaletteScreen;
    app._surface.fill(9); screen.lock().fill(9); screen.unlock();
    app._changeClientFortPalette(app._gameData.pathSpec('battlePal'));
    const cache = app._clientKencloud.globals().cache, cleared = app._surface.pixels[0];
    app._surface.fill(6); app._changeClientFortPalette('GIFCLOUD');
    return [Boolean(cache), cleared, app._clientKencloud.globals().cache === cache, app._surface.pixels[0]];
  });
  expect(battlePalette).toEqual([true, 0, true, 6]);
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
  expect(await page.evaluate(() => [window.netstorm._gameClock, window.netstorm._clientLocal, window.netstorm._clientRandom, window.netstorm._clientSp, window.netstorm._clientPlayers, window.netstorm._clientTemplates, window.netstorm._clientPaletteScreen, window.netstorm._clientColorSort, window.netstorm._clientGump])).toEqual([null, null, null, null, null, null, null, null, null]);
  await page.evaluate(() => window.netstorm.init(document.getElementById('netstorm_root')));
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart' && window.netstorm?._gameClock?.time().number >= 2);
  expect(await page.evaluate(() => window.netstorm._gameClock !== window.previousGameClock && window.netstorm._clientLocal !== window.previousLocal && window.netstorm._clientRandom !== window.previousRandom && window.netstorm._clientSp !== window.previousSp && window.netstorm._clientPlayers !== window.previousPlayers && window.netstorm._clientTemplates !== window.previousTemplates && window.netstorm._clientGameplay.camera() !== window.previousCamera && window.netstorm._clientGameplay.mapRevision().value === 1)).toBe(true);
  expect(await page.evaluate(() => window.netstorm._clientPaletteScreen !== window.previousPaletteScreen && window.netstorm._clientColorSort !== window.previousColorSort && window.netstorm._clientColorSort.captured && window.netstorm._clientColorSort.mapsReady && window.netstorm._clientColorSort.ownerTables.ready)).toBe(true);
  expect(await page.evaluate(() => window.netstorm._clientGump !== window.previousGump && window.netstorm._clientGump.createState().count === 1)).toBe(true);
  expect(errors).toEqual([]);
});
