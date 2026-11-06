# 1030 — 애니메이션 송신과 같은 장기 TYPE

10.78의00467f40/00467f80→00466ac0/00466c20을 공통 직렬화기로 복원해 실제 Session 전송 chain에 연결했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 양판본 원문과 보호 PE는 정적으로 읽었으며 원본 실행/변경과 신규 MP 검사는 하지 않았다.

PATCH의 실제 outer VT+8은 발사7개가467f80,회전/원위치4개가467f40이다. 기존 SunFire 한 VT만 구별하던 독립 writer도 같은 선택기를 사용한다. 실제 다섯 C++ 클래스의 공통 접두를 수신과 송신이 함께 빌리며,새 필드 사본이나 임의 다른 VT의 writer를 만들지 않는다. Ice의 추가16BYTE/Archer의 각상태/방향/좌표는 공통 패킷에 포함하지 않는다.

wrapper는 parentChanged||force일 때 현재 raw 부모DWORD를 인계한다. 내부 writer는 그 값이 비영일 때만 dirty1/부모DWORD를 쓰고,원래 선택된 gunWORD·targetWORD·시간8BYTE 뒤 패킷 길이/dirty를 기록하고 현재 dirty를0으로 소비한다. 부모0 때문에 기존 dirty1을 임의로 지우거나4BYTE를 추가하지 않는다. 실제 헤더00540468/69의3c01과0054046e/6f의3b01은 GameAux의 서로 다른 장기 두BYTE로 소유한다. 원문이 읽지 않는 전역 선언 길이는 이 writer에 도입하지 않는다. 물리 버퍼 부족은 기존 호스트 관례대로 쓰기 전에 거부한다.

실제 연결은 Aviary→Animation→Gun→Path의 일반/전체 writer와 packet/transmit이다. 애니메이션은 현재 RawSquidTransmissionState의 공통2048BYTE 버퍼를 직접 사용하고 같은 방송 장치로 전송한다. 대상별 전체 상태 전송은 같은 packet 조회로 이 버퍼를 읽는다. 다른 클래스는 기존 분배로 인계하며 새 성공 대체를 만들지 않는다.

장기35/36을 ProcessHostStorage가 같은 상태에 pin하고,다섯 일반 생성 factory·총의 현재 animation 조회/삭제와 전체 수신이 같은 번호를 읽도록 통합했다.71/128/140도 기존 시계/프레임 처리기에 선택적 장기 포인터를 연결해 수신 공장과 생성/현재 부모 검증이 같은 주소를 읽는다. 독립 입력과 다른 파생의 기존 타입/시계는 유지한다.

CD10.72의 기존 발사2/회전3 VT도 보호 PE에서4a2ec0/4a3240을 확인했다. 두 wrapper→4a0e80/4a0ca0의 필드 폭/순서와3b/3c·프로토콜1은 같다. CD의 헤더 변수 간격은 DWORD 단위이며 PATCH 두BYTE 배치와 구별해 문서화했다. 이번 제품 연결은 기존 PATCH 전용 World이며 CD 전체 복원/구동 완료를 뜻하지 않는다.

Release 빌드는 종료0·경고/오류0이다. 제품1030/11.01은12,033,536바이트, SHA-256은 `8732407ce98d3f951d175f96efe5693c18049f700bdb3e07064a3c4d3d3fce32`다. 신규MP/단위/게임/통신 검사와 fixture 생성은0이며 보호2800파일 불변이다. 컴파일 결과를 실제 공격/애니메이션/LAN 플레이 검증으로 세지 않는다.

정적 후속 감사에서 기존 건설37h의 진단을 예외로 중단하는 처리와 현재 TYPE 대여가 보완 대상으로 확인됐다. RegisterConstructionData 함수는 이미 있지만 실제 Session에서 RawConstructionData 생성/등록·writer 분배는 미연결임을 전체 제품 소스에서 확인했다. Player 내용물 통지00595344가 inBattle에 연결돼 있던 것도 원문4d55e0의 PlayerKnown과 맞춰야 한다. 나머지 decoder·재동기화/Activate·메타/Outpost/checking5,한국어 후속과 Edit 시나리오 UI는 잔여다.
