# WB04-E976 앱 미션 Kernel 프레임

메인 메뉴 상태에서 현재 World의 로컬 접속이 완료되면 앱 프레임의 안내·메뉴 입력 처리 뒤 같은 World의 `prepareKernelFrame()` → Client 공통 Kernel 한 번 → `finishKernelFrame()`을 수행한다. 접속 중이나 연결 콜백 재진입 중에는 이 경로를 실행하지 않는다. 게임 시계 정지 중에도 원본 Client 메인 루프처럼 Kernel 자체는 실행하며, Session 준비에는 현재 정지 상태를 전달한다.

검증: 실제 Tutorial1 로그인·Start·안내가 열린 정지 프레임과 창 종료 후 7프레임에서 공유 Kernel 관찰 프로세스가 프레임당 정확히 한 번 방문됐다. 원본 ProcessForm 482행, Kernel 진단과 기존 시계/음향 경계를 포함한 Node 24건 통과. 최초 시험 명령에 없는 `kernel.test.mjs`를 지정했지만 Node 26이 이를 생략해 실제 `kernel-diagnostics.test.mjs` 및 `process-form.test.mjs`로 다시 실행했다.

원본 메인 루프의 공기·바람·다리 붕괴·Gump 정리/idle·전투 자원/완료 등 주변 단계는 아직 이 단위에서 연결하지 않았다. 월드 합성·입력·자연 이동과 장기 플레이도 미검증이다. E974 고정 Node 22 전체 회귀가 실행 중이다.
