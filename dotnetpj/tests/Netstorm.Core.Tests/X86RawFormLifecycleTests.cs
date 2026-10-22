using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 생산 목록 ContentForm(RawContentFormHost)과 감시 쌍(RawDependFormHost) 수명을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/FormLifecycleTests.cpp 의 FormLifecycle_IndependentPeFixture 와 같다:
/// 행마다 같은 크기의 풀을 만들고 명시 입력 슬롯을 복원한 뒤 Pair·Destroy·Notify·Has·Remove·Attach·Robust·Damage 하나를 실행한다.
/// 호출마다 반환 값·외부 통지 순서·지정 슬롯 전체 바이트·풀 머리/꼬리/체크섬·공통 삭제 깊이를 비교한다.
/// 기준 판본은 10.78 이며 originalCD 행은 읽지 않는다 (W03 삭제·수명 열한째 조각, 2026-10-22).
/// </summary>
public sealed class X86RawFormLifecycleTests
{
    /// <summary>10.78 행의 판본 이름.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수.</summary>
    private const int RowsPerEdition = 456;

    /// <summary>fixture 의 열 번호.</summary>
    private enum Column
    {
        /// <summary>판본 이름.</summary>
        Edition,

        /// <summary>실행할 연산.</summary>
        Operation,

        /// <summary>첫 번호(Form 또는 부모).</summary>
        First,

        /// <summary>둘째 번호(부모·피해량·플래그).</summary>
        Second,

        /// <summary>Pop·삭제 플래그 또는 피해량의 float 비트.</summary>
        Flags,

        /// <summary>서버 풀 여부.</summary>
        Server,

        /// <summary>제거 통지에서 Form +20 에 덮어쓸 값(0 이면 그대로).</summary>
        Mutation,

        /// <summary>명시 입력 슬롯.</summary>
        Inputs,

        /// <summary>대조할 출력 슬롯.</summary>
        Outputs,

        /// <summary>기대 반환 값.</summary>
        Result,

        /// <summary>풀·삭제 깊이 열한 값.</summary>
        Meta,

        /// <summary>외부 사건 문자열.</summary>
        Events,

        /// <summary>열 수.</summary>
        Count,
    }

    /// <summary>"번호:hex;번호:hex" 입력을 슬롯에 그대로 복원한다.</summary>
    /// <param name="pool">대상 풀.</param>
    /// <param name="slots">입력 문자열.</param>
    private static void Input(RawSidPool pool, string slots)
    {
        if (slots.Length == 0)
        {
            return;
        }
        // 실행 전 관찰된 초기 바이트만 복원하고 결과로 입력을 만들지 않는다
        foreach (string item in slots.Split(';'))
        {
            string[] fields = item.Split(':');
            int sid = int.Parse(fields[0]);
            byte[] bytes = X86Fixture.Bytes(fields[1]);
            Span<byte> raw = pool.RawSlot(sid);
            Assert.Equal(raw.Length, bytes.Length);
            bytes.CopyTo(raw);
        }
    }

    /// <summary>지정 슬롯 전체를 원본 실행기와 같은 순서로 hex 로 합친다.</summary>
    /// <param name="pool">대상 풀.</param>
    /// <param name="slots">출력 열.</param>
    private static string Dump(RawSidPool pool, string slots)
    {
        if (slots.Length == 0)
        {
            return string.Empty;
        }
        var text = new List<string>();
        // 출력 열의 "번호:hex" 형식을 그대로 지킨다(원본 실행기는 hex 칸만 비교한다)
        foreach (string item in slots.Split(';'))
        {
            int sid = int.Parse(item.Split(':')[0]);
            text.Add($"{sid}:{Convert.ToHexStringLower(pool.Slot(sid))}");
        }
        return string.Join(';', text);
    }

