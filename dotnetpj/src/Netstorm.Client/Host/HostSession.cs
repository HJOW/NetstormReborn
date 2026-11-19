using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Automation;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Host;

// 참가자 가입의 실제 외부 효과 경계다. 2026-11-12 R4 실제 호스트·월드 조립(0.403)에서 추가됐다.
public sealed class PlayerJoinHooks
{
    /// <summary>로컬이 아닌 소유자의 접속 주소를 읽는다. 2026-11-12 R4 실제 호스트·월드 조립(0.403)에서 추가됐다.</summary>
    public Func<uint, string>? PeerAddress { get; set; }

    /// <summary>가입·재전송 대상에게 Player 기록을 보낸다. 2026-11-12 R4 실제 호스트·월드 조립(0.403)에서 추가됐다.</summary>
    public Action<uint, uint>? SendPlayer { get; set; }
}

// 준비 마스크 packet의 실제 외부 효과 경계다. 2026-11-12 R4 실제 호스트·월드 조립(0.405)에서 추가됐다.
public sealed class PlayerReadyHooks
{
    /// <summary>방장 여부다. 거짓이면 원본처럼 boss 진단을 낸다. 2026-11-12 R4 실제 호스트·월드 조립(0.405)에서 추가됐다.</summary>
    public bool IsBoss { get; set; }

    /// <summary>방장 아님 진단을 받는다. 원본의 assertion(boss)과 같다. 2026-11-12 R4 실제 호스트·월드 조립(0.405)에서 추가됐다.</summary>
    public Action<string>? Assertion { get; set; }

    /// <summary>대상 0일 때 전체에 packet을 뿌린다. 2026-11-12 R4 실제 호스트·월드 조립(0.405)에서 추가됐다.</summary>
    public Action<byte[]>? Broadcast { get; set; }

    /// <summary>대상이 있을 때 그 대상에 packet을 보낸다. 2026-11-12 R4 실제 호스트·월드 조립(0.405)에서 추가됐다.</summary>
    public Action<uint, byte[]>? Send { get; set; }
}

