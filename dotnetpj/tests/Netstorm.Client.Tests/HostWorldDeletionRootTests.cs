using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// MetaZone 마커 삭제가 세션 Root 전체 분배를 타는지 검증한다.
// 2026-11-26 1.036에서 WP-3 전체 Native World 자산 가상 삭제 분배용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>Native 장기 장치와 Root 전체 분배 설치를 같은 세션에서 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>제품 흐름과 같은 순서로 사제·패킷 장치를 연결한 세션을 만든다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface) WorldDeletionRootScene(bool sharedNative = false)
    {
        // 전체 Native 자산 가상 삭제는 Native 전송·선택 계약을 요구한다. typed 경로의 DefaultProcessAssetHooks는 거부한다.
        // 근거: RawSurfaceStorage.EmitDefaultAssetEffect의 UsesNativeTransmissionDevices 관문과
        // CurrentProcessSelection의 선택 장치 관문이다. 설치 순서(전송·선택→Host)는 제품과 같다.
        var game = new ClientGameplayGlobals();
        game.Authority.Server = true;
        var local = new ClientLocalStateGlobals(game, new());
        var clock = new ClientClockGlobals(local);
        var kernel = new Kernel();
        var templates = sharedNative ? new CampaignTemplateStorage() : null;
        var sp = sharedNative ? new ClientSpGlobals(new GameRandom(17), new UIntBox { Value = 1 }, new UIntBox(), () => 1760000000) : null;
        sp?.Store.Initialize();
        var surface = new ClientSurfaceGlobals(game, clock, kernel, () => 0, installDevices: owner =>
        {
            if (templates is not null) owner.Storage.BindSerializationGlobals(templates.SerializationGlobals);
            if (templates is not null) { owner.Storage.EnablePatchDestroyDiagnostics(); owner.Storage.BindGraphTrace((_, _) => { }); }
            owner.Storage.BindTransmissionDevices(new RawSurfaceTransmissionDevices { Broadcast = _ => { }, Connected = _ => false, Send = (_, _) => { } });
            if (templates is not null) owner.Storage.BindGumpType(game.GumpFormTypeBox);
            owner.Storage.BindProcessDevices(new RawSurfaceProcessDevices { Selected = () => 0, ClearSelection = () => { } });
            if (templates is not null) owner.Storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
            if (templates is not null)
            {
                owner.Storage.BindRewardDevices(new() { Sp = sp!.Store, Editor = game.World.EditorBox, RefreshSp = () => { }, CreditWallet = (_, _) => { } });
                owner.Storage.BindContentDevices(new() { Cursor = game.Cursor.PreviewBox, LocalOwner = game.Player.BattleOwnerBox, ContentType = game.ContentFormTypeBox,
                    Effects = new() { CancelCursor = () => game.Cursor.Preview = 0, LocalNotice = _ => { }, RemoteNotice = _ => { }, Sound = (_, _) => { } } });
            }
        });
        var (session, types) = Scene(surface, initializeSurfaceAssets: true, sharedTemplates: templates);
        session.BuildPriestWorldDevices();
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) => throw new InvalidOperationException($"객체 패킷 진단:{text} {file}:{line:x}"));
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { FormPacket = _ => { } });
        return (session, types, surface);
    }

    /// <summary>좌표·타입·소유자·가상표를 직접 채운 원시 슬롯을 만든다.</summary>
    private static int MakeWorldDeletionSlot(HostSession session, uint type, float x, float y, uint owner, uint vtable)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, vtable);
        raw[RawSidPool.TypeOffset] = unchecked((byte)type);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(14, 4), x);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(18, 4), y);
        raw[34] = unchecked((byte)owner);
        return sid;
    }

    /// <summary>Root 전체 분배를 설치하고 중복·미설치를 거부한다.</summary>
    [Fact]
    public void WorldDeletionRootInstallsFullDispatch()
    {
        var (session, _, _) = WorldDeletionRootScene();
        RawSquidFormDispatch dispatch = session.BuildWorldDeletionRootDevices();
        Assert.NotNull(dispatch);
        Assert.Throws<InvalidOperationException>(() => session.BuildWorldDeletionRootDevices());
        session.ReleasePlayerProcesses();
        var bare = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => bare.BuildWorldDeletionRootDevices());
        Assert.Throws<InvalidOperationException>(() => bare.DestroyResourceObject(1, 0));
        bare.ReleasePlayerRecordViews();
    }

    /// <summary>세션 Root 전체 분배가 MetaZone과 같은 실제 Form 가상 삭제를 수행한다.</summary>
    [Fact]
    public void WorldDeletionRootDestroysFormThroughFullDispatch()
    {
        var (session, _, _) = WorldDeletionRootScene();
        RawSquidFormDispatch dispatch = session.BuildWorldDeletionRootDevices();
        Assert.NotNull(dispatch);
        int parent = session.BuildPlayerWorldHooks().Create!(152, 2);
        int calls = 0;
        var observed = new ObservedWorldDeletionProcess(_ => calls++);
        Assert.Same(observed, session.PlayerProcesses.Attach(observed, 10, parent, 0x50));
        int form = observed.Form();
        Assert.Equal(0, session.Pool.Slot(form)[11] & RawSidPool.VoidBit);
        // MetaZone 마커 삭제와 같은 실제 전체 분배 경로로 지운다. 일반 Pre/Post로 대신하지 않는다.
        session.DestroyWorldObject(form, 0);
        Assert.Equal(1, calls);
        Assert.NotEqual(0, session.Pool.Slot(form)[11] & 1);
        int secondParent = session.BuildPlayerWorldHooks().Create!(152, 2);
        var second = new ObservedWorldDeletionProcess(_ => calls++);
        Assert.Same(second, session.PlayerProcesses.Attach(second, 11, secondParent, 0x50));
        int secondForm = second.Form();
        Assert.True(session.DestroyResourceObject(secondForm, 0));
        Assert.Equal(2, calls); Assert.NotEqual(0, session.Pool.Slot(secondForm)[11] & 1);
        session.ReleasePlayerProcesses();
    }

    /// <summary>제품처럼 장기 Surface를 공유하면 같은 삭제 깊이의 GumpForm도 실제 분배를 통과한다.</summary>
    [Fact]
    public void WorldDeletionRootSharesNativeGumpFormAndDestroyDepth()
    {
        var (session, _, surface) = WorldDeletionRootScene(sharedNative: true);
        Assert.Same(surface.Storage.BaseDestroy(), session.PlayerProcesses.Destroy());
        Assert.True(surface.Storage.BaseDestroy().HasPatchDiagnostics);
        session.BuildWorldDeletionRootDevices(); var forms = surface.Storage.GumpForms();
        surface.Storage.Types[4].ConstructorAddress = OriginalConstructors.Address(3);
        session.Gameplay.GumpFormTypeBox.Value = 4;
        int form = forms.Create(); Assert.True(forms.IsGumpForm(form)); Assert.Equal(4, session.Pool.Slot(form)[10]);
        Assert.True(session.DestroyResourceObject(form, 0)); Assert.NotEqual(0, session.Pool.Slot(form)[11] & 1);
        int content = surface.Storage.FactoryExecutor().CreateContained(2); session.Gameplay.Cursor.Preview = (uint)content;
        Assert.True(session.DestroyResourceObject(content, 0)); Assert.Equal(0u, session.Gameplay.Cursor.Preview); Assert.NotEqual(0, session.Pool.Slot(content)[11] & 1);
        Assert.Equal(0u, surface.Storage.BaseDestroy().PreDepth); Assert.Equal(0u, surface.Storage.BaseDestroy().PostDepth);
        session.ReleasePlayerProcesses();
    }

    /// <summary>실제 Form 가상 통지만 관찰하는 프로세스다.</summary>
    private sealed class ObservedWorldDeletionProcess(Action<uint> destroyed) : SquidProcess
    {
        /// <summary>실제 가상 소멸 통지의 관측 장치다.</summary>
        private readonly Action<uint> _destroyed = destroyed;

        /// <summary>풀 반납은 실제 Host에 맡기고 원문 가상 통지 시점만 기록한다.</summary>
        /// <param name="flags">삭제 플래그다.</param>
        public override void OnFormDestroy(uint flags) => _destroyed(flags);
    }

    /// <summary>원문 83개 가상표 밖에 있는 자산은 명시 거부한다.</summary>
    [Fact]
    public void WorldDeletionRootRejectsUnknownVtable()
    {
        var (session, _, _) = WorldDeletionRootScene();
        RawSquidFormDispatch dispatch = session.BuildWorldDeletionRootDevices();
        Assert.NotNull(dispatch);
        // 타입은 자산 범위(158)이지만 가상표는 원문 83개 어디에도 없다.
        int sid = MakeWorldDeletionSlot(session, 158, 50, 60, 1, 0xDEADBEEFu);
        var failure = Assert.Throws<InvalidOperationException>(() => session.DestroyWorldObject(sid, 0));
        Assert.Contains("미복원 가상표", failure.Message);
        Assert.Contains("미복원 가상표", Assert.Throws<InvalidOperationException>(() => session.DestroyResourceObject(sid, 0)).Message);
        session.ReleasePlayerProcesses();
    }
}
