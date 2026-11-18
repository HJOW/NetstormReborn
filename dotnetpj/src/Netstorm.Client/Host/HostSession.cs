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
        var participation = new Netstorm.Core.Simulation.PlayerParticipation(PlayerRecords, Alliances, HiddenPlayers, new Netstorm.Core.Simulation.PlayerParticipationState(), new Netstorm.Core.Simulation.PlayerParticipationHooks());
        var create = new RawAiCreate(OriginalEdition.Patch1078, participation, hooks);
        // 인공지능(활성·전산) 소유자만 번호 순서대로 만든다.
        for (int owner = 1; owner <= MissionPlayers.PlayerCount; owner++)
        {
            if (!players.Players[owner].Active || !players.Players[owner].Computer) continue;
            if (!AiSettings.TryGetValue(owner, out byte[]? settings))
                throw new InvalidOperationException("인공지능 설정 없이 생성할 수 없다: " + owner);
            AiBrains[(uint)owner] = create.Create((uint)owner, settings);
        }
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
    /// SetState·FinishOwner는 기록만 하며(R6 공장 후처리·거주 배치에서 잇는다), Destroy·VirtualCount·내용물 부착은 미션 적재(Fort=false)에서 도달하지 않으므로 명시 경계로 둔다.
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
            SetState = (_, _) => { },
            SetOwner = SetWorldObjectOwner,
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
                // 다리·섬·표면과 공통 지원 타입은 지형 분배 Pop으로 등록한다. 전용 파생이 필요한 종류는 기록만 남긴다.
                if (TryPopFortTerrain(sid, x, y, flags))
                {
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

    /// <summary>적재한 FortTemplate의 객체를 실제 로더로 풀에 만든다. 장치 연결 전에 호출하면 거부한다.</summary>
    /// <param name="fort">객체를 읽을 FortTemplate다.</param>
    /// <param name="slot">미션 Template 칸 번호다. 1~8이면 기본 소유자로 쓴다.</param>
    /// <remarks>2026-11-12 0.467에서 요새 객체 live 적재용으로 추가했다. Generate 뒤 Objects 순서에서 부른다.</remarks>
    /// <summary>지형 분배 Pop으로 잇지 않고 기록만 남긴 요새 Pop 호출이다. R6 파생 본체 대기 목록이다. 0.607에서 추가됐다.</summary>
    private readonly List<string> _fortPopSkipped = [];

    /// <summary>지형 분배 Pop으로 잇지 않고 기록만 남긴 요새 Pop 호출이다. R6 파생 본체 대기 목록이며 풀 바이트에 영향을 주지 않는다. 0.607에서 추가됐다.</summary>
    public IReadOnlyList<string> FortPopSkips => _fortPopSkipped;

    /// <summary>요새 객체를 다리·섬·표면 분배 또는 공통 후처리로 등록한다. 전용 파생이 필요하면 거짓을 돌려준다.</summary>
    /// <param name="sid">등록할 오브젝트 번호다.</param>
    /// <param name="x">등록할 월드 가로 좌표다.</param>
    /// <param name="y">등록할 월드 세로 좌표다.</param>
    /// <param name="flags">Pop 호출 플래그다.</param>
    /// <returns>지형 분배 Pop으로 등록했으면 참이다. 전용 파생(R6)이나 장치 미연결이면 거짓이다.</returns>
    /// <remarks>0.607에서 R4-2 요새 일반 Pop 결선용으로 추가했다. 분배 본문의 분기 순서와 같은 판정이며 풀·공간을 바꾸기 전에 가린다.</remarks>
    private bool TryPopFortTerrain(int sid, float x, float y, uint flags)
    {
        // 분배 장치나 그래프 관문이 없으면 기록 경로로 돌린다.
        if (_terrainPostPop is null || _terrainPop is null)
        {
            return false;
        }
        uint vtable = BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid));
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
