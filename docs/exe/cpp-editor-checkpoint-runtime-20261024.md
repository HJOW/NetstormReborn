# 실제 작업본 파일과 기존 Editor 설치 checkpoint 통합

2026-10-24 단위385 통합·검증 기록이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 직전 단위384는 `29bee739 / 20261024-06 CP`로 커밋했으며, 전체2532검사·같은 제품34진입·자연Early1과 originals1394동일을 확인했다. 전체 Editor나 사용자 전체 완주가 완료된 상태는 아니다.

## 실제 파일과 Session 순서

실제 파일장치 제안 `ba7a9cda55e66e341ae5715c646b46c9518556b1c433d889f0532a69c6d6bc48`을 check/apply하고, 이어 checkpoint 제안 `76c1bb69e7b1580c3bd9e2a68510a11ee9b7fd46393e7ca1f004bccae3677ac5`을 check/apply했다. 네 단계 모두 종료0이며 원시 제안은 그대로 보존한다.

`EditorFortFileDevices`는 현재 Fort/같은 FileSpec·실제 Config.PathSpec을 보호된 작업 namespace로 해석한다. 준비는 명시적인 원본 복사 단계이고, 기존 작업본이 있으면 원본을 다시 읽거나 덮어쓰지 않는다. 파일 출처는 실제 fopen rb의5/0, 읽기 전용은 실제 CRT stat의 쓰기 비트다. Config의 readOnlyForts와 합치지 않는다. 원본/CD 보호와 원문의 유효성 첫DWORD 판정도 유지한다.

`deferExistingEditorFile`의 기본값은 false다. ExistingEditor에서만 명시 true를 허용하고, ctor의 실제 모듈/Runtime 수명은 먼저 구성하되 manager는 File.load에서 같은 Storage.LoadFile이 공개한다. 기존 Campaign/LocalLogin·단계별 Editor의 즉시 설치 계약은 보존한다. 보조 FortTemplate과 실제 읽은 작업 파일 바이트는 같아야 하며 새 파서나 섹션 복사를 추가하지 않는다.

checkpoint는 첫 이름변경/Notify보다 먼저 실제 저장 장치를 연결한다. 원문 File/Subscriber/Config/두 Title→World/Deck→실제 사제 부모·카메라·생산→CoreWrite/전체 Snapshot→State/Notify/Startup→EditorPlayers/맵 tail 순서다. 실제 파일과 Slot/State의 부분효과는 실패 때 복구하지 않는다. 최종 HUD/State/loaded는 이 API의 완료점이 아니다.

## 원문 부모와 난수 수명

Startup390, Chat644, PlayerCoreData804, Kencloud v2의246 관측을 제품 작성 전에 동결한 독립 모듈과 함께 등록한다. Kencloud v1의 같은246행은 계측기 rep stosd 종료 처리 문제를 보존한 이전 자료이며, v2 통과 수에 더하지 않는다. Chat/Core의 최초 보고서는 원본 바이트 그대로 `cpppj/chat-manager-prefix-native.json`과 `cpppj/player-core-data-native.json`에 복사했다. `tools/verify_native_manifest.py`는 보고서의 원래 입력 경로나 SHA를 고치지 않고 모두 검증하며 게임을 실행하지 않는다.

공유 GameRandom 제안 `d181375733140d0c34235ad3b794cf49f21dfc404f416bfaa7e4ba47919510e7`도 check/apply 종료0이다. Client의 같은 실제 GameRandom 초기55/모드0을 Session의 기존 SP·지형·객체·AI·저장이 참조하며, 독립 Session의 local0 fallback은 유지한다. CRT와는 별개다. 이 단계에서는 원문 공통 시작의 SP 초기화/추적DWORD 수명을 옮기거나 난수 호출을 추가하지 않았다. 전체 시작 난수열이나 실제 구름 표시가 완성됐다는 판정은 아니다.

## 통합 중 발견한 설정 소유자 누락

최초 공용 빌드1은 파일장치 검사 전용 경로 매크로 누락으로 실패했다. 전용 두 매크로만 보완한 빌드2는 종료0·관측 경고오류0이다. 신규 집중 검사에서 checkpoint 한 곳이 실패했고, 진단용 빌드3으로 `stopped=1 chat=1 title=2 config=1 subscriber=7 configured=<missing> handle=Player`를 확인했다. 원문 `Config::Set`과 최초 fixture는 바꾸지 않았다.

이때 이름 없는 두 객체만 있는 Registry에서는 `fort.mySubHandle`이 루트 `mySubHandle`로 저장되는 것이 기존 몸체의 결과였다. 이를 확인하려고 검사 기대를 일시적으로 무접두어 조회로 바꾼 빌드4는 종료0이며, 그 단계에서는 추가 검사나 전체 CTest를 실행하지 않았다. 이후 실제 CRT 표와 생성자를 조사해 원본의 장기 `global`·`fort` 객체 누락을 찾았다. 단순히 불완전한 Registry에 검사를 맞추는 것으로 끝내지 않고 [실제 네 객체의 순서](cpp-config-static-owners-20261024.md)를 복구한다.

