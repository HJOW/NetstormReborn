using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 임시 사용자 파일로 원문21BYTE 접두/현재 필드와 실패 전 부분 효과를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>기록 장치의 실제 파일 경계/상태와 고정 바이너리 형식을 확인한다.</summary>
public sealed class ClientPacketRecordTests
{
    /// <summary>검사마다 새로 만든 명시 임시 사용자 루트이며 생성 자체는 디렉토리를 만들지 않는다.</summary>
    private static UserDataRoot TempRoot() => new(Path.Combine(Path.GetTempPath(), "netstorm-packet-record-" + Guid.NewGuid().ToString("N")));
    /// <summary>기존 쓰기 핸들을 허용하는 공유 모드로 실제 기록 파일을 읽는다.</summary>
    private static byte[] ReadRecord(string path)
    {
        using var file = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        byte[] bytes = new byte[checked((int)file.Length)]; file.ReadExactly(bytes); return bytes;
    }
    /// <summary>검사가 선택한 절대 임시 루트 안인지 확인한 뒤 자기 파일만 지운다.</summary>
    private static void Remove(UserDataRoot user)
    {
        string temp = Path.GetFullPath(Path.GetTempPath()), root = Path.GetFullPath(user.Root);
        Assert.StartsWith(temp, root, StringComparison.OrdinalIgnoreCase); Assert.StartsWith("netstorm-packet-record-", Path.GetFileName(root));
        if (Directory.Exists(root)) Directory.Delete(root, true);
    }
    /// <summary>현재 실제 프레임/주소/sender/WORD 길이를 두 방향의 고정 원문 바이트 형식으로 대조한다.</summary>
    [Fact]
    public void ActualFileKeepsNativePrefixCurrentFieldsAndDeclaredLength()
    {
        var user = TempRoot();
        try
        {
            var frame = new FrameTimeBox(); using var record = new ClientPacketRecord(frame, user); var connection = new PacketConnection();
            record.Requested.Value = 37; record.Write(false, connection, ReadOnlyMemory<byte>.Empty, 0xffff); Assert.False(Directory.Exists(user.Root));
            record.Open("ignored\\capture.rec\0tail"); string path = Path.Combine(user.Diagnostics(), "capture.rec"); Assert.True(File.Exists(path)); Assert.False(Directory.Exists(Path.Combine(user.Root, "ignored")));
            frame.Value.Wall = 1.5; frame.Value.Number = 0x100000000UL + 0x12345678;
            BinaryPrimitives.WriteUInt32LittleEndian(connection.Address.AsSpan(4), 0x11223344); connection.Sender = 0x55667788;
            byte[] storage = Enumerable.Repeat((byte)0xd3, 32).ToArray(); Memory<byte> packet = storage.AsMemory(5, 16); new byte[] { 3, 0x41, 5, 0, 0xaa }.CopyTo(packet.Span);
            byte[] before = storage.ToArray(); record.Write(true, connection, packet, 0xffff); Assert.Equal(before, storage);
            Assert.Equal("01000000000000f83f78563412443322118877665503410500aa", Convert.ToHexStringLower(ReadRecord(path)));
            frame.Value.Wall = BitConverter.UInt64BitsToDouble(0x8000000000000000); frame.Value.Number = 0x200000000UL + 0xabcdef90;
            BinaryPrimitives.WriteUInt32LittleEndian(connection.Address.AsSpan(4), 0x99aabbcc); connection.Sender = 0x10203040; new byte[] { 3, 0x41, 6, 0, 0xcc, 0xdd }.CopyTo(packet.Span);
            record.Write(false, connection, packet, 1); record.Close(); Assert.Equal(37u, record.Requested.Value);
            Assert.Equal("01000000000000f83f78563412443322118877665503410500aa00000000000000008090efcdabccbbaa994030201003410600ccdd", Convert.ToHexStringLower(ReadRecord(path)));
            record.Write(true, connection, ReadOnlyMemory<byte>.Empty, 0); Assert.Equal(53L, new FileInfo(path).Length);
        }
        finally { Remove(user); }
    }
    /// <summary>잘못된 본문보다21BYTE 접두가 먼저 쓰이며 열기 실패의enabled1과 별도 요청을 보존한다.</summary>
    [Fact]
    public void PrefixPrecedesPacketValidationAndFailedOpenKeepsEnabled()
    {
        var user = TempRoot();
        try
        {
            using var record = new ClientPacketRecord(new(), user); var connection = new PacketConnection(); record.Requested.Value = 9;
            record.Open("partial.rec"); Assert.Throws<ArgumentOutOfRangeException>(() => record.Write(true, connection, new byte[] { 3, 0x41, 200, 0 }, 4)); record.Close();
            Assert.Equal("01" + new string('0', 40), Convert.ToHexStringLower(ReadRecord(Path.Combine(user.Diagnostics(), "partial.rec"))));
            Directory.CreateDirectory(Path.Combine(user.Diagnostics(), "occupied")); record.Open("occupied"); Assert.Throws<InvalidOperationException>(() => record.Write(false, connection, ReadOnlyMemory<byte>.Empty, 0));
            record.Close(); record.Write(false, connection, ReadOnlyMemory<byte>.Empty, 0); Assert.Equal(9u, record.Requested.Value);
            Assert.Throws<ArgumentException>(() => record.Open(".")); Assert.Throws<InvalidOperationException>(() => record.Write(false, connection, ReadOnlyMemory<byte>.Empty, 0)); record.Close();
        }
        finally { Remove(user); }
    }
    /// <summary>요청 재설정은 0054dbac만 0으로 되돌리고 파일을 건드리지 않는다. 0.933에서 추가됐다.</summary>
    [Fact]
    public void ResetRequestZeroesRequestOnly()
    {
        var user = TempRoot();
        try
        {
            using var record = new ClientPacketRecord(new FrameTimeBox(), user);
            record.Requested.Value = 9;
            record.ResetRequest();
            Assert.Equal(0u, record.Requested.Value);
            Assert.False(Directory.Exists(user.Root));
        }
        finally { Remove(user); }
    }
    /// <summary>요청 재설정은 열린 기록 FILE과 enabled1을 유지한다. 0.933에서 추가됐다.</summary>
    [Fact]
    public void ResetRequestKeepsOpenFileAndEnabled()
    {
        var user = TempRoot();
        try
        {
            using var record = new ClientPacketRecord(new FrameTimeBox(), user);
            record.Requested.Value = 9;
            record.Open("keep.rec");
            string path = Path.Combine(user.Diagnostics(), "keep.rec");
            record.Write(true, new PacketConnection(), new byte[] { 3, 0x41, 4, 0 }, 4);
            long before = new FileInfo(path).Length;
            Assert.True(before > 0);
            record.ResetRequest();
            Assert.Equal(0u, record.Requested.Value);
            record.Write(true, new PacketConnection(), new byte[] { 3, 0x41, 4, 0 }, 4);
            Assert.True(new FileInfo(path).Length > before);
        }
        finally { Remove(user); }
    }
    /// <summary>요청 재설정은 닫힌 기록을 열거나 enabled하지 않는다. 0.933에서 추가됐다.</summary>
    [Fact]
    public void ResetRequestOnIdleRecordKeepsClosedState()
    {
        var user = TempRoot();
        try
        {
            using var record = new ClientPacketRecord(new FrameTimeBox(), user);
            record.Requested.Value = 5;
            record.ResetRequest();
            Assert.Equal(0u, record.Requested.Value);
            record.Write(false, new PacketConnection(), ReadOnlyMemory<byte>.Empty, 0);
            Assert.False(Directory.Exists(user.Root));
        }
        finally { Remove(user); }
    }
}
