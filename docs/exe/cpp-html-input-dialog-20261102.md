# HTML Input의 실제 문자·숫자 창과 후속 Tell — 단위708

2026-11-02, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 미연결이던 HTML `$Input`의004d3ed0/004d3c20/004cfae0을 기존 실제 문자·두 버튼·모달과 연결했다. 전체 목표는 미완료다.

## 원문 동작과 구현

메타의 세 인자는 **제목·모드·후속 Tell 섹션**이다. 모드는 문자 제한값이 아니고 후속 섹션은 설정 키가 아니다. 모드1만 초기 버퍼에 `Enter Text`를 넣고, 나머지는 `Enter Number`를 넣는다. 두 경우 모두 기본 TextEntry를 쓰며 원문대로 숫자 전용 필터를 추가하지 않았다.

`SaveFortNameDialog::ShowInput`은 원문의 공통 두 버튼 부모를 사용한다. root 요청 크기0x128, VT00517d00, kind68, 현재 mode DWORD와 후속 섹션80BYTE를 보존한다. 호스트 노드는 기존 Save as/송금 공통 저장소를 사용하되 후속 섹션과 canCancel을 별도 필드에 보관한다. 원문 물리 구조체 전체 ABI를 복제했다는 뜻은 아니다.

실제 TextEntry는 원문 root 폭×float0.8의 CRT 절삭 폭, 높이18, maxLength0x29, multiline0이다. 실제 입력의 strict 길이 비교로40문자를 허용한다. x는 가운데, y는45다. 테두리는 폭+8/높이+4이고 x-4/y-2다. 제목 helper의 폭은 **편집 자식의 x값**이며 임의로 root 폭80%로 바꾸지 않았다. 제목 Fit→Push→align4/모드2 재부착 뒤 기존 버튼 배치, root 높이+4·제목 폭+32와의 최대 폭, 화면 가운데·깊이-76·Modal 순서다.

`ConfirmHtmlInput`은 현재 편집 자식의 CopyText256→마지막 강제NUL→첫LF 절단을 수행한다. 공백 검사 루프는 원문처럼 마지막 문자가 아닌 **종료 NUL에서 시작**하므로 정상 CRT에서는 끝 공백을 제거하지 않는다. mode1 외에는 atol을 호출하지만 결과를 전달 문자열에 사용하지 않는다. 현재 후속 섹션과 입력 원문을 `%s(%s)`로 서식화해 같은 Tell을 호출하고1을 반환한다. 설정을 직접 변경하거나 입력을 숫자로 재서식하지 않는다. 확인/Enter·취소의 지연 Kill과 공통 소멸은 기존 부모가 처리한다.

`UberGump::LoadingHtmlContext`의 input 서비스를 같은 RawMenuControls/RawTextEntryControls/clipboard/Tell에 연결했다. 파일·월드·송금 장치를 요구하지 않는다. 기존 메타의 원시 인자 캡처와 Input 뒤 MenuJustClosed 예약은 바꾸지 않았다. Zsend는 별도 미연결이다.

## 원문 대조와 집중 검사

[원문 도구](../../tools/decomp_html_input_dialog.py)는 실제 생성004d3c20·공장004d3ed0·확인004cfae0과 CRT 절삭/strchr를 **109입력**으로 실행했다. 생성20개는 네 mode와 다섯 제목 폭을 교차해 부모/편집/테두리/제목의 실제 필드·float 비트와 배치·깊이·Modal을 읽었다. 확인89개는 빈 문자열·공백·LF/CR·내부NUL·숫자처럼 보이는 문자열·240문자·CopyText 뒤 mode/후속 섹션 변경을 포함한다. [fixture](../../cpppj/tests/fixtures/html-input-dialog-x86.tsv)와 [원문 보고서](../../cpppj/html-input-dialog-native-20261102.json)를 따른다.

공통 두 버튼/기본·문자/배경/제목 helper·부착·모달·깊이, 번역/복사/isspace/atol/printf/Tell·heap은 명시 경계다. 공통 버튼 배치 helper 전체와 할당 실패/SEH·손상 포인터/전체 ABI를 새로 검증했다고 세지 않는다. 최초 도구는 atol 주소 미등록으로 fixture 생성 전에 중단됐고, 실제004e66de 경계를 연결한 다음109개가 정상 반환했다. fixture를 덮어쓰지 않았다.

새 C++3개는109행 전체 대조와 실제 HTML 메타→공통 모달/문자→후속 Tell, 숫자 비필터/40문자 제한/취소다. 실제 공통 Draw도 수행했으며 운영체제 게임 창을 실행한 것은 아니다. 기존 Save as의 원문288행·확인/취소·마우스/clipboard/수명6개도 통과했다. **서로 다른9개 최종 실패0**이며 기존288행은 재생으로 구별한다. MP/송금 검사는 필터로 선택하지 않았다.

최초 focused-v1은 메타 검사 입력의 인자 구분 공백 누락으로2단언이 실패했다. 기존 메타의 원시 캡처를 바꾸지 않고 검사 문법을 고친 focused-v2 새3개와 legacy-v1 기존6개가 통과했다. 원문109 대조와 숫자/취소는 최초부터 통과했다.

## 제품·보호·인계

최초 build-v1의 C4458 변수명 경고1을 수정했다. 보조 정적 읽기의 bytes/str 오류로 수정 전에 수행된 build-v2는 갱신 없는 빌드다. 최종 build-v3 Release 제품·집중 검사 빌드의 **경고·오류0**이다. 제품708/표시11.01 SHA-256은 `cb0fc8488155ad2767099b900109ff4edad28ecc232bad19f93dbd2b28d758df`다. [제품 보고서](../../cpppj/html-input-dialog-20261102.json)와 원시 `extracted/cpp-html-input-dialog-708/`를 따른다.

기존 AGENTS·원본1394·CD425·fixture805의2625개 변경0, 새 fixture1 포함 현재2626개다. AGENTS와 전체 디컴파일은 수정하지 않았다. 게임/GUI 운영체제 창·MP·통신, 포획/희생·특정 적 공격·승리·공중3종 전용 행동의 실행은0이다. 최신 실제 GUI는704 제품이며 전체 C++/34/새 ZIP을 반복하지 않았다.

원문의 TARC에 있는 관리자/길드/방송 후속 Tell과 전체 실제 표시·픽셀 동일성·물리 입력/PCM은 검증하지 않았다. 707의 의식 선택 요청/비미션 창도 정적 사전 구현 상태를 유지한다. 다음은 Zsend를 원문에 따라 무시험으로 연결하고, 남은 Edit/시나리오·수명 경로를 진행하는 것이다.
