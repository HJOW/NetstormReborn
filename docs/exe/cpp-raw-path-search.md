# cpppj 실제 경로 검색·초기화 연결

마지막 디컴파일 수행 PC: **VM-W11-CODEX, 2026-10-13**. 대상은 패치 10.78이며 CD 10.72의 실제 함수와도 비교했다.

`RawPathSearch`를 빌드와 검사에 등록하고 같은 SID 풀·hash0·spot·현재 타입/프레임을 사용하는 `MakePathSearchHooks`로 `RawPathProcess`에 연결했다. 표면 사본이나 별도 이동 허가 정책을 만들지 않는다.

| 몸체 | 패치 10.78 | CD 10.72 |
| --- | --- | --- |
| 실제 표면 조회 | 0040eaf0 | 0045b670 |
| BFS·역추적·축약 | 00488370 | 00402750 |
| 시작/목적 Graph 검사·검색 분배 | 00488af0 | 00402100 |
| Path Init | 0048a660 | 0047ec60 |

원본의 직선 방문 순서 0/2/4/6과 대각선 선행 방향 조건, signed WORD 거리표, 16,384개 두 BYTE 큐, 같은 SID의 즉시 접합과 현재 프레임의 `Bridge::Connects`를 유지한다. 역추적 비용은 이전 방향 10/다른 방향 15, 대각선 -10, raw+12의 `&6==2`이면 +30이다. 패치 정책값 5의 방향 마스크 변경과 기존 경로/double 시각을 보존한 append도 포함한다.

Init은 시작 좌표의 실제 표면 SID가 0이면 Pop·검색·current 초기화를 모두 건너뛴다. 표면이 있으면 void 부모의 Pop, flags를 전달한 검색, current 초기화 순서로 실행한다. 좌표 범위만 확인하던 연결을 이 흐름으로 교정했다.

새 독립 관찰은 BFS/축약 **644개**와 Init **64개**, 합계 **708개**다. `pathsearch-x86.tsv`와 `pathinit-x86.tsv`를 C++에서 재생하며 꼭짓점·방향·double 시각·전체 거리표/큐 FNV·정책 마스크와 부모 좌표/void/current를 비교한다. 기존 Path 시간/단계 360개는 재감사했으며 신규 관찰에 더하지 않는다.

통합 검사는 같은 raw 풀에서 실제 Pop/Unpop·hash/spot 등록·BFS·Kernel 갱신·도착 반환 1000의 Form 삭제를 실행한다. 두 PE 각각의 client Form을 사용한다. 외부 부모의 Halt/Arrival·효과음·cleanup은 명시 관찰 경계이며 실제 사제나 건설 부모의 전체 구현으로 세지 않는다. native Init의 부모 가상 Pop도 명시 좌표/void 경계다. BFS의 프로파일링 시계는 0 입력으로 대체한다.

표면0 반환과 기존 경로 보존, flags1의 직선 검색, 다른 풀의 어댑터 거부, 표면 alias 및 실제 Kernel 이동을 포함한 집중 검사 **5개**가 통과했다. 최종 전체 검증 결과와 현재 SHA는 `cpppj/campaign-pathsearch.json`에 기록한다. 성공 로그 없이 완료 보고서를 만들 수 없도록 `tools/analyze_campaign_pathsearch.py`가 확인한다.

GameWorld 비표면 객체의 실제 풀 이관, Factory/썬 아비어리 후처리, 서버 Path 직렬화, 실제 사제 운반·의식·도착, 건설·전투·승패와 캠페인 1-1 완주는 남아 있다. 원본의 큐 읽기 넘침과 손상 포인터는 정상 입력의 호스트 계약에 포함하지 않는다. 전체화면 DirectDraw는 이번 범위에서 변경하지 않았다.
