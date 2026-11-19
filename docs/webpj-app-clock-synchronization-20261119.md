# 앱의 Session 시간 동기화 수명 (WB04-E947)

E821의 게임 시계 보정은 현재 앱 `Netstorm._initializeClientRuntime`에서 같은 `ClientLocalStateGlobals.gameTicks()` 상자에 연결되어 있다. 실제 Session Login 앞단은 같은 전역의 동기화 상태·70표본을 대여하고 패킷 프레임에서 Poll을 호출한다. State 정리 역시 같은 상자를 초기화한다.

기존 앱 State7·State8의 Tutorial1 `BridgeTheGap` 로그인 통합 검사에 고정 시각을 주입했다. Ready 이후 같은 Client 표본에 왕복 250ms인 관측을 넣어 실제 앱 프레임의 Session 패킷 Pump가 보정 125ms를 산출함을 확인했다. 다음 앱 프레임의 게임 시각에는 보정이 적용되고 벽시각에는 적용되지 않는다. 같은 State 정리로 보정을 초기화한 다음 프레임은 두 시각이 다시 일치한다. 실제 원격 접속이나 소켓 지연 FIFO를 사용하지 않았고, Native 장기 수신기 baseline과 월드 교체 수명은 아직 남는다.

첫 실행에서는 이진 부동소수점 뺄셈 결과가 0.12499999999999997이어서 0.125와의 정확한 비교에 실패했다. 산출된 DWORD 보정은 정확히 125였으며, 초 단위 차이는 1e-12 이내로 비교하도록 교정했다.

검증: 수정 후 실제 앱 Session 검사 1건과 시계·로그인·상태 연관 Node v26 검사 56건이 통과했다. Node v22 정기 전체 회귀는 `extracted/webpj-e947-node22.log`에 실행 중이며 결과는 후속 단위에서 확인한다.
