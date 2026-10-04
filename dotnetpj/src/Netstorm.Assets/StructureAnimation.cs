namespace Netstorm.Assets;

/// <summary>건물·가이저처럼 제자리에서 도는 그림의 지금 프레임: 본체 클러스터와, 본체 위에 겹쳐 그리는 클러스터(없으면 null).</summary>
/// <param name="Body">본체 클러스터 번호 (그림자는 이 클러스터의 것을 쓴다)</param>
/// <param name="Overlay">본체 위에 같은 기준점으로 겹쳐 그릴 클러스터 번호</param>
public readonly record struct StructureFrames(int Body, int? Overlay = null);

/// <summary>
/// 제자리 애니메이션의 프레임 선택 규칙. 2026-10-03 TEST01 시험 전투 녹화를 내보낸 스프라이트와 맞춰 본 결과다
/// (<c>tools/sprite_match.py</c>, docs/videos/test01-visuals-20261003.md).
/// 간격은 원본의 두 계열을 쓴다: 0.04초 계열 → 화면에서 약 24Hz, 0.08초 계열 → 약 12Hz (docs/videos/animation-timing.md).
/// </summary>
public static class StructureAnimation
{
    /// <summary>가이저 증기 한 프레임의 길이(초). 원본 0.04초 타이머가 75fps 루프에서 3루프 = 약 41.7ms 가 된다.</summary>
    public const double SteamFrameSeconds = 1.0 / 24;

    /// <summary>신전 회오리·번개, 풍선 흔들림 한 프레임의 길이(초). 원본 0.08초 타이머 → 약 83.3ms.</summary>
    public const double SwirlFrameSeconds = 1.0 / 12;

    /// <summary>Rain 워크샵의 겹침 조각이 바뀌는 간격(초). 녹화에서 0.5초마다 바뀌어 세 단계가 1.5초에 한 바퀴 돌았다.</summary>
    public const double WorkshopFrameSeconds = 0.5;

    /// <summary>
    /// 가이저가 평소에 되풀이하는 증기 프레임 수 (A00~A16). 녹화에서는 이 17장만 차례로 나왔다.
    /// A17~A48 은 바위 없이 증기만 있는 작은 그림이며 어느 상태에서 쓰는지는 확인하지 못했다.
    /// </summary>
    public const int GeyserSteamFrames = 17;

    /// <summary>워크샵 레벨 1·2·3 의 방향 글자 (A·B·C). 녹화의 FactoryState 0·1·2 와 풍선 도움말 "Level III" 로 확인했다.</summary>
    private const string FactoryLevelSides = "ABC";

    /// <summary>
    /// 같은 이름 클러스터가 진행 방향별 4장씩 놓인 수송 유닛(Sail Skater)의 방향 묶음 시작 번호.
    /// .type 주석 순서: Going North(0~3), West(4~7), South(8~11), East(12~15).
    /// 색인은 <c>UnitHeading</c> 의 8방향(0 = 북, 시계 방향)이며 대각선은 가까운 동·서 묶음으로 보낸 추정이다.
    /// </summary>
    private static readonly int[] SailGroupStart = [0, 12, 12, 12, 8, 4, 4, 4];

    /// <summary>방향 묶음 하나의 프레임 수.</summary>
    private const int SailGroupSize = 4;

    /// <summary>경과 시간으로 되풀이 애니메이션의 프레임 순번을 구한다 (프레임 수가 0 이하이면 0).</summary>
    /// <param name="seconds">게임 경과 시간(초)</param>
    /// <param name="frameSeconds">한 프레임의 길이(초)</param>
    /// <param name="count">되풀이할 프레임 수</param>
    public static int LoopIndex(double seconds, double frameSeconds, int count)
    {
        if (count <= 0 || frameSeconds <= 0) return 0;
        long step = (long)Math.Floor(Math.Max(0, seconds) / frameSeconds);
        return (int)(step % count);
    }

