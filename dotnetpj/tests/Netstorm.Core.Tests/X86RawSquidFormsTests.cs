using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 내장 Form 생성·지정 Take·공통 Unpop(RawSquidForms)을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/FormTakeTests.cpp 의 FormTake_IndependentPeFixture 와 같다:
/// 행마다 같은 크기의 풀을 만들고 명시 입력 슬롯을 복원한 뒤 Create·Take·Unpop·Reset 하나를 실행한다.
/// 호출마다 반환 번호·부모 제거 통지 문자열·지정 슬롯 전체 바이트·풀 머리/꼬리/체크섬을 비교한다.
/// 기준 판본은 10.78 이며 originalCD 행은 읽지 않는다 (W03 삭제·수명 열한째 조각, 2026-10-22).
/// </summary>
public sealed class X86RawSquidFormsTests
{
    /// <summary>10.78 행의 판본 이름.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수.</summary>
    private const int RowsPerEdition = 300;

    /// <summary>fixture 의 열 번호.</summary>
    private enum Column
    {
        /// <summary>판본 이름.</summary>
        Edition,

        /// <summary>실행할 연산(Create·Take·Unpop·Reset).</summary>
        Operation,

        /// <summary>타입 번호.</summary>
        Type,

        /// <summary>지정 슬롯 또는 Reset 크기.</summary>
        Argument,

        /// <summary>서버 풀 여부.</summary>
        Server,

        /// <summary>명시 입력 슬롯.</summary>
        Inputs,

        /// <summary>대조할 출력 슬롯.</summary>
        Outputs,

        /// <summary>기대 반환 번호.</summary>
        Return,

        /// <summary>풀 크기.</summary>
        Capacity,

        /// <summary>빈 번호 카운터.</summary>
        FreeCount,

        /// <summary>예측 커서.</summary>
        Cursor,

        /// <summary>클라이언트 첫 빈 번호.</summary>
        ClientFirst,

        /// <summary>클라이언트 꼬리.</summary>
        ClientTail,

        /// <summary>서버 첫 빈 번호.</summary>
        ServerFirst,

        /// <summary>서버 꼬리.</summary>
        ServerTail,

        /// <summary>풀 전체 Adler-32.</summary>
        Adler,

        /// <summary>부모 제거 통지 문자열.</summary>
        Notices,

        /// <summary>열 수.</summary>
        Count,
    }

