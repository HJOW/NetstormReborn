// 한국어 큰 슬롯(제목, 슬롯 3·4)의 글자 크기가 갈무리11 설계 격자(12px)의 정수배이고 글자 윗부분이 줄 맨 위 밖으로 나가 잘리지 않는지 실제 Chromium 의 실제 글꼴·래스터라이저로 확인한다.
// (2026-11-23 WB04-E1259, WB04-E1288: 기준 크기 11 → 12 로 바꿔 24·48px 과 폭 12 배수를 기대)
// 이전에는 제목이 19px(슬롯 3)·45px(슬롯 4)이고 기준선이 원본 Arial 값이라 한글 윗부분이 줄 위로 3줄 나가 잘려 `잔`→`산`, `라`→`다`, `혹`→`폭` 처럼 읽혔다.
const { test, expect } = require('@playwright/test');

test('한국어 제목 슬롯: 갈무리11 정수 배율이고 글자 윗부분이 줄 밖으로 잘리지 않는다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, language: 2 } })));
  await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  const result = await page.evaluate(() => {
    const fonts = window.netstorm._resources.fonts, out = {};
    // 가나다순으로 고르게 뽑은 한글 음절 120개와 문제였던 글자(잔·혹·라·구·한)를 본다.
    const codes = [0xc794, 0xd639, 0xb77c, 0xad6c, 0xd55c]; for (let code = 0xac00; code <= 0xd7a3; code += 93) codes.push(code);
    for (const slot of [3, 4]) {
      const font = fonts.unicodeFont({ font: slot, style: 0 }); let minTop = Infinity, maxBottom = -Infinity, inked = 0;
      // 글자 그림 영역에는 문턱 아래의 옅은 가장자리 줄이 포함될 수 있어, 실제로 잉크(indices 비영)가 있는 첫 줄·끝 줄 기준으로 줄 맨 위(0)와 비교한다.
      for (const code of codes) {
        const image = font.glyph(code).image; if (!image) continue;
        let first = -1, last = -1; for (let y = 0; y < image.height; y += 1) { let row = false; for (let x = 0; x < image.width; x += 1) if (image.indices[y * image.width + x]) { row = true; break; } if (row) { if (first < 0) first = y; last = y; } }
        if (first < 0) continue; inked += 1; minTop = Math.min(minTop, image.top + first); maxBottom = Math.max(maxBottom, image.top + last);
      }
      out[slot] = { em: font.spec.emSize, height: font.height, ascent: font.ascent, minTop, maxBottom, inked };
    }
    // 슬롯별 확장 글자(한글 음절 120개·호환 자모 U+3131~314E·한자 U+4E00·6F22·AC00 등)의 전진 폭 집합이다.
    out.advances = {}; const extended = [...codes, 0x3131, 0x3141, 0x314e, 0x4e00, 0x6f22, 0x97d3];
    for (const slot of [0, 3, 4]) { const font = fonts.unicodeFont({ font: slot, style: 0 }); out.advances[slot] = [...new Set(extended.map((code) => font.glyph(code).advance))]; }
    // 잔의 맨 윗줄 잉크 폭이다(ㅈ 의 위쪽 가로획이 남아 있으면 10칸 이상).
    const g = fonts.unicodeFont({ font: 3, style: 0 }).glyph(0xc794).image; let topRun = 0; if (g) { let first = 0; while (first < g.height && !g.indices.slice(first * g.width, (first + 1) * g.width).some(Boolean)) first += 1; for (let x = 0; x < g.width; x += 1) if (g.indices[first * g.width + x]) topRun += 1; out.jan = { top: g.top, first, topRun }; }
    return out; });
  // 글자 크기는 12 의 정수배(슬롯 3 → 24px, 슬롯 4 → 48px). (E1288)
  expect([result[3].em, result[4].em]).toEqual([24, 48]);
  // 어떤 글자도 줄 맨 위(0) 위로 나가지 않는다.
  expect(result[3].minTop, '슬롯 3 윗부분 잘림 없음').toBeGreaterThanOrEqual(0); expect(result[4].minTop, '슬롯 4 윗부분 잘림 없음').toBeGreaterThanOrEqual(0);
  expect(result[3].inked).toBeGreaterThan(100);
  // 확장 글자(한글 음절·호환 자모·한자)의 전진 폭은 모두 칸 폭(= 글자 크기)이다: 원문은 확장 글자 하나의 폭(`가` 의 폭)을 대표로 모든 확장 문자의 폭으로 쓰므로(`BitmapFont::ExtendedAdvance`) 같은 결과이다.
  expect(result.advances, '확장 글자 폭').toEqual({ 0: [12], 3: [24], 4: [48] });
  // 잔(ㅈ)의 맨 윗줄에 가로획이 있다.
  expect(result.jan.topRun, '잔 의 위쪽 가로획').toBeGreaterThanOrEqual(10);
  expect(errors).toEqual([]);
});
