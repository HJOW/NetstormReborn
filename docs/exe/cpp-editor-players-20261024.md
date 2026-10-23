# Editor Player004904b0의 이름·필드·직접 기술 생성

2026-10-24 **신규 독립 준비·미등록** 자료다. AGENTS와 공통 최신 상단/닷넷 인수인계를 다시 확인했으며 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행2026-10-23**으로 같다. 기존 정밀 C·보호 PE만 읽었고 새 디컴파일·게임/GUI·기존 제품/Client/CMake/LEFT/helper 변경·공유빌드·git은 수행하지 않았다. 신규 경로의 창 없는 격리 컴파일/검사만 실행했다. 실제 generic Editor 설치/장부/파일 저장 완료와 구별한다.

## 확정한 원본 범위와 순서

10.78 보호 PE SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`의 **004904b0~004905c0,273BYTE**를 실제 실행했다. 정밀 원문은 `extracted/refined/originals/Netstorm.c:115690~115755`다. 끝은004905bc의 **0046e980 tail JMP**이며 그 하위 몸체를273BYTE에 합산하지 않는다.10.72 정밀 대응은004077c0이지만 이번 함수/기대값은10.78 기준이다.

| 단계 | 실제 원본 효과 | callback 뒤 보존할 주소/값 |
|---|---|---|
| Human 이름 | translate("Human Player")004de9a0 →현재0059534c 읽기 →**Player1+0c(표+c0)**에 NUL까지 BYTE복사 | 번역이 Player표를 바꾸면 바뀐 표를 사용한다. Player1의 다른 필드는 쓰지 않는다. |
| 현재 handle | translate("Player") →현재00540ca8 포인터 읽기 →NUL까지 BYTE복사 | Player 번역 뒤의 현재 쓰기 버퍼를 사용한다. |
| owner2..8 준비 | **00490250(owner)** 호출/반환 SID 보관 →현재0059534c 재조회 →해당owner 레코드 주소 캡처 →**+2c DWORD=owner** | Player 생성 중 현재 표가 바뀌면 새 표를 사용한다. 생성된 부모 SID는 해당 owner의 타입 반복 동안 고정한다. |
| Computer 이름 | translate("Computer #%d") →004b9390(**capacity30**,번역 원문,owner−1) →**캡처한 레코드+0c**에 NUL까지 BYTE복사 | 번역/서식이 현재 표를 바꾸어도 이미 캡처한 레코드를 유지한다. 문자열을 다시 현재 표에 쓰지 않는다. |
| flags | 같은 캡처 레코드의 현재+50 DWORD 읽기 → **`(flags & 0xfffffeff) | 3`** 쓰기 | 번역/서식 callback이 바꾼 flags의 다른 모든 비트를 보존한다. bit8만 지우고 하위2비트를 세운다. |
| 직접 기술 | 현재signed00541348>0일 때 **타입0부터**,0049a840의현재기록 group!=10 && theme!=5마다 **004afa70(parent,type,0,4,0x10,0)** | 타입 조회/Create callback 뒤 count를 다시 읽는다. 매 Create 직전 원문은 현재005c8464와 고정 부모 SID를 결합한다. |
| 끝 | owner8을 끝낸 뒤0046e980으로 tail JMP | 후처리 실제 hook이 필수다. 정상 함수처럼 빈 callback을 자동 주입하지 않는다. |

타입 count가0/음수여도 owner2..8의 새 Player·이름·flags와 마지막 후처리는 실행한다. 선택 테마 필터는 없고 theme5만 제외한다. 타입0도 eligible이면 직접 생성한다. 기존 기술 존재조회/Find/Ensure/중복 방지는 없다. 같은 입력을 두 번 적용하면 동일 parent/type에도 Create 요청을 두 번 한다.

본문에는 **Config·창 제목·HTML 종료 호출이 없다**. 현재 handle이 바뀌었다는 이유로 fort.mySubHandle/제목/dirty 등을 추가하지 않았다. 별도 원본 설치 부모에서 요구하는 효과는 그 부모에 연결한다.

## 신규 모듈과 실제 연결 계약

[RawEditorPlayers.h](../../cpppj/src/o/RawEditorPlayers.h)/[.cpp](../../cpppj/src/o/RawEditorPlayers.cpp)는 `InitializeEditorPlayers(const EditorPlayersHooks&)`를 제공한다. hooks는 translate,현재players,현재handle,technology,format,typeCount/typeAt,createContained,finish다. 필드 쓰기/문자 복사/owner·타입 반복/eligible 판단은 본체가 수행한다.

- **`technology(owner)`는 기존 `RawPlayerWorld::Create(owner)`에 연결해야 한다.** 원본00490250은 새Player 부모 생성이다. 같은 클래스의 `Technology(owner)`/`CurrentTechnology()` getter나 기존 AddTechnology/Ensure로 대체하지 않는다. [RawPlayerWorld.h:37](../../cpppj/src/o/RawPlayerWorld.h)의 Create→Factory/Owner/Pop 실제 경계를 재사용할 수 있다.
- players는 실제9×180BYTE 표, handle은 실제 Client 쓰기 버퍼다. getter는 현재 저장 주소의 순수 조회이며 span을 동기 번역/서식/타입/Create callback 중 무효화하지 않는다. 단지 다른 표가 현재가 되는 것은 허용하고 기존 표도 유효하게 유지한다.
- translate/format은 **NUL 포함 원시바이트 span**을 반환한다. UTF8 변환·앞뒤 공백 제거·이름 기본값을 이 모듈에 넣지 않는다. 원본처럼 한 BYTE 복사 뒤 NUL을 판정하므로 물리범위 거부 전 이미 쓴 바이트는 유지한다.
- typeAt은 현재 `RiftTypeRecord&`의 group/theme를 읽는다. 반환된 기록을 별도 오래된 타입 사본으로 치환하지 않는다. createContained는 고정 SID를 현재 실제 Pool과 결합해 직접 생성하며 반환은 사용하지 않는다. 부모 SID0을 이 본문에서 추가 거부하지 않는다.
- finish0046e980은 Pool 순회 등을 갖는 별도 원본 후처리다. 이번 모듈은 **정확한 마지막 호출**만 보존한다. 이 하위를 구현하지 않은 상태에서 generic Editor 전체 성공을 보고하지 않는다.

도달하지 않은 createContained는 요구하지 않는다. 반대로 선택된 누락 hook은 앞선 필드/생성 효과를 보존한 채 명시 실패한다. 예를 들어 format이 없으면 owner2 Player 생성/+2c 쓰기/Computer 번역 뒤 실패하며 이전 효과를 되돌리지 않는다.

## 최초 독립 x86 fixture

[생성기](../../tools/decomp_editor_players.py)는 보호 PE를 격리 Unicorn 메모리에 복사해273BYTE만 실행한다. 번역004de9a0,Player00490250,서식004b9390,TYPE0049a840,ContainedCreate004afa70,후처리0046e980은 **명시 외부 경계**다. 실제 하위 Factory/Contained/서식/후처리 몸체나 파일/OS/게임은 실행하지 않았다. 마지막 후처리의 원본 tail 인계와 cdecl 반환 ESP+4를 확인했다.

**333고유입력×x87제어어27f/37f=666관측**, 정상626/명시중단40, 실제 **761,158명령**, 두 정밀도 차이0이다. 중단은 Human6/Player6/technology3 6/format2 6/record1 6/create1 4/finish6이며 지정 callback 진입까지의 모든 앞선 효과를 보존한다.

관측 입력은 signedcount−1/0/1/7/16,타입0/혼합 필터/모든group10,영어/빈문자열/원시CP1252 번역,flags0/ffffffff/상위비트 보존,owner2 부모SID0,다음 외부효과를 포함한다.

- Human 번역/기술 부모 생성 뒤 현재Player표 전환,Player 번역 뒤 현재handle 전환.
- Computer 번역/서식 중 현재Player표 전환과 **캡처 레코드flags 변경**.
- 기술생성 전·TYPE·Create callback 중 count0/3/12 변경,TYPE getter가 반환할 기록의 eligible 필드 변경.
- Create 중 현재Pool 주소 전환과 Player+9c 기술부모기록 변경. 원본은 **처음 반환된 부모 SID**를 계속 쓴다.
- 선택되지 않은 외부효과와 각 단계의 명시 중단.

각 관측은 두Player표 **9×180BYTE 전체**와 두handle64BYTE 전체를 exact hex로,현재 count/표/handle/Pool·호출수와 모든 번역·서식·TYPE·현재count·Create 인자를 trace로 보관한다. 기대 기술목록을 Python으로 계산해 원문을 대신하지 않는다.

[fixture](../../cpppj/tests/fixtures/editor-players-x86.tsv) SHA256은 **`7da2dcf1c11fcc0fcdfe5558f0a39d8e2ff015c08cb4c3dff2b1a3a42365baf7`**다. 이 fixture와[보고서](../../cpppj/editor-players-native.json)를 **제품 소스 작성 전에 생성·동결**했다. 도구는 기존 두 산출물 덮어쓰기를 거부하며 `--verify`는 보호입력/정밀원문/helper/도구/fixture SHA만 읽기 확인한다. 제품을 맞추려고 최초 관측이나 helper를 수정하지 않았다.

## 격리 검사 결과와 다음 연결

[EditorPlayersTests.cpp](../../cpppj/tests/EditorPlayersTests.cpp)의 `IndependentNative666`은 입력열만으로 신규 제품을 실행한 뒤666관측·명시중단40의 모든 현재값/raw/trace를 대조한다. `DirectRepeatedAndSelectedBoundaries`는 두 번 호출의 직접Create14회,모든타입제외 때 create hook 불요,누락 format의 선행+2c 보존,이름 물리범위 거부 전 부분 BYTE복사를 별도 확인한다. 마지막 물리범위 검사는 C++ 안전경계이며 native callback 중단 관측으로 합산하지 않는다.

`extracted/editor-players-20261024-isolated`에 신규모듈/검사/작은 runner만 연결했다. MSVC14.51 `/std:c++20 /EHsc /utf-8 /W4` 컴파일 **종료0/경고0**, 창 없는 독립 실행 **2검사/실패check0/종료0**이다. 공유제품/CMake/검사파일·공유빌드·GUI는 변경/실행하지 않았다. CMake 등록용 macro는 **`NETSTORM_EDITOR_PLAYERS_FIXTURE`**다.

실제 일반 Editor 연결은 State1/2의 생성 전 모드·같은 Player표/Client handle/Pool·실제 Create와 마지막0046e980을 공급해야 한다. 이 모듈만으로 기존/새Fort 설치·전체snapshot·작업파일·Test Battle/Return·사용자 편집 완성을 주장하지 않는다. 후속은 [Client 설계](cpp-editor-client-next-20261023.md)와 [전체snapshot 설계](cpp-editor-snapshot-next-20261023.md)를 따른다. 원본/CD·AGENTS와 다른 프로젝트는 변경하지 않았으며 git 작업은 root가 맡는다.
