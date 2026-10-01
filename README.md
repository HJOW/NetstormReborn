Netstorm - Reborn
-------------------------------------------------------------------------

# 개요 
Activision 사에서 1997년도에 개발 후 한참 전에 손 놓은
Netstorm - Islands at war 게임을
AI 를 이용해 되살리는 프로젝트입니다.

현재 실행하면 **메인 메뉴 → 캠페인 → 자유를 위한 투쟁 → 1-1 전쟁의 시작!**을 플레이할 수 있습니다.
옵션에서 해상도·창/전체화면·효과음/음악 볼륨을 바꿀 수 있으며 다른 메뉴·미션은 잠금 표시합니다.
`dotnet run --project src/Netstorm.Game -c Release -- --language korean`으로 시작합니다(영어: `--language english`).
원본 데이터가 필요하며 임시 AI·건설 규칙과 검증 한계는 [캠페인 1-1 구현](docs/gameplay/campaign-one.md)에 정리했습니다.

# License

MIT License
Copyright (c) 2026 HJOW
