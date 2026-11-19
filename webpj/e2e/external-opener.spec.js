// 실제 사용자 클릭에서 웹 주소의 새 탭 열기와 브라우저 차단을 확인한다. (2026-11-19 WB04-E942)
const { test, expect, errors } = require('@playwright/test');

// 테스트 전용 문서에 운영 링크 판정·열기 모듈을 설치한다. blocked이면 문서 정책에서 팝업 권한을 제외한다.
async function openProbe(page, blocked) {
  const headers = blocked ? { 'Content-Security-Policy': 'sandbox allow-scripts allow-same-origin' } : {};
  await page.route('**/link-probe.html', (route) => route.fulfill({
    contentType: 'text/html; charset=utf-8',
    headers,
    body: '<!doctype html><button>링크 열기</button><output></output>',
  }));
  await page.goto('/link-probe.html');
  await page.evaluate(async () => {
    const { openExternalLink } = await import('/platform/external-opener.js');
    const { resolveExternalLink } = await import('/ui/external-link.js');
    // 같은 테스트 서버를 다른 호스트명으로 열어 출처가 다른 탭을 검사한다.
    const target = new URL('/link-destination.html', location.href);
    target.hostname = '127.0.0.1';
    document.querySelector('button').addEventListener('click', () => {
      document.querySelector('output').value = String(openExternalLink(document, resolveExternalLink(target.href)));
    });
  });
}

test('웹 링크를 별도 탭에 열고 원래 문서와 주소를 격리한다', async ({ page, context }) => {
  await context.route('**/link-destination.html', (route) => route.fulfill({
    contentType: 'text/html; charset=utf-8',
    body: '<!doctype html><title>링크 목적지</title>',
  }));
  await openProbe(page, false);
  // opener가 분리되므로 대상 탭은 브라우저 컨텍스트에서 찾는다.
  const [destination] = await Promise.all([
    context.waitForEvent('page'),
    page.getByRole('button').click(),
  ]);
  await expect(destination).toHaveTitle('링크 목적지');
  expect(await destination.evaluate(() => ({ opener: window.opener, referrer: document.referrer })))
    .toEqual({ opener: null, referrer: '' });
  await expect(page).toHaveURL(/\/link-probe\.html$/);
  await expect(page.locator('output')).toHaveText('true');
});

test('브라우저가 팝업을 차단해도 현재 문서는 계속 동작한다', async ({ page, context }) => {
  await openProbe(page, true);
  // 차단된 경우 새 탭이 생기지 않는다. 다른 브라우저 오류는 그대로 실패로 처리한다.
  const popup = context.waitForEvent('page', { timeout: 750 }).catch((error) => {
    if (error instanceof errors.TimeoutError) return null;
    throw error;
  });
  await page.getByRole('button').click();
  await expect(page.locator('output')).toHaveText('true');
  expect(await popup).toBeNull();
  expect(context.pages()).toEqual([page]);
  await expect(page).toHaveURL(/\/link-probe\.html$/);
});
