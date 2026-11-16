# Session 송신 합성과 다리·로딩 깊이 후단 (WB04-E618)

기준선 `225553df`의 생성자1815..1824와 Dais/Construction/AviaryTransmission
세 합성 함수를 `RawCampaignSessionTransmission`에 연결했다.
`RawCampaignSession.cpp`·`RawGameAuxState.h`는 기준선 뒤 변경0이다.
실제 Runtime 게시1825는 다음 작업이며 전체 Session/Load 완료로 세지 않는다.

## 구현

- 실제 자산→전투→건설→Aviary/Timer/비행→Dais 순서로 함수를 감싼다.
  각 fallback 함수 표는 C++ 값 인수처럼 복사하고 같은 Impl의 마지막 writer 표식은
  여러 반환 transmission이 공유한다. writer 실패 전에도 파생 선택을 갱신한다.
- Dais/건설은 같은 현재 Gameplay GameAux의 공통2048BYTE 버퍼와 헤더를 읽는다.
  `DaisProcessHeader`00532690/91=6a/1·`ConstructionHeader`005325b8/b9=37/1을
  장기 owner에 추가했다. 일반 reset은 같은2BYTE를 유지하고 새 owner는 독립이다.
  Dais는 protocol bit7을 지우지 않는 기존 writer를 그대로 쓴다.
- 독립 typed 타이머222BYTE와 네 개별64BYTE 버퍼를 보관한다.
  Native이면 원문처럼 생성 순간 공통 버퍼를 선택하고 현재 실제 serialize/broadcast
  장치를 요구한다. Dais/건설은 Native에서도 직접 writer를 유지한다.
  미설치 장치를 typed service로 우회하지 않는다.
- 상태 타이머의 선언 길이22는 실제 활성 Player/peer가 있는 송신에서만
  기존 finalize observer로 확정한다. 작성/packet/송신은 같은 선택 버퍼를 쓴다.
- walkerFall은 현재 사제 부모, 다리 제거는 파편 Variant2, 낙하 소리는 현재
  SoundList의 bridgeFall.wav와 원문 위치/인자5개를 사용한다.
  로딩 깊이는 같은 객체/Carrier 상태와 사제/자산에 쓰고 현재 daisState가 있으면
  같은 의식 준비를 실행한다. 다른 다리 효과는 명시 오류다.
- owner의 종료는 실제 Host를 비운 뒤 이 송신 대여를 닫는다.

## 검증

신규 Session7건과 현재 헤더1건을 포함한 집중18건이 통과했다.
기존 전체 조립에서 실제 Dais/건설 writer, 같은 버퍼와 공유 선택,
일반/강제 작성·상위 protocol bit·현재 송신 서비스 교체·fallback 함수 캡처,
Native broadcast 관문·미설치 장치 거부·타이머 현재 peer/길이 확정,
낙하/파편/소리·깊이 순서와 선행/반복/반납 관문을 확인했다.
두 헤더4BYTE는 보호 원본10.78 PE를 읽어 직접 대조했다. Node v22.23.3·v26.13.0 전체는 각각5,920건 통과했고 실패·취소·건너뜀0·종료0이다(--test-concurrency=4). v24는 기존 지시로 생략·호환 가정이다. 대응표 오류0·구문/UTF-8·git diff --check 통과다.

최초 Session 집중은6/7 통과였다. 마지막 검사 장면이 부분 sound 표를
기본 표에 합치지 않고 덮어써 실제 SoundProcess 필수 장치를 빠뜨렸다.
검사 owner에서 기본 장치와 명시한 장치를 합친 뒤18/18 통과했다.
제품의 SoundProcess 검증은 유지했다. 최초 실패는 게임 규칙 실패가 아니다.

공중3종은 기존 코드/정적 조립만 옮겼고 새 전용 생성/생산/우클릭/행동을
실행하거나 검사하지 않았다. Native 장기 소유자 설치·실제 Kernel/Load·
포획/희생·특정 적 공격·승리 재현·청취·원본 게임 실행은 이번 범위 밖이다.
Native 관문 검사는 명시한 경계 장치의 분배 계약이며 실제 장기 설치 완료가 아니다.

기존 `original-pe-data` manifest806항목 중1항목의 범위/계산 정보를 갱신했다.
새 fixture0이며 원본/기존 fixture를 변경하지 않았다. 로그 접두는
`extracted/webpj-e618-`다. 다음은1825 Runtime 게시와 나머지 생성자 모듈 조립이다.
