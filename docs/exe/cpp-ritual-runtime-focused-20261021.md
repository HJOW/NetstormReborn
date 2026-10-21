# Devastation·포획·희생 집중 실행과 다이스 파편 검사 교정

사용자의 테스트 전용 다리 지정 공급 후 Devastation·사제 포획·희생 의식 구동 요청을 수행한다. 준비 커밋은 `49729df6`이다. 현재/마지막 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 같으며 새 디컴파일·원본 게임 실행·원본/CD/AGENTS 변경은 없다. 기존 실행 이월 지시는 이번 명시 요청 범위에서 해제됐다.

## 집중 실행

최신 검사 바이너리 `58ee50f88f6a36a30fbccab77e70bfd891db88ad7f2c4ebf3851f091878c021d`에서 다음13개가 통과했다. 각 실행은 `tools/cpp_bridge_ritual_runtime.py`로 새 폴더에 입력·바이너리·종료·원시 로그를 보존했다.

| 필터 | 개수 | 근거 폴더 |
| --- | --- | --- |
| `CampaignDevastation_` | 4 | `extracted/ritual-runtime-20261021-devastation2/` |
| `CampaignSacrifice_` | 3 | `extracted/ritual-runtime-20261021-sacrifice2/` |
| `CampaignDais_` | 5 | `extracted/ritual-runtime-20261021-dais2/` |
| `CampaignRitualProgress_` | 1 | `extracted/ritual-runtime-20261021-progress1/` |

Devastation 실제 예약·대상 검색·HP100→1·시전 종료·실제 이동/도착·0x4f 포획·0x50 일반 Drop, 사제 메뉴/커서/패킷으로 다이스 건설·포로 전달·제단/소유자 통지·룬5·지연 음향·희생 완료·SP2500→7500·다이스/제단/포로 삭제 경로를 실행했다.

이들은 실제 공통 게임 몸체를 조립한 **집중 C++ 검사**다. 시전자/포로 내용물/시계 등 일부 입력이 명시돼 있으므로 실제 GUI의 자연 진행/포획 성공이나11미션 완주로 바꿔 보고하지 않는다. 희생의 음악 장치 요청 확인도 실제 음향 청취와 구분한다.

## 실패 보존과 교정

최초 `CampaignDais_` 실행은5개 중 파편 그림 검사1개가 실패했다. `extracted/ritual-runtime-20261021-dais1/`을 보존했다. 해당 검사는 `World.Destroy(dais,0)`으로 일반 삭제를 요청하면서 붕괴 파편 생성을 기대했다. 기존 원본 복원 `RawDamageablePreDestroy::PreDestroy`는 flags의 **200000h**가 있을 때만 붕괴 파편을 요청한다. 파편을 검사하는 입력을 `World.Destroy(dais,0x200000U)`로 교정하고 한국어 주석을 달았다. 원래 게임의 삭제 규칙이나 파편 생성 조건은 수정하지 않았다.

교정 후 해당5개가 전부 통과했다. Release `extracted/cpp-ritual-runtime-20261021-build1.log`는 종료0·경고0·오류0이다. 앱 바이너리는 다리 준비 실행의 `a244b88c3010cd8f981d0a0ca51cd3ac67fcc277d881fe065d54b86da0a65555`와 같고 이번 변경은 검사 입력뿐이다.

## 실제 GUI 진행 중

다리 지정 공급을 켠 Early5의 실제 GUI를 실행했다. HP/SP/적 좌표/미션 단계를 직접 쓰지 않는다. 첫 실행은 원래 입력으로 자연 A→B→C→D, 워크샾/생산/선택을 확인했다. 두 번째는19번 조각과 기존 회전 입력으로 전진 방향이 맞지 않아 D에 머물렀으며 성공으로 보고하지 않는다. 세 번째는 `Unable to open d/options.cfg`로 종료1이었고 결과와 복구 후 원본 해시 동일을 보존했다. 이후 같은4칸 조각의 회전 입력을 조정한 네 번째 실행을 진행한다.

입력/상태/BMP/원시 로그/실행 바이너리/복구 후 보호 원본 전체 해시는 `extracted/ritual-runtime-20261021-ui1/`부터 각각 독립 보존한다. UI2의 정상 종료0도 시전/포획/희생 성공을 뜻하지 않는다. 화면과 상태를 더 확인하며 GUI 시전·포획·제단 전달·희생까지 계속한다.

이번 집중 검사 단위는 `20261021-35 CP`로 로컬 커밋한다. GUI 진행을 끝내거나 중단 요청이 오기 전까지 일반 DataManager 후속 작업으로 돌아가지 않는다. 전체 목표는 active이며11미션 자연 완주와 원본 동일성을 완료했다고 표시하지 않는다. 다른 세션의 dotnetpj 변경은 건드리지 않고 push하지 않는다.
