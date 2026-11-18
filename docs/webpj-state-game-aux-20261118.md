# WB04-E811 State GameAux 전역

기준선 `225553df`의 GameAux에서 누락된 주문 기한 `005950d0`(double), 수입 준비 `00595124`(DWORD)와 중앙 좌표 `00568afc/00568b00`(float32 두 칸)를 장기 소유자에 추가했다. 일반 월드 준비가 이 값들을 재설정하지 않는다. 중앙 좌표는 같은 `Float32Array` 물리를 빌려 State DWORD 접근에서 NaN 페이로드도 그대로 유지한다. 기준선 이후 `RawGameAuxState.h` 변경은 없다.

State 정리의 GameAux DWORD 13개와 청크 mode, 중앙 좌표를 실제 같은 소유자에 연결했다. signed 자원 DWORD와 bool 쓰기, 읽기 전용 필드를 구별한다. 새 필드의 보호 PE 초기값과 별도 수명, 중앙 좌표 비트·자원 부호를 검사했다. Audio·Template·TYPE·Pool, 손실 좌표 float 및 나머지 WORD/double 접근과 실제 State 부모 연결은 남았다. BattleResourceFrame의 실제 프레임 수명은 여전히 연결되지 않았다.

검증: GameAux 집중 검사 12건과 State 전역 집중 검사 통과, 대응표 1,160행 오류 0. Node v26.13.0과 v22.23.3 전체 검사 각각 6,301건 통과(실패·취소·건너뜀 0); 두 버전은 순차 실행했다.
