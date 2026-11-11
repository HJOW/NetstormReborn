using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Core.Tests;

/// <summary>연결 활성 0x64 Timer 수신의 Native 대여 판정을 확인한다.</summary>
/// <remarks>2026-11-11 0.366에서 cpppj HEAD의 `RegisterConnectionActivity` 대여 분기를 반영하며 추가했다.</remarks>
public sealed class ConnectionActivityRegistrationTests
{
    /// <summary>같은 metadata의 Native 수신을 대여할 표를 만든다.</summary>
    /// <returns>대여 가능한 등록 표다.</returns>
    private static PacketRegistrationTable BorrowableTable()
    {
        var table = new PacketRegistrationTable(OriginalEdition.Patch1078);
        PacketRegistration row = table.Registration(ConnectionActivityRegistration.ActivityType);
        row.CanonicalReceive = new CanonicalPacketReceiver(
            ConnectionActivityRegistration.NativeTimerEntry, (_, _, _) => { });
        row.Version = ConnectionActivityRegistration.ExpectedVersion;
        row.Mismatch = ConnectionActivityRegistration.ExpectedMismatch;
        row.Length = ConnectionActivityRegistration.ExpectedLength;
        row.Observer = _ => { };
        row.PacketObserver = null;
        return table;
    }

    /// <summary>같은 값의 Native 수신이면 대여하고 같은 한 쌍을 돌려준다.</summary>
    [Fact]
    public void BorrowValidReceiverReturnsTrue()
    {
        PacketRegistrationTable table = BorrowableTable();
        Assert.True(ConnectionActivityRegistration.TryBorrowTimerReceiver(table, out CanonicalPacketReceiver? borrowed));
        Assert.Same(table.Registration(ConnectionActivityRegistration.ActivityType).CanonicalReceive, borrowed);
    }

    /// <summary>표준 수신이 없으면 일반 등록 경로로 돌려보낸다.</summary>
    [Fact]
    public void BorrowWithoutReceiverReturnsFalse()
    {
        var table = new PacketRegistrationTable(OriginalEdition.Patch1078);
        Assert.False(ConnectionActivityRegistration.TryBorrowTimerReceiver(table, out CanonicalPacketReceiver? borrowed));
        Assert.Null(borrowed);
    }

    /// <summary>다른 원본 주소의 수신이면 일반 등록 경로로 돌려보낸다.</summary>
    [Fact]
    public void BorrowWithOtherEntryReturnsFalse()
    {
        PacketRegistrationTable table = BorrowableTable();
        table.Registration(ConnectionActivityRegistration.ActivityType).CanonicalReceive =
            new CanonicalPacketReceiver(0x00400000u, (_, _, _) => { });
        Assert.False(ConnectionActivityRegistration.TryBorrowTimerReceiver(table, out CanonicalPacketReceiver? borrowed));
        Assert.Null(borrowed);
    }

    /// <summary>Native 수신인데 버전이 다르면 멈춘다.</summary>
    [Fact]
    public void BorrowWithVersionMismatchStops()
    {
        PacketRegistrationTable table = BorrowableTable();
        table.Registration(ConnectionActivityRegistration.ActivityType).Version = 2;
        Assert.Throws<InvalidOperationException>(() =>
            ConnectionActivityRegistration.TryBorrowTimerReceiver(table, out CanonicalPacketReceiver? _));
    }

    /// <summary>Native 수신인데 불일치 모드가 다르면 멈춘다.</summary>
    [Fact]
    public void BorrowWithMismatchModeStops()
    {
        PacketRegistrationTable table = BorrowableTable();
        table.Registration(ConnectionActivityRegistration.ActivityType).Mismatch = 1;
        Assert.Throws<InvalidOperationException>(() =>
            ConnectionActivityRegistration.TryBorrowTimerReceiver(table, out CanonicalPacketReceiver? _));
    }

    /// <summary>Native 수신인데 선언 길이가 다르면 멈춘다.</summary>
    [Fact]
    public void BorrowWithLengthMismatchStops()
    {
        PacketRegistrationTable table = BorrowableTable();
        table.Registration(ConnectionActivityRegistration.ActivityType).Length = 6;
        Assert.Throws<InvalidOperationException>(() =>
            ConnectionActivityRegistration.TryBorrowTimerReceiver(table, out CanonicalPacketReceiver? _));
    }

    /// <summary>Native 수신인데 관찰기가 없으면 멈춘다.</summary>
    [Fact]
    public void BorrowWithoutObserverStops()
    {
        PacketRegistrationTable table = BorrowableTable();
        table.Registration(ConnectionActivityRegistration.ActivityType).Observer = null;
        Assert.Throws<InvalidOperationException>(() =>
            ConnectionActivityRegistration.TryBorrowTimerReceiver(table, out CanonicalPacketReceiver? _));
    }

    /// <summary>Native 수신인데 가변 관찰기가 있으면 멈춘다.</summary>
    [Fact]
    public void BorrowWithPacketObserverStops()
    {
        PacketRegistrationTable table = BorrowableTable();
        table.Registration(ConnectionActivityRegistration.ActivityType).PacketObserver = (_, _) => { };
        Assert.Throws<InvalidOperationException>(() =>
            ConnectionActivityRegistration.TryBorrowTimerReceiver(table, out CanonicalPacketReceiver? _));
    }

    /// <summary>표가 없으면 거부한다.</summary>
    [Fact]
    public void BorrowWithoutTableThrows()
    {
        Assert.Throws<ArgumentNullException>(() =>
            ConnectionActivityRegistration.TryBorrowTimerReceiver(null!, out CanonicalPacketReceiver? _));
    }
}
