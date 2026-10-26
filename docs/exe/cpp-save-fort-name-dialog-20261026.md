# Save as 실제 이름 입력 부모·공통 모달 조립

작업 PC는 **HJOW-Athlon**, 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 원본/CD·AGENTS.md·디컴파일 결과·기존 fixture를 변경하지 않았다. 실제 게임·빌드·통합 검사·커밋·LEFT_JOBS 갱신은 root가 맡는다. 아래 새 모듈 작성과 Native 관찰만으로 제품의 자연 Save as 완료를 주장하지 않는다.

## 구현 범위

`SaveFortNameDialog.h/.cpp`는 원본 `004d3250/004d3490` 생성 외곽, `00487530/00487360` 두 버튼 배치, `00487460` 입력 부모, `004cf9a0` 확정, `004cfab0` Cancel을 제공한다. 실제 문자/Gump/파일 본문은 기존 모듈에 위임한다.

`RawMenuSaveFort.inc`와 `RawMenuControls` 변경은 같은 등록표의 `SaveFortNameDialogNode`/현재 장치/소유 토큰을 보관하고, 실제 장식·TextEntry·한 줄 TextGump·돌 버튼·배경·Route·Draw·Redraw·모달·자식 우선 반납에 연결한다. 별도 HTML·Win32 입력창을 만들지 않는다.

공개 API는 다음과 같다.

```cpp
ShowSaveFortNameDialog(context, fortMode = 1, canCancel = 1, initial = {});
SaveFortNameNode(token);
CloseSaveFortNameDialogs();
```

`SaveFortNameDialogContext`에는 필수 `const EditorFiles* work`, `editor.now`, 번역, 현재 `fortPath(name)`, 현재 owner20BYTE `header()`, 전체 `setName(name)`, `tell(section)`이 들어간다. fortMode0을 연결하려면 별도 `readFort(name)`도 필요하다. 선택된 장치가 없으면 예외이며 앞선 부분 효과를 성공으로 바꾸지 않는다.

root가 연결할 `setName`은 **같은 work 장치를 전달한 FortCurrentNameDevices**를 사용해야 한다. 이 함수가 readonly 변경 뒤 실제 전체 snapshot·manager 이름·old 경로 삭제·제목/목록 dirty를 소유한다. GUI 모듈은 snapshot이나 파일 형식을 재구현하지 않는다.

## 실제 원문 확인과 준비 문서 정정

다음 사항은 준비 문서의 refined C 표기만으로 판단하지 않고 보호 PE 기계어/초기 바이트를 다시 확인했다.

- Save as kind `0054043b`의 값은 **32**다. 생성 후 VT는 `00517bc8`이다.
- TextEntry `+100=40`, `+f4=0`이며 **+f4는 multiline**이다. `changeNotice(+f8)`가 아니다. Enter를 실제 부모에 전달하려면 이 구분이 필요하다.
- 제목의 기본 TextGump는 부모폭×float32 0.8을 _ftol한 폭, **높이20·font0·색0**이다. 실제 제목 문자열 조각은 문맥 `{2,3,현재흰색,0,0,0,1,1}`로 추가한다. “font20”으로 해석하면 안 된다.
- 기본 대화창 높이는100이고 두 버튼 배치 `00487360`은 부모 높이를 쓰지 않는다. 생성자 마지막 +4 뒤 **104**다.
- 두 버튼 Push의 관문은 `flags&8`이다. 각 버튼은70×18 생성 뒤 실제 FitToText를 실행한다.
- `004652b0`은 모든 자식부터 깊이를 **더하는 AddDepth**다. 인자는 현재 부모 depth−76이다. SetDepth로 바꾸지 않는다.
- 배경 `005ca920=fortGump`, `005ca924=0` 초기화가 실제 소스에 있다. 편집 테두리는 같은 TYPE/프레임0을 사용한다.
- Cancel leaf는 현 functions.tsv에서 별도 함수로 누락돼 있다. `004cfab0..004cfad1`의 실제34BYTE는 Native 보고서에 기록했다. 디컴파일 파일에 임의 함수를 추가하지 않았다.

## 확정/Cancel의 Native 독립 관찰

`tools/decomp_save_fort_name.py`는 실제 `004cf9a0`과 누락 Cancel leaf를 x86 Unicorn에서 ret까지 실행한다. 첫 LF 절단·문자별 분류 호출/치환·빈 이름·존재 분기·header+8 직접0쓰기·선택된 전체 이름/ReadFort 예약·반환은 원문 CPU가 수행한다.

- **288개 관측**: Confirm256개 + Cancel32개다.
- 8개 이름 입력을 mode/cancel/manager/readonly/exists의 두 값과 교차한다.
- 실제 게임/OS 호출은0이다. 스택 복구를 검사한다.
- CopyText, ASCII CRT 분류, path/exists, 전체 이름, Tell, 문자열 고리/명령 큐는 **명시 외부 장치**다. 전체 파일 저장이나 GUI helper 실행으로 합산하지 않는다.
- 원본 CRT strchr의 허용 기계어 body를 실행한다. isalnum은 이번 ASCII 입력에서 명시 CRT 경계로 기록한다. 비ASCII의 원본 CRT locale 동작은 이 관찰에 포함하지 않는다.
- ReadFort 큐의 현재 전역값 조회를 확인하려고 명시 입력 `00542a1c=0x020000ce`, 문자열 토큰 `0x11223344`를 사용했다. **이는 PE의 초기 명령값이라는 주장이 아니다.** 실제 제품의 ReadFort 초기 ID는 기존 `kReadFortCommand=0x020000ca`를 사용해야 한다. 최초 fixture의 명시 입력을 후속에 실제 초기값처럼 재해석하거나 고치지 않는다.

