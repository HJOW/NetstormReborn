* 원본 바이너리 및 실행 파일은 originals 디렉토리에 존재함
* 이 프로젝트의 목적은 원 개발사가 1997년 지원을 중단하여 Abandonware 가 된 원본 게임을 
  디컴파일, 혹은 분석 후 클론코딩하여
  윈도우 10, 11 및 GUI 환경의 리눅스에서 동작하는 게임 빌드를 만들고, 여러 언어 (영어, 한국어가 1차 목표) 를 지원하도록 수정하여, 버려진 게임을 되살리는 것이다. 
  게임 내 그래픽과 사운드는 기존 게임의 사운드, 그래픽, 애니메이션 등 거의 모든 요소를 가능한 한 동일하게, 최대한 모방하는 것으로 한다.
  (리눅스 지원 및 멀티플레이 요소 구현은 우선순위가 낮다. 그외 사항들은 이 프로젝트의 궁극적인 목표에 해당한다.)
* 이 게임은 오래 전 게임 개발사의 지원이 끊겨 저작권 행사가 없다. 
  필요 시 기존 파일을 재활용하는 것이 가능하다.
* 프로젝트 결과물은 풀스크린 모드 및 화면비 16:9, 16:10, 4:3 를 지원해야 한다. 
  (본래 게임에서는 4:3 해상도만 지원했었다.)
  기존 게임은 요소마다 애니메이션 프레임이 달랐던 것으로 추정된다.
  새 클론 게임도 기존 게임 수준의 프레임으로 먼저 만든다. 이후 60, 120프레임을 지원하도록 수정한다.
  (화면 갱신 속도 및 애니메이션의 품질을 뜻함.)
* 풀스크린 모드에서는 마우스 커서를 화면 끝에 가져갔을 때 화면 이동이 되어야 한다. (원본 게임에서도 옵션에서 해당 기능을 켰을 때 지원했었음)
* 현재 originals 디렉토리에 있는 기존 게임 (10.78 버전)은 윈도우10, 11 상에서 제한적으로 구동이 가능하다.
  Netstorm.exe 실행 시 창 모드로 구동이 된다. 
  옵션을 통해 전체화면 전환 시 전환도 되지만, 이 경우 게임 종료 후 재실행 시 오류가 발생한다. 이후 재실행하면 다시 창 모드로 실행된다.
  이 문제는 새 클론 게임에서는 발생하지 않아야 한다.
* originalCD 디렉토리에는 10.72 버전 (위 originals 디렉토리의 게임보다 과거 버전) 의 기존 게임의 설치 CD 파일들이 들어있다.
  이 버전은 윈도우 95, 98, ME 에서 호환됐으며, XP에서는 호환되지 않았다.
  인트로 동영상 자료가 있으며, 
  버전이 다르므로 이 또한 디컴파일하여 originals 디렉토리의 버전과 비교하는 것이 의미가 있을수도 있다.
* 원본 게임의 플레이 동영상 자료가 있다.
  원본 게임은 1024*768 이 최대 지원 해상도여서 이 해상도로 설정 후 풀스크린으로 진행하면서 촬영하였다.
  PC 모니터는 16:9 해상도를 사용하고 있었으므로 영상에 동일한 크기의 레터박스가 들어갔다.
  실제 방송 중 촬영하였으므로 사람 음성이 같이 녹음되었다.
  영상 자료는 playingVideos 디렉토리에 있다. (용량이 커서 github 에 커밋하지 않는다.)
  이 파일들은 평소보다 밝게 촬영되었다. (HDR 설정 관련 문제로 추정)
