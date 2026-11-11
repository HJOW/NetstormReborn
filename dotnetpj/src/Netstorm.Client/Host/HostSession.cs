using Netstorm.Client.Automation;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Host;

// 미션 세션의 호스트 소유자다. Kernel·난수는 빌리고 저장소는 소유한다.
public sealed class HostSession
{
    /// <summary>미션 풀 슬롯 수다. L2 thewarbegins 기준선 덤프의 pool 행과 같은 값이다.</summary>
    /// <remarks>2026-11-12 0.392에서 world 1 덤프의 첫 관문으로 고정했다. 미션별 가변은 월드 조립에서 잇는다.</remarks>
    public const int MissionPoolCapacity = 32768;

    /// <summary>Player 기록 바이트 수다(9기록×180바이트, cpppj kPlayerBytes).</summary>
    public const int PlayerRecordBytes = 9 * 180;

    /// <summary>동맹 표 DWORD 수다(9×9).</summary>
    public const int AllianceDwords = 9 * 9;

    /// <summary>덤프와 세션이 함께 읽는 현재 Kernel이다. 호출자가 소유한다.</summary>
    public Kernel Kernel { get; }

    /// <summary>덤프와 세션이 함께 읽는 게임 난수다. 호출자가 소유한다.</summary>
    public GameRandom Game { get; }

    /// <summary>덤프와 세션이 함께 읽는 CRT 난수다. 호출자가 소유한다.</summary>
    public CrtRandom Crt { get; }

    /// <summary>세션이 소유한 오브젝트 풀이다. 싱글 플레이는 로컬이 서버다.</summary>
    public RawSidPool Pool { get; }

    /// <summary>세션이 소유한 Player 9기록 원문이다. Start 순서에서 RawPlayerParticipation과 같은 버퍼를 쓴다.</summary>
    public byte[] PlayerRecords { get; } = new byte[PlayerRecordBytes];

    /// <summary>세션이 소유한 Chat 숨김 9바이트다. Start 순서에서 같은 버퍼를 쓴다.</summary>
    public byte[] HiddenPlayers { get; } = new byte[9];

    /// <summary>세션이 소유한 동맹 81 DWORD다. Start 순서에서 같은 버퍼를 쓴다.</summary>
    public uint[] Alliances { get; } = new uint[AllianceDwords];

    /// <summary>SP 저장소가 쓰는 전역 난수다. Start 순서에서 실제 난수에 연결한다.</summary>
    private readonly NetstormRandom _spRandom = new();

    /// <summary>SP 초기화의 재시드 공급이다. 없으면 시작 SP 지정이 거부된다.</summary>
    /// <remarks>2026-11-12 0.395에서 StartPlayerSp 연결로 추가했다. 실제 공급(RawGameRandomSeed 사슬)은 미션 시작 흐름에서 잇는다.</remarks>
    private Func<uint>? _spReseed;

    /// <summary>세션이 소유한 분산 SP 저장소다. 초기화 전에는 덤프에 대시를 쓴다.</summary>
    public RawSpStore Sp { get; }

    /// <summary>세션이 소유한 Graph 표다. 월드 적재의 ResetTable로 초기화한다.</summary>
    public RawGraphStorage Graph { get; } = new();

    /// <summary>세션이 소유한 TotalMade 세 표다.</summary>
    public RawTotalMadeStorage TotalMade { get; } = new();

    /// <summary>장부의 총 비용이다. 월드 장부(포스트팝) 연결 전까지 0이다.</summary>
    public int TotalCost { get; set; }

    /// <summary>장부의 postPop 깊이다. 월드 장부 연결 전까지 0이다.</summary>
    public uint PostPopDepth { get; set; }

    /// <summary>장부의 생산 재계산 알림이다. 월드 장부 연결 전까지 0이다.</summary>
    public uint ProductionDirty { get; set; }

    /// <summary>세션이 소유한 인공지능 600바이트 설정이다. 키는 소유자 번호다. 난이도 배율은 생성 때 적용하므로 원본 그대로 둔다.</summary>
    public Dictionary<int, byte[]> AiSettings { get; } = [];

