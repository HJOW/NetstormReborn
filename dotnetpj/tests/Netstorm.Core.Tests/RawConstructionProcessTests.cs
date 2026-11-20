using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제Kernel·Form·Host 위에서 건설 프로세스의 진행·완료·삭제 수명을 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 본체의 합성 경계 계약과 실제 프로세스 소유권 검사다.</summary>
public sealed class RawConstructionProcessTests
{
    /// <summary>진행 시각과 실제Host·부모 슬롯을 공유하는 검사 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>실제PATCH 할당 영역을 사용하는 관리 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>프로세스를 소유하는 실제Kernel이다.</summary>
        public readonly Kernel Kernel = new();
        /// <summary>Form과부모 수명을 처리하는 실제Host다.</summary>
        public readonly SquidProcessHost Host;
        /// <summary>원문 내장 생성자 주소를 가진 실제TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types;
        /// <summary>현재 프로세스 전역이다.</summary>
        public readonly ConstructionProcessState State = new() { Now = 2 };
        /// <summary>실제 프로세스 호스트의 현재 권한이다.</summary>
        public readonly SquidProcessState Clock = new();
        /// <summary>선택한 음향·공간·표시 경계의 실제 기록이다.</summary>
        public readonly List<string> Events = [];
        /// <summary>서버 영역의 실제 부모다.</summary>
        public readonly int Parent;
        /// <summary>현재 효과를 조립하는 함수 표다.</summary>
        public readonly ConstructionProcessHooks Hooks;
        /// <summary>독립 원문 판본이다.</summary>
        public bool Patch => Pool.Edition == OriginalEdition.Patch1078;
        /// <summary>현재abstract BYTE다.</summary>
        public int Extra => Patch ? 40 : 35;
        /// <summary>현재 부모의부착Form이다.</summary>
        public int Form => BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(Parent).Slice(6, 2));
        /// <summary>현재 판본의HP 읽기다.</summary>
        public int Hp => Patch ? BinaryPrimitives.ReadInt32LittleEndian(Pool.Slot(Parent).Slice(26, 4))
            : BinaryPrimitives.ReadInt16LittleEndian(Pool.Slot(Parent).Slice(26, 2));

        /// <summary>실제 생성·부착·Kill을 준비하고 외부 효과만 명시 응답으로 제공한다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            Pool = new(24000, true, null, edition);
            Types = Enumerable.Range(0, Patch ? 188 : 171).Select(index => new OriginalTypeRecord
            {
                ConstructorAddress = OriginalConstructors.Address(edition, index),
            }).ToArray();
            Types[100].Flags2 = 0x4000;
            var destroy = new RawSquidDestroy(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
            Host = new(Pool, Types, Kernel, destroy, Clock, asset: new SquidDestroyHooks
            {
                Emit = effect =>
                {
                    if (effect.Effect == SquidDestroyEffect.PreDestroy) destroy.CompletePreDestroy();
                    else if (effect.Effect == SquidDestroyEffect.PostDestroy) destroy.CompletePostDestroy();
                },
                Selected = () => 0,
            });
            Parent = Pool.Allocate(SidAllocation.Server); Pool.AllocatedBytes(Parent).Clear();
            Pool.AllocatedBytes(Parent)[10] = 100; Pool.AllocatedBytes(Parent)[Patch ? 34 : 32] = 1;
            Pool.AllocatedBytes(Parent)[Extra] = 1;
            Hooks = new()
            {
                Reserve = () => true, Effect = (_, instant) => Events.Add(instant ? "I" : "E"),
                LoadSound = name => { Assert.Equal("jimBuild.wav", name); return 77; }, ReserveSound = () => true,
                AttachSound = (_, sound, alternate, flags) => { Assert.Equal(0u, alternate); Events.Add($"P:{sound}:{flags}"); },
                Refresh = () => Events.Add("R"), MaxHitPoints = _ => 300, UpdateDisplay = _ => Events.Add("D"),
                RemoveContained = (_, type) => Events.Add($"X:{type}"), PlayAt = (_, _, name, _) => Events.Add("W:" + name),
                ReadySound = _ => 85, Play = sound => Events.Add($"G:{sound}"),
                Unpop = (sid, _) => { Events.Add("U"); Pool.AllocatedBytes(sid)[11] |= 4; },
                Repop = (sid, flags) => { Events.Add($"Q:{flags}"); Pool.AllocatedBytes(sid)[11] &= 0xfb; },
                Push = (_, _, type, owner) => Events.Add($"B:{type}:{owner}"),
            };
        }
        /// <summary>같은 실제Host에 본체를 연결한다.</summary>
        public RawConstructionProcess Make() => new(Pool, Types, State, Hooks, Host);
        /// <summary>같은 Kernel이 소유한 프로세스의 실제 제거 통지를 끝낸다.</summary>
        public void Dispose() => Kernel.Clear();
    }

    /// <summary>10.78의 실제 Kernel 진행·Kill·Form 삭제 통지로 abstract를 해제한다.</summary>
    [Fact]
    public void ActualHostRunsToCompletion()
    {
        using var scene = new Scene(); RawConstructionProcess logic = scene.Make();
        logic.Start(scene.Parent, 8);
        Assert.Equal("E;P:77:1;R", string.Join(';', scene.Events)); Assert.Equal(1, scene.Kernel.Size()); Assert.NotEqual(0, scene.Form);
        scene.Events.Clear(); scene.State.Now = 4; scene.Kernel.RunFrame();
        Assert.Equal(75, scene.Hp); Assert.Equal("D", Assert.Single(scene.Events)); Assert.Equal(1, scene.Kernel.Size());
        scene.Events.Clear(); scene.State.Now = 10; scene.Kernel.RunFrame();
        Assert.Equal("D;X:47;R;W:buildDone.wav;G:85;U;Q:9;B:100:1", string.Join(';', scene.Events));
        Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(300, scene.Hp); Assert.Equal(0, scene.Form);
        Assert.Equal(0, scene.Pool.Slot(scene.Parent)[scene.Extra] & 1); Assert.Equal(0, scene.Pool.Slot(scene.Parent)[11] & 4);
    }

    /// <summary>첫 Kill로 제거한 일반 부모의 완료를 두 번 실행하지 않는다.</summary>
    [Fact]
    public void RemovedProcessSkipsSecondKill()
    {
        using var scene = new Scene(); RawConstructionProcess logic = scene.Make(); logic.Start(scene.Parent, 8);
        scene.Pool.AllocatedBytes(scene.Parent)[scene.Extra] = 0; scene.State.Now = 10; scene.Events.Clear(); scene.Kernel.RunFrame();
        Assert.Equal("D;X:47;R;W:buildDone.wav;G:85", string.Join(';', scene.Events)); Assert.Equal(0, scene.Kernel.Size());
    }

    /// <summary>완료 관문은진행과 다른 시점의 같은 현재권한을 다시 읽는다.</summary>
    [Fact]
    public void SharedBossIsReadAfterRefresh()
    {
        using var scene = new Scene(); bool boss = false, flip = false; scene.State.SharedBoss = () => boss;
        scene.Hooks.Refresh = () => { scene.Events.Add("R"); if (flip) boss = false; };
        RawConstructionProcess logic = scene.Make(); logic.Start(scene.Parent, 8); scene.State.QuickBuild.Set(true); scene.State.Now = 10;
        scene.Kernel.RunFrame(); Assert.Equal(1, scene.Kernel.Size()); Assert.Equal(300, scene.Hp);
        boss = true; flip = true; scene.Kernel.RunFrame();
        Assert.Equal(0, scene.Kernel.Size()); Assert.False(boss); Assert.Equal(1, scene.Pool.Slot(scene.Parent)[scene.Extra] & 1);
    }

    /// <summary>부착한 실제Form의필드만 끝 시각을 변경하며시작·소리·다른dirty를 보존한다.</summary>
    [Fact]
    public void ResetEndUsesCurrentHostForm()
    {
        using var scene = new Scene(); RawConstructionProcess logic = scene.Make(); logic.Start(scene.Parent, 8);
        var process = Assert.IsType<ConstructionProcess>(scene.Host.FromForm(scene.Form));
        process.Fields.Dirty = 0x81; scene.State.Now = 7.5; logic.ResetEndFromForm(scene.Form, 0xffffffff);
        Assert.Equal(2, process.Fields.Start); Assert.Equal(17.5, process.Fields.End); Assert.Equal(77u, process.Fields.Sound); Assert.Equal(0x85, process.Fields.Dirty);
        Assert.Throws<ArgumentException>(() => logic.ResetEndFromForm(scene.Parent, 1));
    }

    /// <summary>PATCH 진단은 현재 Form→진단 복귀→현재 부모→HP 순서로 실행한다.</summary>
    [Fact]
    public void CheckingRechecksParentAfterDiagnostic()
    {
        using var scene = new Scene(); scene.State.Checking.Set(0x80000000u);
        scene.Hooks.FormRobust = form => { Assert.Equal(91, form); scene.Events.Add("robust"); return false; };
        scene.Hooks.Assertion = (condition, path, line) =>
        {
            Assert.Equal("getForm().isRobust()", condition); Assert.Equal("\\Ns\\O\\Construction.cpp", path); Assert.Equal(0x1c4u, line);
            scene.Events.Add("assert"); scene.State.Checking.Set(0);
        };
        RawConstructionProcess logic = scene.Make(); scene.Events.Clear();
        logic.Run(new() { Start = 2, End = 10 }, 0, _ => throw new InvalidOperationException("선택되지 않아야 하는Kill"),
            () => { scene.Events.Add("form"); return 91; }, () => { scene.Events.Add("parent"); return scene.Parent; });
        Assert.Equal("form;robust;assert;parent;D", string.Join(';', scene.Events)); Assert.Equal(0, scene.Hp);
    }

    /// <summary>예약·첫 쓰기 전에 누락 효과와 다른 Host 풀을 거부한다.</summary>
    [Fact]
    public void RejectsMissingHooksAndForeignPool()
    {
        using var scene = new Scene(); ConstructionProcessHooks hooks = scene.Hooks.Copy(); hooks.Push = null;
        Assert.Throws<ArgumentException>(() => new RawConstructionProcess(scene.Pool, scene.Types, scene.State, hooks, scene.Host));
        using var other = new Scene(); Assert.Throws<ArgumentException>(() => new RawConstructionProcess(scene.Pool, scene.Types, scene.State, scene.Hooks, other.Host));
        Assert.Empty(scene.Events);
    }
}
