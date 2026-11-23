// Tell 창 본문 폭이 가장 긴 낱말보다 좁게 맞춰져 낱말이 잘리지 않는지 실제 Chromium 에서 확인한다. (2026-11-23 WB04-E1260)
// 원본 폭 맞춤(fit)은 낱말 하나가 후보 폭보다 넓은 경우를 가정하지 않아 한국어의 긴 어절이 본문 폭(Credits 목록 74px)을 넘어 잘렸다: 제목 `NetStorm 크레딧` 의 `NetStorm`(111px)과 캠페인 목록 소개의 `플레이하시겠습니까?`.
// 영어 창 크기는 이 보정 전과 같아야 한다(보정은 맞춘 폭이 낱말보다 좁을 때만 폭을 넓힌다).
const { test, expect } = require('@playwright/test');

// 지정한 언어로 메인 메뉴를 열고 Tell 섹션들의 창 크기와 가장 넓은 줄·본문 폭을 돌려준다.
async function measure(page, lang, names) {
  await page.addInitScript((code) => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, language: code === 'ko' ? 2 : 0 } })), lang);
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto(`/?lang=${lang}&mute&aspect=4:3`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  return page.evaluate((list) => { const app = window.netstorm, out = {};
    for (const name of list) { app._mainMenu.setDialog(null); app._openTell(name); const d = app._mainMenu.dialog; out[name] = d ? { width: Math.round(d.width), body: Math.round(d.body.baseWidth), widest: Math.round(d.body.maximumWidth()) } : null; }
    // 제목 슬롯(3)으로 잰 `NetStorm` 의 폭이다. 한국어도 E1288 부터 원본 Arial 20 글리프 폭이다. (2026-11-23 WB04-E1288)
    const fonts = app._resources.fonts; out.netstormWidth = fonts.measure(fonts.encode('NetStorm'), { flags: 0, font: 3, style: 0, color: 0, extra: 0, shadowColor: 0, shadowX: 0, shadowY: 0 }).width;
    app._mainMenu.setDialog(null); return out; }, names);
}

const NAMES = ['Credits', 'UCampaign', 'Welcome', 'Patch1101.ResetAll', 'Patch1101.Credits', 'ABORT'];

test('Tell 창: 한국어 본문 폭이 가장 긴 줄·낱말보다 좁지 않아 낱말이 잘리지 않는다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  const sizes = await measure(page, 'ko', NAMES);
  for (const name of NAMES) expect(sizes[name], name).not.toBeNull();
  // 좁은 목록 창은 가장 넓은 줄이 본문 폭 안에 들어간다(다른 창의 긴 본문 줄은 줄 나눔 폭 계산이 달라 이 불변식을 쓰지 않는다).
  for (const name of ['Credits', 'UCampaign']) expect(sizes[name].widest, `${name} 가장 넓은 줄`).toBeLessThanOrEqual(sizes[name].body);
  // 이전에는 본문이 74px 로 맞춰져 `NetStorm`(당시 갈무리11 22px 로 111px)·`플레이하시겠습니까?` 가 잘렸다. E1288 부터 `NetStorm` 은 원본 Arial 20 글리프라 그 폭 이상이면 된다.
  expect(sizes.Credits.body).toBeGreaterThanOrEqual(sizes.netstormWidth); expect(sizes.netstormWidth).toBeGreaterThan(60); expect(sizes.UCampaign.body).toBeGreaterThan(100);
  expect(errors).toEqual([]);
});

test('Tell 창: 영어 창 크기는 낱말 보정 전과 같다', async ({ page }) => {
  test.setTimeout(120000);
  const sizes = await measure(page, 'en', NAMES);
  // 보정 전 기준선(E1260 직전 실측): [창 폭, 본문 폭].
  expect([sizes.Credits.width, sizes.Credits.body]).toEqual([234, 129]);
  expect([sizes.UCampaign.width, sizes.UCampaign.body]).toEqual([238, 138]);
  expect([sizes.Welcome.width, sizes.Welcome.body]).toEqual([327, 227]);
  expect([sizes['Patch1101.ResetAll'].width, sizes['Patch1101.ResetAll'].body]).toEqual([241, 141]);
  expect([sizes['Patch1101.Credits'].width, sizes['Patch1101.Credits'].body]).toEqual([373, 273]);
  expect([sizes.ABORT.width, sizes.ABORT.body]).toEqual([420, 360]);
});
