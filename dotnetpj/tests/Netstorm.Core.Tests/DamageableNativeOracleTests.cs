using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제Native전처리의현재TYPE/권한·선택해방·지도감김과부분효과를검증한다.
namespace Netstorm.Core.Tests;

/// <summary>원본자료재생외에현행Native/장기owner경계의합성계약을확인한다.</summary>
public sealed class DamageableNativeOracleTests
{
    /// <summary>이검사의부모/후보·현재장기전역과효과를보관한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제client목록을먼저확보하는동일풀이다.</summary>
        public RawSidPool Pool { get; } = new(24000, false);
        /// <summary>Native가실제로도달한flatBYTE를읽는지도다.</summary>
        public byte[] Spots { get; } = new byte[65536];
        /// <summary>현재권한/내용물TYPE/진단을공유하는상태다.</summary>
        public ContainedFinderState Finder { get; } = new() { Boss = false };
        /// <summary>독립999와다른장기PriestType을연결하는전처리상태다.</summary>
        public DamageablePreDestroyState State { get; } = new() { PriestType = 999 };
        /// <summary>실제Client의장기현재사제TYPE owner다.</summary>
        public RawGameAuxState Aux { get; } = new();
        /// <summary>순서와부모이후부분효과를관찰하는기록이다.</summary>
        public List<string> Events { get; } = [];
        /// <summary>구성후대리자를바꿔도원문함수사본이유지되는지검사할입력이다.</summary>
        public DamageablePreDestroyHooks Hooks { get; }
        /// <summary>각효과에서현재입력을바꾸는선택적원본대체경계다.</summary>
        public Action<string>? Change { get; set; }
        /// <summary>같은실제부모와두후보를먼저할당하고표/훅만준비한다.</summary>
        public Scene()
        {
            // 합성raw준비는실제FIFO할당을끝낸뒤수행한다.
            for (int sid = 5; sid <= 7; sid++) Pool.Allocate(SidAllocation.Client);
            Pool.RawSlot(5).Clear(); Pool.RawSlot(5)[10] = 74; Position(1, 1); State.SharedPriestType = Aux.PriestTypeBox;
            Hooks = new() { Collapse = _ => Record("C"), Explosion = _ => Record("X"), Sound = sound => Record(sound.Sound == DamageableSound.PriestFree ? "F" : "A"), ReleaseContained = _ => Record("R"), ValidateBase = (_, _) => Record("V"), Base = (_, _) => Record("B") };
        }
        /// <summary>관측효과를저장한뒤현재입력변경을공급한다.</summary>
        private void Record(string value) { Events.Add(value); Change?.Invoke(value); }
        /// <summary>부모의현재원문float좌표를쓴다.</summary>
        public void Position(float x, float y) { BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(5)[14..], BitConverter.SingleToUInt32Bits(x)); BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(5)[18..], BitConverter.SingleToUInt32Bits(y)); }
        /// <summary>원문종속후보의타입/링크/종류WORD만만든다.</summary>
        public void Candidate(int sid, ushort next, ushort kind) { Pool.RawSlot(sid).Clear(); Pool.RawSlot(sid)[10] = 6; BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(sid)[4..], next); BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(sid)[18..], kind); BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(sid)[20..], 1); }
        /// <summary>실제같은풀/지면/장기전역으로직접전처리를구성한다.</summary>
        public RawDamageablePreDestroy Body() => new(Pool, Spots, Finder, State, Hooks);
    }
    /// <summary>좌표별지도자격대신원문의감긴Y*256+X가도달한한BYTE를사용한다.</summary>
    [Theory]
    [InlineData(300f, -2f, 44)]
    [InlineData(-2f, 1f, 255)]
    [InlineData(9f, 16777216f, 9)]
    public void NativeUsesWrappedFlatByte(float x, float y, int index)
    {
        var s = new Scene(); s.Position(x, y); s.Spots[index] = 6; s.Candidate(6, 0, 158); BinaryPrimitives.WriteUInt16LittleEndian(s.Pool.RawSlot(5)[6..], 6); var body = s.Body(); Assert.Throws<ArgumentOutOfRangeException>(() => body.PreDestroy(5, 0)); Assert.Empty(s.Events); body.PreDestroyNative(5, 0, null); Assert.Equal(["F", "B"], s.Events);
    }
    /// <summary>Native의사전검사생략과효과뒤지도실패의부분효과·extra한번분기를확인한다.</summary>
    [Fact]
    public void NativePreservesEffectsBeforeFlatMapFailureAndSkipsUnusedReads()
    {
        var s = new Scene(); s.Position(65536, 0); var body = s.Body(); body.EnableNativeBody(); body.Validate(-1, 0); Assert.Throws<ArgumentOutOfRangeException>(() => body.PreDestroy(5, 0x200000)); Assert.Equal(["C", "A"], s.Events); s.Events.Clear(); s.Pool.RawSlot(5)[11] = 9; s.Pool.RawSlot(5)[10] = 0; s.Pool.RawSlot(5)[40] = 9; s.Position(float.NaN, float.PositiveInfinity); s.Finder.SharedBoss = () => throw new InvalidOperationException("extra생략된권한미조회"); body.PreDestroy(5, 0x300000); Assert.Equal(["B"], s.Events);
        var cd = new RawDamageablePreDestroy(new RawSidPool(24000, false, edition: OriginalEdition.Cd1072), new byte[65536], new(), new(), s.Hooks); Assert.Throws<ArgumentException>(cd.EnableNativeBody);
    }
    /// <summary>소리뒤현재PriestType의전체DWORD와저장한next를읽으며독립999는보존한다.</summary>
    [Theory]
    [InlineData(159u, 2)]
    [InlineData(0x1000009fu, 1)]
    public void NativeRequeriesSharedPriestTypeAfterSound(uint nextType, int expectedSounds)
    {
        var s = new Scene(); s.Candidate(6, 7, 158); s.Candidate(7, 0, 159); BinaryPrimitives.WriteUInt16LittleEndian(s.Pool.RawSlot(5)[6..], 6); s.Spots[257] = 6; s.Change = effect => { if (effect == "F") { s.Aux.PriestTypeBox.Value = nextType; BinaryPrimitives.WriteUInt16LittleEndian(s.Pool.RawSlot(5)[6..], 0); BinaryPrimitives.WriteUInt16LittleEndian(s.Pool.RawSlot(6)[4..], 0); } }; var body = s.Body(); body.EnableNativeBody(); body.PreDestroy(5, 0); Assert.Equal(expectedSounds, s.Events.Count(value => value == "F")); Assert.Equal("B", s.Events[^1]); Assert.Equal(999u, s.State.PriestType);
    }
    /// <summary>해방대리자는현재boss/flags뒤에만요구하고직접Native의대체함수를선택한다.</summary>
    [Fact]
    public void NativeOnlyRequiresSelectedReleaseAndUsesCurrentBoss()
    {
        var s = new Scene(); bool boss = false; s.Finder.Boss = true; s.Finder.SharedBoss = () => boss; var body = s.Body(); s.Hooks.ReleaseContained = null; body.PreDestroyNative(5, 0, null); Assert.Equal(["B"], s.Events); s.Events.Clear(); boss = true; body.PreDestroyNative(5, 0x800, null); Assert.Equal(["B"], s.Events); s.Events.Clear(); Assert.Throws<InvalidOperationException>(() => body.PreDestroyNative(5, 0, null)); Assert.Empty(s.Events); body.PreDestroyNative(5, 0, _ => s.Events.Add("O")); Assert.Equal(["O", "B"], s.Events); s.Events.Clear(); body.EnableNativeBody(); body.PreDestroy(5, 0); Assert.Equal(["R", "B"], s.Events); s.Events.Clear(); s.Change = effect => { if (effect == "A") boss = false; }; body.PreDestroy(5, 0x200000); Assert.Equal(["C", "A", "B"], s.Events);
    }
    /// <summary>실제Carrier와두Native부모를같은풀로연결해typed검증이원문효과앞에끼지않게한다.</summary>
    [Fact]
    public void ActualCarrierAdapterUsesNativeDamageableParent()
    {
        var s = new Scene(); s.Pool.RawSlot(5)[10] = 0; s.Pool.RawSlot(5)[40] = 9; BinaryPrimitives.WriteUInt16LittleEndian(s.Pool.RawSlot(5)[6..], 0xffff); var damageable = s.Body(); damageable.EnableNativeBody(); var hooks = new CarrierPreDestroyHooks { DependentFound = () => throw new InvalidOperationException("bossfalse종속미조회") }; var attached = RawDamageablePreDestroy.MakeDamageablePreDestroyHooks(s.Pool, damageable, hooks); Assert.Null(hooks.DamageablePre); Assert.Same(hooks.DependentFound, attached.DependentFound); var carrier = new RawCarrierPreDestroy(s.Pool, s.Finder, attached); carrier.EnableNativeBody(); carrier.PreDestroy(5, 0); Assert.Equal(["B"], s.Events); Assert.Throws<ArgumentException>(() => RawDamageablePreDestroy.MakeDamageablePreDestroyHooks(new RawSidPool(24000, false), damageable, hooks));
    }
}
