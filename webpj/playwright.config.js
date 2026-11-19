// Playwright E2E 테스트 설정. 단위 테스트(tests/*.test.mjs, node --test)와 분리하여 e2e 디렉토리만 실행한다.
const { defineConfig, devices } = require('@playwright/test');

// server.js 가 사용하는 고정 포트 (server.js 의 PORT 와 같아야 함)
const PORT = 9803;

// 호환성 점검에는 설치된 Chrome·Edge와 Playwright Firefox를 추가한다. 평소에는 Chromium으로 실행한다. (2026-11-19 WB04-E945)
const extraProjects = process.env.NETSTORM_E2E_MATRIX === '1' ? [
  { name: 'chrome', use: { ...devices['Desktop Chrome'], channel: 'chrome' } },
  { name: 'edge', use: { ...devices['Desktop Edge'], channel: 'msedge' } },
  { name: 'firefox', use: { ...devices['Desktop Firefox'] } },
] : [];

module.exports = defineConfig({
  testDir: './e2e',
  outputDir: './test-results',
  timeout: 60000,
  expect: { timeout: 10000 },
  fullyParallel: true,
  reporter: [['list'], ['html', { open: 'never', outputFolder: 'playwright-report' }]],
  use: {
    baseURL: `http://localhost:${PORT}`,
    trace: 'retain-on-failure',
    screenshot: 'only-on-failure'
  },
  // 테스트 시작 시 정적 서버를 자동으로 띄운다. 이미 실행 중이면 재사용한다.
  webServer: {
    command: 'node server.js',
    url: `http://localhost:${PORT}/`,
    reuseExistingServer: true,
    timeout: 30000
  },
  projects: [
    { name: 'chromium', use: { ...devices['Desktop Chrome'] } },
    ...extraProjects,
  ]
});
