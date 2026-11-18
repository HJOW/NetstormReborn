# WB04-E859 Client 임시 문자열 고리

기준선 `225553df`의 `ClientStaticStrings` 중 장기 전역·별도 `RawFileName` 공유, 실제 할당 토큰과 뷰, 명시 초기화·지연 예약, 캡처 범위 안내를 이식했다. 원본 `RawStaticStrings`가 같은 네 DWORD와 표식을 관리하며 예약 크기 이후에도 실제 할당 끝까지 쓸 수 있다. 이전 할당은 재초기화 뒤에도 빌린 뷰가 유효하도록 보유한다. 기준선 이후 해당 C++ 파일 변경은 없다.

cpppj `StaticStrings_ActualOwnerReservationAndAlias`와 `CaptureInputTests`의 해당 범위에 맞춰 지연 8192·명시 10240, 고리 순환과 물리 별칭, 기존 파일 이름 고리 공유, 예약 80을 넘는 안내와 물리 범위 실패를 확인했다. 집중 검사 2건과 Node v26.13.0·v22.23.3 전체 회귀 각 6,376건이 통과했다(실패·취소·건너뜀 0).

정수 서식·서식 원본의 고리 별칭 판정과 실제 Client 수명 연결은 남아 있어 `partial`로 둔다. 원본 게임 창은 실행하지 않았다.
