// 실제 Options/파일 선택/저장/명시 교체·삭제와 좁은 한국어 모달을 검사한다. 게임 목표 수행은 하지 않는다.
const { test, expect } = require('@playwright/test');

// 설치 중 실행되면 실패하도록 정상 구문의 top-level throw를 포함한다.
function packageFile({ version = '1.0.0', name = '<img src=x onerror=alert(1)>', id = 'test.panel' } = {}) {
    return { name: id + '.nsmod.json', mimeType: 'application/json', buffer: Buffer.from(JSON.stringify({ format: 'netstorm-mod-package', formatVersion: 1, manifest: { id, name, version, apiVersion: '0.1.0', entry: 'main.js', description: 'Line 1\nLine 2' }, modules: { 'main.js': "throw new Error('설치 중 실행 금지'); export default function(api){}" } })) };
}
// 실제 메뉴의 논리 좌표를 캔버스 물리 좌표로 바꾼다.
async function clickLabel(page, label) {
    const point = await page.evaluate((label) => { const app = window.netstorm, point = app._mainMenu.controlPoint(label), rect = app.canvas.getBoundingClientRect(); if (!point) throw new Error('메뉴 단추 없음: ' + label); return { x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight }; }, label);
    await page.mouse.click(point.x, point.y);
}
// 실제 저장 목록의 사본만 읽고 Worker/원래 게임 객체를 변경하지 않는다.
async function stored(page) { return page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.mods.v0') ?? '{"mods":[]}').mods); }
// 독립 문서에서 메뉴까지 정상 rAF로 시작한다. sandbox child에는 저장소 주입을 하지 않는다.
async function start(page, language = 'english', rawMods = null) {
    await page.addInitScript(({ rawMods }) => { if (window !== window.top || sessionStorage.getItem('netstorm.mod-panel-test-seeded')) return; sessionStorage.setItem('netstorm.mod-panel-test-seeded', '1'); localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })); if (rawMods !== null) localStorage.setItem('netstorm.mods.v0', rawMods); }, { rawMods });
    await page.goto('/index.html?title=1&lang=' + language); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 }); await page.locator('canvas').click(); await page.waitForFunction(() => window.netstorm.state === 'mainMenu', null, { timeout: 60000 });
}

