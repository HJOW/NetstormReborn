// 실제 창 크기 변경이 논리 표면, Canvas와 입력 좌표에 반영되는지 확인한다. (2026-11-19 WB04-E943)
const { test, expect } = require('@playwright/test');

test('16:9 창을 넓혔다가 줄여도 논리 표면과 포인터 위치가 일치한다', async ({ page }) => {
  const errors = [];
  page.on('pageerror', (error) => errors.push(error.message));
  await page.setViewportSize({ width: 1280, height: 720 });
  await page.goto('/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const canvas = page.locator('#netstorm_root canvas');

  // 시작 화면을 클릭해 실제 표시 표면을 설치한다.
  await canvas.click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await page.setViewportSize({ width: 1920, height: 1080 });
  await expect(canvas).toHaveAttribute('width', '1920');
  await expect(canvas).toHaveAttribute('height', '1080');
  await page.mouse.move(960, 540);
  expect(await page.evaluate(() => {
    const app = window.netstorm;
    return [app._surface.width, app._surface.height, app._input.pointer.x, app._input.pointer.y];
  })).toEqual([1280, 720, 640, 360]);
  await page.setViewportSize({ width: 1280, height: 720 });
  await expect(canvas).toHaveAttribute('width', '1280');
  await expect(canvas).toHaveAttribute('height', '720');
  expect(await page.evaluate(() => [window.netstorm._surface.width, window.netstorm._surface.height])).toEqual([1280, 720]);
  expect(errors).toEqual([]);
});

// 큰 네 화면비에서도 실제 게임 표면·입력 좌표는1280 상한이고 Canvas만 브라우저 크기로 표시한다.
test('네 화면비의 최대 논리 표면과 대형 표시/입력 변환', async ({ page }) => {
  const errors=[]; page.on('pageerror',(error)=>errors.push(error.message));
  await page.addInitScript(()=>localStorage.setItem('netstorm.settings',JSON.stringify({version:1,values:{welcomeGiven:1,tellTips:0,autoDemo:0,sound:0,music:0}})));
  await page.goto('/?startwait=0'); await page.waitForFunction(()=>window.netstorm?.state==='clickToStart'); await page.locator('canvas').click(); await page.waitForFunction(()=>window.netstorm?.state==='mainMenu');
  for(const [width,height,logicalHeight] of [[1920,1080,720],[1920,1200,800],[1920,1440,960],[1280,1024,1024],[3840,2160,720]]) { // 네 화면비와 초대형 창에서 같은 실제 표면을 확인한다.
    await page.setViewportSize({width,height}); await page.waitForFunction(([w,h])=>window.netstorm._layout.logicalWidth===w && window.netstorm._layout.logicalHeight===h,[1280,logicalHeight]);
    await page.mouse.move(width/2,height/2);
    const state=await page.evaluate(()=>{const app=window.netstorm; return {screen:[app._clientPaletteScreen.width,app._clientPaletteScreen.height],surface:[app._surface.width,app._surface.height],pointer:[app._input.pointer.x,app._input.pointer.y]};});
    expect(state).toEqual({screen:[1280,logicalHeight],surface:[1280,logicalHeight],pointer:[640,logicalHeight/2]});
  }
  expect(errors).toEqual([]);
});
