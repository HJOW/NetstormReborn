# 생산 UI의 실제 Client 문맥 연결

- 단위: WB04-E1377, 2026-11-27, HJOW-Athlon.

생산 패널·격자·항목·선택은 ProductionControlContext의 실제 storage·gameplay·players·options를 빌린다. 초기 생성과 빈 격자는 Session을 조회하지 않고 같은 Pool·finder·커서 데이터를 사용한다. 커서 집기·취소와 비용처럼 선택된 World 효과는 gameplay.requireSession()으로 현재 실제 Runtime을 요구한다. Runtime을 흉내 낸 객체나 빈 목록 우회는 추가하지 않았다.

생산 자본은 Native RawProductionControls::Money와 같이 ClientSpGlobals의 실제 분산 DWORD를 float로 읽는다. 독립 기존 문맥만 현재 실제 Session의 SP 조회로 돌아간다. 생산 패널 생성 시 같은 옵션 BYTE에서 슬롯 수를 읽고 벽 시각으로 bridgeUntil·initialized를 작성하여 후속 World 도킹에서 다시 초기화하지 않는다. 실제 앱의 조립기는 같은 Context를 각 하위 구성 요소에 전달한다. 공유 메뉴의 World 의존 때문에 조립기 입구의 Session은 다음 단위에서 분리한다.

새 검사는 실제 보호 자산·RawSurfaceStorage·ClientGameplayGlobals·PlayerRecords와 실제 네 UI 구성 요소를 사용한다. Session 없이 생성, 옵션/시각 변경 뒤 초기 슬롯·기한 보존, SID0 손상 체인의 실제 진단, 비활성 선택의 무효 처리와 유효 선택의 Runtime 요구를 확인했다. 별도 검사는 실제 분산 SP의 양수/음수 float와 미연결 관문을 확인한다. 글꼴/예산 표시 경계만 검사 장치를 제공한다.

## 검증

Node26 집중9/9 통과. 운영 번들은3,463,176바이트이며 크기 한도 내다. Chromium3/3에서 생산 정보/도움말·1-1 진입 화면3곳0픽셀/상태덤프3곳허용밖0·D 재선택을 확인했다. 공격·포획·희생·승리는 수행하지 않았다. 번들 재현/대응표 감사8/8도 통과했다. [브라우저 원문](webpj-e1377-evidence/browser.log) · [검증값](webpj-e1377-evidence/verification.json).

## 후속

공유 메뉴 writer·buffers·Chat 생성과 World 전용 About/Player/사제/Factory/객체/지식 어댑터 연결을 나눈다. 실제 Client Context로 전체 HUD를 먼저 만들고, Runtime 반납이 새 HUD를 닫지 않게 수명을 분리한 뒤 State7 공통 부모를 연결한다. Console 기한·정렬·화면 오차를 직접 보정하지 않는다.
