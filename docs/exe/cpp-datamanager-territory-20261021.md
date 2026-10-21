# 같은 DataManager의 Territory 선택과 원시 섹션 연결 — 단위264

시작 커밋은 `b68dd03f`이며 현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 같다. 직전 단위263에 이어 작업했다. 원본/CD/AGENTS/dotnetpj 변경, 새 디컴파일·원본 관측·게임·C++ 검사 실행은 없다.

원본 `004bd660`은 같은 Template manager에서 `0044ceb0("Territory",0)`으로 읽기 섹션을 선택하고 현재 MemStream의120BYTE를 읽는다. 기존 실제 월드 로더 연결은 파서의 `fort.territory` 편의 사본과 별도 커서를 반환하며 manager.current 선택을 생략했다.

이번에는 `RawCampaignSession`의 월드 설치 공급자를 `CampaignTemplateStorage.SelectTerritory`에 연결했다. 이 선택은 같은 manager 목록과 원본 검색/선택 몸체를 사용하고, 읽기 허용 인자0으로 빈 섹션을 진단한 뒤 해당 노드의 pos를0으로 만든다. 실제 Territory 커서는 이제 `fort.sections[Territory]`의 같은 원시 버퍼를 빌린다. 파서의 편의 사본은 파일 해석용으로 유지하며 실제 이 getter는 원시 버퍼를 사용한다.

State와 Territory의 선택은 같은 manager.current를 바꾸며 각 섹션의 커서는 구별한다. 빌린 헤더도 같은 manager와 두 cursor를 사용한다. 원래 데이터 버퍼를 복제하지 않고 선택/읽기와 최종 반납이 같은 원시 섹션을 사용하도록 연결했다. 호스트 span/벡터 어댑터이며 전체 DataManager/MemStream 생성·파일 저장·오류 복원은 아니다.

근거는 현재 PC 정밀 소스의 `004bd660`, 직전 [단위263](cpp-datamanager-selection-20261021.md)의 검색/선택 원문864행이다. 같은864행을 보존해 재사용하며 새로운 관측 수로 합산하지 않는다. 제단·희생 UI도 정적으로 확인했으나 캠페인에서 보상창을 생략하는 현재 관문이 이미 연결되어 있었으므로 수정하지 않았다. 과거 인수인계의 미연결 기록을 현재 결함으로 재등록하지 않는다.

실제 `thewarbegins.fort`를 사용해 빌린 헤더의 Territory 읽기 선택→State 쓰기 선택→Territory 재선택, 각 커서 유지/초기화와 원시 버퍼 포인터/변경 공유/반납을 검사하는 C++ 사례1개를 추가·컴파일했다. **사용자 실행 이월 지시대로 검사·CTest·게임은 실행하지 않았다.** 단위263의 기존3검사도 이번 전체 검사 바이너리에 컴파일했으나 새 통과로 세지 않는다.

최종 Release는 **종료0·경고0·오류0**이다. 중간 빌드 뒤 원시 버퍼 연결과 주석을 모두 반영한 최종 로그는 `extracted/cpp-datamanager-territory-20261021-build3.log`이며, 원래 stdout과 종료 코드는 같은 접두의 `.raw.log`·`-result.json`에 보존했다. 새 보고서 `cpppj/datamanager-territory-session.json`은 현재 소스·속성·원본 자료·빌드/종료 기록·세 최종 바이너리의 해시를 기록한다. `python tools/cpp_datamanager_territory_audit.py`가 원문 보존/실제 소스 연결/현재 바이트를 확인한다. 과거 session 보고서의 소스/빌드 핀은 변경하지 않았다.

이번 작업 후에도 전체 DataManager 생성·섹션 추가/교체·파일 읽기/저장·빈 State 초기화·전체 State7/전투 정리004b2df0·Fort 선택/Guide 공장/가상 소멸은 남는다. 전체11미션 자연 완주·전략AI·지형/포획/희생/대피·외관/음향/동작 동일성과 통합 실행 검증도 미완료다. 목표는 계속 active다. 인수인계 뒤 **20261021-28 CP**로 로컬 커밋하며 push하지 않는다. 마지막 정밀 디컴파일 PC는 계속 **VM-W11-CODEX / 2026-10-21**이다.