    /// <summary>같은 Kernel·난수를 빌리고 저장소를 새로 만든다.</summary>
    /// <param name="kernel">현재 Kernel.</param>
    /// <param name="game">게임 난수.</param>
    /// <param name="crt">CRT 난수.</param>
    /// <param name="poolCapacity">풀 슬롯 수. 0 이하면 미션 기본값이다.</param>
    public HostSession(Kernel kernel, GameRandom game, CrtRandom crt, int poolCapacity = 0)
    {
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(crt);
        Kernel = kernel;
        Game = game;
        Crt = crt;
        Pool = new RawSidPool(poolCapacity <= 0 ? MissionPoolCapacity : poolCapacity);
        Sp = new RawSpStore(_spRandom, () => _spReseed is not null ? _spReseed() : throw new InvalidOperationException("SP 재시드 미연결"));
    }

    /// <summary>SP 초기화의 재시드 공급을 연결한다. 미션 시작 흐름에서 실제 사슬로 잇는다.</summary>
    /// <param name="reseed">재시드 값을 주는 콜백.</param>
    public void BindSpReseed(Func<uint> reseed) => _spReseed = reseed ?? throw new ArgumentNullException(nameof(reseed));

    /// <summary>참가자의 시작 SP를 미션 목록 값으로 지정한다. 원본 로그인 setSp와 같은 분산 저장소를 쓴다.</summary>
    /// <param name="players">미션 플레이어 목록.</param>
    /// <remarks>2026-11-12 0.395에서 미션 값 채우기로 추가했다. 읽기는 로컬 활성·1번 owner·비편집 고정이며(세션 전역 연결은 다음 단위),
    /// 표시는 바꾸지 않는다. 활성 참가자만 지정한다.</remarks>
    public void StartPlayerSp(MissionPlayers players)
    {
        ArgumentNullException.ThrowIfNull(players);
        var reads = new PlayerSpSetReads { ClientActive = () => 1, LocalOwner = () => 1, Editor = () => 0 };
        // 활성 참가자만 번호 순서대로 지정한다.
        for (int owner = 1; owner <= MissionPlayers.PlayerCount; owner++)
        {
            if (!players.Players[owner].Active) continue;
            RawFortMoneyLoad.SetPlayerSpAmount(owner, unchecked((int)players.Players[owner].StormPower), Sp, reads, () => { });
        }
    }

    /// <summary>인공지능 소유자의 600바이트 설정을 미션 대본 값으로 채운다. 원본 미션 Start의 설정 준비와 같다.</summary>
    /// <param name="players">미션 플레이어 목록.</param>
    /// <param name="get">미션 설정 조회. 없으면 null이다.</param>
    /// <remarks>2026-11-12 0.396에서 미션 값 채우기로 추가했다. 인공지능(활성·전산) 소유자만 만들며,
    /// 11차 난이도 배율은 생성 때(RawAiCreate) 적용하므로 여기서는 원본 그대로 둔다. AI 생성 본체 연결은 다음 단위다.</remarks>
    public void PrepareAiSettings(MissionPlayers players, Func<string, string?> get)
    {
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(get);
        var state = new MissionAiSettingsState();
        var hooks = new MissionAiSettingsHooks
        {
            Get = key => get(key) ?? "",
            ParseFloat = ParseAtopFloat,
            UnknownAbilityAssertion = name => throw new InvalidOperationException("미등록 AI 능력 assert: " + name),
            UnknownAbilityLog = name => throw new InvalidOperationException("미등록 AI 능력 로그: " + name),
        };
        var settings = new RawMissionAiSettings(OriginalEdition.Patch1078, state, hooks);
        // 인공지능 소유자만 번호 순서대로 만든다.
        for (int owner = 1; owner <= MissionPlayers.PlayerCount; owner++)
        {
            if (!players.Players[owner].Active || !players.Players[owner].Computer) continue;
            var raw = new byte[RawMissionAiSettings.Size];
            settings.Prepare(raw, owner);
            AiSettings[owner] = raw;
        }
    }

