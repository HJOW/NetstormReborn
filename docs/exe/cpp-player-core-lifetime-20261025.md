# 실제 Player 초기화와 장기 CoreData 수명

2026-10-25 **단위422**이다. 이전421은 `a49c451e / 20261025-15 CP`로 Editor 내부 checkpoint/실제 저장본 재설치와 전체2827검사를 마쳤다. AGENTS/양 인계를 다시 읽었으며 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 같다. 보호1821파일 기준은 `extracted/cpp-player-core-lifetime-422-baseline.json`이다. 실제 State1 설치를 위한 Player/Core 수명 연결이며 전체 State 정리 완료를 뜻하지 않는다.

## 원문 호출과 안전한 실제 소유권

보호 PE의 **0043952c→004910a0**는 공통 시작의 Player 초기화이고 바로 뒤00439537이 ColorSort다. 원문 공통 정리004b2df0은 **004b312d→0048f3b0(0,1)**로 이전 Core10BYTE를 캐시에 보존하고 현재 포인터/active를 지운 뒤 **004b31ad→004910a0**에서 이전 Player/factory를 반납하고 재생성한다. 후반004b3257 및 State1 prefix004b71f5도 실제 Core1 선택을 호출한다. 같은 PE와 기존 원문 fixture(Core804/Player 초기화203/공통 정리294)를 정적으로 대조했다. 새 원본 실행/전체 재디컴파일은 하지 않는다.

`ClientPlayerRecords`는 공통 시작/State 부모가 고정뷰 소비자 없이 실제 초기화할 수 있는 경로를 제공한다. 기존 `Use.Initialize`도 같은 실제 Native 부모/메모리 장치를 사용한다. `Use.AdoptCurrent`는 이미 전체 초기화가 정상 복귀한 같은 주소만 빌리며 카운터/동맹/할당을 다시 쓰지 않는다. 부분 실패는 완료 기록으로 대여하지 않는다.

같은 Core 소유자를 약하게 연결해 Native Player base가 NULL/새 주소로 바뀌는 각 위치에서 실제 Core의 비소유 Player span도 같은 할당으로 갱신한다. 별도 Player 사본은 없다. 이전 Player 안을 가리키는 실제 현재 Core가 남아 있으면 초기화 앞에서 재할당을 거부한다. 호출자가 **known0→Core0 선택/캐시 보존**을 명시 수행해야 한다. 소멸자에서 무조건 current를 지우거나 이전 메모리를 계속 보관해 잘못된 수명을 숨기지 않는다.

## 실제 공통 시작과 기존 미션 어댑터

새 공통 시작 장치는 실제9기록·9factory·81방향 동맹을 기존 RawPlayerInitialization/RawPlayerParticipation 본체로 구성한다. 실제 현재 known0를 조회하며 보호 PE의 숨김9BYTE 초기0는 이 자연 관문에서 읽지 않는다. 아직 없는 World/Chat 장치를 상수 모드/빈 성공으로 실행하지 않는다. Client 생성자/getter는 할당하지 않으며 PATCH Run의 원래0043952c 위치에서 초기화한다.

실제 Client 서비스는 같은 장기 Core 소유자를 Session에 제공한다. 기존 미션 설치 어댑터는 재할당 앞에서 known0/Core0을 실행하고, 실제 Player/모드 설정 뒤 Core1을 다시 선택한다. 이미 원문 부모가 초기화한 기록은 명시 대여할 수 있다. **이 어댑터의 앞뒤 선택은 전체004b2df0의 전송/AI/Kernel/Gump/Pool/UI 순서를 대신하지 않는다.** 최종 Native State1 부모의 그 나머지 연결은 여전히 필요하다.

Login의 current/cache/active는 더 이상 별도 지역 상태가 아니며 같은 Core 소유자의 실제 슬롯/10BYTE/한DWORD를 직접 빌린다. 현재 Core 갱신도 같은 getter를 사용한다. 독립 Session은 자체 같은 실제 소유자를 유지한다. C++ Session 반납은 Player 할당을 소유자에 남기며 다음 원래 정리 호출 전까지 현재 Core를 임의로 선택 해제하지 않는다. `player_core` 보고 행은 실제 base/초기화 상태/현재 Player와 Core 폭·offset/Session 동일 소유자를 읽기만 한다.

## 검증 결과

공통 실제 할당/방향 동맹·Core 뷰, 이전 현재 Core를 남긴 재할당 거부와 실제 Core0 캐시 보존 뒤 재할당/Core1, 초기화 완료 기록의 대여·중복 초기화 거부, 실제 부분 할당 실패, Editor checkpoint 두 Session의 명시 정리/대여와 소멸자 무관한 현재 Core 수명, 실제 로컬 Zone/로그인 패킷의 같은 현재 슬롯 갱신 검사를 추가했다. 빌드/집중/전체 검사와 실제 회귀·최종 보호 감사는 아래에 이어 기록한다.

첫 빌드는 종료0이지만 새 보고 행의 `core` 이름이 기존 내부 로컬을 가리는 **C4456 1건**이 있었다. 첫 소스/제품/검사·로그와 집중25/0을 보존했고 변수 이름을 교정했다. 최종 빌드2는 종료0·컴파일 경고/오류0이며 집중은 공통/수명4+기존 소유자4+checkpoint13+실제 Login1+원문 Core3=**25회·고유25·실패0**이다. 기존 원문 fixture/본체는 수정하지 않았다.

