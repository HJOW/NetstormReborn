# 앱 Fort 이름과 Session 저장 경계 (WB04-E948)

기준선 `225553df`의 `UberGump::SelectCurrentFortName`과 `RawCampaignSession::BindFortCurrentName`에 따라 앱의 현재 Fort 이름 장치가 선택한 스냅샷·통지·State를 실제 World의 Session에서 빌리도록 연결했다. 스냅샷은 같은 Session의 현재 Battle·Fort 모드를 순서대로 재조회하며, 설치 전 스냅샷·통지는 거부한다. World가 없을 때도 이름 복사와 manager 경로 변경 등 앞선 원본 효과는 되돌리지 않고 선택된 하위 호출에서 오류를 낸다. Edit의 설치 중 예외와 파일 삭제는 웹 제외다.

실제 Tutorial1 Session을 앱 State7·State8으로 적재해 설치 전 거부, 같은 Template State 바이트, 읽기 전용 통지, Battle 스냅샷의 Money 저장 및 반납 뒤 거부를 확인했다. 쓰기 가능한 manager의 앱 이름 선택은 먼저 스냅샷을 수행하고, 이후 미션 중 저장 진단에서 중단한다. 읽기 전용 manager에서 이름·경로를 쓴 뒤 통지하는 경우에도 같은 진단과 부분 효과를 확인했다. World가 없을 때의 부분 효과도 별도로 확인했다. 디스크 저장이나 미션 종료 후의 장기 수명은 이 검사 범위가 아니다.

검증: 실제 앱 Session·이름 집중 검사 4건, 원본 Fort 이름 876관측과 snapshot·Guide 연관 검사 37건이 Node v26에서 통과했다. 단독 미션의 지연 요청 소비·화면 설치·Native 수신기는 여전히 후속이다.