// 미션 세션의 호스트 소유자다. Kernel·난수는 빌리고 저장소는 소유한다.
public sealed partial class HostSession
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

    /// <summary>세션이 쓰는 오브젝트 풀이다. 빌린 풀이 없으면 세션 전용으로 만든다. 싱글 플레이는 로컬이 서버다. 0.594에서 외부 풀 빌림을 추가했다.</summary>
    public RawSidPool Pool { get; }

    /// <summary>세션이 소유한 Player 9기록 원문이다. Start 순서에서 RawPlayerParticipation과 같은 버퍼를 쓴다.</summary>
    public byte[] PlayerRecords { get; } = new byte[PlayerRecordBytes];

    /// <summary>세션이 소유한 Chat 숨김 9바이트다. Start 순서에서 같은 버퍼를 쓴다.</summary>
    public byte[] HiddenPlayers { get; } = new byte[9];

    /// <summary>세션이 소유한 동맹 81 DWORD다. Start 순서에서 같은 버퍼를 쓴다.</summary>
    public uint[] Alliances { get; } = new uint[AllianceDwords];

    /// <summary>SP 초기화의 재시드 공급이다. 없으면 시작 SP 지정이 거부된다.</summary>
    /// <remarks>2026-11-12 0.395에서 StartPlayerSp 연결로 추가했다. 실제 공급(RawGameRandomSeed 사슬)은 미션 시작 흐름에서 잇는다.</remarks>
    private Func<uint>? _spReseed;

    /// <summary>세션이 소유한 분산 SP 저장소다. 초기화 전에는 덤프에 대시를 쓴다. 원본처럼 게임 공유 난수를 쓴다.</summary>
    /// <remarks>2026-11-12 0.457에서 단일 공유 스트림으로 바꿨다(근거 cpppj RawCampaignSession.cpp 498행, 995행).
    /// 2026-11-12 0.464에서 호스트 시작 1회 초기화를 잇려고 공유 저장소 빌림을 추가했다(원본 004391ed, cpppj ClientMain 349행).</remarks>
    public RawSpStore Sp { get; }

    /// <summary>세션이 소유한 Graph 표다. 세션 설치(생성자)의 ResetTable로 초기 상태를 둔다.</summary>
    /// <remarks>2026-11-12 0.476에서 생성자 초기화를 잇었다(근거 cpppj RawSurfaceStorage.Acquire의 Graph ResetTable). 적재 경로의 Rebuild는 무효 번호 수집만 하며 번호 부여는 Pop 그래프 연결(R6)에서 잇는다.</remarks>
    public RawGraphStorage Graph { get; } = new();

    /// <summary>세션이 소유한 TotalMade 세 표다.</summary>
    public RawTotalMadeStorage TotalMade { get; } = new();

    /// <summary>공통 PostPop과 덤프가 공유하는 총 비용이다.</summary>
    public int TotalCost { get => TerrainBookkeeping.TotalCost; set => TerrainBookkeeping.TotalCost = value; }

    /// <summary>공통 PostPop과 덤프가 공유하는 중첩 깊이다.</summary>
    public uint PostPopDepth { get => TerrainBookkeeping.Depth; set => TerrainBookkeeping.Depth = value; }

    /// <summary>공통 PostPop과 덤프가 공유하는 생산 재계산 알림이다.</summary>
    public uint ProductionDirty { get => TerrainBookkeeping.ProductionDirty; set => TerrainBookkeeping.ProductionDirty = value; }

    /// <summary>세션이 소유한 인공지능 600바이트 설정이다. 키는 소유자 번호다. 난이도 배율은 생성 때 적용하므로 원본 그대로 둔다.</summary>
    public Dictionary<int, byte[]> AiSettings { get; } = [];

    /// <summary>세션이 만든 인공지능 brain 토큰이다. 키는 소유자 번호다. 실제 생성 전에는 비어 있다.</summary>
    /// <remarks>2026-11-12 0.399에서 AI 생성 본체 연결로 추가했다. 실제 brain 생성자·초기 갱신은 호출자가
    /// 주입한 효과로 받으며(월드 본체 연결 전까지 기록용 스텁), 같은 Player 기록에 부착한다.
    /// 라이브 호스트 시작 흐름 연결은 월드 효과와 함께 다음 단위다.</remarks>
    public Dictionary<uint, uint> AiBrains { get; } = [];

    /// <summary>세션이 소유한 실제 AI 본체 메모리다. 생성자·직접 명령·전략 본체가 같은 토큰을 공유한다.</summary>
    /// <remarks>2026-11-19 0.638에서 R4-5 AI 직접 명령 세션 결선용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 1001행의 aiMemory(첫 토큰 0x20000000)다. 원문은 할당 입력을 호스트 효과에서 받지만 DN 세션은 0으로 채운다. 본체는 풀 슬롯이 아니라 L2 세션 덤프에 직접 나가지 않는다.</remarks>
    public RawAiBrainMemory AiBrainMemory { get; } = new(0x20000000u, static bytes => bytes.Clear());

    /// <summary>같은 Kernel·난수를 빌리고 저장소를 새로 만든다. 공유 SP·CRT 상자를 넘기면 미션 시작 때 재초기화하지 않고 그대로 쓴다.</summary>
    /// <param name="kernel">현재 Kernel.</param>
    /// <param name="game">게임 난수.</param>
    /// <param name="crt">CRT 난수.</param>
    /// <param name="poolCapacity">풀 슬롯 수. 0 이하면 미션 기본값이다.</param>
    /// <param name="sharedSp">호스트가 시작 때 초기화한 공유 SP 저장소. null이면 세션 전용으로 만들고 첫 Set 때 초기화한다.</param>
    /// <param name="sharedCrtBox">호스트가 시작 때 reseed한 공유 CRT 상자. null이면 세션 전용 상자를 쓴다.</param>
    /// <param name="pool">빌릴 외부 오브젝트 풀. null이면 세션 전용으로 만든다. NativeStorage 전환 전까지 개발 호스트의 별도 풀을 유지하며, 넘긴 풀의 수명은 호출자가 세션보다 길게 둔다.</param>
    /// <remarks>2026-11-12 0.464에서 cpppj 공유 SP 경로와 같은 시작 1회 초기화를 잇려고 공유 빌림을 추가했다. 미션별 reseed는 원본에 없다.
    /// 2026-11-12 0.476에서 세션 설치 의미로 Graph 저장소를 ResetTable 초기 상태에 둔다(근거 cpppj RawSurfaceStorage.Acquire의 Graph ResetTable, o/RawGraphStorage.cpp). 0번 예약이 없으면 Rebuild의 임시 번호 0이 기존 graph-0과 충돌해 큰 맵에서 번호가 소진된다.
    /// 0.594에서 NativeStorage 풀 전환의 첫 단계로 외부 풀 빌림을 추가했다. 별도 풀 제거·세션 수명 일원화는 다음 단위다.</remarks>
    public HostSession(Kernel kernel, GameRandom game, CrtRandom crt, int poolCapacity = 0, RawSpStore? sharedSp = null, UIntBox? sharedCrtBox = null, ClientGameplayGlobals? sharedGameplay = null, RawStateSidLists? sharedLists = null, RawSidPool? pool = null)
    {
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(crt);
        Kernel = kernel;
        Game = game;
        Crt = crt;
        _aiParticipation = new PlayerParticipation(
            PlayerRecords, Alliances, HiddenPlayers,
            new Netstorm.Core.Simulation.PlayerParticipationState(),
            new Netstorm.Core.Simulation.PlayerParticipationHooks
            {
                DestructBrain = brain => AiConstructorFor(brain).Destruct(brain),
                FreeBrain = brain =>
                {
                    AiConstructorFor(brain).FreeBody(brain);
                    _aiConstructors.Remove(brain);
                },
            });
        Gameplay = sharedGameplay ?? new ClientGameplayGlobals();
        Pool = pool ?? new RawSidPool(poolCapacity <= 0 ? MissionPoolCapacity : poolCapacity, true, () => Gameplay.Authority.Server);
        if (pool is not null)
        {
            // 빌린 풀은 미션 덤프와 같은 크기·판본이어야 한다. poolCapacity는 빌린 풀이 있을 때 쓰지 않는다.
            if (pool.Edition != OriginalEdition.Patch1078)
            {
                throw new ArgumentException("개발 호스트가 빌리는 외부 풀의 판본 오류", nameof(pool));
            }

            if (pool.Capacity != MissionPoolCapacity)
            {
                throw new ArgumentException("개발 호스트가 빌리는 외부 풀의 크기 오류", nameof(pool));
            }
        }
        Sp = sharedSp ?? new RawSpStore(Game, () => _spReseed is not null ? _spReseed() : throw new InvalidOperationException("SP 재시드 미연결"));
        TerrainCrt = sharedCrtBox ?? new();
        TerrainBookkeeping = new RawPostPopState(TotalMade);
        SidLists = sharedLists ?? new RawStateSidLists();
        PriestState = new(GameAux.PriestTypeBox, GameAux);
        PriestState.Priests.SharedPriests = SidLists.Priests;
        PriestState.Regen.SharedPriestType = GameAux.PriestTypeBox;
        PriestState.Regen.Patch.SharedSample = Gameplay.PatchSample;
        PriestState.Regen.Patch.SharedChangedSample = Gameplay.PatchDiagnostic;
        RawGraphStorage.ResetTable(Graph);
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

    /// <summary>인공지능 소유자의 brain을 실제 생성기에 만들어 같은 Player 기록에 부착한다. 원본 미션 Start의 AI 생성 순서와 같다.</summary>
    /// <param name="players">미션 플레이어 목록. 활성 인공지능 소유자만 만든다.</param>
    /// <param name="hooks">실제 색·월드·생성자·초기 갱신 효과. 월드 본체 연결 전까지 호출자가 스텁으로 준다.</param>
    /// <remarks>2026-11-12 0.399에서 미션 값 채우기로 추가했다. 설정이 없는 인공지능 소유자는 명시 거부한다.
    /// 같은 세션 버퍼를 공유하는 참여 모듈에 부착하므로 Player 덤프 행에 brain·flags가 반영된다.
    /// 라이브 시작 흐름 연결은 월드 효과와 함께 다음 단위다.</remarks>
    public void CreateAiBrains(MissionPlayers players, AiCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(hooks);
        var create = new RawAiCreate(OriginalEdition.Patch1078, _aiParticipation, hooks);
        // 인공지능(활성·전산) 소유자만 번호 순서대로 만든다.
        for (int owner = 1; owner <= MissionPlayers.PlayerCount; owner++)
        {
            if (!players.Players[owner].Active || !players.Players[owner].Computer) continue;
            if (!AiSettings.TryGetValue(owner, out byte[]? settings))
                throw new InvalidOperationException("인공지능 설정 없이 생성할 수 없다: " + owner);
            AiBrains[(uint)owner] = create.Create((uint)owner, settings);
        }
    }

    /// <summary>같은 현재 Player 기록과 세션 소유 본체로 직접 AI 지갑을 고정 30000으로 쓴다.</summary>
    /// <remarks>2026-11-19 0.638에서 R4-5 AI 직접 명령 세션 결선용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 4866행의 CrankAiMoney다. 기존 Credit·데모 하한·분산 SP를 쓰지 않는다.</remarks>
    public void CrankAiMoney()
    {
        RawAiCommandState.CrankAiMoney(new AiCommandStateHooks { Players = () => PlayerRecords, Body = AiBrainMemory.Bytes });
    }

    /// <summary>같은 현재 Player 포인터와 세션 소유 본체를 각 원문 재조회 위치에 연결해 직접 AI 파라미터를 쓴다.</summary>
    /// <param name="packed">소유자·선택기·값을 packed한 원문 인자다. 소유자 0은 아무것도 읽지 않고 반환한다.</param>
    /// <remarks>2026-11-19 0.638에서 R4-5 AI 직접 명령 세션 결선용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 4871행의 SetAiParameter다. 생성자·초기 전략 live 연결은 다음 단위다.</remarks>
    public void SetAiParameter(uint packed)
    {
        if ((packed & 7u) == 0)
        {
            return;
        }

        RawAiCommandState.SetAiParameter(packed, new AiCommandStateHooks { Players = () => PlayerRecords, Body = AiBrainMemory.Bytes });
    }

    /// <summary>Player 기록의 이름 오프셋이다. RawPlayerParticipation과 같은 0xc다.</summary>
    private const int PlayerNameOffset = 0xc;

    /// <summary>Player 기록의 이름 폭이다. RawPlayerParticipation과 같은 32다.</summary>
    private const int PlayerNameWidth = 32;

    /// <summary>Player 기록 한 개의 바이트 수다. RawPlayerParticipation과 같은 0xb4다.</summary>
    private const int PlayerRecordStride = 180;

    /// <summary>Player 기록의 동맹 마스크 오프셋이다. RawPlayerParticipation과 같은 0x54다.</summary>
    private const int PlayerAlliesOffset = 0x54;

    /// <summary>Player 기록의 구독자 DWORD 오프셋이다. RawPlayerParticipation과 같은 0x2c다.</summary>
    private const int PlayerSubscriberOffset = 0x2c;

    /// <summary>Player 기록의 참여 flags DWORD 오프셋이다. RawPlayerParticipation과 같은 0x50이다.</summary>
    private const int PlayerFlagsOffset = 0x50;

    /// <summary>Player 기록의 접속 주소 오프셋이다. RawPlayerParticipation과 같은 0x30이다.</summary>
    private const int PlayerConnectionOffset = 0x30;

    /// <summary>로그인 때 지우는 해제 비트다. RawPlayerParticipation과 같은 0x100이다.</summary>
    private const uint PlayerReleasedFlag = 0x100u;

    /// <summary>준비 표시 비트다. RawMissionLogin과 같은 0x40이다.</summary>
    private const uint PlayerReadyFlag = 0x40u;

    /// <summary>Player 기록의 요새 순서 DWORD 오프셋이다. RawMissionLogin과 같은 0x5c다.</summary>
    private const int PlayerFortOrderOffset = 0x5c;

    /// <summary>Player 기록의 예측 수 DWORD 오프셋이다. RawMissionLogin과 같은 0x60이다. 풀 예측 커서를 둔다.</summary>
    private const int PlayerPredictableOffset = 0x60;

    /// <summary>Player 기록의 요새 수신 표시 DWORD 오프셋이다. RawMissionLogin과 같은 0x64다.</summary>
    private const int PlayerFortReceivedOffset = 0x64;

    /// <summary>Player 기록의 첫 지역 DWORD 오프셋이다. RawPlayerInitialization·RawFortFinishOwner와 같은 0x78이다.</summary>
    /// <remarks>2026-11-12 0.473에서 로그인 초기화 잔여(비참가자 127) 재현용으로 추가했다. 근거: cpppj o/RawPlayerInitialization.cpp 46행·o/RawFortFinishOwner.h 9행(기본 127). 참가자 지역은 전투섬 배정 갈고리·AI 설정 사슬에서 덮어쓰므로(R6·R4 후속) 여기서는 9기록 모두 127로 둔다.</remarks>
    private const int PlayerFirstRegionOffset = 0x78;

    /// <summary>참가자 이름·동맹을 미션 목록 값으로 채운다. 원본 로그인의 기록 쓰기와 같다.</summary>
    /// <param name="players">미션 플레이어 목록.</param>
    /// <param name="localHandle">로컬 1번의 핸들. 대본 이름("Player")이 아니라 실제 핸들을 쓴다.</param>
    /// <remarks>2026-11-12 0.397에서 미션 값 채우기로 추가했다. 동맹 81칸은 원본과 바이트 동일하게 맞는다.
    /// 이름은 Latin1+NUL을 0xc에 쓴다. 1번 바이트 15의 NUL(기준선 "You\0pson")은 로그인 쓰기 순서가 밝혀져야 재현되므로 남긴다.
    /// 핸들 출처(메뉴·설정)는 Start 흐름 단위에서 잇는다.</remarks>
    public void StartPlayers(MissionPlayers players, string localHandle)
    {
        ArgumentNullException.ThrowIfNull(players);
        ArgumentException.ThrowIfNullOrEmpty(localHandle);
        // 동맹 81칸을 채운다. 대각선(자기 자신)은 기준선과 대조로 항상 1이며, 나머지는 목록의 대칭 판정이다.
        // 대각선 작성 주체는 Start 순서 단위에서 잇는다.
        for (int index = 0; index < 9; index++) Alliances[index * 9 + index] = 1u;
        for (int first = 1; first <= MissionPlayers.PlayerCount; first++)
        {
            for (int second = 1; second <= MissionPlayers.PlayerCount; second++)
            {
                if (!players.Allied(first, second)) continue;
                Alliances[first * 9 + second] = 1u;
                Alliances[second * 9 + first] = 1u;
            }
        }
        // 이름은 비어 있지 않을 때만 쓴다. 1번은 대본 이름이 아니라 실제 핸들이다.
        for (int owner = 0; owner <= MissionPlayers.PlayerCount; owner++)
        {
            string name = owner == 1 ? localHandle : players.Players[owner].Name;
            if (name.Length == 0) continue;
            byte[] bytes = System.Text.Encoding.Latin1.GetBytes(name);
            if (bytes.Length + 1 > PlayerNameWidth)
                throw new ArgumentException("참가자 이름이 기록 폭을 넘는다: " + name, nameof(localHandle));
            int offset = owner * PlayerRecordStride;
            Array.Clear(PlayerRecords, offset + PlayerNameOffset, PlayerNameWidth);
            Array.Copy(bytes, 0, PlayerRecords, offset + PlayerNameOffset, bytes.Length);
        }
        // 첫 지역(+0x78)은 원문 초기화와 같이 9기록 모두 127로 둔다. 참가자는 전투섬 배정 갈고리에서 덮어쓴다.
        for (int owner = 0; owner <= MissionPlayers.PlayerCount; owner++)
        {
            int offset = owner * PlayerRecordStride + PlayerFirstRegionOffset;
            PlayerRecords[offset] = 127;
            PlayerRecords[offset + 1] = 0;
            PlayerRecords[offset + 2] = 0;
            PlayerRecords[offset + 3] = 0;
        }
    }

    /// <summary>참가자 기록의 동맹 마스크 DWORD를 미션 목록 값으로 채운다. 원본 로그인의 마스크 쓰기와 같다.</summary>
    /// <param name="players">미션 플레이어 목록. 자기 비트와 대칭 판정 비트를 합친다.</param>
    /// <remarks>2026-11-12 0.400에서 미션 값 채우기로 추가했다. 0번 중립은 목록 판정 밖이라 자기 비트만 둔다.
    /// Chat 표시는 바꾸지 않는다. 구독자·flags·brain은 로그인·생성 흐름 단위에서 잇는다.</remarks>
    public void StartPlayerMasks(MissionPlayers players)
    {
        ArgumentNullException.ThrowIfNull(players);
        // 0번 중립을 포함한 9기록을 번호 순서대로 쓴다.
        for (int owner = 0; owner <= MissionPlayers.PlayerCount; owner++)
        {
            uint mask = 1u << owner;
            // 목록의 대칭 판정 비트를 합친다. 0번은 판정 밖이라 자기 비트만 둔다.
            for (int target = 1; target <= MissionPlayers.PlayerCount; target++)
            {
                if (players.Allied(owner, target)) mask |= 1u << target;
            }

            int offset = owner * PlayerRecordStride + PlayerAlliesOffset;
            PlayerRecords[offset] = (byte)mask;
            PlayerRecords[offset + 1] = (byte)(mask >> 8);
            PlayerRecords[offset + 2] = (byte)(mask >> 16);
            PlayerRecords[offset + 3] = (byte)(mask >> 24);
        }
    }

    /// <summary>참가자 가입의 구독자·flags를 미션 로그인 순서와 같이 쓴다. 원본 로그인의 가입 쓰기와 같다.</summary>
    /// <param name="owner">가입하는 Player 번호다. 0~8이다.</param>
    /// <param name="subscriber">쓸 구독자 번호다.</param>
    /// <param name="localOwner">로컬 소유자 번호다. 같으면 접속 주소를 건너뛴다.</param>
    /// <param name="hooks">도달한 실제 외부 효과다. null이면 도달 효과가 없을 때만 통한다.</param>
    /// <remarks>2026-11-12 0.403에서 미션 값 채우기로 추가했다. 구독자를 먼저 쓰고 접속 주소(로컬 제외)를 둔 뒤 flags를 갱신하며,
    /// 접속·참여 표시가 있는 대상에만 전송 효과를 부른다. Ready 0x40·라이브 연결은 다음 단위다.</remarks>
    public void JoinPlayer(uint owner, uint subscriber, uint localOwner, PlayerJoinHooks? hooks)
    {
        if (owner > (uint)MissionPlayers.PlayerCount)
            throw new ArgumentOutOfRangeException(nameof(owner), "참가자 가입의 실제 Player 번호 범위 오류");
        int record = (int)owner * PlayerRecordStride;
        // 구독자 DWORD를 먼저 쓴다.
        PlayerRecords[record + PlayerSubscriberOffset] = (byte)subscriber;
        PlayerRecords[record + PlayerSubscriberOffset + 1] = (byte)(subscriber >> 8);
        PlayerRecords[record + PlayerSubscriberOffset + 2] = (byte)(subscriber >> 16);
        PlayerRecords[record + PlayerSubscriberOffset + 3] = (byte)(subscriber >> 24);
        if (owner != localOwner)
        {
            // 로컬이 아니면 접속 주소를 기록에 둔다.
            string address = hooks?.PeerAddress is null
                ? throw new InvalidOperationException("참가자 가입의 실제 접속 주소 미연결")
                : hooks.PeerAddress(owner);
            byte[] bytes = System.Text.Encoding.Latin1.GetBytes(address);
            if (bytes.Length + 1 > PlayerRecordStride - PlayerConnectionOffset)
                throw new ArgumentOutOfRangeException(nameof(hooks), "참가자 가입의 실제 접속 주소 범위 오류");
            Array.Copy(bytes, 0, PlayerRecords, record + PlayerConnectionOffset, bytes.Length);
            PlayerRecords[record + PlayerConnectionOffset + bytes.Length] = 0;
        }

        // 해제 비트를 지우고 연결·사람 비트를 켠다.
        uint flags = (uint)(PlayerRecords[record + PlayerFlagsOffset]
            | ((uint)PlayerRecords[record + PlayerFlagsOffset + 1] << 8)
            | ((uint)PlayerRecords[record + PlayerFlagsOffset + 2] << 16)
            | ((uint)PlayerRecords[record + PlayerFlagsOffset + 3] << 24));
        flags = (flags & ~PlayerReleasedFlag) | 3u;
        PlayerRecords[record + PlayerFlagsOffset] = (byte)flags;
        PlayerRecords[record + PlayerFlagsOffset + 1] = (byte)(flags >> 8);
        PlayerRecords[record + PlayerFlagsOffset + 2] = (byte)(flags >> 16);
        PlayerRecords[record + PlayerFlagsOffset + 3] = (byte)(flags >> 24);
        // 접속 표시가 있는 대상에게 가입 기록을 보낸다.
        for (uint target = 1; target <= (uint)MissionPlayers.PlayerCount; target++)
        {
            if ((PlayerRecords[(int)target * PlayerRecordStride + PlayerFlagsOffset] & 1) == 0) continue;
            (hooks?.SendPlayer ?? throw new InvalidOperationException("참가자 가입의 실제 전송 미연결"))(owner, target);
        }

        // 사람 표시가 있는 기록을 새 대상에게 다시 보낸다.
        for (uint player = 1; player <= (uint)MissionPlayers.PlayerCount; player++)
        {
            if ((PlayerRecords[(int)player * PlayerRecordStride + PlayerFlagsOffset] & 2) == 0) continue;
            (hooks?.SendPlayer ?? throw new InvalidOperationException("참가자 가입의 실제 전송 미연결"))(player, owner);
        }
    }

    /// <summary>전투 소유자의 준비 비트를 켠다. 원본 로그인의 Ready 쓰기와 같다.</summary>
    /// <param name="owner">준비된 Player 번호다. 0~8이다.</param>
    /// <remarks>2026-11-12 0.404에서 미션 값 채우기로 추가했다. 0x6e 마스크 packet 송신·방장 관문은 실제 전송과 함께 다음 단위다.</remarks>
    public void MarkReady(uint owner)
    {
        if (owner > (uint)MissionPlayers.PlayerCount)
            throw new ArgumentOutOfRangeException(nameof(owner), "준비 표시의 실제 Player 번호 범위 오류");
        int record = (int)owner * PlayerRecordStride;
        // 준비 비트만 켜고 다른 비트는 보존한다.
        uint flags = (uint)(PlayerRecords[record + PlayerFlagsOffset]
            | ((uint)PlayerRecords[record + PlayerFlagsOffset + 1] << 8)
            | ((uint)PlayerRecords[record + PlayerFlagsOffset + 2] << 16)
            | ((uint)PlayerRecords[record + PlayerFlagsOffset + 3] << 24));
        flags |= PlayerReadyFlag;
        PlayerRecords[record + PlayerFlagsOffset] = (byte)flags;
        PlayerRecords[record + PlayerFlagsOffset + 1] = (byte)(flags >> 8);
        PlayerRecords[record + PlayerFlagsOffset + 2] = (byte)(flags >> 16);
        PlayerRecords[record + PlayerFlagsOffset + 3] = (byte)(flags >> 24);
    }

    /// <summary>준비 비트가 켜진 소유자들의 마스크를 계산한다. 원본 Ready 마스크와 같다.</summary>
    /// <returns>1~8번 중 flags 바이트에 0x40이 있는 소유자 비트 집합이다.</returns>
    /// <remarks>2026-11-12 0.404에서 미션 값 채우기로 추가했다. 실제 0x6e packet 송신은 다음 단위다.</remarks>
    public uint ReadyMask()
    {
        uint mask = 0u;
        // 1~8번의 flags 바이트만 읽는다.
        for (uint owner = 1; owner <= (uint)MissionPlayers.PlayerCount; owner++)
        {
            if ((PlayerRecords[(int)owner * PlayerRecordStride + PlayerFlagsOffset] & 0x40) != 0) mask |= 1u << (int)owner;
        }

        return mask;
    }

    /// <summary>준비 마스크 0x6e packet을 만든다. 원본 로그인의 SendReady 순서와 같다.</summary>
    /// <param name="target">받을 대상이다. 0이면 전체에 뿌린다.</param>
    /// <param name="hooks">도달한 실제 외부 효과다. 방장 관문·전송 효과를 받는다.</param>
    /// <remarks>2026-11-12 0.405에서 미션 값 채우기로 추가했다. packet은 8바이트(01 6e 08 00+마스크 LE)이며,
    /// 방장이 아니면 boss 진단을 낸다. 라이브 시작 흐름 연결은 다음 단위다.</remarks>
    public void SendReady(uint target, PlayerReadyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        // 방장이 아니면 원본처럼 boss 진단을 낸다.
        if (!hooks.IsBoss)
            (hooks.Assertion ?? throw new InvalidOperationException("준비 packet의 실제 진단 미연결"))("boss");
        // packet 8바이트를 만든다.
        var packet = new byte[8];
        packet[0] = 1;
        packet[1] = 0x6e;
        packet[2] = 8;
        packet[3] = 0;
        uint mask = ReadyMask();
        // 마스크 DWORD를 little endian으로 쓴다.
        for (int index = 0; index < 4; ++index)
            packet[4 + index] = unchecked((byte)(mask >> (8 * index)));
        if (target == 0)
            (hooks.Broadcast ?? throw new InvalidOperationException("준비 packet의 실제 뿌리기 미연결"))(packet);
        else
            (hooks.Send ?? throw new InvalidOperationException("준비 packet의 실제 전송 미연결"))(target, packet);
    }

    /// <summary>첫 로그인의 요새 순서를 기록한다. 원본 로그인의 첫 로그인 분기와 같다.</summary>
    /// <param name="owner">새 요새를 받은 Player 번호다. 0~8이다.</param>
    /// <remarks>2026-11-12 0.420에서 미션 값 채우기로 추가했다. 0x5c에 현재 순서 수·0x60에 풀 예측 커서를 쓰고
    /// 순서 수를 올린 뒤 마지막 소유자를 둔다. 요새 이미지·전송 흐름 연결은 다음 단위다.</remarks>
    public void WriteFortOrder(uint owner)
    {
        if (owner > (uint)MissionPlayers.PlayerCount)
            throw new ArgumentOutOfRangeException(nameof(owner), "요새 순서의 실제 Player 번호 범위 오류");
        int record = (int)owner * PlayerRecordStride;
        // 순서 수 DWORD를 little endian으로 쓴다.
        for (int index = 0; index < 4; ++index)
            PlayerRecords[record + PlayerFortOrderOffset + index] = unchecked((byte)(FortCount >> (8 * index)));
        // 예측 수 DWORD는 풀 예측 커서를 little endian으로 쓴다.
        uint predictable = Pool.PredictableCursor;
        for (int index = 0; index < 4; ++index)
            PlayerRecords[record + PlayerPredictableOffset + index] = unchecked((byte)(predictable >> (8 * index)));
        FortCount++;
        LastLoginPlayer = owner;
    }

    /// <summary>요새 수신 표시를 켠다. 원본 로그인의 수신 표식 쓰기와 같다.</summary>
    /// <param name="owner">요새를 받은 Player 번호다. 0~8이다.</param>
    /// <remarks>2026-11-12 0.420에서 미션 값 채우기로 추가했다. 실제 이미지·시각 packet 흐름은 다음 단위다.</remarks>
    public void WriteFortReceived(uint owner)
    {
        if (owner > (uint)MissionPlayers.PlayerCount)
            throw new ArgumentOutOfRangeException(nameof(owner), "요새 수신의 실제 Player 번호 범위 오류");
        int record = (int)owner * PlayerRecordStride;
        // 수신 표시 DWORD를 little endian으로 쓴다.
        for (int index = 0; index < 4; ++index)
            PlayerRecords[record + PlayerFortReceivedOffset + index] = unchecked((byte)(1u >> (8 * index)));
    }

    /// <summary>전투섬 배정 갈고리의 실제 기록 몸체다. 참가자 기록의 첫 지역(+0x78)에 배정된 섬 번호를 쓴다.</summary>
    /// <param name="owner">섬을 배정받은 Player 번호다. 0~8이다.</param>
    /// <param name="island">배정된 섬 번호다.</param>
    /// <remarks>2026-11-12 0.474에서 MP 전투섬 배정 region 갈고리 live 연결용으로 추가했다. 근거: cpppj `RawCampaignSession.cpp` 3243행 `hooks.region`(범위 밖 9 이상은 `out_of_range`). 단일 미션 시작 흐름에서는 호출하지 않는다 — 단일 미션은 missionWorld=true라 loadWorld 경로를 타서 makeBattleIsland가 돌지 않으며, 첫 지역은 소용돌이→섬 소유자→첫 지역 사슬이 기록한다(R6, 0.477 정정).</remarks>
    public void WriteBattleIslandRegion(uint owner, uint island)
    {
        if (owner > (uint)MissionPlayers.PlayerCount)
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "전투섬 배정의 실제 Player 번호 범위 오류");
        int offset = (int)owner * PlayerRecordStride + PlayerFirstRegionOffset;
        // 섬 번호 DWORD를 little endian으로 쓴다.
        for (int index = 0; index < 4; ++index)
            PlayerRecords[offset + index] = unchecked((byte)(island >> (8 * index)));
    }

    /// <summary>전투섬 배정 갈고리의 실제 Region 대리자를 만든다. 세션 Player 기록에 직접 쓴다.</summary>
    /// <returns>참가자·섬 번호를 받는 Region 갈고리다. 같은 세션 인스턴스를 공유한다.</returns>
    /// <remarks>2026-11-12 0.474에서 추가했다. MP 전투의 `BattleIslandHooks.Region` 자리에 그대로 꽂을 수 있으며 미연결 `RawBattleIsland` 호출자는 이 대리자를 그대로 쓴다.</remarks>
    public Action<uint, uint> BuildBattleIslandRegionHook() => WriteBattleIslandRegion;

    /// <summary>전투 Territory 120바이트에서 전투 소유자의 시작 섬 번호를 푼다. 배치 순서대로 셈한다.</summary>
    /// <param name="territory">20개 Territory(120바이트)다. 미션 요새의 전투 소유자 슬롯 값이다.</param>
    /// <param name="island">배정된 섬 번호다. 고른 영역이 배치 불가면 0이다.</param>
    /// <returns>고른 영역이 배치 가능하면 true, 아니면 false다.</returns>
    /// <remarks>2026-11-12 0.474에서 MP 전투섬 배정용으로 추가했다. 고른 영역은 `RawBattleIsland.SelectBattleTerritory`와 같은 규칙(0x04 첫 영역·없으면 0번)이며 섬 번호는 `RawIslandBuilder.PlaceAll`과 같은 배치 순서(앞선 배치 가능 레코드 수, flags bit0 켜짐·bit1 꺼짐)다. 배치 불가면 127 초기값을 그대로 두려고 false를 돌려준다. 단일 미션 시작 흐름에서는 호출하지 않는다(0.477 정정).</remarks>
    public static bool TryResolveBattleIslandRegion(byte[] territory, out uint island)
    {
        island = 0u;
        if (territory is null || territory.Length < 20 * 6)
        {
            return false;
        }

        int selected = RawBattleIsland.SelectBattleTerritory(territory);
        // 고른 영역의 배치 가능 여부를 PlaceAll과 같은 조건으로 확인하는 검사다.
        byte flags = territory[selected * 6 + 1];
        if ((flags & 1) == 0 || (flags & 2) != 0)
        {
            return false;
        }

        uint placed = 0u;
        // 고른 영역보다 앞선 배치 가능 레코드 수를 세어 섬 번호로 삼는 반복문이다.
        for (int index = 0; index < selected; ++index)
        {
            byte before = territory[index * 6 + 1];
            if ((before & 1) != 0 && (before & 2) == 0)
            {
                placed++;
            }
        }

        island = placed;
        return true;
    }

    /// <summary>전투 소유자 기록에 공유 버퍼의 현재 이름을 덮어쓴다. 원본 월드 적재의 이름 확정 쓰기와 같다.</summary>
    /// <remarks>2026-11-12 0.428에서 미션 값 채우기로 추가했고 0.430에서 공유 버퍼 읽기로 바꿨다. NUL 뒤 stale 꼬리는 보존하므로 영어 "You"는 "You\0pson"이 된다.</remarks>
    public void WriteBattleName()
    {
        int nul = Array.IndexOf(HandleBuffer, (byte)0);
        if (nul < 0)
            throw new ArgumentOutOfRangeException(nameof(HandleBuffer), "전투 이름의 NUL 종료 바이트 누락");
        uint owner = MissionPlayer.BattleOwner;
        if (owner > (uint)MissionPlayers.PlayerCount)
            throw new ArgumentOutOfRangeException(nameof(HandleBuffer), "전투 이름의 실제 Player 번호 범위 오류");
        int record = (int)owner * PlayerRecordStride;
        if (nul + 1 > PlayerNameWidth)
            throw new ArgumentOutOfRangeException(nameof(HandleBuffer), "전투 이름이 기록 폭을 넘는다");
        // 첫 NUL까지만 복사하고 stale 꼬리는 둔다.
        Array.Copy(HandleBuffer, 0, PlayerRecords, record + PlayerNameOffset, nul + 1);
    }

    /// <summary>세션이 센 로그인 요새 순서 수다. 원본 로그인의 fortCount와 같으며 첫 로그인에 0을 둔다.</summary>
    public uint FortCount { get; private set; }

    /// <summary>세션이 마지막에 새 요새를 받은 Player 번호다. 원본 로그인의 lastLoginPlayer와 같다.</summary>
    public uint LastLoginPlayer { get; private set; }

    /// <summary>세션이 공유하는 현재 Player 상태다. Zone 설치가 소유자를 정하고 이름 확정이 전투 소유자를 읽는다.</summary>
    public PlayerWorldState MissionPlayer { get; } = new();

    /// <summary>세션이 공유하는 현재 mySubHandle NUL 저장소다. 이름 채우기가 쓰고 이름 확정이 읽는다.</summary>
    public byte[] HandleBuffer { get; } = new byte[64];

    /// <summary>월드 적재가 읽는 같은 TYPE 프레임 표 개수다. PATCH 188이며 CD 171은 후속 단위에서 잇는다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 실제 타입 적재 단위에서 표를 채운다.</remarks>
    public const int WorldFrameCount = 188;

    /// <summary>월드 적재가 직접 쓰는 현재 Template header 바이트 수다(9×20, RawCampaignWorldLoad와 같은 값).</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다.</remarks>
    public const int WorldTemplateBytes = 180;

    /// <summary>세션이 소유한 빌더 attach용 원본 ChunkMap이다. 월드 적재의 섬 배치·지형 생성이 공유한다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 부모·갈고리 연결은 다음 단위다.</remarks>
    public ChunkMap World { get; } = new();

    /// <summary>세션이 소유한 섬 런타임 3204바이트 저장소다. 섬 목록의 count DWORD·100개 기록을 둔다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 런타임 부모 연결은 다음 단위다.</remarks>
    public byte[] IslandRecords { get; } = new byte[RawIslandStorage.Bytes];

    /// <summary>세션이 소유한 섬 런타임 모드 상태다. FortFinishOwner의 첫 지역·거주 구역 타입을 공유한다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 런타임 부모 연결은 다음 단위다.</remarks>
    public FortFinishOwnerState IslandModes { get; } = new();

    /// <summary>세션이 소유한 섬 런타임 거주 목록이다. AssetPostPop의 실제 등록 목록을 둔다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 런타임 부모 연결은 다음 단위다.</remarks>
    public RawPostPopList IslandResidences { get; } = new();

    /// <summary>세션이 소유한 표면 조회용 현재 hash0 단어 지도다. 섬 런타임·빌더가 함께 읽는다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 런타임 부모 연결은 다음 단위다.</remarks>
    public RawSquidHash IslandHash { get; } = new();

    /// <summary>세션이 소유한 지도 다시 그리기 공유 값이다. 섬 owner 변경이 증가시킨다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 런타임 갈고리 읽기로 쓴다.</remarks>
    public uint IslandChanged { get; set; }

    /// <summary>세션이 소유한 지도 리비전 공유 값이다. 패치판 owner 변경이 1로 둔다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 런타임 갈고리 읽기로 쓴다.</remarks>
    public uint IslandRevision { get; set; }

    /// <summary>세션이 소유한 같은 TYPE 프레임 표다. 실제 타입 적재 단위에서 채우기 전까지 빈 표다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 적재 본체 연결은 다음 단위다.</remarks>
    public RiftTypeFrames[] WorldFrames { get; } = EmptyWorldFrames();

    /// <summary>세션이 소유한 현재 Template header 180바이트다. 월드 적재가 +8 DWORD를 직접 쓴다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 적재 본체 연결은 다음 단위다.</remarks>
    public byte[] WorldTemplates { get; } = new byte[WorldTemplateBytes];

    /// <summary>세션이 소유한 월드 적재 전역이다. 구독자·편집·임무후 상태를 둔다.</summary>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다. 적재 본체 연결은 다음 단위다.</remarks>
    public CampaignWorldGlobals WorldGlobals { get; } = new();

    /// <summary>세션이 소유한 지형 문자 mapping 전역이다. 초기 지면·경계 타입과 지역 없음 값을 둔다.</summary>
    /// <remarks>2026-11-12 0.444에서 지형 상태 저장소로 추가했다. 바인딩·생성 연결은 다음 단위다.</remarks>
    public TerrainContextGlobals TerrainGlobals { get; } = new();

    /// <summary>세션이 소유한 임의 배치 타입 전역이다. 섬·농지 타입과 지역 owner 규칙을 둔다.</summary>
    /// <remarks>2026-11-12 0.444에서 지형 상태 저장소로 추가했다. 바인딩·생성 연결은 다음 단위다.</remarks>
    public FortRandomPlacementState PlacementState { get; } = new();

    /// <summary>세션이 소유한 수명 긴 초기 캐시 상태다. 기본 섬·경계·장식 타입과 지역 없음 값을 둔다.</summary>
    /// <remarks>2026-11-12 0.444에서 지형 상태 저장소로 추가했다. 바인딩·생성 연결은 다음 단위다.</remarks>
    public TerrainStampState StampState { get; } = new();

    /// <summary>세션이 소유한 지형 진단 묶음이다. 연결 전까지 비어 있어 생성 진입을 거부한다.</summary>
    /// <remarks>2026-11-12 0.444에서 지형 상태 저장소로 추가했다. 진단 연결은 생성 단위에서 잇는다.</remarks>
    public NormalTerrainDiagnostics TerrainDiagnostics { get; } = new();

    /// <summary>세션이 소유한 현재 조각 타입 공유 상자다. 바인딩·생성이 매 요청 읽는다.</summary>
    /// <remarks>2026-11-12 0.444에서 지형 상태 저장소로 추가했다. 실제 값 경로는 생성 단위에서 잇는다.</remarks>
    public UIntBox PuzzlePieceBox { get; } = new();

    /// <summary>세션이 소유한 지형 생성 입력 묶음이다. 바인딩 때 군집과 상자를 공유한다.</summary>
    /// <remarks>2026-11-12 0.444에서 지형 상태 저장소로 추가했다. 입력 값 연결과 바인딩은 생성 단위에서 잇는다.</remarks>
    public TerrainGenerationInputs GenerationInputs { get; } = new();

    /// <summary>세션이 소유한 지형 생성 전용 CRT 상태 상자다. 시드 사슬과 지형 소비자가 공유한다.</summary>
    /// <remarks>2026-11-12 0.447에서 난수 상태 저장소로 추가했다. StartMission의 지역 상자를 여기로 승격한다.</remarks>
    /// <summary>세션이 공유하는 CRT 상태 상자다. 지형 소비자와 덤프가 같은 상자를 읽고 쓴다.</summary>
    /// <remarks>2026-11-12 0.464에서 호스트 공유 상자를 잇려고 생성자에서 정하도록 바꿨다. 미션별 상자 교체는 원본에 없다.</remarks>
    public UIntBox TerrainCrt { get; }

    /// <summary>세션이 소유한 게임 보조 장기 전역이다. 생성·밀도·간헐천 여백의 현재 DWORD를 둔다.</summary>
    /// <remarks>2026-11-12 0.453에서 지형 바인딩 입력 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1830행 terrainAux.</remarks>
    public RawGameAuxState GameAux => Gameplay.GameAux;

    /// <summary>호스트보다 짧은 세션이 빌리는 같은 장기 게임 전역이다.</summary>
    public ClientGameplayGlobals Gameplay { get; }

    /// <summary>사제 조회·후처리·회복이 공유하는 같은 미션 상태다.</summary>
    public Netstorm.Client.Mission.RawPriestWorldState PriestState { get; }

    /// <summary>호스트가 한 번 만든 정적 SID 목록을 미션이 직접 빌린다.</summary>
    public RawStateSidLists SidLists { get; }

    /// <summary>세션이 소유한 소유자 모드다. 미션 전투이므로 battle을 켠다.</summary>
    /// <remarks>2026-11-12 0.453에서 지형 바인딩 입력 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 336행 CampaignLinks(battle=true).</remarks>
    public SquidOwnerMode OwnerMode { get; } = new() { Battle = true };

    /// <summary>세션이 소유한 다리·섬 연결 전역이다. 미션이므로 mission을 켜고 캠페인 연결 타입을 둔다.</summary>
    /// <remarks>2026-11-12 0.453에서 지형 바인딩 입력 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 336행 CampaignLinks.</remarks>
    public BridgeConnectState BridgeState { get; } = new()
    {
        BridgeType = 82,
        IslandType = 94,
        StalagType = 95,
        ConnectorType = 155,
        NoIslandType = 157,
        Battle = true,
        Mission = true,
    };

    /// <summary>세션이 소유한 87바이트 전투 옵션이다. 미션 기본값으로 초기화한다.</summary>
    /// <remarks>2026-11-12 0.453에서 지형 바인딩 입력 결선용으로 추가했다. 근거: cpppj RawMissionBattleOptions(ResetMissionBattleOptions).</remarks>
    public byte[] BattleOptions { get; } = CreateBattleOptions();

    /// <summary>세션이 소유한 AI 진단 checking DWORD 뷰의 장기 값이다. 생성 Growth가 같은 값을 읽는다.</summary>
    /// <remarks>2026-11-12 0.453에서 지형 바인딩 입력 결선용으로 추가했다. 근거: cpppj ClientGameplayGlobals AiDebug().Checking().</remarks>
    public RawCheckingFlag AiChecking { get; } = new();

    /// <summary>미션 기본 87바이트 전투 옵션을 만든다.</summary>
    /// <returns>초기화된 87바이트 배열이다.</returns>
    /// <remarks>2026-11-12 0.453에서 지형 바인딩 입력 결선용으로 추가했다.</remarks>
    private static byte[] CreateBattleOptions()
    {
        var options = new byte[87];
        RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, options);
        return options;
    }

    /// <summary>세션 장기 전역에서 지형 생성 입력을 묶는다. 원문 terrainGeneration 15종의 세션 소유 결선이다.</summary>
    /// <returns>세션 상자·기록을 공유하는 입력 묶음이다. 같은 인스턴스(GenerationInputs)를 돌려준다.</returns>
    /// <remarks>2026-11-12 0.453에서 지형 바인딩 입력 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1831~1853행.
    /// 건설 PlayerBounds(R6)·세션 로그 0x1d(R6)·성장 assertion은 미연결(null)로 두어 GenerateOriginal이 명시 거부한다.</remarks>
    public TerrainGenerationInputs BuildTerrainGenerationInputs()
    {
        TerrainGenerationInputs inputs = GenerationInputs;
        inputs.Enabled = new RawCheckingFlag(() => GameAux.TerrainEnabled);
        inputs.Battle = new RawCheckingFlag(() => OwnerMode.Battle);
        inputs.Mission = new RawCheckingFlag(() => BridgeState.Mission);
        inputs.Layout = () => BattleOptions[5];
        inputs.Spacing = () => RawWorldGeyserSpacing.Spacing(BattleOptions[6]);
        inputs.Span.Value = GameAux.WorldGeyserMargin;
        inputs.Minimum.Value = GameAux.ArchipelagoMinimum;
        inputs.Maximum.Value = GameAux.ArchipelagoMaximum;
        inputs.Percent.Value = GameAux.ArchipelagoPercent;
        inputs.Archipelago.Value = GameAux.Archipelago;
        inputs.Subscriber = ReadPlayerSubscriber;
        inputs.Growth.Checking.Bind(() => AiChecking.Read());
        return inputs;
    }

    /// <summary>현재 Player 기록에서 해당 소유자의 구독자 DWORD를 읽는다. 원문처럼 매 반복 다시 선택한다.</summary>
    /// <param name="player">조회할 Player 번호다. 1~8이다.</param>
    /// <returns>해당 기록의 +0x2c 구독자 DWORD다.</returns>
    /// <remarks>2026-11-12 0.453에서 지형 바인딩 입력 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1836~1840행.</remarks>
    private uint ReadPlayerSubscriber(int player)
    {
        if (player < 1 || player > 8 || PlayerRecords.Length < 9 * PlayerRecordStride)
        {
            throw new ArgumentOutOfRangeException(nameof(player), "지형 root의 현재 Player 9 물리 범위 오류");
        }

        int offset = player * PlayerRecordStride + PlayerSubscriberOffset;
        return (uint)(PlayerRecords[offset]
            | ((uint)PlayerRecords[offset + 1] << 8)
            | ((uint)PlayerRecords[offset + 2] << 16)
            | ((uint)PlayerRecords[offset + 3] << 24));
    }

    /// <summary>세션이 소유한 군집 상태다. 입력 바인딩이 같은 군도·확장 상자를 공유한다.</summary>
    /// <remarks>2026-11-12 0.454에서 지형 바인딩 드라이런 조립용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1829행 normal 바인딩의 farm.</remarks>
    public FarmClusterState TerrainFarm { get; } = new();

    /// <summary>세션이 소유한 공통 장부다. SetOwner·Player PostPop·덤프가 같은 통계표를 공유한다.</summary>
    /// <remarks>2026-11-12 0.455에서 Generate 라이브 연결용으로 추가했다. Pop 장부·그래프·집계(totals·totalmade) 연결은 다음 단위다.</remarks>
    public RawPostPopState TerrainBookkeeping { get; }

    /// <summary>지형 Pop 공간 등록의 세션 소유 공장이다. BuildTerrainPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.466에서 지형 Pop 공간 등록 라이브 연결용으로 추가했다. 근거: cpppj MakeTerrainStampWorldHooks의 core.factory와 같은 공장 생성(vtable 포함)이다.</remarks>
    private RawSquidFactory? _terrainFactory;

    /// <summary>지형 Pop 공간 등록에 쓰는 세션 소유 공간 해시다. BuildTerrainPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.466에서 추가했다. 섬 런타임의 IslandHash(경로 SurfaceAt 전용)와는 다른 오브젝트 체인 해시이며 cpppj core.hash와 같은 구실을 한다.</remarks>
    private RawSquidHash? _terrainHash;

    /// <summary>지형 Pop 공간 등록에 쓰는 세션 소유 칸별 점유 바이트 65,536개다. BuildTerrainPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.466에서 추가했다. cpppj core.spots와 같은 배열이며 Unpop 쪽과 공유한다.</remarks>
    private byte[]? _terrainSpots;

    /// <summary>지형 Pop의 SHP 크기 조회용 세션 소유 표시다. BuildTerrainPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.466에서 추가했다. CellWidth/CellHeight를 해시 단계 계산에만 쓰며 실제 출력은 하지 않는다(R5 표시 연결과 별개).</remarks>
    private SquidDisplay? _terrainDisplay;

    /// <summary>지형 Pop 공간 등록의 세션 소유 몸체다. BuildTerrainPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.466에서 추가했다. 표시·파생 효과는 연결하지 않으며 좌표·화면 좌표·spot·체인·레벨·void 해제를 수행한다.</remarks>
    private RawPop? _terrainPop;

    /// <summary>지형 Pop이 읽는 세션 TYPE 표다. BuildTerrainPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.466에서 추가했다. BuildTerrainStampHooks에 준 표와 같은 인스턴스를 둔다.</remarks>
    private IReadOnlyList<OriginalTypeRecord>? _terrainPopTypes;

    /// <summary>지형 Pop이 읽는 세션 표시 셰이프 목록이다. 농지 파이프라인 자료가 같은 인스턴스를 빌린다.</summary>
    /// <remarks>2026-11-18 0.614에서 R4-2 농지 실결선용으로 추가했다. BuildTerrainPopDevices가 보관하며 파이프라인 수명보다 길게 둔다.</remarks>
    private IReadOnlyList<SquidDisplayShape>? _terrainPopShapes;

    /// <summary>세션 Pop 표시의 빈 싱크다. SHP 크기 조회 경로에서는 호출되지 않는다.</summary>
    /// <remarks>2026-11-12 0.466에서 추가했다. 실제 표시는 R5에서 잇는다.</remarks>
    private sealed class NullTerrainDisplaySink : SquidDisplaySink
    {
        /// <summary>억제 여부를 묻는다. 항상 거짓이다.</summary>
        /// <returns>거짓이다.</returns>
        public override bool Suppressed() => false;

        /// <summary>표시 갱신 요청을 버린다. SHP 크기 조회 경로에서는 호출되지 않는다.</summary>
        /// <param name="rect">변경 영역이다.</param>
        /// <param name="flags">전달 플래그다.</param>
        public override void Invalidate(DisplayRect rect, uint flags)
        {
        }
    }

    /// <summary>지형 도장 Pop의 공장·공간·표시 장치를 세션에 연결한다. 연결 전에는 도장 효과가 중립(기록만)으로 동작한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 패치판 188개·CD판 171개여야 한다.</param>
    /// <param name="shapes">같은 순서의 표시 셰이프 목록이다. 타입 개수와 같아야 한다.</param>
    /// <param name="factory">빌릴 외부 공장. null이면 세션 전용으로 만든다. 넘기면 같은 풀 소속이어야 하며 소유자의 바인딩 그대로 쓴다.</param>
    /// <param name="hash">빌릴 외부 공간 해시. null이면 세션 전용으로 만든다. 넘기면 요새 장치와 같은 인스턴스를 공유한다.</param>
    /// <param name="spots">빌릴 외부 점유 지도. null이면 세션 전용으로 만든다. 해시와 함께 넘겨야 하며 WorldCells² 크기여야 한다.</param>
    /// <remarks>2026-11-12 0.466에서 지형 Pop 공간 등록 라이브 연결용으로 추가했다. 근거: cpppj MakeTerrainStampWorldHooks(core.factory·core.hash·core.shapes→FrameSize)의 세션 결선이다.
    /// 0.597에서 저장소 공장 전환용으로 외부 공장 빌림을 추가했다.
    /// 0.598에서 저장소 공간 공유용으로 외부 해시·점유지 빌림을 추가했다.</remarks>
    public void BuildTerrainPopDevices(IReadOnlyList<OriginalTypeRecord> types, List<SquidDisplayShape> shapes, RawSquidFactory? factory = null, RawSquidHash? hash = null, byte[]? spots = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(shapes);
        if (types.Count != OriginalConstructors.TypeCount || shapes.Count != types.Count)
        {
            throw new ArgumentException("지형 Pop 타입·셰이프 목록 크기 오류", nameof(types));
        }
        if (factory is not null && !ReferenceEquals(factory.Pool, Pool))
        {
            throw new ArgumentException("지형 Pop 외부 공장의 풀 불일치", nameof(factory));
        }
        if ((hash is null) != (spots is null))
        {
            throw new ArgumentException("지형 Pop 외부 해시·점유지는 함께 빌린다", nameof(hash));
        }
        if (spots is not null && spots.Length != RawSquidHash.WorldCells * RawSquidHash.WorldCells)
        {
            throw new ArgumentException("지형 Pop 외부 점유지 크기 오류", nameof(spots));
        }
        _terrainFactory = factory ?? new RawSquidFactory(Pool, types);
        if (factory is null)
        {
            BindFactoryHitPoints(_terrainFactory, types);
        }
        _terrainHash = hash ?? new RawSquidHash();
        _terrainSpots = spots ?? new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        _terrainDisplay = new SquidDisplay(
            Pool.Edition,
            types,
            shapes,
            new NullTerrainDisplaySink(),
            new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 640, 480) });
        _terrainDisplay.ValidateFrameBinding(Pool.Edition, types);
        _terrainPop = new RawPop(Pool, _terrainHash, _terrainSpots);
        _terrainPopTypes = types;
        _terrainPopShapes = shapes;
    }

    /// <summary>지형 Pop 공간 해시를 검사·대조용으로 빌려준다. 장치를 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.466에서 HostTerrainPopTests의 체인·단계 대조용으로 추가했다. 실제 게임 흐름에서는 Pop 내부에서만 쓴다.</remarks>
    public RawSquidHash? TerrainPopHash => _terrainHash;

    /// <summary>지형 Pop 점유 지도를 검사·대조용으로 빌려준다. 장치를 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.466에서 HostTerrainPopTests의 spot 대조용으로 추가했다. Unpop 쪽과 같은 배열을 공유한다.</remarks>
    public byte[]? TerrainPopSpots => _terrainSpots;

    /// <summary>요새 내용물 Form 부착 진단을 검사·대조용으로 빌려준다. 부착 경로 실행 여부를 확인한다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. 원문 진단 복귀 뒤에도 계속 진행하므로 기록만 남긴다.</remarks>
    public IReadOnlyList<string> FortAttachDiagnostics => _fortAttachDiagnostics;

    /// <summary>세션 저장소를 묶은 지형 바인딩이다. 만들기 전까지 null이다.</summary>
    /// <remarks>2026-11-12 0.454에서 지형 바인딩 드라이런 조립용으로 추가했다. Generate 호출 연결은 다음 단위다.</remarks>
    public RawNormalTerrainWorldBinding? TerrainBinding { get; private set; }

    /// <summary>세션 저장소로 지형 바인딩을 조립하고 생성 입력을 묶는다. 원문 normal 바인딩 조립의 세션 소유 결선이다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 패치판 188개·CD판 171개여야 한다.</param>
    /// <param name="diagnostics">원본 진단 묶음이다. 복사해서 가진다.</param>
    /// <param name="islandHooks">섬 런타임의 거주 구역·진단 효과다.</param>
    /// <returns>군집·입력을 묶은 세션 바인딩이다. 같은 인스턴스를 TerrainBinding에 둔다.</returns>
    /// <remarks>2026-11-12 0.454에서 지형 바인딩 드라이런 조립용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1829~1853행.
    /// 바인딩 드라이런까지만 하며 Load.Generate 갈고리·도장 실제 효과 연결은 다음 단위다.</remarks>
    public RawNormalTerrainWorldBinding BuildTerrainBinding(
        IReadOnlyList<OriginalTypeRecord> types,
        NormalTerrainDiagnostics diagnostics,
        IslandRuntimeHooks islandHooks)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(diagnostics);
        ArgumentNullException.ThrowIfNull(islandHooks);
        var binding = new RawNormalTerrainWorldBinding(
            World, BuildIslands(islandHooks), TerrainGlobals, PlacementState, StampState,
            types, Pool, PuzzlePieceBox, WorldFrames, diagnostics);
        PuzzlePieceBox.Value = GameAux.PuzzlePieceType;
        binding.BindGenerationInputs(TerrainFarm, BuildTerrainGenerationInputs());
        TerrainBinding = binding;
        return binding;
    }

    /// <summary>세션 풀·소유자·지역으로 도장 실제 효과를 묶는다. Pop 공간 등록 장치를 연결했으면 실제 Pop을 수행하고, 아니면 기록만 한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. SetOwner·SetRegion이 읽는다.</param>
    /// <returns>세션 저장소를 공유하는 도장 효과다.</returns>
    /// <remarks>2026-11-12 0.455에서 Generate 라이브 연결용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1829행 normal 바인딩의 world 효과.
    /// 0.466에서 BuildTerrainPopDevices 연결 때 실제 Pop(factory 생성·공간 등록)을 수행하도록 바꿨다. 미연결 때는 neutral(기록만)이라 기존 드라이런 검사가 그대로 통과한다.
    /// 군집 배치(place) 효과는 Load 갈고리에서 잇는다.</remarks>
    public TerrainStampHooks BuildTerrainStampHooks(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        var owner = new SquidOwner(Pool, types, TerrainBookkeeping, OwnerMode);
        // 도장도 같은 현재 TYPE를 읽으며 진단이 선택되면 실제 호스트 오류로 전달한다.
        owner.BindDiagnostics(new SquidOwnerDiagnostics
        {
            Types = types,
            Assertion = (expression, file, line) => throw new InvalidOperationException($"도장 Owner 원본 진단: {expression} {file}:{line}"),
        });
        return new TerrainStampHooks
        {
            Create = (type, predictable) =>
            {
                // Pop 장치가 연결됐으면 원문 공장처럼 생성자·가상 표까지 갖춘 슬롯을 만든다. 지형은 예측 영역(1) 할당이다.
                if (_terrainFactory is not null)
                {
                    return _terrainFactory.Create(type, predictable != 0 ? SidAllocation.Predictable : SidAllocation.Client);
                }
                // 원문 공장처럼 두 번째 인자를 풀 할당 flags로 쓴다. 지형은 1(예측 영역 23002~)이다.
                int sid = Pool.Allocate(predictable != 0 ? SidAllocation.Predictable : SidAllocation.Client);
                Span<byte> raw = Pool.AllocatedBytes(sid);
                raw.Clear();
                raw[RawSidPool.StateOffset] = RawSidPool.VoidBit;
                raw[RawSidPool.TypeOffset] = unchecked((byte)type);
                return sid;
            },
            SetOwner = owner.Set,
            Pop = (sid, x, y, flags) => TerrainPopObject(sid, x, y, flags),
            SetRegion = (sid, region) => RawSquidRegion.SetSquidRegion(Pool, types, sid, unchecked((uint)region), unchecked((uint)TerrainGlobals.NoRegion), TerrainDiagnostics.Region),
            ChunkRegion = (x, y) => unchecked((sbyte)World.At(x, y).Island),
            Assertion = TerrainDiagnostics.Stamp,
        };
    }

    /// <summary>지형 Pop 공간 등록의 실제 효과를 수행한다. 도장 후처리와 섬 후처리가 같은 몸체를 공유한다.</summary>
    /// <param name="sid">등록할 오브젝트 번호다.</param>
    /// <param name="x">등록할 월드 x 좌표.</param>
    /// <param name="y">등록할 월드 y 좌표.</param>
    /// <param name="flags">Pop 호출 플래그.</param>
    /// <remarks>2026-11-12 0.466의 도장 Pop 몸체를 0.601에서 섬 후처리와 공유하도록 꺼냈다. 동작은 같다.
    /// 0.602에서 호출 기록을 남긴다. 기록은 풀 바이트·덤프에 영향을 주지 않으며 R4-2 스탬프 실측 발화의 측정 바탕이다.
    /// 0.603에서 분배 Pop으로 교체해 공간 등록 뒤 섬·표면 접두와 장부 집계를 같은 호출에서 실행한다.</remarks>
    private void TerrainPopObject(int sid, float x, float y, uint flags)
    {
        // Pop 장치가 없으면 기록만 한다(드라이런 중립).
        if (_terrainPop is null || _terrainDisplay is null || _terrainPopTypes is null)
        {
            // 호출 자체를 기록하는 반복문이다. 중립 호출도 순서대로 남긴다.
            _terrainPopCalls.Add(FormatTerrainPopCall(sid, x, y, flags, -1, "neutral"));
            return;
        }
        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        byte number = raw[RawSidPool.TypeOffset];
        if (number >= _terrainPopTypes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "Pop 타입 번호 범위 오류");
        }
        // 원본 SHP 크기로 해시 단계를 정한다. 패치판은 +36 DWORD, CD판은 +34 BYTE 프레임을 읽는다.
        int frame = Pool.Edition == OriginalEdition.Patch1078
            ? unchecked((int)(raw[36] | ((uint)raw[37] << 8) | ((uint)raw[38] << 16) | ((uint)raw[39] << 24)))
            : raw[34];
        float[] size = _terrainDisplay.FrameSize(Pool.Edition, raw, frame);
        RawPopResult result = _terrainPop.Pop(sid, _terrainPopTypes[number], size[0], size[1], x, y, flags);
        // 실제 등록 결과를 기록하는 반복문이다. 풀 바이트에는 영향을 주지 않는다.
        _terrainPopCalls.Add(FormatTerrainPopCall(sid, x, y, flags, number, result.ToString()));
    }

    /// <summary>지형 Pop 호출 기록이다. 순서대로 호출 원문을 둔다. 0.602에서 추가됐다.</summary>
    private readonly List<string> _terrainPopCalls = [];
    /// <summary>단독 임무에서 네트워크 또래 없이 송신한 지형 Pop 기록이다. 덤프에는 영향을 주지 않는다. 0.607에서 추가됐다.</summary>
    private readonly List<(int Sid, uint Flags)> _terrainPopTransmissions = [];

    /// <summary>지형 Pop 호출 기록이다. R4-2 스탬프 실측 발화의 측정 바탕이며 풀 바이트·덤프에 영향을 주지 않는다. 0.602에서 추가됐다.</summary>
    public IReadOnlyList<string> TerrainPopCalls => _terrainPopCalls;

    /// <summary>단독 임무에서 네트워크 또래 없이 송신한 지형 Pop 기록이다. 덤프에는 영향을 주지 않는다. 0.607에서 추가됐다.</summary>
    public IReadOnlyList<(int Sid, uint Flags)> TerrainPopTransmissions => _terrainPopTransmissions;

    /// <summary>지형 Pop 호출 한 줄을 같은 형식으로 만든다. 비트 단위 좌표와 섬 후처리 연결 여부를 둔다.</summary>
    /// <param name="sid">등록한 오브젝트 번호다.</param>
    /// <param name="x">등록한 월드 가로 좌표다.</param>
    /// <param name="y">등록한 월드 세로 좌표다.</param>
    /// <param name="flags">Pop 호출 플래그다.</param>
    /// <param name="type">Pop 타입 번호다. 중립 호출은 -1이다.</param>
    /// <param name="outcome">호출 결과다. 중립은 neutral, 실제 등록은 RawPopResult 이름이다.</param>
    /// <returns>기록 한 줄이다.</returns>
    private string FormatTerrainPopCall(int sid, float x, float y, uint flags, int type, string outcome)
    {
        uint xbits = BitConverter.SingleToUInt32Bits(x);
        uint ybits = BitConverter.SingleToUInt32Bits(y);
        string island = _islandPostPop is null ? "island:missing" : "island:ready";
        return $"pop sid={sid} x={xbits:X8} y={ybits:X8} flags={flags:X8} type={type} {outcome} {island}";
    }

    /// <summary>세션 공간 해시의 0단계 머리를 섬 후처리 지도에 빌리는 보기다. 월드·저장소보다 오래 살 수 없다.</summary>
    /// <remarks>0.601에서 추가했다. `RawSurfaceWorld`의 같은 보기와 같은 읽기 계약이다.</remarks>
    private sealed class SessionHashLevelView(RawSquidHash hash) : IReadOnlyList<ushort>
    {
        /// <summary>지도 한 변의 칸 수(256×256)다.</summary>
        public int Count => hash.Entries(0).Length;

        /// <summary>해당 칸의 머리 번호를 읽는다.</summary>
        /// <param name="index">읽을 칸 번호다.</param>
        public ushort this[int index] => hash.Entries(0)[index];

        /// <summary>머리를 순서대로 열거한다.</summary>
        public IEnumerator<ushort> GetEnumerator()
        {
            for (int index = 0; index < Count; index++)
            {
                yield return this[index];
            }
        }

        /// <summary>머리를 순서대로 열거한다.</summary>
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => GetEnumerator();
    }

    /// <summary>세션 소유의 섬 후처리 몸체다. BuildIslandPostPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>0.601에서 R4-2 적재 중 postPop용으로 추가했다. 지면·받침 소유자 스탬프의 실제 몸체다.</remarks>
    private RawIslandPostPop? _islandPostPop;

    /// <summary>세션 섬 후처리 상태다. BuildIslandPostPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>0.604에서 R4-2 적재 깊이 흐름용으로 추가했다. 적재 깊이에 따라 Prefix 프레임 선택이 발화한다.</remarks>
    private IslandPostPopState? _islandPostPopState;
    /// <summary>세션 적재 깊이를 섬 후처리와 공유하는 상자다. SetWorldLoadingDepth가 값을 둔다.</summary>
    /// <remarks>0.604에서 R4-2 적재 깊이 흐름용으로 추가했다. 원본 Storage/Template 깊이 DWORD와 같은 구실이다.</remarks>
    private readonly UIntBox _islandLoadingDepth = new();

    /// <summary>세션 섬 후처리와 연결 순회가 공유하는 소유자 지정 몸체다. BuildIslandPostPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>0.603에서 R4-2 Prefix 실측 발화용으로 추가했다. 섬 후처리와 다리 연결이 같은 Owner를 공유한다.</remarks>
    private SquidOwner? _islandOwner;

    /// <summary>세션 섬 연결 순회가 읽는 이웃 탐색기다. BuildIslandPostPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>0.603에서 R4-2 Prefix 실측 발화용으로 추가했다. 같은 풀·해시·점유지·타입·프레임을 공유한다.</remarks>
    private RawSquidNeighbors? _islandNeighbors;

    /// <summary>세션 섬 후처리 Prefix와 Connect가 공유하는 다리 연결 몸체다. BuildIslandPostPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>0.603에서 R4-2 Prefix 실측 발화용으로 추가했다. `RawSurfaceStorage.ConnectionsExecutor`와 같은 구실이다.</remarks>
    private RawBridgeConnect? _islandConnections;

    /// <summary>세션 지형 Pop의 공통 후처리 분배기다. BuildIslandPostPopDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>0.603에서 R4-2 Prefix 실측 발화용으로 추가했다. 공간 등록 뒤 섬·표면 접두와 장부 집계를 같은 호출에서 실행한다.</remarks>
    private RawPostPop? _terrainPostPop;

    /// <summary>세션 지형의 공간 해제 몸체다. BuildIslandPostPopDevices로 연결하기 전에는 null이다.</summary>
    private RawUnpop? _terrainUnpop;

    /// <summary>세션 지형의 프레임 지정 몸체다. BuildIslandPostPopDevices로 연결하기 전에는 null이다.</summary>
    private SquidFrame? _terrainFrames;

    /// <summary>세션 섬 후처리 진단의 기록이다. 미연결·누락 효과를 검사에서 확인한다.</summary>
    /// <remarks>0.601에서 추가했다. 원본 진단 복귀 뒤에도 계속 진행하므로 기록만 남긴다.</remarks>
    private readonly List<string> _islandPostPopDiagnostics = [];

    /// <summary>세션 섬 후처리의 진단 기록이다. 0.601에서 추가됐다.</summary>
    public IReadOnlyList<string> IslandPostPopDiagnostics => _islandPostPopDiagnostics;

    /// <summary>세션 소유의 섬 후처리 몸체다. 0.601에서 추가됐다.</summary>
    public RawIslandPostPop IslandPostPop => _islandPostPop ?? throw new InvalidOperationException("세션 섬 후처리 미연결");

    /// <summary>세션 섬 연결 순회 몸체다. 0.603에서 추가됐다.</summary>
    public RawBridgeConnect IslandConnections => _islandConnections ?? throw new InvalidOperationException("세션 섬 연결 미연결");

    /// <summary>세션 지형 Pop의 공통 후처리 분배기다. 0.603에서 추가됐다.</summary>
    public RawPostPop TerrainPostPop => _terrainPostPop ?? throw new InvalidOperationException("세션 지형 공통 후처리 미연결");

    /// <summary>세션 섬 후처리 장치를 연결한다. 지형 Pop 장치 뒤에 연결해야 한다(같은 풀·해시·표시를 공유한다).</summary>
    /// <param name="types">같은 실제 TYPE 표다. 패치판 188개여야 한다.</param>
    /// <param name="frames">같은 실제 프레임 표다. 타입 개수와 같아야 한다.</param>
    /// <remarks>0.601에서 R4-2 적재 중 postPop용으로 추가했다. 섬 연결 순회는 R6에서 잇는다.
    /// 0.603에서 연결 순회·지형 공통 후처리 분배를 함께 연결했다. 지형 Pop 호출은 같은 호출에서 접두·장부까지 실행한다.
    /// 0.604에서 적재 깊이 흐름을 함께 연결했다. SetWorldLoadingDepth가 바뀌면 섬 후처리의 프레임 선택 관문이 같은 값을 읽는다.</remarks>
    public void BuildIslandPostPopDevices(IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        if (types.Count != OriginalConstructors.TypeCount || frames.Count != types.Count)
        {
            throw new ArgumentException("섬 후처리 타입·프레임 목록 크기 오류", nameof(types));
        }
        if (_terrainFactory is null || _terrainHash is null || _terrainSpots is null || _terrainDisplay is null || _terrainPop is null)
        {
            throw new InvalidOperationException("섬 후처리 장치는 지형 Pop 장치 뒤에 연결해야 한다");
        }
        if (_islandPostPop is not null)
        {
            throw new InvalidOperationException("세션 섬 후처리 중복 연결");
        }
        var owner = new SquidOwner(Pool, types, TerrainBookkeeping, OwnerMode);
        owner.BindDiagnostics(new SquidOwnerDiagnostics
        {
            Types = types,
            Assertion = (expression, file, line) => throw new InvalidOperationException($"섬 후처리 Owner 원본 진단: {expression} {file}:{line}"),
        });
        _islandOwner = owner;
        // 세션 소유의 이웃 탐색기와 다리 연결 스택을 같은 풀·해시·표로 묶는다. 섬 후처리 Prefix와 Connect가 같은 저장소를 공유한다.
        _islandNeighbors = new RawSquidNeighbors(Pool, _terrainHash, _terrainSpots, types, new RiftFrameCodeTables(frames));
        if (Pool.Edition == OriginalEdition.Patch1078)
        {
            _islandNeighbors.BindCurrentTypes(types);
        }
        _islandConnections = new RawBridgeConnect(Pool, _islandNeighbors, BridgeState, new BridgeConnectHooks
        {
            Create = (type, flags) => _terrainFactory!.Create(type, (SidAllocation)flags),
            SetOwner = _islandOwner.Set,
            Pop = (sid, x, y, flags) => TerrainPopObject(sid, x, y, flags),
            SetFrame = (sid, frame, flags) => (_terrainFrames ?? throw new InvalidOperationException("세션 연결 프레임 미연결")).Set(sid, frame, flags),
            NotifySurface = sid => (_fortBuilding ?? throw new InvalidOperationException("세션 연결 표면 통지 미연결(R6)")).NotifySurface(sid),
        });
        // 세션 지형 Pop의 공통 후처리 분배기다. 섬·표면 접두를 같은 세션 장치에 잇고 그 밖의 파생은 R6에서 잇는다.
        _terrainPostPop = new RawPostPop(Pool, types, TerrainBookkeeping, null, sid => _islandConnections.Connect(sid));
        if (Pool.Edition == OriginalEdition.Patch1078)
        {
            _terrainPostPop.BindCurrentTypes(types);
        }
        _terrainPostPop.SetIslandPrefix((sid, flags) => _islandConnections.IslandPostPopPrefix(sid, flags));
        _terrainPostPop.SetSurfacePrefix((sid, flags) => _islandPostPop!.Prefix(sid, flags));
        // 같은 해시·점유지·표시를 공유하는 분배 Pop으로 교체한다. 공간 등록 뒤 접두·장부가 같은 호출에서 실행된다.
        _terrainPop = new RawPop(Pool, _terrainHash, _terrainSpots, _terrainDisplay, _terrainPostPop);
        _terrainPop.BindDiagnostics(new SquidPopDiagnostics
        {
            Types = types,
            Checking = new RawCheckingFlag(() => AiChecking.Read()),
            Trace = (level, text) => _islandPostPopDiagnostics.Add($"pop-trace:{level}:{text}"),
            Assertion = (expression, file, line) => _islandPostPopDiagnostics.Add($"pop-assert:{expression} {file}:{line}"),
        });
        _terrainPop.BindTransmission((sid, flags) => _terrainPopTransmissions.Add((sid, flags)));
        if (_fortBuilding is not null)
        {
            _terrainPop.SetBuildingSide(_fortBuilding);
        }
        _terrainUnpop = new RawUnpop(Pool, _terrainHash, _terrainSpots, _terrainDisplay);
        _terrainUnpop.BindDiagnostics(new SquidUnpopDiagnostics
        {
            Checking = new RawCheckingFlag(() => AiChecking.Read()),
            Types = types,
            Assertion = (expression, file, line) => _islandPostPopDiagnostics.Add($"unpop-assert:{expression} {file}:{line}"),
        });
        if (_fortBuilding is not null)
        {
            _terrainUnpop.SetBuildingSide(_fortBuilding);
        }
        _terrainFrames = new SquidFrame(Pool, types, SquidFrameBinding.MakeSquidFrameHooks(Pool, types, _terrainDisplay, _terrainUnpop, _terrainPop));
        if (Pool.Edition == OriginalEdition.Patch1078)
        {
            _terrainFrames.BindCurrentTypes(types);
        }
        _islandPostPopState = new IslandPostPopState { SharedLoadingDepth = _islandLoadingDepth };
        _islandPostPop = new RawIslandPostPop(
            Pool,
            new SessionHashLevelView(_terrainHash),
            types,
            frames,
            BridgeState,
            _islandPostPopState,
            new IslandPostPopHooks
            {
                Objects = new BridgeConnectHooks
                {
                    Create = (type, flags) => _terrainFactory!.Create(type, (SidAllocation)flags),
                    SetOwner = owner.Set,
                    Pop = (sid, x, y, flags) => TerrainPopObject(sid, x, y, flags),
                },
                Connect = sid => _islandConnections!.Connect(sid),
                FindTypeAt = (x, y, type) => FindTerrainObjectAt(types, x, y, type),
                UpdateDisplay = sid => _terrainDisplay!.Update(Pool.Slot(sid)),
                IncompleteSupport = (missing, noIsland, noStalag) => _islandPostPopDiagnostics.Add($"incomplete:{missing}:{noIsland}:{noStalag}"),
                Assertion = (expression, file, line) => _islandPostPopDiagnostics.Add($"{expression} {file}:{line}"),
                Detail = text => _islandPostPopDiagnostics.Add(text),
            });
    }

    /// <summary>현재 좌표의 typed 점을 같은 일반 finder로 찾는다. 원문 4b1fa0과 같은 순서다.</summary>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="x">찾을 월드 x 좌표.</param>
    /// <param name="y">찾을 월드 y 좌표.</param>
    /// <param name="type">찾을 타입 번호다.</param>
    /// <remarks>0.601에서 추가했다. `RawDaisDeletion.FindTypeAt`과 같은 절삭·시작 단계·첫 일치 규칙이다.</remarks>
    private int FindTerrainObjectAt(IReadOnlyList<OriginalTypeRecord> types, float x, float y, uint type)
    {
        if (!float.IsFinite(x) || !float.IsFinite(y))
        {
            throw new ArgumentOutOfRangeException(nameof(x), "섬 후처리의 typed 조회 비유한 좌표");
        }
        if (type >= (uint)types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "섬 후처리의 typed 조회 TYPE 범위 오류");
        }
        if (x < -2147483648.0 || x >= 2147483648.0 || y < -2147483648.0 || y >= 2147483648.0)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "섬 후처리의 typed 조회 절삭 범위 오류");
        }
        int cellX = (int)x;
        int cellY = (int)y;
        var finder = new RawSquidFinder(Pool, _terrainHash!, types);
        for (int sid = finder.Begin(new SquidSearchArea(cellX, cellY, cellX, cellY), (types[(int)type].Flags2 & 6u) == 0 ? 1u : 0u);
            sid != 0;
            sid = finder.Next())
        {
            if (Pool.Slot(sid)[RawSidPool.TypeOffset] == type)
            {
                return sid;
            }
        }

        return 0;
    }

    /// <summary>요새 내용물 Form 부착의 세션 소유 몸체다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. 서버 전송은 MP 전송(R6)에서 잇기 전까지 기록만 한다(단독 미션 시작 흐름과 같은 관문).</remarks>
    private RawSquidForms? _fortForms;

    /// <summary>요새 내용물 Form 부착 진단의 세션 소유 기록이다. 부착 경로 실행을 검사에서 확인한다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. 원문 진단 복귀 뒤에도 계속 진행하므로 기록만 남긴다.</remarks>
    private readonly List<string> _fortAttachDiagnostics = [];

    /// <summary>요새 객체 적재의 공장 레벨 저장 몸체다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.630에서 R4-2 공장 초기 레벨용으로 추가했다. 요새 적재 SetState와 공장 업그레이드 SetState가 같은 장기 레벨 몸체를 공유한다. 근거: cpppj RawCampaignAssetBindings.cpp 52·190행의 level 조립과 RawCampaignWorldRuntime.cpp 44행의 setState 결선이다.</remarks>
    private RawSquidLevel? _fortLevel;

    /// <summary>요새 객체 적재의 세션 소유 공장이다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.467에서 요새 객체 live 적재용으로 추가했다. 지형 공장과 같은 풀을 쓰며 서버 할당(0)도 처리한다.</remarks>
    private RawSquidFactory? _fortFactory;

    /// <summary>요새 객체 HP 판정의 세션 소유 보상 몸체다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. 세션 공유 SP 저장소와 지형 장부를 빌린다.</remarks>
    private RawSquidReward? _fortHp;

    /// <summary>실제 요새·사제 회복이 같은 현재 HP 전역을 빌리는 보상 상태다.</summary>
    public SquidRewardState RewardState { get; } = new();

    /// <summary>요새 장치 구성 뒤 같은 실제 HP/보상 처리기를 빌린다.</summary>
    public RawSquidReward Reward => _fortHp ?? throw new InvalidOperationException("세션 실제 HP/보상 미연결");

    /// <summary>지형·요새 공장이 같은 현재 사제TYPE·다이스·약화와 복귀 진단을 읽게 한다.</summary>
    private void BindFactoryHitPoints(RawSquidFactory factory, IReadOnlyList<OriginalTypeRecord> types)
    {
        factory.BindWeakenedMana(GameAux.PriestQuarterHpBox);
        factory.BindHitPointTypes(GameAux.PriestTypeBox, GameAux.DaisTypeBox);
        factory.BindDiagnostics(new()
        {
            Types = types, Checking = new RawCheckingFlag(() => AiChecking.Read()),
            Assertion = (expression, file, line) => throw new InvalidOperationException($"공장 HP 원본 진단: {expression} {file}:{line}"),
        });
    }

    /// <summary>요새 내용물 생성의 세션 소유 몸체다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. 부착은 bound Form 분배로 live 연결하며 서버 전송은 MP 전송(R6)에서 잇는다.</remarks>
    private RawContainedCreate? _fortContained;

    /// <summary>요새 객체 Pop의 건물 옆면·표면 통지 몸체다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. 지형 Pop과 같은 해시·spots를 공유한다(cpppj core.building 대응).</remarks>
    private RawBuildingSpace? _fortBuilding;

    /// <summary>요새 객체 Pop의 세션 소유 몸체다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. 지형 Pop과 같은 해시·spots에 등록한다(cpppj core.pop 공유 대응). 표시·파생은 잇지 않는다.</remarks>
    private RawPop? _fortPop;

    /// <summary>요새 객체 적재의 세션 소유 작업 영역이다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. 미션 적재값(Mission=true·ChaffMode=true, Fort=false·Special=false·Version=0)을 둔다(cpppj RawCampaignSession.cpp 1093행 대응).</remarks>
    private FortObjectLoadState? _fortState;

    /// <summary>요새 객체 적재의 세션 소유 로더다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.467에서 추가했다. Load(fort, 칸) 한 호출이 전체 20개 Terr+Chaff를 적재한다.</remarks>
    private RawFortObjectLoad? _fortLoader;

    /// <summary>Player 월드(화신) 소유자 지정의 세션 소유 몸체다. BuildFortObjectDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.475에서 Player 월드 갈고리 live 연결용으로 추가했다. 요새 객체와 같은 몸체를 공유한다(근거 cpppj RawCampaignSession.cpp 1239행 core.owner). </remarks>
    private SquidOwner? _playerOwner;

    /// <summary>요새 객체 적재의 공장·공간·HP·내용물 장치를 세션에 연결한다. 지형 Pop 장치 뒤에 연결해야 한다(같은 해시·spots·표시를 공유한다).</summary>
    /// <param name="types">같은 실제 TYPE 표다. 패치판 188개여야 한다.</param>
    /// <param name="frames">같은 실제 프레임 표다. 타입 개수와 같아야 한다.</param>
    /// <param name="factory">빌릴 외부 공장. null이면 세션 전용으로 만든다. 넘기면 같은 풀 소속이어야 하며 소유자의 바인딩 그대로 쓴다.</param>
    /// <param name="building">빌릴 외부 건물 공간. null이면 세션 전용으로 만든다. 넘기면 같은 풀·해시 소속이어야 한다.</param>
    /// <remarks>2026-11-12 0.467에서 요새 객체 live 적재용으로 추가했다. 근거: cpppj RawCampaignWorldRuntime.cpp 43~53행(객체 갈고리+MakeFortObjectLoadHooks+RawFortObjectLoad)의 세션 결선이다.
    /// FinishOwner는 기록만 하며(R6 거주 배치에서 잇는다), Destroy·VirtualCount·내용물 부착은 미션 적재(Fort=false)에서 도달하지 않으므로 명시 경계로 둔다.
    /// 2026-11-19 0.630에서 SetState를 장기 레벨 몸체에 이었다. 근거: cpppj RawCampaignAssetBindings.cpp 190행이다.
    /// 2026-11-12 0.475에서 소유자 지정 몸체를 Player 월드 갈고리와 공유하려고 `_playerOwner` 필드에 보관한다.
    /// 0.597에서 저장소 공장 전환용으로 외부 공장 빌림을 추가했다.
    /// 0.599에서 저장소 건물 공간 전환용으로 외부 건물 공간 빌림을 추가했다.</remarks>
    public void BuildFortObjectDevices(IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, RawSquidFactory? factory = null, RawBuildingSpace? building = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        if (types.Count != OriginalConstructors.TypeCount || frames.Count != types.Count)
        {
            throw new ArgumentException("요새 객체 타입·프레임 목록 크기 오류", nameof(types));
        }
        if (_terrainHash is null || _terrainSpots is null || _terrainDisplay is null || _terrainPop is null)
        {
            throw new InvalidOperationException("요새 객체 장치는 지형 Pop 장치 뒤에 연결해야 한다");
        }
        if (factory is not null && !ReferenceEquals(factory.Pool, Pool))
        {
            throw new ArgumentException("요새 객체 외부 공장의 풀 불일치", nameof(factory));
        }
        if (building is not null && (!ReferenceEquals(building.Pool, Pool) || !ReferenceEquals(building.Hash, _terrainHash)))
        {
            throw new ArgumentException("요새 객체 외부 건물 공간의 풀·해시 불일치", nameof(building));
        }
        _fortFactory = factory ?? new RawSquidFactory(Pool, types);
        if (factory is null)
        {
            BindFactoryHitPoints(_fortFactory, types);
        }
        RewardState.SharedPriestType = GameAux.PriestTypeBox;
        RewardState.SharedDaisType = GameAux.DaisTypeBox;
        RewardState.SharedPriestQuarterHp = GameAux.PriestQuarterHpBox;
        RewardState.Checking.Bind(() => AiChecking.Read());
        _fortHp = new RawSquidReward(Pool, types, TerrainBookkeeping, RewardState, Sp);
        _fortHp.BindCurrentTypes(types);
        _fortHp.BindHitPointDiagnostics((expression, file, line) =>
            throw new InvalidOperationException($"보상 HP 원본 진단: {expression} {file}:{line}"));
        _fortForms = new RawSquidForms(Pool, null, (_, _) => { });
        _fortForms.BindAttach(new FormAttachHooks
        {
            Assertion = (expression, file, line) => _fortAttachDiagnostics.Add($"{expression} {file}:{line}"),
        });
        var containedHooks = new ContainedCreateHooks
        {
            NotifySurface = sid => _fortBuilding!.NotifySurface(sid),
            Attach = (form, parent, flags) => _fortForms.Attach(form, parent, flags),
        };
        RawContainedCreate.ConnectFactory(Pool, _fortFactory, containedHooks);
        _fortContained = new RawContainedCreate(Pool, new ContainedCreateState(), containedHooks);
        _fortBuilding = building ?? new RawBuildingSpace(Pool, _terrainHash, types,
            new BuildingSpaceState { IslandType = TerrainGlobals.IslandType, BridgeType = 82 });
        _fortPop = new RawPop(Pool, _terrainHash, _terrainSpots);
        _fortPop.SetBuildingSide(_fortBuilding);
        _fortState = new FortObjectLoadState { Mission = true, ChaffMode = true };
        _fortLevel = new RawSquidLevel(Pool, types, SquidLevelDiagnostics.MakeSquidLevelAssertion(Pool.Edition,
            (expression, file, line) => throw new InvalidOperationException($"요새 레벨 원본 진단: {expression} {file}:{line}")));
        _playerOwner = new SquidOwner(Pool, types, TerrainBookkeeping, OwnerMode);
        // 실제 World와 같은 TYPE를 빌리고 선택된 원문 진단을 현재 호스트 오류 기록으로 전달한다.
        _playerOwner.BindDiagnostics(new SquidOwnerDiagnostics
        {
            Types = types,
            Assertion = (expression, file, line) => throw new InvalidOperationException($"공통 Owner 원본 진단: {expression} {file}:{line}"),
        });
        RawIslandRuntime islands = BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        var hooks = new FortObjectLoadHooks
        {
            NotifySurface = sid => _fortBuilding.NotifySurface(sid),
            // 요새 파일의 공장 초기 레벨을 같은 장기 레벨 몸체에 저장하고 기본 표시를 갱신한다.
            // 2026-11-19 0.630에서 R4-2 공장 초기 레벨용으로 이었다. 근거: cpppj RawCampaignAssetBindings.cpp 190행의 SetState와 RawCampaignWorldRuntime.cpp 44행의 setState 결선이다.
            SetState = (sid, value) => SetFortObjectLevel(sid, value),
            // 요새 적재의 소유자 지정은 신전을 만나면 가상 Owner로 분배한다. 도장·사제 경로는 공통 경로를 쓴다.
            // 2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 바꿨다. 근거: cpppj RawCampaignSession.cpp 3257행의 setOwner 조립이다.
            SetOwner = SetFortObjectOwner,
            HasHitPoints = _fortHp.HasHitPoints,
            MaxHitPoints = _fortHp.MaxHitPoints,
            VirtualCount = _ => throw new InvalidOperationException("Fort 가상 개수 미연결(R6)"),
            Pop = (sid, x, y, flags) =>
            {
                // 실제 몸체가 있는 화신·사제·보호막은 공통 Pop으로 등록한다.
                if (_priests is not null && (_livePostPop!.HandlesPriest(sid) || _livePostPop.HandlesForcefield(sid)))
                {
                    PopWorldObject(sid, x, y, flags);
                    return;
                }
                if (_livePop is not null && _livePostPop is not null && _livePostPop.HandlesPlayer(sid))
                {
                    PopWorldObject(sid, x, y, flags);
                    return;
                }
                // 신전(소용돌이)은 지역 투표 목록에 등록한다. 일반 Pop 분배는 기존 경로를 유지한다.
                if (IsRegionTempleType(types, sid))
                {
                    RegisterRegionTemple(sid, x, y);
                }
                // 다리·섬·표면과 공통 지원 타입은 지형 분배 Pop으로 등록한다. 전용 파생이 필요한 종류는 기록만 남긴다.
                if (TryPopFortTerrain(sid, x, y, flags))
                {
                    // Pop 성공 뒤 실제 좌표의 소유자를 분배한다. 적재 SetOwner(좌표 0·void)와 달리 섬 소유자·첫 지역을 전파한다.
                    // 2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 이었다. 근거: cpppj RawAssetPostPop.cpp 91행이다.
                    if (IsRegionTempleType(types, sid))
                    {
                        DispatchRegionTempleOwner(sid);
                    }

                    return;
                }
                _fortPopSkipped.Add(FormatFortPopSkip(sid, flags, _terrainPostPop is null || _terrainPop is null ? "missing-devices" : "dedicated-r6"));
            },
            Destroy = (_, _) => throw new InvalidOperationException("Fort 파괴 미연결(R6)"),
            CreateContent = _fortContained.Create,
            ChunkRegion = (x, y) => unchecked((sbyte)World.At(x, y).Island),
            TerritoryAllowed = (region, territoryOwner, territory) => islands.Matches(region, unchecked((int)territoryOwner), territory),
            LoadingDepthChanged = SetWorldLoadingDepth,
            FinishOwner = _ => { },
        };
        RawFortObjectLoad.MakeFortObjectLoadHooks(Pool, _fortFactory, _fortHp, _fortContained, hooks);
        _fortLoader = new RawFortObjectLoad(Pool, _terrainHash, types, frames, Game, _fortState, hooks);
    }

    /// <summary>요새 파일의 공장 초기 레벨을 저장하고 기본 표시를 갱신한다.</summary>
    /// <param name="sid">레벨을 쓸 슬롯 번호다.</param>
    /// <param name="value">쓸 레벨 값이다. 0~7이어야 한다.</param>
    /// <remarks>2026-11-19 0.630에서 R4-2 공장 초기 레벨용으로 추가했다. 근거: cpppj RawCampaignAssetBindings.cpp 190행의 SetState(공장 가상84 저장 뒤 가상88 표시 갱신)다. 공장 업그레이드·정기 경로와 같은 몸체를 공유한다.</remarks>
    public void SetFortObjectLevel(int sid, uint value)
    {
        RawSquidLevel level = _fortLevel ?? throw new InvalidOperationException("요새 레벨 몸체 미연결");
        SquidDisplay display = _terrainDisplay ?? throw new InvalidOperationException("요새 레벨 표시 미연결");
        level.Set(sid, value);
        display.Update(Pool.Slot(sid));
    }

    /// <summary>요새 적재·공장 업그레이드가 공유하는 장기 레벨 몸체다.</summary>
    /// <returns>연결 뒤 장기 레벨 몸체다.</returns>
    /// <remarks>2026-11-19 0.630에서 추가했다. L1에서 적재 레벨 저장 경로를 대조할 때 쓴다.</remarks>
    public RawSquidLevel FortLevel => _fortLevel ?? throw new InvalidOperationException("요새 레벨 몸체 미연결");

    /// <summary>적재한 FortTemplate의 객체를 실제 로더로 풀에 만든다. 장치 연결 전에 호출하면 거부한다.</summary>
    /// <param name="fort">객체를 읽을 FortTemplate다.</param>
    /// <param name="slot">미션 Template 칸 번호다. 1~8이면 기본 소유자로 쓴다.</param>
    /// <remarks>2026-11-12 0.467에서 요새 객체 live 적재용으로 추가했다. Generate 뒤 Objects 순서에서 부른다.</remarks>
    /// <summary>지형 분배 Pop으로 잇지 않고 기록만 남긴 요새 Pop 호출이다. R6 파생 본체 대기 목록이다. 0.607에서 추가됐다.</summary>
    private readonly List<string> _fortPopSkipped = [];

    /// <summary>지형 분배 Pop으로 잇지 않고 기록만 남긴 요새 Pop 호출이다. R6 파생 본체 대기 목록이며 풀 바이트에 영향을 주지 않는다. 0.607에서 추가됐다.</summary>
    public IReadOnlyList<string> FortPopSkips => _fortPopSkipped;

    /// <summary>요새 객체가 소용돌이(신전) 종류인지 가린다. 지역 투표·가상 Owner 분배 대상만 고른다.</summary>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="sid">가릴 오브젝트 번호다.</param>
    /// <returns>지역 장치가 연결됐고 신전 종류이면 참이다.</returns>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. 적재 갈고리와 분배 관문이 같은 판정을 공유한다.</remarks>
    private bool IsRegionTempleType(IReadOnlyList<OriginalTypeRecord> types, int sid)
    {
        if (_regionOwnership is null || _regionTemples is null || _regionVortex is null)
        {
            return false;
        }

        uint number = Pool.Slot(sid)[RawSidPool.TypeOffset];
        return number < (uint)types.Count && (types[(int)number].Flags2 & TypeFlag2.Vortex) != 0;
    }

    /// <summary>요새 객체를 다리·섬·표면 분배 또는 공통 후처리로 등록한다. 전용 파생이 필요하면 거짓을 돌려준다.</summary>
    /// <param name="sid">등록할 오브젝트 번호다.</param>
    /// <param name="x">등록할 월드 가로 좌표다.</param>
    /// <param name="y">등록할 월드 세로 좌표다.</param>
    /// <param name="flags">Pop 호출 플래그다.</param>
    /// <returns>지형 분배 Pop으로 등록했으면 참이다. 전용 파생(R6)이나 장치 미연결이면 거짓이다.</returns>
    /// <remarks>0.607에서 R4-2 요새 일반 Pop 결선용으로 추가했다. 분배 본문의 분기 순서와 같은 판정이며 풀·공간을 바꾸기 전에 가린다.
    /// 2026-11-19 0.622에서 전용 자산 본체가 연결된 신전을 지형 분배로 이었다. 근거: cpppj RawCampaignAssetBindings.cpp 181행의 SetAssetPostPop 등록이다.</remarks>
    private bool TryPopFortTerrain(int sid, float x, float y, uint flags)
    {
        // 분배 장치나 그래프 관문이 없으면 기록 경로로 돌린다.
        if (_terrainPostPop is null || _terrainPop is null)
        {
            return false;
        }
        uint vtable = BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid));
        // 전용 자산 본체가 연결된 신전은 지형 분배 Pop으로 등록한다. 다른 전용 파생은 R6 본체를 기다린다.
        if (_vortexAssetPostPop is not null && _terrainDisplay is not null && _terrainPopTypes is { Count: > 0 } popTypes && IsRegionTempleType(popTypes, sid))
        {
            TerrainPopObject(sid, x, y, flags);
            return true;
        }
        // 공장 본체가 연결된 Factory/wrapper는 지형 분배 Pop으로 등록한다. 공통 깊이는 공장 몸체가 한 번 줄인다.
        if (_factoryPostPop is not null && _factoryPostPop.Handles(sid))
        {
            TerrainPopObject(sid, x, y, flags);
            return true;
        }
        // 간헐천 전용 자산 본체가 연결된 Geyser는 지형 분배 Pop으로 등록한다. 전용 파생 검사를 피하려고 공장·신전 다음에 둔다.
        // 2026-11-19 0.634에서 R4-4 간헐천 Pop용으로 추가했다. 근거: cpppj RawSurfaceStorage.cpp 236행의 GeyserPostPop 분배다.
        if (_vortexAssetPostPop is not null && IsGeyserVtable(Pool.Slot(sid)))
        {
            TerrainPopObject(sid, x, y, flags);
            return true;
        }
        // 거주 구역 전용 자산 본체가 연결된 Residence는 지형 분배 Pop으로 등록한다. 다른 전용 파생은 R6 본체를 기다린다.
        // 2026-11-19 0.635에서 R4-4 거주 구역 Pop용으로 추가했다. 근거: cpppj RawSurfaceStorage.cpp의 자산 분배다.
        if (_vortexAssetPostPop is not null && IsResidenceVtable(Pool.Slot(sid)))
        {
            TerrainPopObject(sid, x, y, flags);
            return true;
        }
        // 썬 바리케이드 전용 펜스 본체가 연결된 Fence는 지형 분배 Pop으로 등록한다. 다른 전용 파생은 R6 본체를 기다린다.
        // 2026-11-19 0.636에서 R4-4 펜스 Pop용으로 추가했다. 근거: cpppj RawSurfaceStorage.cpp의 자산 분배와 DefaultResourcePostPop의 펜스 분기다.
        if (_fencePostPop is not null && IsFenceVtable(Pool.Slot(sid)))
        {
            TerrainPopObject(sid, x, y, flags);
            return true;
        }
        // 전용 파생 후처리가 필요한 종류는 R6 본체를 기다린다.
        if (RawPostPop.RequiresDedicatedPostPop(Pool, sid))
        {
            return false;
        }
        // 다리·섬·표면 분기와 공통 지원 표만 지형 분배로 등록한다. 그 밖은 지원표 거부로 기록한다.
        if (!_terrainPostPop.HandlesBridge(sid) && !_terrainPostPop.HandlesIsland(sid) && !_terrainPostPop.HandlesSurface(sid)
            && !OriginalPopDisplay.Supports(Pool.Edition, vtable, flags))
        {
            return false;
        }
        TerrainPopObject(sid, x, y, flags);
        return true;
    }

    /// <summary>요새 Pop 건너뜀 한 줄을 같은 형식으로 만든다.</summary>
    /// <param name="sid">건너뛴 오브젝트 번호다.</param>
    /// <param name="flags">Pop 호출 플래그다.</param>
    /// <param name="reason">건너뛴 이유다. 파생 본체 대기는 dedicated-r6, 장치 미연결은 missing-devices다.</param>
    /// <returns>기록 한 줄이다.</returns>
    private string FormatFortPopSkip(int sid, uint flags, string reason)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        return $"skip sid={sid} type={raw[RawSidPool.TypeOffset]} vtable={BinaryPrimitives.ReadUInt32LittleEndian(raw):X8} flags={flags:X8} {reason}";
    }

    public void LoadFortObjects(FortTemplate fort, uint slot)
    {
        ArgumentNullException.ThrowIfNull(fort);
        if (_fortLoader is null)
        {
            throw new InvalidOperationException("요새 객체 장치가 연결되지 않았다");
        }
        _fortLoader.Load(fort, unchecked((int)slot));
    }


    /// <summary>같은 실제 Player·Pop·정기 프로세스를 조립하고 월드 적재에 넘길 갈고리를 반환한다.</summary>
    /// <returns>세션 수명 동안 같은 Player 몸체와 함께 쓰는 갈고리다.</returns>
    public PlayerWorldHooks BuildPlayerWorldHooks() => BindLivePlayerWorld();

    /// <summary>월드 그래프 재구성의 세션 소유 엔진이다. BuildWorldGraphDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.470에서 그래프 live 재구성용으로 추가했다. 근거: cpppj RawCampaignWorldRuntime.cpp 65·83행(core.graph 공유 엔진)의 세션 결선이다.</remarks>
    private RawGraph? _worldGraph;

    /// <summary>월드 적재 그래프 재구성 장치를 세션에 연결한다. 연결 전에는 재구성 요청이 거부된다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 패치판 188개·CD판 171개여야 한다.</param>
    /// <remarks>2026-11-12 0.470에서 그래프 live 재구성용으로 추가했다. 지형 Pop 장치의 해시·점유 지도(core.hash·core.spots 구실)와 채운 WorldFrames를 그대로 쓰며, ResetTable은 cpppj 적재 경로에서도 호출되지 않아 연결하지 않는다.
    /// 2026-11-12 0.476에서 엔진 표를 세션 설치와 같은 ResetTable 초기 상태에 둔다(근거 cpppj 저장소 Graph가 Acquire 때 ResetTable한 표를 엔진이 빌리는 구조, RawSurfaceWorld.cpp 161행). 초기화 없이 Rebuild하면 빈 표의 임시 번호 0이 기존 graph-0과 충돌해 큰 맵에서 번호가 소진된다.</remarks>
    public void BuildWorldGraphDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException("월드 그래프 타입 목록 크기 오류", nameof(types));
        }
        if (_terrainHash is null || _terrainSpots is null)
        {
            throw new InvalidOperationException("월드 그래프 장치는 지형 Pop 장치 뒤에 연결해야 한다");
        }
        // 월드 프레임(RiftFrameCode 바이트 4개)을 그래프 엔진(FrameCode 문자·정수·플래그)으로 옮기는 반복문이다. 플래그 비트는 양쪽이 같다(fringe 0x01·hard 0x40).
        var frames = new List<IReadOnlyList<FrameCode>>(WorldFrames.Length);
        for (int index = 0; index < WorldFrames.Length; ++index)
        {
            var codes = new List<FrameCode>(WorldFrames[index].Codes.Count);
            for (int code = 0; code < WorldFrames[index].Codes.Count; ++code)
            {
                RiftFrameCode source = WorldFrames[index].Codes[code];
                codes.Add(new FrameCode((char)source.Side, (char)source.Variant, source.Number, (FrameCodeFlags)source.Flags));
            }
            frames.Add(codes);
        }
        _worldGraph = new RawGraph(Pool, _terrainHash, _terrainSpots, types, null, null, frames);
        _worldGraph.ResetTable();
    }

    /// <summary>월드 그래프를 다시 구성하고 세션 표 저장소로 결과를 옮긴다. 장치 연결 전에 호출하면 거부한다.</summary>
    /// <param name="mode">참이면 전체 초기화 재구성, 거짓이면 무효 번호 수집이다.</param>
    /// <remarks>2026-11-12 0.470에서 그래프 live 재구성용으로 추가했다. 적재 흐름의 territory·campaign 갈고리가 mode 거짓으로 부른다. 난수를 소비하지 않으며 슬롯 그래프 바이트·표·스택만 바꾼다.</remarks>
    /// <summary>세션 지형 공용 후처리와 Pop에 월드 그래프 엔진을 연결한다. 섬 후처리·그래프 장치 뒤에 연결해야 한다.</summary>
    /// <param name="requireBuildingSide">참이면 건물 옆면이 없을 때 거부한다. 거짓이면 요새 장치 없이도 결선한다.</param>
    /// <remarks>0.605에서 R4-2 그래프 per-Pop 번호 부여용으로 추가했다. 같은 해시·점유지·표시·장부를 공유하는 분배 Pop을 그래프 엔진과 함께 다시 묶고 공용 후처리의 그래프 관문을 연다.
    /// 섬·표면 접두와 다리 연결은 같은 세션 장치를 그대로 잇는다. 건물 옆면은 요새 장치가 있을 때만 잇는다.</remarks>
    public void WireTerrainPostPopGraph(bool requireBuildingSide = false)
    {
        if (_terrainPostPop is null || _terrainPop is null || _terrainHash is null || _terrainSpots is null
            || _terrainDisplay is null || _terrainPopTypes is null || _worldGraph is null
            || _islandConnections is null || _islandPostPop is null)
        {
            throw new InvalidOperationException("지형 그래프 결선은 섬 후처리·그래프 장치 뒤에 연결해야 한다");
        }
        if (requireBuildingSide && _fortBuilding is null)
        {
            throw new InvalidOperationException("지형 그래프 결선에 건물 공간 장치가 없다");
        }
        if (Pool.Edition == OriginalEdition.Patch1078)
        {
            _worldGraph.BindCurrentTypes(_terrainPopTypes);
        }
        _worldGraph.BindDiagnostics(new GraphDiagnostics
        {
            Checking = new RawCheckingFlag(() => AiChecking.Read()),
            Assertion = (expression, file, line) => throw new InvalidOperationException($"세션 그래프 원본 진단: {expression} {file}:{line}"),
        });
        _terrainPostPop = new RawPostPop(Pool, _terrainPopTypes, TerrainBookkeeping, _worldGraph, sid => _islandConnections.Connect(sid));
        if (Pool.Edition == OriginalEdition.Patch1078)
        {
            _terrainPostPop.BindCurrentTypes(_terrainPopTypes);
        }
        _terrainPostPop.SetIslandPrefix((sid, flags) => _islandConnections.IslandPostPopPrefix(sid, flags));
        _terrainPostPop.SetSurfacePrefix((sid, flags) => _islandPostPop.Prefix(sid, flags));
        BindTerrainCarrierPostPop();
        _terrainPop = new RawPop(Pool, _terrainHash, _terrainSpots, _terrainDisplay, _terrainPostPop);
        _terrainPop.BindDiagnostics(new SquidPopDiagnostics
        {
            Types = _terrainPopTypes,
            Checking = new RawCheckingFlag(() => AiChecking.Read()),
            Trace = (level, text) => _islandPostPopDiagnostics.Add($"pop-trace:{level}:{text}"),
            Assertion = (expression, file, line) => _islandPostPopDiagnostics.Add($"pop-assert:{expression} {file}:{line}"),
        });
        _terrainPop.BindTransmission((sid, flags) => _terrainPopTransmissions.Add((sid, flags)));
        if (_fortBuilding is not null)
        {
            _terrainPop.SetBuildingSide(_fortBuilding);
        }
        _terrainUnpop = new RawUnpop(Pool, _terrainHash, _terrainSpots, _terrainDisplay);
        _terrainUnpop.BindDiagnostics(new SquidUnpopDiagnostics
        {
            Checking = new RawCheckingFlag(() => AiChecking.Read()),
            Types = _terrainPopTypes,
            Assertion = (expression, file, line) => _islandPostPopDiagnostics.Add($"unpop-assert:{expression} {file}:{line}"),
        });
        if (_fortBuilding is not null)
        {
            _terrainUnpop.SetBuildingSide(_fortBuilding);
        }
        _terrainFrames = new SquidFrame(Pool, _terrainPopTypes, SquidFrameBinding.MakeSquidFrameHooks(Pool, _terrainPopTypes, _terrainDisplay, _terrainUnpop, _terrainPop));
        if (Pool.Edition == OriginalEdition.Patch1078)
        {
            _terrainFrames.BindCurrentTypes(_terrainPopTypes);
        }
        TerrainBookkeeping.GraphsEnabled = true;
    }

    /// <summary>세션 지형 공용 후처리와 같은 풀을 쓰는 운반체·피해 몸체다. WireTerrainPostPopGraph에서 연결한다.</summary>
    /// <remarks>2026-11-18 0.608에서 R4-2 농지 배치 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1856행의 MakeNormalTerrainFarmWorldHooks와 RawNormalTerrainWorldBinding.cpp 40행의 SetCarrierPostPop 결선이다.
    /// 농지 지면 교체(타입 121, Damageable 가상표)의 Repop이 공통 경로 대신 전용 Damageable 몸체로 분배되게 한다.</remarks>
    private RawCarrierPostPop? _terrainCarrierPostPop;

    /// <summary>지형 분배 Pop의 운반체·피해 파생을 실제 몸체에 연결한다. 농지 Repop의 Damageable 분배가 미복원으로 멈추지 않게 한다.</summary>
    /// <remarks>2026-11-18 0.608에서 추가했다. 검증·공통 몸체는 같은 공용 후처리(@base)에 위임하며, +0xCC 확인·지면 갱신은 농지 스코프에서 도달할 수 없어 미연결로 둔다(도달하면 실패로 드러난다).</remarks>
    private void BindTerrainCarrierPostPop()
    {
        if (_terrainPostPop is null || _terrainPopTypes is null)
        {
            throw new InvalidOperationException("지형 운반체 몸체는 지형 공용 후처리 뒤에 연결해야 한다");
        }
        var hooks = RawCarrierPostPop.MakeCarrierPostPopHooks(
            Pool,
            _terrainPostPop,
            _ => throw new InvalidOperationException("지형 운반체 +0xCC 확인 미연결(Damageable 직접 호출 경로에서 도달 불가)"),
            (_, _, _) => throw new InvalidOperationException("지형 지면 소유자 갱신 미연결(농지 121에는 갱신 비트가 없다)"));
        _terrainCarrierPostPop = new RawCarrierPostPop(Pool, _terrainPopTypes, new CarrierPostPopState(), hooks);
        _terrainPostPop.SetCarrierPostPop(_terrainCarrierPostPop);
    }

    /// <summary>농지 단일 배치의 월드 효과 여섯 갈고리를 세션 장치에 연결한다. 배치·생성·HP 다섯 갈고리는 파이프라인·공장·보상 실결선기에 위임한다.</summary>
    /// <param name="islands">바인딩과 같은 섬 런타임이다. 지역 소유자를 같은 목록에서 읽는다.</param>
    /// <param name="placement">같은 월드의 실제 배치 파이프라인이다.</param>
    /// <param name="hp">같은 월드의 실제 HP 조회다.</param>
    /// <returns>월드 효과가 연결된 훅 묶음이다.</returns>
    /// <exception cref="InvalidOperationException">농지 배치의 지형 장치 미연결.</exception>
    /// <exception cref="ArgumentException">농지 월드훅 SID 풀 불일치.</exception>
    /// <remarks>2026-11-18 0.613에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. 근거: cpppj/src/client/RawNormalTerrainWorldBinding.cpp 11행의 MakeNormalTerrainFarmWorldHooks다.</remarks>
    public FortRandomPlacementHooks MakeFarmWorldHooks(RawIslandRuntime islands, RawCanonPlacementPipeline placement, RawSquidReward hp)
    {
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(placement);
        ArgumentNullException.ThrowIfNull(hp);
        RawSquidFactory factory = _terrainFactory ?? throw new InvalidOperationException("농지 배치 지형 공장 미연결");
        RawUnpop unpop = _terrainUnpop ?? throw new InvalidOperationException("농지 배치 지형 공간 해제 미연결");
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("농지 배치 지형 TYPE 미연결");
        SquidOwner owner = _islandOwner ?? throw new InvalidOperationException("농지 배치 섬 소유자 미연결");
        if (!ReferenceEquals(Pool, islands.Pool()) || !ReferenceEquals(Pool, placement.Pool) || !ReferenceEquals(Pool, factory.Pool) || !ReferenceEquals(Pool, hp.Pool))
        {
            throw new ArgumentException("농지 월드훅 SID 풀 불일치");
        }

        FortRandomPlacementHooks hooks = RawFortRandomPlacement.MakeFortRandomPlacementHooks(Pool, placement, factory, hp, new FortRandomPlacementHooks());
        hooks.ChunkRegion = (x, y) => unchecked((sbyte)World.At(x, y).Island);
        hooks.RegionOwner = region => islands.Owner(region);
        hooks.SetOwner = RawBridgeConnect.MakeOwnerDispatch(Pool, BridgeState, owner.Set);
        hooks.Unpop = (sid, flags) => unpop.Unpop(sid, types[Pool.Slot(sid)[RawSidPool.TypeOffset]], flags);
        hooks.Repop = (sid, flags) =>
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            float x = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[14..]));
            float y = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..]));
            TerrainPopObject(sid, x, y, flags);
        };
        hooks.Pop = (sid, x, y, flags) => TerrainPopObject(sid, x, y, flags);
        return hooks;
    }

    /// <summary>농지 파이프라인이 읽는 세션 소유 상태다. BuildFarmPlacementDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.614에서 R4-2 농지 실결선용으로 추가했다. 같은 인스턴스를 파이프라인이 빌리며 수명은 세션이 진다.</remarks>
    private CanonPlacementPipelineState? _farmPipelineState;

    /// <summary>농지 파이프라인의 별도 추가 목록이다. BuildFarmPlacementDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. 적재 스코프에서는 비어 있으며 R6의 실제 앵커 추가 연결은 다음 단위에서 잇는다.</remarks>
    private RawPostPopList? _farmAdditional;

    /// <summary>농지 파이프라인의 내용물 찾기 상태다. BuildFarmPlacementDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. 적재 스코프에서는 기본값이며 실제 boss 결선은 R6에서 잇는다.</remarks>
    private ContainedFinderState? _farmContained;

    /// <summary>농지 파이프라인의 섬 지역 목록이다. BuildFarmPlacementDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. 농지 타입(121)은 소용돌이 제한을 읽지 않으므로 비어 있으며 R4-6의 실제 제한·존재 결선은 다음 단위에서 잇는다.</remarks>
    private List<CanonPlacementIslandRegion>? _farmRegions;

    /// <summary>농지 파이프라인이 읽는 타입별 프레임 자료다. BuildFarmPlacementDevices가 정의에서 만든다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. 자산 번호 70+적재 순서의 FrameTable과 기본 프레임을 둔다.</remarks>
    private PriestPlainCanonType[]? _farmFrames;

    /// <summary>농지 파이프라인이 읽는 타입별 핫스폿 자료다. BuildFarmPlacementDevices가 타입 표에서 만든다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. TYPE 정의 기준점과 같은 값을 둔다.</remarks>
    private PriestTypeHotspot[]? _farmHotspots;

    /// <summary>농지 파이프라인 입력 묶음이다. BuildFarmPlacementDevices가 세션 장치를 공유해 만든다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. 같은 인스턴스를 파이프라인이 빌린다.</remarks>
    private CanonPlacementPipelineData? _farmPipelineData;

    /// <summary>농지 단일 배치의 실제 판단 파이프라인이다. BuildFarmPlacementDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. 근거: cpppj RawCampaignSession.cpp 1827행의 적재 파이프라인 조립이다.</remarks>
    private RawCanonPlacementPipeline? _farmPipeline;

    /// <summary>농지 단일 배치 몸체다. BuildFarmPlacementDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. MakeFarmWorldHooks의 여섯 갈고리와 실결선 다섯 갈고리를 공유한다.</remarks>
    private RawFortRandomPlacement? _farmSingle;

    /// <summary>농지 군집 몸체다. BuildFarmPlacementDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.614에서 추가했다. 근거: cpppj RawNormalTerrainWorldBinding.cpp 38행의 farms_ 조립이다.</remarks>
    private RawFarmCluster? _farmCluster;

    /// <summary>농지 파이프라인·단일·군집을 세션 장치에 연결한다. Generate 현장의 빈 place를 실제 배치로 바꾼다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <param name="definitions">로딩 순서와 같은 타입 정의 목록이다. 프레임·패턴 번호를 만든다.</param>
    /// <param name="shapes">같은 순서의 표시 셰이프 목록이다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">농지 배치의 지형·요새·바인딩 장치 미연결.</exception>
    /// <exception cref="ArgumentException">농지 파이프라인 자료·풀 불일치.</exception>
    /// <remarks>2026-11-18 0.614에서 R4-2 농지 잔여의 Generate 실결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1826~1858행과 RawNormalTerrainWorldBinding.cpp 36~40행이다. 동맹·충돌·추가·내용물의 실제 게임 값과 섬 제한·존재·첫 지역·정기 처리는 R4-6·R6에서 잇는다.</remarks>
    public void BuildFarmPlacementDevices(IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeDefinition> definitions, IReadOnlyList<SquidDisplayShape> shapes)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(definitions);
        ArgumentNullException.ThrowIfNull(shapes);
        if (_farmPipeline is not null || _farmSingle is not null || _farmCluster is not null)
        {
            throw new InvalidOperationException("농지 배치 장치 중복 연결");
        }
        RawSquidHash hash = _terrainHash ?? throw new InvalidOperationException("농지 배치 공간 해시 미연결");
        byte[] spots = _terrainSpots ?? throw new InvalidOperationException("농지 배치 점유지 미연결");
        IReadOnlyList<OriginalTypeRecord> popTypes = _terrainPopTypes ?? throw new InvalidOperationException("농지 배치 지형 TYPE 미연결");
        IReadOnlyList<SquidDisplayShape> popShapes = _terrainPopShapes ?? throw new InvalidOperationException("농지 배치 표시 셰이프 미연결");
        if (!ReferenceEquals(types, popTypes) || !ReferenceEquals(shapes, popShapes))
        {
            throw new ArgumentException("농지 배치 TYPE·셰이프는 지형 Pop 장치와 같은 인스턴스여야 한다");
        }
        RawNormalTerrainWorldBinding binding = TerrainBinding ?? throw new InvalidOperationException("농지 배치 지형 바인딩 미연결");
        RawSquidReward hp = _fortHp ?? throw new InvalidOperationException("농지 배치 HP·보상 미연결");
        uint expected = Pool.Edition == OriginalEdition.Patch1078 ? 188u : 171u;
        if ((uint)types.Count != expected || shapes.Count != types.Count)
        {
            throw new ArgumentException("농지 배치 타입·셰이프 크기 오류");
        }
        if (RiftTypeTable.FirstAssetTypeNumber + definitions.Count > WorldFrames.Length)
        {
            throw new ArgumentException("농지 배치 정의가 프레임 표 폭을 넘는다");
        }
        var frames = new PriestPlainCanonType[types.Count];
        // 모든 타입 번호마다 빈 프레임 표를 먼저 둔다.
        for (int index = 0; index < frames.Length; ++index)
        {
            frames[index] = new PriestPlainCanonType();
        }
        // 자산 순서대로 프레임 코드와 기본 프레임을 채운다.
        for (int asset = 0; asset < definitions.Count; ++asset)
        {
            int number = RiftTypeTable.FirstAssetTypeNumber + asset;
            frames[number].Frames = definitions[asset].FrameTable();
            frames[number].DefaultFrame = definitions[asset].SpecialFrames.DefaultFrame;
        }
        var hotspots = new PriestTypeHotspot[types.Count];
        // 모든 타입 번호마다 TYPE 정의 기준점을 넣는다.
        for (int index = 0; index < hotspots.Length; ++index)
        {
            hotspots[index] = new PriestTypeHotspot(types[index].HotspotX, types[index].HotspotY);
        }
        var state = new CanonPlacementPipelineState();
        // 원문 패턴 전역 8개를 로딩 순서에서 찾는다. 0.613 테스트의 고정 107·82·94·157·131·129·140·142와 같은 값이어야 한다.
        string[] patterns = ["puzzlePiece", "bridge", "island", "noIsland", "thunderCannon", "rainCannon", "windArcher", "windBlocker"];
        IReadOnlyList<string> order = RiftTypeLoadOrder.Get(Pool.Edition);
        // 패턴 이름마다 70+적재 순서를 구한다.
        for (int index = 0; index < patterns.Length; ++index)
        {
            int found = -1;
            // 로딩 순서에서 첫 일치를 찾는다.
            for (int slot = 0; slot < order.Count; ++slot)
            {
                if (RiftTypeDefinition.AsciiEqual(order[slot], patterns[index]))
                {
                    found = slot;
                    break;
                }
            }
            if (found < 0)
            {
                throw new InvalidOperationException("농지 배치 패턴 타입을 찾을 수 없다: " + patterns[index]);
            }
            state.Geometry.PatternTypes[index] = unchecked((uint)(RiftTypeTable.FirstAssetTypeNumber + found));
        }
        // 적재 스코프 농지(소유자 0)의 MayPlace가 그래프 미활성으로 거부되지 않게 한다. 동맹·충돌 실제 값은 R6에서 잇는다.
        state.Permission.GraphReady = 1;
        state.Permission.UseAlliances = 1;
        state.Anchor.GraphReady = 1;
        state.Anchor.FenceType = 90;
        state.Anchor.WindArcherType = 140;
        state.Placement.LocalPlayer = 1;
        var additional = new RawPostPopList();
        var contained = new ContainedFinderState();
        var regions = new List<CanonPlacementIslandRegion>();
        var data = new CanonPlacementPipelineData
        {
            Types = types,
            Frames = frames,
            Shapes = shapes,
            Hotspots = hotspots,
            Spots = spots,
            Islands = hash.BorrowEntries(0),
            Regions = regions,
        };
        var pipeline = new RawCanonPlacementPipeline(Pool, hash, data, TerrainBookkeeping, additional, contained, state);
        FortRandomPlacementHooks hooks = MakeFarmWorldHooks(binding.Islands, pipeline, hp);
        var single = new RawFortRandomPlacement(Pool, hash, spots, types, WorldFrames, Game, PlacementState, hooks);
        var clusterHooks = new FarmClusterHooks { Assertion = _ => { } };
        RawFarmCluster.MakeFarmClusterPlacementHook(Pool, single, clusterHooks);
        var cluster = new RawFarmCluster(Pool, World, binding.Islands, PlacementState, TerrainFarm, clusterHooks);
        _farmFrames = frames;
        _farmHotspots = hotspots;
        _farmPipelineState = state;
        _farmAdditional = additional;
        _farmContained = contained;
        _farmRegions = regions;
        _farmPipelineData = data;
        _farmPipeline = pipeline;
        _farmSingle = single;
        _farmCluster = cluster;
    }

    /// <summary>농지 배치 파이프라인을 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>같은 월드의 실제 배치 파이프라인이다.</returns>
    /// <remarks>2026-11-18 0.614에서 추가했다. MayPlace 동기화의 실제 게임 값(R4-6·R6) 연결 전에는 적재 스코프 기본값으로 동작한다.</remarks>
    public RawCanonPlacementPipeline FarmPipeline => _farmPipeline ?? throw new InvalidOperationException("농지 배치 파이프라인 미연결");

    /// <summary>군집 사각형·지역·수량을 실제 단일 배치로 실행한다. Generate 현장의 place 효과다.</summary>
    /// <param name="area">군집 사각형이다.</param>
    /// <param name="region">군집 지역이다.</param>
    /// <param name="quantity">성공 목표 수량이다.</param>
    /// <remarks>2026-11-18 0.614에서 추가했다. 근거: cpppj RawNormalTerrainWorldBinding.cpp 190행의 farms_-&gt;Place 호출이다.</remarks>
    public void PlaceFarm(FortRandomRectangle area, int region, uint quantity)
    {
        RawFarmCluster cluster = _farmCluster ?? throw new InvalidOperationException("농지 군집 미연결");
        cluster.Place(area, region, quantity);
    }

    /// <summary>지역 칠하기·소유권 투표의 세션 소유 상태다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 결선용으로 추가했다. 같은 인스턴스를 도장·투표가 빌리며 수명은 세션이 진다.</remarks>
    private RegionPaintState? _regionPaintState;

    /// <summary>지역 소유권 투표의 세션 소유 상태다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 투표용으로 추가했다. 같은 인스턴스를 도장·투표가 빌리며 수명은 세션이 진다.</remarks>
    private RegionOwnershipState? _regionOwnershipState;

    /// <summary>지역 투표가 읽는 세션 소유 별도(outpost) 목록이다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 투표용으로 추가했다. 2026-11-19 0.622에서 세션 공유 100칸 목록으로 바꿨다. 근거: cpppj o/RawAssetPostPop.h 59행의 Outposts()와 0055a708 헤더다.</remarks>
    private RawPostPopList? _regionAdditional;

    /// <summary>지역 투표가 읽는 세션 소유 신전 목록이다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 투표용으로 추가했다. 요새 적재 중 신전 배치를 만나면 등록한다. 2026-11-19 0.622에서 세션 공유 20칸 목록으로 바꿨다. 근거: cpppj o/RawAssetPostPop.h 55행의 Temples()와 005e1898 헤더다.</remarks>
    private RawPostPopList? _regionTemples;

    /// <summary>사원 전용 후처리가 쓰는 세션 소유 자산 상태다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. 신전·별도 목록은 세션 공유 목록을 빌리고 로딩 깊이는 SetWorldLoadingDepth가 동기화한다.</remarks>
    private AssetPostPopState? _vortexAssetState;

    /// <summary>사원 전용 후처리가 쓰는 세션 소유 운반체 몸체다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. 피해 후처리의 공통 장부와 지면 갱신을 잇는다.</remarks>
    private RawCarrierPostPop? _vortexAssetCarrier;

    /// <summary>사원 전용 후처리가 쓰는 세션 소유 자산 본체다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. 지형 분배 Pop의 자산 갈고리로 등록한다.</remarks>
    private RawAssetPostPop? _vortexAssetPostPop;

    /// <summary>공장 정기 트리거가 쓰는 세션 소유 공장 본체다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.625에서 R4-2 공장 정기 트리거용으로 추가했다. 지형 분배 Pop의 공장 갈고리로 등록하며 세션 공유 목록·운반체·프레임을 빌린다.</remarks>
    private RawFactoryPostPop? _factoryPostPop;

    /// <summary>공장 정기 분배 전에 사용하던 공통 정기 분배기다. 사제 월드 결선이 이 사슬을 감싸므로 해제 순서는 사제가 먼저다.</summary>
    /// <remarks>2026-11-19 0.629에서 R4-2 공장 정기 분배용으로 추가했다. 공장 가상표가 아니면 이전 처리기로 넘긴다.</remarks>
    private RegularHandler? _beforeFactoryRegular;

    /// <summary>사전·사후Warning과 별개인 간헐천 AI 등록 요청 기록이다. 세션 연결 뒤에만 쌓이며 덤프에는 나가지 않는다.</summary>
    /// <remarks>2026-11-19 0.634에서 R4-4 간헐천 Pop용으로 추가했다. 실제 AI 등록은 R4-5에서 잇는다.</remarks>
    private readonly List<string> _geyserRegistrations = [];

    /// <summary>공장 중복 삭제 직전의 원본 경고 기록이다. 세션 연결 뒤에만 쌓이며 덤프에는 나가지 않는다.</summary>
    /// <remarks>2026-11-19 0.625에서 R4-2 공장 정기 트리거용으로 추가했다. 원본 로그를 L1에서 대조할 때 쓴다.</remarks>
    private readonly List<string> _factoryWarnings = [];

    /// <summary>사원 전용 후처리가 요청한 충격음 횟수다. Generate 범위의 실제 재생은 R6 음향에서 잇는다.</summary>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. L2는 음소거이므로 덤프에 영향을 주지 않는다.</remarks>
    private uint _vortexAssetSoundRequests;

    /// <summary>지역 투표가 읽는 세션 소유 타입별 테마 표다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 투표용으로 추가했다. 같은 인스턴스를 투표가 빌린다.</remarks>
    private uint[]? _regionThemes;

    /// <summary>지역 getter용 세션 소유 지형 몸체다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 투표용으로 추가했다. 배치 순회에는 쓰지 않고 RegionAt 조회에만 쓴다.</remarks>
    private RawCanonPlacementTerrain? _regionTerrain;

    /// <summary>지역 getter용 세션 소유 지형 상태다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 투표용으로 추가했다. 같은 인스턴스를 지형 몸체가 빌린다.</remarks>
    private CanonPlacementTerrainState? _regionTerrainState;

    /// <summary>같은 월드의 실제 지역 도장 몸체다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 칠하기 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1266~1267행의 ownership 조립이다.</remarks>
    private RawRegionPaint? _regionPaint;

    /// <summary>같은 월드의 실제 지역 투표 몸체다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 투표용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1266~1267행의 ownership 조립이다.</remarks>
    private RawRegionOwnership? _regionOwnership;

    /// <summary>같은 월드의 실제 신전 가상 소유자 몸체다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다. 요새 적재의 소유자 지정이 신전을 만나면 섬·지역·Player 기록을 갱신한다. 근거: cpppj RawCampaignOwnership.cpp 54~60행의 소유권 조립이다.</remarks>
    private RawVortexOwner? _regionVortex;

    /// <summary>신전 가상 Owner가 패치 생산 갱신 뒤에 증가시키는 세션 소유 revision이다. 덤프에는 직접 나가지 않는다.</summary>
    /// <remarks>2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다. CD판에서는 쓰지 않는다.</remarks>
    private readonly UIntBox _regionVortexRevision = new();

    /// <summary>신전 가상 Owner가 요청한 생산 갱신 횟수다. Generate 범위의 UI 갱신은 R6에서 잇는다.</summary>
    /// <remarks>2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다. L1에서 호출 순서만 확인한다.</remarks>
    private uint _regionVortexRefreshes;

    /// <summary>신전 가상 Owner가 존재하지 않는 섬을 만나 보고한 진단이다. 보고 뒤에도 계속 진행한다.</summary>
    /// <remarks>2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다. 근거: cpppj RawVortexOwnerDiagnostic.cpp의 보고 뒤 계속이다.</remarks>
    private readonly List<string> _regionVortexDiagnostics = [];

    /// <summary>지역 칠하기·소유권 투표 장치를 세션에 연결한다. 요새 객체 적재의 신전 배치가 투표를 일으킨다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <param name="themeMode">패치 theammode 읽기다. 없으면 1(켜짐)을 쓴다.</param>
    /// <exception cref="InvalidOperationException">지역 투표의 지형 장치·공통 Owner·지형 바인딩이 미연결이다.</exception>
    /// <exception cref="ArgumentException">지역 투표의 TYPE·풀 불일치다.</exception>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1266~1267행의 ownership 조립과 RawCampaignOwnership.cpp 49~53행의 paint·ownership·terrain 결선이다. 공장 후처리·신전 경로의 실제 트리거는 R4-6·R6에서 잇는다.
    /// 2026-11-19 0.621에서 신전 가상 Owner 결선을 같은 장치에 이었다. 근거: cpppj RawCampaignOwnership.cpp 54~60행이다. 도장의 SetOwner 갈고리는 재귀를 피하려고 가상 Owner를 거치지 않는다.</remarks>
    public void BuildRegionOwnershipDevices(IReadOnlyList<OriginalTypeRecord> types, Func<uint>? themeMode = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_regionPaint is not null || _regionOwnership is not null)
        {
            throw new InvalidOperationException("지역 투표 장치 중복 연결");
        }

        // 거주 구역 신전 일치 판정이 같은 테마 모드 값을 읽는다. 2026-11-19 0.635에서 추가했다.
        _themeModeReader = themeMode;

        RawSquidHash hash = _terrainHash ?? throw new InvalidOperationException("지역 투표 공간 해시 미연결");
        IReadOnlyList<OriginalTypeRecord> popTypes = _terrainPopTypes ?? throw new InvalidOperationException("지역 투표 지형 TYPE 미연결");
        if (!ReferenceEquals(types, popTypes))
        {
            throw new ArgumentException("지역 투표 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        if (WorldFrames.Length != types.Count)
        {
            throw new ArgumentException("지역 투표 프레임 표 크기 오류", nameof(types));
        }

        ushort[] surfaces = hash.BorrowEntries(0);
        var paintState = new RegionPaintState();
        var ownershipState = new RegionOwnershipState();
        // 섬 소유권 투표가 읽는 별도·신전 목록은 세션 공유 목록이다. 근거: cpppj RawCampaignAssetBindings.cpp 163행의 assets.Outposts()/Temples()다.
        var additional = SidLists.Outposts;
        var temples = SidLists.Temples;
        var regionThemes = new uint[types.Count];
        // 모든 타입 번호마다 현재 테마 값을 넣는 반복문이다. 투표의 kind→테마 판정이 읽는다.
        for (int i = 0; i < regionThemes.Length; ++i)
        {
            regionThemes[i] = unchecked((uint)types[i].Theme);
        }

        var terrainState = new CanonPlacementTerrainState();
        var plain = new PriestPlainCanonType[types.Count];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다. RegionAt 조회는 프레임을 읽지 않는다.
        for (int i = 0; i < plain.Length; ++i)
        {
            plain[i] = new PriestPlainCanonType();
        }

        var terrain = new RawCanonPlacementTerrain(Pool, types, plain, surfaces, terrainState, new CanonPlacementTerrainHooks
        {
            CandidatePermission = (_, _) => throw new InvalidOperationException("지역 소유권 getter의 배치 후보 호출"),
            ShapePermission = (_, _) => throw new InvalidOperationException("지역 소유권 getter의 배치 모양 호출"),
            FinishRelations = (_, _) => throw new InvalidOperationException("지역 소유권 getter의 배치 완료 호출"),
        });
        var paintHooks = new RegionPaintHooks
        {
            SetOwner = SetWorldObjectOwner,
            BoundaryAt = (x, y, type) => FindTerrainObjectAt(types, x, y, type),
            RegionAt = (x, y) => terrain.RegionAt((int)x, (int)y),
        };
        var paint = new RawRegionPaint(Pool, WorldFrames, surfaces, TerrainGlobals.IslandType, PlacementState.EdgeFarmType, Game, paintState, ownershipState, paintHooks);
        var ownershipHooks = new RegionOwnershipHooks { ThemeMode = themeMode ?? (() => 1u) };
        ownershipHooks = RawRegionPaint.MakePaintOwnershipHooks(Pool, paint, ownershipHooks);
        ownershipHooks = RawRegionOwnership.MakeTerrainRegionOwnershipHooks(Pool, terrain, ownershipHooks);
        var ownership = new RawRegionOwnership(Pool, additional, temples, regionThemes, ownershipState, ownershipHooks);
        _regionPaintState = paintState;
        _regionOwnershipState = ownershipState;
        _regionAdditional = additional;
        _regionTemples = temples;
        _regionThemes = regionThemes;
        _regionTerrainState = terrainState;
        _regionTerrain = terrain;
        _regionPaint = paint;
        _regionOwnership = ownership;
        BuildRegionVortexDevices(types, terrain, ownership);
        BuildRegionVortexPostPopDevices(types);
    }

    /// <summary>지역 투표 몸체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>같은 월드의 실제 지역 투표 몸체다.</returns>
    /// <remarks>2026-11-18 0.619에서 추가했다. 공장·신전 트리거 결선(R4-6·R6) 전까지는 적재 중 신전 배치만 투표를 일으킨다.</remarks>
    public RawRegionOwnership RegionOwnership => _regionOwnership ?? throw new InvalidOperationException("지역 투표 미연결");

    /// <summary>지역 도장 몸체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>같은 월드의 실제 지역 칠하기 몸체다.</returns>
    /// <remarks>2026-11-18 0.619에서 추가했다. 소유자 투표의 도장 효과를 L2에서 대조할 때 쓴다.</remarks>
    public RawRegionPaint RegionPaint => _regionPaint ?? throw new InvalidOperationException("지역 도장 미연결");

    /// <summary>지역 신전 목록을 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>요새 적재 중 등록한 신전 목록이다. 세션 공유 20칸 목록이다.</returns>
    /// <remarks>2026-11-18 0.619에서 추가했다. 2026-11-19 0.622에서 세션 공유 목록으로 바꿨다.</remarks>
    public RawPostPopList RegionTemples => _regionTemples ?? throw new InvalidOperationException("지역 신전 목록 미연결");

    /// <summary>지역 투표가 읽는 별도(outpost) 목록을 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>섬 소유권 투표가 읽는 세션 공유 100칸 목록이다.</returns>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. 근거: cpppj o/RawAssetPostPop.h 59행의 Outposts()다.</remarks>
    public RawPostPopList RegionAdditional => _regionAdditional ?? throw new InvalidOperationException("지역 별도 목록 미연결");

    /// <summary>신전 배치를 지역 투표 목록에 등록하고 즉시 투표·도장한다. Generate 뒤 Objects 적재에서 신전을 만나면 부른다.</summary>
    /// <param name="sid">등록할 신전 슬롯 번호다.</param>
    /// <param name="x">신전 가로 좌표다.</param>
    /// <param name="y">신전 세로 좌표다.</param>
    /// <remarks>2026-11-18 0.619에서 R4-2 지역 소유권 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 2694행의 vortex regionOwnership 호출과 RawAssetPostPop.cpp 287행의 신전 목록 등록이다. 좌표 쓰기·목록 등록·투표까지만 수행하고 신전 전용 후처리 본체는 R6에서 잇는다.</remarks>
    public void RegisterRegionTemple(int sid, float x, float y)
    {
        RawPostPopList temples = _regionTemples ?? throw new InvalidOperationException("지역 신전 목록 미연결");
        RawRegionOwnership ownership = _regionOwnership ?? throw new InvalidOperationException("지역 투표 미연결");
        Span<byte> raw = Pool.AllocatedBytes(sid);
        uint xb = BitConverter.SingleToUInt32Bits(x);
        uint yb = BitConverter.SingleToUInt32Bits(y);
        raw[14] = unchecked((byte)xb);
        raw[15] = unchecked((byte)(xb >> 8));
        raw[16] = unchecked((byte)(xb >> 16));
        raw[17] = unchecked((byte)(xb >> 24));
        raw[18] = unchecked((byte)yb);
        raw[19] = unchecked((byte)(yb >> 8));
        raw[20] = unchecked((byte)(yb >> 16));
        raw[21] = unchecked((byte)(yb >> 24));
        temples.Append(sid);
        ownership.Update(x, y);
    }

    /// <summary>신전 가상 Owner 장치를 세션에 연결한다. 요새 적재의 소유자 지정이 신전을 만나면 섬·지역·Player 기록을 갱신한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <param name="terrain">지역 번호를 조회하는 같은 getter 지형 몸체다.</param>
    /// <param name="ownership">같은 월드의 실제 지역 투표 몸체다.</param>
    /// <exception cref="InvalidOperationException">가상 Owner의 공통 Owner·지형 바인딩이 미연결이다.</exception>
    /// <remarks>2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다. 근거: cpppj RawCampaignOwnership.cpp 54~60행의 소용돌이→섬 소유자→첫 지역 사슬이다. 별도(outpost) 목록은 같은 투표의 추가 목록을 빌리며 실제 목록 생산자는 R11에서 잇는다.</remarks>
    private void BuildRegionVortexDevices(IReadOnlyList<OriginalTypeRecord> types, RawCanonPlacementTerrain terrain, RawRegionOwnership ownership)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(terrain);
        ArgumentNullException.ThrowIfNull(ownership);
        if (_regionVortex is not null)
        {
            throw new InvalidOperationException("지역 가상 Owner 장치 중복 연결");
        }

        SquidOwner baseOwner = _playerOwner ?? throw new InvalidOperationException("지역 가상 Owner 공통 Owner 미연결");
        RawNormalTerrainWorldBinding binding = TerrainBinding ?? throw new InvalidOperationException("지역 가상 Owner 지형 바인딩 미연결");
        RawPostPopList additional = _regionAdditional ?? throw new InvalidOperationException("지역 가상 Owner 별도 목록 미연결");
        var hooks = new VortexOwnerHooks
        {
            IslandNumber = (x, y) => RegionIslandAt(x, y),
            IslandExists = island => binding.Islands.Exists(island) != 0,
            SetIslandOwner = (island, owner) => binding.Islands.Owner(island, owner),
            RegionOwnership = (x, y) => ownership.Update(x, y),
            RefreshProduction = () => { _regionVortexRefreshes++; },
            MissingIsland = (sid, island, owner) => _regionVortexDiagnostics.Add($"vortex-missing island={island} owner={owner} sid={sid}"),
        };
        _regionVortex = new RawVortexOwner(Pool, baseOwner, PlayerRecords, additional, _regionVortexRevision, hooks);
    }

    /// <summary>사원 전용 후처리 자산 본체를 세션에 연결하고 지형 분배 Pop의 자산 갈고리로 등록한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">자산 후처리의 지형 분배·공통 Owner·프로세스·지형 바인딩이 미연결이다.</exception>
    /// <exception cref="ArgumentException">자산 후처리의 TYPE·풀 불일치다.</exception>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. 근거: cpppj RawCampaignAssetBindings.cpp 149~164행의 PostHooks 조립과 181행의 SetAssetPostPop 등록이다. 신전 종류만 분배가 열리며 사제·운반체·피해·Gun·거주 구역·간헐천·너겟·바리케이드는 전용 파생(R6)에서 잇는다.</remarks>
    private void BuildRegionVortexPostPopDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_vortexAssetPostPop is not null)
        {
            throw new InvalidOperationException("사원 자산 후처리 장치 중복 연결");
        }

        if (!ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("자산 후처리 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        RawPostPop terrainPostPop = _terrainPostPop ?? throw new InvalidOperationException("자산 후처리 지형 분배 미연결");
        RawNormalTerrainWorldBinding binding = TerrainBinding ?? throw new InvalidOperationException("자산 후처리 지형 바인딩 미연결");
        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("자산 후처리 정기 프로세스 미연결");
        var assetState = new AssetPostPopState
        {
            SharedTemples = SidLists.Temples,
            SharedOutposts = SidLists.Outposts,
            SharedResidences = IslandResidences,
            SharedGeysers = SidLists.Geysers,
        };
        assetState.LoadingDepth = _worldLoadingDepth;
        var carrierState = new CarrierPostPopState
        {
            SharedBoss = () => Gameplay.Authority.Boss,
            SharedLoadingDepth = () => _worldLoadingDepth,
            SharedGeyserType = () => GameAux.GeyserType,
        };
        var carrierHooks = RawCarrierPostPop.MakeCarrierPostPopHooks(
            Pool,
            terrainPostPop,
            static _ => { },
            // 적재 중 지면 받침 소유자 갱신은 같은 월드의 실제 섬 후처리로 실행한다. 근거: cpppj RawCampaignSession.cpp 1198행의 claimGround 조립이다.
            // 2026-11-19 0.636에서 R4-4 펜스 Pop용으로 연결했다. Flags1 0x400 표식이 있는 적재가 대상이며 간헐천은 제외된다.
            (x, y, owner) => (_islandPostPop ?? throw new InvalidOperationException("지면 소유자 섬 후처리 미연결")).SetSupportOwner(x, y, owner));
        var carrier = new RawCarrierPostPop(Pool, types, carrierState, carrierHooks);
        if (Pool.Edition == OriginalEdition.Patch1078)
        {
            carrier.BindCurrentTypes(types);
        }

        var hooks = new AssetPostPopHooks
        {
            ReserveRegular = static () => true,
            AddRegular = (sid, ev, payload) => { processes.AddRegular(sid, ev, payload); },
            TypeDword = TempleTypeDword,
            PlayImpactSound = _ => { _vortexAssetSoundRequests++; },
            ClearPlacement = () => { },
            IslandNumber = (x, y) => RegionIslandAt(x, y),
            IslandExists = island => binding.Islands.Exists(island) != 0,
            IslandTemple = island => unchecked((int)binding.Islands.Temple(island)),
            RegisterTemple = (island, sid) => binding.Islands.Temple(island, unchecked((uint)sid)),
            RegionNumber = (x, y) => World.At((int)x / 16, (int)y / 16).Island,
            Destroy = (sid, flags) => throw new InvalidOperationException("신전 swept 삭제 미연결(R6)"),
            HasContent = (sid, slot) => throw new InvalidOperationException("신전 내용물 조회 미연결(R6)"),
            CreateSlot = (sid, slot) => throw new InvalidOperationException("신전 내용물 생성 미연결(R6)"),
            RefreshProduction = () => { _regionVortexRefreshes++; },
            Assertion = (expression, file, line) => throw new InvalidOperationException($"사원 자산 후처리 원본 진단: {expression} {file}:{line}"),
            // 간헐천 목록 등록은 실제 AI 등록 대신 기록만 남긴다. R4-5에서 실제 등록으로 잇는다.
            // 2026-11-19 0.634에서 R4-4 간헐천 Pop용으로 추가했다. 근거: cpppj RawCampaignAssetBindings.cpp의 PostHooks 조립이다.
            GeyserRegistration = (sid, op) => _geyserRegistrations.Add($"geyser sid={sid} op={op}"),
            // 간헐천 이웃 찾기는 같은 월드의 실제 finder를 빌린다. 근거: cpppj RawAssetPostPop.cpp 257행의 visitAtPosition 분기다.
            VisitAtPosition = RawAssetPostPop.MakeAssetPostPopFinder(Pool, _terrainHash ?? throw new InvalidOperationException("간헐천 이웃 찾기 해시 미연결"), types),
            // 이웃 섬·종유 해제는 같은 월드의 실제 공간 해제를 쓴다. 근거: cpppj RawAssetPostPop.cpp 253행의 unpop 호출이다.
            Unpop = (sid, flags) => (_terrainUnpop ?? throw new InvalidOperationException("간헐천 이웃 해제 미연결")).Unpop(sid, types[Pool.Slot(sid)[RawSidPool.TypeOffset]], flags),
            // 이웃 재등록은 같은 월드 좌표의 실제 Pop을 다시 실행한다. 근거: cpppj RawSurfaceStorage.cpp 371행의 RepopAsset다.
            Repop = (sid, flags) =>
            {
                ReadOnlySpan<byte> raw = Pool.Slot(sid);
                TerrainPopObject(sid, BitConverter.ToSingle(raw.Slice(14, 4)), BitConverter.ToSingle(raw.Slice(18, 4)), flags);
            },
            // 거주 구역 지면 조회는 같은 월드의 실제 표면 찾기를 쓴다. 근거: cpppj RawAssetPostPop.cpp의 Surface 후크다.
            // 2026-11-19 0.635에서 R4-4 거주 구역 Pop용으로 추가했다.
            Surface = (x, y) => RawPathSurface.SurfaceAt(_terrainHash ?? throw new InvalidOperationException("거주 구역 지면 찾기 해시 미연결"), new RawPathPoint { X = x, Y = y }),
            // 주민 변화 뒤 실제 연결 재평가는 건물 옆면과 같은 통지를 쓴다. 근거: cpppj RawCampaignAssetBindings.cpp의 PostHooks 조립이다.
            // 2026-11-19 0.635에서 추가했다.
            NotifySurface = sid => (_fortBuilding ?? throw new InvalidOperationException("거주 구역 연결 통지 미연결")).NotifySurface(sid),
            // 썬 바리케이드 전용 후처리는 같은 세션의 펜스 본체로 위임한다. 근거: cpppj o/RawAssetPostPop.cpp의 SunFenceKind 분기와 client/RawSurfaceFence.cpp의 FencePostPop 조립이다.
            // 2026-11-19 0.636에서 R4-4 펜스 Pop용으로 추가했다. 펜스 본체는 뒤에 연결되므로 호출 시점에 조회한다.
            SunFencePostPop = (sid, flags) => (_fencePostPop ?? throw new InvalidOperationException("펜스 후처리 미연결(R6)")).PostPop(sid, flags),
            // cpppj PostHooks의 setOwner는 같은 소유자 지정 분배다. 도장 갈고리와 달리 가상 Owner를 거친다.
            SetOwner = SetFortObjectOwner,
        };
        var body = new RawAssetPostPop(Pool, types, terrainPostPop, carrier, assetState, hooks);
        body.BindPlayerRecords(PlayerRecords, () => unchecked((uint)MissionPlayer.BattleOwner));
        _vortexAssetState = assetState;
        _vortexAssetCarrier = carrier;
        _vortexAssetPostPop = body;
        terrainPostPop.SetAssetPostPop(body);
        // 간헐천 정기 분배를 공장 분배보다 먼저 둔다. 공장 사슬이 간헐천 사슬을 감싸므로 해제 순서는 공장이 먼저다.
        // 2026-11-19 0.634에서 R4-4 간헐천 정기용으로 추가했다.
        BuildGeyserRegularDevices(types);
        // 거주 구역 정기 분배를 간헐천 분배보다 나중에 둔다. 간헐천 사슬이 거주 구역 사슬을 감싸므로 해제 순서는 거주 구역이 먼저다.
        // 2026-11-19 0.635에서 R4-4 거주 구역 정기용으로 추가했다.
        BuildResidenceRegularDevices(types);
        BuildFactoryPostPopDevices(types);
        // 펜스 후처리·정기 본체를 공장 분배보다 나중에 둔다. 펜스 사슬이 공장 사슬을 감싸므로 해제 순서는 펜스가 먼저다.
        // 2026-11-19 0.636에서 R4-4 펜스 Pop·정기용으로 추가했다.
        BuildFenceDevices(types);
        // 장식 정기 분배를 펜스 분배보다 나중에 둔다. 장식 사슬이 펜스 사슬을 감싸므로 해제 순서는 장식이 먼저다.
        // 2026-11-19 0.642에서 R4-4 장식 정기용으로 추가했다.
        BuildDecorationRegularDevices(types);
        // Player 정기 분배를 장식 분배보다 나중에 둔다. Player 사슬이 장식 사슬을 감싸므로 해제 순서는 Player가 먼저다.
        // 2026-11-19 0.643에서 R4-4 Player 정기용으로 추가했다. 화신 장치는 BuildPlayerWorldHooks에서 먼저 연결된다.
        BuildPlayerRegularDevices(types);
        // AI 런타임 장치를 Player 정기 분배보다 나중에 둔다. 통지·등록 본체는 적재·틱에서 같은 세션 저장소를 공유한다.
        // 2026-11-19 0.650에서 R4-5 AI 런타임 결선용으로 추가했다.
        BuildAiRuntimeDevices(types);
        // 스프라이트 정기 분배를 AI 런타임 장치보다 나중에 둔다. 가상표가 겹치지 않으므로 순서는 결과에 닿지 않는다.
        // 2026-11-19 0.651에서 R4-4 스프라이트 정기용으로 추가했다.
        BuildSpriteRegularDevices(types);
    }

    /// <summary>공장 정기 트리거 본체를 세션에 연결하고 지형 분배 Pop의 공장 갈고리로 등록한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">공장 후처리의 지형 분배·운반체·프레임·프로세스·상태가 미연결이다.</exception>
    /// <exception cref="ArgumentException">공장 후처리의 TYPE·풀 불일치다.</exception>
    /// <remarks>2026-11-19 0.625에서 R4-2 공장 정기 트리거용으로 추가했다. 근거: cpppj RawCampaignAssetBindings.cpp 73·81~88행의 Factory 조립과 182행의 SetFactoryPostPop 등록이다. 정기 예약은 실제 프로세스에 부착하고 중복 경고만 기록한다.</remarks>
    private void BuildFactoryPostPopDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_factoryPostPop is not null)
        {
            throw new InvalidOperationException("공장 후처리 장치 중복 연결");
        }

        if (!ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("공장 후처리 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        RawPostPop terrainPostPop = _terrainPostPop ?? throw new InvalidOperationException("공장 후처리 지형 분배 미연결");
        RawCarrierPostPop carrier = _vortexAssetCarrier ?? throw new InvalidOperationException("공장 후처리 운반체 미연결");
        AssetPostPopState state = _vortexAssetState ?? throw new InvalidOperationException("공장 후처리 공유 상태 미연결");
        SquidFrame frames = _terrainFrames ?? throw new InvalidOperationException("공장 후처리 프레임 미연결");
        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("공장 후처리 정기 프로세스 미연결");
        RawRegionOwnership ownership = _regionOwnership ?? throw new InvalidOperationException("공장 아웃포스트 소유권 미연결");
        RawSquidHash hash = _terrainHash ?? throw new InvalidOperationException("공장 중복 검사 해시 미연결");
        // 모든 공장 갈고리를 실제 세션 장치에 연결하는 반복문이다. 도달하지 않은 가상80·상태 장치는 기본 getter를 쓴다.
        var hooks = new FactoryPostPopHooks
        {
            ReserveRegular = static () => true,
            AddRegular = (sid, ev, payload) =>
            {
                processes.AddRegular(sid, ev, payload);
            },
            WarnDuplicate = (sid, x, y) => _factoryWarnings.Add($"factory-warning sid={sid} x={x} y={y}"),
            Destroy = (_, _) => throw new InvalidOperationException("공장 swept 삭제 미연결(R6)"),
            RegionOwnership = (x, y) => ownership.Update(x, y),
            UpdateState = sid => _terrainDisplay!.Update(Pool.Slot(sid)),
        };
        hooks = RawFactoryPostPop.MakeFactoryPostPopHooks(Pool, frames, hooks);
        var body = new RawFactoryPostPop(Pool, hash, types, WorldFrames, terrainPostPop, carrier, state, hooks);
        _factoryPostPop = body;
        terrainPostPop.SetFactoryPostPop(body);
        _beforeFactoryRegular = processes.Regular();
        processes.SetRegularHandler(RegularHandlerWithFactory(_beforeFactoryRegular));
    }

    /// <summary>공장 가상표 부모를 공장 정기 본체에 분배하고 나머지는 이전 처리기로 넘긴다.</summary>
    /// <param name="fallback">공장이 맡지 않는 부모의 기존 처리기다. 없으면 payload를 돌려준다.</param>
    /// <returns>같은 풀의 현재 가상표로 분배하는 처리기다.</returns>
    /// <remarks>2026-11-19 0.629에서 R4-2 공장 정기 분배용으로 추가했다. 사제 월드는 이 사슬을 이전 처리기로 감싸므로 두 순서를 합성한다.</remarks>
    private RegularHandler RegularHandlerWithFactory(RegularHandler? fallback) => (sid, ev, count, payload) =>
    {
        if (_factoryPostPop is not null && _factoryPostPop.Handles(sid))
        {
            return RunFactoryRegularSession(sid, ev, count, payload, fallback);
        }

        return fallback?.Invoke(sid, ev, count, payload) ?? payload;
    };

    /// <summary>거주 구역 가상표(패치 0x50f540)인지 판정한다. CD판은 거주 구역 전용 분배를 열지 않는다.</summary>
    /// <param name="raw">조회할 슬롯 바이트다.</param>
    /// <returns>거주 구역 가상표이면 참이다.</returns>
    /// <remarks>2026-11-19 0.635에서 R4-4 거주 구역 Pop·정기용으로 추가했다. 근거: cpppj RawAssetPostPop.cpp 38행의 kResidence 대응표다.</remarks>
    private bool IsResidenceVtable(ReadOnlySpan<byte> raw)
    {
        if (Pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(raw) == 0x0050f540u;
    }

    /// <summary>썬 바리케이드 가상표(패치 0x509470)인지 판정한다. CD판은 펜스 전용 분배를 열지 않는다.</summary>
    /// <param name="raw">조회할 슬롯 바이트다.</param>
    /// <returns>썬 바리케이드 가상표이면 참이다.</returns>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스 Pop·정기용으로 추가했다. 근거: cpppj RawFencePostPop.h 51행의 FenceVtable이다.</remarks>
    private bool IsFenceVtable(ReadOnlySpan<byte> raw)
    {
        if (Pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(raw) == 0x00509470u;
    }

    /// <summary>장식 가상표 2종(패치 0x50a820·0x50a8e8)인지 판정한다. CD판은 장식 전용 분배를 열지 않는다.</summary>
    /// <param name="raw">조회할 슬롯 바이트다.</param>
    /// <returns>장식 가상표이면 참이다.</returns>
    /// <remarks>2026-11-19 0.642에서 R4-4 장식 정기용으로 추가했다. 근거: cpppj client/RawCampaignSession.cpp 1800~1806행의 장식 분기다.</remarks>
    private bool IsDecorationVtable(ReadOnlySpan<byte> raw)
    {
        if (Pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        return table == 0x0050a820u || table == 0x0050a8e8u;
    }

    /// <summary>간헐천 가상표인지 판정한다. 패치 0x50da88·CD 0x500760만 받는다.</summary>
    /// <param name="raw">조회할 슬롯 바이트다.</param>
    /// <returns>간헐천 가상표이면 참이다.</returns>
    /// <remarks>2026-11-19 0.634에서 R4-4 간헐천 Pop·정기용으로 추가했다. 근거: cpppj RawAssetPostPop.cpp 38행의 kGeyser 대응표다.</remarks>
    private bool IsGeyserVtable(ReadOnlySpan<byte> raw)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        return Pool.Edition == OriginalEdition.Patch1078 ? table == 0x0050da88u : table == 0x00500760u;
    }

    /// <summary>간헐천 정기 분배 전에 사용하던 공통 정기 분배기다. 공장 분배가 이 사슬을 감싸므로 해제 순서는 공장이 먼저다.</summary>
    /// <remarks>2026-11-19 0.634에서 R4-4 간헐천 정기용으로 추가했다. 간헐천 가상표가 아니면 이전 처리기로 넘긴다.</remarks>
    private RegularHandler? _beforeGeyserRegular;

    /// <summary>간헐천 가상표 부모를 간헐천 정기 본체에 분배하고 나머지는 이전 처리기로 넘긴다.</summary>
    /// <param name="fallback">간헐천이 맡지 않는 부모의 기존 처리기다. 없으면 payload를 돌려준다.</param>
    /// <returns>같은 풀의 현재 가상표로 분배하는 처리기다.</returns>
    /// <remarks>2026-11-19 0.634에서 R4-4 간헐천 정기용으로 추가했다. 근거: cpppj client/RawSurfaceStorage.cpp 297행의 분배다.</remarks>
    private RegularHandler RegularHandlerWithGeyser(RegularHandler? fallback) => (sid, ev, count, payload) =>
    {
        if (IsGeyserVtable(Pool.Slot(sid)))
        {
            return RunGeyserRegularSession(sid, ev, count, payload, fallback);
        }

        return fallback?.Invoke(sid, ev, count, payload) ?? payload;
    };

    /// <summary>간헐천 정기 본체를 세션에 연결하고 공장 분배보다 먼저 분배 사슬에 둔다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">간헐천 정기 분배의 지형 표시·정기 프로세스·프레임 표가 미연결이다.</exception>
    /// <remarks>2026-11-19 0.634에서 R4-4 간헐천 정기용으로 추가했다. 근거: cpppj client/RawSurfaceStorage.cpp 297행의 분배다. 간헐천 ev0은 난수를 소비하지 않는다.</remarks>
    private void BuildGeyserRegularDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_beforeGeyserRegular is not null)
        {
            throw new InvalidOperationException("간헐천 정기 분배 장치 중복 연결");
        }

        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("간헐천 정기 분배 프로세스 미연결");
        if (_terrainDisplay is null)
        {
            throw new InvalidOperationException("간헐천 정기 분배 표시 미연결");
        }

        if (_terrainPopTypes is null || !ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("간헐천 정기 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        _beforeGeyserRegular = processes.Regular();
        processes.SetRegularHandler(RegularHandlerWithGeyser(_beforeGeyserRegular));
    }

    /// <summary>같은 장기 풀·타입·표시의 현재 조회로 간헐천 정기 사건을 실행한다.</summary>
    /// <param name="sid">처리할 간헐천 부모 번호다.</param>
    /// <param name="event">사건 번호다.</param>
    /// <param name="count">개수다.</param>
    /// <param name="payload">전달 값이다.</param>
    /// <param name="fallback">선택된 조기 반환 이외의 이전 처리기다.</param>
    /// <returns>원문과 같은 반환 값이다.</returns>
    /// <remarks>2026-11-19 0.634에서 R4-4 간헐천 정기용으로 추가했다. 근거: cpppj o/RawAssetRegular.cpp 51~62행의 RunNativeGeyserRegular다. 표시 갱신은 실제 가상88이며 프레임 쓰기는 직접 DWORD 저장이다.</remarks>
    private float RunGeyserRegularSession(int sid, uint @event, uint count, float payload, RegularHandler? fallback)
    {
        var hooks = new GeyserRegularHooks
        {
            DisplayUpdate = (parent, flags) => (_terrainDisplay ?? throw new InvalidOperationException("간헐천 정기 분배 표시 미연결")).Update(Pool.Slot(parent), flags),
            Damageable = (parent, code, times, value) => fallback?.Invoke(parent, code, times, value) ?? value,
        };
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("간헐천 정기 TYPE 미연결");
        return RawGeyserRegular.Run(Pool, types, sid, @event, count, payload, hooks);
    }

    /// <summary>간헐천 AI 등록 요청 기록을 빌려준다. 연결 뒤에도 비어 있을 수 있다.</summary>
    /// <returns>등록 뒤 누적 기록이다. 실제 등록이 아니라 요청만 센다.</returns>
    /// <remarks>2026-11-19 0.634에서 추가했다. L1에서 호출 순서와 좌표를 대조할 때 쓴다. 실제 AI 등록은 R4-5에서 잇는다.</remarks>
    public IReadOnlyList<string> GeyserRegistrations => _geyserRegistrations;

    /// <summary>지역 투표가 읽는 테마 모드 조회다. BuildRegionOwnershipDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.635에서 R4-4 거주 구역 정기용으로 추가했다. 거주 구역 신전 일치 판정이 같은 값을 읽는다.</remarks>
    private Func<uint>? _themeModeReader;

    /// <summary>거주 구역 가상표 부모를 거주 구역 정기 본체에 분배하고 나머지는 이전 처리기로 넘긴다.</summary>
    /// <param name="fallback">거주 구역이 맡지 않는 부모의 기존 처리기다. 없으면 payload를 돌려준다.</param>
    /// <returns>같은 풀의 현재 가상표로 분배하는 처리기다.</returns>
    /// <remarks>2026-11-19 0.635에서 R4-4 거주 구역 정기용으로 추가했다. 간헐천 분배보다 나중에 두어 공장 사슬 안쪽에 둔다.</remarks>
    private RegularHandler RegularHandlerWithResidence(RegularHandler? fallback) => (sid, ev, count, payload) =>
    {
        if (IsResidenceVtable(Pool.Slot(sid)))
        {
            return RunResidenceRegularSession(sid, ev, count, payload, fallback);
        }

        return fallback?.Invoke(sid, ev, count, payload) ?? payload;
    };

    /// <summary>거주 구역 정기 본체를 세션에 연결하고 간헐천 분배보다 나중에 분배 사슬에 둔다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">거주 구역 정기 분배의 지형 표시·정기 프로세스·해시·프레임 표가 미연결이다.</exception>
    /// <remarks>2026-11-19 0.635에서 R4-4 거주 구역 정기용으로 추가했다. 간헐천 사슬을 감싸므로 해제 순서는 간헐천이 먼저다.</remarks>
    private void BuildResidenceRegularDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("거주 구역 정기 분배 프로세스 미연결");
        if (_terrainDisplay is null)
        {
            throw new InvalidOperationException("거주 구역 정기 분배 표시 미연결");
        }

        if (_terrainHash is null)
        {
            throw new InvalidOperationException("거주 구역 정기 분배 해시 미연결");
        }

        if (_terrainPopTypes is null || !ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("거주 구역 정기 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        if (_beforeResidenceRegular is not null)
        {
            throw new InvalidOperationException("거주 구역 정기 분배 장치 중복 연결");
        }

        _beforeResidenceRegular = processes.Regular();
        processes.SetRegularHandler(RegularHandlerWithResidence(_beforeResidenceRegular));
    }

    /// <summary>거주 구역 정기 분배 전에 사용하던 공통 정기 분배기다. 공장 분배가 이 사슬을 감싸므로 해제 순서는 공장이 먼저다.</summary>
    /// <remarks>2026-11-19 0.635에서 R4-4 거주 구역 정기용으로 추가했다. 거주 구역 가상표가 아니면 이전 처리기로 넘긴다.</remarks>
    private RegularHandler? _beforeResidenceRegular;

    /// <summary>같은 장기 풀·해시·타입·프레임·난수의 현재 조회로 거주 구역 정기 사건을 실행한다.</summary>
    /// <param name="sid">처리할 거주 구역 부모 번호다.</param>
    /// <param name="event">사건 번호다. 0·1·3만 처리하고 2는 R6 주민 생성에서 잇는다.</param>
    /// <param name="count">개수다. 그대로 돌려주지 않는다.</param>
    /// <param name="payload">전달 값이다. 사건별 다음 지연으로 바뀐다.</param>
    /// <param name="fallback">선택된 조기 반환 이외의 이전 처리기다. 거주 구역 본체가 쓰지 않는다.</param>
    /// <returns>원문과 같은 반환 값이다.</returns>
    /// <remarks>2026-11-19 0.635에서 R4-4 거주 구역 정기용으로 추가했다. 근거: cpppj o/RawResidenceRegular.cpp 16행의 RunResidenceRegular다. 주민 생성(ev2)은 R6에서 잇는다.</remarks>
    private float RunResidenceRegularSession(int sid, uint @event, uint count, float payload, RegularHandler? fallback)
    {
        _ = fallback;
        var hooks = new ResidenceRegularHooks
        {
            Random = () => Game,
            Notify = parent => (_fortBuilding ?? throw new InvalidOperationException("거주 구역 정기 연결 통지 미연결")).NotifySurface(parent),
            Frame = (parent, value, flags) => (_terrainFrames ?? throw new InvalidOperationException("거주 구역 정기 프레임 미연결")).Set(parent, value, flags),
            Surface = (x, y) => RawPathSurface.SurfaceAt(_terrainHash ?? throw new InvalidOperationException("거주 구역 정기 지면 찾기 미연결"), new RawPathPoint { X = x, Y = y }),
            SetOwner = SetFortObjectOwner,
            Dude = _ => throw new InvalidOperationException("거주 구역 주민 생성 미연결(R6)"),
            Element = temple => _terrainPopTypes![Pool.Slot(temple)[RawSidPool.TypeOffset]].Theme,
            ThemeMode = () => _themeModeReader?.Invoke() ?? 1u,
            DecorationType = () => _vortexAssetState?.DecorationType ?? 102u,
            RecordScan = scanned => { if (_vortexAssetState is not null && _vortexAssetState.MaxDecorationScan < scanned) _vortexAssetState.MaxDecorationScan = scanned; },
        };
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("거주 구역 정기 TYPE 미연결");
        RawSquidHash hash = _terrainHash ?? throw new InvalidOperationException("거주 구역 정기 해시 미연결");
        return RawResidenceRegular.Run(Pool, hash, types, WorldFrames, SidLists.Temples, sid, @event, count, payload, hooks);
    }

    /// <summary>같은 장기 풀·해시·타입에서 이웃을 찾는 펜스 이웃 본체다. BuildFenceDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스 Pop·정기용으로 추가했다. 근거: cpppj RawSurfaceFence.cpp의 이웃 결선이다.</remarks>
    private RawFenceNeighbors? _fenceNeighbors;

    /// <summary>같은 장기 풀의 펜스 후처리 본체다. BuildFenceDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스 Pop용으로 추가했다. 근거: cpppj o/RawFencePostPop.h·cpp의 RawFencePostPop이다.</remarks>
    private RawFencePostPop? _fencePostPop;

    /// <summary>같은 장기 풀·프레임의 펜스 정기 본체다. BuildFenceDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스 정기용으로 추가했다. 근거: cpppj o/RawFenceRegular.h·cpp의 RawFenceRegular다.</remarks>
    private RawFenceRegular? _fenceRegular;

    /// <summary>같은 장기 풀·이웃 중심의 펜스 표식 생성 본체다. BuildFenceDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.644에서 R4-4 펜스 재생성용으로 추가했다. 근거: cpppj o/RawFenceCreate.h·cpp의 RawFenceMarkerCreate다.</remarks>
    private RawFenceMarkerCreate? _fenceMarkerCreate;

    /// <summary>같은 장기 풀·프레임의 펜스 포스 필드 생성 본체다. BuildFenceDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.644에서 R4-4 펜스 재생성용으로 추가했다. 근거: cpppj o/RawFenceCreate.h·cpp의 RawFenceShieldCreate다.</remarks>
    private RawFenceShieldCreate? _fenceShieldCreate;

    /// <summary>표식 삭제가 읽는 fencemark 타입 번호 상자다. 같은 세션 수명을 공유한다.</summary>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스용으로 추가했다. 근거: cpppj DAT005411c0의 현재 fencemark 번호다.</remarks>
    private readonly UIntBox _fenceMarkerType = new();

    /// <summary>연결선 정리가 읽는 fenceShield 타입 번호 상자다. 같은 세션 수명을 공유한다.</summary>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스용으로 추가했다. 근거: cpppj DAT00541300의 현재 연결선 타입 번호다.</remarks>
    private readonly UIntBox _fenceLineType = new();

    /// <summary>타입 표에서 이름이 일치하는 첫 타입 번호를 찾는다.</summary>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="name">찾을 자산 이름이다.</param>
    /// <returns>일치하는 타입 번호다.</returns>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스 표식·연결선 타입용으로 추가했다. 이름 목록은 RiftTypeLoadOrder와 같은 순서다.</remarks>
    private static int FindTypeByName(IReadOnlyList<OriginalTypeRecord> types, string name)
    {
        // 모든 타입 번호를 순서대로 비교하는 반복문이다. 없으면 데이터 오류로 거부한다.
        for (int i = 0; i < types.Count; i++)
        {
            if (types[i].Name == name)
            {
                return i;
            }
        }

        throw new InvalidOperationException($"펜스 자산 이름 미등록: {name}");
    }

    /// <summary>같은 실제 전체 파괴로 가상+10 삭제를 잇는 효과를 만든다. 펜스 정리와 AI 전략 삭제가 공유한다.</summary>
    /// <param name="name">진단 문장에 쓸 장치 이름이다.</param>
    /// <param name="processes">같은 실제 삭제 몸체를 빌리는 프로세스 호스트다.</param>
    /// <returns>실제 파괴를 호출하는 삭제 효과다.</returns>
    /// <remarks>2026-11-19 0.650에서 펜스 결선에서 분리했다. 근거: cpppj o/RawFencePostPop.cpp·o/RawFenceCreate.cpp의 가상+10 삭제다. Pre/Post는 같은 몸체의 깊이로 닫고 종속은 같은 몸체로 재귀한다. 단일 미션의 송신은 기록만 하며 선택 해제는 화면이 없어 생략한다. 전송·통지 일치는 R4-6 전송 결선에서 대조한다.</remarks>
    private Action<int, uint> BuildSessionDestroy(string name, SquidProcessHost processes)
    {
        ArgumentException.ThrowIfNullOrEmpty(name);
        ArgumentNullException.ThrowIfNull(processes);
        RawSquidDestroy body = processes.Destroy();
        SquidDestroyHooks? hooks = null;
        hooks = new SquidDestroyHooks
        {
            Emit = e =>
            {
                switch (e.Effect)
                {
                    case SquidDestroyEffect.PreDestroy:
                        body.CompletePreDestroy();
                        break;
                    case SquidDestroyEffect.PostDestroy:
                        body.CompletePostDestroy();
                        break;
                    case SquidDestroyEffect.Transmit:
                        _terrainPopTransmissions.Add((e.Sid, e.Flags));
                        break;
                    case SquidDestroyEffect.ReleaseDependent:
                    case SquidDestroyEffect.DestroyDependent:
                        body.Destroy(e.Sid, e.Flags, hooks!);
                        break;
                    case SquidDestroyEffect.ClearSelection:
                        break;
                    default:
                        throw new InvalidOperationException($"{name} 삭제 효과 미연결: {e.Effect}");
                }
            },
            Selected = () => 0,
            UnpopForm = sid => processes.UnpopForm(sid),
        };
        return (sid, flags) => body.Destroy(sid, flags, hooks!);
    }

    /// <summary>펜스 후처리·정기 본체와 이웃·표식·연결선·재생성 장치를 세션에 연결한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">펜스 장치의 표시·정기 프로세스·해시·프레임 표가 미연결이다.</exception>
    /// <exception cref="ArgumentException">펜스 TYPE이 지형 Pop 장치와 다르다.</exception>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스 Pop·정기용으로 추가했다. 근거: cpppj client/RawSurfaceFence.cpp의 BindFenceLifetimeDevices·BindFenceRegularDevices 상당 결선이다. 표식 생성·삭제와 연결선 생성은 R6에서 잇는다.</remarks>
    private void BuildFenceDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_fencePostPop is not null || _fenceRegular is not null)
        {
            throw new InvalidOperationException("펜스 장치 중복 연결");
        }

        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("펜스 정기 분배 프로세스 미연결");
        RawSquidHash hash = _terrainHash ?? throw new InvalidOperationException("펜스 이웃 찾기 해시 미연결");
        SquidFrame frames = _terrainFrames ?? throw new InvalidOperationException("펜스 정기 프레임 미연결");
        if (_terrainPopTypes is null || !ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("펜스 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        if (_terrainDisplay is null)
        {
            throw new InvalidOperationException("펜스 정기 분배 표시 미연결");
        }

        _fenceMarkerType.Value = unchecked((uint)FindTypeByName(types, "fencemark"));
        _fenceLineType.Value = unchecked((uint)FindTypeByName(types, "fenceShield"));
        var neighbors = new RawFenceNeighbors(Pool, hash, types, type => types[(int)type].Range, (row, col) => Alliances[row * 9 + col]);
        _fenceNeighbors = neighbors;
        SquidProcessHost fenceProcesses = PlayerProcesses ?? throw new InvalidOperationException("펜스 삭제 프로세스 미연결");
        Action<int, uint> fenceDestroy = BuildSessionDestroy("펜스", fenceProcesses);
        var markerClear = new RawFenceMarkerClear(Pool, hash, types, _fenceMarkerType,
            fenceDestroy,
            parent => { ReadOnlySpan<byte> raw = Pool.Slot(parent); return new float[] { BitConverter.ToSingle(raw.Slice(14, 4)), BitConverter.ToSingle(raw.Slice(18, 4)) }; });
        var line = new RawFenceLine(Pool, hash, types, WorldFrames, _fenceLineType, fenceDestroy);
        // 표식·포스 필드 생성은 같은 풀·공장·표면·소유자·Pop으로 잇는다. 근거: cpppj client/RawSurfaceFence.cpp 26~50행의 생성 결선이다.
        // 0.644에서 R4-4 펜스 재생성 경로로 연결했다. 표면 통지는 요새 공간 장치가 있어야 하며 지형·요새 장치 뒤에 연결되므로 실제 경로를 쓴다.
        RawSquidFactory createFactory = _fortFactory ?? throw new InvalidOperationException("펜스 생성 공장 미연결");
        var createHooks = new FenceCreateHooks
        {
            Create = (type, flags) => createFactory.Create(type, (Netstorm.Core.Simulation.SidAllocation)flags),
            NotifySurface = sid => (_fortBuilding ?? throw new InvalidOperationException("펜스 생성 표면 통지 미연결")).NotifySurface(sid),
            SetOwner = SetFortObjectOwner,
            Pop = (sid, x, y, flags) => TerrainPopObject(sid, x, y, flags),
        };
        _fenceMarkerCreate = new RawFenceMarkerCreate(Pool, _fenceMarkerType, createHooks, sid => neighbors.Center(sid));
        _fenceShieldCreate = new RawFenceShieldCreate(Pool, WorldFrames, _fenceLineType, createHooks);
        var rebuild = new RawFenceRebuild(Pool, new FenceRebuildHooks
        {
            Neighbor = (parent, direction) => neighbors.Neighbor(parent, direction),
            WithinRange = (parent, endpoints) => neighbors.WithinRange(parent, endpoints),
            Markers = (parent, other) => FenceMarkerCreate.Create(parent, other),
            Shields = (points, first, second, firstId, secondId) => FenceShieldCreate.Create(points, first, second, firstId, secondId),
        });
        var postPop = new RawFencePostPop(Pool, new FencePostPopHooks
        {
            Theme = sid => unchecked((uint)types[Pool.Slot(sid)[RawSidPool.TypeOffset]].Theme),
            Neighbor = (sid, direction) => neighbors.Neighbor(sid, direction),
            ReserveRegular = static () => true,
            AddRegular = (sid, ev, payload) => { processes.AddRegular(sid, ev, payload); },
            ClearLine = points => line.Clear(points),
            Connections = sid => neighbors.Connections(sid),
            ValidateBase = (sid, flags, projected) => (_terrainPostPop ?? throw new InvalidOperationException("펜스 후처리 공통 검사 미연결")).ValidateBase(sid, flags, projected),
            Damageable = (sid, flags) => (_vortexAssetCarrier ?? throw new InvalidOperationException("펜스 후처리 피해 분배 미연결")).DamageablePostPop(sid, flags),
        });
        _fencePostPop = postPop;
        RawPostPop terrainPostPop = _terrainPostPop ?? throw new InvalidOperationException("펜스 후처리 지형 분배 미연결");
        terrainPostPop.SetFencePostPop(postPop);
        var regular = new RawFenceRegular(Pool, WorldFrames, frames, new FenceRegularHooks
        {
            Neighbor = (sid, direction) => neighbors.Neighbor(sid, direction),
            ClearMarkers = (parent, other) => markerClear.Clear(parent, other),
            ClearLine = points => line.Clear(points),
            Connections = sid => neighbors.Connections(sid),
            ReserveRegular = static () => true,
            AddRegular = (sid, ev, payload) => { processes.AddRegular(sid, ev, payload); },
            RefreshNeighbors = sid => throw new InvalidOperationException("펜스 이웃 새로고침 미연결(R6)"),
            Rebuild = sid => rebuild.Rebuild(sid),
            Damageable = (parent, code, times, value) => _beforeFenceRegular?.Invoke(parent, code, times, value) ?? value,
        });
        _fenceRegular = regular;
        _beforeFenceRegular = processes.Regular();
        processes.SetRegularHandler(RegularHandlerWithFence(_beforeFenceRegular));
    }

    /// <summary>펜스 정기 분배 전에 사용하던 공통 정기 분배기다. 펜스 사슬이 공장 사슬을 감싸므로 해제 순서는 펜스가 먼저다.</summary>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스 정기용으로 추가했다. 펜스 가상표가 아니면 이전 처리기로 넘긴다.</remarks>
    private RegularHandler? _beforeFenceRegular;

    /// <summary>썬 바리케이드 가상표 부모를 펜스 정기 본체에 분배하고 나머지는 이전 처리기로 넘긴다.</summary>
    /// <param name="fallback">펜스가 맡지 않는 부모의 기존 처리기다. 없으면 payload를 돌려준다.</param>
    /// <returns>같은 풀의 현재 가상표로 분배하는 처리기다.</returns>
    /// <remarks>2026-11-19 0.636에서 R4-4 펜스 정기용으로 추가했다. 근거: cpppj client/RawSurfaceFence.cpp의 정기 분배다.</remarks>
    private RegularHandler RegularHandlerWithFence(RegularHandler? fallback) => (sid, ev, count, payload) =>
    {
        if (_fenceRegular is not null && IsFenceVtable(Pool.Slot(sid)))
        {
            return _fenceRegular.Handle(sid, ev, count, payload);
        }

        return fallback?.Invoke(sid, ev, count, payload) ?? payload;
    };

    /// <summary>펜스 후처리 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>지형 분배 Pop의 펜스 갈고리로 등록한 본체다.</returns>
    /// <remarks>2026-11-19 0.636에서 추가했다. L1에서 적재 경로를 대조할 때 쓴다.</remarks>
    public RawFencePostPop FencePostPop => _fencePostPop ?? throw new InvalidOperationException("펜스 후처리 미연결");

    /// <summary>펜스 표식 생성 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>재생성 갈고리가 쓰는 실제 생성 본체다.</returns>
    /// <remarks>2026-11-19 0.644에서 추가했다. L1에서 재생성 경로를 대조할 때 쓴다.</remarks>
    public RawFenceMarkerCreate FenceMarkerCreate => _fenceMarkerCreate ?? throw new InvalidOperationException("펜스 표식 생성 미연결");

    /// <summary>펜스 포스 필드 생성 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>재생성 갈고리가 쓰는 실제 생성 본체다.</returns>
    /// <remarks>2026-11-19 0.644에서 추가했다. L1에서 재생성 경로를 대조할 때 쓴다.</remarks>
    public RawFenceShieldCreate FenceShieldCreate => _fenceShieldCreate ?? throw new InvalidOperationException("펜스 포스 필드 생성 미연결");

    /// <summary>장식 정기 분배 전에 사용하던 공통 정기 분배기다. 장식 사슬이 펜스 사슬을 감싸므로 해제 순서는 장식이 먼저다.</summary>
    /// <remarks>2026-11-19 0.642에서 R4-4 장식 정기용으로 추가했다. 장식 가상표가 아니면 이전 처리기로 넘긴다.</remarks>
    private RegularHandler? _beforeDecorationRegular;

    /// <summary>장식 가상표 부모를 장식 정기 본체에 분배하고 나머지는 이전 처리기로 넘긴다.</summary>
    /// <param name="fallback">장식이 맡지 않는 부모의 기존 처리기다. 없으면 payload를 돌려준다.</param>
    /// <returns>같은 풀의 현재 가상표로 분배하는 처리기다.</returns>
    /// <remarks>2026-11-19 0.642에서 R4-4 장식 정기용으로 추가했다. 근거: cpppj client/RawCampaignSession.cpp 1800~1806행의 장식 분기다.</remarks>
    private RegularHandler RegularHandlerWithDecoration(RegularHandler? fallback) => (sid, ev, count, payload) =>
    {
        if (IsDecorationVtable(Pool.Slot(sid)))
        {
            return RunDecorationRegularSession(sid, ev, count, payload, fallback);
        }

        return fallback?.Invoke(sid, ev, count, payload) ?? payload;
    };

    /// <summary>장식 정기 분배 장치를 세션에 연결한다. Pop 분배는 기존 지형 경로를 그대로 쓴다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">장식 정기 분배의 정기 프로세스·HP 장치가 미연결이다.</exception>
    /// <exception cref="ArgumentException">장식 정기 TYPE이 지형 Pop 장치와 다르다.</exception>
    /// <remarks>2026-11-19 0.642에서 R4-4 장식 정기용으로 추가했다. 근거: cpppj client/RawSurfaceDecoration.cpp 10~23행의 DecorationRegular 결선이다.</remarks>
    private void BuildDecorationRegularDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_beforeDecorationRegular is not null)
        {
            throw new InvalidOperationException("장식 정기 분배 장치 중복 연결");
        }

        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("장식 정기 분배 프로세스 미연결");
        if (_terrainPopTypes is null || !ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("장식 정기 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        _beforeDecorationRegular = processes.Regular();
        processes.SetRegularHandler(RegularHandlerWithDecoration(_beforeDecorationRegular));
    }

    /// <summary>같은 장기 풀·HP·해제의 현재 조회로 장식 정기 사건을 실행한다.</summary>
    /// <param name="sid">처리할 장식 부모 번호다.</param>
    /// <param name="event">사건 번호다.</param>
    /// <param name="count">개수다.</param>
    /// <param name="payload">전달 값이다.</param>
    /// <param name="fallback">선택된 조기 반환 이외의 이전 처리기다.</param>
    /// <returns>원문과 같은 반환 값이다.</returns>
    /// <remarks>2026-11-19 0.642에서 R4-4 장식 정기용으로 추가했다. 근거: cpppj client/RawCampaignSession.cpp 1802~1805행의 typed 경로 장식 갈고리다. 가상98 전체 몸체 대신 현재 HP 직접 차감으로 잇는다. 방어 보정·소유자 통지·연소 갱신은 R6 피해 결선에서 잇는다. 삭제는 world.Destroy 전체 효과 대신 지형 해제로 잇는다. 송신·통지는 R6에서 잇는다.</remarks>
    private float RunDecorationRegularSession(int sid, uint @event, uint count, float payload, RegularHandler? fallback)
    {
        RawSquidReward hp = _fortHp ?? throw new InvalidOperationException("장식 정기 HP 미연결");
        var hooks = new DecorationRegularHooks
        {
            MaxHitPoints = parent => hp.MaxHitPoints(parent),
            HitPoints = (parent, amount, _, _) =>
            {
                Span<byte> raw = Pool.AllocatedBytes(parent);
                int current = BinaryPrimitives.ReadInt32LittleEndian(raw.Slice(26, 4));
                BinaryPrimitives.WriteInt32LittleEndian(raw.Slice(26, 4), unchecked(current - amount));
            },
            Destroy = (parent, flags) =>
            {
                RawUnpop unpop = _terrainUnpop ?? throw new InvalidOperationException("장식 파괴 지형 해제 미연결");
                IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("장식 정기 TYPE 미연결");
                unpop.Unpop(parent, types[Pool.Slot(parent)[RawSidPool.TypeOffset]], flags);
            },
            Damageable = (parent, code, times, value) => fallback?.Invoke(parent, code, times, value) ?? value,
        };
        return RawDecorationRegular.Run(Pool, Gameplay.Authority.Boss, sid, @event, count, payload, hooks);
    }

    /// <summary>Player 정기 분배 전에 사용하던 공통 정기 분배기다. Player 사슬이 장식 사슬을 감싸므로 해제 순서는 Player가 먼저다.</summary>
    /// <remarks>2026-11-19 0.643에서 R4-4 Player 정기용으로 추가했다. Player 가상표가 아니면 이전 처리기로 넘긴다.</remarks>
    private RegularHandler? _beforePlayerRegular;

    /// <summary>Player 정기에서 기록한 그룹 29 로그·진단 문장이다. 화면 로그 UI는 R6에서 잇는다.</summary>
    /// <remarks>2026-11-19 0.643에서 추가했다. L1에서 사건 1의 소어 루저 문장과 미지원 사건 진단을 대조할 때 쓴다.</remarks>
    private readonly List<string> _playerRegularLogs = [];

    /// <summary>Player 정기 둘째 사건이 표시한 Template 칸 번호 기록이다.</summary>
    /// <remarks>2026-11-19 0.643에서 추가했다. L1에서 State 갱신 통지 순서를 대조할 때 쓴다.</remarks>
    private readonly List<uint> _playerTemplateMarks = [];

    /// <summary>Player 정기 로그·진단 기록을 빌려준다. 연결 뒤에도 비어 있을 수 있다.</summary>
    /// <returns>기록 뒤 누적 기록이다. 실제 화면 로그가 아니라 순서만 센다.</returns>
    /// <remarks>2026-11-19 0.643에서 추가했다. 실제 화면 로그 UI는 R6에서 잇는다.</remarks>
    public IReadOnlyList<string> PlayerRegularLogs => _playerRegularLogs;

    /// <summary>Player 정기 Template 표시 기록을 빌려준다. 연결 뒤에도 비어 있을 수 있다.</summary>
    /// <returns>표시 뒤 누적 칸 번호다. 실제 DataManager 통지가 아니라 순서만 센다.</returns>
    /// <remarks>2026-11-19 0.643에서 추가했다. 실제 DataManager 스트림·UI 통지는 R6에서 잇는다.</remarks>
    public IReadOnlyList<uint> PlayerTemplateMarks => _playerTemplateMarks;

    /// <summary>Player 가상표 부모를 Player 정기 본체에 분배하고 나머지는 이전 처리기로 넘긴다.</summary>
    /// <param name="fallback">Player가 맡지 않는 부모의 기존 처리기다. 없으면 payload를 돌려준다.</param>
    /// <returns>같은 화신의 현재 가상표로 분배하는 처리기다.</returns>
    /// <remarks>2026-11-19 0.643에서 R4-4 Player 정기용으로 추가했다. 근거: cpppj o/RawPlayerRegular.cpp 18행의 Handles 판정이다. 화신 장치가 없으면 이전 처리기로 넘긴다.</remarks>
    private RegularHandler RegularHandlerWithPlayer(RegularHandler? fallback) => (sid, ev, count, payload) =>
    {
        if (_livePlayerWorld is not null && _livePlayerWorld.Handles(sid))
        {
            return RunPlayerRegularSession(sid, ev, count, payload, fallback);
        }

        return fallback?.Invoke(sid, ev, count, payload) ?? payload;
    };

    /// <summary>Player 정기 분배 장치를 세션에 연결한다. Pop 분배는 기존 화신 경로를 그대로 쓴다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">Player 정기 분배의 화신·정기 프로세스 장치가 미연결이다.</exception>
    /// <exception cref="ArgumentException">Player 정기 TYPE이 지형 Pop 장치와 다르다.</exception>
    /// <remarks>2026-11-19 0.643에서 R4-4 Player 정기용으로 추가했다. 근거: cpppj client/RawCampaignSession.cpp 1799행의 Native 분기 0x50ee08이다.</remarks>
    private void BuildPlayerRegularDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_beforePlayerRegular is not null)
        {
            throw new InvalidOperationException("Player 정기 분배 장치 중복 연결");
        }

        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("Player 정기 분배 프로세스 미연결");
        if (_livePlayerWorld is null)
        {
            throw new InvalidOperationException("Player 정기 화신 미연결");
        }

        if (_terrainPopTypes is null || !ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("Player 정기 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        _beforePlayerRegular = processes.Regular();
        processes.SetRegularHandler(RegularHandlerWithPlayer(_beforePlayerRegular));
    }

    /// <summary>같은 장기 풀·전투 소유자·모드의 현재 조회로 Player 정기 사건을 실행한다.</summary>
    /// <param name="sid">처리할 Player 부모 번호다.</param>
    /// <param name="event">사건 번호다.</param>
    /// <param name="count">개수다.</param>
    /// <param name="payload">전달 값이다.</param>
    /// <param name="fallback">선택된 조기 반환 이외의 이전 처리기다.</param>
    /// <returns>원문과 같은 반환 값이다.</returns>
    /// <remarks>2026-11-19 0.643에서 R4-4 Player 정기용으로 추가했다. 근거: cpppj o/RawPlayerRegular.cpp 24~43행의 RunPlayerRegular다. 새 정기 예약 실패(new 40)는 관리 코드에서 일어나지 않으므로 예약은 항상 성공으로 두고 실제 부착 실패는 null 반환으로 잇는다. Template State는 세션 적재 header 180바이트에서 잇는다. DataManager 스트림·UI 통지는 R6에서 잇는다.</remarks>
    private float RunPlayerRegularSession(int sid, uint @event, uint count, float payload, RegularHandler? fallback)
    {
        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("Player 정기 프로세스 미연결");
        var hooks = new PlayerRegularHooks
        {
            Assertion = kind => _playerRegularLogs.Add("assert:" + kind),
            Log = (group, text) => _playerRegularLogs.Add(group + ":" + text),
            ReserveRegular = static () => true,
            AddRegular = (parent, code, value) => { processes.AddRegular(parent, code, value); },
            TemplateState = slot =>
            {
                // 9칸 header에서 해당 칸 20바이트를 빌린다. 범위를 벗어나면 원문 손상 입력 전에 거부한다.
                if (slot > 8)
                {
                    throw new ArgumentOutOfRangeException(nameof(slot), slot, "Player Template 칸 번호 범위 오류");
                }

                return WorldTemplates.AsMemory(unchecked((int)slot) * 20, 20);
            },
            TemplateChanged = slot => _playerTemplateMarks.Add(slot),
        };
        return RawPlayerRegular.Run(Pool, MissionPlayer.BattleOwner, MissionPlayer.TemplateOwner, OwnerMode.Battle, sid, @event, count, payload, hooks);
    }

    /// <summary>같은 장기 풀의 AI 통지 본체다. BuildAiRuntimeDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.650에서 R4-5 AI 런타임 결선용으로 추가했다. 근거: cpppj o/RawAiObjectNotice.h·cpp의 RawAiObjectNotice다.</remarks>
    private RawAiObjectNotice? _aiObjects;

    /// <summary>같은 장기 풀의 AI 월드 등록 본체다. BuildAiRuntimeDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.650에서 R4-5 AI 런타임 결선용으로 추가했다. 근거: cpppj o/RawAiWorldRegistration.h·cpp의 RawAiWorldRegistration이다.</remarks>
    private RawAiWorldRegistration? _aiWorld;

    /// <summary>같은 장기 풀의 AI 준비 본체다. BuildAiRuntimeDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.650에서 R4-5 AI 런타임 결선용으로 추가했다. 근거: cpppj o/RawAiPreparation.h·cpp의 RawAiPreparation이다.</remarks>
    private RawAiPreparation? _aiPreparation;

    /// <summary>같은 장기 풀의 AI 전략 설정 본체다. BuildAiRuntimeDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.650에서 R4-5 AI 런타임 결선용으로 추가했다. 근거: cpppj o/RawAiStrategySetup.h·cpp의 RawAiStrategySetup이다.</remarks>
    private RawAiStrategySetup? _aiStrategySetup;

    /// <summary>같은 장기 풀의 AI 전체 프레임 본체다. BuildAiRuntimeDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.650에서 R4-5 AI 런타임 결선용으로 추가했다. 근거: cpppj o/RawAiRuntime.h·cpp의 RawAiRuntime이다. 매 프레임 호출은 R6 틱 결선에서 잇는다.</remarks>
    private RawAiRuntime? _aiRuntime;

    /// <summary>AI 내용물 조회가 공유하는 현재 권한·내용물 TYPE다. BuildAiRuntimeDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.650에서 추가했다. 전략 설정과 통지의 finder가 같은 상태를 공유한다.</remarks>
    private ContainedFinderState? _aiFinderState;

    /// <summary>AI 준비·전략이 공유하는 섬 런타임 본체다. BuildAiRuntimeDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.653에서 추가했다. 지역 경계 조회가 같은 저장소를 공유한다.</remarks>
    private RawIslandRuntime? _aiIslands;

    /// <summary>AI 그래프 조회 본체다. BuildAiRuntimeDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-19 0.657에서 추가했다. 준비·전략의 그래프 번호 조회가 같은 풀·표면을 공유한다.</remarks>
    private RawAiGraph? _aiGraph;

    /// <summary>AI 그래프 조회 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>같은 풀·표면의 그래프 조회 본체다.</returns>
    /// <remarks>2026-11-19 0.657에서 추가했다. L1에서 그래프 번호를 대조할 때 쓴다.</remarks>
    public RawAiGraph AiGraph => _aiGraph ?? throw new InvalidOperationException("AI 그래프 조회 미연결");

    /// <summary>AI 통지·실행시간 로그 기록이다. 화면 로그 UI는 R6에서 잇는다.</summary>
    /// <remarks>2026-11-19 0.650에서 추가했다. L1에서 통지·느린 틱 순서를 대조할 때 쓴다.</remarks>
    private readonly List<string> _aiRuntimeLogs = [];

    /// <summary>AI 준비·전략·등록 진단 기록이다.</summary>
    /// <remarks>2026-11-19 0.650에서 추가했다. L1에서 진단 순서를 대조할 때 쓴다.</remarks>
    private readonly List<string> _aiRuntimeDiagnostics = [];

    /// <summary>AI 통지 로그 기록을 빌려준다. 연결 뒤에도 비어 있을 수 있다.</summary>
    /// <returns>기록 뒤 누적 기록이다. 실제 화면 로그가 아니라 순서만 센다.</returns>
    /// <remarks>2026-11-19 0.650에서 추가했다. 실제 화면 로그 UI는 R6에서 잇는다.</remarks>
    public IReadOnlyList<string> AiRuntimeLogs => _aiRuntimeLogs;

    /// <summary>AI 진단 기록을 빌려준다. 연결 뒤에도 비어 있을 수 있다.</summary>
    /// <returns>기록 뒤 누적 기록이다.</returns>
    /// <remarks>2026-11-19 0.650에서 추가했다.</remarks>
    public IReadOnlyList<string> AiRuntimeDiagnostics => _aiRuntimeDiagnostics;

    /// <summary>AI 통지 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <remarks>2026-11-19 0.650에서 추가했다.</remarks>
    public RawAiObjectNotice AiObjects => _aiObjects ?? throw new InvalidOperationException("AI 통지 미연결");

    /// <summary>AI 월드 등록 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <remarks>2026-11-19 0.650에서 추가했다.</remarks>
    public RawAiWorldRegistration AiWorld => _aiWorld ?? throw new InvalidOperationException("AI 월드 등록 미연결");

    /// <summary>AI 준비 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <remarks>2026-11-19 0.650에서 추가했다.</remarks>
    public RawAiPreparation AiPreparation => _aiPreparation ?? throw new InvalidOperationException("AI 준비 미연결");

    /// <summary>AI 전략 설정 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <remarks>2026-11-19 0.650에서 추가했다. 0.658에서 신전 생산 TYPE을 실제 GameAux 상자로 이었다. 근거: cpppj src/client/RawCampaignSession.cpp 822행의 templeProduct다.</remarks>
    public RawAiStrategySetup AiStrategySetup => _aiStrategySetup ?? throw new InvalidOperationException("AI 전략 설정 미연결");

    /// <summary>AI 전체 프레임 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <remarks>2026-11-19 0.650에서 추가했다. 매 프레임 호출은 R6 틱 결선에서 잇는다.</remarks>
    public RawAiRuntime AiRuntime => _aiRuntime ?? throw new InvalidOperationException("AI 전체 프레임 미연결");

    /// <summary>AI 런타임이 공유하는 섬 런타임 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>준비·전략이 함께 쓰는 실제 섬 런타임이다.</returns>
    /// <remarks>2026-11-19 0.653에서 추가했다. L1에서 지역 경계를 둘 때 쓴다.</remarks>
    public RawIslandRuntime AiIslands => _aiIslands ?? throw new InvalidOperationException("AI 섬 런타임 미연결");

    /// <summary>같은 실제 중심·발자국으로 현재 SID 발 중심을 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호다.</param>
    /// <returns>발 중심 좌표다.</returns>
    /// <remarks>2026-11-19 0.650에서 추가했다. 근거: cpppj RawFenceNeighbors.cpp의 Center 계산이다. 이웃 본체 없이 같은 풀·타입에서 읽는다.</remarks>
    private float[] AiFootCenter(int sid)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("AI 발 중심 TYPE 미연결");
        OriginalTypeRecord type = types[raw[RawSidPool.TypeOffset]];
        return [
            (float)(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4))) - type.FootX * 0.5),
            (float)(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4))) - type.FootY * 0.5)];
    }

    /// <summary>같은 섬 런타임의 현재 지역 경계를 float 사각형으로 읽는다.</summary>
    /// <param name="islands">같은 세션 저장소를 공유하는 섬 런타임이다.</param>
    /// <param name="region">조회할 지역 번호다.</param>
    /// <returns>좌·상·우·하 순서의 현재 경계다.</returns>
    /// <remarks>2026-11-19 0.653에서 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 2866·2872행의 regionBounds 조립이다. 청크 int 경계의 16배 float 변환과 같다.</remarks>
    private static float[] AiRegionBounds(RawIslandRuntime islands, int region)
    {
        ArgumentNullException.ThrowIfNull(islands);
        FortRandomRectangle bounds = islands.Bounds(region);
        return [bounds.Left, bounds.Top, bounds.Right, bounds.Bottom];
    }

    /// <summary>AI 준비·전략·통지·등록·전체 프레임 본체와 세션 결선을 연결한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">AI 런타임의 화신·공장·프로세스·해시 장치가 미연결이다.</exception>
    /// <exception cref="ArgumentException">AI 런타임 TYPE이 지형 Pop 장치와 다르다.</exception>
    /// <remarks>2026-11-19 0.650에서 R4-5 AI 런타임 결선용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 2831~2870행의 통지·등록 조립과 2937~2943행의 전체 프레임 조립이다. 수집자·시설·부족스캔·사제·배치·다리·운반·경제·신전생산은 R6에서 잇는다. 분산 SP·자금·지도상한은 0.654에서 실제 저장소·독립 기본값으로 이었다. 지역 경계는 0.653에서 같은 섬 런타임으로, 그래프 조회는 0.657에서 같은 풀·표면으로 이었다. 매 프레임 호출도 R6 틱 결선에서 잇는다.</remarks>
    private void BuildAiRuntimeDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_aiPreparation is not null || _aiStrategySetup is not null || _aiObjects is not null || _aiWorld is not null || _aiRuntime is not null || _aiGraph is not null)
        {
            throw new InvalidOperationException("AI 런타임 장치 중복 연결");
        }

        if (_terrainPopTypes is null || !ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("AI 런타임 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        RawPlayerWorld player = _livePlayerWorld ?? throw new InvalidOperationException("AI 런타임 화신 미연결");
        RawSquidHash hash = _terrainHash ?? throw new InvalidOperationException("AI 런타임 해시 미연결");
        RawSquidReward hp = _fortHp ?? throw new InvalidOperationException("AI 런타임 HP 미연결");
        _aiFinderState = new ContainedFinderState();
        var finder = new RawCarrierPreDestroy(Pool, _aiFinderState, new CarrierPreDestroyHooks
        {
            ValidateDamageable = (_, _) => throw new InvalidOperationException("AI 전략 사전 파괴 검증 미연결(R6)"),
            DamageablePre = (_, _) => throw new InvalidOperationException("AI 전략 사전 파괴 후처리 미연결(R6)"),
            DependentFound = () => throw new InvalidOperationException("AI 전략 종속 통지 미연결(R6)"),
        });
        RawContainedCreate create = _fortContained ?? throw new InvalidOperationException("AI 런타임 내용물 생성 미연결");

        // 섬 조회가 같은 세션 기록·모드·목록·해시를 읽는 부모다. 거주 구역 owner는 실제 지정으로 잇는다.
        RawIslandRuntime islands = BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (sid, owner) => SetFortObjectOwner(sid, owner),
            Assertion = text => _aiRuntimeDiagnostics.Add("island:" + text),
        });
        _aiIslands = islands;
        var graph = new RawAiGraph(Pool, types, hash.BorrowEntries(0), new AiGraphHooks
        {
            GraphReady = () => _worldGraph is not null,
            FenceType = () => _fenceMarkerType.Value,
            WindArcherType = () => unchecked((uint)FindTypeByName(types, "windArcher")),
            GraphAssertion = (a, b, c) => _aiRuntimeDiagnostics.Add("graph:" + a + ":" + b + ":" + c),
            Contained = _aiFinderState,
        });
        _aiGraph = graph;
        var preparation = new RawAiPreparation(AiBrainMemory, Pool, PlayerRecords, Alliances,
            new AiPreparationGlobals
            {
                LocalOwner = () => MissionPlayer.BattleOwner,
                PlayerKnown = () => MissionPlayer.PlayerKnown,
                NoRegion = () => TerrainGlobals.NoRegion,
                InvalidGraph = () => TerrainBookkeeping.InvalidGraph,
                Now = () => ProcessTime.Value,
            },
            new AiPreparationHooks
            {
                MissingEnemyAssertion = () => _aiRuntimeDiagnostics.Add("assert:enemyPlayerId != INVALID_PLAYER_ID"),
                Priest = owner => new RawPriestLookup(Pool, PriestState.Priests).Find(owner),
                Temple = region => islands.Temple(region),
                MaxHitPoints = sid => hp.MaxHitPoints(sid),
                PointDistance = (source, target) =>
                {
                    double dx = source[0] - target[0], dy = source[1] - target[1];
                    return Math.Sqrt(dx * dx + dy * dy);
                },
                Graph = sid => (_aiGraph ?? throw new InvalidOperationException("AI 그래프 조회 미연결")).GraphFor(sid),
                RegionBounds = region => AiRegionBounds(islands, region),
                StoredSp = owner => BitConverter.UInt32BitsToSingle(Sp.Get(owner)),
                AddLocalSp = delta => (_fortHp ?? throw new InvalidOperationException("AI 준비 로컬 SP 보상 미연결")).AddSp(MissionPlayer.BattleOwner, delta),
                Demo = () => 0u,
                KnowledgeOpen = () => false,
                BlockingGumpOpen = () => false,
                InitialScan = token => (_aiWorld ?? throw new InvalidOperationException("AI 초기 전체 등록 미연결")).FullScan(token),
            });
        var objects = new RawAiObjectNotice(AiBrainMemory, Pool, types, PlayerRecords,
            () => ProcessTime.Value,
            () => unchecked((uint)FindTypeByName(types, "outpost")),
            () => OwnerMode.Challenge,
            new AiObjectNoticeHooks
            {
                CarrierTargets = _ => throw new InvalidOperationException("AI 통지 이동 목표 조회 미연결(R6)"),
                DistanceSquared = (first, second) =>
                {
                    float[] source = AiFootCenter(first), target = AiFootCenter(unchecked((int)second));
                    double dx = source[0] - target[0], dy = source[1] - target[1];
                    return dx * dx + dy * dy;
                },
                Log = (brain, entry) => _aiRuntimeLogs.Add(brain + ":" + entry.Kind + ":" + entry.Object),
            });
        var world = new RawAiWorldRegistration(AiBrainMemory, Pool, types, PlayerRecords, Alliances, hash.BorrowEntries(0), objects,
            new AiWorldRegistrationHooks
            {
                Center = sid => AiFootCenter(unchecked((int)sid)),
                FartherGeyser = (brain, sid) => _aiRuntimeLogs.Add(brain + ":farther:" + sid),
                MissingSurface = sid => _aiRuntimeDiagnostics.Add("surface:" + sid),
            });
        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("AI 런타임 프로세스 미연결");
        Action<int, uint> destroy = BuildSessionDestroy("AI 전략", processes);
        var strategy = new RawAiStrategySetup(AiBrainMemory, player, preparation, finder, create, _aiFinderState, types, Game, Alliances,
            new AiStrategySetupGlobals
            {
                LocalOwner = () => MissionPlayer.BattleOwner,
                NoRegion = () => TerrainGlobals.NoRegion,
                TempleProduct = () => GameAux.TempleProductTypeBox.Value,
                BridgeType = () => BridgeState.BridgeType,
                // 지도 상한은 독립 입력 기본값 256을 쓴다. 공유 Client 주소 연결은 R6에서 잇는다.
                // 2026-11-19 0.654에서 추가했다. 근거: cpppj RawCarrierDropPoint.h 10행의 독립 기본값이다.
                MapLimit = () => 256,
                Now = () => ProcessTime.Value,
            },
            new AiStrategySetupHooks
            {
                RegionBounds = region => AiRegionBounds(islands, region),
                Temple = region => islands.Temple(region),
                Destroy = (sid, flags) => destroy(sid, flags),
                Assertion = text => _aiRuntimeDiagnostics.Add("strategy:" + text),
                Demo = () => false,
            });
        var runtime = new RawAiRuntime(AiBrainMemory, PlayerRecords,
            new AiRuntimeGlobals
            {
                Battle = () => OwnerMode.Battle,
                Timing = () => 0u,
                DryLog = () => 0u,
            },
            new AiRuntimeHooks
            {
                Initialize = brain => strategy.Initialize(brain),
                Prepare = brain => preparation.Update(brain),
                SelectGoal = brain => strategy.SelectGoal(brain),
                Collectors = _ => throw new InvalidOperationException("AI 수집자 행동 미연결(R6)"),
                Factories = _ => throw new InvalidOperationException("AI 시설 행동 미연결(R6)"),
                DryScan = _ => throw new InvalidOperationException("AI 부족좌표 스캔 미연결(R6)"),
                DryNotice = (_, _, _) => throw new InvalidOperationException("AI 부족좌표 통지 미연결(R6)"),
                Priest = (_, _) => throw new InvalidOperationException("AI 사제 행동 미연결(R6)"),
                Drops = _ => throw new InvalidOperationException("AI 배치 행동 미연결(R6)"),
                Bridges = _ => throw new InvalidOperationException("AI 다리 행동 미연결(R6)"),
                Carriers = _ => throw new InvalidOperationException("AI 운반 행동 미연결(R6)"),
                Economy = _ => throw new InvalidOperationException("AI 경제 행동 미연결(R6)"),
                Money = owner => BitConverter.UInt32BitsToSingle(Sp.Get(owner)),
                Wall = () => ProcessTime.Value,
                Took = (brain, elapsed) => _aiRuntimeLogs.Add(brain + ":took:" + elapsed),
                Assertion = (text, line) => _aiRuntimeDiagnostics.Add("runtime:" + line + ":" + text),
                Destruct = brain => AiConstructorFor(brain).Destruct(brain),
                Free = brain => AiConstructorFor(brain).FreeBody(brain),
            });
        _aiPreparation = preparation;
        _aiObjects = objects;
        _aiWorld = world;
        _aiStrategySetup = strategy;
        _aiRuntime = runtime;
        RawPostPop terrainPostPop = _terrainPostPop ?? throw new InvalidOperationException("AI 통지 지형 분배 미연결");
        terrainPostPop.BindPatchAiNotice((owner, sid, added) => objects.Notice(owner, sid, added));
    }

    /// <summary>같은 brain에 00416eb0 준비 전체를 명시 호출한다. 없으면 거짓이다.</summary>
    /// <param name="owner">준비할 AI 소유자다.</param>
    /// <returns>brain이 있어 준비를 실행하면 참이다.</returns>
    /// <remarks>2026-11-19 0.650에서 R4-5 AI 초기화용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 461행의 PrepareAi다. 매 프레임 호출은 R6 틱 결선에서 잇는다.</remarks>
    public bool PrepareAi(uint owner)
    {
        if (!AiBrains.TryGetValue(owner, out uint brain))
        {
            return false;
        }

        (_aiPreparation ?? throw new InvalidOperationException("AI 준비 미연결")).Update(brain);
        return true;
    }

    /// <summary>같은 brain의 00414af0 초기 전략을 명시 재시도한다. 258 완료 여부를 반환한다.</summary>
    /// <param name="owner">초기화할 AI 소유자다.</param>
    /// <returns>258이 서 있으면 참이다.</returns>
    /// <remarks>2026-11-19 0.650에서 R4-5 AI 초기화용으로 추가했다. 근거: cpppj src/client/RawCampaignSession.cpp 462행의 InitializeAi다. 매 프레임 호출은 R6 틱 결선에서 잇는다.</remarks>
    public bool InitializeAi(uint owner)
    {
        if (!AiBrains.TryGetValue(owner, out uint brain))
        {
            return false;
        }

        (_aiStrategySetup ?? throw new InvalidOperationException("AI 전략 설정 미연결")).Initialize(brain);
        return BinaryPrimitives.ReadUInt32LittleEndian(AiBrainMemory.Bytes(brain).AsSpan(0x258, 4)) != 0;
    }

    /// <summary>스프라이트 가상표 부모를 스프라이트 정기 본체에 분배하고 나머지는 이전 처리기로 넘긴다.</summary>
    /// <param name="fallback">스프라이트가 맡지 않는 부모의 기존 처리기다. 없으면 payload를 돌려준다.</param>
    /// <returns>같은 풀의 현재 가상표로 분배하는 처리기다.</returns>
    /// <remarks>2026-11-19 0.651에서 R4-4 스프라이트 정기용으로 추가했다. 근거: cpppj client/RawCampaignSession.cpp 1772행의 스프라이트 분기다.</remarks>
    private RegularHandler RegularHandlerWithSprite(RegularHandler? fallback) => (sid, ev, count, payload) =>
    {
        if (RawSpriteRegular.Handles(Pool, sid))
        {
            SquidFrame frames = _terrainFrames ?? throw new InvalidOperationException("스프라이트 정기 프레임 미연결");
            return RawSpriteRegular.Run(Pool, WorldFrames, frames, sid, payload);
        }

        return fallback?.Invoke(sid, ev, count, payload) ?? payload;
    };

    /// <summary>스프라이트 정기 분배 장치를 세션에 연결한다. Pop 분배는 기존 지형 경로를 그대로 쓴다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 지형 Pop 장치와 같은 인스턴스여야 한다.</param>
    /// <exception cref="InvalidOperationException">스프라이트 정기 분배의 프레임 장치가 미연결이다.</exception>
    /// <exception cref="ArgumentException">스프라이트 정기 TYPE이 지형 Pop 장치와 다르다.</exception>
    /// <remarks>2026-11-19 0.651에서 R4-4 스프라이트 정기용으로 추가했다. 근거: cpppj client/RawCampaignSession.cpp 1772행이다. 사건·개수와 무관하며 난수를 소비하지 않는다.</remarks>
    private void BuildSpriteRegularDevices(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (_beforeSpriteRegular is not null)
        {
            throw new InvalidOperationException("스프라이트 정기 분배 장치 중복 연결");
        }

        SquidProcessHost processes = PlayerProcesses ?? throw new InvalidOperationException("스프라이트 정기 분배 프로세스 미연결");
        if (_terrainFrames is null)
        {
            throw new InvalidOperationException("스프라이트 정기 프레임 미연결");
        }

        if (_terrainPopTypes is null || !ReferenceEquals(types, _terrainPopTypes))
        {
            throw new ArgumentException("스프라이트 정기 TYPE은 지형 Pop 장치와 같은 인스턴스여야 한다", nameof(types));
        }

        _beforeSpriteRegular = processes.Regular();
        processes.SetRegularHandler(RegularHandlerWithSprite(_beforeSpriteRegular));
    }

    /// <summary>스프라이트 정기 분배 전에 사용하던 공통 정기 분배기다. 해제 순서는 스프라이트가 먼저다.</summary>
    /// <remarks>2026-11-19 0.651에서 R4-4 스프라이트 정기용으로 추가했다. 스프라이트 가상표가 아니면 이전 처리기로 넘긴다.</remarks>
    private RegularHandler? _beforeSpriteRegular;

    /// <summary>같은 장기 풀·프레임·난수의 현재 조회로 공장 정기 사건을 실행한다.</summary>
    /// <param name="sid">처리할 공장 부모 번호다.</param>
    /// <param name="event">사건 번호다.</param>
    /// <param name="count">개수다.</param>
    /// <param name="payload">전달 값이다.</param>
    /// <param name="fallback">선택된 조기 반환 이외의 이전 처리기다.</param>
    /// <returns>원문과 같은 반환 값이다.</returns>
    /// <remarks>2026-11-19 0.629에서 R4-2 공장 정기 분배용으로 추가했다. 근거: cpppj client/RawSurfaceFactoryRegular.cpp 27~36행의 FactoryRegular 조립이다. 레벨은 현재 가상80 상위 3비트, 큰 착공·완성·플레어 효과는 R6에서 잇는다.</remarks>
    private float RunFactoryRegularSession(int sid, uint @event, uint count, float payload, RegularHandler? fallback)
    {
        var hooks = new FactoryRegularHooks
        {
            Level = parent => Pool.Slot(parent)[RawSidPool.StateOffset] >> 5,
            FirstFrame = (parent, level) => FactoryCurrentRun(parent, level).First,
            FrameCount = (parent, level) => FactoryCurrentRun(parent, level).Count,
            Random = limit =>
            {
                if (limit == 0)
                {
                    Game.Next(1u);
                    throw new InvalidOperationException("공장 실제 같은 게임 난수 0범위");
                }

                return Game.Next(limit);
            },
            Frame = (parent, value, flags) => (_terrainFrames ?? throw new InvalidOperationException("세션 연결 프레임 미연결")).Set(parent, value, flags),
            Advance = (parent, delta, flags) => new RawFrameAdvance(Pool, WorldFrames, _terrainFrames ?? throw new InvalidOperationException("세션 연결 프레임 미연결")).Advance(parent, delta, flags),
            Begin = (_, _) => throw new InvalidOperationException("호스트 공장 착공 실제 몸체 미연결(R6)"),
            Finish = _ => throw new InvalidOperationException("호스트 공장 완성 실제 몸체 미연결(R6)"),
            Flare = (_, _, _) => throw new InvalidOperationException("호스트 공장 플레어 실제 몸체 미연결(R6)"),
            Damageable = (parent, code, times, value) => fallback?.Invoke(parent, code, times, value) ?? value,
        };
        return RawFactoryRegular.Run(Pool, sid, @event, count, payload, hooks);
    }

    /// <summary>레벨 조회 뒤에만 현재 TYPE·글자 구간을 읽고 물리 범위를 확인한다.</summary>
    /// <param name="sid">조회할 공장 부모 번호다.</param>
    /// <param name="level">현재 가상80 레벨이다.</param>
    /// <returns>현재 타입의 글자 구간이다.</returns>
    /// <remarks>2026-11-19 0.629에서 R4-2 공장 정기 분배용으로 추가했다. 근거: cpppj client/RawSurfaceFactoryRegular.cpp 20~22행의 CurrentFactoryRun이다.</remarks>
    private RiftTypeFrames.LetterRun FactoryCurrentRun(int sid, int level)
    {
        if (level < 0 || level >= 16)
        {
            throw new ArgumentOutOfRangeException(nameof(level), "공장 현재 상태 프레임 물리 범위 오류");
        }

        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        if (raw[RawSidPool.TypeOffset] >= WorldFrames.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "공장 현재 프레임 타입 물리 범위 오류");
        }

        return WorldFrames[raw[RawSidPool.TypeOffset]].Run(unchecked((byte)('A' + level)));
    }

    /// <summary>신전 슬롯의 타입 DWORD를 읽는다. 프레임 RUN은 실제 표에서, 충격음 토큰은 0으로 둔다.</summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <param name="offset">읽을 타입 DWORD 오프셋이다. 0x74·0x130·0x134만 쓴다.</param>
    /// <returns>해당 오프셋의 DWORD다.</returns>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. 근거: cpppj RawCampaignAssetBindings.cpp 162행의 typeDword 조립이다. 충격음 실제 재생은 R6 음향에서 잇는다.</remarks>
    private uint TempleTypeDword(int sid, uint offset)
    {
        uint number = Pool.Slot(sid)[RawSidPool.TypeOffset];
        if (number >= (uint)WorldFrames.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "신전 타입 프레임 표 범위 오류");
        }

        if (offset == 0x130)
        {
            return unchecked((uint)WorldFrames[(int)number].Run((byte)'A').First);
        }

        if (offset == 0x134)
        {
            return unchecked((uint)WorldFrames[(int)number].Run((byte)'B').First);
        }

        return 0u;
    }

    /// <summary>같은 월드의 실제 신전 가상 Owner 몸체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>요새 적재의 소유자 지정이 신전을 만나면 쓰는 가상 몸체다.</returns>
    /// <remarks>2026-11-19 0.621에서 추가했다. 도장의 SetOwner 갈고리는 이 몸체를 거치지 않는다.</remarks>
    public RawVortexOwner RegionVortex => _regionVortex ?? throw new InvalidOperationException("지역 가상 Owner 미연결");

    /// <summary>사원 전용 후처리 자산 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>지형 분배 Pop의 자산 갈고리로 등록한 본체다.</returns>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. 신전 종류만 분배가 열리며 다른 전용 파생은 R6에서 잇는다.</remarks>
    public RawAssetPostPop RegionVortexPostPop => _vortexAssetPostPop ?? throw new InvalidOperationException("사원 자산 후처리 미연결");

    /// <summary>공장 정기 트리거 본체를 빌려준다. 연결하기 전에는 예외다.</summary>
    /// <returns>지형 분배 Pop의 공장 갈고리로 등록한 본체다.</returns>
    /// <remarks>2026-11-19 0.625에서 R4-2 공장 정기 트리거용으로 추가했다. 일반 Factory와 아웃포스트 wrapper를 모두 처리한다.</remarks>
    public RawFactoryPostPop FactoryPostPop => _factoryPostPop ?? throw new InvalidOperationException("공장 후처리 미연결");

    /// <summary>공장 중복 경고 기록을 빌려준다. 연결 뒤에도 비어 있을 수 있다.</summary>
    /// <returns>중복 워크샾 삭제 직전의 원본 경고 기록이다.</returns>
    /// <remarks>2026-11-19 0.625에서 R4-2 공장 정기 트리거용으로 추가했다. L1에서 호출 순서와 좌표를 대조할 때 쓴다.</remarks>
    public IReadOnlyList<string> FactoryWarnings => _factoryWarnings;

    /// <summary>사원 전용 후처리가 요청한 충격음 횟수다.</summary>
    /// <returns>연결 뒤 누적 횟수다. 실제 재생이 아니라 요청만 센다.</returns>
    /// <remarks>2026-11-19 0.622에서 R4-2 사원 Pop 파생용으로 추가했다. Generate 범위의 실제 재생은 R6 음향에서 잇는다.</remarks>
    public uint RegionVortexSoundRequests => _vortexAssetSoundRequests;

    /// <summary>현재 좌표의 지면 지역 번호를 조회한다. 가상 Owner의 섬 번호와 같은 getter다.</summary>
    /// <param name="x">조회할 가로 좌표다.</param>
    /// <param name="y">조회할 세로 좌표다.</param>
    /// <returns>지면 지역 번호다.</returns>
    /// <remarks>2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다. 근거: cpppj RawCampaignOwnership.cpp 64행의 IslandNumber다. 원본 float 절삭을 사용하며 배치 순회에는 쓰지 않는다.</remarks>
    public int RegionIslandAt(float x, float y)
    {
        RawCanonPlacementTerrain terrain = _regionTerrain ?? throw new InvalidOperationException("지역 getter 지형 미연결");
        return unchecked((int)terrain.RegionAt((int)x, (int)y));
    }

    /// <summary>신전 가상 Owner가 패치 생산 갱신 뒤에 증가시킨 revision이다.</summary>
    /// <returns>현재 DWORD다. CD판에서는 바뀌지 않는다.</returns>
    /// <remarks>2026-11-19 0.621에서 추가했다. 덤프에는 직접 나가지 않는다.</remarks>
    public uint RegionVortexRevision => _regionVortexRevision.Value;

    /// <summary>신전 가상 Owner가 요청한 생산 갱신 횟수다.</summary>
    /// <returns>연결 뒤 누적 횟수다.</returns>
    /// <remarks>2026-11-19 0.621에서 추가했다. Generate 범위의 UI 갱신은 R6에서 잇는다.</remarks>
    public uint RegionVortexRefreshes => _regionVortexRefreshes;

    /// <summary>신전 가상 Owner가 존재하지 않는 섬을 만나 보고한 진단이다.</summary>
    /// <returns>보고 뒤에도 계속 진행한 기록이다.</returns>
    /// <remarks>2026-11-19 0.621에서 추가했다. 근거: cpppj RawVortexOwnerDiagnostic.cpp의 보고 뒤 계속이다.</remarks>
    public IReadOnlyList<string> RegionVortexDiagnostics => _regionVortexDiagnostics;

    /// <summary>요새 객체 적재의 소유자 지정을 실제 가상 Owner 또는 기존 공통 부모에 전달한다. 도장·사제 경로는 쓰지 않는다.</summary>
    /// <param name="sid">소유자를 정할 객체 번호다.</param>
    /// <param name="player">새 소유자 번호다.</param>
    /// <remarks>2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다. 근거: cpppj RawCampaignOwnership.cpp 78~80행의 SetOwner 분배다. 도장의 SetOwner 갈고리는 소유자 투표의 재귀를 피하려고 기존 공통 경로를 그대로 쓴다.</remarks>
    public void SetFortObjectOwner(int sid, uint player)
    {
        if (_regionVortex is not null && RawVortexOwner.HandlesRawVortexOwner(Pool, sid))
        {
            _regionVortex.Set(sid, player);
            return;
        }

        SetWorldObjectOwner(sid, player);
    }

    /// <summary>Pop 시점 실제 좌표의 신전 소유자를 가상 Owner로 분배한다. 적재 갈고리에서 실제 좌표와 함께 부른다.</summary>
    /// <param name="sid">분배할 신전 슬롯 번호다.</param>
    /// <exception cref="InvalidOperationException">가상 Owner 미연결·미복원 신전 가상 표다.</exception>
    /// <remarks>2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다. 근거: cpppj RawAssetPostPop.cpp 287행의 신전 목록 등록 뒤 91행의 SetOwner 분배다. 적재 SetOwner(좌표 0)와 달리 실제 좌표로 섬 소유자·첫 지역을 전파한다.</remarks>
    public void DispatchRegionTempleOwner(int sid)
    {
        RawVortexOwner vortex = _regionVortex ?? throw new InvalidOperationException("지역 가상 Owner 미연결");
        if (!RawVortexOwner.HandlesRawVortexOwner(Pool, sid))
        {
            throw new InvalidOperationException("미복원 신전 가상 Owner 표");
        }

        int ownerOffset = Pool.Edition == OriginalEdition.Patch1078 ? FortFinishOwnerShape.PatchOwnerOffset : FortFinishOwnerShape.CdOwnerOffset;
        vortex.Set(sid, Pool.Slot(sid)[ownerOffset]);
    }

    public void RebuildWorldGraph(bool mode)
    {
        if (_worldGraph is null)
        {
            throw new InvalidOperationException("월드 그래프 장치가 연결되지 않았다");
        }
        _worldGraph.Rebuild(mode);
        // 엔진이 복사해 가진 표·스택을 세션 저장소로 옮기는 반복문이다. 덤프 관측(DumpGraph)이 읽는 표다.
        for (int index = 0; index < Graph.Records.Length; ++index)
        {
            Graph.Records[index] = _worldGraph.Records[index];
        }
        for (int index = 0; index < Graph.Stack.Length; ++index)
        {
            Graph.Stack[index] = _worldGraph.FloodStack[index];
        }
    }

    /// <summary>빈 프레임 표 188개를 만든다. 검사 하네스의 LoadScene과 같은 공유 빈 표다.</summary>
    /// <returns>프레임 표다.</returns>
    /// <remarks>2026-11-12 0.436에서 월드 적재 공유 저장소로 추가했다.</remarks>
    private static RiftTypeFrames[] EmptyWorldFrames()
    {
        var frames = new RiftTypeFrames[WorldFrameCount];
        var empty = new RiftTypeFrames([]);
        // 모든 칸을 빈 표로 채우는 반복문이다.
        for (int index = 0; index < frames.Length; ++index)
        {
            frames[index] = empty;
        }

        return frames;
    }

    /// <summary>타입 정의의 클러스터 코드를 세션 프레임 표에 채운다. 자산 번호 = 70 + 적재 순서다.</summary>
    /// <param name="definitions">로딩 순서와 같은 타입 정의 목록이다.</param>
    /// <remarks>2026-11-12 0.442에서 실제 타입 채우기로 추가했다. 자산 밖 번호(0~69·186~187)는 빈 표로 둔다.
    /// 실제 지형·객체 효과가 프레임을 읽기 전까지 덤프에 영향을 주지 않는다.</remarks>
    public void FillWorldFrames(IReadOnlyList<RiftTypeDefinition> definitions)
    {
        ArgumentNullException.ThrowIfNull(definitions);
        if (RiftTypeTable.FirstAssetTypeNumber + definitions.Count > WorldFrames.Length)
            throw new ArgumentOutOfRangeException(nameof(definitions), "타입 정의가 프레임 표 폭을 넘는다");
        // 적재 순서마다 클러스터 코드를 해당 자산 번호 칸에 두는 반복문이다.
        for (int index = 0; index < definitions.Count; ++index)
        {
            var codes = new RiftFrameCode[definitions[index].Clusters.Count];
            // 클러스터 선언 순서대로 코드를 옮기는 반복문이다.
            for (int code = 0; code < codes.Length; ++code)
            {
                codes[code] = definitions[index].Clusters[code].Code;
            }

            WorldFrames[RiftTypeTable.FirstAssetTypeNumber + index] = new RiftTypeFrames(codes);
        }
    }

    /// <summary>월드 적재의 로딩 깊이 공유 값이다. Territory·Money·Meta가 함께 읽고 쓴다.</summary>
    /// <remarks>2026-11-12 0.437에서 월드 적재 드라이런용으로 추가했다. 실제 값 경로는 시작 흐름 단위에서 잇는다.</remarks>
    private uint _worldLoadingDepth;

    /// <summary>세션 저장소를 공유하는 섬 런타임 부모를 만든다. 호출자가 효과 묶음을 준다.</summary>
    /// <param name="islandHooks">섬 런타임의 거주 구역·진단 효과다.</param>
    /// <returns>세션 풀·기록·모드·목록·해시를 공유하는 섬 런타임이다.</returns>
    /// <remarks>2026-11-12 0.452에서 섬 부모 조립용으로 추가했다. 지형 바인딩과 적재 본체가 같은 부모를 공유하는 방식은 다음 단위에서 잇는다.
    /// 2026-11-12 0.477에서 세션 Player 기록을 묶었다(근거 cpppj RawCampaignOwnership.cpp 61행 islands.BindPlayerRecords: 소용돌이→섬 소유자→첫 지역 사슬이 같은 Player 기록을 읽고 쓴다). 단일 미션 +0x78 기록의 전제이며 호출 자체는 소용돌이 체인(R6)에서 잇는다.</remarks>
    public RawIslandRuntime BuildIslands(IslandRuntimeHooks islandHooks)
    {
        ArgumentNullException.ThrowIfNull(islandHooks);
        var islands = new RawIslandRuntime(
            Pool, IslandHash, IslandRecords, IslandModes, IslandResidences,
            () => IslandChanged, value => IslandChanged = value,
            () => IslandRevision, value => IslandRevision = value, islandHooks);
        islands.BindPlayerRecords(PlayerRecords);
        return islands;
    }

    /// <summary>월드 적재 setConfig 전달을 실제 설정 경계에 연결한다. NUL 포함 핸들은 C 문자열처럼 앞에서 자른다.</summary>
    /// <param name="configuration">쓸 설정 인터페이스다.</param>
    /// <returns>이름·NUL 포함 바이트를 받는 전달 효과다.</returns>
    /// <remarks>2026-11-12 0.439에서 SetConfig 라이브 연결로 추가했다. fort. 접두 키는 fort 객체에만 쓰고
    /// 루트 변경 표시를 켜지 않아 options 저장을 촉발하지 않는다(0.438). 전체 바이트는 공유 HandleBuffer에 남는다.</remarks>
    public static Action<string, byte[]> BuildSetConfigHook(ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(configuration);
        return (name, handle) =>
        {
            ArgumentException.ThrowIfNullOrEmpty(name);
            ArgumentNullException.ThrowIfNull(handle);
            configuration.Set(name, System.Text.Encoding.Latin1.GetString(handle));
        };
    }

    /// <summary>세션 저장소를 공유하는 월드 적재 부모를 만든다. 호출자가 효과 묶음을 모두 준다.</summary>
    /// <param name="islandHooks">섬 런타임의 거주 구역·진단 효과다.</param>
    /// <param name="territoryHooks">Territory 적재의 child·진단 효과다.</param>
    /// <param name="types">장부 몸체가 읽는 실제 타입 표다.</param>
    /// <param name="playerHooks">Player 부모의 생성·공간 효과다.</param>
    /// <param name="loadHooks">적재 본체의 번역·설정·생성·카메라 효과다.</param>
    /// <returns>세션 풀·기록·핸들을 공유하는 적재 부모다.</returns>
    /// <remarks>2026-11-12 0.437에서 월드 적재 드라이런용으로 추가했다. 덤프·시작 흐름 연결 없이 호출 순서만 기록한다.
    /// 로컬 소유자는 세션 TemplateOwner를 읽으며, puzzlePiece·지역없음은 세션 장기 전역에서 잇는다(0.455).</remarks>
    public RawCampaignWorldLoad BuildWorldLoad(
        IslandRuntimeHooks islandHooks,
        FortWorldLoadHooks territoryHooks,
        IReadOnlyList<OriginalTypeRecord> types,
        PlayerWorldHooks playerHooks,
        CampaignWorldLoadHooks loadHooks)
    {
        ArgumentNullException.ThrowIfNull(islandHooks);
        ArgumentNullException.ThrowIfNull(territoryHooks);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(playerHooks);
        ArgumentNullException.ThrowIfNull(loadHooks);
        var islands = new RawIslandRuntime(
            Pool, IslandHash, IslandRecords, IslandModes, IslandResidences,
            () => IslandChanged, value => IslandChanged = value,
            () => IslandRevision, value => IslandRevision = value, islandHooks);
        var inner = new RawPostPop(Pool, types, new RawPostPopState { Suppressed = true });
        var player = ReferenceEquals(playerHooks, _livePlayerHooks)
            ? PlayerWorld
            : new RawPlayerWorld(Pool, inner, PlayerRecords, MissionPlayer, playerHooks);
        var territory = new RawFortWorldLoad(
            Pool, World, islands, WorldFrames,
            () => GameAux.PuzzlePieceType, () => MissionPlayer.TemplateOwner, () => TerrainGlobals.NoRegion,
            () => _worldLoadingDepth, value => _worldLoadingDepth = value, territoryHooks);
        return new RawCampaignWorldLoad(
            World, islands, WorldFrames, () => GameAux.PuzzlePieceType, () => TerrainGlobals.NoRegion, territory, player,
            WorldGlobals, WorldTemplates, HandleBuffer, loadHooks);
    }

    /// <summary>공유 핸들 버퍼에 번역 이름을 채운다. 원본 월드 적재의 핸들 버퍼 쓰기와 같다.</summary>
    /// <param name="translated">NUL 포함 번역 바이트다. 첫 NUL까지만 쓴다.</param>
    /// <remarks>2026-11-12 0.430에서 미션 값 채우기로 추가했다. 설정 전달과 이름 확정이 같은 버퍼를 읽는다.</remarks>
    public void FillHandleName(byte[] translated)
    {
        ArgumentNullException.ThrowIfNull(translated);
        int nul = Array.IndexOf(translated, (byte)0);
        if (nul < 0)
            throw new ArgumentOutOfRangeException(nameof(translated), "핸들 이름의 NUL 종료 바이트 누락");
        if (nul + 1 > HandleBuffer.Length)
            throw new ArgumentOutOfRangeException(nameof(translated), "핸들 이름이 버퍼 폭을 넘는다");
        // 첫 NUL까지만 복사하고 stale 꼬리는 둔다.
        Array.Copy(translated, 0, HandleBuffer, 0, nul + 1);
    }

    /// <summary>덤프용 Player 관측을 만든다. Start 순서 전까지 0이다.</summary>
    public SessionDumpPlayers DumpPlayers() =>
        new((byte[])PlayerRecords.Clone(), (byte[])HiddenPlayers.Clone(), (uint[])Alliances.Clone());

    /// <summary>덤프용 SP 관측을 만든다. 초기화 전에는 대시 9칸으로 나간다.</summary>
    public SessionDumpSp DumpSp() =>
        new(Sp.Initialized, Sp.Offset, Sp.Pools.ToArray(), Sp.Permutation.ToArray());

    /// <summary>덤프용 Graph 표 관측을 만든다. 세션 설치의 ResetTable 초기 상태를 둔다.</summary>
    public IReadOnlyList<SessionDumpGraphRow> DumpGraph()
    {
        var rows = new SessionDumpGraphRow[Graph.Records.Length];
        // 표 전체를 순서대로 복사한다.
        for (int index = 0; index < rows.Length; index++)
            rows[index] = new(Graph.Records[index].Surfaces, Graph.Records[index].InUse, Graph.Records[index].Reserved);
        return rows;
    }

    /// <summary>공통 장부의 현재 비용·깊이·생산 알림을 덤프로 관측한다.</summary>
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
