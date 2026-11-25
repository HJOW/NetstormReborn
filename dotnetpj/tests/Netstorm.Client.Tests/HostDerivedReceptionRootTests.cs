using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 같은 Host의 파생 수신·변경·원격 삭제가 전체 가상 삭제 분배를 타는지 검증한다.
// 2026-11-26 1.037에서 WP-4a 파생 수신 결선용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>파생 수신·원격 삭제와 Root 전체 분배 설치를 같은 세션에서 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>제품 흐름과 같은 순서로 Native 전송·사제·패킷·전체 삭제 장치를 연결한 세션을 만든다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface) DerivedReceptionRootScene(bool withVirtualDestroy)
    {
        // 전체 Native 파생 수신은 Native 전송·선택 계약을 요구한다. typed 경로의 기본 자산 훅은 거부한다.
        // 근거: RawSurfaceStorage.EmitDefaultAssetEffect의 UsesNativeTransmissionDevices 관문이다.
        var game = new ClientGameplayGlobals();
        game.Authority.Server = true;
        var local = new ClientLocalStateGlobals(game, new());
        var clock = new ClientClockGlobals(local);
        var kernel = new Kernel();
        var surface = new ClientSurfaceGlobals(game, clock, kernel, () => 0, installDevices: owner =>
        {
            owner.Storage.BindTransmissionDevices(new RawSurfaceTransmissionDevices { Broadcast = _ => { }, Connected = _ => false, Send = (_, _) => { } });
            owner.Storage.BindProcessDevices(new RawSurfaceProcessDevices { Selected = () => 0, ClearSelection = () => { } });
        });
        var (session, types) = Scene(surface, initializeSurfaceAssets: true);
        session.BuildPriestWorldDevices();
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) => throw new InvalidOperationException($"객체 패킷 진단:{text} {file}:{line:x}"));
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        var effects = new HostSquidPacketEffects
        {
            Log = (_, _) => { },
            FormPacket = _ => { },
        };
        if (withVirtualDestroy)
        {
            // 제품 진입 경로와 같은 지연 조회로 같은 Host의 전체 가상 삭제를 잇는다.
            // 근거: ClientHostGame의 BindSquidPacketDevices VirtualDestroy다.
            effects = effects with { VirtualDestroy = (sid, flags) => session.DestroyWorldObject(sid, flags) };
        }

        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), effects);
        session.BuildWorldDeletionRootDevices();
        return (session, types, surface);
    }

    /// <summary>실제 Form 가상 통지만 관찰하는 프로세스다.</summary>
    private sealed class ObservedDerivedProcess(Action<uint> destroyed) : SquidProcess
    {
        /// <summary>실제 가상 소멸 통지의 관측 장치다.</summary>
        private readonly Action<uint> _destroyed = destroyed;

        /// <summary>풀 반납은 실제 Host에 맡기고 원문 가상 통지 시점만 기록한다.</summary>
        /// <param name="flags">삭제 플래그다.</param>
        public override void OnFormDestroy(uint flags) => _destroyed(flags);
    }

    /// <summary>같은 Host에 붙인 실제 ProcessForm을 만들고 통지 관측기를 단다.</summary>
    private static int AttachObservedForm(HostSession session, Action<uint> destroyed)
    {
        int parent = session.BuildPlayerWorldHooks().Create!(152, 2);
        var observed = new ObservedDerivedProcess(destroyed);
        Assert.Same(observed, session.PlayerProcesses.Attach(observed, 10, parent, 0x50));
        return observed.Form();
    }

    /// <summary>단일 원격 삭제(53h)가 같은 Host의 전체 가상 삭제로 실제 Form을 지운다.</summary>
    [Fact]
    public void DerivedRemoteDeletedSingleDestroysThroughFullDispatch()
    {
        var (session, _, _) = DerivedReceptionRootScene(withVirtualDestroy: true);
        int calls = 0;
        int form = AttachObservedForm(session, _ => calls++);
        var packet = new byte[6];
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), unchecked((ushort)form));
        session.ReceiveFixedGameplayPacket(0x4436b0, packet);
        Assert.Equal(1, calls);
        Assert.NotEqual(0, session.Pool.Slot(form)[11] & 1);
        session.ReleasePlayerProcesses();
    }

    /// <summary>다중 원격 삭제(62h)가 같은 Host의 전체 가상 삭제로 두 실제 Form을 지운다.</summary>
    [Fact]
    public void DerivedRemoteDeletedManyDestroysThroughFullDispatch()
    {
        var (session, _, _) = DerivedReceptionRootScene(withVirtualDestroy: true);
        int calls = 0;
        int first = AttachObservedForm(session, _ => calls++);
        int second = AttachObservedForm(session, _ => calls++);
        var packet = new byte[12];
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), 2u);
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(8, 2), unchecked((ushort)first));
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(10, 2), unchecked((ushort)second));
        session.ReceiveFixedGameplayPacket(0x433120, packet);
        Assert.Equal(2, calls);
        Assert.NotEqual(0, session.Pool.Slot(first)[11] & 1);
        Assert.NotEqual(0, session.Pool.Slot(second)[11] & 1);
        session.ReleasePlayerProcesses();
    }

    /// <summary>변경 삭제 수신(2eh)이 같은 Host의 전체 가상 삭제로 실제 Form을 지운다.</summary>
    [Fact]
    public void DerivedMutationDeletedDestroysThroughFullDispatch()
    {
        var (session, _, _) = DerivedReceptionRootScene(withVirtualDestroy: true);
        session.Gameplay.Authority.Boss = false;
        int calls = 0;
        int form = AttachObservedForm(session, _ => calls++);
        var packet = new byte[8];
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), unchecked((ushort)form));
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), 0);
        session.ReceiveFixedGameplayPacket(0x433170, packet);
        Assert.Equal(1, calls);
        Assert.NotEqual(0, session.Pool.Slot(form)[11] & 1);
        session.ReleasePlayerProcesses();
    }

    /// <summary>전체 가상 삭제 미연결은 파생 수신에서 명시 실패하고 슬롯을 보존한다.</summary>
    [Fact]
    public void DerivedReceptionWithoutVirtualDestroyFailsExplicitly()
    {
        var (session, _, _) = DerivedReceptionRootScene(withVirtualDestroy: false);
        int calls = 0;
        int form = AttachObservedForm(session, _ => calls++);
        var packet = new byte[6];
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), unchecked((ushort)form));
        var failure = Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x4436b0, packet));
        Assert.Contains("미연결", failure.Message);
        Assert.Equal(0, calls);
        Assert.Equal(0, session.Pool.Slot(form)[11] & 1);
        session.ReleasePlayerProcesses();
    }
}
