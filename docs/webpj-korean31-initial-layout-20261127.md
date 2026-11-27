# 한국어31미션 첫 안내의 실제 화면과 줄폭

WB04-E1430, 2026-11-27, HJOW-Athlon. 실제 운영 번들을 Chromium에서 열어 한국어 공식31미션의 첫 안내를 모두 확인했다. 창은 논리1024픽셀 안에 들어왔고 가장 큰 창은539픽셀이다. 실제 마지막 글자까지의 최대 폭은 모든 본문 폭보다 최소1픽셀 작았다. 게임/브라우저 오류0이다.

원래 앱의 `_beginCampaignFromDialog`와 반납 경로로 진입하고 첫 안내가 보이면 읽었다. Play/Start/Next/안내 단추나 유닛 명령을 실행하지 않았으며 완료값을 쓰지 않았다. 내부 `gate.started`는 부팅 때 이미1인 경로가 있어 이 값으로 사람의 미션 시작 단추 입력을 주장하지 않는다. 모든 캠페인의 단순 진입/첫 안내를 허용한 사용자 범위다. 후반 플레이·목표·승리 검증은 아니다.

첫 수집은 `maximumWidth <= baseWidth`를 요구해 Tutorial6에서 실패했다. 전체 관찰로 넓힌 두 번째 수집은 여섯 미션의 논리 폭1~2픽셀 초과를 기록했다. 원래 공백 줄바꿈은 단어를 채택한 뒤 뒤 공백도 복사하므로 마지막 공백의3픽셀 advance까지 줄 폭에 포함한다(`ui/scroll-text.js`의 `_splitText`). 원본과 같은 동작을 제품에서 바꾸지 않았다.

최종 수집은 원시 줄 폭·각 조각의 문자열/전체 폭/advance를 그대로 보존하면서 줄 끝 ASCII 공백을 뺀 마지막 내용 위치를 같은 실제 FontSet으로 별도 측정했다. 여섯 경우 모두 초과분은 마지막 공백뿐이었다. 그림 조각은 실제 그림 폭을 그대로 포함한다. Tutorial6·ThunderingPower·TheNoose·bc9ssun_bars76 화면을 직접 확인했고 글자 잘림이 없었다. 첫 실패와 전체 논리 폭 실패도 숨기지 않고 보존했다. 픽셀 차이를 마스킹하거나 오라클 단언을 완화한 변경은 없다.

[31상태와 줄/조각 원문](webpj-e1430-evidence/initial-missions.json)·[최종 로그](webpj-e1430-evidence/korean31.log)·[논리 폭 여섯 경우](webpj-e1430-evidence/logical-width-issues.json)·[첫 실패](webpj-e1430-evidence/first-failure.log)·[수집기](webpj-e1430-evidence/collector.cjs)·[출처/해시](webpj-e1430-evidence/provenance.json). [Tutorial6](webpj-e1430-evidence/Tutorial6.png)·[ThunderingPower](webpj-e1430-evidence/ThunderingPower.png)·[TheNoose](webpj-e1430-evidence/TheNoose.png)·[마지막 Priest Training](webpj-e1430-evidence/bc9ssun_bars76.png).

M4의 첫 안내 화면/줄폭 자동 확인분을 완료한다. 모든 미션의 중간 안내 전체와 한국어 문장·용어의 사람 검수는 사용자 확인으로 남긴다. v26=1/4·v22=1/8이다.
