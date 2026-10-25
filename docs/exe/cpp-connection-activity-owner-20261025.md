# 연결 활동 표의 Client 수명과 실제 수신 연결

2026-10-25 단위432. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 전체 정밀본은2026-10-23이다. 대상은 PATCH10.78의004b6180이다. 기존 정밀C·보호PE·변환 fixture를 읽었으며 새 디컴파일이나 원본 실행을 하지 않았다. 이전 통합 제품은60a5c7de의2,894검사·메뉴·자연 Early2→Tutorial3다.

## 물리와 소비자

`ClientConnectionActivityGlobals`가005c84e8부터frame22DWORD,005c8540/544의Zone 두DWORD,005c8548부터wall22DWORD를 별개 필드로 소유한다. 두 표 사이의 간극을 다음 배열의 첫칸으로 처리하지 않는다. 바로 뒤005c85a0의CentralCreated도 다른 owner다. 최초 Client만 BSS0을 만들며 Session 대여/반납은 표를 지우지 않는다.

실제 Client 서비스는 같은 owner와 매번 현재 `Client::Time().number` 하위DWORD를 공급한다. Login의 activeStamp/zoneWord는 같은 간극의 참조다. 독립 Session만 별도 owner를 한 번 구성한다. Frame/UI 장치가 없는 독립 Session도 수신이 선택되기 전에는 생성할 수 있으며, 선택된 필수 효과는 해당 위치에서 누락을 실패로 처리한다.

원문004b6180은 현재 double wall을 조회하고004e49c0의signed64 절삭 하위DWORD를 얻은 다음 현재 frame을 캡처한다. wall store 뒤 현재 server를 다시 읽고 캡처했던 frame을 저장한다. 마지막은 서버이면 상태2/sender, 아니면 상태1/player0 삭제다. 효과 뒤 같은 표를 되돌리지 않는다. NaN/무한대/범위밖의 하위DWORD0은 기존ftol 관측의 계약이며 새로운 Native 실행 결과가 아니다.

일반 분배기에0x64의빈observer/version1/mismatch0/수신 몸체를 등록한다. 기존 길이5 표는 재설정하지 않는다. Login Timer의remoteStatus(9,0,0)도 같은 `Receive(9)`를 직접 호출한다. 연결 존재는 서버의Open이나flags2가 아닌 실제Client9 생성 완료를 조회한다. 기존 전송에는 실제 endpoint 삭제 몸체가 없어 그 뒤의 존재 반납은 후속이다.

실제 UI adapter는 현재missionPost를 먼저 읽는다. 상태1/2는missionPost가비0이면 원문004d3090처럼 UI 표 초기화·조회·삭제 전 자연 반환한다. 그 외에는 기존WarningMessages.Remove(kind,player,0)에 연결한다. `ui.removeMessage`는 다른 callback으로move되기 전에 값으로 소유 복사한다. 로그용playerNotice를 삭제 부모로 사용하지 않는다.

Editor는 실제checkpoint/UI/State 공통 확정 뒤 기존13종과별도로64h를 등록한다. 전체 충돌을 쓰기 전에 확인하고 부분 실패/Session 반납에서는receive만 지운다. observer/version/mismatch/length는 유지한다. 정상client9 수신과 지원 밖 음수pending은 구별한다.

## 검증과 남은 범위

준비본17소스를 적용했다. 줄바꿈만 달랐던 적용 원시 바이트를 확인해 기존HEAD의 파일별 줄바꿈을 유지한 Source3으로 동결했으며 Source1/2는 빌드하지 않았다. **첫 빌드 종료0·컴파일 경고/오류0, 집중68개·실패0**이다. 신규15검사(순수6·실제Session5·실제Transport2·기존Editor fixture2)의 실제 실행을 이름으로 확인했고 기존 전송/Timer/로그인/Client owner/Editor 회귀도 통과했다.

보호1821파일·17소스·4실행 파일 SHA가 같다. 게임 SHA는 `4d67bffab1bad698a5b1d48684d0cc04fe638be536db6783325a7abc7236af20`다. [실행 보고서](../../cpppj/connection-activity-runtime-20261025.json)에 빌드·실제 집중 로그와 원시 manifest를 기록했다. 이번 단위는 전체 CTest·긴 GUI를 반복하지 않았으며 마지막 통합 검증은 이전60a5c7de 제품이다.

창 없는 Session 검사는 실제 Client owner와 Login·Timer·Editor 큐/dispatcher를 사용하며 외부 wall/frame/UI 관측 장치는 명시 공급한다. 실제 factory의WarningMessages GUI 삭제는 소스 대조 범위다. CLI 검사만으로 GUI 삭제까지 검증했다고 주장하지 않는다.

Native sender 범위 검사는 없지만 이 구현은 정상0..21 밖을 첫 wall 효과 전에 `out_of_range`로 거부한다. 이는 지원 계약이며 인접 Zone/Central 주소 덮기의 재현이 아니다. Native 전체 cleanup의frame22 clear, 선택slot clear, 주기64 송신/timeout/UI 부모, 실제 endpoint 삭제, 전체 권한owner는 미완료다. 실제 Edit State1 부모·사용자 편집/저장·전체미션 자연 완주·진행 저장/재실행·원본 완전 동일성도 계속 남는다.
