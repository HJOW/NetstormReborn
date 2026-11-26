# 한국어 캠페인1-2 진입 중 AI 이름 오류 수정

- 단위: WB04-E1379, 2026-11-27, HJOW-Athlon.

한국어 MasterOfWhirligigs의 AI 이름 ‘훨리기그의 지배자’는 확장 표시 부호화에서33BYTE였다. 실제 AI 설정은 이름30BYTE와 다음 기술 문자열을 나란히 쓰므로 NUL을 남긴 상한은29BYTE다. RawCampaignMissionStartBindings가 예외를 던져 Start가 끝나지 않았고, 프레임 오류 뒤에도 메뉴만 남아 사제 선택이 막혔다.

번역 원본과 공개 복사본의 AI 이름만 ‘훨리기그 지배자’(29BYTE)로 고쳤다. 미션 제목·본문·명령·AI 능력·자본은 그대로다. 길이 제한을 풀거나 표시 토큰을 중간에서 자르지 않았다. prepare-assets로 공개 파일과 매니페스트의 크기·SHA-256을 함께 갱신했다. 외부 언어 자산 변경이므로 운영 JS 번들은 바뀌지 않는다.

## 재현과 검증

수정 전 실제 Chromium에서 사용자와 같은 RangeError·Start 스택을 재현했고 새 전수 검사도33BYTE로 실패했다. 수정 후 동일한 실제 메뉴 경로로 미션 진입·적 AI 생성·브리핑의 미션 시작·게임 시계 전진·실제 화면 적중점 좌클릭의 사제 선택까지 통과했다. 브라우저 오류0, 공격·포획·희생·승리는 수행하지 않았다. 진행 해제값만 검사 환경에서 준비하고 현재 미션의 HP·SP·AI·승패·선택 값을 직접 쓰지 않았다.

한국어 미션 AI 이름59개를 전수 확인했으며 제한 초과는 이 이름 하나였다. 새 검사에서 실제 표시 코덱 왕복·29BYTE 상한·NUL·600BYTE 설정의 다음 기술 문자열 보존을 확인한다. 관련 Node26검사39/39와 Chromium1/1이 통과했다. 기존 탭은 새로고침 후 미션을 다시 시작해야 새 언어 자산을 읽는다.

[수정 전 실제 오류](webpj-e1379-evidence/before-browser.log) · [수정 전 길이 검사](webpj-e1379-evidence/before-node.log) · [수정 후 실제 검사](webpj-e1379-evidence/browser.log) · [이름59개](webpj-e1379-evidence/names.json) · [검증값](webpj-e1379-evidence/verification.json) · [사제 선택 화면](webpj-e1379-evidence/selected.png).
