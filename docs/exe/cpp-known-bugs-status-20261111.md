# 10차 알려진 버그 NB01~NB34 판정 현황 (제품 1285~, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음). 원문 증상은 [등록 목록](cpp-known-bugs-plan-20261109.md)을 따른다. 판정은 `수정 완료 / 현행에 이미 반영 / 해당 없음 / 미완료(조사 중)`이며, 허용 검증 결과와 사용자 확인 잔여를 따로 적는다. 포획/희생·특정 공격·승리 재현과 MP 실행은 하지 않는다.

| ID | 판정 | 근거·조치 | 검증 |
| --- | --- | --- | --- |
| NB07 | 현행에 이미 반영 | `Create New Map`에 기존 이름을 넣으면 `Unique Name Required — A Fortress with that name already exists...` 안내가 뜨고 기존 fort를 열지 않는다. | 실제 화면(`extracted/cpp-resume-1278/nb07/`) |
| NB08 | **수정 완료** | 세 겹의 원인을 실제 실행으로 찾아 교정했다. (1) `FileSpecDirectoryHooks`의 `root/path`가 `\D\guide*.english`처럼 루트 디렉터리만 있는 패턴에서 게임 폴더를 버리고 드라이브 루트(`H:\D\...`)를 검색해 디스크의 `Guide*.english`(사용자 캠페인)가 한 건도 목록에 오르지 않았다(`root/path.relative_path()`). (2) `BaseFileSystem::MatchFinder`의 결과 경로 접두 비교도 같은 이유로 버려졌다(같은 교정). (3) 원문 PE `4c8c12..36`의 `%` 인라인 목록 삽입이 합친 문자열 전체가 아니라 `replacement` 길이만 복사해 본문 뒤쪽의 목록이 잘렸다. `HtmlWindowMetadataHooks::repairInlineList`(제품에서만 켬, 기본 꺼짐)로 전체와 NUL을 쓰게 했다. | 실제 화면: `Campaign → User Made Campaigns`에 `Legendary Missions Of Ass` 등 목록과 스크롤바 표시 |
| NB24 | **수정 완료** | 긴 캠페인(Tell 파일) 제목이 80BYTE 지역 버퍼를 넘으면 원문처럼 `PhysicalLimitError`로 종료했다. `FitDisplayLimit`로 마지막 UTF-8 문자부터 줄여 표시한다(NB08 교정으로 목록이 열리며 즉시 노출된 같은 증상). | 실제 데이터에 80BYTE 이상 제목(`GuideHades` 86자, `GuideKQ` 102자: 아이콘 마크업 포함)이 있어 NB08 교정 직후 처음에는 `Tell 파일 목록의 원본 지역 NUL 범위 초과`로 종료했고, 교정 뒤 목록이 정상 표시되며 종료·로그 없음 |

(나머지 행은 조사 진행에 따라 이 표에 추가한다.)
