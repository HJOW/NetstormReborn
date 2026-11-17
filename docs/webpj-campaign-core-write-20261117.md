# 현재 Player Core와 CoreData 결선 (WB04-E635)

기준선 `225553df`의 Session4447..4482·4488..4509, 합계58줄을 같은
실제 Session 기반에 연결했다. 해당 .cpp/.h는 기준선 뒤 변경0이다.
편집기 전용 WriteExistingEditorCoreData4483..4487은 만들지 않았다.

현재 Core는 같은 ClientCoreDataGlobals의 실제 포인터를 호출 위치마다
조회한다. 같은 기술 부모/ContainedFinder·현재 Pool/TYPE·다이스 역할과
가상80에 연결하며 Core 사본이나 대용 Player를 만들지 않는다. 가상
객체는 같은50BYTE Pool의 주소/물리를 검사한다. Native 장기 분기와
전체 설치된 미션의 가상 분배 재대조는4D-8·4D-10에 남는다.

CoreData는 같은9Template 헤더·Create 반환 노드/생성 수명을 보관한다.
같은 실제 Player 배열, Client가 제공한 fortCoreSeed DWORD와 게임 난수,
일반 writer·SP 인라인 writer를 연결한다. 서비스에 Client seed가 없으면
Session fallback seed로 대신하지 않고 첫10BYTE 쓰기 뒤 선택 위치에서
명시 실패한다. post/editor 자연 반환은 이 하위 장치를 요구하지 않는다.

새 실제 owner 검사4건은 첫 실행부터 통과했다. Player2의 같은 현재
포인터/이웃 보존, Pool/RNG 불변, 실제 CoreData276BYTE의10BYTE/seed2BYTE/
세88BYTE SP 기록 복호, 같은 Client seed 상위 WORD와 게임 난수1536회
소비를 확인했다. manager 진단은 post보다 먼저이며 현재 Core 포인터
누락은 Create 뒤 일반 쓰기 앞, Client seed 누락은10BYTE 쓰기 뒤다.
모든 검사에서 loaded는false를 유지하고 반납 뒤 두 API도 거부한다.

추가 검토에서 SidPool.bytes()가 독립 사본을 반환한다는 점을 확인했다.
가상80의 물리 동일성 검사가 매번 다른 사본을 비교하는 결선 오류였다.
실제 다이스165/원본 L프레임·실제 RawTempleRitual.count와 가상80 분배를
연결한 새 검사는 먼저 같은 물리 범위 오류로 실패했다. physical(0)의
같은 할당으로 전체 원시 뷰를 빌리도록 고친 뒤5검사가 모두 통과했다.
같은 다이스의 실제 count3이 현재 Core 하한2에 반영되고 Pool은 그대로다.
의식/포획/희생은 실행하지 않았다. 첫 전체 검사는 이 추가 교정을 위해
중단했으며 최종 전체 결과와 구별한다.

초안 probe는 미선택 Core 포인터의 정상 물리 거부, 미설치 Template의
원문1119 진단을 먼저 관측했다. 공개 select/install API로 명시 부품
입력을 준비한 뒤 CoreData276BYTE 경로가 통과했다. 이를 제품 오류로
바꾸거나 loaded=true로 우회하지 않았다. 정식4검사의 기대값/제품
교정은 위 물리 뷰 변경 외에는 없다. ClientFortCoreSeedTests의 소유권/늦은 장치 계약을 참고했으며
실제 Client 자체의 수명 비교는4D-7 뒤다.

이 단위는4D-1b의 Core58줄 완료이며 전체 snapshot 부모71줄과 외곽은
아직 남았다. 실제 전체 Session Load/Start·전체 snapshot/저장 재로드
성공을 뜻하지 않는다. E633·E634와 기존 player-core-refresh 원본 관측은
그 소비 검사로 재대조하며 새 결선 검사를 직접 fixture 소비자로 등록하지
않는다. 기존 관련 manifest 항목에 모듈/근거/범위를 갱신하고 신규 fixture는0이다.
보호 관측/원본과 공통 인계·cpppj는 수정하지 않았고 게임 창도 실행하지 않았다.

교정 뒤 신규5건·저장 자식/현재 Core 원본 소비·메타데이터 집중32건이
통과했다. 최종 Node v22.23.3·v26.13.0 전체는 각각6,014건 통과,
실패·취소·건너뜀0이다. v24는 사용자 지시대로 생략하고 호환을 가정했다.
대응표1,160행 오류0·manifest823항목(신규0)·`git diff --check`를 확인했다.
최종 전체 로그는 Git 제외 루트의 `extracted-webpj-e635-node22-final.log`·
`extracted-webpj-e635-node26-final.log`이며 앞선 중단 로그와 구별한다.

사용자의 현재 작업 마무리/일시 중지 요청에 따라 이 단위까지만 커밋한다.
재개 지점은4D-1b의 전체 snapshot 부모71줄이다. 검토용 로컬 초안은
Git 제외 `extracted/raw-campaign-session-snapshot-e636.mjs`이며 정식
구현으로 세지 않는다. 예전 `extracted/prepare-webpj-e635.mjs`는 교정 전
물리 뷰 코드를 다시 생성하므로 실행하지 않는다. 정식 현재 Core는
`webpj/www/core/raw-campaign-core-write.js`와 실제 다이스 회귀 검사가 기준이다.
