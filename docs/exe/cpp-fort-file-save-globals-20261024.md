# Fort 파일 저장 관문의 실제 전역과 Client 바인딩 설계

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. AGENTS·공통/닷넷 인수인계, 보호10.78 PE/정밀 C와 현재 C++를 읽기 전용으로 조사했다. 이 작업은 이 새 문서만 작성하며 제품·동결 모듈·검사·CMake·LEFT·공유 빌드·GUI·git·원본/CD를 변경하거나 실행하지 않는다. 아래 초기값/명령은 정적 PE 근거이며 새 native 실행 fixture나 실제 파일 저장 성공 관측이 아니다.

**핵심 결론:** 현재 `playerState.battleOwner`와 `battleOptions+0x3e` 숨김 span은 실제 Session 소비자들이 이미 공유한다. **readOnlyForts의 장기 Client 소유 필드와 설정 변경 연결은 아직 없다.** 새 필드의 최초0은 원본 BSS 근거가 있지만, 저장할 때마다 지역0이나 설정 getter의 기본0으로 채워 복원을 가장해서는 안 된다.

## 원본 초기값과 실제 쓰기

| 전역 | 실제 PE 초기값 | 쓰기와 현재 제품 소유자 |
|---|---|---|
| readOnlyForts `0054db30` DWORD | 원시파일 범위 밖 가상0 영역, 즉 최초0 | 실제키주소전달은 `00435860 push 0054db30` → `0043586a call 00441270` 한곳이다. 현재Client에대응필드/읽기가없다. |
| 현재 Player owner `00540c70` DWORD | 원시 `.data`의 `01000000`, 즉1 | 실제직접쓰기는 `0048f3c2` 한곳이다. 현재Session `Impl::playerState.battleOwner`가같은역할을소유한다. |
| 숨김 `0052f81e..0052f826` 9BYTE | 원시 `.data`의9BYTE모두0 | `0052f7e0` 87BYTE옵션배열의+0x3e이다. 현재Client의`missionOptions_`와Session의`battleOptions`/`hiddenPlayers`가같은실제배열을빌린다. |

`0054db30`에는 별도 직접 MOV 초기화가 없다. 로더의 가상0 이후 일반 `ReadInt`가 키를 찾고 값이 달라졌을 때 현재 전달 포인터에 쓴다. 그 실제 간접 쓰기는 **004412dd `mov dword ptr [esi],eax`**다. `ReadInt`는 누락 키에서 현재값을 보존하고, signed `atol`의 전체 DWORD를 쓰므로 -1 등 비영값도 저장 억제다. bool1만 억제로 판정하거나 매번0에서 읽으면 안 된다.

전체 실행코드의 readOnly 주소 즉시참조는 위 키주소전달과 **0044d915의 저장 관문 읽기**뿐이다. 정밀 C의 호출자/중복 블록을 별도 readOnly 전역 쓰기로 세지 않는다. 같은 `Interpret Options00435220`의 재호출은 값 쓰기 코드가 여러 개인 것이 아니라 같은 `ReadInt` 지점에 다시 도달하는 것이다.

## readOnly 설정 변경의 현재 누락

원본 `004359f0`은 `00441790(00435220,...)`로 설정 변경 callback을 등록하고 시작 설정을 읽은 뒤 `00435a0a`에서 `00435220`을 호출한다. `00440e00`은 받은 callback을 `00558dd0`에 보관한다. 이후 `004414d0`의 Set/SetInt 및 스크립트 Config 명령이 실제 설정을 쓴 뒤 이 callback을 동기 호출한다. 따라서 `Config(readOnlyForts=1)`도 다음 저장 전 같은 장기 DWORD에 반영된다.

현재 PE의 직접 `Interpret Options` 호출은 다음 세곳이며 설정 변경 callback 호출은 간접 경로다.

- `00435a0a`: 시작 설정 해석.
- `004365e0`: 원본 등록 창 메시지의0x100 분기에서 설정 재적재/해석.
- `004b325f`: 상태 정리 `004b2df0` 뒤의 옵션 재해석.

현재 C++의 `Client::Run`은 `ClientMain.cpp`에서 Startup→SaveOptions→InterpretOptions를 수행하지만, 그 부분 구현은 `readOnlyForts`를 읽지 않는다. `ConfigInterface::Set`에는 `onChanged` 호출이 이미 있으나 **제품 전체에서 그 callback을 설치하는 코드가 없다.** `UberGump`의 Config 명령은 `ApplyScriptConfig` 뒤 소리 옵션만 적용한다. 저장 때 Config를 새로 읽는 임시 getter를 붙이는 것으로 이 Client 상태 소유권/변경 경로를 완료했다고 하지 않는다.