    /// <summary>가이저의 지금 증기 프레임 (A00~A16 을 약 24Hz 로 되풀이). 그 그림이 없는 타입이면 기본 프레임이다.</summary>
    /// <param name="type">geyser 타입</param>
    /// <param name="seconds">게임 경과 시간(초)</param>
    public static int GeyserFrame(TypeDefinition type, double seconds)
    {
        IReadOnlyList<int> steam = type.Frames.Sequence('A', TypeFrameTable.DefaultVariant);
        int count = Math.Min(GeyserSteamFrames, steam.Count);
        return count == 0 ? type.Frames.DefaultFrame : steam[LoopIndex(seconds, SteamFrameSeconds, count)];
    }

    /// <summary>
    /// 워크샵의 지금 프레임. 방향 글자가 레벨(1 = A, 2 = B, 3 = C)이고 번호 01 이 건물 본체다.
    /// 번호 02 이상은 본체 위에 겹치는 작은 조각(Rain: 물 반짝임, Sun: 불 켜진 창)이다.
    /// Rain 워크샵만 "조각 없음 → 02 → 03" 을 0.5초마다 되풀이한다 (녹화에서 1.5초 주기).
    /// Sun 워크샵의 창 조각은 녹화 내내 보이지 않았고 언제 켜지는지 확인하지 못해 그리지 않는다.
    /// </summary>
    /// <param name="type">워크샵 타입</param>
    /// <param name="level">워크샵 레벨 (1~3, 범위 밖은 가까운 값으로 맞춘다)</param>
    /// <param name="seconds">게임 경과 시간(초)</param>
    public static StructureFrames FactoryFrames(TypeDefinition type, int level, double seconds)
    {
        char side = FactoryLevelSides[Math.Clamp(level, 1, FactoryLevelSides.Length) - 1];
        // 도움말 그림(P00, Thunder 의 둘째 D01)은 방향 글자가 달라 후보에 들지 않는다.
        IReadOnlyList<int> frames = type.Frames.Sequence(side, TypeFrameTable.DefaultVariant);
        if (frames.Count == 0) return new StructureFrames(type.Frames.DefaultFrame);
        bool cycles = string.Equals(type.GetString("theme"), "rain", StringComparison.OrdinalIgnoreCase);
        int step = cycles ? LoopIndex(seconds, WorkshopFrameSeconds, frames.Count) : 0;
        return new StructureFrames(frames[0], step > 0 ? frames[step] : null);
    }

    /// <summary>
    /// 타입의 <c>hotFootRatioX·Y</c> 로 그림 기준점을 칸 기준점에서 옮기는 논리 픽셀 (왼쪽·위쪽이 음수).
    /// 원본은 비율 × 한 칸 크기(16×11)만큼 왼쪽·위에 그린다. TEST01 녹화에서 건물(비율 없음) 대비
    /// 가이저 (0.22, 0.22) → (−3, −2), Rain 수정탑 (0.5, 0.4) → (−8, −5), 사제 (0.5, 0.2) → (−8, −2),
    /// Cloud Floater (1.0, 0.7) → (−16, −7) 캡처 환산값이 모두 1픽셀 안에서 맞았다. 소수점은 버린다.
    /// </summary>
    /// <param name="type">오브젝트 타입</param>
    /// <param name="cellWidth">한 칸의 화면 폭(논리 픽셀)</param>
    /// <param name="cellHeight">한 칸의 화면 높이(논리 픽셀)</param>
    public static (int X, int Y) HotFootShift(TypeDefinition type, int cellWidth, int cellHeight) =>
        (-(int)((type.GetDouble("hotFootRatioX") ?? 0) * cellWidth), -(int)((type.GetDouble("hotFootRatioY") ?? 0) * cellHeight));

