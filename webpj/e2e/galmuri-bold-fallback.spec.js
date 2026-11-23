// 굵은 갈무리11 파일에 없는 글자(가나·라틴 확장·한자 등 8천여 자)가 굵게 요청돼도 같은 갈무리11 일반 글리프로 그려지는지 실제 Chromium 에서 확인한다. (2026-11-23 WB04-E1244)
// 원본 복원 cpppj 1322(BitmapFont::ExtendedGlyph)는 굵은 글꼴에 없는 문자를 같은 갈무리11 일반 파일에서 찾고 시스템 대체나 합성 굵기를 쓰지 않는다.
const { test, expect } = require('@playwright/test');

test('굵은 글꼴에 없는 글자는 합성 굵기 없이 일반 갈무리11 글리프로 대체된다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(String(error)));
  await page.goto('/debug.html');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const result = await page.evaluate(async () => {
    const { createCanvasGlyphRasterizer } = await import('/platform/glyph-rasterizer.js');
    const load = async (path) => { const response = await fetch(`/data/${path}`); if (!response.ok) throw new Error(`글꼴 적재 실패 ${path}`); return new Uint8Array(await response.arrayBuffer()); };
    const [regular, bold] = await Promise.all([load('fonts/Galmuri11.woff2'), load('fonts/Galmuri11-Bold.woff2')]);
    const rasterizer = await createCanvasGlyphRasterizer({ document, regular, bold });
    const spec = { emSize: 11, ascent: 9, height: 12, italic: false };
    const draw = (code, isBold) => { const glyph = rasterizer.glyph(code, { ...spec, bold: isBold }); return { advance: glyph.advance, left: glyph.left, top: glyph.top, width: glyph.width, height: glyph.height, sum: glyph.coverage.reduce((total, value) => total + value, 0), coverage: [...glyph.coverage].join(',') }; };
    // 굵은 파일에 없는 가나(あ), 그리스(α), 키릴(Ж), 호환 자모(ㅥ U+3165) 와 굵은 파일에 있는 한글(한), 라틴(A) 이다.
    const out = { faces: [...document.fonts].filter((face) => face.family.replaceAll('"', '').startsWith('Galmuri11')).map((face) => `${face.family.replaceAll('"', '')}/${face.weight}`).sort() };
    for (const [name, code] of [['kana', 0x3042], ['greek', 0x3b1], ['cyrillic', 0x416], ['jamo', 0x3165], ['hangul', 0xd55c], ['latin', 0x41]]) out[name] = { regular: draw(code, false), bold: draw(code, true) };
    rasterizer.dispose();
    return out;
  });
  // 세 FontFace(일반 400·굵은 700·대체 100 900)가 등록된다.
  expect(result.faces).toEqual(['Galmuri11/400', 'Galmuri11/700', 'Galmuri11Regular/100 900']);
  // 굵은 파일에 없는 글자는 굵게 요청해도 일반 글리프와 모양·전진 폭이 완전히 같고(시스템 글꼴·합성 굵기 아님) 비어 있지 않다.
  for (const name of ['kana', 'greek', 'cyrillic', 'jamo']) {
    expect(result[name].regular.sum, `${name} 일반 글리프가 있다`).toBeGreaterThan(0);
    expect(result[name].bold, `${name} 굵게=일반`).toEqual(result[name].regular);
  }
  // 굵은 파일에 있는 글자는 실제 굵은 글리프를 써서 일반과 다르다(대체가 굵은 글리프를 가리지 않는다).
  for (const name of ['hangul', 'latin']) {
    expect(result[name].bold.sum, `${name} 굵은 글리프가 있다`).toBeGreaterThan(0);
    expect(result[name].bold.coverage, `${name} 굵게≠일반`).not.toBe(result[name].regular.coverage);
  }
  expect(errors).toEqual([]);
});
