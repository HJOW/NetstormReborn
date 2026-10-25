# Graph 기록·스택의 실제 월드 저장소 수명

2026-10-25 단위431. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 정밀본은2026-10-23이다. 이전430은bae8b702의집중83개 검증 제품이다. 이번에는 기존 정밀본/PE/Native16 관측과 구현을 대조하며 새 원본 실행이나 디컴파일은 하지 않는다.

## 물리와 실행기

`RawGraphStorage`는 records255×6BYTE와 flood stack4096DWORD만 소유한다. 원문의00562c18/00562c20 두 heap을 하나의 연속 Native 구조체라고 취급하지 않는다. `RawSurfaceStorage::graph`가 Client 표면 물리와 함께 장기 보관하고, World의 새 `RawGraph` 실행기는 명시 storage 참조 생성자로 현재 배열을 빌린다. 공유 생성자/소멸자는 배열에 쓰지 않는다.

타입/FrameCode snapshot과 `GraphRecovery`는 기존 World 실행기에 남긴다. recovery는 원문 전역이 아니라 현재 host의 오류 처리 정책이다. 기존 records/stack 입력 생성자는 계속 독립 배열을 소유한다. `CalculatePostDestroy`의 임시 `next`도 이 기존 생성자를 사용하므로 Validate 성공이나 후반 체인 오류가 실제 장기 표를 선행 변경하지 않는다. 정상 Commit에서만 원래 물리에 반영한다.

`ResetRawGraphTable(storage)`는 기존 `RawGraph::ResetTable()`의 실제00462b20 본문을 공유한다. 앞251행의 surfaces/inUse WORD0, Graph0.inUse1/reserved.lowBYTE1, sentinel254의 두WORD7dfd만 쓴다. reserved.highBYTE·틈251~253·sentinel.reserved·stack·Pool/Hash/spot은 보존한다. 기존 실행기 메서드도 같은 helper를 호출하므로 Session 없는 State cleanup에서 dummy 실행기 없이 사용할 수 있다. 전체 cleanup 호출자는 이번에 연결하지 않는다.

기존 일반 Acquire는 새 지역 Graph가 만들던 전체0/부분 Reset 결과를 같은 장기 배열에서 유지한다. prepared Acquire/반납과 준비 World 생성은 현재 Graph에 쓰지 않는다. 기존 World 생성자의 무조건 ResetTable은 제거한다. `RawGraph`/`RawSurfaceStorage` 자체의 복사·이동은 주소 참조 수명 때문에 금지한다.

## 원문 최초 확보와 남은 범위

이 변경은 원문의 최초 시작00462eb0을 새로 이식한 것이 아니다. 원문 시작의255행 초기화와 reserved.lowBYTE0/highBYTE 보존, 첫 Flood00462f10의 lazy new0x4000·4096DWORD0은 현재 고정 배열/일반 Acquire 호환 초기화와 구별한다. 실제 State cleanup의 partial Reset과 prepared 대여를 가능하게 하는 저장소 수명만 복원한다.

새 검사6개는 공유/독립 snapshot, Validate 성공·후반 체인 오류/Commit의 분리, 기존 Native16을 helper/공유 실행기로 두 번 재생한 전체 바이트, World/lease 이후 같은 배열 수명, 준비 생성 실패의 현재 바이트 보존, 일반 fresh 호환을 다룬다. 원문 fixture와 기존 검사 기대는 변경하지 않는다.

## 첫 통합 실패와 실제 월드 정리 교정

8소스 Source1의 빌드2와 집중94개는 통과했다. 빌드1은 파일별 정의 때문에 직렬화된 작업을 중단한 기록이며 성공 제품으로 세지 않는다. 빌드2부터 로컬 VS18의 MultiToolTask를 사용해 최대4개 컴파일러로 증분 빌드했다. 기존 소스나 Native 기대 자료를 빌드 방식에 맞춰 바꾸지 않았다.

Source1의 통합 batch1에서는 Home/메뉴 GUI가 통과했지만 자연 Early2 뒤 다음 Tutorial3 설치에서 게임이 종료1로 실패했다. goal.bmp까지 도달했고 nextBriefing은 생성되지 않았다. 전체 CTest는 시작하지 않았다. 194.19초의 실패 로그와 실행 당시 보고서 사본을 `extracted/cpp-graph-storage-431-batch1/`에 보존한다. 원본/CD·소스·제품 불변과 허용된 두 설정의 복원은 통과했다.

429에서 Client 수명으로 옮긴 CaptureEnd가 현재 부분 C++ 월드 정리에서 지워지지 않아 다음 Zone의 `!earlyCaptureTimer.isRunning()` 진단 경로가 선택됐다. 정밀C와 보호PE에서 State7의004b2df0→004b4be0 순서,004b30ed의005c8618 +0, 말미004b329b/004b32b9의 LoginEnd/ConnectEnd +0을 확인했다. Source3은 실제 월드가 있을 때 `UberGump::ClearWorld`에서 CaptureEnd를 Session/UI/Kernel 반납 전에, 나머지 두 기한을 월드 반납 뒤에 쓴다. RawSession 생성/소멸의 보존 계약과 ActivityEnd/State는 유지한다.

이는 현재 월드 정리 어댑터에 명시적인 세 쓰기를 연결한 교정이다. 전체 Native004b2df0/State7 부모·다른 전역·처음 HUD 정리의 복원이 아니다. 진단을 무시하거나 다음 Session 생성자에서 값을 덮어 실패를 숨기지 않는다.

최종 Source3은 기존8개와 UberGump.cpp의9소스다. 같은 제품의 집중검사2 **94개·실패0**, batch2의 전체 CTest **2,894개·실패0**을 통과했다. Source3 빌드4의 대상 출력과 별도로 명시적 종료를 기록한 빌드5가 종료0이며 컴파일 경고·오류0이다. 보호1821파일·9소스·4실행 파일 SHA가 같다. 게임 SHA는 `18b672612ef444352bea750b941eb310c954c12c4b5c2eec40458983ed95dc82`다.

같은 최종 제품의 Home/메뉴 GUI가 통과했고 실제 캡처에서 안내·덱·미니맵·Early 목록을 확인했다. 자연 Early2는 실제 타이머1/상태 주입false/기존 진행값 보존으로 A~I를 거쳐 환불75·최종SP3075·Tutorial3 초기SP2000과 같은 프레임 다음 안내를 확인했다. 다음 안내 캡처의 SP1700은 설치 후 실제 생산이 진행된 화면이며 초기 관측값2000과 구별한다. 모든 단계의 원본/CD 불변과 허용된 두 설정의 정확 복원도 통과했다.

[실행 보고서](../../cpppj/graph-storage-runtime-20261025.json)는 첫 실패와 교정 제품의 실제 로그·해시를 함께 기록한다. 이는429~431의 통합 검증이며 실제 Edit State1 전체 정리/설치나 사용자 편집·저장, 모든 미션 완주·진행 저장/재실행의 완료를 뜻하지 않는다. 원본 최초 Graph 할당·lazy stack·전체 cleanup 부모 접합도 계속 남는다.
