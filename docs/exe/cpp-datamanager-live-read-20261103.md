# 769 — DataManager 실제 파일과 Template 소유 할당

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위769 완료·전체 목표 미완료다.

## 실제 caller와 소유권

일반10.78 미션의 현재 DataManager 파일 읽기44d980만 명시41baa0의 length0/check1/extra0에 접합했다. UberGump가 Native players이면서 mission.FileSource가 없을 때만 readWhole callback을 설치하고, 기존 Login FileHooks의 복사/legacy read 보충 뒤에도 그 callback을 유지한다. 명시 Editor 작업본의 삭제 장부/namespace·CD1072·GameAssets·CLI span과 기존 read/snapshot/직접OS API는 바꾸지 않았다.

DataManagerFileAllocation은 물리 storage와 signed 논리 length를 따로 소유한다. 미발견은 nullopt/0토큰이고, 존재하는 빈 파일은 최소4/length0의 비NULL 할당으로 서명 검사에 들어간다. 같은 현재 FILE/키는 전체 읽기에서 반납하고 소유 할당은 서명/파서가 빌리는 동안 유지한다. Raw DataManager의 release가 그 할당을 반납하며 이미 복사된 section 버퍼와 혼동하지 않는다. 실패 때 이미 공개된 manager/header/부분 노드를 임의 rollback하지 않는다.

짧은 암호 fread는 FILE/키가 반납된 뒤 명시 거부한다. 선택된 TAFF의 Open 실패를 보조 파일이나 optional 없음으로 숨기지 않는다. host vector의 0padding과 원문의 명시205seed/미정tail은 별도 경계다. 바깥 RAII의 미완성Open/미공개할당 추가 반납과 length 필드의 주소 폭도 원문32비트 FILE/MarkStream20/SEH 전체 동등성으로 세지 않는다.

## 원문 범위와 검사

원문60입력/120관측과 준비 재관측120·root 추가120 및교정도구재관측120회가 일치했다. 최초 관측은44d380을 가상본문 경계로 남겨 prepared-observations에 보존했고, 최종은 실제44d980→41baa0→4bce10→44d380/44cdb0와 marker cleanup4f93a0/4fa070→44d040을 실행했다. 길이1 F의 pos>end 진단을 정상false로 바꾸지 않았다. 이 Native는 generic emptySNART/빈 이름표 경로이며 실제 Fort35개 섹션 전체의 새 Native 검증이라고 주장하지 않는다.

최초C++ 대조4실패는 시나리오번호7의입력모형충돌이었다. 상위WholeReadOracle의7은malloc NULL이며명시mapped0페이지에fread한뒤0토큰으로본문진입을건너뛴다. C++은할당성공/본문예외로주입했다. 기존60fixture는유지하고NULLtoken과별도4BYTE mapped0페이지로장치를맞췄다. 실제Client의vector할당은NULL을반환하지않고bad_alloc을던지므로이CPU scaffold를실제OS의NULL읽기지원으로세지않는다. 본문예외는겹치지않는9번4입력/8관측을새fixture로고정하고추가8회대조했다. v3의행수검사가추가4행삽입누락을잡았고두fixture합본을실제로실행하도록고쳤다. 모든최초실패로그를보존하며제품과기존fixture는바꾸지않았다.

별도 정적 감사에서 실제 Template vtable514690의 이름표4bccb0는 기본15+Terr20=35이며 빈 payload도 각각 노드를 생성함을 확인했다. 단독 비멀티460c40/Edit4b5ca0도 같은 고정 vtable을 쓴다. 따라서 generic 짧은 이름표 성공은 실제 Fort의 잘린 레코드 허용 근거가 아니며 기존 created35 guard를 유지했다. Find 누락0/Get 누락 진단/명시 Create와 빈 State 초기화도 구별한다.

최종 Release 경고/오류0, 집중 **15개(새6개)/실패0**. 현재 backing TAFF와 기존 snapshot 분리·미발견/빈파일·선택한TAFF 열기 실패·짧은암호파일의 반납/교체·정상I/O 뒤 파서실패 prefix와 기존 Template 이름/readonly/현재슬롯/공개manager/부분node/실패 정리를 확인했다. 새 실행대상0/기존66개·전체71대상을 유지한다.

기존 보호 2692개 모두 불변·새fixture2 포함2694개/fixture874. 다음 기준은 extracted/cpp-resume-769/protected-after.json, 제품 SHA-256은 `f5f473e7d488e4cf3e699b9b461815258383ca7f3cdb0b157d60023931e03dec`다. [보고서](../../cpppj/datamanager-live-read-20261103.json)와 같은 extracted의 build/tests/최초·교정·추가 Native/정적감사를 따른다.

새 게임/GUI/CLI/MP·물리/PCM·공중3종전용검사·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP 검사는0이다. 다음은770 생성 부모와 각 reader/AI/호환성이다.
