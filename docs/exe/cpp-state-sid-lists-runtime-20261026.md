# 단위455: 같은 장기 SID 목록과 등록 표

## 범위와 근거

- 작업 PC: **HJOW-Athlon**. 마지막 전체 정밀 디컴파일은 **2026-10-23 / HJOW-Athlon**이며 이번에는 결과를 다시 생성하거나 수정하지 않았다.
- 원본 10.78의 `00472830`은 `00592938`의 20개 포인터를 순서대로 읽고, 비영 포인터가 가리키는 헤더의 `+0c` 개수만 0으로 쓴다. 배열·용량·포인터·비활성 꼬리·등록 자체는 보존한다.
- `00472880`은 첫 빈 슬롯에 등록한다. 중복·0 인자를 허용하고 표가 가득 차면 쓰지 않는다. `004728a0`은 모든 일치 슬롯을 해제한다.
- 제품 변경 전에 [독립 관측 도구](../../tools/decomp_state_sid_lists.py)로 세 실제 PE 몸체를 격리 CPU에서 실행했다. **66관측**에 빈 표, 전체 표, 구멍, 중복, 끝 슬롯, 앞선 count 쓰기가 뒤 포인터를 바꾸는 입력과 두 x87 환경이 포함된다. 함수나 효과를 치환하지 않았다. 원본 게임·OS·별도 창은 실행하지 않았다.
- 최초 출력은 [TSV](../../cpppj/tests/fixtures/state-sid-lists-x86.tsv), 원문/함수/도구/관측 SHA는 [Native 보고서](../../cpppj/state-sid-lists-native-20261026.json)에 있다. CD 10.72의 같은 몸체를 실행한 증거로 합산하지 않는다.
- 용량/정적 생성 순서는 앞선 [실제 PE 정적 감사](../../cpppj/state-sid-list-static-audit-20261026.json)의 아홉 `push capacity → mov ecx,this → call 0044fb20`를 사용한다. CRT 생성자/할당기 전체를 이번에 실행했다고 주장하지 않는다.

| 헤더 | 실제 목록 | 용량 | 실제 count 소유자 |
|---|---|---:|---|
| 0055a4d0 | factories | 500 | RawSurfaceStorage.sidLists |
| 0055a4ec | dude population | 30 | RawSurfaceStorage.sidLists |
| 0055a708 | outposts | 100 | RawSurfaceStorage.sidLists |
| 00592f80 | providers | 256 | RawSurfaceStorage.sidLists |
| 005950d8 | geysers | 100 | RawSurfaceStorage.sidLists |
| 005954c0 | priests | 16 | RawSurfaceStorage.sidLists |
| 0059a9d4 | residences | 50 | RawSurfaceStorage.sidLists |
| 005caf28 | multiple selection | 30 | ClientGameplayGlobals.selection |
| 005e1898 | temples | 20 | RawSurfaceStorage.sidLists |

## 연결 방식

`RawStateSidLists`는 여덟 실제 `SquidPostPopList`와 현재 등록 표를 보관한다. 다중선택의 signed count는 기존 Client owner에서 직접 쓴다. 등록 표에는 실제 Native 헤더 식별자를 저장하며, 이 숫자를 호스트 포인터로 역참조하거나 C++ vector header를 Native ABI로 위장하지 않는다. 등록/해제/정리는 현재 20칸을 순서대로 처리한다. 미지원 헤더에 도달하면 앞서 완료한 실제 쓰기는 남기고 그 위치에서 실패한다.

공통 Pop/삭제, 사제 접두/검색/삭제, 자산/공장 후처리, 지역 소유권, 섬·피해·수집·AI·생산·에너지·상태 타이머의 소비자는 같은 장기 목록을 빌린다. 기존 독립 raw fixture는 지역 목록을 계속 사용한다. `SquidPostPopState` 전체를 옮기지 않았으며, 로딩/권한/효과/ownerFactories와 Runtime 수명은 따로 유지한다. Dude의 실제 생성/행동/정기는 같은 장기 population을 참조한다.

