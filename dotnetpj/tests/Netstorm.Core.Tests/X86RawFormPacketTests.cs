using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Form 패킷 writer(RawFormPacket)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/FormPacketTests.cpp 의 CheckFormPackets 와 같다:
/// 24,000칸 서버 풀에 Form 세 개를 Take 하고 매 행마다 Form 슬롯 전체와 패킷 버퍼·헤더·삭제 표식을 입력값으로 되돌린 뒤
/// 감시(13)·내용물(18)·삭제(8) writer 하나를 실행한다.
/// 반환 크기·패킷 전체 64바이트·표식 소비 결과와, writer 가 슬롯을 바꾸지 않았는지를 대조한다.
/// 기준 판본은 10.78 이며 originalCD 행은 읽지 않는다 (W03 삭제·수명 열두째 조각, 2026-10-22).
/// </summary>
public sealed class X86RawFormPacketTests
{
    /// <summary>10.78 행의 판본 이름.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수.</summary>
    private const int RowsPerEdition = 576;

    /// <summary>패킷 writer 가 쓰는 Form 번호.</summary>
    private const int Form = 20000;

    /// <summary>감시 writer 가 부모로 읽는 다른 Form 번호.</summary>
    private const int Other = 20001;

    /// <summary>감시 writer 가 부모로 읽는 세 번째 Form 번호.</summary>
    private const int Other2 = 20002;

    /// <summary>다른 Form 의 부모 번호. server 영역 번호여야 한다.</summary>
    private const int OtherParent = 20101;

    /// <summary>fixture 의 열 번호.</summary>
    private enum Column
    {
        /// <summary>판본 이름.</summary>
        Edition,

        /// <summary>진입 경로 종류(D·W 감시, C·V 내용물, X·Z 삭제).</summary>
        Kind,

        /// <summary>행 이름표(구현이 쓰지 않는다).</summary>
        Label,

        /// <summary>Form 슬롯 전체 50바이트.</summary>
        Input,

        /// <summary>호출 전 패킷 버퍼 64바이트.</summary>
        Initial,

        /// <summary>헤더 메시지 BYTE.</summary>
        Message,

        /// <summary>헤더 프로토콜 BYTE.</summary>
        Protocol,

        /// <summary>헤더 선언 길이.</summary>
        Length,

        /// <summary>입력 기록용 값(구현이 쓰지 않는다).</summary>
        Note,

        /// <summary>삭제 플래그(데이터 writer 는 쓰지 않는다).</summary>
        Flags,

        /// <summary>호출 전 삭제 표식 SID.</summary>
        StampSid,

        /// <summary>삭제 표식 token.</summary>
        StampToken,

        /// <summary>기대 반환 크기.</summary>
        Return,

        /// <summary>기대 패킷 64바이트.</summary>
        Expected,

        /// <summary>호출 뒤 기대 표식 SID.</summary>
        StampAfter,

        /// <summary>wrapper 방송 경로였는지(1/0).</summary>
        Wrapper,

        /// <summary>열 수.</summary>
        Count,
    }

    /// <summary>감시·삭제 계열 종류인지.</summary>
    /// <param name="kind">종류 글자.</param>
    private static bool IsDependency(char kind) => kind is 'D' or 'W';

    /// <summary>삭제 writer 종류인지.</summary>
    /// <param name="kind">종류 글자.</param>
    private static bool IsDeletion(char kind) => kind is 'X' or 'Z';

    /// <summary>
    /// 원본 writer 입력 장면을 만든다. 서버 풀의 세 Form 은 Take 로 만든 뒤 부모만 직접 쓴다.
    /// </summary>
    private static RawSidPool Scene()
    {
        var pool = new RawSidPool(24000, true);
        var forms = new RawSquidForms(pool);
        // 원본 테스트와 같은 server 슬롯 번호를 Take 로 확보한다
        forms.Take(1, Form);
        forms.Take(1, Other);
        forms.Take(1, Other2);
        foreach (int sid in new[] { Other, Other2 })
        {
            Span<byte> other = pool.AllocatedBytes(sid);
            other[16] = unchecked((byte)OtherParent);
            other[17] = (byte)(OtherParent >> 8);
        }
        return pool;
    }