test('Options에서 비실행 가져오기/켜기/저장유지/명시교체/삭제와 실패보존이 동작한다', async ({ page }) => {
    test.setTimeout(240000); const errors = []; page.on('pageerror', (error) => errors.push(error.message)); await page.setViewportSize({ width: 1024, height: 768 }); await start(page);
    await clickLabel(page, 'Options'); await clickLabel(page, 'Mods');
    let panel = page.getByRole('dialog', { name: 'Mods', exact: true }); await expect(panel).toBeVisible(); await expect(panel.getByText('No mods installed.')).toBeVisible();
    await expect(panel.getByRole('button', { name: 'Import mod…', exact: true })).toBeEnabled();
    const choosing = page.waitForEvent('filechooser'); await panel.getByRole('button', { name: 'Import mod…', exact: true }).click(); await (await choosing).setFiles(packageFile()); await expect(panel.getByText('Imported. The new mod is off.', { exact: true })).toBeVisible();
    expect((await stored(page))[0].enabled).toBe(false); await expect(panel.getByText('<img src=x onerror=alert(1)>', { exact: true })).toBeVisible(); expect(await panel.locator('img').count()).toBe(0);
    await panel.getByRole('checkbox', { name: 'Enabled: test.panel', exact: true }).check(); await expect.poll(async () => (await stored(page))[0].enabled).toBe(true);
    // 새 문서에서도 저장은 검증만 하므로 켜진 top-level throw가 메뉴 진입을 막지 않는다.
    await page.reload(); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 }); await page.locator('canvas').click(); await page.waitForFunction(() => window.netstorm.state === 'mainMenu');
    await clickLabel(page, 'Options'); await clickLabel(page, 'Mods'); panel = page.getByRole('dialog', { name: 'Mods', exact: true }); await expect(panel.getByRole('checkbox', { name: 'Enabled: test.panel', exact: true })).toBeChecked();
    const before = await page.evaluate(() => localStorage.getItem('netstorm.mods.v0'));
    await panel.getByLabel('Mod package', { exact: true }).setInputFiles(packageFile({ version: '2.0.0', name: 'Replacement' })); await expect(panel.getByRole('button', { name: 'Replace this mod', exact: true })).toBeVisible(); expect(await page.evaluate(() => localStorage.getItem('netstorm.mods.v0'))).toBe(before);
    await panel.getByRole('button', { name: 'Cancel replacement', exact: true }).click(); expect(await page.evaluate(() => localStorage.getItem('netstorm.mods.v0'))).toBe(before);
    await panel.getByLabel('Mod package', { exact: true }).setInputFiles(packageFile({ version: '2.0.0', name: 'Replacement' })); await panel.getByRole('button', { name: 'Replace this mod', exact: true }).click(); await expect(panel.getByText('Replaced. The new mod is off.', { exact: true })).toBeVisible();
    const changed = (await stored(page))[0]; expect(changed.enabled).toBe(false); expect(JSON.parse(changed.package).manifest.version).toBe('2.0.0');
    // 실제 quota 실패 뒤 UI와 원문 모두 이전 목록을 유지한다.
    await page.evaluate(() => { window.netstormUiOriginalSetItem = Storage.prototype.setItem; Storage.prototype.setItem = function (key, value) { if (key === 'netstorm.mods.v0') throw new DOMException('검사 저장 공간 부족', 'QuotaExceededError'); return window.netstormUiOriginalSetItem.call(this, key, value); }; });
    const safe = await page.evaluate(() => localStorage.getItem('netstorm.mods.v0')); await panel.getByRole('checkbox', { name: 'Enabled: test.panel', exact: true }).click(); await expect(panel.getByRole('status')).toContainText('quotaExceeded'); expect(await page.evaluate(() => localStorage.getItem('netstorm.mods.v0'))).toBe(safe); await expect(panel.getByRole('checkbox', { name: 'Enabled: test.panel', exact: true })).not.toBeChecked();
    await page.evaluate(() => { Storage.prototype.setItem = window.netstormUiOriginalSetItem; delete window.netstormUiOriginalSetItem; });
    // 같은 출처의 실제 다른 문서가 바꾼 값을 오래된 UI가 덮지 못하고 명시 재읽기로만 따라간다.
    const peer = await page.context().newPage(); await peer.route('**/mod-storage-peer.html', (route) => route.fulfill({ contentType: 'text/html', body: '<!doctype html><title>Storage peer</title>' })); await peer.goto(new URL('/mod-storage-peer.html', page.url()).href);
    const external = await peer.evaluate(() => { const data = JSON.parse(localStorage.getItem('netstorm.mods.v0')); data.mods[0].enabled = true; const text = JSON.stringify(data); localStorage.setItem('netstorm.mods.v0', text); return text; }); await peer.close(); await page.bringToFront();
    await panel.getByRole('checkbox', { name: 'Enabled: test.panel', exact: true }).click(); await expect(panel.getByRole('status')).toContainText('permissionDenied'); expect(await page.evaluate(() => localStorage.getItem('netstorm.mods.v0'))).toBe(external);
    await panel.getByRole('button', { name: 'Reload list', exact: true }).click(); await expect(panel.getByText('Reloaded.', { exact: true })).toBeVisible(); await expect(panel.getByRole('checkbox', { name: 'Enabled: test.panel', exact: true })).toBeChecked();
    await panel.getByRole('button', { name: 'Delete: test.panel', exact: true }).click(); await expect(panel.getByText('No mods installed.')).toBeVisible(); expect(await stored(page)).toEqual([]);
    await panel.getByLabel('Mod package', { exact: true }).setInputFiles({ name: 'bad.nsmod.json', mimeType: 'application/json', buffer: Buffer.from('{broken') }); await expect(panel.getByRole('status')).toContainText('invalidPackage'); expect(await stored(page)).toEqual([]);
    await panel.getByLabel('Mod package', { exact: true }).setInputFiles({ name: 'large.nsmod.json', mimeType: 'application/json', buffer: Buffer.alloc(1048577) }); await expect(panel.getByRole('status')).toContainText('1 MiB'); expect(await stored(page)).toEqual([]);
    await page.keyboard.press('Escape'); await expect(panel).toHaveCount(0); await expect.poll(() => page.evaluate(() => window.netstorm._gameClock.paused())).toBe(false); expect(errors).toEqual([]);
    await test.info().attach('mod-panel-flow', { body: JSON.stringify({ errors, final: await stored(page) }), contentType: 'application/json' });
});

