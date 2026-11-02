# cpppj 단위721 — 원본 저장 맵의 개별 Graph 소속 대조

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이며 새 디컴파일은 없다. 사용자 재개 지시로720의 일시중지를 해제했다. 제품은721 / 11.01이다.

## 변경과 확인

`RawSurfaceWorld::Report`에 실제 표면의 Graph BYTE(`raw_surface_graph`)와 Graph 표의 앞 두 signed WORD(`raw_graph_record`)를 추가했다. 보고는 읽기 전용이며 flood/번호 재할당·게임 난수/프레임을 변경하지 않는다. 기존 행의 열과 의미를 유지한다. reserved WORD는 미정 바이트일 수 있어 보고·판정하지 않는다.

`tools/cpp_editor_graph_compare.py --require-membership`는 서로 다른 Pool의 SID/Graph 번호 대신 타입·상태·소유자·float32 좌표별 표면 다중집합으로 연결 성분을 비교한다. 각 성분의 실제 장부와 noGraph254의 개별 소속도 비교한다. 원본 배치 직후678/실제670 같은 차이를 정상화하지 않는다. 과거720 보고는 개별 소속이 없으므로 엄격 모드에서 통과하지 않으며, 기본 모드는 과거 합계 대조를 유지하고 `membership.observed=false`로 표시한다.

원본 실제 Edit에서 저장한 고정 n720 Fort를 제품721의 실제 Edit 메뉴에서 한 번 재적재했다. **표면674개 배치·정상 성분670개와 장부670/1·noGraph4개의 개별 소속이 모두 일치했다.** 같은 번호가 우연히2인 것은 판정 근거가 아니며, 도구 검사에서 번호7도 같은 성분으로 통과한다. 표면/noGraph 하나씩 교환해 합계670/4를 유지하면 실패하며, 누락·틀린 장부·표면 없는 활성 장부도 거부한다. 도구 집중5개 실패0이다.

## 그림 변형과 다음 수정

원본 자산의 실제 `isle.type` 317개 프레임 코드를 독립 Python 파서로 읽었다.720의468개·721의474개 frame 차이는 모두 방향 두 BYTE와 flags가 같고 variant BYTE만 다르다.720은206/674,721은200/674 frame 일치다. 난수 seed/99cache를 같게 맞추지 않았으므로 원본 그림 픽셀 동일성이나 차이의 모든 원인을 확정하지 않는다.

추가 정적 확인에서 원본 `004c04b0`의 `005c8be4`는 최초 한 번99개 CRT 값을 할당하는 전역인데, 현재 `RawCampaignSession::Impl::stamp`는 Session 소유다. 이 수명 차이는 다음722에서 Client 소유 캐시로 수정한다. 이 문제를720/721의 서로 다른 실행 사이 variant 차이의 확정 원인으로 쓰지 않는다.

## 검증과 제한

- Release 빌드 경고/오류0. EXE SHA-256 `23d1f60a595512de24053eeebece1d59e0e3ad00153ab79dba9cdd14f8c7b8ba`.
- 실제 게임1프로세스·250프레임·6.3135초·종료0·timeout없음. 같은 원본 분석 복사본을 사용했고 실행 자료/옵션 복원과 originals 전체 불변을 확인했다.
- 도구5개 통과. 최초 stderr는cp949였고UTF-8로 다시 실행해v2를 보존했다. 게임 재실행은 아니다.
- AGENTS·원본·CD·기존 fixture2640개 변경0. 신규 원본 fixture는 없다.
- 전체 C++/34/ZIP·물리 입력/PCM·새 원본 실행·MP/통신과 제외된 포획/희생·특정 적 공격/승리·공중3종 전용 검사는0이다.

다중 섬/편집 분할·병합, 배치 직후678/670, 전체 파일/시나리오 수명은 남는다. 전체 동일성과 자연 완주는 사용자 확인 대상이다. 원시는 `extracted/cpp-editor-graph-721/`, 요약은 [보고서](../../cpppj/editor-graph-membership-20261102.json)에 있다.