    /// <summary>
    /// Form 패킷 fixture 의 10.78 몫을 재생한다.
    /// </summary>
    [Fact]
    public void FormPacketMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("form-packet");
        int count = 0;
        RawSidPool pool = Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            Assert.Equal((int)Column.Count, row.Length);
            Span<byte> raw = pool.RawSlot(Form);
            byte[] input = X86Fixture.Bytes(row[(int)Column.Input]);
            Assert.Equal(raw.Length, input.Length);
            input.CopyTo(raw);
            char kind = row[(int)Column.Kind][0];
            var headers = new FormPacketHeaders();
            var stamp = new FormDeletionStamp
            {
                Sid = X86Fixture.Int(row[(int)Column.StampSid]),
                Token = X86Fixture.UInt(row[(int)Column.StampToken]),
            };
            FormPacketHeader header = new(
                (byte)X86Fixture.UInt(row[(int)Column.Message]),
                (byte)X86Fixture.UInt(row[(int)Column.Protocol]),
                (ushort)X86Fixture.UInt(row[(int)Column.Length]));
            if (IsDeletion(kind))
            {
                headers.Deletion = header;
            }
            else if (IsDependency(kind))
            {
                headers.Dependency = header;
            }
            else
            {
                headers.Content = header;
            }
            var writer = new RawFormPacket(pool, headers, stamp);
            var packet = X86Fixture.Bytes(row[(int)Column.Initial]);
            Assert.Equal(64, packet.Length);
            ushort size = IsDeletion(kind)
                ? writer.WriteDeletion(Form, X86Fixture.UInt(row[(int)Column.Flags]), packet)
                : writer.WriteData(Form, packet);
            Assert.Equal(X86Fixture.UInt(row[(int)Column.Return]), (uint)size);
            Assert.Equal(row[(int)Column.Expected], Convert.ToHexStringLower(packet));
            Assert.Equal(stamp.Sid, X86Fixture.Int(row[(int)Column.StampAfter]));
            Assert.Equal(stamp.Token, X86Fixture.UInt(row[(int)Column.StampToken]));
            // writer 는 입력 슬롯을 바꾸지 않는다
            Assert.Equal(row[(int)Column.Input], Convert.ToHexStringLower(raw));
            Assert.Equal(X86Fixture.UInt(row[(int)Column.Wrapper]), kind is 'W' or 'V' or 'Z' ? 1u : 0u);
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }

    /// <summary>
    /// 물리 버퍼가 짧으면 출력·표식 변화 전에 거부하고, 표식 token 과 버퍼를 그대로 둔다.
    /// </summary>
    [Fact]
    public void ShortDeleteBufferPreservesStamp()
    {
        var pool = new RawSidPool(24000, true);
        var headers = new FormPacketHeaders();
        var stamp = new FormDeletionStamp { Sid = Form, Token = 0x1234 };
        var writer = new RawFormPacket(pool, headers, stamp);
        byte[] packet = new byte[7];
        Assert.Throws<ArgumentOutOfRangeException>(() => writer.WriteDeletion(Form, 0xffffffff, packet));
        Assert.Equal(Form, stamp.Sid);
        Assert.Equal(0x1234u, stamp.Token);
        Assert.All(packet, value => Assert.Equal(0, value));
    }

    /// <summary>
    /// Header 는 프로토콜 bit7 을 지우고 선언 길이를 최소 4 로 올리며, 반환 크기 뒤의 옛 버퍼는 보존한다.
    /// </summary>
    [Fact]
    public void HeaderNormalizesProtocolAndKeepsTail()
    {
        var pool = new RawSidPool(24000, true);
        var headers = new FormPacketHeaders { Deletion = new FormPacketHeader(0x2e, 0xc5, 1) };
        var stamp = new FormDeletionStamp();
        var writer = new RawFormPacket(pool, headers, stamp);
        byte[] packet = Enumerable.Repeat((byte)0xa5, 12).ToArray();
        Assert.Equal((ushort)8, writer.WriteDeletion(Form, 0, packet));
        Assert.Equal(0x45, packet[0]);
        Assert.Equal(0x2e, packet[1]);
        Assert.Equal(new byte[] { 4, 0 }, packet[2..4]);
        // 반환 크기 뒤의 바이트는 그대로 남는다
        Assert.All(packet[8..], value => Assert.Equal(0xa5, value));
    }

    /// <summary>
    /// 감시 writer 는 서버 권한과 client 부모·상대 Form 을 거부하고, 내용물 writer 는 다른 가상 표를 거부한다.
    /// </summary>
    [Fact]
    public void DataWriterContractsAndServerNumbers()
    {
        var pool = new RawSidPool(24000, true);
        var forms = new RawSquidForms(pool);
        forms.Take(1, Form);
        forms.Take(1, Other);
        var headers = new FormPacketHeaders();
        var writer = new RawFormPacket(pool, headers, new FormDeletionStamp());
        Span<byte> raw = pool.AllocatedBytes(Form);
        byte[] packet = new byte[18];
        // 상대 Form 번호가 client 번호면 감시 writer 가 거부한다
        raw[18] = 5;
        raw[19] = 0;
        Assert.Throws<InvalidOperationException>(() => writer.WriteData(Form, packet));
        raw[18] = 0;
        // 부모가 client 번호면 두 writer 모두 거부한다
        raw[16] = 5;
        raw[17] = 0;
        Assert.Throws<InvalidOperationException>(() => writer.WriteData(Form, packet));
        // 지원하지 않는 가상 표는 writer 선택 전에 거부한다
        raw[16] = unchecked((byte)OtherParent);
        raw[17] = (byte)(OtherParent >> 8);
        raw[0] = 0x99;
        Assert.Throws<InvalidOperationException>(() => writer.WriteData(Form, packet));
        // content 가상 표로 바꾸면 18바이트 writer 가 동작한다
        raw[0] = 0x08;
        raw[1] = 0x22;
        raw[2] = 0x51;
        raw[3] = 0x00;
        Assert.Equal((ushort)18, writer.WriteData(Form, packet));
    }

    /// <summary>
    /// 감시 writer 는 상대 Form 의 현재 가상 표도 확인하므로, 다른 생성자 가상 표는 부모 읽기 전에 거부한다.
    /// </summary>
    [Fact]
    public void DependencyWriterValidatesOtherFormVtable()
    {
        var pool = new RawSidPool(24000, true);
        var forms = new RawSquidForms(pool);
        forms.Take(1, Form);
        forms.Take(1, Other);
        Span<byte> raw = pool.AllocatedBytes(Form);
        raw[16] = unchecked((byte)OtherParent);
        raw[17] = (byte)(OtherParent >> 8);
        raw[18] = unchecked((byte)Other);
        raw[19] = (byte)(Other >> 8);
        var writer = new RawFormPacket(pool, new FormPacketHeaders(), new FormDeletionStamp());
        Span<byte> other = pool.AllocatedBytes(Other);
        other[0] = 0x99;
        Assert.Throws<InvalidOperationException>(() => writer.WriteData(Form, new byte[13]));
        // 실제 감시 가상 표로 되돌리면 13바이트 writer 가 동작한다
        other[0] = 0x10;
        other[1] = 0x21;
        other[2] = 0x51;
        other[3] = 0x00;
        Assert.Equal((ushort)13, writer.WriteData(Form, new byte[13]));
    }

    /// <summary>
    /// 감시 writer 는 client 풀에서 원본 server assert 로 거부한다.
    /// </summary>
    [Fact]
    public void DependencyWriterRequiresServerPool()
    {
        var pool = new RawSidPool(24000, false);
        var forms = new RawSquidForms(pool);
        // client 풀에서도 Take 는 서버 영역 번호를 받는다
        forms.Take(1, Form);
        var raw = pool.AllocatedBytes(Form);
        raw[16] = unchecked((byte)OtherParent);
        raw[17] = (byte)(OtherParent >> 8);
        var writer = new RawFormPacket(pool, new FormPacketHeaders(), new FormDeletionStamp());
        Assert.Throws<InvalidOperationException>(() => writer.WriteData(Form, new byte[13]));
    }

    /// <summary>
    /// 삭제 writer 는 같은 SID 일 때만 표식을 소비하며, 플래그 상위 WORD 의 0xfff0 비트와 token 낮은 WORD 를 합친다.
    /// </summary>
    [Fact]
    public void DeletionStampConsumptionAndFlagBits()
    {
        var pool = new RawSidPool(24000, true);
        var stamp = new FormDeletionStamp { Sid = Form, Token = 0xabcd1234 };
        var writer = new RawFormPacket(pool, new FormPacketHeaders(), stamp);
        byte[] packet = new byte[8];
        Assert.Equal((ushort)8, writer.WriteDeletion(Form, 0x0001_ffff, packet));
        Assert.Equal(0, stamp.Sid);
        Assert.Equal(0xabcd1234u, stamp.Token);
        // flags 상위 WORD 0x0001 은 0xfff0 과 AND 해 0 이 되고, token 낮은 WORD 0x1234 만 남는다
        Assert.Equal(new byte[] { 0x34, 0x12 }, packet[6..8]);
        // 다음 호출은 이미 소비된 표식이라 token 없이 같은 계산만 한다
        writer.WriteDeletion(Form, 0xffff_ffff, packet);
        Assert.Equal(new byte[] { 0xf0, 0xff }, packet[6..8]);
        // 다른 SID 는 표식을 소비하지 않는다
        stamp.Sid = Form + 1;
        writer.WriteDeletion(Form, 0, packet);
        Assert.Equal(Form + 1, stamp.Sid);
        Assert.Equal(new byte[] { 0, 0 }, packet[6..8]);
        // 0~4 번은 원본이 client SID 로 분류하지 않으므로 삭제 writer 가 허용한다
        Assert.Equal((ushort)8, writer.WriteDeletion(4, 0, packet));
        Assert.Equal(new byte[] { 4, 0 }, packet[4..6]);
        // 일반 client 번호는 삭제 writer 가 거부한다
        Assert.Throws<InvalidOperationException>(() => writer.WriteDeletion(5, 0, packet));
    }
}
