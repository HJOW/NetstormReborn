# 단위468 CF_TEXT 클립보드 부모와 최초 TextEntry 문맥 연결

2026-10-26 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 원본/정밀 export를 변경하지 않았다. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었다. 이 부모와 실제 TextEntry 연결을 단위468 제품에 적용했다. 최종 빌드·회귀·GUI와 인계 결과는 [통합 문서](cpp-editor-input-runtime-20261027.md)에 기록한다.

현재 제품에는 원문 clipboard read/write, 실제 Win32 장치, 최초 HUD Chat과 후속 Save as가 공유하는 장기 `TextClipboard`를 연결했다. **TextClipboard 신규2개와 최초 문맥 Save as/Console 통합 신규1개가 통과했다. 실제 Windows 클립보드에 복사하고 외부 앱/게임에서 붙여넣는 OS 검증은 아직 하지 않았다.** 단위 전체의 빌드/회귀/GUI 완료와 구별한다.

## 원문 두 부모와 실패 경계

정밀 `extracted/refined/originals/Netstorm.c:173020`의 `004dd810`과 `173054`의 `004dd870`, 실제 PE 바이트를 기준으로 했다. read의 CD10.72 대응은 `004beb00`으로 정밀 분석에 기록되어 있으나, 이번 관찰/제품 검증은 10.78에 한정했다. write의 CD 대응은 미확정이다.

| 부모 | 실제 순서/관문 |
| --- | --- |
| read `004dd810` | 출력 첫 BYTE0→IsClipboardFormatAvailable(**CF_TEXT=1**)→OpenClipboard(**NULL**)→GetClipboardData(1)→GlobalLock→strncpy(output,source,capacity)→GlobalUnlock→CloseClipboard. 형식 부재/Open0은 즉시 반환하며, Get0/Lock0은 Close로 진행한다. |
| write `004dd870` | OpenClipboard(**NULL**)→EmptyClipboard→GlobalAlloc(**flags0x2000,len+1**)→GlobalLock→명시 len BYTE 복사→마지막 NUL→GlobalUnlock→SetClipboardData(1,handle)→CloseClipboard. Open0은 즉시 반환하고 Alloc0은 Close 뒤 반환한다. |

현재 게임 창 HWND를 Open에 전달하거나 `GMEM_MOVEABLE`을 추가하지 않았다. 원문 할당 플래그는0x2000 그대로다. CF_UNICODETEXT/UTF-8 변환, trim, CR/LF 교체, 문자열 첫 NUL에서 쓰기 길이 절단을 추가하지 않는다.

read의 strncpy는 짧은 문자열 뒤를 capacity까지0 padding한다. 입력이 capacity 이상이면 끝 NUL을 강제로 쓰지 않는다. 원문은 GlobalSize를 조회하지 않으며 구현도 새 크기 조회를 붙이지 않았다. 기존 TextEntryBuffer의 “NUL 없는 실제 물리 범위 거부”는 별도 호스트 경계다. clipboard 부모에서 마지막 BYTE를0으로 보정하여 숨기지 않는다.

write는 Empty/Unlock/Set/Close 반환을 원문처럼 무시하고 **어느 분기에도 GlobalFree를 추가하지 않는다.** Set 실패를 보고 별도 성공/실패 UI를 만들지 않는다. Alloc 성공 뒤 Lock0은 원문이 NULL로 복사하거나 마지막 NUL을 쓰는 접근 실패다. 제품은 같은 위치를 `out_of_range`로 드러내며, 정상 반환/Close/Free를 합성하지 않는다. 실제 OS 오류가 정상 처리됐다는 뜻이 아니다.

read의 빈 출력 span, DWORD 밖 capacity와 write의 len+1 DWORD 감김을 만드는 초대형 입력은 유효한 호스트 물리 범위 밖이므로 OS 진입 전에 거부한다. 임의 원문 포인터/스택 overread까지 안전하게 재현했다는 주장은 하지 않는다.

## 실제 제품 배선과 최초 문맥

`TextClipboard.h/.cpp`는 선택된 OS 장치만 해당 위치에서 요구하는 두 부모와 `Win32TextClipboardDevices()`를 제공한다. API 팩토리는 callback을 만들 뿐 클립보드를 열거나 변경하지 않는다. 실제 Win32 장치는 User32/Kernel32 반환을 값 변환만 하여 위 부모에 넘긴다.

`UberGump`가 장기 `shared_ptr<TextClipboard>`를 만들고, `BindTextClipboard`가 `RawTextEntryContext`의 read/write 두 callback에 같은 부모 수명을 보유한다. `BindTextClipboard`는 **clipboard 두 필드만** 바꾸며 now/recipient/complete/submit를 유지한다.