빌드2 제품은 **`58452decf295b50eb1afd7c72041719bb079010c056ca83d95fe28d84dedfad0`**, 검사는 **`a0f6c079c275d5c53a8c5ef54d4671b058a03c9621ad02202e84a07ca732fd74`**다.15개 소스/빌드파일과 두 실행 파일을 `extracted/cpp-player-core-lifetime-422-source2/`에 고정했다. 실제 메뉴/안내/F4·목록/Back도 통과했다. 초기 메뉴는 실제 Player1620/Core 미선택이며, 미션에서는 재할당한 같은 Player의 Core10/offset288과 동일 Session 소유자가 관측됐다. 실제 단계 전체 진입/자연 진행·전체 CTest·최종 보호 감사는 이어 기록한다.

같은 최종 제품의 일반31개와 Demo3개 초기 플레이 진입, Edit TEST01 체크/TEST02 선택 예약·취소가 모두 종료0으로 통과했다. Early1 안내 뒤 플레이 화면·마지막 캠페인·마지막 데모의 실제 이미지를 확인했다. 각 종료 묶음의 설정은 원래 바이트로 복원됐다. Edit 목록 성공은 최종 State1 설치나 실제 편집/저장 UI 완료로 세지 않는다.

자연 Early2는 같은 제품에서 **196.23초·종료0**으로 A~I/발사기 생산·회수·환불75/SP3075→같은 프레임 Tutorial3 Guide4까지 통과했다. 실제 다음 안내 이미지도 확인했다. 목표 직전 Player base **805347328**은 다음 안내에서 **805388288**로 실제 재할당됐고 두 프레임 모두 Core10/offset288/동일 Session 소유자였다. 상태 주입0·제품타이머1·기존 완료값 보존이며 새 진행0→1/디스크 진행 저장의 증거는 아니다. 모든 GUI 종료 뒤 보호1821파일이 기준과 같은지 확인한 다음 전체 CTest를 시작했다.

최초 전체 CTest는 **2833검사·실패check1·338.17초·종료8**이었다. `SessionPlayerCoreRefresh_RequiresSelectedLoginCoreBeforeFirstStore`가 미선택 현재 Core의 Native 첫 store 거부 대신 과거의 `Login선택Core` 오류 문자열만 요구했다. 새 구현은 같은 장기 슬롯을 읽으므로 Login 유무는 선택 여부를 뜻하지 않는다. 최초 전체 로그/LastTest를 `ctest1.log`/`ctest1-tests.log`로 보존했다. 검사에서는 실제 Core0 선택→빈 슬롯의 첫 store 물리 거부/Player 불변과 실제 Core1 선택→Login 없는 기술·다이스 결과/동일 현재 슬롯을 함께 검증하도록 교정했다. 게임 몸체를 과거 Login 제한으로 되돌리지 않는다. 검사 변경을 포함한 소스16개를 `source3/`에 별도 고정해 빌드3·집중/전체 재검사를 진행했다.

빌드3은 종료0·컴파일 경고/오류0이고 **게임 제품 SHA는 빌드2와 같다.** 검사만 **`2168304e6cc0fe7c894ba694c9fc7d701e7d5b4c71cb4cb2b02fcfc9fb405aa6`**로 갱신됐다. 같은 최종 검사에서 기존 집중25개를 다시 통과했고 CoreRefresh3개도 통과해 **집중28회·고유28·실패0**이다. 실제 미선택의 첫 store 거부/Player 불변과 Login 없이 선택한 기술0x11·다이스2 결과를 확인했다.16소스/두 실행 파일은 `source3/`에 고정했다. 모든 GUI는 같은 게임 제품의 증거다.

최종 전체 CTest2는 **2833검사·실패check0·329.98초·종료0**으로 통과했다. 최초 실패를 보존한 채 새 검사 바이너리로 전체를 다시 실행했다. 마지막 보호1821파일·원본/CD 파일명 목록과 고정16소스/두 실행 파일이 기준과 같다. 실제 checkpoint 수명 검사5회의 저장 파일도 별도 원시 경로에 보존했다. [제품 보고서](../../cpppj/player-core-lifetime-runtime-20261025.json)는 최초 컴파일 경고/전체 실패·최종 결과와 원문 정적 근거·같은 제품의 실제 입력/화면·Player 재할당/현재 슬롯·보호 파일의 SHA를 기록한다. **20261025-16 CP**로 로컬 커밋하며 push하지 않는다. 이번 단위에서 원본 게임/새 Native oracle/전체 재디컴파일은 실행하지 않았다. 마지막 전체 정밀 디컴파일은 계속 **HJOW-Athlon / 2026-10-23**이다.

## 남은 전체 목표

전체 Native State1 공통 정리/prefix·checkpoint 뒤 HUD/팔레트/State 확정/다음Poll4/loaded·실제 편집/저장 UI는 계속 남는다. Core 메타데이터 장부/전체 로드·다른42e050 호출자도 원래 문맥을 연결해야 한다. 전체 Early/캠페인/Demo 사용자 완주·승패·완전 동일성의 완료로 판정하지 않으며 `/goal`은 활성 상태다.
