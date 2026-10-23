# 같은 Subscriber·Core·색상 기록의 실제 세션 접합

2026-10-24 단위388 기록이다. 사용자의 새 `/goal` 요청으로 작업을 재개했으며, 기준은 **88834c49 / 20261024-10 CP**다. 현재 호스트와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 같다. AGENTS.md와 공통/닷넷 인수인계를 다시 읽었고 시작 작업본은 깨끗했다. 기존 일시중지 문서와 원시 자료는 당시 이력으로 유지한다.

이번 게임/공용 회귀는 원본10.78 자산 기준이다. CD10.72의 추가 실행이나 새 디컴파일은 없으며, 기존 독립 문서의 CD SourceMap 대응 기록을 유지한다.

## 변경과 원문 근거

보관한 [네 제안과 manifest](../../cpppj/handoff/paused-20261024/README.md)를 현재 작업본에 대조했다. 대상 교정과 Session CoreRefresh의 check/apply는 모두0이다. Subscriber 최초 check는387의 커서 서비스와 겹치는 세 파일에서1로 실패했다. `CampaignClientServices.cpp`, `RawCampaignSession.h/.cpp`만 현재 문맥에 수동 합성하고, 나머지 파일은 원시 patch를 그대로 적용했다. ColorSort primary check/apply도0이다. manifest의11파일 SHA는 모두 보관값과 같으며 최초 patch나 Native 출력을 수정하지 않았다.

Core 갱신은 현재 Login의 `currentCore/cachedCore/coreActive`와 실제 Player 기록을 기존 `ReadPlayerCoreData`에 제공한다. 기술은 같은 `RawContainedFinder`와 TYPE의 `techBitIndex`를 사용한다. 정상 TYPE165/VT507950의 가상+80은 기존 `RawTempleRitual::Count`다. Player152의 상위 상태 level을 대신 읽지 않는다. Login이 없는 경우 첫 Core 쓰기 전에 명시 실패하며, 기술 기록 뒤 선택된 미복원 VT의 실패는 앞선 기술 비트를 보존한다. Editor CoreWrite의 원문 editor/post 자연 반환도 유지한다. Client에 Player Core alias를 새로 장기 연결한 작업은 아니다.

`ClientSubscriberGlobals`는 PE 초기값 My=ffffffff/Login=1인 두 DWORD를 한 번 소유한다. Client의 getter, UberGump Init, Session의 `CampaignWorldGlobals`가 같은 주소를 빌린다. Chat이 읽는 getter도 실제 const 참조다. 명시 Borrow는 기존 값을 다시 초기화하지 않고, 값 복사/이동은 독립 저장소를 만들며 대입은 목적지 binding을 유지한다. Editor/post 모드는 Session별이다. 실제 bound Login Begin은 두 값을 다시 복사하지 않는다. 독립 검사 경로는 기존 fallback을 유지한다. 비-guided Init의 기존 My=1 쓰기 시점이 원문 전체와 같은지는 여전히 미확정이다.

Client 시작의 색상 캡처는 같은 `RawColorSortPrimary` 0x3800BYTE를 만든다. 기존 실제 malloc 초기 바이트 장치를 공유하며 레코드의 미작성 padding을 유지한다. 건설 캡처의 전용 TYPE111/141/137/94/97 색표는 같은 기록 +8의 저장 L을 읽는다. 독립 logical adapter와 기존9표 계산은 유지한다. 이 작업으로 검색0x5008, qsort, `!color.dat`와 전체 재생성 부모까지 복원한 것은 아니다.

## 실제 검증 결과

공용 configure1/build1/build2는 종료0이며 빌드 로그의 경고/오류는0이다. 첫 focused1의 신규18검사 중17개가 통과했고, `ClientSubscriberGlobals_TwoActualSessionsShareOnlySubscribers`는 Editor 테스트 장치에 현재 필수인 Client 장부/변환표가 빠져 실패했다. 해당 검사에 실제 `client.FortBadges()`와 `client.FortTypeConversion()`을 제공했다. 제품의 필수 경계를 약하게 바꾸지 않았다. 최초 실패 로그는 별도로 보존했다.

최종 focused2는 신규18개를 포함한 **고유29검사/실패0**이다. Core Session3, Subscriber9, primary 원문3와 실제 색상 접합3, 기존 Preview2·Core4·Chat5를 중복 필터 없이 집계했다. 기존 Owner Native15행의 전체2304BYTE도 logical 및 저장 L 경로 각각 같은 독립 원문 출력에 대조했다. 원문 primary80행과 Subscriber120 checkpoint의 기존 도구 `--verify`는 모두0이다. 원문 전체 부모 반환과 부분 checkpoint는 구별한다.

색상 접합 검사는 창 없는 두 Screen의 팔레트가 각각 두 번 바뀌어도 고정 RGBX·raw 주소·전체 BYTE가 유지됨을 확인했다. raw L만 명시 변경한 입력은 RGB adapter와 다른 결과를 내므로 소비자가 실제 저장 L을 읽는지 구별한다. 미캡처/legacy adapter/짧은 초기 물리의 실패와 실제 할당 helper 경로도 검사했다.

