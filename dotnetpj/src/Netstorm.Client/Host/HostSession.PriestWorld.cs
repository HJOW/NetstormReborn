using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 실제 호스트의 공통 공간·HP·목록·프로세스에 사제 몸체를 등록한다.
namespace Netstorm.Client.Host;

/// <summary>사제 적재·후처리·회복이 같은 실제 월드 자원을 빌리는 결선이다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 미션의 실제 사제/보호막 가상 분배기다.</summary>
    private RawPriestWorldBindings? _priests;
    /// <summary>사제 등록 전에 사용하던 공통 정기 분배기다.</summary>
    private RegularHandler? _beforePriestRegular;

    /// <summary>같은 장기 전송 버퍼를 사용하는 실제 단독 미션 writer다.</summary>
    private RawSquidTransmission? _worldTransmission;
    /// <summary>진단 보관이 장기 플레이 메모리를 늘리지 않도록 최근64개만 남긴다.</summary>
    private const int WorldDiagnosticCapacity = 64;
    /// <summary>단독 미션에서 실제 직렬화한 패킷을 비교용으로 보관한다. 네트워크 송신 기록이 아니다.</summary>
    private readonly List<byte[]> _worldPackets = [];
    /// <summary>같은 Pop/전송 부모가 보고한 실제 수준·문장이다.</summary>
    private readonly List<(int Level, string Text)> _worldPopTrace = [];

    /// <summary>단독 미션 writer의 실제 출력 사본을 L2/L3 대조용으로 빌린다.</summary>
    public IReadOnlyList<byte[]> WorldPackets => _worldPackets;
    /// <summary>같은 Pop 부모의 실제 진단 로그를 읽는다.</summary>
    public IReadOnlyList<(int Level, string Text)> WorldPopTrace => _worldPopTrace;

    /// <summary>실제 사제 몸체를 빌린다. 구성 전과 반납 후의 조회는 거부한다.</summary>
    public RawPriestWorldBindings PriestWorld => _priests ?? throw new InvalidOperationException("호스트 실제 사제 월드 미연결");

    /// <summary>아직 이식하지 않은 선택 효과를 성공으로 대신하지 않고 호출 위치에서 드러낸다.</summary>
    private static InvalidOperationException PriestMissing(string effect) => new($"호스트 사제 {effect} 실제 몸체 미연결(R6)");

    /// <summary>같은 Player 공통 부모에 사제·보호막을 등록하고 실제 권한/Player9/SP를 빌린다.</summary>
    /// <remarks>cpppj RawCampaignSession의1191~1231행 조립 대응이다. 외부 피해/해방·소리/안내는
    /// 호출자가 실제 효과를 제공하며, 미연결 선택은 명시 예외로 유지한다. ValidateReady는 후속 경계까지 연결한 뒤 호출한다.</remarks>
    public RawPriestWorldBindings BuildPriestWorldDevices(RawPriestWorldEffects? effects = null)
    {
        if (_priests is not null) return _priests;
        _ = BindLivePlayerWorld();
        if (_fortHp is null || _fortFactory is null || _playerOwner is null || _fortBuilding is null)
            throw new InvalidOperationException("사제 월드는 실제 요새 HP/공장/공간 뒤에 연결해야 한다");

        RawPriestWorldEffects live = effects?.Copy() ?? new();
        live.Pop = PopWorldObject; live.Unpop = UnpopWorldObject; live.Repop = RepopWorldObject;
        live.OwnerNotice = _fortBuilding.NotifySurface; live.NotifySurface = _fortBuilding.NotifySurface;
        live.NonPriestOwner ??= _playerOwner.Set;
        live.OwnerAssertion ??= (expression, file, line) => throw new InvalidOperationException($"사제 Owner 원본 진단: {expression} {file}:{line}");
        live.ForcefieldAssertion ??= (expression, file, line) => throw new InvalidOperationException($"보호막 원본 진단: {expression} {file}:{line}");
        live.FindTypeAt = FindWorldTypeAt;
        live.Destroy ??= (_, _) => throw PriestMissing("가상 삭제");
        live.ClaimGround ??= (_, _, _) => throw PriestMissing("다이스 소유자");
        live.DependentFound ??= () => throw PriestMissing("너겟 보충");
        // cpppj 실제 조립의 reserveRegular []{return true;}와 같으며 AddRegular가 실제 객체를 할당한다.
        live.ReserveRegular ??= static () => true;
        live.FallSound ??= (_, _, _) => throw PriestMissing("낙하 위치 소리");
        live.OtherWalkerFall ??= _ => throw PriestMissing("다른 Walker 낙하");
        live.Shield.ReserveSound ??= () => throw PriestMissing("보호막 소리 예약");
        live.Shield.LoadSound ??= _ => throw PriestMissing("보호막 소리 조회");
        live.Shield.AttachSound ??= (_, _, _, _) => throw PriestMissing("보호막 Sound47 부착");
        live.Shield.NoticeSound ??= _ => throw PriestMissing("사제 안내 소리");
        live.Shield.Translate ??= _ => throw PriestMissing("사제 안내 번역");
        live.Shield.Tell ??= _ => throw PriestMissing("사제 안내 Tell");
        live.Regen.Refresh = sid => _terrainDisplay!.Update(Pool.Slot(sid));
        live.Regen.PatchAudit = Sp.Rotate;
        live.Regen.SpReads = RawPriestRegen.BindPriestRegenSpStore(Sp);
        live.Regen.Damageable ??= _ => throw PriestMissing("피해 표시 갱신");
        live.Regen.NeutralAction ??= _ => throw PriestMissing("중립 사제 대피");
        live.Regen.Measurement ??= () => throw PriestMissing("회복 추적 측정");

        PriestState.Carrier.SharedLoadingDepth = () => _worldLoadingDepth;
        PriestState.Carrier.SharedGeyserType = () => GameAux.GeyserType;
        _beforePriestRegular = PlayerProcesses.Regular();
        var bound = new RawPriestWorldBindings(new(Pool, _terrainHash!, _terrainPopTypes!, WorldFrames,
            _terrainSpots!, _terrainHash!.BorrowEntries(0), _fortHp, Game, _fortFactory, _playerOwner,
            OwnerMode, _livePostPop!, _liveFrame!), PriestState, live);
        bound.BindProcesses(PlayerProcesses); bound.BindAuthority(Gameplay.Authority);
        bound.BindPlayerRecords(PlayerRecords); bound.SetLocalPlayer(MissionPlayer.BattleOwner);
        bound.SetLoadingDepth(_worldLoadingDepth);
        PlayerProcesses.SetRegularHandler(bound.RegularHandler(_beforePriestRegular));
        _priests = bound;
        return bound;
    }

    /// <summary>로더의 동일 깊이를 사제 Owner·보호막·Carrier·중립 회복에 전달한다.</summary>
    public void SetWorldLoadingDepth(uint depth)
    {
        _worldLoadingDepth = depth;
        _priests?.SetLoadingDepth(depth);
    }

    /// <summary>실제 현재 Pop 부모와35BYTE/압축 writer를 같은 TYPE·공장·커널에 연결한다.</summary>
    /// <remarks>cpppj RawSurfaceWorld.PopCurrent의 독립 typed 경로를 따른다. 단독 미션은 writer 뒤
    /// 방송에서 자연 반환하고 출력만 진단에 보관한다. MP 경로를 같은 성공으로 대체하지 않는다.</remarks>
    private void BindWorldPopParent()
    {
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes!;
        // 같은 원문 수준·문장을 실제 호스트 진단 저널에 보존한다.
        void Trace(int level, string text)
        {
            if (_worldPopTrace.Count == WorldDiagnosticCapacity) _worldPopTrace.RemoveAt(0);
            _worldPopTrace.Add((level, text));
        }
        // 실제 호스트의 기본 진단은 원문 위치·식으로 명시 실패한다.
        static void Assertion(string expression, string file, uint line) =>
            throw new InvalidOperationException($"월드 Pop 원본 진단: {expression} {file}:{line}");
        _livePop!.BindDiagnostics(new()
        {
            Types = types, Checking = new RawCheckingFlag(() => AiChecking.Read()),
            Metamap = () => OwnerMode.Challenge, Trace = Trace, Assertion = Assertion,
        });
        _worldTransmission = new RawSquidTransmission(Pool, GameAux.SquidTransmission, new()
        {
            Type = number => new SquidPopTypeReference(() => types[number]),
            DefaultType = () => BridgeState.BridgeType,
            MaxHp = number => _fortFactory!.ReadSelectedHitPoints(types[number], types),
            Trace = Trace, Assertion = Assertion,
            Broadcast = packet =>
            {
                if (!BridgeState.Mission) throw new InvalidOperationException("현재 호스트 MP Squid 방송 미연결");
                if (_worldPackets.Count == WorldDiagnosticCapacity) _worldPackets.RemoveAt(0);
                _worldPackets.Add(packet.ToArray());
            },
            Connected = _ => throw new InvalidOperationException("현재 호스트 Squid 대상 연결 미연결"),
            Send = (_, _) => throw new InvalidOperationException("현재 호스트 Squid 대상 송신 미연결"),
            ProcessPacket = (form, packet, argument) => PlayerProcesses.WriteFormPacket(form, argument, packet, GameAux.SquidTransmission),
        });
        _livePop.BindTransmission(_worldTransmission.Pop);
    }

    /// <summary>사제의 실제 가상 소유자 또는 기존 공통 부모에 같은 슬롯을 전달한다.</summary>
    private void SetWorldObjectOwner(int sid, uint player)
    {
        if (_priests is not null) _priests.SetOwner(sid, player);
        else (_playerOwner ?? throw new InvalidOperationException("월드 공통 Owner 미연결")).Set(sid, player);
    }

    /// <summary>같은 실제 공통 Pop으로 좌표·공간·사제/보호막 가상 후처리를 함께 실행한다.</summary>
    public void PopWorldObject(int sid, float x, float y, uint flags)
    {
        if (_livePop is null || _terrainPopTypes is null || _terrainDisplay is null)
            throw new InvalidOperationException("월드 공통 Pop 미연결");
        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        int frame = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
        float[] size = _terrainDisplay.FrameSize(Pool.Edition, raw, frame);
        TerrainBookkeeping.LocalOwner = checked((byte)MissionPlayer.BattleOwner);
        _livePop.Pop(sid, _terrainPopTypes[raw[10]], size[0], size[1], x, y, flags);
    }

    /// <summary>같은 현재 타입과 실제 해시/점유 지도의 공간 등록을 해제한다.</summary>
    public void UnpopWorldObject(int sid, uint flags)
    {
        if (_liveUnpop is null || _terrainPopTypes is null) throw new InvalidOperationException("월드 공통 Unpop 미연결");
        _liveUnpop.Unpop(sid, _terrainPopTypes[Pool.Slot(sid)[10]], flags);
    }

    /// <summary>공간 해제 뒤 슬롯에 남은 실제 좌표를 같은 Pop에 전달한다.</summary>
    public void RepopWorldObject(int sid, uint flags)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        float x = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[14..]));
        float y = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..]));
        PopWorldObject(sid, x, y, flags);
    }

    /// <summary>RawBridgeConnect.FindTypeAt과 같은 타입별 단계로 현재 좌표의 첫 일치 객체를 찾는다.</summary>
    private int FindWorldTypeAt(float x, float y, uint type)
    {
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("월드 TYPE 미연결");
        if (type >= types.Count || !float.IsFinite(x) || !float.IsFinite(y)) throw new ArgumentOutOfRangeException(nameof(type));
        var finder = new RawSquidFinder(Pool, _terrainHash!, types);
        var area = new SquidSearchArea(checked((int)x), checked((int)y), checked((int)x), checked((int)y));
        // 원문 Island|Bridge 마스크0x6에 따라 단계0 또는 단계1부터 첫 타입 일치를 찾는다.
        for (int sid = finder.Begin(area, (types[(int)type].Flags2 & 0x6u) == 0 ? 1u : 0u); sid != 0; sid = finder.Next())
            if (Pool.Slot(sid)[10] == type) return sid;
        return 0;
    }

    /// <summary>실제 커널의 이 Host 프로세스를 먼저 비운 뒤 가상 등록·Player9 대여를 반납한다.</summary>
    private void ReleasePriestWorld()
    {
        if (_priests is null) return;
        PlayerProcesses.SetRegularHandler(_beforePriestRegular);
        _priests.Dispose(); _priests = null; _beforePriestRegular = null;
    }
}
