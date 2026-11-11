using Netstorm.Client.Automation;
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

    /// <summary>세션이 만든 인공지능 brain 토큰이다. 키는 소유자 번호다. 실제 생성 전에는 비어 있다.</summary>
    /// <remarks>2026-11-12 0.399에서 AI 생성 본체 연결로 추가했다. 실제 brain 생성자·초기 갱신은 호출자가
    /// 주입한 효과로 받으며(월드 본체 연결 전까지 기록용 스텁), 같은 Player 기록에 부착한다.
    /// 라이브 호스트 시작 흐름 연결은 월드 효과와 함께 다음 단위다.</remarks>
    public Dictionary<uint, uint> AiBrains { get; } = [];

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

    /// <summary>전투 소유자 기록에 번역된 현재 이름을 덮어쓴다. 원본 월드 적재의 이름 확정 쓰기와 같다.</summary>
    /// <param name="translated">NUL 포함 번역 바이트다. 첫 NUL까지만 쓴다.</param>
    /// <remarks>2026-11-12 0.428에서 미션 값 채우기로 추가했다. NUL 뒤 stale 꼬리는 보존하므로 영어 "You"는 "You\0pson"이 된다.</remarks>
    public void WriteBattleName(byte[] translated)
    {
        ArgumentNullException.ThrowIfNull(translated);
        int nul = Array.IndexOf(translated, (byte)0);
        if (nul < 0)
            throw new ArgumentOutOfRangeException(nameof(translated), "전투 이름의 NUL 종료 바이트 누락");
        uint owner = MissionPlayer.BattleOwner;
        if (owner > (uint)MissionPlayers.PlayerCount)
            throw new ArgumentOutOfRangeException(nameof(translated), "전투 이름의 실제 Player 번호 범위 오류");
        int record = (int)owner * PlayerRecordStride;
        if (nul + 1 > PlayerNameWidth)
            throw new ArgumentOutOfRangeException(nameof(translated), "전투 이름이 기록 폭을 넘는다");
        // 첫 NUL까지만 복사하고 stale 꼬리는 둔다.
        Array.Copy(translated, 0, PlayerRecords, record + PlayerNameOffset, nul + 1);
    }

    /// <summary>세션이 센 로그인 요새 순서 수다. 원본 로그인의 fortCount와 같으며 첫 로그인에 0을 둔다.</summary>
    public uint FortCount { get; private set; }

    /// <summary>세션이 마지막에 새 요새를 받은 Player 번호다. 원본 로그인의 lastLoginPlayer와 같다.</summary>
    public uint LastLoginPlayer { get; private set; }

    /// <summary>세션이 공유하는 현재 Player 상태다. Zone 설치가 소유자를 정하고 이름 확정이 전투 소유자를 읽는다.</summary>
    public PlayerWorldState MissionPlayer { get; } = new();

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