* 다음 유튜브 URL로부터도 영상 자료를 얻을 수 있다.
- 튜토리얼 : https://www.youtube.com/watch?v=CI3dCrUt4tY
- 캠페인 1-1~4 : https://www.youtube.com/watch?v=AMEzorbjQYQ
- 캠페인 1-5 : https://www.youtube.com/watch?v=adR1Kap60hw
- 캠페인 1-6 : https://www.youtube.com/watch?v=PUeIPg1BEzo
- 캠페인 2-1 : https://www.youtube.com/watch?v=zpZsx4dRac8
- 캠페인 2-2 : https://www.youtube.com/watch?v=2NxTN314RnE
- 캠페인 2-3 : https://www.youtube.com/watch?v=LHkgSp0J73E
* 다음 유튜브 URL로부터도 영상 자료를 얻을 수 있다.
- 캠페인 2-1 : https://www.youtube.com/watch?v=8tcj3rI_YqE
- 캠페인 3-2 : https://www.youtube.com/watch?v=WDQSrGqZAH0
- 캠페인 3-5 : https://www.youtube.com/watch?v=b3VzVERd9CE
- 캠페인 3-1 to 3-5 연속 플레이 : https://www.youtube.com/watch?v=0p7VvzSxTAY
( 기존 게임 영상만을 전문으로 올리는 유튜브 채널 주소 : https://www.youtube.com/@netstormcampaigns2591 )
- 캠페인 3-4 (사운드 있음, 후편집 추가요소 주의) : https://youtu.be/t2g9fASt4do?si=-O2tjL5u-cAan6fG
- 캠페인 3-5 (사운드 있음, 후편집 추가요소 주의) : https://youtu.be/6F8En-b6FXU?si=kfNuAjtnXIdOYpZq
* 게임 종합 소개 영상 : https://www.youtube.com/watch?v=q4eSz1TjqYQ
* 게임 플레이 방법을 소개한 홈페이지도 참고할 수 있다.
  https://hjow.duckdns.org/netstorm/learnmain.htm
* 커스텀 맵 제작 방법 1
  https://blog.naver.com/hujinone22/221213700658
* 커스텀 맵 제작 방법 2
  https://netstorm.fandom.com/wiki/Starting_Editing
* 새로 추가된 커스텀 맵 TEST01 은 originals/d 디렉토리 안에 있으며, TEST01.fort 와 TEST01.english 파일로 구성되어 있다.
  fort 파일은 게임 내 Edit 메뉴를 통해 생성/수정할 수 있으며, english 파일은 사용자(개발자)가 텍스트 에디터로 수동 작성해야 하는 텍스트 파일이다.
  english 파일로 게임 내 사용자 및 인공지능 구성, 시작 SP 및 브리핑 창 내용을 코딩할 수 있다.
  다른 언어 지원 시 해당 언어 이름을 확장자로 하여 english 파일과 마찬가지로 작성하는 방식이다.
* 스크린샷 자료는 프로젝트 최상위 경로 내 screenShots 디렉토리에 있다.
  외부 캡처 도구를 이용해 캡처한 스크린샷으로 작업표시줄과 창 테두리 및 바깥 영역이 일부 포함되어 있다.
  메인 메뉴, 도움말 스크린샷을 제외하면 미션 이름이 파일 이름 앞에 붙어있다.
* 클론 게임 개발은 세 가지 버전으로 진행한다.
  cpppj    - C++ , 기존 게임을 디컴파일하여 C++ 코드로 최대한 똑같이 복원하는 것이 1차 목표이다. 10.78 버전으로 복원하도록 한다.
                (단 멀티플레이어 및 Outpust 구현은 2차 목표보다 후순위이다. 참고로 Outpust 는 멀티플레이어 내에서만 사용된다. 그리고 추후 CD판 10.72 버전 복원도 필요할 수 있으니 문서화 시 10.72 관련 내용도 기록한다.)
             이후 윈도우10, 11에서 오류 없이 실행 가능한 수준으로 만들고 (2차 목표, 전체화면 DirectDraw 오류도 이 때 수정되어야 함. 10.72 는 윈도우10에서 아예 구동이 안되는데, 차후 10.72 작업 시 이 해결방안 또한 문서화한다.)
             이후 멀티플레이어 쪽을 구현하되 공식 서버는 현재 도메인이 없어졌으므로 TCP/IP 기반 로컬 네트워크만 구현한다. (3차 목표, Outpust 구현을 포함하여 기존 게임 모든 기능과 코드 복원 필요)
             이후 추가 요구사항을 반영한다. (4~8차 목표.)
                 4차 - 로컬 MCP 추가 작업 및 와이드 해상도 지원 작업 진행 
                       로컬 MCP 를 추가하는 이유는 dotnetpj 개발을 위해 게임 분석에 활용하기 위해서이다. 로컬 MCP를 통해 게임 내에서 모든 조작, 화면 캡처 및 녹화(사운드 포함) 이 가능하여야 한다.
                       와이드 해상도는 16:9 비율의 해상도를 의미한다. 최대 1920 * 1080 해상도까지 지원하는 것을 목표로 한다. (이 단계까지는 해상도 지원만 하며, 그래픽 크기 스케일을 조정할 필요는 없다.)
                 5차 - 설정 및 캠페인 진행 상황 저장 디렉토리를 OS 사용자 홈 디렉토리 내부로 이동 (Windows 기준 %USERPROFILE%\Saved Games\Netstorm\Saves\ 에 저장)
                       커스텀 맵 (Edit 기능으로 작성한 맵) 은 기존 디렉토리와 사용자 홈 디렉토리 내부 (Windows 기준 %USERPROFILE%\Saved Games\Netstorm\CustomMaps\ ) 둘 다 불러오되, Edit 메뉴에서 신규 작성한 맵은 사용자 홈 디렉토리 내부 (Windows 기준 %USERPROFILE%\Saved Games\Netstorm\CustomMaps\ ) 에 저장하도록 한다.
                       추가로, 메인 메뉴에서 Edit 선택 시 나타나는 커스텀 맵 목록에 스크롤을 도입하고 출력 갯수 제한을 무제한으로 변경한다.
                       이 사항은 단순히 저장 디렉토리를 변경하는 것이 아니다. 게임을 관리자 권한 없이는 파일 수정이 안되는 곳에 설치했을 때에도 게임이 정상 동작하도록 하기 위한 것이기도 하다. 게임 구동 시 자동 생성되어야 하는 모든 파일과 디렉토리가 %USERPROFILE%\Saved Games\Netstorm\ 하위로 들어가야 하며, 해당 파일을 읽는 코드도 그에 맞게 수정되어야 한다.
                 6차 - 한국어 지원 작업
                 7차 - 안티앨리어싱 및 FSR 1.0 선택사항으로 추가
                       메인 메뉴 - Options 에서 안티앨리어싱 및 FSR 1.0 을 선택적으로 켤 수 있도록 기능 추가 (기본값으로는 꺼져 있어야 함)
                       게임 내에 해당 기능 적용
                       (FSR 은 FidelityFX 의 Super Resolution 기능으로 https://github.com/GPUOpen-Effects/FidelityFX-FSR 참고, 해상도 업스케일링 기능 사용)
                       FSR 1.0 지원 가능한 환경에서는 옵션 내 해상도 목록에서 2560 * 1440 및 3840 * 2160 또한 지원되어야 한다. 
                       (이 두 해상도 사용 시 FSR 1.0 이 자동으로 켜지도록 하며, 이 경우 실질 해상도는 1920 * 1080 으로 하며 이를 FSR 로 업스케일링하여 목표 해상도를 구현한다.)
                 8차 - 커스텀 맵 에디터 (메인 메뉴 - Edit 선택 시) 에서 맵과 구성요소 (유닛 등) 배치 뿐만 아니라 시나리오 자체를
                       수정할 수 있는 기능 구현
                       english 파일이 없는 상태에서 Test Battle 진입 시 게임이 튕기는 문제 수정
                       (새 맵 생성 시 english 파일 기본 컨텐츠로 자동 생성)
                       편집 모드 전용 별도의 UI를 마련하여, 편집 모드인 경우 시나리오 구성 (플레이어 초기 자본과 테크, 적 구성과 동맹 등) 을 화면 내에서 편집할 수 있도록 새 기능을 구현해야 함
                 9차 - 기존 게임에서 알려져 있던 버그 수정
                       10.78 에 다음 버그가 있다고 함. 이를 확인 및 수정.
                          Mission Bug Where the game would suddenly crash after an hour of game play on replenishing geyser spawn maps. This bug was the result of an AI update which expanded geyser connection capability but neglected to change the parameter in all incidents.
                          Bug that allowed players to exploit the Emboss offset System and move there text further back than it was intended.
                          The bug where workshop production slots dint display.
                          Bug in Even Geyser Mode such that the player in the bottom corner will no longer receive geysers last.
                          A issue where putting too much text in the chat bar would result in a game crash.
                          The AI bug where it would fail to see it found a temple and repeated create a temple.
                          A Bug in the Editor that caused existing fort files to be open when in fact a new file should have been created  A message now appears relaying that the file exist. 
                          User Made Campaign Bug that was preventing User Made Campaigns from showing up under the User Made Campaign Menu.
                          Bug that if player guild tags exceeded maximum length right clicking them caused the client to crash.
                          A Bug in Single Player Missions that would result in a crash because of a invalid player side.
                          A bug in the demo where severely hurting a AI workshop would result in a crash.
                          A bug that caused you to instantly quit out of a game if you were typing when Battlemaster launched.
                          Fixed Floating Point Round up Issue with Cids that was causing a edge suck when it shouldn't.
                          Bug that allowed you to load multiplayer forts in a single player editor
                          Client Acid barricade bug that was causing barricades not to spin.
                          Client Side Pickup bug that caused clients not to register the bulf picking up crystals correctly.
                          Client Side Drop bug that caused the Clients not to register the bulfs dropping crystals correctly.
                          Bulf Bug where if your bulf had a nugget he could pick up a priest and the priest would transform into another nugget.
                          Bulf bug where if you dropped a storm crystal then went to a storm power processor you wouldn't receive 200 sp.
                          Bug where you select the priest, yet a item you weregoing to build remains attached to the cursor.
                          Selection Bug Where you could have a collector selected,then click on a item on the build menu and drop the item only to find the collected selected but not useable, or unselectable withoutsome forceful persuasion.
                          Issue where pressing the P or R key and the F key caused Netstorm to crash.
                          A rare island generation crash.
                          Long campaign names cause game to crash.
                          Sun Cannon range display doesn't match firing arc.
                          Thunder cannon shoots forever at units directly in front of it.
                          Random crash on construction caused by air targeting changes.
                          Too many geysers when playing multiple games in a row.
                          Adjusted starting geyser formula which was incorrect due to an error.
                          Fort data (including stats) is lost when host crashes before all forts are loaded.
                          Fort stays logged into zone after resetting Netstorm for resolution change.
                          Crash caused by Ice Cannons attacking units adjacent to a Workshop or Temple.
                          Ice Tower islands not destroyed properly in the editor. 
                          Unit health incorrectly appearing to reset to full on client-side. Especially Whirlibases.
             cpppj 는 Windows 10/11 대상으로 개발한다. 
             (x86-64, AMD64 플랫폼 대상으로 개발하되, ARM 플랫폼용 Windows 11 지원도 고려한다.)
  dotnetpj - C# + MonoGame 기반으로 개발. cpppj 완성 후 이를 분석하여 개발.
             dotnetpj 또한 일단은 기존 게임과 완전히 동일하게 동작하는 것을 목표로 개발 후, 요구사항을 반영한다.
             cpppj 의 4~8차 목표 내용이 dotnetpj 후순위 요구사항에도 포함된다.
             리눅스용 개발은 후순위이다.
  webpj - html + javascript 기반으로 개발. cpppj 완성 후 이를 분석하여 개발.
          dotnetpj 또한 일단은 기존 게임과 완전히 동일하게 동작하는 것을 목표로 개발 후, 요구사항을 반영한다.
          (단, 커스텀 맵 (Edit), 멀티플레이 기능 구현은 제외한다. 그리고 웹 환경과 맞지 않는 기능은 제외한다. 예를 들면 게임 종료, 옵션 내에서 해상도 설정이나 전체화면 모드 등.)
          웹 플랫폼 특성상 화면 내부를 1회 클릭하지 않으면 사운드 재생 등이 불가능하므로, 메인 메뉴 화면을 보이기 전 "클릭하여 시작" 화면을 도입한다.
          에셋이 필요한 경우 webpj/www 내부로 에셋을 복사해 사용한다.
* cpppj, dotnetpj 모두, Windows 용 게임 실행 파일의 이름은 Netstorm.exe 로 생성되어야 한다.
* 멀티플레이어의 로컬 서버 접속 기능을 제외한, 외부 도메인이나 IP 에 접속하는 기능 (클라이언트 유효성 검사 기능 등) 은 모두 찾아 따로 문서화하여야 하며,
  특히 클라이언트 유효성 검사 기능은 비활성화하여야 한다. (해당 서버 도메인이 서비스 종료하였기 때문에 의미가 없어졌다.)
* 새 버전 (cpppj, dotnetpj, wepbj) 모두 버전 표기는 11.01 으로 한다. 
  메인 메뉴에서 Credits 메뉴 클릭 시, NetStorm Credits 목록에서 항목이 3개 (기존 : 2개) 가 나와야 하며,
  새로 추가되는 항목은 "Netstorm 11.01 Patch Credits" 으로 한다.
  해당 항목 선택 시 다른 Credits 메시지 창과 동일한 Credits 메시지 창이 나오되, 
  제목은 "NetStorm 11.01 Patch Credits" 으로 하고, 내용에는 HJOW 개발, AI 사용에 대한 내용을 기재한다.
* 프롬프트의 응답과, 새로 만드는 코드의 주석은 한국어로 작성하며, 새로 만드는 문서와 코드들은 UTF-8 인코딩을 사용해야 한다.
* 한국어를 위한 글꼴이 필요한 경우 fonts 경로상에 있는 D2Coding 글꼴을 사용한다. 한국어를 지원하며 트루타입 monospace 글꼴이다.
  (최근 이 글꼴 마이너 업그레이드 버전이 출시되어 동일 경로상에 넣었다.)
* 새로 만드는 코드 내 네임스페이스, 상수, 함수, 구조체, 클래스, 멤버함수, 멤버변수, 반복문에는 모두 주석으로 설명이 달려야 한다.
  이 때, 주석은 한국어로 작성하며, 해당 네임스페이스, 상수, 함수, 클래스, 멤버함수, 반복문의 설명과 사용법이 주 내용이 되어야 하고, 이 것이 언제 및 왜 추가/수정됐는지 대한 내용은 주 내용 뒤에 들어가야 한다. 이 사항은 cpppj, dotnetpj, webpj 모두 해당된다.
* 작업 전 LEFT_JOBS.md , LEFT_JOBS.dotnetpj.md 문서를 읽어 인수인계 사항이 있는지 확인한다.
* 작업 도중 중단 요청이 있었거나, 인수인계 자료를 작성하라는 요구가 들어온 경우 
  cpppj 및 공통 사항 - LEFT_JOBS.md 에 작성한다.
  dotnetpj - LEFT_JOBS.dotnetpj.md 에 작성한다.
  webpj - LEFT_JOBS.webpj.md 에 작성한다.
* cpppj 및 dotnetpj 모두 각각, 배포판 압축파일 생성용 스크립트 (bat 혹은 ps1으로 작성) 를 만들어야 한다.
  이 스크립트 실행 시 매개변수 입력이 안된 경우, 사용자에게 배포판 압축파일 저장 위치 (파일명 포함) 를 입력 받는다. (매개변수 입력된 경우, 매개변수 값을 배포판 압축파일 저장 위치로 사용한다.)
  빈 값 입력 시 취소로 간주하여 종료한다.
  실제 저장위치 입력 시, 해당 위치 및 파일명으로 압축 파일을 생성해야 하는데,
  이 압축파일 안에는 실제 게임 실행파일은 물론, 실행에 필요한 에셋 파일들이 모두 포함되어야 한다.
  이 압축파일을 그대로 다른 사용자에게 배포했을 때 다른 사용자가 압축 해제 후 게임을 실행할 수 있어야 한다. (단 닷넷 런타임은 각 사용자가 따로, 미리 설치한다고 가정한다.)
* 작업 완료 후, LEFT_JOBS.md / LEFT_JOBS.dotnetpj.md / LEFT_JOBS.webpj.md 에서 해당 작업 완료 항목을 제거하거나 작업 완료했음을 명시한다.
* 프로젝트 진행상황 요약을 요청받은 경우, cpppj 와 dotnetpj, webpj 프로젝트를 각각, 게임 실행 및 플레이 가능여부와, 기존 게임과의 유사성 (동일성) 두 가지 기준으로 평가하여
  어느 정도까지 진행되었는지를 응답한다.
* 이 문서 (AGENTS.md) 는 절대 수정하지 않는다. 수정이 필요한 경우 개발자(사용자) 에게 요청한다.
* 프로젝트 루트 경로 상의 README.md 의 "개요" 단락 내용도 절대 수정하지 않는다. 단 README.en.md 의 "Overview" 단락의 경우 README.md 의 "개요" 단락 내용을 번역해야 하므로 수정이 가능하다.
* 기존 게임 (originals 디렉토리 및 그 하위의 모든 파일, originalCD 디렉토리 및 그 하위의 모든 파일) 파일 또한 절대 수정 및 삭제하지 않는다.
  해당 디렉토리 내에 파일 생성은 가능하다.
  options.cfg 와 fullScreenStateFile.dat 는 수정, 삭제 가능하다. (기존 게임 구동 시 자동 생성되거나, 옵션 설정 변경 시 수정되는 파일)
  수정,삭제가 필요한 경우 사용자(개발자) 에게 요청한다.
  사용자(개발자)가 명시적으로 해당 디렉토리 내 파일 수정/삭제를 허용한다고 한 경우 또한 예외적으로 작업이 가능하다.
* 원본 게임 자동 탐험 프로그램(analyzeManager)을 이용하면서 실제 게임 구동이 필요하거나 작업 중 별도 창이 뜨는 작업 시, 
  실행 전에 개발자(사용자)에게 실행 목적과 필요성을 설명하고 명시적인 확인을 받은 후 진행해야 한다.
  이 규칙은 CLI/MCP의 start_session, mcp_smoke.py --live, 테스트를 위한 원본 또는 복사본 직접 실행에 모두 적용한다.
  일반적인 작업 진행 지시나 이전 실행 이력만으로는 새로운 게임 실행이 허용되었다고 간주해서는 안 된다.
  게임을 구동하지 않는 빌드, 정적 분석, 단위 테스트, MCP 프로토콜 검사는 이 실행 확인 대상에 포함하지 않는다.
  실제 게임 구동이 허용된 경우 해당 작업단계 완료까지 허용 상태를 유지한다.
  위 규칙은 특정 시스템에서는 예외를 적용한다.
  - 다음 IP주소 및 호스트명에 해당하는 시스템에서는 개발자(사용자) 에게 실제 게임 구동 확인을 받지 않아도 되며, 별도 창이 뜨는 작업도 확인 없이 가능하다.
    시스템 1 - IP : 10.0.0.15     HOSTNAME : vm-debian-codex
    시스템 2 - IP : 10.0.0.17     HOSTNAME : VM-W11-Codex
    시스템 3 - IP : 192.168.0.94  HOSTNAME : HJOW-Athlon
  - 사용자(개발자)가, 기존 게임 수동 컨트롤 방식 혹은 기존 게임 플레이 녹화 분석 방식으로 분석 진행을 직접 요청한 경우 
    해당 작업 단계에서는 실제 게임 구동 확인을 받지 않아도 된다.
* cpppj 작업 및 기존 게임 디컴파일 소스 분석이 필요한 경우, LEFT_JOBS.md 문서를 참고하여, 마지막 디컴파일한 PC 호스트명을 확인해 현재의 PC가 맞는지 확인한다. 맞지 않으면 디컴파일을 다시 진행한 후 작업해야 한다.
* 디컴파일 소스 변경 시 작업 내역 문서 및 LEFT_JOBS.md 에 마지막 디컴파일을 수행한 PC 호스트명을 명시/갱신하여야 한다.
* git 히스토리를 읽거나 원격 저장소에서 pull 하는 건 가능하나, 원격 저장소에 push 해서는 안 된다.
* 작업 단위가 하나 완료되면 인수인계 문서 작성을 완료한 후 git 에 커밋을 수행한다.
  커밋 메시지는 YYYYMMDD-NN PJ MSG 형식으로 작성한다. 
  YYYYMMDD 에는 커밋한 날짜를 넣고, NN 자리에는 번호를 넣는다.
  PJ 자리에는 해당 프로젝트 코드 (dotnetpj 의 경우 DN, cpppj 의 경우 CP, webpj 의 경우 WB, 셋 모두 해당하지 않거나 공통사항인 경우 CM 으로 입력한다.)
  MSG 자리에는 1줄로 요약된 작업 내용을 넣는다.
  (예: 20261014-01 실제 지형과 섬 목록 및 썬 디스크 애니메이션과 신전 룬 복원 검증)
* 게임 내 건물과 유닛의 이름을 한국어로 표현할 때, 사제 (Priest) 와 신전 (Temple), 다리 (Bridge), 발사기 (Thrower), 간헐천 (Geyser), 제단 (Altar), Residence (거주 구역), Empty (고갈된) 은 번역을 해도 되나
  그 외의 경우는 영어 발음을 한국어로 표현 (예: Workshop --> 워크샾, Generator --> 제네레이터) 한다.
  (이유가 있는데, 예를 들면 판타지 세계관에서 Generator 를 제네레이터로 번역하면 "발전"의 "전" 글자가 전기를 뜻하므로, 이 세계관에서는 맞지 않는 의미가 된다.)
  발음 표현이 모호한 경우는 다음과 같이 정한다.
  - Whirlibase   : 훨리베이스
  - Whirligig    : 훨리기그
  - Vander Tower : 반더 타워
  