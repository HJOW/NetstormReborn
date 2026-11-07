// Session보다 오래 사는 실제 풀·공간·자산 배열의 저장소 껍데기와 스캔 상태를 공유한다.
namespace Netstorm.Core.Original;

// 서피스 저장소 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.

// 00422490/00422bc0의 스캔 상태다. First부터 Last까지 양 끝을 포함한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.
public sealed class BridgeDecayScanState
{
    /// <summary>훑는 번호 범위의 첫 번호다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public int First { get; set; }

    /// <summary>훑는 번호 범위의 마지막 번호다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public int Last { get; set; }

    /// <summary>다음에 훑을 번호(커서)다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public int Cursor { get; set; }

    /// <summary>지금 주기가 끝나는 게임 시각(초)이다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public double Next { get; set; }
}

// Session보다 오래 사는 실제 저장소의 진단·바람·대여 껍데기다. 풀·해시·spot·그래프·집계·섬·청크 물리와 장기 ProcessHost·실제 소진 진단은 전체 생성 단위에서 잇는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.
public sealed class RawSurfaceStorage
{
    /// <summary>기존 실제 월드의 SID 풀 크기이며 다음 대여에서 재확보하지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public const int PoolCapacity = 32768;

    /// <summary>원본 256×256 spot 배열의 실제 바이트 수다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public const int SpotCells = 65536;

    /// <summary>실제 사제 SID 목록과 별개인 일반 Session 지역 9DWORD의 칸 수다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public const int PriestOwnerCount = 9;

    /// <summary>0052f968의 원본 최초 float이며 Native 정리는 이 현재 값을 읽고 덮어쓰지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public const float BridgeDecayPeriodDefault = 10.0f;

    /// <summary>같은 Client 수명의 바람이다. 대여·반납에서 재초기화하지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public AmbientWindState Wind { get; } = new();

    /// <summary>005c8458의 최초 BSS 0이며 모든 월드가 같은 금지 DWORD를 빌린다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint DisallowSquidAllocation { get; set; }

    /// <summary>미니맵과 현재 월드가 같은 실제 세 모드 값을 읽는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public SquidOwnerMode OwnerMode { get; } = new();

    /// <summary>생산 UI와 현재 월드가 같은 실제 표면 종류 전역을 읽는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public BridgeConnectState Links { get; } = new();

    /// <summary>00595428부터 9DWORD의 최초 BSS 0이다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint[] PriestOwnerState { get; } = new uint[PriestOwnerCount];

    /// <summary>545370/545374 실패점을 Pool과 같은 장기 수명에 둔다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public BoardTargetState BoardTargetState { get; } = new();

    /// <summary>0052f96c·970·005453a8·b8의 같은 현재 스캔 범위·커서·기한을 World 반납 뒤에도 유지한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public BridgeDecayScanState BridgeDecayScan { get; set; } = new();

    /// <summary>현재 스캔 주기(초)다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public float BridgeDecayPeriod { get; set; } = BridgeDecayPeriodDefault;

    /// <summary>Pool의 현재 서버 포인터와 World 실행기보다 먼저 만드는 실제 권한 owner다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly RawAuthorityState _authority;

    /// <summary>실제 장부·게임 TYPE 참조보다 먼저 구성하는 실제 게임 보조 owner다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly RawGameAuxState _gameAux;

    /// <summary>바람 진행이 판본별 잡음 순서를 고를 때 읽는 판본이다. 풀 생성 단위에서 같은 판본의 풀로 잇는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>대여·World 반납 뒤에도 같은 현재 Client 진단 DWORD를 읽는 뷰다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private RawCheckingFlag _checking = new();

    /// <summary>같은 Client의 상세 owner를 빌린다. Storage보다 오래 살아야 한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private StringBox? _displayDetail;

    /// <summary>현재 표시 진단 장치이며 World·Session 콜백을 보관하지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private Action<string, string, uint>? _displayAssertion;

    /// <summary>Client 수명 동안 현재 게임 시각을 읽는 장치다. World 대여·반납은 교체하지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private Func<double>? _graphClock;

    /// <summary>실제 Client의 현재 로그 장치를 빌리며 World보다 오래 살아 있다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private Action<int, string>? _graphTrace;

