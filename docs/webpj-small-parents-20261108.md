# webpj 소형 부모 다섯 개(제단 해방·Tutorial 소멸·다리 낙하 진단·사제 회복 HP·Boardcoord 각도) 이식 — WB04-E15

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열다섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 작은 `o/` 단위 다섯 개(10.78 경로)를 한 번에 옮겼다. 대부분 독립 원본 관측을 가지며 서로 의존하지 않는다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/raw-altar-release-parent.js` | 00448bd0 | `runAltarReleaseParent(altar, checking(), hooks)`: 최초 검색 SID를 진단 전에 저장 → checking 비0이면 방향 12이거나 두 번째로 읽은 방향 11이면 원문 진단(`!isInvisibleFrame() && !isSacrifice()`, `.\Dais.cpp`, 0x578) → 복귀해도 저장한 SID로 계속 → 현재 풀 범위에 있으면 기존 해방 → Unpop(0) → 표시 통지 없는 P 프레임 raw 쓰기 → Repop(0). 이미 진입한 방향 판정은 checking이 0이 되어도 다시 시작하지 않는다. |
| `www/core/raw-tutorial-mission-release.js` | 004c3210 / unwind 004fb940 | `releaseTutorialMission(state, hooks)`: 허용 표 갱신(1) → 여섯 전역(회수 금지·승급 금지·미션 번호·Early 분배 번호·단계·단계 변경 표식)을 순서대로 0 → owner 1..8 AI 전체 반납 → 기본 Mission 해제. 기본 Mission은 정상 꼬리와 예외 unwind에서 모두 정확히 한 번(`try/finally`) 수행하고, 정상 꼬리의 실패는 재시도하지 않는다. |
| `www/core/raw-bridge-fall-parent.js` | 00421f90 | `runBridgeFallParent(bridge, x, y, checking(), hooks)`: checking 비0이고 현재 genus에 다리 비트(0x4)가 없으면 원문 진단(`s.getGenus() & gBRIDGE`, `\Ns\O\Bridge.cpp`, 0x323), 복귀해도 같은 SID·원래 float 인자로 예약(`schedule`)한다. checking 0이면 genus/진단 장치를 요구하지 않는다. 실제 `SquidProcessHost` 연결(`ScheduleBridgeFallChecked`)은 낙하 예약 단위에서 잇는다. |
| `www/core/raw-priest-regen-hit-points.js` | 4945ee | `readPriestRegenHitPoints(hooks)`: 현재 HP·최대 HP 읽기 → 초기 HP≥최대면 회복 없음(`null`) → 증분(번호 읽기→현재 HP 재읽기→TYPE 최대 HP 조회→현재 HP+trunc(typeHp/6), 32비트 감김) 후보를 만들고 현재 최대를 다시 읽어 후보가 크면 최대를 한 번 더 읽어 반환, 아니면 증분을 한 번 더 계산한다. |
| `www/core/raw-board-angle.js` | 0041cdb0 | `normalizeBoardAngle(angle, {checking, assertion}, modulus)`: float 0이면 +0 → `fmod`(JS `%`)로 나머지, 음수면 360을 더함 → 360 이상이면 checking 비0이고 361 이상일 때 원문 진단(`angle < 361.0f`, `\Ns\O\Boardcoord.cpp`, 0xb3)을 요청하고 어떤 경우든 float 359.9999(비트 43b3fffd)를 반환. |

## 2. 검증 (`tests/small-parents.test.mjs`, 11건)

| 검사 | 근거 |
| --- | --- |
| 제단 1596행(첫/두 번째 방향·checking·범위·저장 SID·현재 P 프레임·가상 요청) + 장치 누락/진단 중단의 부분 효과 | `altar-release-parent-hardware-x86.tsv` |
| Tutorial 소멸: 원문 store 순서·허용 표 실패·AI 반납 실패의 부분 쓰기 보존·기본 Mission 한 번(원본 관측 없음, PE 정적 확인 계약) | (독립 입력) |
| 다리 낙하 200행(현재 checking·genus 진단·복귀 뒤 원래 좌표 비트, NaN 비트 포함) | `bridge-fall-parent-x86.tsv` |
| 사제 회복 HP originals 432행(Getter/번호/HP 반복 읽기 순서) + 선택하지 않은 장치 | `priest-regen-hit-points-current-x86.tsv`(`originalCD` 432행 제외) |
| Boardcoord 각도 originals 544행(실제 CRT fmod·현재 checking·이진 double) + 진단 장치 누락 경계 | `board-angle-hardware-x86.tsv`(`originalCD` 544행 제외) |

최초 실패와 교정: 11건이 처음부터 통과했다(소스는 셸 heredoc 대신 파일 쓰기 도구로 저장해 역슬래시 손실을 피했다). 기대값은 원문 열만 썼다.

## 3. 경계

- `ScheduleBridgeFallChecked`(실제 `SquidProcessHost`/Regular 예약)와 cpppj의 저장소 대여 수명 검사는 해당 Host/World 단위에서 대조한다.
- 사제 회복 HP의 `MakeCurrentTypeHitPoints`(UI 번호 HP 읽기)와 CD 경로는 이 단위에 없다. Boardcoord의 CD binary32 중간 저장 경로도 옮기지 않았다.
- 제단 해방의 사제 생성/해방/희생 몸체, Tutorial의 허용 표·AI 반납·기본 Mission 해제는 호스트가 hooks로 공급한다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
