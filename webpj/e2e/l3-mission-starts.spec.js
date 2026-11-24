// L3 시작 상태 대조(전 미션): 31개 미션을 각각 시작해 5·30·120프레임의 Session 덤프를 cpppj 오라클의 정규화 해시(`tests/fixtures/cpppj/candidate-225553df/mission-starts/hashes.json`)와 비교한다.
// 해시 정의는 `tools/l3-mission-starts-collect.mjs` 의 `dumpHash` 와 같다(LF 정규화·`sp`·`player` 행 제외). 시작 시점의 슬롯 순서·객체 수·난수 같은 차이를 한 번에 잡는다.
// 미션마다 브라우저를 새로 열어 약 45초가 걸리므로 전체 실행은 약 25분이다: 옵트인으로, 환경 변수 `NETSTORM_L3_ALL=1` 일 때만 돈다(일부만 보려면 `NETSTORM_L3_ONLY=Tutorial2,bc7sun_bars76`).
// bc7sun_bars76의 로그인 중 보충 기한 예약을 고쳐 모든 미션에 같은 일치 조건을 적용한다. (2026-11-24 WB04-E1313)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { oracleFolder } = require('./l3-common');

// 오라클 해시 모음이다.
const HASHES = JSON.parse(fs.readFileSync(path.join(oracleFolder('mission-starts'), 'hashes.json'), 'utf8'));

// 덤프 문자열의 정규화 해시이다(`dumpHash` 와 같은 정의).
function dumpHash(text) {
  const kept = text.split(/\r?\n/).filter((line) => !(line.startsWith('sp\t') || line.startsWith('player\t')));
  return crypto.createHash('sha256').update(kept.join('\n'), 'utf8').digest('hex');
}

const only = (process.env.NETSTORM_L3_ONLY ?? '').split(',').map((value) => value.trim()).filter(Boolean);
const names = Object.keys(HASHES.missions).filter((name) => only.length === 0 || only.includes(name));

test.describe('전 미션 시작 상태', () => {
  test.skip(process.env.NETSTORM_L3_ALL !== '1' && only.length === 0, '옵트인(NETSTORM_L3_ALL=1 또는 NETSTORM_L3_ONLY=이름,...)');
  test('오라클이 모든 시작 프레임을 가진다', () => {
    expect(HASHES.cpppjCommit).toBe('225553df'); expect(HASHES.frames).toEqual([5, 30, 120]);
    expect(Object.keys(HASHES.missions).length).toBeGreaterThanOrEqual(31);
    for (const [name, frames] of Object.entries(HASHES.missions)) expect(Object.keys(frames).map(Number), name).toEqual(HASHES.frames);
  });
  for (const name of names) {
    test(`${name}: 시작 5·30·120프레임 덤프가 cpppj 와 같다`, async ({ browser }) => {
      test.setTimeout(240000);
      const context = await browser.newContext({ viewport: { width: 1024, height: 768 }, locale: 'en-US' }), page = await context.newPage();
      const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
      await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
      await page.goto('/debug.html?fixed=25&aspect=4:3&title=1');
      await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
      await page.locator('canvas').click();
      await page.evaluate(() => window.netstorm._prepareCampaignAudio());
      while ((await page.evaluate(() => window.netstorm.state)) !== 'mainMenu') await page.evaluate(() => window.netstorm.stepFixedFrames(1));
      await page.evaluate(() => window.netstorm.stepFixedFrames(1));
      await page.evaluate((mission) => window.netstorm._beginCampaignFromDialog(mission), name);
      // 주 루프 프레임 번호 n(메뉴 1프레임 포함)을 1씩 올리며 지정 프레임마다 덤프를 해시한다.
      const actual = {};
      for (let n = 1; n <= HASHES.frames.at(-1); n += 1) {
        if (HASHES.frames.includes(n)) actual[n] = dumpHash(await page.evaluate(() => window.netstorm.sessionDump()));
        if (n < HASHES.frames.at(-1)) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
      }
      await context.close();
      expect(errors).toEqual([]);
      for (const frame of HASHES.frames) expect(actual[frame], `${name} ${frame}프레임 덤프 해시`).toBe(HASHES.missions[name][frame]);
    });
  }
});
