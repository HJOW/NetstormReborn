# WB04-E739 의식·기도·Meltdown 수신 등록

현재 Session 분배기에 `6bh` 의식 선택, `6dh` 사제 기도, `75h` Meltdown 효과, `76h` Meltdown 요청을 등록했다. 의식 선택은 현재 Pool·내용물 생성기·월드 삭제기를 사용하고, 나머지는 같은 Session에 설치된 처리기로 전달한다. 원본 고정·가변 길이와 빈 관찰기 계약을 유지한다.

근거는 기준선 `225553df`의 `RawCampaignSession.cpp`, `RawRitualChoice.cpp`, `RawPriestPrayer.cpp`, `RawMeltdown.cpp`이며 기준선 이후 이 파일들의 변경은 없다. 기존 단위에서 기도 fixture 1,126행과 Meltdown fixture 442행을 대조했다. 의식 선택은 원본 fixture 없이 소스 계약으로 검사했으며 이번에 새 fixture는 추가하지 않았다.

Tutorial 1 Session 분배기에서 반환된 슬롯의 의식 선택·기도와 죽은 슬롯의 Meltdown 요청 관문을 확인했다. Meltdown 효과는 등록 metadata만 확인했다. 집중 검사 27건이 통과했으며 실제 의식·포획·희생·공격 재현은 실행하지 않았다.

Node v26.13.0 전체 회귀 6,216건을 통과했다. 대응표 오류는 0건이며 Node v22 전체 재검사 주기는 1/8회다.

`31h` 개발자 명령 해석기와 Native 분기, 내부 로그인 서버 후반·응답 왕복, 행 base 뒤 변경 판정 및 앱 미션 연결은 남아 있다.
