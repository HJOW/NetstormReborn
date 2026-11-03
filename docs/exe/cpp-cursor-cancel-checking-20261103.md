# 커서 취소의 현재 checking과 진단 복귀

단위801은10.78의00446e70에서 반환음 이후 현재 checking을 읽는 조건과 원문 `false` 진단을 연결한다. 같은 ClientGameplayGlobals의 AiDebug DWORD를 CursorLifecycleState가 빌리며, 일반 월드 초기화/반납에서도 이 뷰를 유지한다. 독립 기존 입력의 teleportBlocked는 값형 호환용으로 남긴다. 실제 Session은 현재 진단 서비스를 호출하고, 진단에서 돌아오면 현재 TYPE의 비용 차감과 기존 임시 목록/프로세스 정리를 계속한다.

원문은 잠금/preview/dead/0x100 분기가 진단 전체를 우회한다. 반환음이 checking을 바꿀 수 있으므로 반환음 전에 조건을 미리 계산하지 않는다. 비용도 반환음/진단이 끝난 뒤 현재 TYPE에서 읽는다. 진단 장치가 없으면 선택된 지점에서 명시 실패하며, 진단이 선택되지 않는 입력에는 장치를 요구하지 않는다. 기존 Take/Activate와 취소의 나머지 몸체는 유지한다.

새 원문 도구는 기존 전체 취소/Activate 관측 장치를 재사용한다. checking0/1/상위비트/FFFFFFFF, 비용 플래그 네 종류, 반환음/진단의 현재 checking·TYPE 변경, 잠금/빈 preview/죽은 대상156입력을 두 x87 정밀도에서 실행했다. 최초312회와 재관측312회가 상태78DWORD·raw150BYTE·전체 효과·반환/중단·현재 checking까지 바이트 일치한다. 입력당 정상 반환149/진단 중단7, 진단 선택21이다. 최초 보고서가 detach의 `D:`도 진단으로 세어144로 표시한 집계만21로 정정했으며 최초/반복 출력 바이트는 불변이다.

가상/UI/음향/SP/진단은 명시 장치이며 실제 PCM·창·진단 UI·전체 SEH를 실행한 근거가 아니다. 실제 Client의 현재 DWORD 연결과 장기 수명은 C++에서 검증하고 실제 Session 진단 서비스 연결은 소스/빌드로 확인한다. 기존565행 원문도 같은 검증 대상에서 유지한다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon/2026-10-23이며 현재 호스트가 같고 추출물 변경은 없다. 원시 관측·빌드·검사와 보호 기준은 `extracted/cpp-resume-801/`에 보관한다. 최종 결과는 [JSON 보고서](../../cpppj/cursor-cancel-checking-20261103.json)에 기록한다. 이번 범위에 새 게임/원본 GUI·포획/희생·특정 적 공격·시나리오 승리·공중 전용·MP/통신·물리/PCM·전체34/전체 C++/전체71대상·ZIP은 없다.

Release 게임/집중 대상 빌드는 경고·오류0이며 집중7개(새2개)가 실패0이다. checking 잔여는42→41(부모32·부분6·MP3)이다. 최종 EXE SHA256은 `611edbeeb24248ad05952a1ac5e341f2d7a0f3038912bdc8a30f9279763ea0d6`이다. 기존 보호2730개 불변·새fixture1 포함2731개/fixture911개이며 다음 기준은 `extracted/cpp-resume-801/protected-after.json`이다.
