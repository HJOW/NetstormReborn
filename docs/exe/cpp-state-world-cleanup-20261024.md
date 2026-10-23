# 공통 State 정리 부모와 실제 Core 해제 순서

2026-10-24 **후속 단위386 독립 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. PATCH10.78 **004b2df0~004b32d7,1256BYTE**를 제품 작성 전에 독립 PE 실행으로 고정하고 신규 `RawStateWorldCleanup.h/.cpp`로 복원했다. CD10.72 SourceMap 대응은 **0042f5c0**이며 이번에는 CD 몸체를 실행하지 않았다. 기존 [UI·Kernel 정리 분석](cpp-ui-reset-analysis.md)의 하위 구현을 재사용할 부모이며, 과거 잘못 기재한 `004b2c60`은 이 부모가 아니다.

**최초 Native147입력×x87 두 모드=294관측(정상194/명시중단100),162506명령과 C++ 전체 대조가 일치했고, 첫 격리 build1 종료0·경고/오류0·2검사/실패0**이다. 공용 제품/CTest/실제 Client 또는 게임 전환을 검증한 결과로 확대하지 않는다. 공유 기존 소스/CMake/LEFT/git,공용 build/lib,원본/CD 및 GUI에는 변경·실행이 없다.

## Native 선관측과 관측 범위

`tools/decomp_state_world_cleanup.py`는 신규 제품·검사 세 파일이 없는 것을 검사한 뒤 최초 TSV/report를 생성했다. 원본 PE에서 정리 부모와 기존 **0048f3b0 Core 선택/정리 자식**을 실제 CPU 실행했다. UI/Kernel/Pool/Player 재할당·SP·저장·전송 등의 하위는 주소/인자/순서와 명시 효과만 기록하는 외부 장치다. 이 장치 반환을 해당 하위 몸체의 복원 성공으로 세지 않는다. 최초 자료는 덮어쓰기를 거부하고 이후 `--verify`는 최초 입력 SHA만 확인한다.

입력은 reason0/비영·client0/비영·fort/battle/metamap 조합·중복모드 진단·Editor/missionPost/phase·known/owner·현재 Template header·정지깊이·signed 기본SP 극값과 원시 초기 패턴을 포함한다. 선택한 하위에서 현재 owner/Player base/Core/모드/카운터를 바꾸거나 첫·두 번째 호출에서 중단하는 입력도 포함한다. 두 x87 정밀도는 동일 입력별 전체 출력이 같다. 무한 정지나 임의 유효하지 않은 물리 포인터 전체를 포괄하는 관측은 아니다.

고정 TSV는 현재 Core 주소역할·정지깊이·부모가 직접 접근하는 전역의 전체 BYTE·현재/이전/교체 Player 세 벌 각각1620BYTE·TYPE+c4·9×20BYTE Template header·전체 하위 호출 trace를 기록한다. 주소는 같은 물리 할당을 나타내는 식별자로만 정규화한다. C++ 검사는 원문 출력으로 기대값을 읽고, 장치가 부모 분기나 정리 결과를 계산하지 않는다. 배열 초기 패턴과 명시 하위 효과만 같은 입력으로 구성한다.

## 복원한 부모 순서

1. reason0의 전송 정리 또는 최초 client 값으로 선택한1..8/9의 disconnect 뒤10/11을 호출한다. fort를 먼저 캡처하고 client/FullDeck를0으로 쓴 뒤 현재 나머지 모드로 중복을 진단한다. 진단이 복귀하면 계속한다. 정지깊이는 매 resumeGame→resumeAudio 뒤 다시 읽는다.
2. 기존 네 자식 뒤 실제22DWORD를0으로 쓴다. 현재 Template slot/header 첫DWORD는 다음 여덟 전역쓰기보다 먼저 읽는다. manager가 있고 현재 fort·비Editor이면 Fort를 저장하고 **그 분기에서만** slot을 다시 읽는다. 뒤의 battle/missionPost/phase14 관문은 현재값을 읽는다.
3. Player1..8의 +7c..+98 여덟DWORD를0으로 쓰며 **각 DWORD 쓰기마다 현재59534c를 다시 조회**한다. AI→사제 정리 뒤 현재 known/owner를 읽어 다른 Player 정리 또는 같은 owner의 실제SP setter/표시를 호출한다. Player0/다른 필드까지 지우지 않는다.
4. Template 정리 뒤 metamap을 캡처하고54c8a0을0으로 쓴다. 선택된 메타맵 자식과 커서 정리를 거쳐 기본SP를 TYPE+c4에 쓴다. **004b307c는 signedDWORD00540bdc의 FILD→float32**이며 정밀 C의 float/double 표현을 그대로 해석하지 않았다. SP 값과 type번호는 TYPE 조회 callback 앞에 캡처되어 있다.
5. Player 링크→Kernel→Gump Kill→Cleanup→Uber 교체→43b540 순서다. seed005c89b4는 **하위WORD만0**으로 쓰고 known0→**SelectCore(0,1)**을 수행한다. 그 뒤 Pool/그래프/기타 표, **004b31ad Player 해제·재생성004910a0**과 Zone을 호출한다. 실제 Core 정리 호출004b312d가 Player 재할당보다 앞선다.
6. Editor를 캡처한 뒤 카메라/상태 전역을 쓰고 현재 known에 따라 SP를 갱신한다. 현재 State/missionWorld를 캡처한 뒤 missionPost와 fort/battle/metamap/Editor를 쓴다. 캡처 State29/33 외에는 SelectCore(1,1)이다. 설정 재해석→현재카운터 증가→두 번째 MapMode→새 벽시각+원문double10의 기한→나머지 전역/네DWORD−1 순서다. 같은 자식의 두 호출을 합치지 않는다.

