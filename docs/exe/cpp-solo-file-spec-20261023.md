# 단위357: 단독 맵 목록의 현재 FileSpec 키와 캐시 수명

시작 HEAD는 `9f401229 / 20261023-20 CP`다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이며 기존 정밀 원문과 보호 PE의 직접 읽기를 사용했다. 새 디컴파일/기존 산출물 변경은 없다.

## 원본과 기존 차이

보호 10.78 PE의 `005327ab`는 `?fortSpec\0`, 실제 가변 키 버퍼는 `005327ac`부터9BYTE다. 포인터 전역 `005327e0`의 초기 DWORD는 `005327ac`다. 실제 파일의 pefile 읽기로 확인했으며 REA의 Ghidra 바이트 읽기 성공으로 기록하지 않는다.

`00460650`·`00460680`·`00460c40`은 각 PathSpec 직전에 이 현재 버퍼를 참조한다(정밀 원문78142/78155/78525). Tell의 `*SOLOLIST*`(159342)는 dirty1→같은 버퍼에 `SOLOSpec\0` 쓰기→Refresh2/행 작성→dirty1→`fortSpec\0` 복구다. 키 변경과 캐시 무효화는 별도 효과다.

기존 SoloMapCatalog는 파일 열거, 종류 분류용 읽기와 수락된 파일의 별도 메타 읽기를 모두 `fortSpec`으로 고정했다. 이대로는 SOLOLIST의 현재 namespace 전환을 전달할 수 없었다.

## 변경

- `SoloMapFileSpec`은 같은9BYTE의 키와 마지막 NUL을 보유한다. SelectSolo/SelectFort는 같은 할당을 덮어쓰고 Current가 반환하는 view의 주소를 바꾸지 않는다. Client가 이를 장기 소유하면 이후 목록/상태 소비자가 같은 현재 값을 빌릴 수 있다.
- `SoloMapCatalogEffects.fileSpecKey`는 각 PathSpec 직전에 현재 키를 반환한다. 파일 열거에서 한 번, 분류용 MapProbe.Load와 별도 메타용 MapProbe.Load에서 각각 조회한다. Refresh 시작의 키를 캡처하지 않는다.
- 기존 호출자가 getter를 생략하면 catalog가 소유하는 기본 fortSpec 저장소를 사용한다. 고정 주소 Impl의 수명보다 짧은 임시 문자열을 캡처하지 않는다.
- 키 저장소는 dirty를 변경하지 않는다. 기존 RawSoloMapList의 OS 열거 순서·종류 필터·50개 상한·캐시·행 형식 및 명시 Invalidate를 그대로 사용한다.

## 검증

최종 Release 빌드1은 **종료0·경고0·오류0**이다. 관련 **10개·실패check0**이다(Catalog8/기존 List2). 새2검사는 빌드 검사 디렉터리 아래 두 실제 폴더에 서로 다른 State의 같은 이름 Fort를 생성한다. 보호 원문에는 쓰지 않는다.

첫 검사는 같은 버퍼 주소/NUL, fortSpec/SOLOSpec의 현재 분류, 키만 바꿨을 때 기존 캐시 유지, 명시 Invalidate 뒤 새 메타/손상 flags 적용을 확인했다. 열거·분류·메타의 세 조회 키도 모두 확인했다. 두 번째 검사는 분류 파일을 이미 읽은 뒤 typeCount 경계에서 키만 변경한다. 조회 키가 `fortSpec,fortSpec,SOLOSpec`이고 분류는 앞선 파일의 종류2, 메타는 뒤 파일의 손상 flags2를 사용함을 확인했다. 이 효과는 재진입/현재 자료 재조회를 검증하는 명시 입력이며 실제 원본 GUI에서 자연 발생한 사례라는 뜻은 아니다.

기존 실제 보호 Fort 목록/OS 열거/TAFF·보조 경로 읽기/빈·짧은 State/작성자/파일 소실/전체 점 제거 및 원본 캐시·행 검사도 통과했다. 기존 raw 목록 알고리즘과 fixture는 바꾸지 않았다. 로그는 `extracted/cpp-solo-file-spec-20261023-*`에 있다.

최종 전체 CTest1은 **2342개·실패check0·종료0/183.35초**다. LastTest.log의 `2342 test(s), 0 failed check(s)`로 내부 집계를 확인했다. 변경 C++/이 기록의 UTF-8 해석·추가 주석/보호 경로 미변경 및 `git diff --check`도 확인했다. 제품/검사 C++는 최종 빌드1/관련·전체 검사 뒤 바꾸지 않았다. 인수인계와 이 기록을 마감하여 **20261023-21 CP**로 로컬 커밋한다.

## 다음 연결

실제 UberGump에 SOLOLIST/FORTLIST의 행 조립·020000ca 선택 명령·열 분할·dirty/키 복구를 연결해야 한다. c9/ca의 모드/pending 이름/next State 예약과 실제 Editor Session은 아직 남아 있다. 키 getter 모듈을 실제 Edit 사용자 진입 완료로 계산하지 않는다. [직전 단위의 Edit 상태 전환 근거](cpp-fort-selection-order-20261023.md)도 이어서 사용한다.

전체 Early Missions·캠페인·데모3개의 사용자 완주, Edit/커스텀 시나리오·다른 메뉴/GUI·음향 동일성 및 배포 압축 목표는 계속 활성이다. 원본/CD·AGENTS.md·다른 프로젝트 수정이나 원본 게임 실행, 원격 push는 없다.
