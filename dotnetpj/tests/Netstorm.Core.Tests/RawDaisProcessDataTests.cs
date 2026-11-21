using System.Buffers.Binary;
using Netstorm.Core.Original;

// 원문 Dais의 실제40BYTE/선택 codec/Kernel 소유권과 전송 분배를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>RawDaisProcessData.cpp와 실제 DaisTransmission의 부분 효과/현재 값을 확인한다.</summary>
public sealed class RawDaisProcessDataTests
{
    /// <summary>기대 이미지의 원문 DWORD 위치만 명시한다.</summary>
    private static void Put(byte[] bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset, 4), value);
    /// <summary>생성자는 선택 필드 외 padding을 보존하며 원문 값 구조체를 복사한다.</summary>
    [Fact]
    public void ConstructorAndDestructionPreserveWholeImageAndInputValue()
    {
        var input = new DaisProcessFields(0xa5); byte[] expected = input.Bytes.ToArray();
        Put(expected, 8, 0); Put(expected, 4, 0); Put(expected, 0x10, 0x507830); expected.AsSpan(0x20, 8).Clear(); expected[0x18] = 0xff; Put(expected, 0, 0x507834);
        var process = new DaisProcess(input); Assert.Equal(expected, process.Fields.Bytes); Assert.All(input.Bytes, value => Assert.Equal(0xa5, value));
        process.Dispose(); Put(expected, 0, 0x5019ec); Put(expected, 4, 0xffffcfc7); Put(expected, 8, 0xffffcfc7); Assert.Equal(expected, process.Fields.Bytes);
    }
    /// <summary>신규 입력은 부모를 반환하고 실제 deadline8BYTE 외 필드를 보존한다.</summary>
    [Fact]
    public void ReadNewStoresOnlySelectedDeadlineAndReturnsParentDword()
    {
        var fields = new DaisProcessFields(0xb6); byte[] expected = fields.Bytes.ToArray(); byte[] packet = Convert.FromHexString("0000000000000000FF88776655000000000000F43F");
        Assert.Equal(0x55667788u, RawDaisProcessData.Read(fields, packet, true)); BinaryPrimitives.WriteUInt64LittleEndian(expected.AsSpan(32, 8), 0x3ff4000000000000);
        Assert.Equal(expected, fields.Bytes); Assert.Equal(0xb6, fields.Bytes[24]);
    }
    /// <summary>기존 객체는 parent bit의 입력까지 읽지 않으며 실제 field 저장이 없으면 NULL/다른 클래스를 허용한다.</summary>
    [Fact]
    public void ExistingParentBitSkipsPayloadAndNullUntilAFieldIsSelected()
    {
        byte[] packet = new byte[9]; packet[8] = 0xfd; Assert.Equal(0u, RawDaisProcessData.Read(null, packet, false));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawDaisProcessData.Read(null, packet, true)); packet[8] = 2;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawDaisProcessData.Read(null, packet, false));
        Array.Resize(ref packet, 17); Assert.Throws<InvalidOperationException>(() => RawDaisProcessData.Read(null, packet, false));
    }
    /// <summary>현재 mask3의 기존 decoder는 부모를 건너뛰고 그 뒤8BYTE만 그대로 저장한다.</summary>
    [Fact]
    public void ExistingDecoderKeepsParentAndPreservesDeadlineBits()
    {
        var fields = new DaisProcessFields(0xa5); byte[] packet = new byte[21]; packet[8] = 3;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9, 4), uint.MaxValue); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(13, 8), 0x7ff8000012345678);
        Assert.Equal(0u, RawDaisProcessData.Read(fields, packet, false)); Assert.Equal(0x7ff8000012345678ul, BinaryPrimitives.ReadUInt64LittleEndian(fields.Bytes.AsSpan(32, 8)));
        Assert.Equal(0xa5a5a5a5u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(8, 4)));
    }
    /// <summary>실제21BYTE 헤더/부모/기한과 끝의dirty0을 검증하고 버퍼 꼬리를 보존한다.</summary>
    [Fact]
    public void WriterUsesExactHeaderPayloadAndDoesNotMaskProtocolBitSeven()
    {
        var fields = new DaisProcessFields(); RawDaisProcessData.Initialize(fields); BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(32, 8), 1.25);
        byte[] packet = Enumerable.Repeat((byte)0xa5, 40).ToArray(); Assert.Equal(21, RawDaisProcessData.Write(fields, packet, 0x11223344, 0x55667788, false, 0x81, 0x6b));
        Assert.Equal(Convert.FromHexString("816B150044332211FF88776655000000000000F43F"), packet.AsSpan(0, 21).ToArray());
        Assert.All(packet.Skip(21), value => Assert.Equal(0xa5, value)); Assert.Equal(0, fields.Bytes[24]);
    }
    /// <summary>force가bit1을 켜도 부모payload0은 쓰지 않는 원문 비대칭을 정상화하지 않는다.</summary>
    [Fact]
    public void ForcedZeroParentStillAdvertisesBitOneWithoutAddingPayload()
    {
        var fields = new DaisProcessFields(); BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(32, 8), 1.25); var packet = new byte[32];
        Assert.Equal(17, RawDaisProcessData.Write(fields, packet, 5, 0, true)); Assert.Equal(Convert.FromHexString("016A110005000000FF000000000000F43F"), packet.AsSpan(0, 17).ToArray());
        Assert.Equal(0, fields.Bytes[24]);
    }
    /// <summary>부모 쓰기가 실패해도force/dirty와 이미 쓴 헤더/SID를 유지하고 뒤쪽 길이/mask는 쓰지 않는다.</summary>
    [Fact]
    public void WriterFailureKeepsEarlierDirtyAndPacketPrefix()
    {
        var fields = new DaisProcessFields(); byte[] packet = Enumerable.Repeat((byte)0xa5, 10).ToArray();
        Assert.Throws<ArgumentOutOfRangeException>(() => RawDaisProcessData.Write(fields, packet, 0x11223344, 7, true));
        Assert.Equal(0xff, fields.Bytes[24]); Assert.Equal(Convert.FromHexString("016AA5A544332211A5A5"), packet);
    }
    /// <summary>현재 기한이 지나도 빈RunFrame/Notify는 같은 이미지를 보존하고 실제 Kernel 반납이 소멸 표식을 기록한다.</summary>
    [Fact]
    public void ActualKernelOwnsEmptyProcessAndDestructsOnlyAtRemoval()
    {
        var process = new DaisProcess(); BinaryPrimitives.WriteDoubleLittleEndian(process.Fields.Bytes.AsSpan(32, 8), -100); byte[] before = process.Fields.Bytes.ToArray();
        var kernel = new Kernel(); uint pid = kernel.Add(process); kernel.RunFrame(); process.Notify(1, 2, 3); Assert.Equal(before, process.Fields.Bytes); Assert.Same(process, kernel.Get(pid));
        kernel.Remove(pid); Assert.Null(kernel.Get(pid)); Assert.Equal(0, kernel.Size()); Assert.Equal(0x5019ecu, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes));
    }
    /// <summary>같은 Dais/헤더/버퍼와 마지막 writer 선택을 유지하며 다른 클래스는 기존 부모 함수로 분배한다.</summary>
    [Fact]
    public void TransmissionRereadsHeaderAndSharesWriterSelectionWithFallback()
    {
        var aux = new RawGameAuxState(); var trace = new List<string>(); var fallback = new ProcessFormTransmission
        {
            WriteData = (_, changed) => { trace.Add("other:" + changed); return 2; }, WriteFullData = (_, force) => { trace.Add("full:" + force); return 3; },
            Packet = length => Enumerable.Repeat((byte)9, length).ToArray(), Transmit = length => trace.Add("other-send:" + length),
        };
        ProcessFormTransmission codec = RawDaisTransmission.Make(aux, length => trace.Add("dais-send:" + length), fallback); var dais = new DaisProcess();
        Assert.Equal(17, codec.WriteData!(dais, false)); Assert.Equal(0x6a, codec.Packet!(17)[1]); codec.Transmit!(17);
        Assert.Equal(2, codec.WriteData(new SquidProcess(), true)); Assert.Equal(new byte[] { 9, 9 }, codec.Packet(2)); codec.Transmit(2);
        aux.DaisProcessHeader[0] = 0x70; aux.DaisProcessHeader[1] = 0x81; Assert.Equal(17, codec.WriteFullData!(dais, 0x10000)); Assert.Equal(0xff, codec.Packet(17)[8]); Assert.Equal(0x81, codec.Packet(17)[0]); Assert.Equal(0x70, codec.Packet(17)[1]);
        Assert.Equal(3, codec.WriteFullData(new SquidProcess(), 5)); Assert.Equal(new byte[] { 9, 9, 9 }, codec.Packet(3)); codec.Transmit(3);
        Assert.Equal(new[] { "dais-send:17", "other:True", "other-send:2", "full:5", "other-send:3" }, trace);
    }
    /// <summary>같은 장기Dais TYPE와 두 헤더 BYTE는 일반 준비로 재초기화하지 않는다.</summary>
    [Fact]
    public void DaisCurrentGlobalsSurviveGeneralWorldReset()
    {
        var aux = new RawGameAuxState(); UIntBox type = aux.DaisProcessTypeBox; byte[] header = aux.DaisProcessHeader;
        Assert.Equal(58u, type.Value); Assert.Equal(new byte[] { 0x6a, 1 }, header); type.Value = 59; header[0] = 0x70; header[1] = 0x81;
        aux.ResetForGeneralWorld(); Assert.Same(type, aux.DaisProcessTypeBox); Assert.Same(header, aux.DaisProcessHeader); Assert.Equal(59u, type.Value); Assert.Equal(new byte[] { 0x70, 0x81 }, header);
    }
}
