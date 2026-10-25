# Native State 정리의 실제 Client 전역 연결 (단위448)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 디컴파일 날짜는 **2026-10-23**이다. 기준 HEAD는 `ef83ab1d`다. 기존 정밀 C와 보호 파일을 읽기 전용으로 사용했다. 디컴파일 산출물·원본 게임·기존 fixture는 바꾸지 않았다.

## 연결한 범위

`CleanupStateWorld`의 순서·직접 쓰기 본체는 이미 복원되어 있었지만, 실제 Client에서 사용할 전역 접근 묶음은 없었다. 새 `BindClientStateWorldGlobals`는 현재 Client·같은 표면·Template9·공통 Quick Help·ClientAudio를 빌린다. 별도 주소→값 저장소나 Frame 준비 단계의 복사를 만들지 않는다.

| 접근 | 실제 저장소와 경계 |
| --- | --- |
| DWORD 읽기 69대상 | 직접 쓰기 대상과 readonly 6필드, 연결 프레임22칸을 포함한다. 현재 Client·GameAux·표면 모드·Chunk·팝업·표시 값을 조회한다. |
| DWORD 쓰기 63대상 | 원문이 직접 쓰는 일반41필드와 프레임22칸이다. signed/float의 32비트와 이웃 저장소를 보존한다. |
| WORD 쓰기 1대상 | FortCoreSeed `005c89b4`의 하위16비트만 바꾸고 상위16비트를 유지한다. |
| 64비트 쓰기 9대상 | 실제 음악 종료와 네 GameAux·네 State 기한에 직접 쓴다. 음악은 기존 `WriteSongEndBits`를 재사용한다. |
| Player·TYPE·Template | 각 store 직전에 현재 Player span을 조회하고, 실제 mutable TYPE.cost 참조와 현재20BYTE 헤더 첫 DWORD를 사용한다. |

실제 bool 소비자 여덟 필드는 정상0/1 모델을 유지한다. 다른 DWORD 값은 쓰기 전에 거부한다. 원문 cleanup이 이 필드에 쓰는 값은0이다. 이 연결을 임의 Native DWORD 전체를 보존하는 bool 모델로 설명하지 않는다. 미등록 주소·정렬 불일치·다른 폭·다른 Client의 표면 또는 음향 owner도 거부한다.

사제 HP 1/4 설정 `005ca8f0`는 GameAux의 실제 장기 저장소로 옮겼다. 같은 값을 표면 공장의 생성자, 보상/최대HP, 사제 HP setter, 연소점 선택이 직접 읽는다. Session의 Frame→연소 옵션 복사는 제거했다. 독립 fixture의 값형 fallback은 유지하며 장기 주소 setter는 다른 주소 재연결과 임시 bool을 거부한다. 일반 fresh 월드의 기존0 초기화는 호환하고 prepared 대여·반납으로 설정을 초기화하지 않는다.

건물 숨김 `0059a8ac`도 ClientGameplayGlobals가 소유한다. 기존 Uber 메뉴와 그리기는 같은 필드의 참조를 빌리고, Native cleanup의0 쓰기와 현재값을 공유한다. 별도 메뉴 구현은 만들지 않았다.

음악 종료 필드 검사도 실제 ClientAudio를 사용한다. 이를 위해 기존 Run의 init sound 본문2,231BYTE를 `InitializeNativeAudio`로 옮기고 원래 위치에서 호출한다. 본문 바이트는 그대로 보존했다. 기존 noAudio 경로를 창·장치 없이 초기화할 수 있으며, 이미 준비한 owner의 중복 초기화는 대여 주소를 바꾸기 전에 거부한다.

## 원본 근거와 검증

정밀 원문 PATCH `004b2df0`, CD 비교 `0042f5c0`, PATCH 최대HP `0049fce0`·HP setter `004942b0`의 발췌와 원시 SHA를 `extracted/cpp-state-client-globals-448-static/`에 보관했다. CD 부모는 비교 자료이며 이 연결의 제품 대상은 PATCH 10.78이다.

신규11사례는 실제 필드의 독립 주소 대조, 현재값 재조회, signed/float 비트, 프레임22칸·이웃 표, seed 상위WORD, double의 -0/NaN payload, 현재 Player 재할당, TYPE/Template, 다른 owner 거부, 원문 부모→실제 전역, 실제 Session 사제 생성/회복을 검사한다. 기존 연소 선택 Native 양판본 전체 입력을 공유 현재값으로도 재생하며 fallback은 반대로 둔다. 기존230개 TEST_CASE 본문 바이트와 관련 fixture4개를 유지했다.

첫 Source1 빌드는 제품 라이브러리 성공 뒤 음향 초기화 재사용을 먼저 반영하기 위해 명시 중단했다. 그때의 MSBuild 강제 종료 진단6개와 원시 로그를 보존했다. 중단 전 소스 오류·경고는0이었다.

Source2 빌드는 경고·오류0으로 완료했지만, 첫 집중에서 새7장면이 같은 준비 단계에서 실패했다. 연결 코드가 초기화 전 `Client::SurfaceStorage()`를 NULL 조회라고 잘못 가정한 것이 원인이다. 기존 getter는 초기화되지 않으면 예외를 내므로 실제 Native store 전에 모두 중단됐다. 기존 Native 기대값과 검사 본문은 바꾸지 않았다. 기존 예외 계약을 유지하고 `SurfaceStorageIfInitialized()`의 순수 현재값 조회를 추가해 이 가정을 바로잡았다. Client가 실제 표면을 초기화한 뒤에는 그 동일성 검사도 유지한다. 그 첫 집중의 나머지482고유 사례는 통과했으며, 실패 원시 결과와 진단도 보존했다.

Source3의 **2,030소스 동결**, build3 종료0·경고/오류0, 집중 **489고유/491관측·실패0**(신규11개 전부)을 확인했다. 기존230사례를 실제로 다시 실행했다. 같은 최종 게임의 메뉴 왕복·실제 Early1 화면 GUI2경로도 통과했다. 보호1,821파일과 GUI 전후 원본/CD 전체1819파일 집합·허용 설정은 그대로다. 이번 단위에서는 전체 CTest·자연 Early2 반복·원본 PE 재실행을 하지 않았다.

게임 SHA는 `bc9ce17cd3c23eb738c8ac514704aaf1fc9fb87656a0433c2c00366aabb79c31`다. [실행 보고서](../../cpppj/state-client-globals-20261026.json), 원시 `extracted/cpp-state-client-globals-448-*`에 빌드·집중·실패 진단·보존 감사·GUI 관측을 보존했다. GUI는 원본과의 픽셀 동일성 검증이 아니다.

## 다음 연결

이 단위의 원문 부모 검사는 Effect/SP/AI/Core/Pool 하위를 명시한 호출 관측 경계로 둔다. 따라서 실제34개 Effect 전체, Native State1 cleanup/prefix와 Session의 GameplayUse·고정뷰 반납 coordinator, Edit checkpoint·초기 HUD 설치는 완료하지 않았다. 이들을 같은 실제 owner에 연결해야 자연 메뉴의 Edit 예약을 소비할 수 있다. 작은 미소유 leaf와 전체 Deck 명령 부모·SID16 잔여도 다음 작업이다.

직전447의 전체 CTest·GUI·자연 Early2 결과는 그때 제품의 증거로 유지한다. 이번 제품의 집중 검사와 구별한다. 전체 Early·캠페인·Priest Training·데모 자연 완주 및 표시·음향 동일성 목표는 계속 진행 중이다.
