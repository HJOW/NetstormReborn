using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 원본기대값재생과별도로실제PATCH의선택장치/복귀진단/현재필드 경계를검증한다.
namespace Netstorm.Core.Tests;

/// <summary>dead선기록·전체선택·저장next·실제반납의현재계약을검사한다.</summary>
public sealed class RawDestroyPatchCurrentTests
{
    /// <summary>하나의실제할당root와현재서버/장치·raw입력을소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>각권한판정위치에서다시읽는현재서버값이다.</summary>
        public bool Server { get; set; } = true;
        /// <summary>실제FIFO/이력/슬롯을소유하는동일풀이다.</summary>
        public RawSidPool Pool { get; }
        /// <summary>진단로그와실제Unpop이같이읽는현재TYPE다.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>선택장치의실제순서를보관한다.</summary>
        public List<string> Events { get; } = [];
        /// <summary>원문단언의식/파일/줄을그대로보관한다.</summary>
        public List<(string Expression, string File, uint Line)> Assertions { get; } = [];
        /// <summary>각단언이정상복귀하기전에현재raw를변경할선택입력이다.</summary>
        public Action<uint>? AssertEffect { get; set; }
        /// <summary>실제공통삭제와카운터/장치를연결한몸체다.</summary>
        public RawSquidDestroy Body { get; }
        /// <summary>이번장면이실제확보한root번호다.</summary>
        public int Root { get; }
        /// <summary>각효과진입에서현재함수를조회할원문가상묶음이다.</summary>
        public SquidDestroyHooks Hooks { get; }
        /// <summary>실제서버/클라이언트FIFO에서할당하고void자산입력으로외부공간을분리한다.</summary>
        public Scene(bool client = true, bool native = true)
        {
            Pool = new(32768, sharedServer: () => Server); Root = Pool.Allocate(client ? SidAllocation.Client : SidAllocation.Server);
            Span<byte> raw = Pool.RawSlot(Root); raw.Clear(); raw[10] = 74; raw[11] = 4;
            BinaryPrimitives.WriteUInt32LittleEndian(raw, OriginalConstructors.BaseVtable); Types[74].Name = "first\0ignored";
            Body = new(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
            if (native) Body.BindPatchDiagnostics(new()
            {
                Assertion = (expression, file, line) => { Assertions.Add((expression, file, line)); AssertEffect?.Invoke(line); },
                Trace = (level, text) => { Assert.Equal(0x27, level); Events.Add("trace:" + text); },
            });
            Hooks = new()
            {
                Selected = () => 0,
                Emit = effect =>
                {
                    Events.Add($"{effect.Effect}:{effect.Sid}");
                    if (effect.Effect == SquidDestroyEffect.PreDestroy) Body.CompletePreDestroy();
                    if (effect.Effect == SquidDestroyEffect.PostDestroy) Body.CompletePostDestroy();
                    if (effect.Effect == SquidDestroyEffect.DestroyDependent) Pool.RawSlot(effect.Sid)[11] |= 2;
                },
            };
        }
        /// <summary>현재풀의비정렬WORD/DWORD입력을지정한다.</summary>
        public void Put(int sid, int offset, uint value, int width = 4)
        {
            // 지정폭의원문바이트만쓴다.
            for (int i = 0; i < width; i++) Pool.RawSlot(sid)[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>dead는장치를요구하지않고free단언은dead선기록뒤정상복귀하여현재상태를다시읽는다.</summary>
    [Fact]
    public void DeadAndFreePathsKeepOriginalLazyDevicesAndReturningAssertion()
    {
        var s = new Scene(); s.Body.BindPatchDiagnostics(new()); s.Pool.RawSlot(s.Root)[11] = 2;
        s.Body.Destroy(s.Root, 0, new()); Assert.Equal(2, s.Pool.Slot(s.Root)[11]);
        s.Pool.RawSlot(s.Root)[11] = 1;
        s.Body.BindPatchDiagnostics(new()
        {
            Assertion = (expression, file, line) =>
            {
                Assert.Equal("!isFree()", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x24du, line);
                Assert.Equal(3, s.Pool.Slot(s.Root)[11]); s.Events.Add("rescued"); s.Pool.RawSlot(s.Root)[11] = 6;
            },
            Trace = (_, _) => s.Events.Add("trace"),
        });
        s.Body.Destroy(s.Root, 0, s.Hooks); Assert.Equal("rescued", s.Events[0]); Assert.Equal("trace", s.Events[1]);
        Assert.Equal(3, s.Pool.Slot(s.Root)[11]); Assert.Equal((uint)s.Root, s.Pool.Deletions(true)[0].Sid);
    }

    /// <summary>원격SID의권한거부는dead를남기고server-origin분기는후행실제반납의현재서버값을사용한다.</summary>
    [Fact]
    public void RemoteGuardLeavesDeadAndServerOriginUsesCurrentClientRelease()
    {
        var s = new Scene(false); s.Server = false; uint free = s.Pool.FreeCount;
        s.Body.Destroy(s.Root, 0, new()); Assert.Equal(new[] { ("isClientSid()", @"\Ns\O\Squid.cpp", 0x269u) }, s.Assertions);
        Assert.Empty(s.Events); Assert.Equal(6, s.Pool.Slot(s.Root)[11]); Assert.Equal(0u, s.Body.PreDepth);
        s.Pool.RawSlot(s.Root)[11] = 4; s.Assertions.Clear(); s.Body.Destroy(s.Root, 8, s.Hooks);
        Assert.Empty(s.Assertions); Assert.Equal(1, s.Pool.Slot(s.Root)[11]); Assert.Equal(free, s.Pool.FreeCount);
        Assert.Equal((uint)s.Root, s.Pool.Deletions(false)[0].Sid);
    }

    /// <summary>선택DWORD의상위비트는보존하고진단함수사본과NUL앞현재TYPE이름으로로그를만든다.</summary>
    [Fact]
    public void WholeSelectedDwordAndCopiedDiagnosticsKeepCurrentTraceContract()
    {
        var s = new Scene(); var input = new SquidDestroyDiagnostics
        {
            Assertion = (_, _, _) => throw new InvalidOperationException("정상삭제의진단"),
            Trace = (level, text) => { Assert.Equal(0x27, level); Assert.Equal($"destroying first {s.Root}\n", text); s.Events.Add("copied"); },
        };
        s.Body.BindPatchDiagnostics(input); input.Trace = (_, _) => throw new InvalidOperationException("입력장치교체가샘");
        s.Hooks.Selected = () => s.Root; s.Hooks.SelectedRaw = () => 0x10000u + (uint)s.Root;
        s.Body.Destroy(s.Root, 0, s.Hooks);
        Assert.Equal("copied", s.Events[0]); Assert.DoesNotContain(s.Events, value => value.StartsWith("ClearSelection", StringComparison.Ordinal));
    }

    /// <summary>두진입의Raw-only선택함수와동일/상위DWORD비교를기존SID함수없이실행한다.</summary>
    [Theory]
    [InlineData(false, 0u)]
    [InlineData(false, 0x10000u)]
    [InlineData(true, 0u)]
    [InlineData(true, 0x10000u)]
    public void RawOnlySelectionRetainsWholeDwordInBothEntries(bool native, uint high)
    {
        var s = new Scene(native: native); s.Hooks.Selected = null; s.Hooks.SelectedRaw = () => high + (uint)s.Root;
        s.Body.Destroy(s.Root, 0, s.Hooks);
        Assert.Equal(high == 0, s.Events.Contains($"ClearSelection:{s.Root}")); Assert.Equal(3, s.Pool.Slot(s.Root)[11]);
        Assert.Equal(0u, s.Body.PreDepth); Assert.Equal(0u, s.Body.PostDepth);
    }

    /// <summary>로그장치누락은dead와pre깊이선기록뒤가상Pre호출전에만실패한다.</summary>
    [Fact]
    public void MissingTraceKeepsEarlierDeadAndDepthEffects()
    {
        var s = new Scene(); s.Body.BindPatchDiagnostics(new());
        Assert.Contains("로그", Assert.Throws<InvalidOperationException>(() => s.Body.Destroy(s.Root, 0, new())).Message);
        Assert.Equal(6, s.Pool.Slot(s.Root)[11]); Assert.Equal(1u, s.Body.PreDepth); Assert.Equal(0u, s.Body.PostDepth); Assert.Empty(s.Events);
    }

    /// <summary>실행중trace의수명은복사하고그뒤단언은재연결한현재진단을선택한다.</summary>
    [Fact]
    public void TraceCanReplaceDiagnosticsBeforeLaterReturningAssertion()
    {
        var s = new Scene(); s.Body.BindPatchDiagnostics(new()
        {
            Assertion = (_, _, _) => throw new InvalidOperationException("이전진단이샘"),
            Trace = (_, _) => s.Body.BindPatchDiagnostics(new()
            {
                Assertion = (expression, file, line) => { s.Assertions.Add((expression, file, line)); s.Body.CompletePreDestroy(); },
            }),
        });
        s.Hooks.Emit = effect => { if (effect.Effect == SquidDestroyEffect.PostDestroy) s.Body.CompletePostDestroy(); };
        s.Body.Destroy(s.Root, 0, s.Hooks);
        Assert.Equal(0x276u, Assert.Single(s.Assertions).Line); Assert.Equal(0u, s.Body.PreDepth); Assert.Equal(3, s.Pool.Slot(s.Root)[11]);
    }

    /// <summary>두번째free단언이next/free를바꿔도이미저장한next와Release뒤현재free를구별한다.</summary>
    [Fact]
    public void NextIsSavedBeforeSecondFreeAssertionAndCurrentStateIsReadAfterRelease()
    {
        var s = new Scene(); int first = s.Pool.Allocate(SidAllocation.Client), second = s.Pool.Allocate(SidAllocation.Client);
        s.Pool.RawSlot(first).Clear(); s.Pool.RawSlot(second).Clear(); s.Pool.RawSlot(first)[10] = s.Pool.RawSlot(second)[10] = 20;
        s.Pool.RawSlot(first)[11] = 1; s.Put(s.Root, 6, (uint)first, 2); s.Put(first, 4, (uint)second, 2);
        s.AssertEffect = line => { if (line == 0x280) { s.Put(first, 4, 0, 2); s.Pool.RawSlot(first)[11] = 0; } };
        s.Body.Destroy(s.Root, 0, s.Hooks);
        Assert.Equal(new uint[] { 0x27d, 0x280 }, s.Assertions.Select(a => a.Line).ToArray());
        Assert.Contains($"DestroyDependent:{first}", s.Events); Assert.Contains($"ReleaseDependent:{second}", s.Events);
        Assert.Contains($"DestroyDependent:{second}", s.Events); Assert.Equal(3, s.Pool.Slot(s.Root)[11]);
    }

    /// <summary>미완료pre진단이복귀하면잔여깊이를보존하면서post와실제반납까지계속한다.</summary>
    [Fact]
    public void ReturningDepthAssertionDoesNotRollbackOrSkipLaterRelease()
    {
        var s = new Scene(); s.Hooks.Emit = effect =>
        {
            s.Events.Add(effect.Effect.ToString()); if (effect.Effect == SquidDestroyEffect.PostDestroy) s.Body.CompletePostDestroy();
        };
        s.Body.Destroy(s.Root, 0, s.Hooks);
        Assert.Equal("baseDestroy == curDestroy", Assert.Single(s.Assertions).Expression); Assert.Equal(0x276u, s.Assertions[0].Line);
        Assert.Equal(1u, s.Body.PreDepth); Assert.Equal(0u, s.Body.PostDepth); Assert.Equal(3, s.Pool.Slot(s.Root)[11]);
        Assert.Contains("PostDestroy", s.Events); s.Body.CompletePreDestroy(); Assert.Equal(0u, s.Body.PreDepth);
    }

    /// <summary>원문가상+48은TYPE가아닌현재Form VT를선택하고장치누락은앞효과뒤에만실패한다.</summary>
    [Fact]
    public void CurrentFormVtableSelectsUnpopAndRequiresDeviceOnlyAtCallSite()
    {
        var s = new Scene(); s.Pool.RawSlot(s.Root)[11] = 0; s.Put(s.Root, 0, RawSquidForms.VtableFor(6));
        Assert.Contains("Form Unpop", Assert.Throws<InvalidOperationException>(() => s.Body.Destroy(s.Root, 0, s.Hooks)).Message);
        Assert.NotEmpty(s.Events); Assert.Equal(0u, s.Body.PreDepth); Assert.Equal(0u, s.Body.PostDepth); Assert.Equal(2, s.Pool.Slot(s.Root)[11]);
        var next = new Scene(); next.Pool.RawSlot(next.Root)[11] = 0; next.Put(next.Root, 0, RawSquidForms.VtableFor(6));
        next.Hooks.UnpopForm = sid => { Assert.Equal(74, next.Pool.Slot(sid)[10]); next.Events.Add("form"); next.Pool.RawSlot(sid)[11] |= 4; };
        next.Body.Destroy(next.Root, 0, next.Hooks); Assert.Contains("form", next.Events); Assert.Equal(3, next.Pool.Slot(next.Root)[11]);
    }

    /// <summary>Form가상복귀뒤추가void선검사없이Post를마친뒤실제반납의두단언을순서대로실행한다.</summary>
    [Fact]
    public void FormReturnWithoutVoidReachesPostThenNativeReleaseAssertions()
    {
        var s = new Scene(); uint free = s.Pool.FreeCount; s.Pool.RawSlot(s.Root)[11] = 0; s.Put(s.Root, 0, RawSquidForms.VtableFor(6));
        s.Hooks.UnpopForm = _ => s.Events.Add("form");
        s.AssertEffect = line =>
        {
            Assert.Contains($"PostDestroy:{s.Root}", s.Events); Assert.Equal(0u, s.Body.PostDepth);
            if (line == 0x15c) { Assert.Equal(2, s.Pool.Slot(s.Root)[11]); s.Pool.RawSlot(s.Root)[11] |= 1; }
        };
        s.Body.Destroy(s.Root, 0, s.Hooks);
        Assert.Equal(new uint[] { 0x15c, 0x15d }, s.Assertions.Select(a => a.Line).ToArray());
        Assert.Equal(3, s.Pool.Slot(s.Root)[11]); Assert.Equal(free + 1, s.Pool.FreeCount); Assert.Equal((uint)s.Root, s.Pool.Deletions(true)[0].Sid);
    }

    /// <summary>선택된반납단언은실제기록/현재type/FIFO효과를보존하며독립Release의선검사는그대로다.</summary>
    [Fact]
    public void NativePoolReleaseRetainsReturningDiagnosticsAndTailEffects()
    {
        var s = new Scene(); var lines = new List<uint>(); s.Pool.RawSlot(s.Root)[11] = 0;
        s.Pool.ReleasePatch(s.Root, (_, _, line) => lines.Add(line)); Assert.Equal(new uint[] { 0x15c }, lines);
        uint free = s.Pool.FreeCount; SidDeletion[] before = s.Pool.Deletions(true).ToArray();
        Assert.Throws<InvalidOperationException>(() => s.Pool.Release(s.Root)); Assert.Equal(before, s.Pool.Deletions(true).ToArray()); Assert.Equal(free, s.Pool.FreeCount);
        lines.Clear(); s.Pool.ReleasePatch(s.Root, (expression, file, line) =>
        {
            lines.Add(line);
            if (line == 0x123) { Assert.Equal("n != getSid()", expression); Assert.Equal(@"c:\ns\o\squid.h", file); s.Pool.RawSlot(s.Root)[10] = 75; }
        });
        Assert.Equal(new uint[] { 0x15c, 0x15d, 0x123 }, lines); Assert.Equal(free + 1, s.Pool.FreeCount);
        Assert.Equal(75, s.Pool.Slot(s.Root)[10]); Assert.Equal(74u, s.Pool.Deletions(true)[0].Type); Assert.Equal(s.Root, s.Pool.Tail(true));
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(s.Pool.Slot(s.Root)[4..]));
    }
}
