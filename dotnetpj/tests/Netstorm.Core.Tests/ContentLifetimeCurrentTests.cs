using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 현재 Content 클래스·커서·보상 owner를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj ContentLifetimeTests와 FormTakeTests의 현재 클래스·DWORD·너겟 역할 계약을 옮긴다.</summary>
public sealed class ContentLifetimeCurrentTests
{
    /// <summary>아직188칸 계약인 관리 공통 삭제 하네스와 실제 Content의 판본별 TYPE 입력을 분리한다.</summary>
    private static RawSquidDestroy CommonDestroy(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types)
    {
        OriginalTypeRecord[] storage = types.Concat(Enumerable.Range(0, 188 - types.Count).Select(_ => new OriginalTypeRecord())).ToArray();
        return new(pool, new RawUnpop(pool, new(), new byte[65536]), storage);
    }

    /// <summary>현재 수명 검사만 소유하는 실제 풀·공장·정렬·삭제 묶음이다.</summary>
    private sealed class Scene
    {
        /// <summary>동일한 실제 관리 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);
        /// <summary>보상 콜백이 바꾼 비용을 다시 읽는 실제 TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 생성자를 사용하는 공장이다.</summary>
        public readonly RawSquidFactory Factory;
        /// <summary>같은 풀의 공통 삭제와 깊이를 사용한다.</summary>
        public readonly RawSquidDestroy Destroy;
        /// <summary>같은 풀의 정렬·해제 몸체다.</summary>
        public readonly RawContentFormHost Forms;
        /// <summary>내용물의 실제 부모 번호다.</summary>
        public readonly int Parent;

        /// <summary>부모 좌표·발자국을 cpp 너겟 역할 검사와 같은 입력으로 준비한다.</summary>
        public Scene()
        {
            Types[6].ConstructorAddress = OriginalConstructors.Address(6);
            Types[71].Cost = 99.5f; Types[72].Cost = 125.75f; Types[90].FootX = 2; Types[90].FootY = 4;
            Factory = new(Pool, Types);
            Destroy = new(Pool, new RawUnpop(Pool, new(), new byte[65536]), Types);
            Parent = Factory.Create(90, SidAllocation.Client);
            Pool.RawSlot(Parent)[34] = 1;
            BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(Parent)[14..], 40.75f);
            BinaryPrimitives.WriteSingleLittleEndian(Pool.RawSlot(Parent)[18..], 41.25f);
            Forms = new(Pool, Types, (_, _, _) => { });
        }

