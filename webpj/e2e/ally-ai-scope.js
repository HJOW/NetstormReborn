// 캠페인 1-1~1-5와 스커미시에서만 실제 아군 AI의 공격·포획·희생·승리 검사를 허용한다.
const ALLY_AI_MISSIONS = new Set(['thewarbegins', 'masterofwhirligigs', 'savetheisland', 'fragilefortune', 'thunderingpower', 'skirmish']);

// 브라우저 접근 전에 미션을 검사하고 소문자 이름을 반환한다. 다른 미션의 단순 진입 검사는 이 함수를 사용하지 않는다.
function assertAllyAiMission(mission) {
  if (typeof mission !== 'string') throw new TypeError('아군 AI 시험의 미션 이름 문자열 필요');
  const normalized = mission.toLowerCase();
  if (!ALLY_AI_MISSIONS.has(normalized)) throw new Error('아군 AI 시험 허용 미션이 아님: 캠페인 1-1~1-5와 스커미시만 허용합니다.');
  return normalized;
}

module.exports = { assertAllyAiMission };
