# WB04-E977 앱 Renderer 프레임 전역 수명

구름 변경 4칸·행 스캔·묶음 숨김을 담는 `ClientRendererFrameGlobals`를 앱 Client 수명에 생성하고 앱 반납 시 참조를 놓는다. 화면 크기 변경으로 실제 Screen·Renderer가 교체되어도 이 전역 객체와 저장 값은 유지한다. 현재 단계는 소유권 연결이며 구름 생성·프레임 수집·화면 합성에 아직 사용하지 않는다.

검증: 실제 Tutorial1 앱의 로그인·Start 후 화면비 전환에서 Renderer만 교체하고 프레임 전역과 스캔·구름 개수의 동일 값을 보존했다. 원본 구름 관측 695행과 기존 전역·시계 검사 등 Node 14건 통과. 처음 시험에서 존재하지 않는 `renderer-cloud-queue.test.mjs`가 Node 26에서 생략되어 실제 `client-renderer-frame-globals.test.mjs`, `raw-renderer-clouds.test.mjs`로 다시 실행했다. E974 고정 Node 22 전체 회귀는 실행 중이다.
