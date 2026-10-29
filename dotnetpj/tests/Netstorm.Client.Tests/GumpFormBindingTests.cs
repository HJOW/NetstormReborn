using System.Buffers.Binary;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 메뉴 객체 연결과 공용 SID/Form/자식 우선 지연 수명을 결합한다.

/// <summary>기존 검증된10.78 Form 몸체와 실제 UI의 연결 검사.CD Form의 추가 동작 대조나 멀티플레이 검사는 아니다.</summary>
public sealed class GumpFormBindingTests
{
    /// <summary>같은 실제 풀/팩토리/삭제/메뉴를 소유하는 입력 장면.</summary>
    private sealed class Scene
    {
        /// <summary>실제 메뉴와 기본 트리.</summary>
        public MenuWidgetScene Ui { get; } = new();
        /// <summary>실제 공용 raw SID 풀.</summary>
        public RawSidPool Pool { get; } = new(32768, true);
        /// <summary>실제 공간 해시.</summary>
        public RawSquidHash Hash { get; } = new();
        /// <summary>같은188타입 입력.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>같은 자산 Unpop.</summary>
        public RawUnpop Unpop { get; }
        /// <summary>같은 공통 삭제.</summary>
        public RawSquidDestroy Destroy { get; }
        /// <summary>같은 Form 생성기.</summary>
        public RawSquidFactory Factory { get; }
        /// <summary>같은 실제 Form 소유/부착/알림.</summary>
        public RawGumpFormHost Forms { get; }
        /// <summary>실제 가상 객체 삭제 도달 관측.</summary>
        public List<(uint Token, int Object)> Notices { get; } = [];
        /// <summary>다른 Form의 기존 삭제 도달 관측.</summary>
        public List<(uint Form, uint Flags)> OtherForms { get; } = [];
        /// <summary>기존 공통 Form과 실제 UI를 같은 소유 ID/삭제 콜백으로 연결한다.</summary>
        public Scene()
        {
            Unpop = new(Pool, Hash, new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]); Destroy = new(Pool, Unpop, Types); Factory = new(Pool, Types, false, Unpop); var fallback = new SquidDestroyHooks { Selected = () => 0, Emit = e => { if (e.Effect == SquidDestroyEffect.PreDestroy) Destroy.CompletePreDestroy(); else if (e.Effect == SquidDestroyEffect.PostDestroy) Destroy.CompletePostDestroy(); } }; var notice = GumpFormBinding.MakeGumpFormNotice(Ui.Scene.Create, (t, o) => { Notices.Add((t, o)); GumpFormBinding.ReleaseGumpObject(Ui.Scene.Lifetime, t, o); }); Forms = new(Pool, Factory, Types, Destroy, notice, fallback); GumpFormBinding.BindMenuGumpForms(Ui.Scene.Create, Ui.State, Ui.Hooks, Forms); Ui.Scene.LifetimeHooks.DeleteForm = (form, flags) => OtherForms.Add((form, flags)); GumpFormBinding.BindGumpFormLifetime(Ui.Scene.LifetimeHooks, Forms);
        }
        /// <summary>공간 해시의 실제 자산 입력을 만든다.생성/공격/승리 재현을 실행하지 않는다.</summary>
        public int Asset()
        { int sid = Pool.Allocate(); Span<byte> raw = Pool.AllocatedBytes(sid); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, OriginalConstructors.BaseVtable); raw[10] = 90; BinaryPrimitives.WriteUInt32LittleEndian(raw[14..], BitConverter.SingleToUInt32Bits(10.5f)); BinaryPrimitives.WriteUInt32LittleEndian(raw[18..], BitConverter.SingleToUInt32Bits(20.5f)); ref ushort head = ref Hash.Bucket(0, 10.5f, 20.5f); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid; return sid; }
    }
    /// <summary>controlled 객체 부착→실제 같은 풀 Form→알림/부모 삭제→UI 지연 Kill/실제 정리의 전체 연결.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void ActualObjectAttachmentAndDeferredRelease(bool deleteParent)
    {
        var s = new Scene(); s.Ui.Widgets.BeginVertical(); int parent = s.Asset(); uint id = s.Ui.Menu.Node.Id; s.Ui.Menu.Menu.AttachObjectCurrent((uint)parent, 2, 3); uint form = s.Ui.Menu.Node.Form; Assert.Equal((3U, (uint)parent, 1U, 2f, 3f, 0U), (s.Ui.State.ObjectFormType, s.Ui.Menu.Node.Object, s.Ui.Menu.Node.Parent, s.Ui.Menu.Node.X, s.Ui.Menu.Node.Y, s.Ui.Menu.State.Index)); Assert.True((s.Ui.Menu.Node.Flags & 0x40) != 0); Assert.Equal(id, s.Forms.Owner((int)form)); Assert.Equal(parent, s.Forms.Parent((int)form)); Assert.Equal((ushort)form, BinaryPrimitives.ReadUInt16LittleEndian(s.Pool.Slot(parent)[6..])); s.Ui.Widgets.Reference(40, 0); Assert.Equal(form, s.Ui.Menu.Node.Form);
        if (deleteParent) s.Destroy.Destroy(parent, 0, s.Forms.Hooks()); else s.Forms.Notify((int)form);
        Assert.Equal((40U, parent), Assert.Single(s.Notices)); Assert.Equal(1, s.Pool.Slot((int)form)[11] & 1); Assert.NotEqual(0U, s.Ui.Menu.Node.Flags & 0x10000); Assert.DoesNotContain(40U, s.Ui.Scene.Released); s.Ui.Scene.Lifetime.Cleanup(); Assert.Contains(40U, s.Ui.Scene.Released); Assert.Equal(1U, s.Ui.Scene.Create.Count); Assert.Equal(0U, s.Ui.Menu.State.Instances); Assert.Empty(s.OtherForms); if (deleteParent) Assert.Equal(1, s.Pool.Slot(parent)[11] & 1);
    }
    /// <summary>소유 알림은 생성 당시 토큰 대신 알림 시점의 실제 등록표를 읽는다.직접 소유0 조회는 루트를 허용한다.</summary>
    [Fact]
    public void NoticeReadsCurrentRegistryAndChecksOnlyReachedSlot()
    {
        var state = new GumpCreateState(); var calls = new List<(uint, int)>(); var notice = GumpFormBinding.MakeGumpFormNotice(state, (t, o) => calls.Add((t, o))); state.Entries[7] = 40; notice(7, 11); state.Entries[7] = 41; notice(7, 12); state.Entries[0] = 1; notice(0, 13); Assert.Equal(new[] { (40U, 11), (41U, 12), (1U, 13) }, calls); state.Entries[7] = 0; Assert.Throws<InvalidOperationException>(() => notice(7, 14)); Assert.Throws<ArgumentOutOfRangeException>(() => notice(1000, 15));
    }
    /// <summary>실제 풀 판본/타입/SID 표현 범위와 다른 Form의 기존 삭제 분배를 보존한다.</summary>
    [Fact]
    public void ActualPoolAndSidContractsKeepFallback()
    {
        var s = new Scene(); uint free = s.Pool.FreeCount; s.Ui.State.ObjectFormType = 4; Assert.Throws<ArgumentException>(() => s.Ui.Widgets.Reference(40, 90)); Assert.Equal(free, s.Pool.FreeCount); Assert.Throws<ArgumentOutOfRangeException>(() => s.Ui.Hooks.FormOwner!(65536, 7)); int parent = s.Asset(); s.Ui.Scene.LifetimeHooks.DeleteForm!((uint)parent, 77); Assert.Equal(((uint)parent, 77U), Assert.Single(s.OtherForms)); var cd = new MenuWidgetScene(OriginalEdition.Cd1072); Assert.Throws<ArgumentException>(() => GumpFormBinding.BindMenuGumpForms(cd.Scene.Create, cd.State, cd.Hooks, s.Forms)); Assert.Equal(0U, cd.State.ObjectFormType); Assert.Throws<ArgumentOutOfRangeException>(() => s.Ui.Scene.LifetimeHooks.DeleteForm!(65536, 0));
    }
}