    /// <summary>원본 atof처럼 앞의 숫자를 실수로 읽는다. RawMissionPlayers의 같은 규칙이다.</summary>
    private static double ParseAtopFloat(string? value)
    {
        if (value is null) return 0;
        int position = 0;
        // 앞의 공백류를 건너뛴다.
        while (position < value.Length && char.IsWhiteSpace(value[position])) position++;
        int start = position;
        // 부호와 숫자·소수점·지수 형태를 앞에서부터 읽는다.
        if (position < value.Length && value[position] is '-' or '+') position++;
        bool dot = false;
        while (position < value.Length && (char.IsAsciiDigit(value[position]) || (!dot && value[position] == '.')))
        {
            if (value[position] == '.') dot = true;
            position++;
        }
        if (position < value.Length && value[position] is 'e' or 'E')
        {
            int tail = position + 1;
            if (tail < value.Length && value[tail] is '-' or '+') tail++;
            int digits = tail;
            while (digits < value.Length && char.IsAsciiDigit(value[digits])) digits++;
            if (digits > tail) position = digits;
        }
        if (position == start) return 0;
        if (double.TryParse(value[start..position], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out double number)) return number;
        return 0;
    }

    /// <summary>덤프용 Player 관측을 만든다. Start 순서 전까지 0이다.</summary>
    public SessionDumpPlayers DumpPlayers() =>
        new((byte[])PlayerRecords.Clone(), (byte[])HiddenPlayers.Clone(), (uint[])Alliances.Clone());

    /// <summary>덤프용 SP 관측을 만든다. 초기화 전에는 대시 9칸으로 나간다.</summary>
    public SessionDumpSp DumpSp() =>
        new(Sp.Initialized, Sp.Offset, Sp.Pools.ToArray(), Sp.Permutation.ToArray());

    /// <summary>덤프용 Graph 표 관측을 만든다. 월드 적재 ResetTable 전까지 0이다.</summary>
    public IReadOnlyList<SessionDumpGraphRow> DumpGraph()
    {
        var rows = new SessionDumpGraphRow[Graph.Records.Length];
        // 표 전체를 순서대로 복사한다.
        for (int index = 0; index < rows.Length; index++)
            rows[index] = new(Graph.Records[index].Surfaces, Graph.Records[index].InUse, Graph.Records[index].Reserved);
        return rows;
    }

    /// <summary>덤프용 장부 관측을 만든다. 월드 장부 연결 전까지 0이다.</summary>
    public SessionDumpTotals DumpTotals() => new(TotalCost, PostPopDepth, ProductionDirty);

    /// <summary>덤프용 TotalMade 관측을 만든다. 집계 전까지 빈 표다.</summary>
    public SessionDumpTotalMade DumpTotalMade() =>
        new((uint[])TotalMade.LocalCounts.Clone(), (uint[])TotalMade.LocalSecondaryCounts.Clone(), (uint[])TotalMade.GlobalCounts.Clone());

    /// <summary>덤프용 풀 관측을 만든다. 예약 0~4번과 사용 중 슬롯만 담고 빈 칸은 제외한다.</summary>
    /// <returns>풀 행·슬롯 행에 쓰는 관측.</returns>
    /// <remarks>제외 규칙은 cpppj 덤프와 같다(id ≥ 5이면서 free 비트가 켜진 칸은 pool 해시로만 비교한다).</remarks>
    public SessionDumpPool DumpPool()
    {
        var slots = new List<SessionDumpSlot>();
        // 번호 순서대로 읽으며 빈 칸은 건너뛴다.
        for (int sid = 0; sid < Pool.Capacity; sid++)
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            if (sid >= RawSidPool.FirstOrdinary && (raw[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            slots.Add(new SessionDumpSlot(unchecked((uint)sid), raw.ToArray()));
        }
        return new SessionDumpPool(
            Pool.Capacity,
            RawSidPool.Stride,
            Pool.FreeCount,
            unchecked((uint)Pool.FirstFree(false)),
            unchecked((uint)Pool.FirstFree(true)),
            Pool.PredictableCursor,
            Pool.Bytes.ToArray(),
            slots);
    }
}
