# dotnetpj 0.527 — 실제 호스트의 장기 SP·게임 전역 연결

2026-11-14, HJOW-Athlon. 사용자 지시로 목표를 재개했다. 기준 소스는 `4a9cc0f0c6198e567d2a2b68ac73d401d6274251`의 `client/ClientSpGlobals.h/.cpp`, `ClientGameplayGlobals.h/.cpp`와 `RawCampaignSession.cpp`의 1012~1025·1091·1226~1227행이다. 디컴파일 산출물은 변경하지 않았다.

## 구현

`ClientSpGlobals`는 실제 호스트의 기존 GameRandom·CRT·모드·시각을 빌리고 SP 추적 IntBox를 초기화 전에 연결한다. 생성과 조회에서는 시각이나 난수를 소비하지 않는다. `ClientHostGame`의 기존 공통 시작 위치에서만 명시 Initialize를 호출한다. 미션별 초기화는 없다.

`ClientGameplayGlobals`는 현재 GameAux·Authority와 최초값6인 PatchSample·PatchDiagnostic을 호스트 수명으로 소유한다. 이번 이식은 이 네 부분이며, 전체 cpp 클래스의 커서·선택·Runtime 대여는 아직 옮기지 않았다. PORT_MAP 상태는 port이고 전체 파일 완료를 뜻하지 않는다.

제품 `StartMission`이 같은 SP·게임 전역을 HostSession에 전달한다. HostSession의 GameAux는 이제 그 실제 owner이며, 같은 사제TYPE/이동 역할로 만든 사제 월드 상태의 회복에도 같은 TYPE·표본·진단을 연결했다. 기존 독립 HostSession 호출은 자체 게임 전역을 소유하는 API를 유지한다. 기존 공유 SP 호출자에게 늦은 추적 연결이나 숨은 재초기화를 추가하지 않았다.

## 검증

전체 비증분 Release 빌드는 원복 뒤 경고0·오류0이다. Client 집중25건(새 공유 owner3·사제 월드13·Player9), 관련 Core 및 메타데이터40건이 실패0·건너뜀0·각 종료0으로 통과했다. 초기 추적 주소를123으로 바꾼 대표 변이는 새3건 중1건이 검출했고 원본 바이트를 복원한 뒤 재빌드·재통과했다. 전체 회귀 완주로 합산하지 않는다.

HJOW-Athlon 예외PC에서 thewarbegins 단순 진입·대기만 cpp/dn 각120프레임 실행했다. 두 프로세스 종료0, originals1394파일 전후 해시 동일, 추적 assets 옵션 변경 없음이다. dn 덤프2133행은 0.512 최종분과 frame 행 하나를 제외하고 바이트 동일하다. Game2317693072·CRT296677442·1854슬롯을 보존한다. 현재 cpp 오라클 SHA는 `3663ed8181451adb873edde8d0297a3e1e157b28acdbcd29298001192f464d76`이다. 전체 L2 차이(cpp1941슬롯·dn1854슬롯·cpp_only87·dn_only0)는 기존 미연결 범위이며 M1 통과로 세지 않는다.

자료는 `extracted/dotnet-l2-0527/`와 `extracted/client-sp-globals-0527-original.bin`에 있다(Git 제외). PORT_MAP805행(L1 385+port420), fixture276개 유지. 새 NB 수정·외부 접속·기존 코드 은퇴는 없다.

## 다음 연결

장기 owner 연결은 완료했지만 사제 월드 전체의 제품 연결은 아직 아니다. 결선 검토에서 `RawSquidReward.TypeHitPoints`가 현재 사제TYPE/1/4HP 주소·정상 복귀 진단과 현재 표 기준을 아직 읽지 않는 것을 확인했다. 다음0.528은 이 실제 HP 부모를 현행화하고 HostSession 보상에 연결한다. 그 뒤 사제 Pop/Unpop/Repop·공유 프로세스·소리·피해/해방을 연결한다. 일반 지역 정책·AI 시작·그 밖의 객체 후처리·M1전체0/34는 잔여다.

사제 포획/희생·특정 적 공격·시나리오 승리·MP 재현은 하지 않았다. AGENTS.md·원본/CD·README 개요·cpppj·LEFT_JOBS.md·webpj를 변경하지 않았다.
