# WB04-E789 일반 종료의 Process 정리 오류

cpppj `RawCampaignSession.cpp`의 Impl 소멸자는 자기 Process Host 정리 실패를 진단하고 나머지 모듈 반납을 계속한다. 웹에서도 일반 종료의 첫 Host 정리 오류를 console에 기록하고 이후 모듈·World·Client 대여 정리를 계속하도록 했다. 명시적 State Clear의 실패 및 증빙 발급 관문은 바꾸지 않았다.

실제 Session의 Regular Process 소멸기가 첫 호출에서만 오류를 던지도록 한 검사에서 수정 전에는 예외가 종료 밖으로 전파됐다. 수정 후 원인이 기록되고 World 최종 정리에서 재시도해 Kernel과 Client 대여가 반납됐다. E788의 Player 링크 선행 정리와 E786의 State 증빙 보존 검사도 통과했다. 계속 실패하는 외부 장치의 전체 복구와 실제 Client/앱 종료는 별도 검증이 남는다.

Node v26.13.0 전체 회귀 6,292건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E787의 Node v22 전체 회귀 뒤 두 번째 v26 회귀다.
