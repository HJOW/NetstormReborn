# 선택·권한·FullDeck의 실제 장기 입력 전역 연결

## 범위와 기준

10.78의 선택 전역, 서버/권한 관문, FullDeck DWORD를 실제 플레이 소비자와 같은 저장소에 연결한다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이며 마지막 수행일은 2026-10-23이다. 이번에는 새 디컴파일이나 원본 게임 실행을 하지 않았다. 10.72는 기존 권한 소비자 fixture의 비교 대상으로 유지한다.

436·437·438의 겹치는 변경을 하나의 기능 묶음으로 적용했다. 원시 합집합65파일 중 현재 main과 같은2개는 제외하여 실제 제품 변경은63파일(기존59개, 신규4개)이다. 새 검사는32개이며 이전432·433·434·435의37개와 합친 보존 감사69개를 이번 신규 검사 수로 세지 않는다. 공통5파일의9충돌은 양쪽 독립 추가를 보존하고 실제 저장소 정체성 및 생성 순서를 병합했다. 공통 검사 helper의 `prepareStorage`와 `beforeCtor`는 각각 한 번 실행한다.

## 선택

`ClientUserSelectionGlobals`는 enabled, 강조 모드, 선택 SID, 마우스 TYPE, 원시 마우스 좌표 두 개, 기억 좌표 두 개, signed 다중 선택 개수의 아홉 DWORD와 DWORD 요소 배열을 소유한다. GameWorld와 실제 입력 모듈은 같은 주소를 빌린다. Sid16 참조나 bool 참조로 DWORD 저장소를 위장하지 않는다. 좌표와 개수의 비트 관측에는 `bit_cast`를 사용한다.

다중 선택 처리는 통지 callback 뒤 다음 반복에서 현재 배열과 signed 개수를 다시 읽는다. callback의 배열 재할당이나 개수 변경 후 예전 span을 계속 읽지 않는다. 기존 독립 raw 모듈의 span 경계도 유지한다.

선택 정리는 원본 `004d5c60`의 enabled 관문 → `004d5490` 다중 선택 정리 → selected=0 → `004d5bb0` 강조0 순서를 사용한다. 강조 모드가 이미0이면 mouseType을 임의로 지우지 않는다. 첫 메뉴의 빈 목록/음수 개수 경로는 Session 없이도 자연 반환하고, 실제 객체 통지에 도달할 때만 현재 Session과 같은 Pool을 요구한다. `Select(0)` 호출로 대신하지 않는다.

State7의 `004978d0`은 선택 정리가 아니라 렌더러 분모 선택이다. 해당 주소에 이 선택 정리를 잘못 연결하지 않는다. 일부 기존 deletion/이미지 소비자의 Sid16 경계는 후속이며 모든 기존 leaf의 임의 큰 DWORD SID 처리를 복원한 것은 아니다.

## 권한

`RawAuthorityState`의 server/boss 두 bool을 Gameplay와 RawSurfaceStorage가 공유하고 Storage는 Pool보다 오래 소유한다. 두 PE의 최초 DWORD는 각각1이므로 최초 논리값은 true/true다. bool은 정상0/1 소비 경계이며 Native DWORD의 임의 비영 상위 비트까지 보관하는 구현은 아니다.

Pool, Process, GunProcess, AssetPostPop, AssetRegular와 실제 Session의 기존 참조는 같은 현재 권한을 읽는다. 효과 callback 뒤 권한이 바뀌는 관문도 현재 getter로 다시 조회한다. Gun에서 Process로 매 프레임 복사하던 쓰기, prepared 생성자의 무조건 true 쓰기를 제거했다. Broadcast의 SendExcept 관문에는 boss가 아니라 server를 전달한다.

일반 생성의 기존 fresh true/true 정책과 prepared 획득·반납의 현재값 보존을 구분한다. Storage와 Gameplay가 서로 다른 권한 owner를 명시한 경우 Acquire나 값 쓰기 전에 거부한다. RawPriestWorldBindings의 hitpoints/fall/carrier/contained 네 leaf와 일부 Prepare 값형 snapshot은 후속이다.

## FullDeck

