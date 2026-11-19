// 실제 브라우저 앱의 게임 시계 생성·프레임 갱신·반납·재초기화를 확인한다.
const { test, expect } = require('@playwright/test');

test('게임 시계는 앱 프레임과 같은 수명으로 생성되고 재시작된다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(String(error)));
  await page.goto('/');
  await page.waitForFunction(() => window.netstorm?._gameClock?.time().number >= 2);
  const initial = await page.evaluate(() => {
    const app = window.netstorm; window.previousGameClock = app._gameClock; window.previousLocal = app._clientLocal; window.previousRandom = app._clientRandom; window.previousSp = app._clientSp;
    return { frames: app._gameClock.time().number, sameTicks: app._clientLocal.gameTicks().value, spInitialized: app._clientSp.store().initialized, shared: app._campaignSharedServices().randomGlobals === app._clientRandom && app._clientSp.randomGlobals() === app._clientRandom };
  });
  expect(initial.frames).toBeGreaterThanOrEqual(2); expect(initial.sameTicks).toBe(0); expect(initial.spInitialized).toBe(true); expect(initial.shared).toBe(true);
  await page.evaluate(() => window.netstorm.destroy());
  expect(await page.evaluate(() => [window.netstorm._gameClock, window.netstorm._clientLocal, window.netstorm._clientRandom, window.netstorm._clientSp])).toEqual([null, null, null, null]);
  await page.evaluate(() => window.netstorm.init(document.getElementById('netstorm_root')));
  await page.waitForFunction(() => window.netstorm?._gameClock?.time().number >= 2);
  expect(await page.evaluate(() => window.netstorm._gameClock !== window.previousGameClock && window.netstorm._clientLocal !== window.previousLocal && window.netstorm._clientRandom !== window.previousRandom && window.netstorm._clientSp !== window.previousSp)).toBe(true);
  expect(errors).toEqual([]);
});
