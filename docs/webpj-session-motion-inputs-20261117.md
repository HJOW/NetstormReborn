# Session 기준점·경로 효과·자원 줍기 (WB04-E641)

기준선 `225553df`의 `RawCampaignSession.cpp` **3806..3844, 39줄**을
기존 실제 owner에 설치했다. 다음 공개 경계는 3845의 BoardTarget과
CarrierChoice다. 전체 ConnectMotion·Load 완료로 세지 않는다.
대상 Session cpp/h와 RawCarrierPickup cpp/h의 기준선 뒤 변경은 없다.

실제 Pool·hash0·장부·추가 목록·ContainedFinder를 같은 기준점에 빌리고,
현재 방향별 동맹81DWORD·진단 서비스·공유 난수·SoundList·건설 준비와
134/105 도착 처리기를 연결했다. 경로 함수 표는 C++ 값형 함수 묶음처럼
중첩 표만 복사한다. 현재 owner·후속 의식/이동 본체는 호출 때 조회한다.
의식·후보/이동이 아직 없으면 선택한 자리에서 명시 오류를 낸다.

일반 자원 줍기는 기존 RawCarrierPickup·ContainedCreate/Host·입력 큐·
World 삭제와 같은 너겟 보충 부모를 사용한다. Session이 무조건 켜는
NB18 때문에 RawCarrierPickup의 기존 3.6③도 이 단위에서 앞당겼다.
기준선의 `refuseHeldPriest` 기본 false와 요청/수신 공통 거부 조건을
옮겼으며 실제 Session에서만 true다. 사제 포획/희생 재현은 하지 않았다.
기존 원본 관측 소비 검사는 기본 false로 계속 대조한다.

신규 검사는 실제 생성자/건설/도착/World/Host를 조립하고 외부 음향·화면·
입력 큐만 제공한다. 설치 순서·반복/반납·물리/난수 불변, 방향 동맹의
현재 읽기와 함수 표 값형 복사, 현재 진단 복귀·미연결, 공유 난수 소비와
위치 소리 토큰, 실제 마나 도착 삭제, 공간 genus 조회와 서버 너겟의
내용물 변환·큐·SP 보존을 확인한다. 경로/실제 내부 전송은 후속이다.

최초 5건 중 1건은 Form writer의 client SID 진단으로 실패했다.
검사가 flags2로 client 영역 부모/너겟을 만들고 서버 수신을 호출한 입력
오류였다. 실제 서버 수신 장면에 맞게 서버 영역 할당(flags0)으로 고쳤고
제품 진단은 유지했다. 다음 실행에서 5/5 통과했다.

추가 정적 대조에서 웹 도착 getter의 `{sid, flag}` 반환과 기존 줍기
모듈이 받는 pair 배열 계약의 차이를 발견했다. 같은 Pool의 실제
RawCarrierArrival/RawPathCleanup을 조립해 후보 조회→배열 변환→9번 대상의
4f 패킷 전송을 기존 자원 검사에 추가했다. 변환 없는 callback은 요청을
누락하므로 제품 결선에서 두 칸 배열로 변환했다. 이 교정 뒤 신규5건이
통과했다. 최초 전체 실행은 교정 전이라 최종 근거로 쓰지 않고,
교정된 코드로 두 Node 버전의 전체 검사를 다시 수행했다.

하위 원본 대조는 기존 carrier-pickup(10.78 2,657행,
파일 SHA-256 `2245911a…441e`)·playeranchor(10.78 1,598행,
`2f21f734…192d`)·rawfinder(10.78 Find 584행, `f6720929…ad16`)·게임 난수
소비 검사를 그대로 사용한다. 다른 판본 행은 기존처럼 제외한다.
신규 조립 검사를 원본 관측의 직접 소비자로
등록하지 않는다. manifest의 관련 모듈/범위만 확장한다. 새 원본 실행이나
관측 자료는 없다. 관련 집중 검사는 11/11 통과했다.

최종 Node v22.23.3·v26.13.0 전체는 각각 **6,047/6,047 통과**,
실패·취소·건너뜀0이다(`--test-concurrency=4`). v24는 지시대로 생략하고
호환을 가정한다. 대응표1,160행 오류0·manifest823항목을 유지하며
공백 오류0이다. 검사 로그는 Git 제외 `extracted/webpj-e641-final22.log`·
`webpj-e641-final26.log`에 보관했다.

실제 loaded=false다. 다음은 **4D-2b 3845..3929**의 BoardTarget·
CarrierChoice/Target/Escape·PathRetarget·Mining/Transfer·이동 본체다.
같은 함수 표를 이어 사용하며, 3931 이후 Move/BridgeCarriers의 실제
설치는 원문상 4D-2c에 있어 그 구간에서 함께 처리한다.
