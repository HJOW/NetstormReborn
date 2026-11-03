# 번개 팔레트 Frame과 도달 불가 checking 진단

단위824를 완료했다. 제품은 **824 / 표시11.01**이며 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. AGENTS.md·LEFT_JOBS.md·dotnet 인계를 다시 읽었고823은 `8f60c53d / 20261104-05 CP`로 커밋됐다. 기존 원본/fixture를 변경하지 않았다.

패치00471677~00471687은 `setge al`로 step>=0을0/1로 만든 뒤 eax를64와 비교하고jle로 진단을 건너뛴다. 따라서 checking 전체DWORD가 비영이어도 `0 <= step <= 64` 진단은 **모든 signed step에서 미도달**이다. 실제 기존 Frame 계산을 정상 범위 assertion으로 바꾸거나 새 진단 hook/설정 writer를 만들지 않았다. 이 사실을 실제 Frame에 한국어 주석으로 기록했다. VT0050c108[6]의 유일한 Frame과 공개 번개 요청00470fa0은 주변음004180f0·음악 날씨00469c80·일반 명령004d5300에서 기존 현재 Client/ThunderProcess로 연결된다.

실제 CPU1670행/재관측1670행은 각1690회 원문 Frame 호출이다. 시각/last/interval의 binary64,step/initialized,원래·현재RGBQUAD 예약바이트,checking 전체값,apply250/restore595/Kill1190 요청을 비교했다. 진단은0이다. INT_MIN/-1/0/1/63/64/65와 제한된 INT_MAX,정확한 간격/바로 다음double·역행·NaN/무한·0/음수 간격,빈색/마지막색/전체256색·연속2/7프레임을 포함한다. INT_MAX는 양수 간격에서 최대1회 적분하는 입력만 관측해 수십억회 루프를 만들지 않았다. 유효 Init 뒤 Frame 입력이며 원문 미초기화 메모리 자체 재현은 완료 범위가 아니다. 최종 fixture SHA-256은 `f3bd4686b3f421dc69c8a76e4893a8b52601d90004728e642ee27a66625b0c38`다.

최초 새 관측1670행은 재관측3행이 달랐고 최초 집중 검사는4단언이 실패했다. 표시 콜백이 있는 연속 프레임에서 관측기의 인라인 asm CALL 앞뒤로 C++ 시각/증분이 volatile XMM1/2에 살아 있었다. 실제 호스트 표시 콜백은 그 레지스터를 바꿀 수 있으므로 관측기가 다음 시각을 잘못 공급했다. 호출기계어를 확인하고 관측기만 **명시 thiscall/noinline ABI**로 바꿨다. 최초 새 데이터/보고서는 `checking-inlined-native-first.tsv/.json`,재관측 원시/차이·최초 검사/컴파일 로그와 호출기계어는 원시 경로에 보존했다. 기존 보호 fixture나 제품 계산을 실패에 맞춰 고치지 않았다. 교정된 새 CPU 결과와 독립 C++ 계산이 일치했고 재관측도 안정됐다. 최초 프로브 EXE는 최종 빌드로 교체됐으므로 당시 데이터/기계어/해시와 구별한다.

집중 **10개(새2) 최종 실패0**이다. 새 CPU1670행/실제 Client Kernel·FrameTime·현재 checking/게임 정지·Form 자연 종료2개,기존 PaletteFlash4개(옛768행의 판본별 허용 범위·창 없는 메모리DC 복구 포함),ThunderProcess3개,명령 밝기의 실제 Form1개다. 실제 Client와 같은 Kernel/FrameTime owner에 연결한 효과는 game0 정지 중 wall4.0→4.125→4.51에서 진행/종료하고,단일 복구/삭제·부모 목록/해제Form·소유 checking ffffffff를 보존했다. 게임 GUI/실제 원본 게임/통신/물리 화면은 실행하지 않았다. Release1회와 x86 프로브2회는 종료0·경고/오류0이다.

CD004b1450은 별도 원문이다. 연속 적분의 stack+20/24를 쓰기 전에 읽으므로 기존 문서처럼 미초기화 스택 차이가 남는다. 이번10.78 CPU 결과를 CD 진행 동일성으로 세지 않는다. x87은0x027f/53비트이며 전체64비트 확장 정밀도는 별도 경계다. 현재 Frame의 동일성 대조/연결 완료로 checking은 **22=부모14+부분5+MP3**다.

기존2751/fixture931개는 불변이고 새 CPU fixture 포함2752/932개다. EXE SHA-256은 `0bc85ce37a5a415d99878f7e7a43744b6ed6684b73227e597fac04993142a9b0`이며 [구조화 보고서](../../cpppj/palette-frame-checking-20261104.json)에 소스/검사/보호 해시가 있다. 원시는`extracted/cpp-resume-824/`다. 빌드/프로브/검사는 모두 종료했다. 다음825는0044c060/CD00462480의 DamageableBurning.Refresh 현재 find/index1 진단 복귀·실제 생성/삭제/피격 소비자다. 실제 특정 적 공격/포획/희생/승리·사용자 직접·MP·공중3종 전용·물리/PCM 재시도·전체71/34미션/ZIP 검사는 제외했다. 전체 목표는 미완료다.
