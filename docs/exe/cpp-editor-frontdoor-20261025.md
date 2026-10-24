# 메인 Edit의 실제 맵 선택과 숫자 State 예약 — 2026-10-25 단위416

단위415 `ab298e32` 뒤 AGENTS와 양 인계를 다시 읽었다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일·원본 게임 실행·원격 push는 없다. 이 호스트의 실행 예외에 따라 클론 창을 실행했으며 허용 설정은 매회 원래 바이트/존재 상태로 복구했다. 이전 턴은 실제 Early1/2 완료·전체 검사·커밋을 남긴 진척으로 분류한다.

## 실제 입력 경로

메인 Edit의 `Tell,NotImplemented`를 원본 `AskEditFort`로 바꿨다. 메인 돌 버튼 콜백은 이 명령을 같은 일반 입력 고리에 직접 예약한다. 기존 HTML 메타/메뉴 행·스크립트도 `FortSelectionScriptCode`의 원문 번호와 같은80칸 문자열 고리를 사용한다. Guide와 실제 Gump가 소비하지 않은 사건만 기존 `DispatchFortSelectionCommand`로 보낸다.

원문 `004d7408`의 **mode2 → PathSpec(lastEditFort) → 별도pending → Refresh2 → Tell LoadEditable** 순서를 보존한다. `ReadFort`는 사건 y에서 이름을 읽고 같은 Client의 다섯144BYTE 블록 중 next 첫DWORD에1을 쓴다. mode2와 진행State1을 같은 값으로 취급하지 않으며, 선착순인 `State::Post`로 숫자 next의 덮어쓰기를 대체하지 않는다. Ask는 next를 예약하지 않고 Cancel도 임의로 지우지 않는다.

`BattleActive()`는 현재 Session의 실제 자산 Battle 관문을 읽는다. 현재 부모 분배는 첫 관문에 그 값을 공급한다. **Battle/phase14의 기존 파일 보존 하위는 아직 실제 Fort 파일/State 장치 접합이 필요하며 선택되면 기존 필수 경계에서 실패한다.** 빈 성공이나 현재 미션 이름의 가짜 복사로 채우지 않았다.

`fort_selection` 보고는 mode/확정phase/현재요청/next/pending 순서다. `fort_catalog`는 현재50칸 캐시의 개수/dirty/설정/키만 읽는다. 현재 메인 UI의 Native 확정phase 초기0과 실제 원본 MainMenu 부모의 미복원도 구별한다.

## 실제 목록과 작업본 입력

기본 보호 디스크에는 Fort 파일 **433개**가 있다. 독립 원시 State 읽기에서 Alerted/Alliance/TEST01/TEST02의+17은 모두2다. 원문 목록은 수락한 이름을50개까지 보관하므로 기본 실제 창에도50개가 표시된다. `mayEditMissions=0`은 종류1 포함을 막는 설정이며, 이 설정에서도 종류2인 맵이50개를 먼저 채워 TEST01이 목록에 나오지 않는다. 제품의 열거 순서·필터·상한을 바꾸지 않았다.

사용자 작업 파일 경로도 검증하기 위해 별도 실행 폴더에 **보호 TEST01/TEST02의 원문 Fort를 바이트 그대로 복사**했다. 기존 설정 `fortSpec`/`SOLOSpec`을 그 작업 폴더로 지정하고 실제 PathSpec·디스크 열거·Template 분류·HTML 목록을 사용한다. 목록/State 바이트를 합성하거나 보호 디렉터리를 정리하지 않는다. 원문 `lastEditFort=TEST01` 설정은 실제 pending과 체크 표시 검사에 사용했다. 이는 SP·HP·단계·다리 패턴·완료값 주입이 아니다.

최종 제품 **`cbdb3eaecd663663761fe1f62e06cc316bc40459400dbd9fd1b2602b34a82556`**에서 `tools/cpp_editor_selection_smoke.py`의 네 경로가 모두 통과했다.

- 기본50행 → Cancel.
- 별도 작업폴더2행/TEST01 체크 → Cancel.
- TEST01 체크 → TEST01 선택 → 같은15프레임에 pending TEST01/next1.
- TEST01 체크 → TEST02 선택 → 같은15프레임에 pending TEST02/next1.

버튼을 클릭한 **같은3프레임**에 실제 LoadEditable 부모가 열리고 `Create New Map`/`Cancel`을 확인했다. 모든 상태와 이름이 같은 BMP의 계획 프레임을 검사했고 작업 목록/체크 화면을 직접 확인했다. 네 실행은 clone/wrapper 종료0, 상한 미도달, 원본 복구 동일이다.

## 회귀·실패 보존·경계

같은 최종 제품의 Campaign8행/Early6행/Back 복귀와 Early1의 MORE→A1.→OK→플레이→F4→B./66을 확인하고 화면도 검토했다. 전체 Early 자연 완료나 Editor 편집 완료를 새로 입증한 실행은 아니다.

최초 `select1`은 clone 종료0/복구 동일이지만 설정 복구 `OSError22` 두 재시도와 PowerShell wrapper 종료1을 보존했다. 원인은 이번에도 확정하지 않는다. 공유 도구의 최종 네 실행은 wrapper0이다. 최초 Early 회귀에는 검사기의 실제 별칭 `home=VK_F4`를 잘못 `f4`로 바꿔 `Unknown UI key: f4`/clone1이 발생했다. 실패 driver와 원시를 보존하고 기존 별칭으로 복원한 `early2`가 통과했다. 제품 CLI를 이 입력 오류에 맞춰 수정하지 않았다. 첫 빌드/관측과 두 번째 빌드의 원시도 구별한다.

최종 집중 FortSelection4/맵메뉴4/실제목록9/HTML메타11 **28회·고유28·실패0**이며 기존 독립 원문 **3672행** 대조를 포함한다. 새 Native 실행/fixture 재생성은 하지 않았다. 전체 CTest는 **2796검사·실패check0·종료0·319.59초**다. 두 빌드의 관측 경고/오류는0이며 검사 SHA는 **`096680183fa8de49eecba3e45b6290cb5bf3ac8b5ba4c5fee7409b709962158b`**다. 보호1821파일 감사와 현재 자료의 SHA는 [제품 보고서](../../cpppj/editor-frontdoor-runtime-20261025.json)에 기록한다. 최종 소스5개와 실행 파일·검사·원시는 `extracted/cpp-editor-frontdoor-416-*`에서 SHA로 보존한다.

**이 단위의 선택은 아직 소비되지 않은 실제 next1 예약이다. 현재 다음 Tick은 기존 Editor State1 부모를 실행하지 않으며 선택 후 편집 월드가 설치되지 않는다.** 후속 작업은 원문의 State 정리/prefix → 작업본과 current Fort 이름 → 기존 Editor checkpoint → HUD/팔레트 → 공통 State 확정/다음Poll4 → 실제 설치 공개와 편집 명령 전송이다. 전체 Uber 배경GIF/HUD dock/resize와 단일 생산/미니맵 소유 제한도 선행 의존이다. Create New Map·Save as·편집 도구·저장/재로드·커스텀 언어별 시나리오, 전체 Early/캠페인/Demo 사용자 완주와 원본 완전 동일성은 계속 미완료다.