권장 연결은 `Client` 수명의 실제 DWORD 필드(또는 한 DWORD를 소유한 주입 객체)를 두고, 초기0→현재 `ReadInt("readOnlyForts",...)`를 시작 해석과 같은 설정 변경 해석에 연결하는 것이다. `ReadInt`가 signed참조를 요구한다면 `bit_cast<int32_t>(현재DWORD)`의 지역값으로 읽고 결과를 `bit_cast<uint32_t>`해 **같은 소유 필드에 대입**한다. 지역값의 참조를 저장 소비자에 보관하지 않는다. 콜백은 Read만 수행해 재귀 Set을 만들지 않으며, 다른 기존 옵션 효과를 덮어쓰지 않도록 Client의 해석 경계에 합친다. 전체 그래픽/음향 옵션 해석이 복원됐다고 주장할 범위는 아니다.

구체적인제안API는 `Client::ReadOnlyForts() const -> const uint32_t&`와 Session의 `BindFortFileSave(const uint32_t& readOnlyForts,const EditorFiles& files)`다. 두참조의실제소유자는Session보다오래살아야하며파일저장바인딩을명시할때모두필수로받는다. 이는현재코드에있는API가아니라후속구현안이다.

## 현재 owner의 실제 변경과 수명

원본 setter **0048f3b0**은 먼저 `00540cac=둘째인자`, `00540c70=첫째인자`를 쓴다. owner0이면 현재 CoreData를 보존/분리하고, 비0이면 PlayerKnown과 현재 CoreData 선택을 갱신한다. 파일 저장은 **첫째 Player owner00540c70**을 숨김 인덱스로 사용하며 Template owner00540cac와 합치지 않는다.

실제 PE의 setter 직접 CALL은 `004a78a4`, `004b312d`, `004b3257`, `004b3b87`, `004b4752`, `004b4d40`, `004b6ef3`, `004b71f5`, `004b7e08`, `004d52a9`의10곳이다. 로그인/상태 전환/편집 진입/소유자 선택 경로에서 같은 setter를 사용한다. C에 인자가 일부 생략되거나 겹친 블록이 있어도 값형기본1을 새 저장용 owner로 만들어서는 안 된다.

현재 `RawMissionClientPackets::LoginBegin`은 같은 `PlayerWorldState`의 templateOwner/battleOwner를 패킷의 현재 번호로 쓴다. 로그인/참여/현재Technology와 메뉴도 이 `playerState.battleOwner`를 읽는다. `RawCampaignSession::LocalOwner()`는 값 반환 getter이므로 `DataManagerFileSaveState`의 장기 `const uint32_t&`로 보관할 대상이 아니다. Session 내부 BindFileSave에서는 실제 `s.playerState.battleOwner` 참조를 직접 사용한다.

이 필드는 현재 **Session 소유**이며 Client 전체의 별도 owner 필드는 없다. 살아 있는 Session의 Notify→저장에는 같은 참조를 사용할 수 있지만, 월드 해제 뒤 저장까지 지원해야 한다면 원본 Player 전역의 Client 수명을 별도로 통합해야 한다. 그 공백을 `world가 없으면1/0`으로 채우거나 파일 저장 전용 복제 owner를 두지 않는다. Session이 사라진 뒤 그 필드 참조를 캡처한 파일 callback도 남기지 않는다.

## 숨김 표의 alias와 변경 경로

현재 물리 소유자는 `UberGumpClient::missionOptions_`의87BYTE 배열이다. `RawCampaignSessionServices::missionOptions`로 같은 shared_ptr를 받고, Session은 `battleOptions` 참조와 **`battleOptions.data()+0x3e,9`** span을 만든다. `RawMissionClientPackets` 생성자는 숨김 span이 그 실제 주소와 같은지 확인한다. 메뉴/참여/로그인 소비자도 같은 숨김 span을 빌리며 별도 active/동맹/표시bool 배열로 대신하지 않는다.

