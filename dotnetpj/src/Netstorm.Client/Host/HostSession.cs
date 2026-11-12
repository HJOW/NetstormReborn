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

    /// <summary>SP 초기화의 재시드 공급이다. 없으면 시작 SP 지정이 거부된다.</summary>
    /// <remarks>2026-11-12 0.395에서 StartPlayerSp 연결로 추가했다. 실제 공급(RawGameRandomSeed 사슬)은 미션 시작 흐름에서 잇는다.</remarks>
    private Func<uint>? _spReseed;

    /// <summary>세션이 소유한 분산 SP 저장소다. 초기화 전에는 덤프에 대시를 쓴다. 원본처럼 게임 공유 난수를 쓴다.</summary>
    /// <remarks>2026-11-12 0.457에서 단일 공유 스트림으로 바꿨다(근거 cpppj RawCampaignSession.cpp 498행, 995행).
    /// 2026-11-12 0.464에서 호스트 시작 1회 초기화를 잇려고 공유 저장소 빌림을 추가했다(원본 004391ed, cpppj ClientMain 349행).</remarks>
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
    /// <remarks>2026-11-12 0.464에서 cpppj 공유 SP 경로와 같은 시작 1회 초기화를 잇려고 공유 빌림을 추가했다. 미션별 reseed는 원본에 없다.</remarks>
    public HostSession(Kernel kernel, GameRandom game, CrtRandom crt, int poolCapacity = 0, RawSpStore? sharedSp = null, UIntBox? sharedCrtBox = null)
    {
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(crt);
        Kernel = kernel;
        Game = game;
        Crt = crt;
        Pool = new RawSidPool(poolCapacity <= 0 ? MissionPoolCapacity : poolCapacity);
        Sp = sharedSp ?? new RawSpStore(Game, () => _spReseed is not null ? _spReseed() : throw new InvalidOperationException("SP 재시드 미연결"));
        TerrainCrt = sharedCrtBox ?? new();
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
    /// <remarks>2026-11-12 0.474에서 R4 전투섬 배정 region 갈고리 live 연결용으로 추가했다. 근거: cpppj `RawCampaignSession.cpp` 3243행 `hooks.region`(범위 밖 9 이상은 `out_of_range`). AI 참가자의 지역은 AI 설정 사슬에서 덮어쓰므로(R6) 시작 흐름에서는 전투 소유자(로컬 인간)만 호출한다.</remarks>
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
    /// <remarks>2026-11-12 0.474에서 추가했다. `BattleIslandHooks.Region` 자리에 그대로 꽂을 수 있으며 미연결 `RawBattleIsland` 호출자는 이 대리자를 그대로 쓴다.</remarks>
    public Action<uint, uint> BuildBattleIslandRegionHook() => WriteBattleIslandRegion;

    /// <summary>전투 Territory 120바이트에서 전투 소유자의 시작 섬 번호를 푼다. 배치 순서대로 셈한다.</summary>
    /// <param name="territory">20개 Territory(120바이트)다. 미션 요새의 전투 소유자 슬롯 값이다.</param>
    /// <param name="island">배정된 섬 번호다. 고른 영역이 배치 불가면 0이다.</param>
    /// <returns>고른 영역이 배치 가능하면 true, 아니면 false다.</returns>
    /// <remarks>2026-11-12 0.474에서 시작 흐름 연결용으로 추가했다. 고른 영역은 `RawBattleIsland.SelectBattleTerritory`와 같은 규칙(0x04 첫 영역·없으면 0번)이며 섬 번호는 `RawIslandBuilder.PlaceAll`과 같은 배치 순서(앞선 배치 가능 레코드 수, flags bit0 켜짐·bit1 꺼짐)다. 배치 불가면 127 초기값을 그대로 두려고 false를 돌려준다.</remarks>
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
    public RawGameAuxState GameAux { get; } = new();

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

    /// <summary>세션이 소유한 지형 생성 장부다. SetOwner의 소유자별 작업장 목록을 둔다. 장부 집계 연결은 다음 단위다.</summary>
    /// <remarks>2026-11-12 0.455에서 Generate 라이브 연결용으로 추가했다. Pop 장부·그래프·집계(totals·totalmade) 연결은 다음 단위다.</remarks>
    public RawPostPopState TerrainBookkeeping { get; } = new();

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
    /// <remarks>2026-11-12 0.466에서 지형 Pop 공간 등록 라이브 연결용으로 추가했다. 근거: cpppj MakeTerrainStampWorldHooks(core.factory·core.hash·core.shapes→FrameSize)의 세션 결선이다.</remarks>
    public void BuildTerrainPopDevices(IReadOnlyList<OriginalTypeRecord> types, List<SquidDisplayShape> shapes)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(shapes);
        if (types.Count != OriginalConstructors.TypeCount || shapes.Count != types.Count)
        {
            throw new ArgumentException("지형 Pop 타입·셰이프 목록 크기 오류", nameof(types));
        }
        _terrainFactory = new RawSquidFactory(Pool, types);
        _terrainHash = new RawSquidHash();
        _terrainSpots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        _terrainDisplay = new SquidDisplay(
            Pool.Edition,
            types,
            shapes,
            new NullTerrainDisplaySink(),
            new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 640, 480) });
        _terrainDisplay.ValidateFrameBinding(Pool.Edition, types);
        _terrainPop = new RawPop(Pool, _terrainHash, _terrainSpots);
        _terrainPopTypes = types;
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
            Pop = (sid, x, y, flags) =>
            {
                // Pop 장치가 없으면 기록만 한다(드라이런 중립).
                if (_terrainPop is null || _terrainDisplay is null || _terrainPopTypes is null)
                {
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
                _terrainPop.Pop(sid, _terrainPopTypes[number], size[0], size[1], x, y, flags);
            },
            SetRegion = (sid, region) => RawSquidRegion.SetSquidRegion(Pool, types, sid, unchecked((uint)region), unchecked((uint)TerrainGlobals.NoRegion), TerrainDiagnostics.Region),
            ChunkRegion = (x, y) => unchecked((sbyte)World.At(x, y).Island),
            Assertion = TerrainDiagnostics.Stamp,
        };
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
    /// <remarks>2026-11-12 0.467에서 요새 객체 live 적재용으로 추가했다. 근거: cpppj RawCampaignWorldRuntime.cpp 43~53행(객체 갈고리+MakeFortObjectLoadHooks+RawFortObjectLoad)의 세션 결선이다.
    /// SetState·FinishOwner는 기록만 하며(R6 공장 후처리·거주 배치에서 잇는다), Destroy·VirtualCount·내용물 부착은 미션 적재(Fort=false)에서 도달하지 않으므로 명시 경계로 둔다.
    /// 2026-11-12 0.475에서 소유자 지정 몸체를 Player 월드 갈고리와 공유하려고 `_playerOwner` 필드에 보관한다.</remarks>
    public void BuildFortObjectDevices(IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames)
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
        _fortFactory = new RawSquidFactory(Pool, types);
        _fortHp = new RawSquidReward(Pool, types, TerrainBookkeeping, new SquidRewardState(), Sp);
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
        _fortBuilding = new RawBuildingSpace(Pool, _terrainHash, types,
            new BuildingSpaceState { IslandType = TerrainGlobals.IslandType, BridgeType = 82 });
        _fortPop = new RawPop(Pool, _terrainHash, _terrainSpots);
        _fortPop.SetBuildingSide(_fortBuilding);
        _fortState = new FortObjectLoadState { Mission = true, ChaffMode = true };
        _playerOwner = new SquidOwner(Pool, types, TerrainBookkeeping, OwnerMode);
        RawIslandRuntime islands = BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        var hooks = new FortObjectLoadHooks
        {
            NotifySurface = sid => _fortBuilding.NotifySurface(sid),
            SetState = (_, _) => { },
            SetOwner = _playerOwner.Set,
            HasHitPoints = _fortHp.HasHitPoints,
            MaxHitPoints = _fortHp.MaxHitPoints,
            VirtualCount = _ => throw new InvalidOperationException("Fort 가상 개수 미연결(R6)"),
            Pop = (sid, x, y, flags) =>
            {
                // 요새 객체 Pop의 공간 등록은 R6 postPop·표시·그래프 장치와 함께 잇는다.
                // 실측 결과 thewarbegins 요새 객체에 Pop 지원표 밖 가상 표(71·75·78·89·122 등)가 있어
                // 지금 공간 등록을 수행하면 파생 등록 효과 미지원으로 멈춘다. 생성·소유자·HP·프레임은 live로 둔다.
            },
            Destroy = (_, _) => throw new InvalidOperationException("Fort 파괴 미연결(R6)"),
            CreateContent = _fortContained.Create,
            ChunkRegion = (x, y) => unchecked((sbyte)World.At(x, y).Island),
            TerritoryAllowed = (region, territoryOwner, territory) => islands.Matches(region, unchecked((int)territoryOwner), territory),
            LoadingDepthChanged = depth => _worldLoadingDepth = depth,
            FinishOwner = _ => { },
        };
        RawFortObjectLoad.MakeFortObjectLoadHooks(Pool, _fortFactory, _fortHp, _fortContained, hooks);
        _fortLoader = new RawFortObjectLoad(Pool, _terrainHash, types, frames, Game, _fortState, hooks);
    }

    /// <summary>적재한 FortTemplate의 객체를 실제 로더로 풀에 만든다. 장치 연결 전에 호출하면 거부한다.</summary>
    /// <param name="fort">객체를 읽을 FortTemplate다.</param>
    /// <param name="slot">미션 Template 칸 번호다. 1~8이면 기본 소유자로 쓴다.</param>
    /// <remarks>2026-11-12 0.467에서 요새 객체 live 적재용으로 추가했다. Generate 뒤 Objects 순서에서 부른다.</remarks>
    public void LoadFortObjects(FortTemplate fort, uint slot)
    {
        ArgumentNullException.ThrowIfNull(fort);
        if (_fortLoader is null)
        {
            throw new InvalidOperationException("요새 객체 장치가 연결되지 않았다");
        }
        _fortLoader.Load(fort, unchecked((int)slot));
    }

    /// <summary>캠페인 월드 적재의 Player 월드(화신) 갈고리를 실제 장치에 연결한다. 요새 객체 장치 뒤에 연결해야 한다.</summary>
    /// <returns>같은 풀·공장·소유자·공간·표시를 공유하는 Player 월드 갈고리다.</returns>
    /// <remarks>2026-11-12 0.475에서 Player 월드 갈고리 live 연결용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1235~1242행(playerHooks 결선: find→world.Find·create→world.Create·setOwner→core.owner.Set·pop→world.Pop). 화신 생성의 Pop 호출은 기록만 하며 공간 등록은 postPop·표시 장치와 함께 R6에서 잇고, ReserveRegular·AddRegular도 정기 프로세스 몫이라 R6에서 잇는다(패치판 생성자는 요구하지 않는다).</remarks>
    public PlayerWorldHooks BuildPlayerWorldHooks()
    {
        if (_fortFactory is null || _playerOwner is null
            || _terrainHash is null || _terrainPopTypes is null)
        {
            throw new InvalidOperationException("Player 월드 장치는 요새 객체 장치 뒤에 연결해야 한다");
        }
        RawSquidFactory factory = _fortFactory;
        SquidOwner owner = _playerOwner;
        RawSquidHash hash = _terrainHash;
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes;
        return new PlayerWorldHooks
        {
            Find = (x, y, type) =>
            {
                // 같은 타입의 첫 슬롯을 돌려주고 없으면 원문처럼 0을 돌려준다.
                var finder = new RawSquidFinder(Pool, hash, types);
                int left = unchecked((int)x);
                int top = unchecked((int)y);
                // 찾은 후보마다 슬롯 타입을 확인하는 반복문이다.
                for (int sid = finder.Begin(new SquidSearchArea(left, top, left, top)); sid != 0; sid = finder.Next())
                {
                    if (Pool.Slot(sid)[RawSidPool.TypeOffset] == type)
                    {
                        return sid;
                    }
                }
                return 0;
            },
            Create = (type, flags) => factory.Create(type,
                (flags & 1) != 0 ? Netstorm.Core.Simulation.SidAllocation.Predictable
                : (flags & 2) != 0 ? Netstorm.Core.Simulation.SidAllocation.Client
                : Netstorm.Core.Simulation.SidAllocation.Server),
            SetOwner = (sid, player) => owner.Set(sid, player),
            Pop = (_, _, _, _) =>
            {
                // 화신 Pop의 공간 등록은 R6 postPop·표시·그래프 장치와 함께 잇는다.
                // 실측 결과 152번(화신) 가상표는 표시·파생 등록 미지원이라 지금 공간 등록을 수행하면 멈춘다(0.467 요새 객체 Pop과 같은 경계).
            },
        };
    }

    /// <summary>월드 그래프 재구성의 세션 소유 엔진이다. BuildWorldGraphDevices로 연결하기 전에는 null이다.</summary>
    /// <remarks>2026-11-12 0.470에서 그래프 live 재구성용으로 추가했다. 근거: cpppj RawCampaignWorldRuntime.cpp 65·83행(core.graph 공유 엔진)의 세션 결선이다.</remarks>
    private RawGraph? _worldGraph;

    /// <summary>월드 적재 그래프 재구성 장치를 세션에 연결한다. 연결 전에는 재구성 요청이 거부된다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 패치판 188개·CD판 171개여야 한다.</param>
    /// <remarks>2026-11-12 0.470에서 그래프 live 재구성용으로 추가했다. 지형 Pop 장치의 해시·점유 지도(core.hash·core.spots 구실)와 채운 WorldFrames를 그대로 쓰며, ResetTable은 cpppj 적재 경로에서도 호출되지 않아 연결하지 않는다.</remarks>
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
    }

    /// <summary>월드 그래프를 다시 구성하고 세션 표 저장소로 결과를 옮긴다. 장치 연결 전에 호출하면 거부한다.</summary>
    /// <param name="mode">참이면 전체 초기화 재구성, 거짓이면 무효 번호 수집이다.</param>
    /// <remarks>2026-11-12 0.470에서 그래프 live 재구성용으로 추가했다. 적재 흐름의 territory·campaign 갈고리가 mode 거짓으로 부른다. 난수를 소비하지 않으며 슬롯 그래프 바이트·표·스택만 바꾼다.</remarks>
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
    /// <remarks>2026-11-12 0.452에서 섬 부모 조립용으로 추가했다. 지형 바인딩과 적재 본체가 같은 부모를 공유하는 방식은 다음 단위에서 잇는다.</remarks>
    public RawIslandRuntime BuildIslands(IslandRuntimeHooks islandHooks)
    {
        ArgumentNullException.ThrowIfNull(islandHooks);
        return new RawIslandRuntime(
            Pool, IslandHash, IslandRecords, IslandModes, IslandResidences,
            () => IslandChanged, value => IslandChanged = value,
            () => IslandRevision, value => IslandRevision = value, islandHooks);
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
        var player = new RawPlayerWorld(Pool, inner, PlayerRecords, MissionPlayer, playerHooks);
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
