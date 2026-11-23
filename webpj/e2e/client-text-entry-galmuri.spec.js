// Chromium에서 실제 갈무리11과 Client 편집기·입력 큐·픽셀 출력을 함께 확인한다.
const { test, expect } = require('@playwright/test');

test('실제 갈무리11 편집 입력과 Gump 픽셀 출력', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(String(error)));
  await page.goto('/debug.html');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart' && window.netstorm?._clientGump);
  const result = await page.evaluate(async () => {
    const [{ FontSet }, { UnicodeFontProvider }, { createCanvasGlyphRasterizer }, { displayTextCodec }, { queueCommittedText },
      { ClientTextEntryControls }, { createClientTextOutput }, { TextEntryGumpState }, { ButtonGumpState }, { StoneButtonGumpState },
      { ScrollGumpState }, { ScrollGumpWidgetState }, { GumpDraw, GumpDrawState, GumpPixelTarget, setGumpDrawClip },
      { GumpBackgroundDraw, GumpBackgroundDrawState }, { GumpGlyphColorState }] = await Promise.all([
      import('/assets/font-set.js'), import('/assets/unicode-font.js'), import('/platform/glyph-rasterizer.js'), import('/assets/display-text.js'), import('/core/display-text-input.js'),
      import('/ui/client-text-entry-controls.js'), import('/ui/client-text-entry-draw.js'), import('/ui/text-entry-gump.js'), import('/ui/button-gump.js'), import('/ui/stone-button-gump.js'),
      import('/ui/scroll-gump.js'), import('/ui/scroll-gump-widgets.js'), import('/render/gump-draw.js'), import('/render/gump-background-draw.js'), import('/render/gump-text-draw.js')]);
    // 배포된 글꼴 바이트를 실제 FontFace와 원본 비트맵 글꼴에 공급한다.
    const load = async (path) => { const response = await fetch(`/data/${path}`); if (!response.ok) throw new Error(`글꼴 적재 실패 ${path}`); return new Uint8Array(await response.arrayBuffer()); };
    const [regular, bold, bitmap] = await Promise.all([load('fonts/Galmuri11.woff2'), load('fonts/Galmuri11-Bold.woff2'), load('d/!Arial.normal.14.0.chfnt')]);
    const rasterizer = await createCanvasGlyphRasterizer({ document, regular, bold });
    const fonts = new FontSet(new Map([['d/!Arial.normal.14.0.chfnt', bitmap]]), { unicode: new UnicodeFontProvider(rasterizer) });
    const app = window.netstorm, owner = app._clientGump, drawState = new GumpDrawState(), pixels = new Uint8Array(240 * 100);
    setGumpDrawClip(drawState, { left: 0, top: 0, right: 240, bottom: 100 });
    const target = new GumpPixelTarget(pixels, 240, 100, drawState);
    const background = new GumpBackgroundDraw(owner.collection(), drawState, new GumpBackgroundDrawState(), { node: (token) => owner.node(token), fill: (...args) => target.fill(...args) });
    const controls = new ClientTextEntryControls(owner, { entryState: new TextEntryGumpState(), buttonState: new ButtonGumpState(), stoneState: new StoneButtonGumpState(), scrollState: new ScrollGumpState(), widgetState: new ScrollGumpWidgetState(),
      backgroundColor: () => 35, now: () => 1, poll: () => ({ code: 0 }), sound: () => {}, baseEvent: () => 0, fonts, ...createClientTextOutput(owner, fonts, background, target, new GumpGlyphColorState()) });
    const token = controls.allocate(); controls.construct(token, 150, 60, null, 5, 0x04000005, 42); owner.attach(token, 10, 10);
    const node = owner.node(token), buffer = controls.storage().find(node.buffer), queue = app._clientInputQueue;
    // 브라우저 확정 문자 사건을 원본 큐에서 꺼내 실제 공통 입력으로 전달한다.
    if (!queue.empty || !queueCommittedText(queue, 'A한글B', 0, 0)) throw new Error('편집 검사 입력 큐 준비 실패');
    while (!queue.empty) { const event = queue.pop(false); owner.input().capture(token); owner.input().route(event.code, event.x, event.y); }
    const bytes = Uint8Array.from(buffer.text(), (character) => character.charCodeAt(0)), decoded = displayTextCodec.decode(bytes);
    new GumpDraw(owner.collection(), drawState, { draw: (current) => controls.backend().draw(current) }).draw(token);
    const provider = fonts.unicodeFont({ font: 5, style: 0 }), glyph = provider.glyph('한'.codePointAt(0));
    const result = { decoded, glyphPixels: glyph.image?.opacity.filter((value) => value === 255).length ?? 0,
      inkPixels: pixels.filter((value) => value === 42).length, background: pixels[65 * 240 + 20], nodeCount: owner.createState().count };
    controls.close(); result.cleaned = owner.createState().count === 1 && controls.storage().bufferCount() === 0; rasterizer.dispose(); return result;
  });
  expect(result.decoded).toBe('A한글B'); expect(result.glyphPixels).toBeGreaterThan(0); expect(result.inkPixels).toBeGreaterThan(0);
  expect(result.background).toBe(5); expect(result.nodeCount).toBe(14); expect(result.cleaned).toBe(true); expect(errors).toEqual([]);
});
