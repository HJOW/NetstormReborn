using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 현재 Host의 공장/공간/TYPE/이름/장기 전역으로 실제 객체 수신 본체를 실행한다.
namespace Netstorm.Client.Host;

/// <summary>객체 수신에 필요한 실제 표시 BYTE Tell·원문 로그·UI 명령 함수 값이다.</summary>
public sealed record HostSquidPacketEffects
{
    /// <summary>표시 디코더/실제 Tell에 넘길 현재 BYTE 문장이다.</summary>
    public Action<ReadOnlyMemory<byte>>? Tell { get; init; }
    /// <summary>현재 수준과 원문 검증 문장을 쓰는 실제 로그다.</summary>
    public Action<int, string>? Log { get; init; }
    /// <summary>현재 종속 UI의 명령을 실제 입력 큐로 전달한다.</summary>
    public Action<uint, uint, uint>? Command { get; init; }
    /// <summary>카메라 수신이 원문 위치에서 조회하는 현재 논리 화면 크기다.</summary>
    public Func<(int Width, int Height)>? ScreenSize { get; init; }
    /// <summary>독립 typed 수명에서 이미 작성한 실제 Form 패킷을 전달한다. Native는 장기 전송 장치를 직접 사용한다.</summary>
    public Action<ReadOnlyMemory<byte>>? FormPacket { get; init; }
    /// <summary>건설 환불의 실제6ch 송신이다. 설치 전 양수 환불은 통지 위치에서 명시 실패한다.</summary>
    public Action<uint, ConstructionMoneyNotice>? ConstructionMoney { get; init; }
}

