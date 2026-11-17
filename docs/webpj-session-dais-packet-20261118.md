# WB04-E721: Session의 6ah DaisProcess 수신

Carrier 등록 앞단 다음 6ah의 typed 수신기를 연결했다. 현재 Form이 비어 있으면 원본 mask 진단과 실제 decoder를 거쳐 같은 Kernel/Process Host에 지정 Form과 부모로 새 `DaisProcess`를 부착한다. 기존 Form은 현재 PID가 가리키는 같은 Process의 선택 필드만 갱신한다. 실패 시 Kernel에 아직 넘기지 않은 새 Process만 소멸하며, 이미 등록된 객체의 앞선 효과는 보존한다.

공유 `GameAuxState`에 원본 `00541158`의 DaisProcess TYPE 58을 추가했다. 자산의 Dais TYPE 165와 다른 장기 DWORD이며 현재 값을 부착 시 읽는다. 보호 PE의 초기값과 웹 TYPE 이름 전체를 다시 대조했다.

일반 Form 부모만을 사용한 프로세스 내부 검사에서 신규 부착과 같은 PID의 기존 갱신, 64비트 기한 비트 보존을 확인했다. 사제 행동·포획·희생·자연 승리는 실행하지 않았다. 기존 decoder·공유 전역·실제 Session 집중 검사 31건이 통과했으며 실제 게임이나 브라우저 창은 열지 않았다.

Carrier의 다음 Squid·Gun·Aviary·Flyer 및 기타 수신과 개발자 해석기, Native 등록, 서버 후반·응답 왕복은 남는다. 공중 3종은 기존 사용자 제한에 따라 정적 이식만 진행하고 전용 생성·행동 검사는 추가하지 않는다.