중요한 설치 위치는 `RawMenuControls::EnableTextEntry`의 최초 `if (!textEntries)`다. HUD의 Chat 생성이 Save as보다 먼저 공통 TextEntry를 만들면 뒤 Save as 문맥은 공통 처리기를 교체하지 않는다. 따라서 `UberHud.inc`의 첫 `createChat`과 `UberEditor.inc`의 Save as 문맥 양쪽에 같은 장치를 공급했다. 현재 `client_.WallSeconds()`의 now를 게임 시각이나 상수로 바꾸지 않았으며 기존 Console 완성/제출 callback도 보존했다. 공통 문자 UI를 뒤에서 새로 만들거나 전체 문맥을 덮어써 누락을 숨기지 않는다.

## 독립 Native 관찰과 제품 검사

`tools/decomp_text_clipboard.py`는 보호 PE의 두 부모, 실제 CRT strncpy `004e51f0`, write rep movsd/movsb와 마지막 NUL 명령을 실행한다. Win32 IAT만 명시 반환/HANDLE/메모리 장치로 대체한다. 제품 C++의 결과로 기대값을 계산하지 않고 허용한 CODE/helper 바이트도 전후 비교한다. Native 도구의 `gameRuns=0`, `osCalls=0`이다.

- fixture `cpppj/tests/fixtures/text-clipboard-x86.tsv`: **880관측 = read240 + write640**, 이 중 **정상 반환800 / write LockNULL 접근 실패80**이다. read의 capacity1/8/256, 빈/ASCII/내부NUL·CRLF/고위BYTE/260BYTE 입력과 OS 반환 분기를 교차한다.
- 열은 `action/inputhex/capacity/flags/completed/faultPC/outputhex/trace`다. faultPC는 원문 접근 실패 위치이고 정상 반환으로 세지 않는다. outputhex는 명시 본문 뒤 가드까지 포함하며 trace는 호출 순서/인자를 보존한다.
- Native 보고서 `cpppj/text-clipboard-native-20261026.json`은 보호 PE SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 실제 부모/CRT 바이트, 도구 SHA를 기록한다. fixture SHA256은 `79537abf02125545001553dafdb855f024727fbe6daf642092232ff864b95f31`이다.
- 재검증 원시 `extracted/cpp-editor-input-468-native2.log`에서 같은880/80/SHA를 확인했다. 최초 채택 중 LF→CRLF 바이트 불일치는 원시를 보존한 뒤 최초880행 바이트로 복구하여 다시 확인했다. 기대값이나 기존 fixture 의미를 바꾼 것이 아니다.
- `TextClipboard_ReplaysNative880ByteCapacityAndFailureOrders`:880행의 정상/실패, 본문/가드 BYTE, 전체 trace를 대조하며80개 호스트 예외를 원문 fault와 구별한다.
- `TextClipboard_BindsActualTextEntryCopyPasteAndPreservesOtherContext`: 실제 TextEntryStorage Copy/Paste, 동일 부모의 callback 수명, 기존 now/submit 보존을 검사한다.
- `SaveFortNameDialog_FirstTextContextClipboardRoutesToMouseSaveAndRetirement`: 먼저 설치한 공통 TextEntry의 clipboard/벽시각으로 실제 Route Copy/Paste→Save as 마우스 down/up/버튼 serial 확정→파일/헤더→모달 반납을 실행한다. 뒤 Save as가 제공한 대체 now/submit는 쓰이지 않으며 이후 Console 제출도 최초 callback을 사용하는지 확인한다. 여기서 clipboard HANDLE/메모리는 명시 장치이고 실제 OS가 아니다.

제품 집중 원시는 `extracted/cpp-editor-input-468-focus1/TextClipboard.log`의 **2개/실패0**, `SaveFortNameDialog.log`의 **6개/실패0**이며 후자의 신규 clipboard 사례는1개다. Save as 기존5개를 이번 신규 개수로 중복 합산하지 않는다. 같은 Source1의 다른 Catalog 설계/검사 실패는 이 클립보드 통과와 별개이며 단위468 전체 성공으로 보고하지 않는다.

## 남은 검증

실제 Windows에서 Ctrl+C/X/V 및 Ctrl/Shift+Insert 계열 입력으로 최초 HUD 생성 뒤 Save as와 Console이 같은 CF_TEXT를 소비하는지 확인해야 한다. 현재 단위 검사의 핵심 Route는 Control+Insert 복사와 Shift+Insert 붙여넣기이며 실제 OS 성공/앱 간 전달 증거는 없다. 실제 실행에서는 원문 NULL HWND/0x2000을 바꾸어 통과시키지 말고 Open/Alloc/Lock/Set의 실제 반환과 사용자에게 보이는 결과를 별도로 기록한다.

ASCII/고위BYTE/CRLF의 OS 복사와 TextEntry 제외문자/이름 확정의 문자 치환을 구별한다. 시작 전 클립보드 보존/복원 범위와 실패 원시를 기록하고, 이 결과를 전체 게임 입력/한국어 클립보드 지원 완료로 확대하지 않는다. 최종 빌드·전체 회귀·GUI·LEFT·커밋은 통합 담당이 수행한다.