실제 제품 SHA는 `d96f7e171b0a6414de4237de889ce47c1496489af242050b7a89be36462a766c`, 최종 검사 SHA는 `8fea56a9bdb1d0350aa9aa473b0d86e1b95d73454184fee227a6d6f845b4f8f3`다. build2는 검사 입력만 수정했으며 제품 SHA는 자연 Early1과 같다.

같은 제품의 자연 Early1은 종료0/171.24초, A~G·세 수확/귀환·SP0→200→400→600·다음 Tutorial2/SP10000을 통과했다. 제품타이머1·상태 주입0이며 기존 DoneTutorial1은1→1로 보존했다. 진행도0→1 디스크 영속성 검증으로 확대하지 않는다. 실행 전후 원본1394파일이 같다.

자연 Early2도 종료0/186.14초, A~I·신전1·워크샾1·발사기 최대4→3·회수패킷1/환불75·최종SP3075·다음 Tutorial3/Guide4/SP2000을 통과했다. 기존 DoneTutorial2는1→1로 보존했고 제품타이머1·상태주입0이다. 마지막 게임시각은79.799초며 실행 전후 원본1394파일이 같다. Early1·Early2의 실제 완료 화면도 직접 확인했다. 다음 Tutorial의 초기 브리핑을 그 Tutorial의 완주 증거로 합산하지 않는다.

같은 제품의 일반31항목(Early6·훈련9·캠페인16)은31/31, 실제 Demo 메뉴3항목은3/3 진입을 통과했다. Demo는 원문15초 안내의 자연 timeout과 로컬 AI/플레이 상태까지 확인했다. 모든 실행의 실제 제품 SHA를 보존하며 사용자 완주·전체 전략/승패 동일성으로 확대하지 않는다. 두 실행군의 원본1394파일도 같다.

게임 메뉴1024×768/1280×720은 ESC·목표 검토·나가기 취소·재시작·View/About와 확장 QWASZX/음량/해상도 No 보존을 통과했다. 두 probe/시나리오 모두 같은 제품 SHA이며 원본 설정을 복구했다. 이번에는 설정 복구 OSError22나 옵션 열기 실패를 관측하지 않았으나 과거 간헐 실패 원인의 해결 증거는 아니다. 원문 문자열 인자는 토큰 관측, left85 전체 dock와 모든 View 명령·픽셀 동일성은 기존 회귀의 범위 밖이다.

원시 로그는 `extracted/cpp-resume-388-*`, 새 [결과 보고서](../../cpppj/shared-identity-color-runtime-20261024.json)는 별도 이름으로 보존한다. 모든 게임/설정 복원 종료 뒤 전체 CTest1은 **2652검사/실패0·종료0**이다. CTest 검사338.07초/전체338.13초, wrapper338.22초이며 그동안 게임이나 원본 설정 쓰기는 없었다. 내부 전체 출력도 다음 CTest가 덮지 못하도록 별도 사본에 보존했다.

최종 전체 원본은 **1394/1394·추가0·삭제0·변경0**이다. 제품/검사 SHA는 위 실제 회귀와 같고, 새 보고서에는 소스35개와 실제 증거48개의 SHA를 기록했다. AGENTS.md·원본/CD·닷넷/웹 프로젝트의 추적 변경은0이다. 새 코드/문서의 엄격 UTF-8와 로컬 링크, 원시 patch/proof 보존·stage 집합과 공백을 확인한 뒤 `20261024-11 CP`로 커밋한다. 원격 push는 하지 않는다.

## 남은 경계

[State/World 정리 표](cpp-state-world-cleanup-reuse-20261024.md)의 같은 Client/Player/Core/Pool 수명·34개 정리 자식과 추가 hook, SP 초기화 순서가 다음 접합 대상이다. Player 해제 이전 Core 선택 정리를 갖추기 전에는 Client Core alias를 공유하지 않는다. Edit의 최종 HUD/State/loaded, NewFort·편집 도구·저장/재로드·언어별 시나리오가 계속 남는다. Early/전체 캠페인/Demo의 모든 사용자 완주와 픽셀·음향·규칙 완전 동일성은 미완료이며 목표는 활성 상태다.

후속 정적 확인으로 `extracted/refined/originals/Netstorm.c`의 현재 `FUN_00445140`은00558ed0..0055a18f의100×48BYTE 기록에서 +2c Subscriber가 비0인 행만48BYTE 전체0으로 만든다. 이는 `RawChatChannels`가 이미 명시하는 같은 실제 주소/필드 저장소다. 기존 `Clear(mask)`는 flags 한 BYTE만 바꾸므로 정확 대체가 아니다. 새 전체 행 정리 몸체/Native 대조/State 부모 접합은 아직 구현하지 않았으며 기존 동결 표의 C분류를 소급해 성공으로 바꾸지 않는다. 같은 정밀 원문의 `004328a0`은0054d4e8의255DWORD만0으로 쓰며, 이 표는 `PlayerNoMake` 중복 경고 분기에서 사용된다. 정확 제품 owner는 아직 찾지 못했다. 두 확인은 새 디컴파일/원문 변경 없이 같은 HJOW-Athlon의 기존 전체 정밀 자료를 읽은 결과다.
