# 피격·건설·삭제·채집의 실제 Client 권한 연결 (단위446)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 디컴파일 날짜는 **2026-10-23**이다. 이번에는 기존 양 판본 정밀 C를 읽기 전용으로 조사했다. 원본 게임 실행이나 디컴파일 산출물 변경은 하지 않았다. 기준 HEAD는 `ca10924a`다.

## 바뀐 동작

Session의 Prepare가 boss/server 값을 상태에 복사하던 10개 State를 같은 Client 권한 주소에 연결했다. 하위 표시·음향·배치·커서 취소 효과가 역할을 바꾸면, 이후의 원문 분기는 그 현재값을 읽는다. 기존 값형 State와 aggregate 필드 순서는 독립 원본 fixture용으로 유지하고, 끝에 주소와 읽기 함수를 추가했다. Session은 생성 시 주소를 한 번 연결하며 Prepare에서 권한 사본을 갱신하지 않는다.

| State | 실제 원문 소비 위치 |
| --- | --- |
| `PriestDamageState` | HP setter·소유자 통지·표시 갱신 뒤 HP1 사제 의존자 해제 |
| `DamageablePostDestroyState` | 주변 다리 순회에서 금 간 다리의 삭제 |
| `ContentLifetimeState` | 커서 취소 뒤 원격 너겟 수입 알림(10.78만) |
| `ConstructionPlaceState` | 잔액 조회 뒤 비용 부족의 boss 분기, 각 조각 Take의 별도 server 분기 |
| `ConstructionClearState` | 예측 조각의 현재 타입이 다를 때 정리 여부 |
| `ConstructionNoticeState` | 배치 효과 뒤 자동 사제/건설 시작 |
| `LocalGeyserState` | 각 Populate 진입의 boss 관문 |
| `ConstructionProcessState` | 표시 갱신 뒤 완료 시도, 완료 갱신 뒤 abstract 해제 |
| `ConstructionEffectsState` | 섬 정리 송신의 server 관문 |
| `GeyserMinedState` | 채집 소리 뒤 고갈 삭제와 Empty 생성 |

`RawNuggetRegular`, Carrier 및 AI 등의 기존 `const bool&` 생성자와 `assetState.Boss()`·`core.processState.Server()/Boss()`의 참조 반환도 확인했다. 이 경로들은 이미 실제 주소를 빌리므로 추가 사본이나 재연결을 만들지 않았다.

## 원본 근거와 검사 범위

`extracted/cpp-runtime-authority-446-static/`에 원문 **22함수**(10.78 12개·10.72 10개), 두 정밀 C의 SHA와 분기 줄을 보관했다. 원문 boss는 `00540bc4/CD00540a2c`, server는 `00540bc0/CD00540a28`이다. CD `004b0300`은 공통 Content 삭제만 수행하며 패치의 너겟 경제/권한 분기가 없다. 로컬 간헐천 Populate와 비용 부족 직접 처리는 이번 제품의 패치판 경로로 검사했다.

기존 검사 11파일의 **269개 사례 바이트**와 원본 fixture 10개의 내용을 보존했다. fixture의 Git LF와 작업 트리 CRLF는 체크아웃 개행으로 구별해 비교하고, 양쪽 SHA를 감사 JSON에 기록했다. 새 12사례는 효과 도중 역할 변경, 서로 반대인 값형 입력과 공유 주소, 실제 finder·HP setter·Content 삭제·Kernel, 실제 Client/Session의 Prepare 없는 간헐천 호출을 확인한다. 새 사례는 자연 플레이 완주 증거로 합산하지 않는다.

## 검증 결과

Source3의 **2,027소스 동결**, build3 종료0·경고0·오류0, 집중 **480고유/480관측·실패0**을 확인했다. 신규12개가 모두 실행됐고 보호1,821파일도 변경되지 않았다. 게임 SHA는 `9bc865e1ae233ff9ea4d7c59beb39fd5cf4570155da4788c10fad5cffbfe0d32`다. [실행 보고서](../../cpppj/runtime-authority-20261026.json), 원시는 `extracted/cpp-runtime-authority-446-*`에 있다.

전체 CTest·제품 GUI·자연 미션 완주는 이 단위에서 다시 실행하지 않았다. 마지막 전체 CTest3,027개와 메뉴 GUI·자연 Early2 통과는 이전 443·444 게임 `423d0911…`의 기록이며, 이번 제품의 전체 통과로 합산하지 않는다.

최초 집중은 480고유/480관측 중 신규 다리 사례의 두 번째 단언이 양 판본에서 실패했다. 원래 (50,49) 버킷에 있는 SID64를 (49,49)의 SID63 next에도 붙인 신규 입력이 원인이었다. 실제 프레임/삭제 진단과 두 버킷을 대조해 신규 입력만 원래의 독립 두 버킷·next0으로 복원하고, 그 불변 조건을 추가했다. 기존 Native fixture·기대값과 제품 본문은 바꾸지 않았다. 최초 실패/진단 로그도 보존했다. 동결 스크립트가 동일한 파일을 다시 써 재빌드 범위를 키운 문제도 수정해 이후에는 바이트가 달라질 때만 파일을 쓴다.

## 다음 경계

사제 HP 회복 허용은 아직 값형 `recoveryAllowed`만 읽고 실제 Player의 신전 SID에 연결되지 않았다. 원문 `004942b0/CD0040ca40`은 낮은 HP에서 절반 이상으로 전환할 때 현재 Player DWORD(`stride0xb4/+0x7c`, CD `stride0xac/+0x74`)가 0이면 이전 HP로 되돌린다. 현재 제품에는 이 허용 배열을 실제 Player로 채우는 경로가 없다. 별도 단위에서 현재 저장소 조회와 실제 Session 회복을 검증한다. Native State1 cleanup/prefix·반납 coordinator와 Edit 제품 진입, 전체 미션 자연 완주 및 표시·음향 동일성은 여전히 남아 있다.