/// <summary>고정F가 같은 객체/압축/검증 본체를 직접 선택하는 실제 Host 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 풀과 Client 진단을 소유하는 장기 저장소다.</summary>
    private RawSurfaceStorage? _squidPacketStorage;
    /// <summary>같은 현재 TYPE/언어·설정의 실제 설명 이름 몸체다.</summary>
    private RawSquidName? _squidPacketNames;
    /// <summary>외부에서 바꾼 원래 record와 독립된 현재 효과 함수 값이다.</summary>
    private HostSquidPacketEffects? _squidPacketEffects;
    /// <summary>생산 전에도 같은 장기 종속 UI 전역을 읽는 실제 기본 종단이다.</summary>
    private RawSquidContainedUnpop? _squidPacketUi;
    /// <summary>공장/공간 구성 뒤 같은 실제 풀/TYPE의 수신 부모를 연결한다. 현재 값을 초기화하거나 패킷을 보내지 않는다.</summary>
    public void BindSquidPacketDevices(RawSurfaceStorage storage, RawSquidName names, HostSquidPacketEffects effects)
    {
        ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(names); ArgumentNullException.ThrowIfNull(effects);
        if (!ReferenceEquals(storage.Pool, Pool) || !ReferenceEquals(storage.GameAux(), GameAux) || !ReferenceEquals(names.Pool, Pool)
            || !ReferenceEquals(names.Types, _terrainPopTypes)) throw new ArgumentException("객체 수신의 같은 실제 Pool/GameAux/TYPE 필요");
        EnsureRuntimeCanRelease();
        if (_playerRecordsReleased || ProcessBindingsReturned() || _playerProcessRuntime is null) throw new InvalidOperationException("객체 수신의 실제 공장/공간 수명 미연결 또는 반납 완료");
        _squidPacketStorage = storage; _squidPacketNames = names; _squidPacketEffects = effects with { };
        _squidPacketUi = new RawSquidContainedUnpop(Pool, new()
        {
            ContentFormType = Gameplay.ContentFormTypeBox, GumpFormType = Gameplay.GumpFormTypeBox, UiEvent = Gameplay.ContentRemovalEventBox,
            CheckingDead = new(() => AiChecking.Read()), InMetamapMode = () => OwnerMode.Challenge, Boss = () => Gameplay.Authority.Boss,
        }, new() { Command = (code, first, second) => FixedPacketNeed(_squidPacketEffects?.Command)(code, first, second), Assertion = storage.AssertDisplay });
        BindDaisPacketTransmission();
        BindPathSearchDevices(storage);
        BindConstructionRefundDevices();
    }
    /// <summary>실제 프로세스 수명이 끝날 때 수신 전용 이름/외부 효과/종속 UI의 참조를 함께 반납한다.</summary>
    private void ReleaseSquidPacketDevices()
    {
        _squidPacketStorage = null; _squidPacketNames = null; _squidPacketEffects = null; _squidPacketUi = null;
        _daisPacketTransmission = null;
        ReleasePathSearchDevices();
        ReleaseConstructionRefundDevices();
        ReleasePathParentDevices();
    }
    /// <summary>번호 선택 시점의 현재 TYPE를 읽는다.</summary>
    private OriginalTypeRecord SquidPacketType(uint type) => _terrainPopTypes is { } types && type < types.Count ? types[(int)type]
        : throw new ArgumentOutOfRangeException(nameof(type), "객체 수신의 현재 TYPE 물리 범위");
    /// <summary>현재 실제 슬롯의 TYPE 디버그 이름만 선택한다.</summary>
    private string SquidPacketTypeName(int sid) => SquidPacketType(Pool.Slot(sid)[10]).Name;
    /// <summary>48f110→42b300의 같은 현재 Core 장부와 진단 뒤 현재 techBitIndex를 읽는다.</summary>
    private bool SquidPacketHasTechnology(uint owner, uint type)
    {
        if (owner == 0 || SquidPacketType(type).TechBitIndex == -1) return true;
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
        if (owner > 8) storage.AssertDisplay("playerId > INVALID_PLAYER_ID && playerId < MAX_PLAYERS", @"\Ns\O\Player.cpp", 0x68);
        int index = SquidPacketType(type).TechBitIndex;
        if (index < 0 || index >= 32) storage.AssertDisplay("techBitIndex >= 0 && techBitIndex < 32", @"\Ns\O\Ch.cpp", 0x164);
        byte[] records = Gameplay.CoreData.Records(); ulong offset = (ulong)owner * 16 + 10;
        if (offset + 4 > (ulong)records.Length) throw new ArgumentOutOfRangeException(nameof(owner), "객체 수신의 실제 Core 장부 물리 범위");
        return (BinaryPrimitives.ReadUInt32LittleEndian(records.AsSpan((int)offset, 4)) & (1u << (index & 31))) != 0;
    }
    /// <summary>Player 이름 뷰를 먼저 선택하고 현재 설명 번역/치환 뒤 같은 현재 이름으로256BYTE Tell을 작성한다.</summary>
    private void SquidPacketNoMake(int sid)
    {
        byte owner = Pool.Slot(sid)[34]; if (owner > 8) throw new ArgumentOutOfRangeException(nameof(sid), "PlayerNoMake의 실제 Player 범위");
        Memory<byte> playerName = PlayerRecords.AsMemory(owner * 180 + 12, 32);
        if (playerName.Span.IndexOf((byte)0) < 0) throw new ArgumentException("PlayerNoMake의 현재 이름 NUL 누락");
        byte[] description = FixedPacketNeed(_squidPacketNames).BaseName(sid); int zero = playerName.Span.IndexOf((byte)0);
        if (zero < 0) throw new ArgumentException("PlayerNoMake의 설명 효과 뒤 현재 이름 NUL 누락");
        byte[] message = "PlayerNoMake("u8.ToArray().Concat(playerName.Span[..zero].ToArray()).Concat(","u8.ToArray()).Concat(description).Concat(")"u8.ToArray()).ToArray();
        if (message.Length >= 256) throw new ArgumentOutOfRangeException(nameof(sid), "PlayerNoMake의 원문256BYTE 범위");
        FixedPacketNeed(_squidPacketEffects?.Tell)(message);
    }
    /// <summary>안내 콜백 뒤 현재 owner의 Player 표식만400으로 갱신한다.</summary>
    private void SquidPacketMarkNoMake(int sid)
    {
        byte owner = Pool.Slot(sid)[34]; if (owner > 8) throw new ArgumentOutOfRangeException(nameof(sid), "PlayerNoMake의 현재 Player 표식 범위");
        Span<byte> word = PlayerRecords.AsSpan(owner * 180 + 0x54, 4); BinaryPrimitives.WriteUInt32LittleEndian(word, BinaryPrimitives.ReadUInt32LittleEndian(word) | 0x400);
    }
    /// <summary>같은 실제 종속 UI/공통 후처리/전송을 사용하며 별도 World나 구 Mission 세션을 만들지 않는다.</summary>
    private void AttachSquidPacketBaseAsset(int sid, int parent, uint flags, RawSurfaceStorage storage)
    {
        RawSquidBaseAttach.Attach(Pool, sid, parent, flags, new()
        {
            Assertion = storage.AssertDisplay, Genus = type => SquidPacketType(type).Flags2, ContentType = () => Gameplay.ContentFormTypeBox.Value,
            NotifyUi = selected => FixedPacketNeed(_worldContentDetach ?? _squidPacketUi).NotifyUi(selected), PostPop = () => FixedPacketNeed(_livePostPop),
            Transmit = (selected, value) => FixedPacketNeed(_worldTransmission).Pop(selected, value),
        });
        storage.NoteChanged();
    }
    /// <summary>433c30의 같은 현재 공장/가상/기술/장부/공간과 세 장기 상자를 조립해 직접 실행한다.</summary>
    private void ReceiveSquidPacket(ReadOnlyMemory<byte> packet)
    {
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
        HostSquidPacketEffects effects = FixedPacketNeed(_squidPacketEffects);
        var hooks = new SquidReceiveHooks
        {
            Assertion = storage.AssertDisplay, Take = (type, sid) => FixedPacketNeed(_fortFactory).Take(type, sid),
            Word = (sid, word) => RawSquidWord.Set(Pool, sid, word, () => BridgeState.BridgeType, selected => FixedPacketNeed(_fortBuilding).NotifySurface(selected)),
            Owner = SetWorldObjectOwner, Flags1 = type => SquidPacketType(type).Flags1,
            FirstPop = sid =>
            {
                if (!RawSquidFactory.SupportsConstructedVtable(Pool.Edition, BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid))))
                    throw new InvalidOperationException("객체 수신의 현재 가상30 미연결");
                return FixedPacketNeed(_fortFactory).FirstPopFlags(sid);
            },
            HasTechnology = SquidPacketHasTechnology, NoMake = SquidPacketNoMake, MarkNoMake = SquidPacketMarkNoMake,
            Pop = PopWorldObject, ContainedPop = (sid, parent, flags) => AttachSquidPacketBaseAsset(sid, parent, flags, storage),
            TypeName = SquidPacketTypeName, Log = effects.Log,
        };
        RawSquidReceive.Receive(Pool, packet, new(GameAux.VerifySidBox, GameAux.SupportTypeWasBox, LocalState.PlayerNoMakeWarnings), hooks);
    }
    /// <summary>저장된 네 고정 entry의 실제 객체/압축/검증 본체다. sender/현재 packet종류/등록표로 다시 선택하지 않는다.</summary>
    public void ReceiveFixedGameplayPacket(uint entry, ReadOnlyMemory<byte> packet)
    {
        if (_playerRecordsReleased || ProcessBindingsReturned() || _playerProcessRuntime is null) throw new ObjectDisposedException(nameof(HostSession), "고정 gameplay의 실제 World 반납 뒤 호출");
        switch (entry)
        {
            case 0x433c30: ReceiveSquidPacket(packet); return;
            case 0x44a280: ReceiveDaisPacket(packet); return;
            case 0x497030: _ = PlayerProcesses.ReceiveSharedRegular(packet.Span); return;
            case 0x4a71c0: ReceivePositionPacket(packet); return;
            case 0x4333f0:
                float y = BinaryPrimitives.ReadSingleLittleEndian(packet.Span.Slice(8, 4)), x = BinaryPrimitives.ReadSingleLittleEndian(packet.Span.Slice(4, 4));
                var size = FixedPacketNeed(_squidPacketEffects?.ScreenSize)(); Gameplay.Camera.RequestCenter(x, y, size.Width, size.Height, LocalState.MetaRebuild); return;
            case 0x4cf980: RawRemoteTellPacket.Receive(packet, text => FixedPacketNeed(_squidPacketEffects?.Tell)(text)); return;
            case 0x434080:
                RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
                byte[] image = RawSquidCompressedReceive.Decode(packet, GameAux.SquidTransmission, new()
                {
                    Assertion = storage.AssertDisplay, DefaultType = () => BridgeState.BridgeType, Type = SquidPacketType,
                    MaximumHp = selected => FixedPacketNeed(_fortFactory).ReadSelectedHitPoints(selected, FixedPacketNeed(_terrainPopTypes)),
                });
                ReceiveSquidPacket(image); return;
            case 0x432710:
            case 0x4326e0:
                var hooks = new SquidVerificationHooks { Assertion = FixedPacketNeed(_squidPacketStorage).AssertDisplay,
                    Log = FixedPacketNeed(_squidPacketEffects).Log, TypeName = SquidPacketTypeName };
                var state = new SquidVerificationState(GameAux.VerifySidBox, GameAux.PreviousVerifySidBox);
                if (entry == 0x432710) RawSquidVerification.ReceiveFree(Pool, packet, state, hooks); else RawSquidVerification.Begin(Pool, packet, state, hooks);
                return;
            default: throw new ArgumentOutOfRangeException(nameof(entry), "나머지 고정 gameplay 실제 본문 미연결");
        }
    }
}