    /// <summary>지정 슬롯에 WORD 를 리틀 엔디언으로 쓴다(합성 입력용).</summary>
    /// <param name="pool">대상 풀.</param>
    /// <param name="sid">대상 번호.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(RawSidPool pool, int sid, int offset, uint value)
    {
        Span<byte> raw = pool.RawSlot(sid);
        raw[offset] = (byte)value;
        raw[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>타입 바이트만 바꾼다(자산 부모 구성용).</summary>
    /// <param name="pool">대상 풀.</param>
    /// <param name="sid">대상 번호.</param>
    /// <param name="type">쓸 타입 번호.</param>
    private static void PutType(RawSidPool pool, int sid, byte type) => pool.RawSlot(sid)[RawSidPool.TypeOffset] = type;

    /// <summary>
    /// 한 행의 풀·처리기·사건 기록을 소유하는 장면이다. 타입 표는 원본 실행기와 같은 188개 빈 레코드다.
    /// </summary>
    private sealed class Scene
    {
        /// <summary>행마다 새로 만드는 32,768칸 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>공통 삭제 본체. Pre·Post 깊이를 함께 본다.</summary>
        public readonly RawSquidDestroy Destroy;

        /// <summary>ContentForm 처리기.</summary>
        public readonly RawContentFormHost Content;

        /// <summary>감시 쌍 처리기.</summary>
        public readonly RawDependFormHost Depend;

        /// <summary>외부 사건 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>원본 실행기와 같은 빈 타입 표(188개).</summary>
        private readonly OriginalTypeRecord[] _types = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>이번 행의 제거 통지 덮어쓰기 값.</summary>
        private readonly uint _mutation;

        /// <summary>장면을 만든다.</summary>
        /// <param name="server">서버 풀인지.</param>
        /// <param name="mutation">부모 제거 통지에서 Form +20 에 덮어쓸 값.</param>
        public Scene(bool server, uint mutation)
        {
            Pool = new RawSidPool(32768, server);
            _mutation = mutation;
            var hash = new RawSquidHash();
            var spots = new byte[65536];
            var unpop = new RawUnpop(Pool, hash, spots);
            Destroy = new RawSquidDestroy(Pool, unpop, _types);
            Content = new RawContentFormHost(Pool, _types, ContentNotice);
            var fallback = new SquidDestroyHooks
            {
                Emit = e =>
                {
                    // 이 장면은 Form 외 타입 삭제 효과를 쓰지 않는다
                    if (e.Effect != SquidDestroyEffect.Transmit)
                    {
                        throw new InvalidOperationException("fixture 미선언 Form 효과");
                    }
                    Transmit(e.Sid, e.Flags);
                },
            };
            Depend = new RawDependFormHost(Pool, Destroy, WatchNotice, fallback, Transmit);
        }

        /// <summary>부모 부착·제거 통지를 기록하고 명시된 경우 +20 만 덮어쓴다.</summary>
        /// <param name="parent">알림을 받는 부모 번호.</param>
        /// <param name="form">부착·제거된 Form 번호.</param>
        /// <param name="reason">1 = 부착, 2 = 제거.</param>
        private void ContentNotice(int parent, int form, uint reason)
        {
            Events.Add($"content:{parent}:{form}:{reason}:{Convert.ToHexStringLower(Pool.Slot(parent))}:{Convert.ToHexStringLower(Pool.Slot(form))}");
            if (reason == 2 && _mutation != 0)
            {
                Put(Pool, form, 20, _mutation);
            }
        }

        /// <summary>감시 쌍 통지를 기록한다.</summary>
        /// <param name="watcher">감시하는 쪽 부모.</param>
        /// <param name="watched">감시 대상 부모.</param>
        /// <param name="form">알림을 보낸 Form.</param>
        private void WatchNotice(int watcher, int watched, int form)
        {
            Events.Add($"watch:{watcher}:{watched}:{form}:{Convert.ToHexStringLower(Pool.Slot(watcher))}:{Convert.ToHexStringLower(Pool.Slot(form))}");
        }

        /// <summary>서버 Pop 전송을 기록한다.</summary>
        /// <param name="form">전송할 Form.</param>
        /// <param name="flags">전송 플래그.</param>
        private void Transmit(int form, uint flags)
        {
            Events.Add($"transmit:{form}:{flags}:{Convert.ToHexStringLower(Pool.Slot(form))}");
        }

        /// <summary>이번 행의 사건 문자열.</summary>
        public string EventText() => string.Join(';', Events);
    }

    /// <summary>
    /// Form 수명 fixture 의 10.78 몫을 재생한다.
    /// </summary>
    [Fact]
    public void FormLifecycleMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("formlifecycle");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            Assert.Equal((int)Column.Count, row.Length);
            var scene = new Scene(row[(int)Column.Server] == "1", X86Fixture.UInt(row[(int)Column.Mutation]));
            Input(scene.Pool, row[(int)Column.Inputs]);
            int first = int.Parse(row[(int)Column.First]);
            int second = int.Parse(row[(int)Column.Second]);
            uint flags = X86Fixture.UInt(row[(int)Column.Flags]);
            uint result = 0;
            switch (row[(int)Column.Operation])
            {
                case "Pair":
                    scene.Depend.Create(first, second);
                    break;
                case "Destroy":
                    scene.Depend.Destroy(first, (uint)second);
                    break;
                case "Notify":
                    scene.Depend.Notify(first, second);
                    break;
                case "Has":
                    result = scene.Depend.Has(first, second) ? 1u : 0u;
                    break;
                case "Remove":
                    scene.Depend.Remove(first, second, flags);
                    break;
                case "Attach":
                    scene.Content.Attach(first, second, flags);
                    break;
                case "Robust":
                    result = scene.Content.Robust(first) ? 1u : 0u;
                    break;
                case "Damage":
                    Assert.Equal(0f, scene.Content.Damage(first, second, BitConverter.UInt32BitsToSingle(flags)));
                    break;
                default:
                    throw new InvalidOperationException($"Form 수명 fixture 실행 연산 오류: {row[(int)Column.Operation]}");
            }
            Assert.Equal(X86Fixture.UInt(row[(int)Column.Result]), result);
            Assert.Equal(row[(int)Column.Events], scene.EventText());
            Assert.Equal(row[(int)Column.Outputs], Dump(scene.Pool, row[(int)Column.Outputs]));
            string[] meta = row[(int)Column.Meta].Split(',');
            // 원본 공통 깊이 기대값은 항상 0 이며 마지막 칸은 예약이다
            uint[] actual =
            [
                (uint)scene.Pool.Capacity,
                scene.Pool.FreeCount,
                scene.Pool.PredictableCursor,
                (uint)scene.Pool.FirstFree(true),
                (uint)scene.Pool.Tail(true),
                (uint)scene.Pool.FirstFree(false),
                (uint)scene.Pool.Tail(false),
                X86Fixture.Adler32(scene.Pool.Bytes),
                scene.Destroy.PreDepth,
                scene.Destroy.PostDepth,
                0,
            ];
            for (int index = 0; index < actual.Length; index++)
            {
                Assert.Equal(X86Fixture.UInt(meta[index]), actual[index]);
            }
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }

    /// <summary>
    /// 감시 Form 삭제는 watched 쪽을 먼저 지운다. 서버 Form 세 개를 직접 연결해 전송 순서로 그 순서를 확인한다.
    /// </summary>
    [Fact]
    public void DependencyPreDestroyOrderIsWatchedFirst()
    {
        var pool = new RawSidPool(32768);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToArray();
        var unpop = new RawUnpop(pool, new RawSquidHash(), new byte[65536]);
        var destroy = new RawSquidDestroy(pool, unpop, types);
        var forms = new RawSquidForms(pool);
        var transmitted = new List<int>();
        var fallback = new SquidDestroyHooks
        {
            Emit = e =>
            {
                // 이 장면의 Form 밖 효과는 서버 전송 하나뿐이다
                Assert.Equal(SquidDestroyEffect.Transmit, e.Effect);
                transmitted.Add(e.Sid);
            },
        };
        var depend = new RawDependFormHost(pool, destroy, null, fallback, null);
        int root = forms.Create(1);
        int watched = forms.Create(1);
        int watcher = forms.Create(1);
        // 공통 삭제 본체는 서버 Form 뿌리의 전송을 Post 전에 알린다
        Put(pool, root, 20, (uint)watched);
        Put(pool, root, 18, (uint)watcher);
        depend.Destroy(root);
        Assert.Equal(new[] { watched, watcher, root }, transmitted);
        Assert.Equal(0u, destroy.PreDepth);
        Assert.Equal(0u, destroy.PostDepth);
    }

    /// <summary>
    /// 감시 쌍은 방향과 상대 삭제를 공유하고, 통지 재진입 삭제 뒤에도 공통 깊이가 0 으로 돌아온다.
    /// </summary>
    [Fact]
    public void DependencyPairDirectionsAndReentrantNotice()
    {
        var pool = new RawSidPool(32768);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToArray();
        var unpop = new RawUnpop(pool, new RawSquidHash(), new byte[65536]);
        var destroy = new RawSquidDestroy(pool, unpop, types);
        var forms = new RawSquidForms(pool);
        // 부모 두 개(자산 90·91)를 직접 만들어 쌍의 방향을 확인한다
        int parentA = pool.Allocate(SidAllocation.Client);
        int parentB = pool.Allocate(SidAllocation.Client);
        PutType(pool, parentA, 90);
        PutType(pool, parentB, 91);
        var depend = new RawDependFormHost(pool, destroy, null, null, (form, flags) => { });
        DependFormPair pair = depend.Create(parentA, parentB);
        Assert.True(depend.Has(parentA, parentB));
        Assert.False(depend.Has(parentB, parentA));
        Assert.Equal(parentA, forms.Parent(pair.WatchedForm));
        Assert.Equal(parentB, forms.Parent(pair.WatcherForm));
        depend.Remove(parentB, parentA);
        Assert.False(depend.Has(parentA, parentB));
        Assert.Equal(0, pool.Slot(parentA)[6] | pool.Slot(parentB)[6]);
        Assert.Equal(3, pool.Slot(pair.WatchedForm)[RawSidPool.StateOffset] & 3);
        Assert.Equal(3, pool.Slot(pair.WatcherForm)[RawSidPool.StateOffset] & 3);
        Assert.Equal((uint)pair.WatchedForm, pool.Deletions(true)[0].Sid);
        Assert.Equal((uint)pair.WatcherForm, pool.Deletions(true)[1].Sid);
        // 통지 콜백 안에서 상대 Form 을 지워도 dead 재진입이 즉시 돌아온다
        uint calls = 0;
        RawDependFormHost? bound = null;
        var reentrant = new RawDependFormHost(pool, destroy, (notifiedWatcher, notifiedWatched, form) =>
        {
            calls++;
            Assert.Equal(parentB, notifiedWatcher);
            Assert.Equal(parentA, notifiedWatched);
            bound!.Destroy(form, 0x10);
        }, null, (form, flags) => { });
        bound = reentrant;
        DependFormPair second = reentrant.Create(parentA, parentB);
        reentrant.Notify(second.WatchedForm, parentA);
        Assert.Equal(1u, calls);
        Assert.False(reentrant.Has(parentA, parentB));
        Assert.Equal(0u, destroy.PreDepth);
        Assert.Equal(0u, destroy.PostDepth);
    }

    /// <summary>
    /// 감시 쌍 생성은 번호·타입·서버 권한 계약을 지키고, 어느 한 부모가 client 번호면 두 Form 을 client 영역에 만든다.
    /// </summary>
    [Fact]
    public void DependencyCreateContractsAndMixedRegions()
    {
        var pool = new RawSidPool(32768, false);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToArray();
        var unpop = new RawUnpop(pool, new RawSquidHash(), new byte[65536]);
        var destroy = new RawSquidDestroy(pool, unpop, types);
        var depend = new RawDependFormHost(pool, destroy);
        int clientA = pool.Allocate(SidAllocation.Client);
        int clientB = pool.Allocate(SidAllocation.Client);
        PutType(pool, clientA, 90);
        PutType(pool, clientB, 91);
        // client 풀에서는 서버 쪽 부모만으로는 쌍을 만들 수 없다
        Assert.Throws<InvalidOperationException>(() => depend.Create(15000, 15001));
        // client 부모가 섞이면 두 Form 모두 client 영역에 만든다
        DependFormPair pair = depend.Create(clientA, clientB);
        Assert.True(pair.WatchedForm < RawSidPool.ServerFirst);
        Assert.True(pair.WatcherForm < RawSidPool.ServerFirst);
        Assert.Equal(1, pool.Slot(pair.WatchedForm)[RawSidPool.TypeOffset]);
        // 같은 부모 안의 두 Form 도 원본처럼 생성할 수 있다
        Assert.NotEqual(0, depend.Create(clientA, clientA).WatchedForm);
        pool.RawSlot(clientB)[RawSidPool.StateOffset] |= RawSidPool.FreeBit;
        Assert.Throws<InvalidOperationException>(() => depend.Create(clientA, clientB));
    }

    /// <summary>
    /// ContentForm 부착은 그룹 순서로 들어가고, 양수 피해는 재부착 알림을 두 번 보낸 뒤 순서 변경 표시를 지운다.
    /// </summary>
    [Fact]
    public void ContentAttachOrderAndDamageReattach()
    {
        var pool = new RawSidPool(32768);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToArray();
        var reasons = new List<uint>();
        var content = new RawContentFormHost(pool, types, (parent, form, reason) => reasons.Add(reason));
        var forms = new RawSquidForms(pool);
        int parent = pool.Allocate(SidAllocation.Server);
        PutType(pool, parent, 90);
        int first = forms.Create(6, SidAllocation.Client);
        int second = forms.Create(6, SidAllocation.Client);
        int third = forms.Create(6, SidAllocation.Client);
        // 같은 그룹의 순서 번호가 2·1·0 이 되도록 미리 넣는다
        content.Attach(first, parent, 0x10);
        content.Attach(second, parent, 0x10);
        content.Attach(third, parent, 0x10);
        Assert.Equal(new uint[] { 1, 1, 1 }, reasons);
        Assert.Equal(first, pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8));
        Assert.Equal(second, pool.Slot(first)[4] | (pool.Slot(first)[5] << 8));
        Assert.Equal(third, pool.Slot(second)[4] | (pool.Slot(second)[5] << 8));
        Assert.Equal(0, pool.Slot(first)[23]);
        Assert.Equal(1, pool.Slot(second)[23]);
        Assert.Equal(2, pool.Slot(third)[23]);
        Assert.True(content.Linked(first));
        reasons.Clear();
        Assert.Equal(0f, content.Damage(first, 1, 7f));
        Assert.Equal(new uint[] { 2, 1 }, reasons);
        Assert.Equal(0, pool.Slot(first)[20] & 0x100);
        Assert.Equal(parent, pool.Slot(first)[16] | (pool.Slot(first)[17] << 8));
        // 0·음수 피해는 아무것도 바꾸지 않고 값을 그대로 돌려준다
        string before = Convert.ToHexStringLower(pool.Slot(first));
        Assert.Equal(-3.5f, content.Damage(first, 0, -3.5f));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(first)));
    }
}
