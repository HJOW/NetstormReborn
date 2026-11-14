using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 제품 호스트와 미션이 공유하는 전역의 초기화·대여 계약을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 장기 SP/게임 전역을 조립하여 숨은 난수 소비와 값 사본 회귀를 검출한다.</summary>
public sealed class ClientSharedGlobalsTests
{
    /// <summary>생성은 주소만 연결하며 실제 공통 시작에서 초기화할 때만 시각·난수를 소비한다.</summary>
    [Fact]
    public void SpConstructionOnlyBindsAddressAndExplicitInitializeMatchesOriginalStore()
    {
        var random = new GameRandom(123); var crt = new UIntBox { Value = 1 }; var mode = new UIntBox();
        int reads = 0;
        var globals = new ClientSpGlobals(random, crt, mode, () => { reads++; return 1760000000; });
        Assert.Same(random, globals.Random); Assert.Same(crt, globals.CrtState);
        Assert.Equal(0, reads); Assert.Equal(123u, random.State()); Assert.Equal(1u, crt.Value);
        Assert.Null(globals.Store.FirstPoolCurrent); Assert.Equal(0, globals.Store.TrackingOffsetCurrent.Value);
        var expectedRandom = new GameRandom(123); var expectedCrt = new UIntBox { Value = 1 };
        var seed = new RawGameRandomSeed(expectedRandom, expectedCrt, new UIntBox(), () => 1760000000);
        var expected = new RawSpStore(expectedRandom, seed.Reseed); expected.Initialize();
        globals.Store.Initialize();
        Assert.Equal(1, reads); Assert.Equal(expectedRandom.State(), random.State()); Assert.Equal(expectedCrt.Value, crt.Value);
        Assert.Equal(expected.OffsetCurrent, globals.Store.OffsetCurrent);
        Assert.Equal(unchecked((int)expected.OffsetCurrent), globals.Store.TrackingOffsetCurrent.Value);
        Assert.Equal(expected.Pools.ToArray(), globals.Store.Pools.ToArray());
        Assert.Equal(expected.Permutation.ToArray(), globals.Store.Permutation.ToArray());
    }

    /// <summary>두 미션 생성과 반납 사이에 같은 실제 배열·추적 주소·현재 TYPE/진단을 보존한다.</summary>
    [Fact]
    public void SessionsBorrowSameLongLivedOwnersWithoutResettingOrReseeding()
    {
        var random = new GameRandom(1); int reads = 0;
        var sp = new ClientSpGlobals(random, new UIntBox { Value = 1 }, new UIntBox(), () => { reads++; return 1760000000; });
        sp.Store.Initialize(); var gameplay = new ClientGameplayGlobals();
        var first = new HostSession(new Kernel(), random, new CrtRandom(), sharedSp: sp.Store, sharedGameplay: gameplay);
        Assert.Same(gameplay.GameAux, first.GameAux);
        Assert.Same(gameplay.GameAux.PriestTypeBox, first.PriestState.Regen.SharedPriestType);
        Assert.Same(gameplay.GameAux.PriestTypeBox, first.PriestState.Movement.PriestBox);
        Assert.Equal(6u, gameplay.PatchSample.Value); Assert.Equal(6u, gameplay.PatchDiagnostic.Value);
        var lease = RawPriestRegen.BindPriestRegenSpStore(first.Sp);
        var address = sp.Store.TrackingOffsetCurrent; var pool = sp.Store.FirstPoolCurrent;
        address.Value = 9599; gameplay.PatchSample.Value = 4; gameplay.PatchDiagnostic.Value = 3;
        gameplay.GameAux.PriestTypeBox.Value = 167; gameplay.Authority.Boss = false;
        uint state = random.State(); uint offset = sp.Store.OffsetCurrent;
        first.ReleasePlayerProcesses();
        var next = new HostSession(new Kernel(), random, new CrtRandom(), sharedSp: sp.Store, sharedGameplay: gameplay);
        Assert.Same(sp.Store, next.Sp); Assert.Same(pool, next.Sp.FirstPoolCurrent);
        Assert.Same(address, RawPriestRegen.BindPriestRegenSpStore(next.Sp).TrackingSequence!());
        Assert.Same(address, lease.TrackingSequence!()); Assert.Equal(9599, address.Value);
        Assert.Equal(167u, next.PriestState.Regen.ReadPriestType()); Assert.Equal(167u, next.PriestState.Movement.Priest);
        Assert.Equal(4u, next.PriestState.Regen.Patch.ReadSample());
        next.PriestState.Regen.Patch.WriteChangedSample(8); Assert.Equal(8u, gameplay.PatchDiagnostic.Value);
        Assert.False(next.Gameplay.Authority.ReadBoss()); Assert.Equal(offset, sp.Store.OffsetCurrent);
        Assert.Equal(state, random.State()); Assert.Equal(1, reads);
        // 사제 fixture 전용 값형 sequence를 현재 Client 추적 주소로 덮어쓰지 않는다.
        Assert.Equal(0, next.PriestState.Regen.Patch.Sequence);
    }

    /// <summary>회전은 현재 offset만 전진시키고 추적 주소·SP 비트·첫 실제 배열을 보존한다.</summary>
    [Fact]
    public void RotationAndCurrentReadsPreserveTrackingAndResourceBits()
    {
        var globals = new ClientSpGlobals(new GameRandom(1), new UIntBox { Value = 1 }, new UIntBox(), () => 1760000000);
        globals.Store.Initialize(); globals.Store.Set(1, 6500); globals.Store.Set(8, 12345);
        var reads = RawPriestRegen.BindPriestRegenSpStore(globals.Store);
        var tracking = reads.TrackingSequence!(); var first = reads.FirstPool!(); int previous = reads.ObservedSequence!();
        tracking.Value = 9600; globals.Store.Rotate();
        Assert.NotEqual(previous, reads.ObservedSequence!()); Assert.Equal(9600, tracking.Value);
        Assert.Same(first, reads.FirstPool!()); Assert.Equal(globals.Store.Permutation[0], reads.FirstPermutation!());
        Assert.Equal(6500u, globals.Store.Get(1)); Assert.Equal(12345u, globals.Store.Get(8));
    }
}