`ClientGameplayGlobals::FullDeck()`은 최초0인 실제 DWORD `0054db14`를 소유한다. Session은 참조를 빌리고 Production/Sync/Deck과 FortObjectLoad, ConstructionConfirm, PlayerPlacementAnchor, 실제 HUD는 같은 현재값을 읽는다. 효과 뒤 변경될 수 있는 관문은 trailing 현재 DWORD 포인터/getter를 사용한다. HUD에서 임시 지역값1을 보여 주던 경로도 실제 DWORD 대여로 바꿨다.

일반 fresh0과 기존 ordinary Editor의1 호환 정책은 유지한다. prepared 생성자의 무조건1 쓰기는 없애고 반납·생성 실패 뒤에도 `80000000`과 `ffffffff`를 포함한 현재 비트를 보존한다. nullable `+ac` 하위는 현재 자연 null 경로이며 전체 FullDeck 명령 부모 `0044f9d0`와 UserInput writer는 후속이다.

## 검증과 남은 범위

Source1의63파일 원시 SHA를 `extracted/cpp-native-input-globals-436438-source1/hashes.json`에 고정했다. 첫 MTT 빌드는 종료0, 컴파일 경고/오류0이며 실제2,119.089초가 걸렸다. 집중1은340고유/349관측 중 신규 권한 검사 하나만 `Only one client may exist`로 실패했다. 실제 Client 둘을 동시에 만들던 검사 fixture가 프로그램의 singleton 제약을 위반한 것이며 실패 원시 로그를 보존했다.

첫 Client의 owner를 공유 포인터로 남기고 두 Client/Session을 순차 생성하도록 해당 검사 한 파일만 교정했다. 기존 검사명·숫자·일곱 조건을 유지하고 보존 owner의 격리를 확인하는 두 조건을 추가했다. 제품 코드는 바꾸지 않았다. Source2의63파일을 별도로 동결했고 증분 build2는29.868초, 종료0·컴파일 경고/오류0이다. **집중2의340고유/349관측, 신규32개 전부가 실패0**으로 통과했다. 보호1821파일·동결63소스·실제 네 실행 파일 SHA도 같다.

[실행 보고서](../../cpppj/native-input-globals-runtime-20261025.json)에 실패 이력과 최종 제품을 구분했다. 게임 SHA는 `a49f474f97cd24876045d7b77c584a47742f7880053b9eccd92360b3e73e535f`다. **같은 제품의 통합batch1도 종료0, 전체 CTest2,963개·실패0**으로 완료했다. 메뉴 GUI 두 실행과 자연 Early2→Tutorial3, 보호1821파일·소스·네 실행 파일·실행 도구/입력·보고서 원시 사본의 SHA 및 원본 전체 집합/두 설정 복원을 확인했다. 실제 세 캡처도 육안 확인했으며 평상시 Host Game 버튼은 없었다.

Early2는 실제 타이머1, 상태 주입false, 초기 진행 주입false로 환불75·종료SP3075·다음Tutorial3 초기SP2000·같은 프레임 다음 안내를 확인했다. 기존 완료 진행값1도1로 유지했다. 다음 안내 캡처의 SP1700은 설치 뒤 실제 생산이 진행된 시점이며 초기SP2000 관측과 구분한다. 첫 메뉴 플레이 캡처에는 Bridges 안내가 열려 있다.

CTest의 stdout과 stderr를 합친 로그에는 기존 Chat 진단과 CampaignMissionStartTests의 진행 진단이 두 검사명 중간에 끼어 있었다. 원시 로그는 수정하지 않고 실제 두 소스에서 확인한 정확한 진단 접두사만 메모리에서 분리했다. 현재/이전 각각44진단의 분리 수와 원시/정규화 SHA를 기록했고, 복원한 이름 집합이 현재2963개 소스/이전HEAD2931개 소스의 전체 등록 이름과 각각 완전히 일치함을 확인했다. **기존2931개 제거0, 실제 추가32개가 이번 신규32개와 정확히 같았다.** 이 보고서 집계 교정 때문에 제품이나 검사 본문을 다시 바꾸거나 전체 검사를 반복하지 않았다.

Native State1 전체 정리/초기화 부모, 실제 Edit 편집·저장, 전체 Early Missions/캠페인/데모 자연 완주, 진행 저장·재실행과 원본 완전 동일성은 계속 미완료다. 이번 저장소 연결이나 짧은 진입 검사를 전체 목표 완료로 세지 않는다. 사용자 요청으로 제거한 평상시 왼쪽 위 Host Game 버튼은 재도입하지 않으며 원본 ESC 메뉴 내부의 Game 열은 유지한다.