    /// <summary>
    /// 신전의 지금 프레임. 기본 클러스터에 <c>baseframe</c> 이 있으면(Wind B01, Rain A00) 그 본체 위에 다른 글자의 그림을
    /// 겹쳐 돌리고(Wind 회오리 A00~A12, Rain 물결 B01~B14), 없으면(Thunder A00) 본체 자체를 B00~B06 으로 바꿔 돌린다.
    /// 간격은 약 12Hz 다. C 묶음은 건설 중 그림이라 쓰지 않는다.
    /// </summary>
    /// <param name="type">신전 타입 (typeflags vortex)</param>
    /// <param name="seconds">게임 경과 시간(초)</param>
    public static StructureFrames VortexFrames(TypeDefinition type, double seconds)
    {
        int body = type.Frames.DefaultFrame;
        if ((uint)body >= (uint)type.Frames.Codes.Count) return new StructureFrames(body);
        char animated = type.Frames.Codes[body].Side == 'A' ? 'B' : 'A';
        IReadOnlyList<int> frames = type.Frames.Sequence(animated, TypeFrameTable.DefaultVariant);
        if (frames.Count == 0) return new StructureFrames(body);
        int current = frames[LoopIndex(seconds, SwirlFrameSeconds, frames.Count)];
        return type.Frames.BaseFrame == body ? new StructureFrames(body, current) : new StructureFrames(current);
    }

    /// <summary>
    /// 풍선(typeflags balloon)의 지금 프레임. A 묶음이 여러 장이면(Cloud Floater A00~A19) 약 12Hz 로 되풀이하고,
    /// 한 장뿐이면 null 이라 호출한 쪽이 기본 프레임을 쓴다.
    /// </summary>
    /// <param name="type">풍선 타입</param>
    /// <param name="seconds">게임 경과 시간(초)</param>
    public static int? BalloonFrame(TypeDefinition type, double seconds)
    {
        IReadOnlyList<int> frames = type.Frames.Sequence('A', TypeFrameTable.DefaultVariant);
        // 도움말 그림이 A00 인 타입(windBalloon)은 한 장뿐이므로 걸러진다.
        return frames.Count > 1 ? frames[LoopIndex(seconds, SwirlFrameSeconds, frames.Count)] : null;
    }

    /// <summary>
    /// 걷기 그림 묶음(A00~H07)이 없고 같은 이름의 클러스터를 방향별 4장씩 둔 수송 유닛(Sail Skater)의 프레임.
    /// 그런 타입이 아니면 null 이다. 움직인 적이 없으면(방향 -1) 첫 클러스터다 — 녹화의 맵 저장 Sail Skater 가 0번 그림이었다.
    /// 움직이는 동안 방향 묶음의 4장을 약 12Hz 로 돌리는 것은 추정이다(녹화에 이동 장면이 없다).
    /// </summary>
    /// <param name="type">유닛 타입</param>
    /// <param name="heading">바라보는 8방향 (0 = 북, 시계 방향. 움직인 적이 없으면 음수)</param>
    /// <param name="moving">지금 이동 중인지</param>
    /// <param name="seconds">게임 경과 시간(초)</param>
    public static int? SailFrame(TypeDefinition type, int heading, bool moving, double seconds)
    {
        IReadOnlyList<FrameCode> codes = type.Frames.Codes;
        int last = SailGroupStart.Max() + SailGroupSize;
        if (codes.Count < last) return null;
        // 방향 묶음에 쓰는 앞쪽 클러스터의 이름이 모두 같아야 이 규칙의 타입이다
        for (int i = 1; i < last; i++)
        {
            if (codes[i].Side != codes[0].Side || codes[i].Variant != codes[0].Variant || codes[i].Number != codes[0].Number) return null;
        }
        if (heading < 0 || heading >= SailGroupStart.Length) return 0;
        int start = SailGroupStart[heading];
        return start + (moving ? LoopIndex(seconds, SwirlFrameSeconds, SailGroupSize) : 0);
    }
}
