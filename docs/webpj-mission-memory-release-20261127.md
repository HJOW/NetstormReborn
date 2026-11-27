# 미션 종료 뒤 공용 진단이 남기던 Session 참조

WB04-E1404, 2026-11-27, HJOW-Athlon. 같은 앱에서 실제 아군 AI를 켠 캠페인1-1을 시작하고600프레임 진행한 뒤 Game → Leave Mission → ABORT로 나가는 과정을 여섯 번 반복했다. 체력·SP·승패·World를 직접 바꾸지 않고 원래 Client 프레임과 입력/State 반납 경로를 사용했다.

`tools/mission-memory-profile.cjs`는 종료한 World와 Session을 WeakRef로만 추적하고 GC뒤 JS heap·backing storage·현재 바인딩을 읽는다. 미션 종료 뒤 메뉴용 NoWorld HUD가 다시 만들어지므로 HUD 객체의 존재 자체를 누수로 판정하지 않는다. World·guide·display가 없고 생산 HUD의 session/world/maintenance 연결이 모두 없어야 한다.

변경 전 여섯 반복은 게임 오류0으로 끝났지만 마지막 World/Session 한 쌍이 남았다. 앞선 반복의 객체들은 다음 진입 때 회수됐다. 추가 비동기 작업·100ms 대기·GC뒤에도 마지막 한 쌍은 남아 단순 WeakRef 작업 수명과 구별했다. 별도 한 번의 재현에서 저장한 V8 heap의 강한 루트 경로는 다음과 같다.

```text
Window.netstorm → RawSurfaceStorage → RawBuildingSpace
 → notifyAssertion → assertion() 환경 → RawCampaignSessionOwners
 ├→ placeBattleNuggets 환경 → RawCampaignSession
 └→ world → deletionSelected 환경 → CampaignGameWorld
```

원인은 Session보다 오래 사는 공용 코어의 진단 콜백이 `assertion(owner, …)`의 owner를 강하게 보유한 것이다. 현재 진단 함수를 소비 때 조회하는 계약과, 생성 때 함수를 복사하는 `capture=true` 계약을 모두 유지해야 한다. 단순히 최초 함수를 콜백에 남기면 그 함수가 owner를 다시 보유할 수 있다.

후보는 owner를 WeakRef로 빌리고 복사한 함수는 owner가 약한 키인 WeakMap에 보관한다. 실제 owner가 살아 있을 때는 같은 현재 함수/최초 복사 함수를 호출하고, 반납한 owner의 수명을 공용 진단이 연장하지 않는다. 변경은 `raw-campaign-session-state.js`의 진단 팩터리에 국한한다.

E1405 전체 회귀 동안 본 소스는 고정했다. 후보 파일 하나만 같은 모듈 URL에 제공한 별도 브라우저에서 여섯 번 모두 World/Session의 WeakRef가 false였고 게임 오류0이었다. GC뒤 마지막 JS사용량은 변경 전32,524,796바이트, 후보28,622,344바이트였다. 두 브라우저의 JIT·비동기 음악·네이티브 저장소 조건이 다를 수 있으므로 차이를 전체 메모리의 정확한 절감량으로 주장하지 않는다. 종료 객체의 실제 회수가 주된 근거다.

본 소스에 적용한 뒤 기존 현재/복사 진단·owner·반납 계약 집중35/35, 선행ESLint/운영 빌드, 재생성 번들 일치1/1이 통과했다. 원래 본 소스를 쓰는 실제 아군AI 여섯 반복에서도 매번 World/Session 전부 회수·게임오류0이었다. 마지막 안정 표본의 JS사용량은28,626,984바이트다. Chromium의 기존 L2 시작 화면/덤프와 실제1-1 저장 사본의 제품 번들1-2 진입/새로고침도2/2 통과했다(1.5분). 이는 저장 읽기 검사이며 새 자연 승리를 만들었다는 뜻은 아니다.

처음 수집기는 메뉴용 NoWorld HUD도 없어야 한다고 잘못 단언했다. 원래 HUD 반납 계약을 확인해 월드 연결 여부로 고쳤고, 실제 Game/Leave/ABORT 경로로 재수집했다. 첫 실패 원문도 보존하며 제품 오류로 세지 않는다.

[변경 전 여섯 반복](webpj-e1404-evidence/before-six-cycles.json), [추가 대기/heap 재현](webpj-e1404-evidence/retainer-probe.json), [강한 루트 경로](webpj-e1404-evidence/retainer-paths.json), [후보 여섯 반복](webpj-e1404-evidence/candidate-six-cycles.json), [본 소스 여섯 반복](webpj-e1404-evidence/after-six-cycles.json), [집중 검사](webpj-e1404-evidence/focused.log), [선행 검사/빌드](webpj-e1404-evidence/build.log), [번들 대조](webpj-e1404-evidence/bundle-fresh.log), [브라우저](webpj-e1404-evidence/browser.log), [명령·해시·한계](webpj-e1404-evidence/provenance.json)를 보존했다. 큰 V8 heap 원문은 Git 제외 경로에 두고 경로 분석 결과만 넣었다.

M5 전체의 장시간 사용·물리120Hz·물리 음향 검증은 이 짧은 수명 검사와 별개로 남는다.
