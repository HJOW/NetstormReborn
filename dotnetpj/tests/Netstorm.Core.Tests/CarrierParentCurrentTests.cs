using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// Carrier의 선택된 장치·현재 TYPE/로딩·실패 전 부분 효과를 원본 cpp 검사와 대조한다.
namespace Netstorm.Tests;

/// <summary>cpp CarrierParentCurrentTests.cpp의12개 현재 상태 계약을 재생한다.</summary>
public sealed class CarrierParentCurrentTests
{
    /// <summary>같은 물리 슬롯과 현재 상태를 처리기보다 오래 유지한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 client 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>같은 물리 원본 입력 번호다.</summary>
        public readonly int Sid;
        /// <summary>대여 시 필드만 바꾸고 재할당하지 않는 현재188TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>독립/공유 전역의 선택 상태다.</summary>
        public readonly CarrierPostPopState State = new();
        /// <summary>실제 호출한 효과의 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>check 경계가 공급하는 입력 변경이나 실패다.</summary>
        public Action? OnCheck;
        /// <summary>ground 경계가 공급하는 입력 변경이나 실패다.</summary>
        public Action? OnGround;
        /// <summary>base 경계가 공급하는 입력 변경이나 실패다.</summary>
        public Action? OnBase;
        /// <summary>마지막 실제 지면 요청 좌표다.</summary>
        public (float X, float Y) Point;
        /// <summary>마지막 실제 지면 요청 소유자다.</summary>
        public byte Owner;
        /// <summary>공통 몸체에 실제 전달한 SID다.</summary>
        public int LastSid;
        /// <summary>원문 하위 경계 관측이며 상위 분기를 대신하지 않는다.</summary>
        public readonly CarrierPostPopHooks Hooks;