최소 제안 `f2dd0aeed9cbf3d4b31d9ef943fe853e66a150b4c829865781ac8c9201ec6b34`을 check/apply해 종료0이다. 언어→무명 설정→global→fort를 같은 Registry에 등록하고, checkpoint 기대도 원래의 `fort.mySubHandle` 조회로 되돌렸다. ConfigInterface 신규3검사를 더해 신규44/예상전체2576이며 공용 빌드5·실행을 진행한다. 원시 실패·진단·기대 교정 전 사본은 모두 보존한다.

빌드5는 제품/검사 모두 종료0·관측 컴파일 경고오류0이다. 정확한13개 필터로 신규44개를 중복 없이 실행해 모두 실패0·종료0이다. 실제 파일장치9/Editor checkpoint8/Config 소유자3도 이 실행에 포함한다. 첫 전체 CTest는2576검사/실패check1·421.00초(전체421.05초)·종료8이었다. 제품SHA는 `9694a2617115cb58f21c222c1739fa9d9833fb00e662b6a4eaaeba04070db0cc`, 당시 검사SHA는 `8f210de8da5b5bd2988a1576c2124c8a64f43f9171be99087eb7e4908222da54`이다.

전체 실행의 한 실패는 기존 `RawMenuControls_ObjectMenuCommandsAndCurrentConfig`가 `global.salvageSid` 설정 뒤 루트 `salvageSid=50`을 기대하던 것이었다. 실제 메뉴 원문은 qualified 키를 전달하며 기존 Native 메뉴 관측도 이를 보존한다. 새 장기 global이 실제 대상이므로 검사에 실제 실행처럼 비어 있지 않은 루트 입력을 두고 `global.salvageSid=50`·global 로컬 값·루트 키 부재를 함께 확인하도록 교정했다. [초기 메뉴 문서](cpp-object-menu-20261017.md)의 잘못된 해석도 날짜를 붙여 교정하고 당시 실패는 보존한다. 제품 메뉴/Config 몸체·최초 Native fixture/report는 바꾸지 않았다. 빌드6 및 해당 검사·최종 전체 재검증을 진행한다.

빌드6 종료0·관측 경고오류0 및 해당 메뉴1검사/실패0이다. 제품SHA는 빌드5와 같고 새 검사SHA는 `66640f53b7fd0aca72611df0c563299e68099fabc5a2b5bbbb5e0b43eb4461e7`이다. 옛 Object 메뉴 도구의 `--verify`는 당시 제품 핀의 검사 파일 변경을 발견해 종료1이다. 최초 보고서를 새 SHA로 덮어쓰지 않았다. 별도 감사에서 원래 Native 입력6개와 fixture1434행/고유입력1434는 모두 그대로이고, 과거 제품 핀은 해당 검사와 CMake2파일에서만 달랐다. `verify_native_manifest.py`로 원문 입력만 구분해 검증한 최종10보고서는 종료0이다. 이 검증을 옛 제품 핀 전체 일치라고 표현하지 않는다.

같은 제품의 자연 Early1은 실제 A~G·다리/사제 이동·세 차례 수확과 귀환·SP0→200→400→600·다음 Tutorial2/Guide4/SP10000을 통과했다. 제품 타이머1·상태 주입0이며 기존 완료값1→1을 유지했다. `lastGameTime=111.476`이고 진행0→1이나 영구 저장 검증으로 세지 않는다. 실행 종료0·원본1394동일이며 원시 `extracted/cpp-editor-checkpoint-20261024-natural-early1/`에 입력·실행·상태·결과를 보존한다. 게임 실행은 공용 검사 소스 컴파일과 독립적으로, 제품 링크 완료 뒤 진행했다. CTest는 해당 실행 및 설정 복구가 끝난 다음 시작했다.

## 실제 전체 진입 재검증

같은 제품SHA로 Early6·Priest Training9·캠페인16, 일반31개가 전부 실제 안내 버튼 뒤 플레이 장면에 진입했다. 실행31/통과31·원본1394동일이다. 실제 Demo 메뉴3개를 이어 실행한 최초 결과는 StormRages와 DissolvedAlliance 통과, GuardMyBack 검사 시작 실패다. 실패 프로세스는0.483초·종료1·상태0BYTE·`Unable to open d/options.cfg`이며 Demo 메뉴 입력 전에 시작 설정 저장에서 실패했다. 이 결과를 해당 미션 자체의 논리 실패나 전체34통과로 바꾸지 않는다.

