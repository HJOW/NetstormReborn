# 공용 CoreData SP writer (WB04-E634)

4D-1b의 `WriteFortCoreDataBody`가 사용하는 RawFortSpWrite.cpp/h(71줄)를
기준선 `225553df`에서 이식했다. 해당 두 소스는 기준선 뒤 변경이 없다.
Session4504..4507에서 캠페인 공용 CoreData 자식으로 호출되므로
`exclude-edit`를 `port`로 교정했다. 편집기 UI/파일 저장은 추가하지 않았다.

caller의 float32를 원본 masked `_ftol`의 signed64 0방향 절삭으로
변환해 하위 DWORD를 사용한다. 비유한/범위 밖은 하위 DWORD0이다.
bit0..31마다 같은 Next(32)를16회 소비하며 해당 bit와 같은 난수도
소비 자체는 생략하지 않는다. 잡음은 해당 bit를 제외한 위치에만
OR한다. 출력은 앞8 BYTE·다음8 WORD·나머지16 DWORD, 합계88BYTE다.

각 폭의 용량 읽기 순서·signed 비교·진단 뒤 현재 pos/data·출력 뒤
pos/end 재조회를 보존한다. stream16BYTE와 출력 버퍼가 같은 실제
할당인 경우에도 사본으로 분리하지 않는다. 누락된 장치/물리는 실제
도달 위치에서만 거부하며 앞선 난수/부분 출력은 되돌리지 않는다.

cpppj `FortSpWrite_ReplaysIndependentNative198`을 같은 이름으로
옮겼다. 최초 x86 관측198행(정상178·명시 중단20)의 세256BYTE 물리,
게임 난수 상태·호출/같은bit 횟수·진단 순서·전체 trace를 대조했다.
단순 비변이 입력은 실제88BYTE 출력의 보호bit를 다시 읽어 원본
`_ftol` EAX와 비교한다. 정상 반환은 모두512회 난수를 소비했다.
최초 집중 검사1건은 통과했고 구현/기대값 교정은 없었다.
착수 대응표 검사는 새 모듈 복사와 등록 전 검사가 겹쳐 미등록1건을
보고했다. 규칙/대응 행/manifest 등록을 마친 뒤 재실행은 오류0이며
저장 자식·추적 목록·대응표 집중23건도 통과했다.

전체 Node v22.23.3·v26.13.0은 각각6,009건 통과, 실패·취소·건너뜀0이다.
v24는 사용자 지시대로 생략하고 호환을 가정했다. 로그는 Git 제외인
루트의 `extracted-webpj-e634-node22.log`·`extracted-webpj-e634-node26.log`다.
대응표 대상1,019단위 중port860/partial39/todo120, 제외141이며
manifest823항목(original-x86 784)을 확인했다. `git diff --check`도 통과했다.

원본 관측을 수정하거나 새 게임 창을 실행하지 않았다. 실제
Session의 CoreData/전체 snapshot 결선은 다음4D-1b 본체 작업이다.
E635 초안은 Git 제외 `extracted/raw-campaign-session-snapshot-e635.mjs`에
있으며 정식 구현/검증으로 세지 않는다. 앱 session0·실제 미션 플레이
불가 상태를 유지한다.