        /// <summary>현재 부모에 실제 Content를 붙이고 kind·선지급 BYTE만 검사 입력으로 지정한다.</summary>
        public int Content(ushort kind, byte paid = 0)
        {
            int form = Factory.CreateContained(2);
            BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(form)[18..], kind);
            BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(form)[20..], 0x20);
            Pool.RawSlot(form)[22] = paid;
            Forms.Attach(form, Parent, 0x10);
            return form;
        }
    }

    /// <summary>원본56개 관측의 전체 커서 DWORD와 취소 횟수를 값형·현재 getter 두 경로로 대조한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void X86CursorComparisonKeepsAllDwordBits(bool getter)
    {
        int observations = 0;
        // 두 판본의 실제 클래스 토큰과 동일한 명시 커서·부모 입력을 공급한다.
        X86Fixture.CheckRows(X86Fixture.Read("content-cursor-width"), row =>
        {
            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            var pool = new RawSidPool(32768, edition: edition);
            OriginalTypeRecord[] types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
            RawSquidDestroy destroy = CommonDestroy(pool, types);
            var forms = new RawContentFormHost(pool, types, (_, _, _) => { });
            pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 6;
            BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(50), edition == OriginalEdition.Patch1078 ? 0x512208u : 0x505b00u);
            BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(50)[16..], (ushort)X86Fixture.UInt(row[3]));
            BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(50)[18..], 85);
            var state = new ContentLifetimeState { Cursor = X86Fixture.UInt(row[2]) };
            if (getter) state.CurrentCursor = () => state.Cursor;
            uint cancelled = 0;
            var lifetime = new RawContentLifetime(pool, types, forms, destroy, state, new() { CancelCursor = () => cancelled++ });
            lifetime.PreDestroy(50);
            Assert.Equal(X86Fixture.UInt(row[4]), cancelled); Assert.Equal(X86Fixture.UInt(row[5]), state.Cursor);
            Assert.Equal(uint.MaxValue, destroy.PreDepth);
            observations++; return true;
        });
        Assert.Equal(56, observations);
    }

    /// <summary>커서 취소·안내·지갑 뒤 현재 너겟 종류와 각 비용 및 로컬 소유자를 각각 다시 읽는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void NuggetRoleAndEachCostUseCurrentSharedOwners(bool transferOwner)
    {
        var s = new Scene(); var role = new UIntBox { Value = 154 }; var local = new UIntBox { Value = 1 };
        var state = new ContentLifetimeState { NuggetType = 154, LocalOwner = 7, SharedNuggetType = role, SharedLocalOwner = local };
        var events = new List<string>();
        var hooks = new ContentLifetimeHooks
        {
            CancelCursor = () => { events.Add("C"); role.Value = 70; },
            LocalNotice = command => { Assert.Equal(state.Notice, command); Assert.Equal(70u, role.Value); events.Add("N"); role.Value = 71; },
            Wallet = (owner, amount) =>
            {
                Assert.Equal(1u, owner); Assert.Equal(99, amount); events.Add("W"); role.Value = 72;
                if (transferOwner) { local.Value = 2; s.Pool.RawSlot(s.Parent)[34] = 2; }
            },
            AddSp = (owner, amount, isLocal) => { Assert.Equal(transferOwner ? 2u : 1u, owner); Assert.Equal(125.75f, amount); Assert.True(isLocal); events.Add("S"); },
            Sound = (x, y) => { Assert.Equal(39.75f, x); Assert.Equal(39.25f, y); events.Add("A"); },
        };
        var lifetime = new RawContentLifetime(s.Pool, s.Types, s.Forms, s.Destroy, state, hooks);
        int form = s.Content(70); state.Cursor = (uint)form;
        lifetime.Destroy(form, 0x10);
        Assert.Equal("CNWSA", string.Concat(events)); Assert.Equal(72u, role.Value); Assert.Equal(154u, state.NuggetType);
        Assert.Equal(0u, s.Destroy.PreDepth); Assert.Equal(0u, s.Destroy.PostDepth); Assert.Equal(1, s.Pool.Slot(form)[11] & 1);
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(s.Pool.Slot(s.Parent)[6..]));
    }

    /// <summary>커서 취소가 바꾼 실제 권한을 원격 알림 위치에서 읽고 독립 boss 사본을 사용하지 않는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void SharedBossIsRecheckedAfterCursorCancellation(bool nextBoss)
    {
        var s = new Scene(); s.Pool.RawSlot(s.Parent)[34] = 2;
        bool boss = !nextBoss; int cancelled = 0, remote = 0;
        var state = new ContentLifetimeState { Boss = !nextBoss, SharedBoss = () => boss };
        var lifetime = new RawContentLifetime(s.Pool, s.Types, s.Forms, s.Destroy, state, new()
        {
            CancelCursor = () => { cancelled++; boss = nextBoss; },
            RemoteNotice = owner => { Assert.Equal(2u, owner); remote++; },
        });
        int form = s.Content(154, 1); state.Cursor = (uint)form;
        lifetime.Destroy(form, 0x10);
        Assert.Equal(1, cancelled); Assert.Equal(nextBoss ? 1 : 0, remote); Assert.Equal(0u, s.Destroy.PreDepth); Assert.Equal(0u, s.Destroy.PostDepth);
    }

    /// <summary>현재 로컬 소유자와 너겟 종류는 전체 DWORD로 비교하며 하위 WORD/BYTE 별칭을 허용하지 않는다.</summary>
    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public void SharedOwnerAndNuggetTypeKeepFullDword(bool ownerAlias)
    {
        var s = new Scene();
        var state = new ContentLifetimeState
        {
            LocalOwner = 1, NuggetType = 154,
            SharedLocalOwner = new() { Value = ownerAlias ? 0x10001u : 1u },
            SharedNuggetType = new() { Value = ownerAlias ? 154u : 0x1009au }, SharedBoss = () => false,
        };
        var lifetime = new RawContentLifetime(s.Pool, s.Types, s.Forms, s.Destroy, state, new()
        {
            LocalNotice = _ => throw new InvalidOperationException("상위 비트를 잃어 로컬 너겟 알림이 선택됨"),
        });
        int form = s.Content(154, 1); lifetime.Destroy(form, 0x10);
        Assert.Equal(0u, s.Destroy.PreDepth); Assert.Equal(0u, s.Destroy.PostDepth);
    }

    /// <summary>사용중/dead 클래스는 실제 가상표로 판정하고 반납된 슬롯의 기존 타입 조회는 유지한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void CurrentClassAndFreeRecordLookupAreDistinct(OriginalEdition edition)
    {
        var pool = new RawSidPool(32768, edition: edition); var types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
        var forms = new RawContentFormHost(pool, types, (_, _, _) => { });
        RawSquidDestroy destroy = CommonDestroy(pool, types);
        var lifetime = new RawContentLifetime(pool, types, forms, destroy, new(), new());
        int form = new RawSquidForms(pool).Create(6, SidAllocation.Client);
        pool.RawSlot(form)[10] = 7; Assert.True(lifetime.IsContentForm(form));
        pool.RawSlot(form)[11] = 2; Assert.True(lifetime.IsContentForm(form));
        pool.RawSlot(form)[10] = 6; new RawSquidForms(pool).ConstructVtableAt(3, form); Assert.False(lifetime.IsContentForm(form));
        pool.RawSlot(form).Clear(); pool.RawSlot(form)[10] = 6; pool.RawSlot(form)[11] = 1; Assert.True(lifetime.IsContentForm(form));
        pool.RawSlot(form)[10] = 7; Assert.False(lifetime.IsContentForm(form));
    }

    /// <summary>CD의 현재 Content pre는 PATCH 너겟 지급 필드를 읽지 않고 공통 깊이만 반납한다.</summary>
    [Fact]
    public void CdSkipsPatchNuggetEffects()
    {
        var pool = new RawSidPool(32768, edition: OriginalEdition.Cd1072);
        var types = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToArray();
        var forms = new RawContentFormHost(pool, types, (_, _, _) => { });
        RawSquidDestroy destroy = CommonDestroy(pool, types);
        int form = new RawSquidForms(pool).Create(6, SidAllocation.Client);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[16..], 65535); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[18..], 154);
        var lifetime = new RawContentLifetime(pool, types, forms, destroy, new(), new());
        lifetime.PreDestroy(form); Assert.Equal(uint.MaxValue, destroy.PreDepth);
    }

    /// <summary>효과/기본 훅은 C++ 값 인자 사본을 유지하고 현재 커서 owner는 계속 읽는다.</summary>
    [Fact]
    public void CallbackCopiesKeepCurrentStateOwners()
    {
        var s = new Scene(); uint cursor = 0; int cancelled = 0;
        var state = new ContentLifetimeState { CurrentCursor = () => cursor };
        var hooks = new ContentLifetimeHooks { CancelCursor = () => cancelled++ };
        var fallback = new SquidDestroyHooks { SelectedRaw = () => cursor | 0x10000u };
        var lifetime = new RawContentLifetime(s.Pool, s.Types, s.Forms, s.Destroy, state, hooks, fallback);
        hooks.CancelCursor = () => throw new InvalidOperationException("외부에서 재대입한 효과를 사용함"); fallback.SelectedRaw = () => 0;
        int form = s.Content(85); cursor = (uint)form;
        Assert.Equal(cursor | 0x10000u, lifetime.Hooks().SelectedRaw!());
        lifetime.Destroy(form, 0x10); Assert.Equal(1, cancelled); Assert.Equal(0u, s.Destroy.PreDepth); Assert.Equal(0u, s.Destroy.PostDepth);
    }
}