    /// <summary>CD 소진에서만 선택하는 현재 Client 객체 덤프 장치다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private Action? _allocationDump;

    /// <summary>CD 회수 뒤 같은 Client 공통 Tell 부모를 호출하는 장치다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private Action<string>? _allocationTell;

    /// <summary>현재 실제 월드가 물리를 빌렸는지다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private bool _borrowed;

    /// <summary>성공한 월드 대여 횟수이며 원본 게임 값으로 쓰지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    private ulong _generation;

    /// <summary>배열은 전체 생성 단위에서 최초 한 번만 확보한다. 이 껍데기는 진단·바람·대여와 스칼라 전역만 둔다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="edition">바람 잡음 순서와 스캔 초기값이 읽는 판본이다.</param>
    /// <param name="authority">빌릴 실제 권한 owner다. null이면 자체 owner를 만든다.</param>
    /// <param name="gameAux">빌릴 실제 게임 보조 owner다. null이면 자체 owner를 만든다.</param>
    public RawSurfaceStorage(OriginalEdition edition = OriginalEdition.Patch1078, RawAuthorityState? authority = null, RawGameAuxState? gameAux = null)
    {
        _edition = edition;
        _authority = authority ?? new RawAuthorityState();
        _gameAux = gameAux ?? new RawGameAuxState();
        Links.BridgeType = 82u;
        Links.IslandType = 94u;
        Links.StalagType = 95u;
        Links.ConnectorType = 155u;
        Links.NoIslandType = 157u;
        BridgeDecayScan = new BridgeDecayScanState
        {
            First = edition == OriginalEdition.Patch1078 ? -1 : 0,
            Last = edition == OriginalEdition.Patch1078 ? -1 : 0,
        };
    }

    /// <summary>Client 전역·Session과 같은 실제 서버·권한 주소의 정체성이다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>빌리거나 자체 만든 권한 owner다.</returns>
    public RawAuthorityState Authority() => _authority;

    /// <summary>같은 Gameplay·Host가 pin하는 실제 게임 보조 owner다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>빌리거나 자체 만든 게임 보조 owner다.</returns>
    public RawGameAuxState GameAux() => _gameAux;

    /// <summary>모든 실제 타입 소비자의 현재 DWORD다. 대여·반납으로 타입 주소를 교체하지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint GeyserType
    {
        get => _gameAux.GeyserType;
        set => _gameAux.GeyserType = value;
    }

    /// <summary>비소유 현재 뷰 또는 독립 자체값을 수명 안전하게 복사한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="checking">복사할 검사 뷰다.</param>
    public void BindChecking(RawCheckingFlag checking)
    {
        ArgumentNullException.ThrowIfNull(checking);
        _checking = new RawCheckingFlag(checking);
    }

    /// <summary>호출 위치에서 현재 owner를 읽을 수 있는 같은 뷰다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>현재 검사 뷰다.</returns>
    public RawCheckingFlag Checking() => _checking;

    /// <summary>첫 World 이전에 같은 Client 상세·진단 수명을 연결한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="detail">같은 Client의 상세 owner 상자다. Storage보다 오래 살아야 한다.</param>
    /// <param name="assertion">현재 표시 진단 장치다.</param>
    public void BindDisplayDiagnostics(StringBox detail, Action<string, string, uint> assertion)
    {
        ArgumentNullException.ThrowIfNull(detail);
        ArgumentNullException.ThrowIfNull(assertion);
        _displayDetail = detail;
        _displayAssertion = assertion;
    }

    /// <summary>00436d20처럼 전달된 문구를 같은 Client 상세 owner에 복사하며 원문 포인터를 보관하지 않는다. 장치가 없는 독립 저장소는 명시 실패한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="text">복사할 문구다.</param>
    public void RecordAssertionDetail(string text)
    {
        if (_displayDetail is null)
        {
            throw new InvalidOperationException("오류 상세 Client owner 미연결");
        }

        _displayDetail.Value = text;
    }

    /// <summary>선택된 원문 진단 장치가 복귀하면 caller도 계속한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="expression">원문 식이다.</param>
    /// <param name="file">원문 파일이다.</param>
    /// <param name="line">원문 행이다.</param>
    public void AssertDisplay(string expression, string file, uint line)
    {
        if (_displayAssertion is null)
        {
            throw new InvalidOperationException("표시 원문 진단 장치 미연결");
        }

        _displayAssertion(expression, file, line);
    }