## 실제 소유자와 필수 하위의 접합 계약

`CleanupStateWorld(reason,globals,hooks)`의 globals는 현재 `read32/write32/write16/write64Bits`와 현재 Player span getter, 현재 Template manager getter, 실제 TYPE+c4 float 참조다. Native 주소는 필드 식별자이며 host 메모리 주소로 역참조하지 않는다. 접근 장치는 같은 Client/Session의 실제 저장소에 순수 조회/직접 쓰기를 해야 한다. callback마다 복제한 DWORD, 모드 bool을 담은 임시DWORD, snapshot 배열을 실제 alias라고 공급하면 계약을 만족하지 않는다.

`hooks.effect`는 enum으로 식별한 기존 하위 몸체에 분배한다. 고정 this 자식568b60/54c8a0/5c86c0도 그 실제 owner를 호출해야 한다. 그 외 저장·AI/사제·SP·Core·Pool·벽시각은 별도 명시 hook이다. **선택해서 도달한 하위만 그 위치에서 필수**이고, 미연결 enum은 오류여야 한다. 큰 게임 계산을 빈 성공 callback으로 채우지 않는다. 진입 시 모든 hook를 선행 요구하지 않으며 중단까지의 현재 전역/Player/캐시 효과를 되돌리지 않는다.

기존 RawLocalMissionState의 `cleanupBattle`는 이 실제 부모를 필요로 한다. 실제 ClientWorldGlobals 공유 전에 Core/Player/Template/모드 필드의 동일 수명을 정리하고 기존 Gump/Kernel/Pool 소유자를 위 순서대로 연결해야 한다. 이 신규 모듈은 실제 owner나 Player 할당을 새로 만들거나 Session별로 리셋하지 않는다. 현재 Client/Core 소유자의 span은 외부 Player 해제 전 위 SelectCore0을 거쳐야 한다. 전체 Client/Session 연결·공통 시작·모드 전환·최종 Editor loaded는 이번 범위에서 미검증이다.

## 검사 등록과 고정 자료

`StateWorldCleanup_ReplaysIndependentPe294`는 정상/중단 전체 행에서 모든 기록 필드와 trace를 대조한다. Core 자식은 동결 `RawPlayerCoreData.cpp`의 같은 실제 참조/캐시/Player뷰를 실행한다. `StateWorldCleanup_MissingCorePreservesEarlierEffects`는 필수 Core hook가 없으면 앞선 known0/seed하위WORD0 효과만 남고 Pool/Player 재할당이 시작되지 않음을 확인한다. 실제 Player 재할당은 검사의 명시 base교체 장치이며 allocator 복원 증거가 아니다.

공용 등록 대상은 `src/o/RawStateWorldCleanup.cpp`, `tests/StateWorldCleanupTests.cpp`와 **`NETSTORM_STATE_WORLD_CLEANUP_FIXTURE`**=`tests/fixtures/state-world-cleanup-x86.tsv` 절대경로다. 선행 `RawPlayerCoreData.cpp`가 필요하며 기존804관측 fixture는 별도 보존한다. 격리 runner는 신규 부모/기존 Core 자식/신규 검사만 컴파일하며 공용 정적 라이브러리를 읽거나 링크하지 않았다.

최초 관측은 [Native report](../../extracted/cpp-state-world-cleanup-20261024-native.json), 전체 실행/소스/로그 SHA와 검증 범위는 [validation manifest](../../extracted/cpp-state-world-cleanup-20261024-validation.json)에 고정한다. 최초 probe/build/test 로그는 `extracted/cpp-state-world-cleanup-20261024-probe1.log`, `extracted/cpp-state-world-cleanup-20261024-isolated/build1.log`, `test1.log`로 보존한다.
