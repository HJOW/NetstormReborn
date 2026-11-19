# Client 다중 줄 Gump

WB04-E1041은 원본 ScrollingTextGump와 Widget·스타일 파서를 같은 Client Gump, FontSet, StyleTextBuffer, Markup 및 Slider 생성기에 조립했다. 생성·가상 부착·입력·방송·삭제와 현재 픽셀 클립 안의 다중 줄 글자·그림 출력이 같은 저장소를 사용한다. 버퍼 토큰 예약과 재진입 안전한 글자 표면 대여도 추가했다.

Tutorial1 실제 글꼴·fortGump 그림과 숨긴 실제 Slider로 다중 줄을 생성해 픽셀 출력 및 자식·조각 삭제를 확인했다. 관련 Node·대응표 검사 28건이 통과했다. 표시되는 Slider의 실제 출력과 한글 글꼴을 장착한 브라우저 출력, 장식 메뉴의 제목·본문 조립은 남았다. E1039 고정 Node 26 전체 회귀는 실행 중이다.