    /// <summary>CD 소진의 객체 덤프와 Tell을 World보다 긴 Client 수명으로 연결한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="dump">현재 Client 객체 덤프 장치다.</param>
    /// <param name="tell">현재 Client Tell 장치다.</param>
    public void BindAllocationDiagnostics(Action dump, Action<string> tell)
    {
        ArgumentNullException.ThrowIfNull(dump);
        ArgumentNullException.ThrowIfNull(tell);
        _allocationDump = dump;
        _allocationTell = tell;
    }

    /// <summary>첫 World 전에 현재 Client의 로그 writer를 연결한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="trace">현재 로그 장치다.</param>
    public void BindGraphTrace(Action<int, string> trace)
    {
        ArgumentNullException.ThrowIfNull(trace);
        _graphTrace = trace;
    }

    /// <summary>같은 Client GameClock 장치를 장기 owner가 빌린다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="clock">현재 게임 시각을 읽는 장치다.</param>
    public void BindGraphClock(Func<double> clock)
    {
        ArgumentNullException.ThrowIfNull(clock);
        _graphClock = clock;
    }

    /// <summary>독립 저장소는 0이며 실제 장치는 현재 정지·누적 시간을 반영한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>현재 그래프 시각이다.</returns>
    public double ReadGraphClock() => _graphClock is not null ? _graphClock() : 0.0;

    /// <summary>현재 장치가 있으면 원문 수준·텍스트를 그대로 전달한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="level">원문 수준이다.</param>
    /// <param name="text">원문 텍스트다.</param>
    public void TraceGraph(int level, string text)
    {
        _graphTrace?.Invoke(level, text);
    }

    /// <summary>매 프레임 동일 게임 난수를 직접 소비하며 Session 또는 별도 난수 사본을 만들지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <param name="random">같은 Client 난수다.</param>
    public void UpdateAmbientWind(GameRandom random)
    {
        ArgumentNullException.ThrowIfNull(random);
        RawAmbientWind.Step(Wind, random, _edition, AngleDiagnostics());
    }

    /// <summary>각도 부모는 같은 현재 검사와 같은 실제 Client 원문 진단 장치를 값 뷰로 보관한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>현재 검사·진단을 빌리는 각도 진단 묶음이다.</returns>
    public BoardAngleDiagnostics AngleDiagnostics() => new()
    {
        Checking = new RawCheckingFlag(_checking),
        Assertion = (expression, file, line) => AssertDisplay(expression, file, line),
    };

    /// <summary>현재 실제 월드가 물리를 빌렸는지 읽는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>대여 중이면 참이다.</returns>
    public bool Borrowed() => _borrowed;

    /// <summary>성공한 월드 대여 횟수이며 원본 게임 값으로 쓰지 않는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>대여 횟수다.</returns>
    public ulong Generation() => _generation;

    /// <summary>앞선 명시 정리가 끝난 실제 배열을 초기화 없이 대여하며 동시 사용은 같은 관문에서 거부한다. 전체 Acquire의 풀·해시·spot·그래프·집계·섬·청크 재초기화는 전체 생성 단위에서 잇는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    /// <returns>반납 경계다.</returns>
    public Use AcquirePrepared()
    {
        if (_borrowed)
        {
            throw new InvalidOperationException("준비된 실제 월드 저장소 중복 대여");
        }

        var use = new Use(this);
        _borrowed = true;
        ++_generation;
        return use;
    }

    /// <summary>대여 중에는 두 번째 월드가 같은 풀을 초기화하지 못하게 한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public sealed class Use : IDisposable
    {
        /// <summary>빌린 저장소다. 반납 뒤에는 null이다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
        private RawSurfaceStorage? _storage;

        /// <summary>이미 검증한 단일 소유자의 반납 경계다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
        /// <param name="storage">빌린 저장소다.</param>
        internal Use(RawSurfaceStorage storage)
        {
            _storage = storage;
        }

        /// <summary>월드의 모든 모듈을 반납한 뒤 대여만 해제한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
        public void Dispose()
        {
            if (_storage is not null)
            {
                _storage._borrowed = false;
                _storage = null;
            }
        }
    }
}
