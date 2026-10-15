# 원본 \Ns\Zacket\ 폴더의 네트워크 패킷 모듈 (Zacket.cpp)
# 멀티플레이는 우선순위가 낮다(AGENTS.md). 다시 만든 파일을 아래 목록에 더한다.
target_sources(netstorm PRIVATE
    # 단독 미션도 원본의 Zone/로그인 바이트와 로컬 두 연결로 분배한다.
    LocalMissionTransport.cpp LocalMissionTransport.h
)
