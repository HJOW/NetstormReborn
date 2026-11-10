# Native 건설·연구4F의 장기 기본값과 Session 대여 — 작업1257

**37h/3eh/3fh/58h 네 고정F의 실제 Source 기본표 설치와 Native Session 대여 연결을 마감했다. M6-l/NoWorld/M6/전체 목표는 미완료다.**1255의 초기 발견은 Session의 짧은 재등록이었다. 현재 소스를 다시 대조하니 모든 미태그63F가 이미 전역 고정F였던 것은 아니었다. 이번 네 행은 전역 고정 진입점도 새로 연결했다.

| type | F | observer | version/policy | 길이 |
| --- | --- | --- | --- | --- |
|37h|004427b0|NULL|1/0|ffff|
|3eh|00444af0|0040b380|1/0|25|
|3fh|004441b0|0040a800|1/0|ffff|
|58h|00453d70|0040b380|1/0|16|

원문40b4b0·네 본문과 보호 PE0052ec40의 길이 WORD를 대조했다. 해당 Source의 InstallClientPacketDefaults가 같은 Client 장기 game/진단 owner를 사용한다. 설치 자체는 현재 Session/Player/모드를 조회하지 않는다. 실제 설치 시점은 해당 Source 기본표 등록이며 모든 Source보다 앞선 별도 startup표 설치를 완료했다는 뜻은 아니다.

고정F는 장기 game과 저장된 코드값만 보유한다. 배송할 때 실제 현재 Session/전체 Runtime을 pin하고 그 코드값의 기존 본문을 직접 호출한다. packet[1]/현재 표의 G를 다시 선택하거나 재등록·재분배하지 않는다. old Session/World/lease/enqueue Runtime pin을 보관하지 않는다. Native Session은 그 불변F의 정체성/metadata를 확인해 이번 lease만 붙인다. 기존 lease 반납은 같은 전역 기본값을 복구한다. 독립 typed/CD 등록은 유지했다.

3eh/3fh는 원래처럼 PrepareConstruction 다음 같은 command.Receive/ActualHandle을 호출한다. 기존 공개 Receive의 추가 ValidateState를 우회하는 ReceiveCommandPacket은 원래 등록과 같은 raw command만 호출한다.37h는 buildingData 확인 뒤 등록하며 수신도 그 본문만 호출한다.58h는 같은 factoryResearch.Receive를 새 Prepare 없이 호출한다. RawConstructionCommand/Data/Notice/FactoryResearch 알고리즘4파일은 변경하지 않았다.

최종 Release/x64 제품2·집중1대상 **426.478초/종료0·경고/오류0**, 고정8파일 불변·기존 **비MP300/300** 통과다. 최종 EXE Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 생성→600프레임 대기→정상 종료0/PNG를 확인했다. 네 수신기의 모든 분기·MP/지연 배송·완공·청취·전체 동일성을 검증했다는 뜻은 아니다. 새 MP 단위/패킷/통신/게임/기록·실제 포획/희생·특정 적 공격·승리/자연 완주·공중3종 전용 실행은0이다.

보호 원본/CD/fixture·AGENTS75e4eed9e **2,808파일 불변**이다. 마지막 Client 양판본2026-11-03/R2026-11-07 디컴파일은 현재와 같은 HJOW-Athlon이며 새 디컴파일/원문 수정/ZIP은 없다. 혼합 줄바꿈은 국소 수정으로 보존했다. 글꼴 외부 fonts 계약은 M8/K1/K4/K5에 남는다.

다음은 같은 건설 등록 범위의54h/51h/2fh/6ch와 나머지 미태그/전역·Native Session 등록이다. 앞 네 행의 원문 F는4436f0/dc050/4332c0/4d56f0이며 observer는모두40b380이다. 각 기존 본문과 준비/부분 효과·metadata를 공유해야 한다. 현재 네 고정F의 실제 수신 본문은 현재 Runtime을 요구하며 전체 NoWorld 장기화 완료가 아니다. 큰 하위·최초 후처리·전수 process/caller/owner·Path/AI/배치/Fort Finish·M6-m/n도 계속한다.

[확정 단계 보고서](../../cpppj/native-construction-receivers-20261110.json), 원시 `extracted/cpp-resume-1257/`.
