# cpppj 일시중지 인계 — 제품1265 이후 재개

2026-11-10 / HJOW-Athlon. 사용자의 **현재 진행 중인 작업 검증·문서·커밋 뒤 목표 일시중지** 요청으로 남기는 준비 자료다. 제품은1265/11.01이며 [현행 인계](../../../../LEFT_JOBS.md)와 [1265 검증](../../cpp-login-debug-burst-20261110.md)이 기준이다. **이 폴더의 코드는 제품 미적용 후보이며 구현/전체 M6 완료나 게임 실행 통과가 아니다.** 재개 지시 전 새 제품 단위는 시작하지 않는다.

## 재개 순서와 자료

1. AGENTS·LEFT_JOBS·LEFT_JOBS.dotnetpj 및 현재 호스트/제품/변경을 먼저 확인한다. 마지막 Client 디컴파일2026-11-03/R2026-11-07의 HJOW-Athlon과 호스트가 다르면 기존 지침대로 다시 디컴파일한다. 원본/CD/AGENTS·README 개요 보호, MP·직접 플레이 실행 제외를 유지한다.
2. [Snapshot/준비 후보 최종 인계](snapshot/HANDOFF.md)와 [정확한 접합 예시](snapshot/README.md)를 읽는다. 네 UTF-8/LF 소스는 `snapshot/cpppj/src/client/` 아래에 있다. 자동 덮어쓰기하지 말고 현행 소스 차이를 선별 병합한다. CMake/입력/이름/State 연결은 미적용이다. **최종 단일 MSVC `/Zs` 두 cpp 종료0·검사 전후 소스 일치**만 확인했으며 제품 링크·GUI·실제 저장·MP/단위는0이다. 앞선 include 오류와 같은 로그 잠금 실패를 숨기지 않고 별도 관측으로 보존했다.
3. Snapshot의 Player 현재 기술 부모는 옛 WIP의+6c 오류를 실제 **+9c**로 교정했다. 같은 장기 Template/Deck/Pool/Player/난수/파일 owner·현재 Content TYPE·원문 manager0/독립 Battle/Fort 부모와 진단 정상 복귀를 유지한다. 51개 원문 함수의 범위/해시·50진단은 [함수 장부](snapshot/native-function-ledger.json)·[진단 장부](snapshot/diagnostic-map.json)다. 원본 C/ASM 대량 발췌는 `extracted/cpp-resume-1265-candidate/`에만 있으며 저장소 준비본에는 복제하지 않았다.
4. [나머지 최초 후처리 부분 감사](postpop/HANDOFF.md)·[83VT 목록](postpop/inventory.json)을 따른다. 원본 +20은46함수다. 확인한 진입49행은 공통28/장기전용18/접두3이며 **하위·호출자·실행 완료 수가 아니다**. 최초 잔여34행(미연결32·비최초만2)을 재개한다. 보호막의 독립2예약 접두와 메타Sprite2종의 공통 허용표 누락 후보가 다음 좁은 조사점이다. 활성 caller 전체는 미감사다.
5. [비영 AI 부분 감사](ai/HANDOFF.md)·[해시/범위](ai/audit.json)를 읽는다. `aiMemory`뿐 아니라 `RawAiBrainConstructor`의 live/destructed 장부가 같은 장기 owner여야 한다. 고정 Player span·현재 Host 경로/목표·월드등록/통지/지갑·정상 State의 brain 정리도 함께 다룬다. 원문 전수/정상 NoWorld 도달성 감사와 실제 구현은 미완료다. 정상 반납 뒤 항상 비영 brain이 남는다고 주장하지 않는다.

## 확정 제품과 구별할 것

- 1264 Dude·세 효과 최초4VT 및1265 debugBurst는 제품에 적용·빌드돼 있다.1265 Release390.695초/종료0·경고/오류0, 비MP306/306·Tutorial4 생산/건설/600프레임·정상 종료0과 보호2,808파일 불변을 확인했다.
- 이 준비본을 제품1266이나 배포 ZIP으로 세지 않는다. 큰 NoWorld·AI/나머지 PostPop·모든 process VT/호출자 수명·M6 전체 및 상위22계획은 남아 있다. 순서는 M→K→E→S→B→L을 유지한다.
- 모든 후보/감사는 MP·사제 포획/희생·특정 공격·승리/사용자 직접 확인 실행을 추가하지 않았다. 제품 링크 후 허용된 일반 Edit/사용자 홈·읽기 전용 설치·미션 진입/대기/생산/건설 검증은 후속 단위에서 한다.
- 다른 세션의 DN/WB 변경은 이 단위에 포함하지 않았다. push하지 않았다. 현재 자료의 원시 출처와 보존 해시는 [MANIFEST.json](MANIFEST.json)에 있다.

Git 텍스트 체크아웃은 개행을 바꿀 수 있다. MANIFEST의 sha256은 작성 당시 원시/작업트리 바이트이며, lfNormalizedSha256은 CRLF를 LF로 정규화한 보존본 값이다. 재개 시 네 후보는 UTF-8/LF 사본이나 git show의 저장 바이트로 최종 후보 해시를 비교하고, 현행 제품의 혼합 줄끝은 일괄 정규화하지 않는다.
