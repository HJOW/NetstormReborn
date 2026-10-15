# 실제 총·배리어의 현재 동맹 조회

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 같은 PC의 전체 정밀 소스와 기존 원본 관찰을 사용한다. 새 디컴파일·native 고유입력·원본 게임/GUI 실행은 없다. DirectDraw 전체화면 오류는 이번 범위에서 제외한다.

## 실제 세션 연결

기존 총 표적 검색은 `GunTargetState.allyMasks`와 `playerFlags`를, 총 프로세스와 배리어는 `GunProcessState.allies`를 읽었다. Session의 실제 Player와 방향별81DWORD가 AI 생성/동맹 설정으로 변경돼도 이 초기 사본에는 반영되지 않았다.

`AllyMask`·`PlayerFlags`·`IsAlly`에 현재 조회 경계를 추가하고 Session이 같은 Player+54 DWORD·Player+50 BYTE·방향별81DWORD의 한 칸을 직접 공급한다. 기존 배열은 독립 native fixture의 값형 입력에 남는다. 같은 소유자의 자기 동맹 초기화는 실제 방향별 표에도 기록한다. 서로 다른 소유자의 반대 방향을 자동으로 변경하지 않는다.

| 소비자 | 원본 읽기 순서 |
| --- | --- |
| `MayTarget` | 친군발사가 꺼졌을 때만 발사자 Player+54를 읽고 후보 소유자 bit를 검사한다. |
| `FindArcher` | finder의 첫 후보/next 준비 뒤 친군발사 조건과 자기 동맹 mask를 검사한다. |
| `CannonRay` | 사거리 진단이 켜졌을 때만 현재 Player+50 BYTE의 AI bit20을 읽는다. |
| `GunProcess::Frame` | boss→유효 표적→죽음/free→방향별 동맹 DWORD→친군발사 순이다. boss가 꺼지면 표적/동맹 조회를 실행하지 않는다. |
| `RawFenceTarget::Across` | 압축된 두 번째 끝점 번호를 저장하고, 동일 소유자는 먼저 반환한 뒤 발사자→첫 끝점 방향의 동맹 DWORD를 검사한다. |

기존 `Frame`은 boss가 꺼져도 동맹 계산을 먼저 실행하고 마지막 Clear만 막았다. 현재 조회기 도입 과정에서 정밀 원문과 대조해 boss 조건을 외부로 옮겼다. 친군발사가 켜졌을 때에도 살아 있는 표적의 동맹 DWORD를 먼저 읽는 원본 순서는 유지한다.

## 검증과 범위

기존 GunTarget 두 판본3,454입력, GunProcess 두 판본의 Frame 입력, 배리어 Across626·Point48입력을 현재 getter로 다시 재생한다. fallback 배열에는 반대값을 넣고 현재 방향별 표에는 nonzero7과 반대 방향의 다른 값을 사용해 현재 저장소와 방향을 확인한다. 기대값은 기존 native fixture의 전체 반환/필드/패킷/효과를 사용한다. 새 고유 관찰로 합산하지 않는다.

별도 짧은 평가 검사는 boss0의 조회0, 죽은 표적의 조회 생략, 살아 있는 표적의 정확한 한 방향 읽기와 getter 이후 친군발사 읽기를 검사한다. getter 내부 변경은 API 읽기 순서 검사이며 원본 게임에서 동시에 상태가 바뀌었다는 관찰은 아니다.

빌드·집중·전체 결과와 현재 소스/기존 fixture SHA는 `cpppj/campaign-current-alliance.json` 및 LEFT_JOBS 최신 절에 기록한다. 과거 `guntarget.json`·`archertype.json`의 당시 제품 SHA는 역사 자료로 유지한다. 이 PC에 일부 과거 선택 export가 없으므로 해당 과거 보고서의 전체 `--verify`가 통과했다고 주장하지 않는다. 현재 전체 정밀 함수표와 보호 PE·fixture SHA를 새 보고서에서 구분해 보존한다.

이 단위는 [실제 Session AI 생성·초기 갱신](cpp-session-ai-20261015.md)의 동맹 소비자를 연결한다. 실제 로그인/Ready/Stage3·AI 전략 활성화·미션 자연 완주와 전체 픽셀/음향 동일성은 별도 후속이다. 기존 분석은 [표적 검색](cpp-gun-target.md), [총 프로세스](cpp-gun-process.md), [배리어 표적](cpp-run33-20261015.md)을 따른다.
