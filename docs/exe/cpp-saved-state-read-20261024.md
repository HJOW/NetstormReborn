# 저장상태 reader 004b5890의 독립 복원

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 원본10.78의 실제772BYTE와 별도 SEH 파일 cleanup11BYTE를 Unicorn 격리 메모리에서 관측했다. 게임/OS/GUI·공용 빌드·공용 라이브러리·git 실행이나 공유 기존 소스 수정은 하지 않았다.

[도구](../../tools/decomp_saved_state_read.py)가 제품 작성 전에 [최초 fixture](../../cpppj/tests/fixtures/saved-state-read-x86.tsv)와 [보고서](../../cpppj/saved-state-read-native.json)를 고정했다. **271입력×두x87정밀도=542관측/375590명령**이며 정상506/명시중단32/계약밖 물리실패4, 정밀도 차이0이다. 본문 중단 중24회는 원문 cleanup004fb5d0→0041b410을 별도로 실제 실행해 파일 해제를 기록했다. 첫 도구의 trace 라벨 TypeError는 fixture 작성 전에 발생했고 원문 입력 변경 없이 교정했으며 [실패 기록](../../extracted/cpp-saved-state-read-20261024-observation-first-failure.txt)을 보존했다.

최초 보고서 entries와 도구 설명의 cleanup6BYTE는 바이트 수 표기 오류다. 실제 허용·실행 범위는 처음부터004fb5d0..004fb5db의11BYTE였으며 [별도 정적 교정](../../cpppj/saved-state-read-cleanup-scope.json)이 실제 lea6+jmp5와 초기 자료 SHA 불변을 확인한다. 최초 도구/fixture/report를 고쳐 쓰지 않았다.

[신규 API](../../cpppj/src/o/RawSavedStateRead.h)는 `ReadSavedState(scratch, globals, hooks) -> int32_t`다. [본문](../../cpppj/src/o/RawSavedStateRead.cpp)은 다음 원문 순서를 따른다.

1. 실제004b5470이 반환한 같은 NUL 경로 주소를 한 번 받는다. 존재004dd560이0이면 다른 장치를 요구하지 않는다. 파일 생성자0041b4c0(path,0,0) 앞에는 같은 지역 File24BYTE의 마지막 두DWORD만0으로 쓴다.
2. 서명4BYTE와 이름256BYTE를 각각 읽으며 **read 반환길이를 검사하지 않는다.** 서명0x000a004e 불일치와 첫NUL 앞 점은 파일 해제 후0이다. 점 뒤 NUL이 없더라도 점이 먼저면 원문처럼 조기 실패할 수 있다.
3. 같은 pending helper00460bb0을 호출한 뒤00542528에ffffffff를 먼저 쓴다. 그 실제DWORD에 저장 owner를 읽고 지역 저장시각/인접DWORD를 읽는다. current Fort 이름 변경으로 대체하지 않는다.
4. 매8회 반복마다0042ce70으로 같은 지역 Core10BYTE를 초기화한다. 번호/ID/첫이름30/둘째이름30/flags/필드+58/Core10BYTE의 일곱 read를 순서대로 수행한다.
5. 그 뒤 현재0059534c의 실제 Player base를 읽고 해당 레코드 주소를 고정한다. flags의0x40만 제거하고 Player+2c의 ID를 먼저 쓴다. 저장 owner와 같으면0054252c를 갱신하고 **새 이름을 쓰기 전에 기존 Player+0c의 이름을005c84c0으로 복사**한다.
6. 두 새 이름과 flags+50/필드+58/Core+6c의4/4/2BYTE를 쓴다. 실제0048f350에 번호/ID/Core10BYTE 값을 전달한다. 이 Core 인자는 원문처럼 호출 전에 값으로 복제되며 별도 장기 상태가 아니다.
7. metadata 복귀 뒤 지역 flags를 다시 읽는다. bit1이 켜지고0x200이 꺼진 레코드만 참가자 수에 더한다. 번호가 다르거나 비참가여도8회 읽기와 Player/metadata 쓰기를 생략하지 않는다.
8.00595090/0055a420/0055b4ac의 실제DWORD에 순서대로 직접 읽는다. 지역 종료DWORD를 먼저0으로 쓰고4BYTE read한다. 종료0이고 참가자2명 이상일 때만 signed 저장시각을 미리 보관해 파일 해제 뒤 반환한다. 늦은 실패로0을 반환해도 앞선 이름/Player/캐시/시각 전역의 부분효과는 남는다.

