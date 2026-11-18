# WB04-E808 Client State Runtime 반납

기준선 `225553df`의 `ClientStateRuntimeRetirement`를 Client 장기 Gameplay·표면·Player·Kernel·Core 소유자 입력에 이식했다. 생성과 각 호출에서 표면 세대·현재 Session/Pool/Player·대여 상태와 활성 콜백을 확인한다. AI 1~8과 Player 링크를 실제 Session에서 정리해 증빙을 반환하고, Kernel Clear 증빙을 한 번만 발급한다. Core 선택 해제와 자식 증빙을 확인한 뒤 Process 결선을 반납하고 외곽 소유자의 실제 Session 반납을 확인한다. 첫 메뉴의 Session 없는 AI·Player·Kernel 경로도 기존 실제 물리를 사용한다. 기준선 이후 해당 cpppj 파일 변경은 없다.

Session에 같은 Player 물리와 외부 전송/Runtime 활성 콜백 조회를 공개했다. TheWarBegins 실제 Session에서 Runtime 고정 중 재진입 거부, Player·AI·Kernel 증빙 누락과 중복 거부, 소유자 반납 실패와 재시도, 실제 정상 반납 및 Session 없는 brain 관문을 확인했다. 처음 집중 시험은 Player와 Core 소유자를 결합하지 않아 거부됐으며 실제 `bindCoreData`를 설치한 뒤 통과했다. 전체 Client·State 부모 수명은 후속이다.

검증: 실제 Session 집중 검사 통과, 대응표 1,160행 오류 0, Node v26.13.0 전체 6,300건 통과(실패·취소·건너뜀 0). E803 Node v22 전체 검사 뒤 v26 회귀 5/8회다.
