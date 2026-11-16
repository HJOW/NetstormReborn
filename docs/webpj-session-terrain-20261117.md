# Session World 게시·지형/소유자 후처리 조립 (WB04-E619)

기준선 `225553df`의 `RawCampaignSession.cpp:1825..1865`를
`RawCampaignSessionTerrain`에 연결했다. 생성자 앞 구간에서 완성한 훅을
실제 `RawSurfaceWorld.bindRuntime`에 게시한 다음 같은 풀의 일반 배치·지형·
Fort 소유자 후처리를 만든다. 외곽 Session/Client 공개·Template 설치·Load는
아직 완료되지 않았다. 대응표의 Session 상태는 partial, 앱 연결 session은0이다.

## 구현과 경계

- World의 실제 Form/Regular/Process 송신 분배와 초기 loadingDepth 효과를
  게시한다. 이후 같은 World의 삭제 패킷이 실제 송신 소비자에 전달된다.
- 같은 metadata의 기하/프레임·현재 hash0·추가 목록·장부·내용물 검색으로
  `RawCanonPlacementPipeline`을 만든다. hash0은 사본인 entries가 아니라
  현재 view를 빌린다. 이 단계의 기본 배치와 후설치 gameplay 배치는 구별한다.
- `RawNormalTerrainWorldBinding`은 같은 청크·섬·HP/Carrier·게임 난수·CRT와
  장기 terrainStamp 캐시를 빌린다. puzzlePiece/CRT도 현재 칸의 참조다.
- 지형 root의 군도·확장·최소/최대/백분율은 같은 GameAux 참조이며 군집과
  동일한 상자 정체성을 갖는다. layout/spacing·Checking과 Player base는
  매번 현재 값을 읽는다. 선택된 Player 배치는 실제 건설 부모로 간다.
  성장 진단 함수는 원문처럼 생성 시 캡처하고 백분율 로그는 현재 서비스를 읽는다.
- 실제 단일 배치와 `RawFortFinishOwner`를 같은 거주 구역 목록/Player 기록에
  연결했다. 미션 chaff는 원문처럼 범위 조회 전 반환한다. 실제 전체 gameplay
  배치가 아직 설치되지 않은 상태에서 그 분기를 선택하면 명시 오류를 유지한다.
- Host를 비운 뒤 지형의 현재 효과 대여를 닫는다. 반복/반납/늦은 World
  게시 관문을 유지하며 전체 Session을 미리 성공 상태로 만들지 않는다.

기준선 뒤 `RawCampaignSession.cpp`·`RawNormalTerrainWorldBinding.cpp`·
`RawPlayerGeyserBounds.cpp` 변경은0이다. cpppj·보호 원본·기존 fixture는
변경하지 않았다. Native 장기 저장소/수신기 설치는 기존 후속 경계이며 이번
구간의 독립 typed World 조립을 실제 Native 실행 완료로 세지 않는다.
공중3종은 기존 조립만 사용했고 새 전용 생성/생산/행동 검사를 추가하지 않았다.

## 최초 실패와 교정

첫 실행은 검사 import에서 잘못 가져온 패턴 표 때문에 중단됐다.
올바른 자산 모듈로 고친 뒤7건 중5건 통과,2건 실패였다. 실제 Session에서
기존 Canon 진단을 숫자 getter로 전달했지만 Decoder는 checking()을 요구했고,
Player 배치 폭/높이는 FrameCodeTable 자체를 코드열처럼 사용했다.
Session 진단을 현재 unsigned DWORD 함수로 연결하고 Player 범위가 현재
FrameCodeTable.codes 또는 기존 독립 코드열을 그대로 빌리도록 교정했다.

다음 집중은33건 중31건 통과였다. 진단 함수의 signed/unsigned 표시를
원문 DWORD로 맞췄고, 검사 순회를 원본68개 중 record 하위6비트로 선택 가능한
앞64개로 고쳤다. 최종 집중33건 모두 통과했다. 기존 진단을 생략하거나
원본 기대값/fixture를 바꾸지 않았다.

## 검증

신규7건과 관련26건(전체33건)이 통과했다. 실제 선행 조립, 동일 풀/난수 보존,
World 송신·깊이 효과, 현재 전역/Player 재선택, 로그/진단 캡처,64패턴×4회전의
현재 자산 Decoder, chaff/invalidRegion/미설치 전체 배치 관문을 확인했다.
자연 완주·포획/희생·특정 적 공격·승리·청취·실제 미션 실행은 검사하지 않았다.

Player 범위의 기존 원본 x86 대조는439데이터행(머리글 포함440줄), SHA-256
`9803c4376774b817ff1cd67cdc75f6cc430c29115d0ecb6bea199013b576020c`를
그대로 사용했다. 새 fixture0, manifest806항목을 유지한다.
Node v22.23.3·v26.13.0 전체는 각각5,927건 통과, 실패·취소·건너뜀0·종료0이다
(`--test-concurrency=4`, v24는 기존 지시로 생략·호환 가정).
대응표 오류0·구문/UTF-8·git diff --check 통과다.
로그 접두는 `extracted/webpj-e619-`이며 실행한 전체 검사의 출력은 보존한다.

다음은 생성자1866..1881의 Template 설치·MakeRuntime·승패/AI 부모 구성,
이어서 외곽 공개와 같은 Template의 TypeNames/Objects/Contents·Load다.
