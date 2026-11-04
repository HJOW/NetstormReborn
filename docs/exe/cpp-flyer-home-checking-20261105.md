# 단위868 — 비행 홈 조회의 현재 TYPE·checking·진단 연결

비행 홈 getter `0045c100`과 실제 AI의 두 호출자를 코드/정적으로 대조했다. 공중 3종 전용 생성·행동·신규 검사와 실제 공격은 사용자 제한에 따라 실행하지 않는다. 기능 검사를 통과했다는 기록이 아니다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**, 현재 호스트도 HJOW-Athlon이며 산출물은 변경하지 않았다.

## 원문 순서

원문은 먼저 현재 대상 TYPE의 genus `0x100000`을 검사한다. 없으면 checking과 관계없이 `\Ns\O\Flyer.cpp:0x8ee`의 `flyer.getGenus() & gFLYER`를 진단한다. 진단이 정상 복귀하면 저장한 대상의 현재 free/dead 비트3을 보고, 둘 다 없을 때 현재 `005410ec` 타입으로 `004b1a90(parent,type,0,0)`의 첫 부착 Form을 검색한다.

finder 다음에 현재 `005e4794`를 읽는다. 저장 결과가0이고 checking이 비영일 때만 `processSid`를 같은 파일 `0x8f5`에서 진단한다. 진단이 돌아오면0을 반환하며 새 Form이 생성돼도 재검색하지 않는다. 결과가 있으면 현재 Form의 `004ae2d0` PID/inner를 읽어 `inner-0x30`의 outer `+0x0a` 홈 WORD를 반환한다. inner가NULL이면 원문은 주소0x0a를 읽으려 한다. 새 호스트에서는 이 잘못된 접근을 실제 fields 장치의 오류로 남기고 성공한 홈0으로 대신하지 않는다.

기계어로 `0045c149` finder → `0045c14e` checking 재조회, `0045c16e` 진단 → 바로0 반환, `0045c18c` inner 조회 → `-0x30/+0xa` WORD를 확인했다. 디컴파일의 `-0x26`만 보고 다른 구조체 필드를 선택하지 않는다.

## 실제 연결

`RawFlyerProcess::Home`에 위 순서와 정상 진단 복귀를 추가했다. 같은 처리기가 읽는 TYPE genus 표는 생성 당시 복사본에서 현재 World 표 대여로 바꿨다. 따라서 기존 `Bind`의 genus도 동일 현재 표를 읽으며, 유효188TYPE 밖은 기존 `.at`의 호스트 범위 오류를 유지한다. caller는 TYPE 표를 처리기보다 오래 유지해야 한다.

`RawGameAuxState::FlyerProcessType`은 보호 PE의 `005410ec=31`로 한 번 초기화하고 Session에서 같은 장기 DWORD를 참조한다. 일반 월드 준비나 Session 교체에서 별도31을 다시 쓰지 않는다. 생성·홈 조회가 같은 참조를 사용한다. `FlyerProcessGlobals::checking`도 실제 Client `AiDebug().Checking()`에 연결했다.

실제 `AiDamageHooks::home`의 고정31 검색/조용한 Form0 반환을 위 공통 본체 호출로 교체했다. 이는 `RawAiDamage::Notice`의 두 원문 호출자가 공유한다. 기존 fields 장치는 실제 같은 Host의 `FlyerProcess`를 조회하며 없으면 오류다. 생성/Finish/Bind/홈의 확인된 진단 줄 `0x2e3/0x41d/0x904/0x8ee/0x8f5`는 같은 현재 Client assertion 서비스로 보낸다. expression API 경계이므로 원문 파일/행 전체를 실제 GUI에 표시한 근거는 아니다.

CD 대응 `0049a650`에는 genus 진단 `0x6ab`은 있지만 PATCH의 processSid checking 진단이 없다. 기존 PATCH 전용 `RawFlyerProcess`를 CD 전체 복원으로 확대하지 않는다. CD의 NULL inner도 주소0x0a 읽기로 남는다.

## 검증과 남은 범위

제품 Release 빌드는 **종료0·326.736초·경고/오류0**이다. 첫 빌드 소스와 최종 SHA-256이 같고 사후 제품 수정은0이다. 보호2761개/fixture941개 전부 불변이며 fixture/기대값 변경0이다. 새 단위·Native·Session/게임·MP·공중 전용·실제 공격/포획/희생/승리 검사는0이다. 마지막 집중 통과는867 제품의28개이며868의 동작 검증으로 합산하지 않는다.

제품은868/11.01, EXE 10592256바이트, SHA-256 `1a6f95e89c81e0f3f28d8f538e1d39b01292964bc9e1116ef400f26cb984b5fe`다. checking 구현 잔여는 **10=부모4+부분3+MP분류3**, 완료부모38/부분3이다. 감소한 부모는0045c100 하나이고, 이 단위의 완료는 코드/Client 연결 완료를 뜻하며 기능 검사는 생략한 상태다.

세부 해시는 [JSON](../../cpppj/flyer-home-checking-20261105.json), 원시는 Git 제외 `extracted/cpp-resume-868/`에 보존한다. 비행 전용 동작은 사용자 실제 플레이 확인 범위로 남긴다. 다음869는 보상/일반 삭제의 다른 현재 TYPE 번호이며 다른 의식/운반 소비자는 별도 잔여다.