        /// <summary>현재 필드와 도달한 효과 관측을 준비한다.</summary>
        public Scene()
        {
            Sid = Pool.Allocate(SidAllocation.Client); Raw().Clear(); Raw()[10] = 74; Raw()[34] = 1;
            Write(14, BitConverter.SingleToUInt32Bits(20.25f)); Write(18, BitConverter.SingleToUInt32Bits(30.5f));
            Hooks = new CarrierPostPopHooks
            {
                CarrierCheck = _ => { Events.Add("check"); OnCheck?.Invoke(); },
                ClaimGround = (x, y, owner) => { Point = (x, y); Owner = owner; Events.Add("ground"); OnGround?.Invoke(); },
                ValidateBase = (_, _) => throw new InvalidOperationException("실제 PATCH에서 도달하지 않는 사전 검사"),
                Base = (sid, _) => { LastSid = sid; Events.Add("base"); OnBase?.Invoke(); },
            };
        }
        /// <summary>free/contained 변이 뒤에도 같은 실제 물리를 읽고 쓴다.</summary>
        public Span<byte> Raw() => Pool.RawSlot(Sid);
        /// <summary>원문 DWORD 하나만 주변 바이트 보존하며 쓴다.</summary>
        public void Write(int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Raw()[offset..], value);
        /// <summary>생성 시 소유 사본을 만들고 필요한 검사만 현재 표를 명시 대여한다.</summary>
        public RawCarrierPostPop Make() => new(Pool, Types, State, Hooks);
    }

    /// <summary>로딩0·권한 있음에서 실제 도달한 base만 요구한다.</summary>
    [Fact]
    public void OnlyReachedBaseRequiresDevice()
    {
        var scene = new Scene(); scene.Hooks.CarrierCheck = null; scene.Hooks.ClaimGround = null; scene.Hooks.ValidateBase = null;
        var post = scene.Make(); scene.Hooks.Base = null; post.PostPop(scene.Sid, 0); Assert.Equal(new[] { "base" }, scene.Events);
    }

    /// <summary>늦은 공통 장치 실패 앞의 check 변경을 유지한다.</summary>
    [Fact]
    public void CheckRunsBeforeMissingCommonDevice()
    {
        var scene = new Scene(); scene.State.Boss = false; scene.Hooks.Base = null; scene.OnCheck = () => scene.Raw()[34] = 5;
        var post = scene.Make(); Assert.Throws<InvalidOperationException>(() => post.PostPop(scene.Sid, 0));
        Assert.Equal(5, scene.Raw()[34]); Assert.Equal(new[] { "check" }, scene.Events);
    }

    /// <summary>check 실패 뒤 타입·공통 몸체에 도달하지 않는다.</summary>
    [Fact]
    public void CheckFailureStopsBeforeDamageable()
    {
        var scene = new Scene(); scene.State.Boss = false; scene.Raw()[10] = 255;
        scene.OnCheck = () => throw new InvalidOperationException("명시 check 중단");
        Assert.Throws<InvalidOperationException>(() => scene.Make().PostPop(scene.Sid, 0)); Assert.Equal(new[] { "check" }, scene.Events);
    }

    /// <summary>반환을 버리는 최초 genus 조회도 로딩0 앞에서 현재TYPE에 접근한다.</summary>
    [Fact]
    public void FirstFlagReadsTypeBeforeZeroLoading()
    {
        var scene = new Scene(); scene.Raw()[10] = 255; var post = scene.Make();
        Assert.Throws<ArgumentOutOfRangeException>(() => post.DamageablePostPop(scene.Sid, 1)); Assert.Empty(scene.Events);
        post.DamageablePostPop(scene.Sid, 0); Assert.Equal(new[] { "base" }, scene.Events);
    }

    /// <summary>최초 flag는 공유 boss getter와 누락 check 모두 생략한다.</summary>
    [Fact]
    public void FirstFlagSkipsMissingVirtualCheck()
    {
        var scene = new Scene(); scene.State.Boss = false; scene.State.SharedBoss = () => throw new InvalidOperationException("미도달 boss 읽기");
        scene.Hooks.CarrierCheck = null; scene.Make().PostPop(scene.Sid, 1); Assert.Equal(new[] { "base" }, scene.Events);
    }

    /// <summary>로딩 중에도 현재 간헐천·extra 관문이면 flags1 접근을 생략한다.</summary>
    [Fact]
    public void ConditionalTypeReadFollowsGeyserAndExtra()
    {
        var scene = new Scene(); scene.State.LoadingDepth = 1; scene.State.GeyserType = 255; scene.Raw()[10] = 255; var post = scene.Make();
        post.DamageablePostPop(scene.Sid, 0); scene.State.GeyserType = 122; scene.Raw()[40] = 1; post.DamageablePostPop(scene.Sid, 0);
        scene.Raw()[40] = 0; Assert.Throws<ArgumentOutOfRangeException>(() => post.DamageablePostPop(scene.Sid, 0));
        Assert.Equal(new[] { "base", "base" }, scene.Events);
    }

    /// <summary>check 뒤 같은 현재 TYPE·깊이·좌표·owner를 읽는다.</summary>
    [Fact]
    public void CheckChangesCurrentTypeFlagsAndLoading()
    {
        var scene = new Scene(); scene.State.Boss = false; uint depth = 0; scene.State.SharedLoadingDepth = () => depth;
        var post = scene.Make(); post.BindCurrentTypes(scene.Types);
        scene.OnCheck = () => { depth = 1; scene.Raw()[10] = 75; scene.Raw()[34] = 5; scene.Types[75].Flags1 = 0x400;
            scene.Write(14, BitConverter.SingleToUInt32Bits(50.25f)); scene.Write(18, BitConverter.SingleToUInt32Bits(60.5f)); };
        post.PostPop(scene.Sid, 0); Assert.Equal(new[] { "check", "ground", "base" }, scene.Events);
        Assert.Equal((50.25f, 60.5f), scene.Point); Assert.Equal(5, scene.Owner); Assert.Equal(0u, scene.State.LoadingDepth);
    }

    /// <summary>공유 깊이·간헐천TYPE를 바꾸면 독립 사본과 무관하게 다음 관문이 바뀐다.</summary>
    [Fact]
    public void CurrentPointersOverrideFallbackCopies()
    {
        var scene = new Scene(); scene.Types[74].Flags1 = 0x400; uint depth = 1, geyser = 74;
        scene.State.SharedLoadingDepth = () => depth; scene.State.SharedGeyserType = () => geyser; var post = scene.Make();
        post.DamageablePostPop(scene.Sid, 0); geyser = 122; post.DamageablePostPop(scene.Sid, 0); depth = 0; post.DamageablePostPop(scene.Sid, 0);
        Assert.Equal(new[] { "base", "ground", "base", "base" }, scene.Events); Assert.Equal(0u, scene.State.LoadingDepth); Assert.Equal(122u, scene.State.GeyserType);
    }

    /// <summary>비최초 호출만 현재 공유 boss를 읽으며 독립 값과 최초flag가 실제 전역을 덮지 않는다.</summary>
    [Fact]
    public void SharedBossChangesBetweenCallsAndKeepsFirstFlag()
    {
        var scene = new Scene(); bool boss = false; int reads = 0;
        scene.State.SharedBoss = () => { reads++; return boss; }; scene.State.Boss = true;
        var post = scene.Make(); post.PostPop(scene.Sid, 0); boss = true; post.PostPop(scene.Sid, 0);
        boss = false; post.PostPop(scene.Sid, 1);
        Assert.Equal(new[] { "check", "base", "base", "base" }, scene.Events);
        Assert.Equal(2, reads); Assert.True(scene.State.Boss); Assert.False(boss);
    }

    /// <summary>지면 실패도 앞선 check의 물리 변경을 되돌리지 않는다.</summary>
    [Fact]
    public void GroundFailureKeepsEarlierCheckMutation()
    {
        var scene = new Scene(); scene.State.Boss = false; scene.State.LoadingDepth = 1; scene.Types[74].Flags1 = 0x400;
        scene.OnCheck = () => scene.Raw()[34] = 5; scene.OnGround = () => throw new InvalidOperationException("명시 ground 중단");
        Assert.Throws<InvalidOperationException>(() => scene.Make().PostPop(scene.Sid, 0));
        Assert.Equal(5, scene.Raw()[34]); Assert.Equal(5, scene.Owner); Assert.Equal(new[] { "check", "ground" }, scene.Events);
    }

    /// <summary>공통 장부 실패도 앞선 ground의 변경을 유지한다.</summary>
    [Fact]
    public void CommonFailureKeepsEarlierGroundMutation()
    {
        var scene = new Scene(); scene.State.LoadingDepth = 1; scene.Types[74].Flags1 = 0x400;
        scene.OnGround = () => scene.Raw()[34] = 6; scene.OnBase = () => throw new InvalidOperationException("명시 base 중단");
        Assert.Throws<InvalidOperationException>(() => scene.Make().DamageablePostPop(scene.Sid, 0));
        Assert.Equal(6, scene.Raw()[34]); Assert.Equal(new[] { "ground", "base" }, scene.Events);
    }

    /// <summary>잘못된 새 대여는 기존 연결을 보존하고 빈 대여는 생성 시 소유 사본으로 돌아간다.</summary>
    [Fact]
    public void BorrowValidationKeepsLiveBindingAndOwnedFallback()
    {
        var scene = new Scene(); scene.State.LoadingDepth = 1; var post = scene.Make(); post.BindCurrentTypes(scene.Types);
        scene.Types[74].Flags1 = 0x400; Assert.Throws<ArgumentException>(() => post.BindCurrentTypes(scene.Types.Take(187).ToArray()));
        post.DamageablePostPop(scene.Sid, 0); post.BindCurrentTypes(null); post.DamageablePostPop(scene.Sid, 0);
        Assert.Equal(new[] { "ground", "base", "base" }, scene.Events);
    }

    /// <summary>예약SID・free・contained에 원문에 없는 PATCH 공통 사전 관문을 추가하지 않는다.</summary>
    [Fact]
    public void ReservedAndInactiveSlotsKeepPhysicalContract()
    {
        var scene = new Scene(); var post = scene.Make(); post.DamageablePostPop(0, 0); Assert.Equal(0, scene.LastSid);
        scene.Raw()[11] = 9; post.DamageablePostPop(scene.Sid, 1);
        Assert.Equal(9, scene.Raw()[11]); Assert.Equal(scene.Sid, scene.LastSid); Assert.Equal(new[] { "base", "base" }, scene.Events);
    }
}