일반 설치의 fresh 초기화와 Native 정리는 구분한다. `AcquirePrepared`와 반납은 SID/count/데이터/주소에 쓰지 않는다. 일반 설치만 기존 새 월드 정책을 같은 배열에서 수행한다. 실제 State의 `Clear472830` 위치는 같은 Client/표면 owner와 World 대여 반납을 검사한 뒤 등록된 현재 count들만 지운다. 이 작은 효과로 전체 State 부모나 실제 Edit 설치가 완료되지는 않는다.

정적 감사 과정에서 `ReloadTheams` 두 번째 목록의 기존 매핑 오류도 발견했다. 원문 UserInput의 `00542b54` 분기는 첫 `0055a714`, 다음 `005e18a4`를 순회한다. 두 번째는 500칸 factories가 아니라 20칸 temples다. 현재 메뉴 어댑터와 기존 조회 검사의 대상 목록을 원문 헤더에 맞춘다. 기존 검사의 숫자 기대값을 완화하는 대신 대상 물리를 바로잡는다.

## 검증 상태

Source1~4 빌드는 모두 경고/오류0이다. 최종 Source4 build4는31.598초, 집중 검사320고유/324관측·신규12개·실패0이다. 전체 CTest full2는339.392초·실제 C++ **3108검사/실패0**이며 이전3096개 이름과 신규12개가 정확히 일치한다. 같은 게임`e10efe6593f0ab427a094dc55107c584dbce23e291354c01b9987bc2f2226cca`의 복사본 실제 GUI2(Early1 MORE/OK/Home, 캠페인 QWASZX/양방향 회전/새 서버 다리15211/보충)도 통과했다. ready/rotated/placed 화면을 직접 검토했다. 다리 GUI는 `useProductTimers=0`이며 자연 미션 완주나 전체 픽셀/음향 동일성 증거로 합산하지 않는다.

원본/CD 보호1821파일의 SHA를 빌드·집중·전체 전후로 대조했고 모두 같다. [최종 실행 보고서](../../cpppj/state-sid-lists-runtime-20261026.json)의 소스/제품/Native/GUI SHA와 `extracted/cpp-state-sid-lists-455-{source1..4,build1..4,focused1..3,full1..2}`를 함께 본다. 원본 게임은 실행하지 않았다. 새 창을 사용하는 실제 클론 GUI는 AGENTS.md의 **HJOW-Athlon 예외**에 해당한다.

첫 집중의 신규3고유/5관측 실패는 Client 단일 인스턴스 제약·실제 Subscriber·선택된 unitLost 음향 장치의 검사 조립 누락으로 교정했다. 첫 전체의3108검사 중 생산 덱2사례/89단언 실패는 Native242의 입력을 이전 지역 fallback에 쓰던3표현식을 같은 실제 Factories()에 제공하도록 고쳤다. Native TSV·출력 기대·단언을 유지했다. Source2 이후 제품 소스/게임 EXE는 바뀌지 않았고 검사의 입력 조립만 교정했다. 원시 첫 실패는 보존했다.

기존 fixture649개는 Git의 checkout 필터로 HEAD 내용과 전량 대조했다. 기존 Session 숫자 단언을 유지했고, 선택의 일반 fresh 기대1개는 빈 vector에서 원문30칸/전체0으로 교정했다. 최종 전체 회귀에서 모든 이름·기대값을 확인했다. 원시 근거는 `extracted/cpp-state-sid-lists-455-test-preservation.json`에 있다.

## 남은 목표

Native State1 전체 cleanup/prefix, Runtime/Player 반납 coordinator, World 이전 HUD 구성과 실제 Tick의 Edit 예약 소비는 남았다. Early/Priest Training/전체 캠페인/데모의 자연 완주·전체 메뉴·시각/음향 동일성도 이 단위의 증거로 완료 처리하지 않는다.