    /// <summary>"번호:hex;번호:hex" 입력을 슬롯에 그대로 복원한다. 빈 슬롯도 포함한다.</summary>
    /// <param name="pool">대상 풀.</param>
    /// <param name="slots">입력 문자열.</param>
    private static void Input(RawSidPool pool, string slots)
    {
        if (slots.Length == 0)
        {
            return;
        }
        // 각 항목은 실행 전 관찰된 초기 바이트이며 구현 결과로 만들지 않는다
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
    /// <param name="slots">출력 문자열.</param>
    /// <returns>실제 hex 문자열.</returns>
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

    /// <summary>
    /// 지정 슬롯에 2바이트 값을 리틀 엔디언으로 쓴다(합성 입력용).
    /// </summary>
    /// <param name="pool">대상 풀.</param>
    /// <param name="sid">대상 번호.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(RawSidPool pool, int sid, int offset, uint value, int width = 2)
    {
        Span<byte> raw = pool.RawSlot(sid);
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>
    /// Form Take fixture 의 10.78 몫을 재생한다.
    /// </summary>
    [Fact]
    public void FormTakeMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("formtake");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            Assert.Equal((int)Column.Count, row.Length);
            var pool = new RawSidPool(32768, row[(int)Column.Server] == "1");
            Input(pool, row[(int)Column.Inputs]);
            var notices = new List<string>();
            var forms = new RawSquidForms(pool, (parent, form) =>
            {
                // 통지 시점의 부모·Form 전체 바이트를 기록한다
                notices.Add($"{parent}:{form}:2:{Convert.ToHexStringLower(pool.Slot(form))}:{Convert.ToHexStringLower(pool.Slot(parent))}");
            });
            int number = int.Parse(row[(int)Column.Type]);
            int argument = int.Parse(row[(int)Column.Argument]);
            int result = 0;
            switch (row[(int)Column.Operation])
            {
                case "Create":
                    result = forms.Create((uint)number, (SidAllocation)argument);
                    break;
                case "Take":
                    result = forms.Take((uint)number, argument);
                    break;
                case "Unpop":
                    forms.Unpop(argument);
                    break;
                case "Reset":
                    pool.Reset(argument);
                    break;
                default:
                    throw new InvalidOperationException($"Form fixture 실행 연산 오류: {row[(int)Column.Operation]}");
            }
            Assert.Equal(X86Fixture.Int(row[(int)Column.Return]), result);
            Assert.Equal(row[(int)Column.Notices], string.Join(';', notices));
            Assert.Equal(row[(int)Column.Outputs], Dump(pool, row[(int)Column.Outputs]));
            // watched 슬롯 밖의 변화까지 풀 머리·꼬리·체크섬으로 함께 본다
            uint[] actual =
            [
                (uint)pool.Capacity,
                pool.FreeCount,
                pool.PredictableCursor,
                (uint)pool.FirstFree(true),
                (uint)pool.Tail(true),
                (uint)pool.FirstFree(false),
                (uint)pool.Tail(false),
                X86Fixture.Adler32(pool.Bytes),
            ];
            for (int index = 0; index < actual.Length; index++)
            {
                Assert.Equal(X86Fixture.UInt(row[(int)Column.Capacity + index]), actual[index]);
            }
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }

    /// <summary>
    /// 살아 있는 같은 슬롯을 다른 타입으로 Take 하면 Unpop 전에 거부하고 바이트를 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void TakeRejectsLiveSlotOfDifferentType()
    {
        var pool = new RawSidPool(32768);
        var forms = new RawSquidForms(pool, null, (form, flags) => { });
        int parent = pool.Allocate(SidAllocation.Server);
        int form = forms.Create(2);
        forms.Attach(form, parent);
        string before = Convert.ToHexStringLower(pool.Slot(form));
        // 같은 생성자(ProcessForm) 가상 표를 쓰더라도 타입 번호가 다르면 거부한다
        Assert.Throws<InvalidOperationException>(() => forms.Take(1, form));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(form)));
        Assert.Equal(2, pool.Slot(form)[RawSidPool.TypeOffset]);
        Assert.Equal(0, pool.Slot(form)[RawSidPool.StateOffset] & RawSidPool.VoidBit);
    }

    /// <summary>
    /// ContentForm 통지는 contained 를 끈 뒤 부모 머리를 지운 상태에서 오고, 통지가 바꾼 payload 는 Take 의 생성자 쓰기에도 남는다.
    /// </summary>
    [Fact]
    public void ContentNoticeTimingKeepsPayload()
    {
        var pool = new RawSidPool(32768);
        var plain = new RawSquidForms(pool);
        int parent = plain.Create(1, SidAllocation.Client);
        int form = plain.Create(6);
        Put(pool, parent, 6, (uint)form);
        Put(pool, form, 16, (uint)parent);
        pool.AllocatedBytes(form)[RawSidPool.StateOffset] = 8;
        string before = Convert.ToHexStringLower(pool.Slot(form));
        // 알림 경계가 없으면 Take 는 슬롯을 바꾸기 전에 거부한다
        Assert.Throws<InvalidOperationException>(() => plain.Take(6, form));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(form)));
        uint notices = 0;
        var bound = new RawSquidForms(pool, (notifiedParent, notifiedForm) =>
        {
            notices++;
            Assert.Equal(parent, notifiedParent);
            Assert.Equal(form, notifiedForm);
            Assert.Equal(4, pool.Slot(form)[RawSidPool.StateOffset] & 12);
            Assert.Equal(0, pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8));
            Put(pool, form, 18, 0xfedc5678, 4);
        });
        Assert.Equal(form, bound.Take(6, form));
        Assert.Equal(1u, notices);
        Assert.Equal(0xfedc5678u, BitConverter.ToUInt32(pool.Slot(form)[18..22]));
        Assert.Throws<InvalidOperationException>(() => bound.Take(6, form));
        Assert.Throws<InvalidOperationException>(() => bound.Take(3, form));
        // 지원하지 않는 생성자 타입은 할당 전에 거부한다
        uint free = pool.FreeCount;
        Assert.Throws<ArgumentOutOfRangeException>(() => bound.Create(4));
        Assert.Equal(free, pool.FreeCount);
    }

    /// <summary>
    /// Take 는 서버 영역 번호만 받고, 부모 dead/free 의 Unpop 은 링크를 보존한다.
    /// </summary>
    [Fact]
    public void TakeRegionAndUnpopParentBoundaries()
    {
        var pool = new RawSidPool(32768);
        var forms = new RawSquidForms(pool);
        Assert.Throws<ArgumentOutOfRangeException>(() => forms.Take(1, RawSidPool.ServerFirst - 1));
        // 부모가 dead 면 Form 은 void 가 되지만 체인 링크를 건드리지 않는다
        int parent = pool.Allocate(SidAllocation.Server);
        int form = forms.Create(1, SidAllocation.Client);
        Put(pool, parent, 6, (uint)form);
        Put(pool, form, 16, (uint)parent);
        Put(pool, form, 4, 7);
        Put(pool, form, 14, 0);
        pool.RawSlot(form)[RawSidPool.StateOffset] = 8;
        pool.RawSlot(parent)[RawSidPool.StateOffset] = RawSidPool.DeadBit;
        forms.Unpop(form);
        Assert.Equal(4, pool.Slot(form)[RawSidPool.StateOffset] & 4);
        Assert.Equal((uint)form, (uint)(pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8)));
        Assert.Equal(7, pool.Slot(form)[4] | (pool.Slot(form)[5] << 8));
        // 두 번 Unpop 은 void 에서 거부한다
        Assert.Throws<InvalidOperationException>(() => forms.Unpop(form));
    }

    /// <summary>
    /// Pop 은 권한에 따라 플래그를 보완하고, 서버 전송이 필요한데 경계가 없으면 void 를 끄기 전에 거부한다.
    /// </summary>
    [Fact]
    public void PopFlagCompletionAndTransmitBoundary()
    {
        var server = new RawSidPool(32768, true);
        var unbound = new RawSquidForms(server);
        int clientSide = unbound.Create(1, SidAllocation.Client);
        Assert.Equal(0x10u, unbound.ValidatePop(clientSide, 0));
        // 클라이언트 쪽 Form 은 서버 전송 없이 Pop 된다
        unbound.Pop(clientSide, 0);
        Assert.Equal(0, server.Slot(clientSide)[RawSidPool.StateOffset] & 4);
        // 서버 쪽 Form 은 전송이 필요하므로 전송 경계 없이 ValidatePop 이 거부한다
        int serverSide = unbound.Create(1);
        Assert.Throws<InvalidOperationException>(() => unbound.ValidatePop(serverSide, 0));
        // 0x40 은 원본이 전송을 건너뛰는 묶음 비트다
        Assert.Equal(0x48u, unbound.ValidatePop(serverSide, 0x40));
        var flags = new List<uint>();
        var bound = new RawSquidForms(server, null, (form, value) => flags.Add(value));
        int sent = bound.Create(1);
        bound.Pop(sent, 1);
        Assert.Equal(new uint[] { 9 }, flags);
        Assert.Equal(0, server.Slot(sent)[RawSidPool.StateOffset] & (4 | 0x10));
        var client = new RawSidPool(32768, false);
        var clientForms = new RawSquidForms(client);
        int local = clientForms.Create(1, SidAllocation.Client);
        Assert.Equal(0x10u, clientForms.ValidatePop(local, 0));
    }

    /// <summary>
    /// Attach 는 ContentForm·자기 자신·빈 부모·손상 머리를 슬롯 변경 전에 거부한다.
    /// </summary>
    [Fact]
    public void AttachRejectsInvalidParents()
    {
        var pool = new RawSidPool(32768);
        var forms = new RawSquidForms(pool);
        int parent = pool.Allocate(SidAllocation.Server);
        int form = forms.Create(1, SidAllocation.Client);
        int content = forms.Create(6, SidAllocation.Client);
        string before = Convert.ToHexStringLower(pool.Slot(form));
        Assert.Throws<InvalidOperationException>(() => forms.Attach(content, parent));
        Assert.Throws<InvalidOperationException>(() => forms.Attach(form, form));
        Assert.Throws<InvalidOperationException>(() => forms.Attach(form, 1));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(form)));
        // 부모 0 은 10.78 처럼 무동작이다
        forms.Attach(form, 0);
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(form)));
        // 종속 머리가 자기 자신이면 거부한다
        Put(pool, parent, 6, (uint)form);
        Assert.Throws<InvalidOperationException>(() => forms.Attach(form, parent));
        Put(pool, parent, 6, 0);
        forms.Attach(form, parent);
        Assert.Equal((uint)form, (uint)(pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8)));
        Assert.Equal(8, pool.Slot(form)[RawSidPool.StateOffset] & 8);
        Assert.Equal(0, pool.Slot(form)[RawSidPool.StateOffset] & 4);
    }
}
