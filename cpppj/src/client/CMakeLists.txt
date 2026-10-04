# 원본 클라이언트 폴더(assert 문자열의 ".\")의 모듈
# (ClientMain·UserInput·Gump 계열·Screen·Renderer·Sound·State·Mission 등)
# 다시 만든 파일을 아래 목록에 더한다. 파일 이름은 SOURCE_MAP.md 의 "cpppj 경로"를 따른다.
target_sources(netstorm PRIVATE
    ClientMain.cpp
    ClientMain.h
    VFXDraw.cpp VFXDraw.h
    Screen.cpp Screen.h
    GameAssets.cpp GameAssets.h
    Mission.cpp Mission.h
)