원본 옵션 reset `0041cb90`은 +0..22와+63..86을 쓰고 **+23..62를 보존**한다. 따라서 숨김[1..8]인+0x3f..+0x46은0으로 지우지만, **숨김[0]인+0x3e는 보존**한다. 실제 전역 reset wrapper0041cc10은 `004b4ed9`, `004b7219`, `004c3308`의세곳에서 호출된다. 다른 `0042c5c4→0041cb90`은 메타표시 객체의 옵션 복사본 생성이며 전역을 새로 지우는 호출로 세지 않는다. 현재 `ResetMissionBattleOptions`도 그 보존 범위를 유지한다.

게임중 변경에는 다음 경로가 있다.

- 원본 `004b4900`은 길이<=87을 진단하고 실제 수신 길이만 전역0052f7e0에 복사한다. 길이가 숨김 바이트에 못 미치면 그 뒤는 보존된다. 현재 `RawMissionClientPackets::Options`는 같은 배열에 복사하며 파생 효과를 이어간다.
- `0041c6a0`은 인덱스0..7에 대해 `this+0x3f+index`를 **기존값!=1**로 바꾼다. 2/ff도1이 되며 단순 boolean NOT과 다르다. 실제 호출 `0047ab5c`/`0047adcb`은 `0047a900`의 메타표시 입력에서 선택된 **옵션 복사본**을 수정하고 변경을 전송한다. 이 지점의this를 전역 배열이라고 가정해 직접 전역쓰기로 바꾸지 않는다. 현재 공유 옵션 수신 경계와 UI 복사본→전송의 복원 범위를 구별한다.

특히 원본 초기값이9BYTE0이라고 해서 매 저장/매 Session에 숨김 표를 다시0으로 만들면 안 된다. 일반 reset도 숨김[0]을 보존하며, 패킷/메뉴/실제 현재 값은 그 뒤 계속 바뀐다.

## 안전 작업본 writer에 연결할 계약

`RawDataManagerFileSave`는 이미 `DataManagerFileSaveState{const uint32_t& readOnlyForts,const uint32_t& localOwner,span<const uint8_t> hiddenPlayers}`를 요구한다. 이 설계를 유지하고 **이미지 생성 뒤** 세 현재 입력을 읽도록 한다. 현재 readOnly가비0이거나 해당 owner의 숨김 BYTE가비0이면 파일 권한/쓰기만 생략하고 이미지 생성/반납은 수행한다. 권한 callback 뒤에는 저장 관문을 다시 검사하지 않는다.

실제 바인딩은 Session 안에서 `templateState->BindFileSave`에 장기 readOnly 소유자 참조와 살아 있는 Session을 연결하고, callback이 실행될 때 `Storage.SaveFile(slot,{readOnlyOwner,s.playerState.battleOwner,s.hiddenPlayers},현재imageHooks,EditorFiles.Writer())`를 호출하는 방향이다. 세 값을 저장 시점 이전에 복사해 고정하거나, 파일 장치가 없다는 이유로 지역0·숨김0배열·빈 성공 writer를 공급하지 않는다. 저장 기능을 요구하는 Session에 공급자가 없다면 도달한 실제 경계에서 명시 실패해야 한다.

`EditorFiles`는 이미 작업 루트/보호된 originals·originalCD 루트를 정규화하고, 권한 변경과 쓰기 직전에 각각 확인한다. 같은 실제 manager의 가상 파일명과 사용자 작업본 경로를 먼저 연결해야 하며 현재 설치가 mission이름만 넘긴다는 공백은 별도다. 단순 `EditorFiles.Writer()` 추가만으로 작업 namespace가 완성되지 않는다. 원시섹션→전체snapshot→Notify→현재manager 이미지→관문→실제작업파일의 순서를 유지한다. 작업본 준비는 명시 Edit/New/선택 단계의 별도 호스트 동작이며 저장 관문을 우회하는 성공 처리로 사용하지 않는다.

후속 검사는 Client 설정 -1/1/0/누락 키 보존과 Config 명령 즉시 반영, 동일 owner/숨김 주소, 이미지 생성 중 세 값 변경의 재조회, 숨김[0] reset보존, 비영 gate의 이미지 생성/반납 및 작업 파일 무변경, 실제 작업 파일 재파싱을 포함해야 한다. 기존 DataManagerFileSave 독립144입력과 EditorFiles 실제 작업본 검사를 재사용하되, 아직 없는 Client 전역 바인딩의 통과로 세지 않는다.

원본 PE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 현재 정밀 C는 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`이다. 이 문서는 읽기 설계이며 실제 파일 저장 연결·generic Editor·전체 사용자 완주/동일성의 완료 판정이 아니다.