최초3 Demo 실행의 설정 복구 뒤 `options.cfg`는954BYTE·Archive·읽기 전용 아님을 확인했고 최종 원본 manifest도 복구됐다. 이 기록 시점에는 일시 파일 접근 실패의 원인이 미확정이다. 첫 실패는 원시 그대로 보존하고, 정적 파일 장치 조사와 같은 제품의 새 GuardMyBack 검증을 진행한다. CTest2는 세 Demo의 종료/설정 복구 뒤 시작했다. CTest 중에는 별도 게임이나 설정 쓰기를 실행하지 않는다.

최종 전체 CTest2는 **2576검사/실패check0·종료0·405.89초(전체405.96초)**다. 결합 실행 wrapper의 종료1은 앞선 Demo 실패에 해당하며 `demoExit=1/ctestExit=0` 원시를 따로 보존한다. 이어 같은 제품의 GuardMyBack을 서로 다른 새 폴더에서 두 차례 재검증한다. 원본/UCRT의 기본fopen 공유 정책은 READ|WRITE로 같고 과거19/23일에도 같은 문구·Python 설정 복구 OSError22가 있었다. 이 사실만으로 특정 파일시스템이나 제품 결함을 원인으로 확정하지 않는다.

GuardMyBack의 새 재실행1·2는 둘 다 실제 Demo 메뉴·자연15초 안내 종료·AI/월드/최종플레이 진입에 성공했다. 두 프로세스와 wrapper 종료0이며 각 원본 manifest도 동일하다. 한 실행의 설정 복구에서 `OSError(22, Invalid argument)`가3회 출력됐으나 기존 제한 재시도 안에 복구됐다. 최종 시작1394/종료1394·변경0·추가0을 확인했다. 따라서 같은 제품의 **고유34진입 성공**은 확인했지만, 총36진입 시도 중 최초 시작1실패/성공35와 복구 재시도는 그대로 남긴다. 사용자 완주·전체 동일성·파일 접근 실패 원인 해결을 뜻하지 않는다.

자연 Early1의 실행시간은177.2815초이며 제품SHA는 모든 일반/Demo/재실행에서 한 값으로 동일하다. 선택89파일만 stage했고 원문 fixture/tool/report27개는 작업본과 인덱스의 원시 BYTE SHA가 모두 같았다. staged 공백 검사는 종료0이다. 후속386의 신규 CoreRefresh/SP/색상 캐시·고정입력/Dot/정리/저장 reader 및 소유자 사본 제안은 이번 공용 등록·커밋에서 제외한다.

## 검증 상태와 남은 경계

파일장치9, checkpoint 기존5/새CoreWrite접합3, Startup2, Chat3, Core3, Kencloud2, GameRandom owner2, CoreWrite5, Client Chat/Core owner5, ColorSortStartup2와 ConfigInterface3으로 신규44검사/예상전체2576을 등록했다. 최초 집중 실행에서 `Kencloud` 필터가 다른 파일의 관련 검사도 포함해3개를 선택한 것은 제품 실패가 아니며, 후속은 `Kencloud_`로 정확히2개를 선택한다. 격리 검사의 통과나 Session 구문 검사로 공용 통합 성공을 대신하지 않는다. 원시 자료와 로그는 `extracted/cpp-editor-checkpoint-20261024-*`와 각 제안의 독립 폴더에 보존한다.

CoreWrite396관측/5검사 부모와 ColorSortStartup116관측/2검사는 별도로 동결한 원문 자료다. Session CoreWrite delta `e4646f2ec0fa6d27e4db7ca35e98d3f13bf3653dd6cb4468c8b93eaeb900dc53`도 check/apply 종료0이다. checkpoint의 CoreWrite는 이제 현재 Storage/Metamap/post/editor/Player와 같은 부모에 자동 연결된다. 일반 Create/쓰기·기존 재시드/난수는 실제 몸체를 사용하되 Core refresh·88BYTE SP writer·실제 seed 소유자가 선택되면 해당 미복원 위치에서 중단한다. 자연 Editor에서는 그 하위가 필요하지 않으며 manager/Metamap 진단도 생략하지 않는다. 최종 loaded는 여전히 확정하지 않는다.

실제 Client Chatmanager/CoreData/Subscriber/Player 수명과 명령 등록, 원문 공통 정리004b2df0, CoreWrite의 선택 직렬화·Core refresh·SP writer, 비영 startupStateCheck의 저장상태 reader, ColorSort 고정초기RGB 및 구름 frame/draw/dirty 합성은 후속 필수 경계다. 공통 정리 부모를004b2c60으로 적었던 별도 조사 오류는 실제 PE 호출자로004b2df0임을 확인해 교정했고, 독립 Core helper804 관측과는 구별한다.

실제 Edit/NewFort/Save as·편집 도구·언어별 커스텀 시나리오·저장/별도 재로드와 최종 HUD/State/loaded·전체 사용자 완주/동일성은 계속 남는다. goal은 active다.