정상 close 직전에 파일 해제 책임을 반납한다. open 정상복귀 뒤 read/pending/Core초기화/metadata의 선택 중단에서는 원문 SEH와 같이 같은 파일을 한 번 해제한 뒤 중단을 전파한다. open 자체의 중단에는 이 부모의 close가 없고 normal close 중단을 다시 close하지 않는다. 정상반환값은 close 전에 보관하므로 close callback의 지역 시각 변경으로 바뀌지 않는다.

미정 stack는 `scratch`의 **명시 초기392BYTE**다. 실제 steadySP+10부터이며 flags0,번호4,ID8,File+0c24BYTE,Core+2410BYTE,반복+30,종료+34,필드+38,저장시각+3c,서명+40,인접DWORD+44,첫이름+4832BYTE,둘째이름+6832BYTE,Fort 이름+88256BYTE다. 모듈은 이를 임의0으로 만들거나 읽기 부족을 padding/파싱 실패로 바꾸지 않는다. 원문 Core 초기화·종료DWORD의 선행0쓰기만 해당 위치에서 수행한다.

`globals`는 실제 상위 owner의 lazy DWORD 참조와 Player/handle span을 빌린다. 파일 없음과 조기 실패에서 아직 선택되지 않은 실제 Client 전역이나 metadata 장치를 선행 요구하지 않는다. 00595360의 **9×16BYTE cachedCoreData**는005953f0의 현재 캐시10BYTE와 다르며 새 모듈이 독립 장부를 만들지 않는다.

실제 하위 접합은 후속이다.

- 경로/존재/open/read/close는 기존 파일 장치에 합성한다. 부분 read는 대상의 prefix만 덮고 반환길이만 돌려줘야 한다. 새 파일 parser나 원본 파일 수정 경로가 없다.
- 존재004dd560은 실제 `GetFileAttributesA(path)!=INVALID_FILE_ATTRIBUTES`이므로 디렉터리도 비영이다. 기존 Editor 작업 파일의 fopen rb→5/0이나 일반 파일 필터를 이 원문 장치의 전체 계약으로 대체하면 안 된다. 실제 파일 검사는 일반 파일 두 입력으로 한정했다.
- pending은 기존 `client::SetPendingFortName`을 재사용한다. 새 이름 정책이나 현재 이름 setter를 추가하지 않는다.
-0042ce70 초기화와0048f350의 실제9×16 장부 갱신은 필수 hooks다. 부모 관측은 그 명시 장치를 사용했으며 실제 Client 장부 소유자/로딩 완료를 가장하지 않는다.
- metadata 인자의 마지막2BYTE 뒤 stack padding2는 실제0048f350이 읽지 않는다. 의미있는 Core10BYTE만 값으로 보존하며 미정 padding을 새로운 장기 DWORD나 장부 값으로 만들지 않는다.
- Startup 부모의 `readSavedState`에는 이 signed 반환을 그대로 연결한다. 옵션 비영에서 reader0을 기본 성공으로 넣거나 Config를0으로 강제하지 않는다.

[신규4검사](../../cpppj/tests/SavedStateReadTests.cpp)는 지원 물리 **538관측 전체의 정상 EAX/전역/두Player/장부 padding/handle/392stack/파일cursor/전체trace/SEHcleanup24**를 대조한다. 잘린 read를 개별 변조해 다음 Player 번호가 물리 밖이 된4관측은 원문 분석자료로 보존하고 제품 성공으로 비교하지 않는다. host는 Player/NUL 물리 끝에서 명시 예외를 던지며 파일 해제를 수행하는 별도 계약 검사다. 실제 접근 위반의 OS/SEH 처리까지 복원했다고 주장하지 않는다.

추가 실제 파일 검사는 기존 BaseFileSystem/BaseFile.cpp/OriginalText.cpp를 자체 컴파일하고 **소유 extracted 작업 경로**의976BYTE 및 끝DWORD 없는972BYTE 파일을 읽었다. 정상 두 참가자에서 signed0x80000000 반환·8metadata·파일 해제·파일 불변을 확인했다.972BYTE도 종료지역0이 유지되는 원문 결과를 보존한다. 이 검사 장치가 실제 BaseFile 객체24BYTE나 OS 핸들 필드를 복원한 것은 아니다.

[격리 검증](../../extracted/cpp-saved-state-read-20261024-isolated/verification.json)은 MSVC Release `/W4 /utf-8 /permissive-` 빌드/4검사 종료0, 관측 경고·오류0과 최초 SHA/UTF-8/AST/공백/링크를 기록한다. 공용 등록·Client 전체 재접속·Editor 최종 HUD/State/loaded 성공은 아직 포함하지 않는다. CD10.72 정밀 대응은00432a10이며 이번 PE 관측은10.78만 대상으로 한다.
