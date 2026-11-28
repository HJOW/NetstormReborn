// 캠페인 1-1~1-5와 스커미시에서만 실제 아군 AI의 공격·포획·희생·승리 검사를 허용한다.
const ALLY_AI_MISSIONS = new Set(['thewarbegins', 'masterofwhirligigs', 'savetheisland', 'fragilefortune', 'thunderingpower', 'skirmish']);
// 실제 AI 설정으로 선택할 수 있는 검사 전략이다. 게임 자원·기술·승패 값은 바꾸지 않는다.
const ALLY_AI_PROFILES = new Set(['default', 'guard-priest']);

// 브라우저 접근 전에 미션을 검사하고 소문자 이름을 반환한다. 다른 미션의 단순 진입 검사는 이 함수를 사용하지 않는다.
function assertAllyAiMission(mission) {
  if (typeof mission !== 'string') throw new TypeError('아군 AI 시험의 미션 이름 문자열 필요');
  const normalized = mission.toLowerCase();
  if (!ALLY_AI_MISSIONS.has(normalized)) throw new Error('아군 AI 시험 허용 미션이 아님: 캠페인 1-1~1-5와 스커미시만 허용합니다.');
  return normalized;
}

// 메뉴에서 실제로 기다릴 프레임 수와 AI 생성 전략을 검사한다. CLI 문자열도 같은 규칙으로 정규화한다.
function allyAiCondition(menuFrames = 0, profile = 'default') {
  const frames = Number(menuFrames);
  if (!Number.isInteger(frames) || frames < 0 || frames > 2400) throw new RangeError('아군 AI 메뉴 대기는 0~2400 정수 프레임이어야 한다');
  if (!ALLY_AI_PROFILES.has(profile)) throw new RangeError('아군 AI 검사 전략은 default 또는 guard-priest여야 한다');
  return { menuFrames: frames, profile };
}

module.exports = { assertAllyAiMission, allyAiCondition };
