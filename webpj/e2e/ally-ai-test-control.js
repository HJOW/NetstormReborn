// 사용자 허용 미션의 실제 AI만 켜는 e2e 전용 설정이다. debug.html의 ES 모듈 앱에서 미션 진입 전에 설치한다.
const { assertAllyAiMission, allyAiCondition } = require('./ally-ai-scope');
// preserveDifficulty=true이면 현재 스커미시/옵션 난이도를 그대로 사용해 배율을 관찰한다. 기본값은 기존 아군 어려움·적 쉬움이다.
async function installAllyAi(page, mission, { preserveDifficulty = false, profile = 'default' } = {}) {
  // 최신 사용자 범위를 공용 CLI와 공유하고 실제 페이지 접근 전에 검사한다.
  const normalized = assertAllyAiMission(mission);
  allyAiCondition(0, profile);
  if (typeof preserveDifficulty !== 'boolean') throw new TypeError('아군 AI 검사 난이도 보존 선택은 boolean이어야 한다');
  await page.evaluate(async ({ name, preserveDifficulty, profile }) => {
    // 원래 시작 몸체와 설정 초기화기를 그대로 빌린다.
    const { RawCampaignMissionStartBindings } = await import(new URL('./core/raw-campaign-mission-start-bindings.js', location.href));
    const { RawMissionAiSettings, MISSION_AI_SETTINGS_SIZE } = await import(new URL('./core/raw-mission-ai-settings.js', location.href));
    const { AI_SETTINGS_ALLY_MASK_OFFSET } = await import(new URL('./core/ai-alliance-side.js', location.href));
    const app = window.netstorm, gameplay = app._campaignGameplayServices, original = RawCampaignMissionStartBindings.prototype.start;
    if (app._allyAiTestInstalled) throw new Error('같은 페이지의 아군 AI 검사 중복 설치');
    app._allyAiTestInstalled = true; // 한 페이지에는 한 검사 미션만 설치해 prototype/난이도 감싸기를 중복하지 않는다.
    // 현재 조사 미션에서만 C++ 테스트 옵션과 같은 아군 어려움·적 쉬움을 사용한다.
    if (!preserveDifficulty) app._campaignGameplayServices = function () {
      const services = gameplay.call(this), difficulty = services.ai.difficulty;
      services.ai.difficulty = (owner, enemy) => this._clientGuide?.current()?.script.name.toLowerCase() === name ? (enemy ? 1 : 3) : difficulty(owner, enemy);
      return services;
    };
    // 실제 원본 Start가 끝난 같은 시점에 현재 Player의 사제·설정·지갑을 읽어 AI를 만든다.
    RawCampaignMissionStartBindings.prototype.start = function () {
      original.call(this);
      if (app._clientGuide?.current()?.script.name.toLowerCase() !== name) return;
      const session = app._campaignWorld.session(), owner = session.localOwner(), records = session.playerRecords();
      if (owner < 1 || owner > 8 || session.editor() || !this.findLivePriest(owner)) throw new Error('검사 아군 AI 생성 전제 오류');
      // 현재 이름·동맹·SP·기술을 먼저 읽고 기본 전략에서는 시나리오의 기존 brain을 재사용한다.
      const currentName = records.subarray(owner * 180 + 12, owner * 180 + 44), nul = currentName.indexOf(0), length = Math.min(nul < 0 ? currentName.length : nul, 29);
      const players = new DataView(records.buffer, records.byteOffset, records.byteLength), money = session.sp(owner);
      // 실제 기술 목록과 첫 NUL까지의 이름을 읽어 생성 자체가 진행을 바꾸지 않았는지 확인한다.
      // 실제 FindTechnology의 유효 TYPE은1 이상이며0은 원문 범위 진단 대상이다.
      const technology = () => Array.from({ length: session.core().types.length - 1 }, (_, index) => Boolean(session.findTechnology(owner, index + 1)));
      const before = { money, allies: players.getUint32(owner * 180 + 0x54, true), name: Array.from(currentName.subarray(0, length)), technology: technology() };
      const existing = session.participation().brain(owner);
      if (existing && profile !== 'default') throw new Error('기존 아군 AI의 전략 변경은 지원하지 않는다');
      let token = existing;
      if (!token) {
        const settings = new Uint8Array(MISSION_AI_SETTINGS_SIZE);
        // lean-guard는 원래 owner 설정의 Collectors 한 키만 제공하며 미션 공통/적 설정을 빌리지 않는다.
        const initializer = new RawMissionAiSettings(session.missionAiSettings(), { get: (key) => profile === 'lean-guard' && key === `ai${owner}Collectors` ? '1' : '' });
        initializer.initialize(settings);
        // 실제 설정 로더로 수집자 목표만3→1로 줄인다. 자금·기술 키는 빈값이므로 바뀌지 않는다.
        if (profile === 'lean-guard') initializer.apply(settings, owner);
        // 원래 aiAbility 설정의 사제 채취 능력만 끈다. 사제 이동/건설·운반체·적 AI의 규칙은 그대로다.
        if (profile === 'guard-priest' || profile === 'lean-guard') initializer.setAbility(settings, 'USE_PRIEST_TO_COLLECT', false);
        // 초기 갱신이 현재 이름·동맹·SP를 덮어 바꾸지 않게 C++ PrepareLocalTestAiSettings와 같이 준비한다.
        settings.fill(0, 0, 30); settings.set(currentName.subarray(0, length), 0); settings[0x1e] = 0;
        const view = new DataView(settings.buffer);
        view.setUint32(0x22c, Math.max(0, Math.min(0x7fffffff, Number.isFinite(money) ? Math.trunc(money) : 0)), true);
        view.setUint32(AI_SETTINGS_ALLY_MASK_OFFSET, players.getUint32(owner * 180 + 0x54, true), true);
        // 강제 적·신전 금지·색 덮기·AI 중지의 기본0을 명시한다.
        for (const offset of [0x240, 0x244, 0x250, 0x254]) view.setUint32(offset, 0, true);
        token = session.createAi(owner, settings);
      }
      // 결과를 읽기만 하며 기술·SP·HP·게임 단계·승패를 직접 변경하지 않는다.
      const brain = session.aiMemory().bytes(token), view = new DataView(brain.buffer, brain.byteOffset, brain.byteLength);
      // 실제 본체의 능력·수집자 목표도 읽어 CLI의 조건/장시간 관찰과 연결한다.
      const abilities = view.getUint32(0x248, true), collectors = view.getInt32(0x234, true);
      window.netstormAllyTestState = { owner, token, reused: Boolean(existing), profile, abilities, collectors, before, money: session.sp(owner), allies: players.getUint32(owner * 180 + 0x54, true), name: Array.from(records.subarray(owner * 180 + 12, owner * 180 + 12 + length)), technology: technology(), brain: session.participation().brain(owner) };
    };
  }, { name: normalized, preserveDifficulty, profile });
}
module.exports = { installAllyAi };
