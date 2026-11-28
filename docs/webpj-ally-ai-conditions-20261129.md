# 다른 아군 AI 진행 조건 — E1495 이후

2026-11-29, HJOW-Athlon. E1494의1-3 패배/1-4 상한 뒤 사용자는 **“다른 AI 진행 조건으로 계속 시도”**를 선택했다. 승리·완료값을 합성하거나 시작 자금·기술·HP·게임 규칙을 바꾸지 않고 실제 AI 설정과 메뉴 대기 조건을 사용한다.

## E1495 조건 도구 — 완료

- `default`: 기존 아군 어려움/적 쉬움 AI와 같은 기본 전략이다.
- `guard-priest`: 원래 `RawMissionAiSettings.setAbility`로 `USE_PRIEST_TO_COLLECT`만 끈다. 600바이트 기본 설정의 능력 비트가 `0xffff → 0xf7ff`가 되며, 사제 이동/건설·운반체·적 AI·생산/전투 규칙은 그대로다. 실제 `raw-ai-priest-actions.js`는 이 비트가 없으면 채취 대신 원래 home 경계를 고른다. 기존 brain의 중간 전략 변경은 지원하지 않고 명시 거부한다.
- 메뉴 대기는0~2400 정수 프레임이다. `stepFixedFrames`로 실제 메뉴 프레임을 진행해 시각/원래 난수 소비를 바꾸며 난수 상태를 직접 쓰지 않는다. 고정25ms이므로40프레임은1초다.
- 실행 조건별 출력 이름을 나눠 기존 증빙을 덮지 않는다. 기본0/default는 기존 파일 이름을 유지한다. 시작 직전 게임 시각·Unix 초·난수 상태, AI 생성 직후 능력 비트와 보호 값의 전후 비교를 기록한다.

```powershell
cd webpj
node tools/ally-ai-campaign-flow.cjs savetheisland 60000 0 guard-priest
node tools/ally-ai-campaign-flow.cjs fragilefortune 60000 40 guard-priest
```

추가 인자는 메뉴 대기 프레임과 전략이다. 생성 파일은 예를 들어 `extracted/ally-flow-savetheisland-wait0-guard-priest.{json,...}`다. 범위 밖 미션·잘못된 대기/전략은 실제 브라우저/파일명 작성 전에 거부한다. 허용 미션은1-1~1-5·스커미시이며 승리 CLI의 스커미시는 기존 별도 도구를 사용한다.

Node의 조건/거부·원본 AI 설정/능력 관련 **7/7**, Chromium의 범위 거부·기본1-3/1-4·사제 보호1-3의 실제600프레임 **4/4**, 변경 파일 lint가 통과했다. 실제 능력63487·원래2000SP·동맹/이름/기술 보존·brain 초기화/비정지·오류0을 확인했다. 로그는 `webpj/extracted/webpj-e1495-condition-node-related.log`, `webpj-e1495-condition-e2e.log`, `webpj-e1495-lint.log`, `webpj-e1495-cli-lint.log`다. 제품 `www`/운영 번들 변경은 없다.

## E1496 사제 보호 관찰 — 완료, 두 승리는 미완료

위 두 명령을 독립 Chromium/저장소에서 각각60,000프레임 실행했다.1-3은 사제 보호만,1-4는 사제 보호와 메뉴40프레임 대기를 사용했다. 실제 시작 관측은 각각 `(game=9.125, unix=946684809, random=2892766120)`·`(10.125, 946684810, 4143987439)`이며, 두 AI 능력63487·초기2000SP·기술/이름/동맹 보존을 확인했다.

| 미션 | 종료 게임 시각 | SP | 실제 결과 |
| --- | ---: | ---: | --- |
| 1-3 Save The Island | 1509.175초 | -800 | 양쪽 사제 생존·미포획, 완료 창 없음, 프레임 상한 |
| 1-4 Fragile Fortune | 1510.175초 | 1750 | 양쪽 사제 생존·미포획, 완료 창 없음, 프레임 상한 |

게임 프레임 오류는 각각0이다. 아군/적 brain은 모두 초기화1·정지0이며 관측상 돈이 부족한1-3의 경제 정체를 사제 보호만으로 해소하지 못했다. SP -800의 원인이나 제품 결함 여부는 이 결과만으로 단정하지 않는다. 두 CLI는 실제 자연 승리 단언에서 예상대로 종료1이며 저장/새로고침/다음 미션 해제 검사는 실행하지 않았다. 승리·저장 항목은 미완료다.

기록과 최종 화면은 [1-3 JSON](webpj-e1496-evidence/campaign1-3.json)·[화면](webpj-e1496-evidence/campaign1-3.png), [1-4 JSON](webpj-e1496-evidence/campaign1-4.json)·[화면](webpj-e1496-evidence/campaign1-4.png)에 보존했다. 전체 로그/Session TSV는 Git 제외 `webpj/extracted/webpj-e1495-guard-campaign1-{3,4}.log`·`ally-flow-*-guard-priest-terminal.tsv`다. Chromium156.0.8078.4 headless·Windows10 build19045·1024×768·DPR1·25ms·영어·음향끔 조건이며 실제 음향 청취/물리 모니터 검증을 대신하지 않는다. 제품 소스/운영 번들은 바뀌지 않았다.

다음 조건은 원래 AI `Collectors` 목표를3에서1로 줄이고 사제 보호를 함께 적용하는 방식이다. 실제 설정 로더를 사용하며 자금·가격·기술·HP·목표/승패를 바꾸지 않는다. 사용자의 다른 AI 조건 계속 시도 선택을 유지한다.

원래 Kernel·Renderer·Screen 프레임은 모두 실행한다. 실제 Next Mission 안내가 있어야 승리→신규 저장→새로고침→다음 미션 해제 검사를 수행하며, 패배/상한/오류는 성공으로 표시하지 않는다.