test('손상된 모드 저장소는 관리 화면에서도 자동 덮어쓰기 없이 보존한다', async ({ page }) => {
    test.setTimeout(180000); await start(page, 'english', 'broken saved mods'); await clickLabel(page, 'Options'); await clickLabel(page, 'Mods');
    const panel = page.getByRole('dialog', { name: 'Mods', exact: true }); await expect(panel.getByText('Saved mods could not be read. Reload the list to try again.')).toBeVisible(); await expect(panel.getByRole('button', { name: 'Import mod…', exact: true })).toBeDisabled();
    await panel.getByRole('button', { name: 'Reload list', exact: true }).click(); await expect(panel.getByRole('status')).toContainText('Could not complete this action.'); expect(await page.evaluate(() => localStorage.getItem('netstorm.mods.v0'))).toBe('broken saved mods');
    await page.keyboard.press('Escape'); await expect(panel).toHaveCount(0);
});

test('한국어 좁은 화면에서 글꼴/줄바꿈/키보드 모달과 앱 반납을 유지한다', async ({ page }) => {
    test.setTimeout(180000); const errors = []; page.on('pageerror', (error) => errors.push(error.message)); await page.setViewportSize({ width: 640, height: 480 }); await start(page, 'korean');
    // 이 경로도 실제 Tell 대상이 같은 패널을 여는지 확인한다. 영어 검사가 실제 Options 행 클릭을 담당한다.
    await page.evaluate(() => window.netstorm._openTell('Patch1101.Mods')); const panel = page.getByRole('dialog', { name: '모드 관리', exact: true }); await expect(panel).toBeVisible();
    await panel.getByLabel('모드 패키지', { exact: true }).setInputFiles(packageFile({ name: '긴 모드 이름 '.repeat(7) })); await expect(panel.getByText('가져왔습니다. 새 모드는 꺼져 있습니다.', { exact: true })).toBeVisible();
    // Tab과 Shift-Tab을 반복해 모두 DOM 모달 안에서만 초점이 이동해야 한다.
    for (const key of ['Tab', 'Tab', 'Tab', 'Shift+Tab', 'Shift+Tab']) { await page.keyboard.press(key); expect(await page.evaluate(() => document.querySelector('[data-netstorm-mods]').contains(document.activeElement))).toBe(true); }
    const layout = await panel.evaluate((node) => { const rect = node.getBoundingClientRect(); return { left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom, scrollWidth: node.scrollWidth, clientWidth: node.clientWidth, font: getComputedStyle(node).fontFamily, ready: document.fonts.check('12px Galmuri11'), paused: window.netstorm._gameClock.paused() }; });
    expect(layout.left).toBeGreaterThanOrEqual(0); expect(layout.top).toBeGreaterThanOrEqual(0); expect(layout.right).toBeLessThanOrEqual(640); expect(layout.bottom).toBeLessThanOrEqual(480); expect(layout.scrollWidth).toBeLessThanOrEqual(layout.clientWidth); expect(layout.font).toContain('Galmuri11'); expect(layout.ready).toBe(true); expect(layout.paused).toBe(true);
    await page.screenshot({ path: test.info().outputPath('korean-mod-panel.png') });
    await page.evaluate(() => window.netstorm.destroy()); await expect(panel).toHaveCount(0); expect(await page.evaluate(() => window.netstorm._modPanel)).toBe(null); expect(errors).toEqual([]);
    await test.info().attach('korean-mod-panel-layout', { body: JSON.stringify({ layout, errors }), contentType: 'application/json' });
});