최초 fixture는 `cpppj/tests/fixtures/save-fort-name-x86.tsv`, 보고서는 `cpppj/save-fort-name-native.json`이다. 첫 생성 뒤 `--verify`로 원문 부모를 재실행해 **fixture 전체 바이트 동일**을 확인했다. 생성 도구는 기존 fixture를 기본 모드에서 덮어쓰지 않는다.

## 현재 준비한 C++ 검사

`SaveFortNameDialogTests.cpp`의 신규5개이며 최종 Source6 집중/전체 검사와 실제 GUI를 통과했다. [최종 통합](cpp-editor-menu-runtime-integration-20261026.md)에 결과와 남은 새 이름 목록 재진입을 구별해 기록했다.

1. `SaveFortNameDialog_ReplaysNativeConfirmAndCancel288`: 원문288행 반환·readonly·확정 문자열·외부 효과 순서를 모두 재생한다.
2. `SaveFortNameDialog_PreservesReadonlyPrefixOnSetFailure`: 전체 이름 장치의 미연결 실패 앞에서 이미 쓴 readonly0을 보존한다. canCancel1은 Tell을 요구하지 않는다.
3. `SaveFortNameDialog_ActualModalTextEntryEnterAndRetirement`: 실제 보호 자산/팔레트/글꼴·공통 Tree에 모달/자식을 생성하고 실제 문자 Route·Enter → 같은 work의 파일 장치 callback → Kill/Cleanup → 문자 버퍼 반납을 검사한다. 파일 callback은 조립 증빙이며 실제 Fort 직렬화 완료는 root의 제품 연결에서 따로 확인한다.
4. `SaveFortNameDialog_ActualDuplicateEmptyEscapeAndCancel`: 중복/빈 이름의 창 유지, ESC 기본 경로, 원문 Cancel serial, 강제 이름 입력 canCancel0 안내와 명시 정리를 검사한다.

5. `SaveFortNameDialog_ActualMouseUsesSerialAfterPriorGumpRetirement`: 이전 창 반납 뒤 등록 id와 다른 serial을 가진 새 OK를 실제 마우스로 발화해 저장·Kill·모달 반납을 확인한다.

존재 검사는 work로 경로 경계를 검증한 뒤 error_code 버전 filesystem::exists를 사용한다. **EditorFiles::Exists는 fopen 기반**이라 디렉터리를 항상 비영으로 취급하지 않으므로, 원문 GetFileAttributesA 조건인 이 경계에 직접 재사용하지 않았다.

### 467 통합 중 추가로 확인한 원문 경계

최초 실제 모달 검사의 `AllocationCount==0` 기대는 제외문자 문자열 수명을 놓쳤다. `00452730` 소멸자는 +0x108의 소유 버퍼만 반납하고 +0x104의 제외문자 포인터를 반납/0쓰기 하지 않는다. 기존 Native 도구를 변경하지 않고 `excluded=\\/:*?"<>|`, owns0/1 × x87 `027f/037f`의 **비영 포인터 입력4회**를 추가 실행했다. 네 번 모두 +0x104는 전후 `16801792`로 보존되었다. owns1은 `release:16842752;release:16908288;delete:16781312;destructBase:16814080`만 호출하고 +0x108만0이며, owns0은 destructBase만 호출한다. 실제 base `00464420`의 정적 원문도 +0x78/+0x7c만 반납한다.

입력·전체 출력·trace·코드/기존 fixture SHA를 포함한 원시는 `extracted/cpp-save-name-excluded-467-native.json`, 파일 SHA256은 `6e55b64bfbfbc77b0d8271e3e1b756132379274997bb59d82166b38da4f3e16b`이다. 기존 fixture는 바꾸지 않았다. 따라서 새 모달 검사에서는 root/버퍼 반납과 제외문자 한 블록 잔류를 분리하고, 최상위 `TextEntryStorage`의 RAII 종료 시 호스트 할당 반납을 별도 수명으로 판단한다. 원문 소멸자에 새 해제를 끼워 넣지 않는다.

실제 마우스 GUI는 기본 두 버튼 부모의 비교값도 드러냈다. PE `00487484`의 OK 비교와 `004874b8`의 Cancel 비교는 모두 **자식+0x2c(serial)**다. 최초 모듈의 id 비교와 직접 id를 넣은 검사는 실제 버튼 발화를 대조하지 못했다. serial 비교로 교정한 뒤 실제 mouse Route 검사와 같은 최종 제품의 OK/중복거부/Cancel GUI를 통과했다. 원시와 최종 보고서를 함께 보존했다.

## 제품 연결 후 필요한 검증

메인 Edit→기존 Fort→ESC Edit/Save as→실제 문자 입력·OK→새 작업본 Fort 파싱→SaveGoMain→새 이름 선택·재진입을 같은 제품에서 검증한다. 기존 readonly0 강제 뒤 old/new snapshot·old 삭제가 실제로 선택될 수 있으므로 old/new 경로 모두 editor work에 머무는지 확인한다. 취소·중복·빈 이름·강제입력과 화면 재진입 시 문맥 수명, 원본 보호 해시도 함께 확인한다.

새 모듈·fixture와 기존 공통 TextEntry/HTML/Chat/메뉴 회귀를 집중 실행한 뒤 같은 제품의 전체 검사와 GUI 증거를 남긴다. Add Island/NewFort/Test Battle/언어별 시나리오 및 전체 미션 완주는 이 단위의 완료에 합산하지 않는다.

최종 serial/제외문자 원문 근거는 [추가 Native 보고서](../../cpppj/save-fort-name-ui-native-20261026.json)에 보존했다. 새 이름 메뉴 재진입·clipboard·NewFort·일반 SaveGame은 다음 단계이며 완료로 합산하지 않는다.
