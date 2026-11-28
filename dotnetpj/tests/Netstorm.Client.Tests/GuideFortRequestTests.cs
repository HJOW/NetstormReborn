using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Tests;

// Guide 한 슬롯 요청과 실제 Subscriber 본체를 독립 PE 전체 관측에 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 next/예약 순번과 독립 조합 모두 같은 원문144요청·54Subscriber를 재생한다.</summary>
public sealed class GuideFortRequestTests
{
    /// <summary>행별 명시 입력과 실제 RawFortSubscriberChange가 변경할 물리 저장소다.</summary>
    private sealed class Devices
    {
        /// <summary>실제720BYTE 상태와 예약 순번을 보유한다.</summary>
        public readonly ClientLocalStateGlobals Local = new(new(), new());
        /// <summary>실제 또는 독립 요청의 현재 뷰다.</summary>
        public readonly MissionFortRequestGlobals Globals;
        /// <summary>원문 선택 효과가 변경하는9BYTE 주소다.</summary>
        public readonly byte[] Address;
        /// <summary>현재 handle64BYTE이며 NUL 뒤 stale도 비교한다.</summary>
        public readonly byte[] Handle = Enumerable.Repeat((byte)0xa5, 64).ToArray();
        /// <summary>현재 자기 Subscriber DWORD이며 원문에 ref로 전달한다.</summary>
        public uint My = uint.MaxValue;
        /// <summary>효과 뒤 현재 owner를 다시 읽는다.</summary>
        public uint Owner = 1;
        /// <summary>현재 Fort 모드다.</summary>
        public uint Fort;
        /// <summary>현재 포트이며 선택/복제 효과가 바꿀 수 있다.</summary>
        public uint Port;
        /// <summary>현재 manager/모드 관문 입력이다.</summary>
        public readonly uint Gate;
        /// <summary>원문 콜백 변이 입력이다.</summary>
        public readonly uint Variant;
        /// <summary>실제 물리 결선일 때 참이다.</summary>
        private readonly bool _bound;
        /// <summary>양옆8BYTE를 포함한 전체160BYTE 원문 입력이다.</summary>
        public readonly byte[] Before;
        /// <summary>실제 외부 사건 순서를 기록한다.</summary>
        public string Trace = "";
        /// <summary>복제 뒤 전체512BYTE의 원문 관측이다.</summary>
        public string Packet = "";
        /// <summary>선택된 요청이 소유하는 Zone 사본이다.</summary>
        public readonly MissionFortLoadRequest Image = new();

        /// <summary>기대값을 계산하지 않고 원문 입력 열만 현재 물리에 설치한다.</summary>
        public Devices(string[] row, bool bound)
        {
            using JsonDocument doc = JsonDocument.Parse(row[1]); JsonElement[] input = doc.RootElement.EnumerateArray().ToArray();
            Address = Convert.FromHexString(input[1].GetString()!); Port = input[2].GetUInt32();
            Gate = input[4].GetUInt32(); Variant = input[5].GetUInt32(); Fort = Gate != 1 ? 1u : 0u; _bound = bound;
            Globals = bound ? new(Local.State, Local.ZonePending, Address, () => Port) : new() { Port = Port };
            if (!bound) { Address.CopyTo(Globals.LocalAddress, 0); Address = Globals.LocalAddress; }
            Globals.Sequence = input[3].GetUInt32(); SetHandle(input[6].GetString()!); Before = Convert.FromHexString(row[2]);
            // 원문의 전체 stale36DWORD를 유지하며 지정 입력만 설치한다.
            for (int index = 0; index < 36; index++) Globals.WriteNext(index, BinaryPrimitives.ReadUInt32LittleEndian(Before.AsSpan(8 + index * 4)));
        }
        /// <summary>현재 포트의 원래 owner를 바꾸고 독립 조합만 자체값도 갱신한다.</summary>
        private void SetPort(uint value) { Port = value; if (!_bound) Globals.Port = value; }
        /// <summary>이름과 NUL만 복사하고 뒤의 기존 바이트를 지우지 않는다.</summary>
        public void SetHandle(string value) { int length = Encoding.Latin1.GetBytes(value, Handle); Handle[length] = 0; }
        /// <summary>같은 현재 물리의 NUL 앞 문자열을 읽는다.</summary>
        public string CurrentHandle() => CString(Handle);
        /// <summary>실제 Subscriber 본체에 현재 owner/handle과 명시 외부 경계만 공급한다.</summary>
        public FortSubscriberChangeHooks Hooks() => new()
        {
            Owner = () => Owner, Manager = owner => Gate == 0 ? 0u : 0x12340000u + owner,
            FortMode = () => Fort, Handle = () => Handle,
            Configure = (key, value) =>
            {
                Assert.Equal("fort.mySubHandle", key); Trace = "Config," + Hex(Encoding.Latin1.GetBytes(value));
                if (Variant == 1) { Owner = 8; My = 91; Fort = 0; SetHandle("AfterConfig"); "ABCDEFGHI"u8.CopyTo(Address); SetPort(12345); Globals.Sequence = 0xfffffffe; }
            },
            Title = () => Trace += ";Title",
            Subscriber = (owner, id, current) =>
            {
                if (Trace.Length != 0) Trace += ';'; Trace += $"Subscriber,{owner},{id},{Hex(Encoding.Latin1.GetBytes(CString(current)))}";
                if (Variant == 2) { Owner = 7; My = 0xabababab; SetHandle("AfterSubscriber"); }
            },
            Notify = owner => Trace += $";Notify,{owner}",
        };
        /// <summary>원문 observer와 같은 길이 확정·소유 복제만 수행하고 현재 순번 변이를 기록한다.</summary>
        public uint Duplicate(byte[] packet)
        {
            Assert.Equal(4, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2)));
            if (Trace.Length != 0) Trace += ';'; Trace += "Allocate,86";
            if (Variant == 3) { Globals.Sequence = uint.MaxValue; SetPort(9876); Globals.WriteNext(24, 0x76543210); }
            Assert.Equal(1u, MissionFortRequest.CaptureZone(Image, packet)); Packet = Hex(packet); return 0x12200000;
        }
        /// <summary>양옆 물리·현재 전역·전체 handle·외부 사건을 원문 출력과 비교한다.</summary>
        public void Check(string[] row)
        {
            byte[] after = (byte[])Before.Clone(); Globals.CopyNextTo(after.AsSpan(8, 144));
            Assert.Equal(row[3], Hex(after)); Assert.Equal(row[4], $"{Globals.Sequence},{My},{Owner},{Fort},{Port}");
            Assert.Equal(row[5], Hex(Handle)); Assert.Equal(row[8], Trace.Length == 0 ? "-" : Trace);
            if (_bound) Assert.Equal(after.AsSpan(8, 144).ToArray(), Local.State.Next.ToArray());
        }
    }

    /// <summary>원문 BYTE를 소문자16진 문자열로만 표현한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>범위 안 첫NUL까지 원문 BYTE를 일대일 문자열로 읽는다.</summary>
    private static string CString(ReadOnlySpan<byte> bytes) { int end = bytes.IndexOf((byte)0); return end >= 0 ? Encoding.Latin1.GetString(bytes[..end]) : throw new ArgumentException("원문 handle NUL 누락"); }

    /// <summary>요청144행과 Subscriber54행을 같은 원문 입력/기대값으로 전수 재생한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void MatchesAllNativeRequestsAndSubscriberChanges(bool bound)
    {
        string[][] rows = X86Fixture.Read("guide-fort-request"); Assert.Equal(198, rows.Length); int requests = 0, subscribers = 0;
        // 행을 새 제품 결과로 다시 쓰지 않고 모든 원문 출력 열을 비교한다.
        foreach (string[] row in rows)
        {
            var scene = new Devices(row, bound);
            if (row[0] == "R")
            {
                requests++;
                MissionFortRequest.WriteCurrentGuide(scene.Globals, new()
                {
                    Subscriber = (id, name) => RawFortSubscriberChange.ChangeFortSubscriber(id, Encoding.Latin1.GetBytes(name + '\0'), ref scene.My, scene.Hooks()),
                    Handle = scene.CurrentHandle, Duplicate = scene.Duplicate,
                });
                Assert.Equal(row[6], scene.Packet); Assert.Equal(row[7], Hex(scene.Image.Zone));
            }
            else
            {
                Assert.Equal("S", row[0]); subscribers++;
                using JsonDocument doc = JsonDocument.Parse(row[1]); JsonElement[] input = doc.RootElement.EnumerateArray().ToArray();
                RawFortSubscriberChange.ChangeFortSubscriber(input[7].GetUInt32(), input[8].GetBoolean() ? "Selected\0"u8.ToArray() : null, ref scene.My, scene.Hooks());
            }
            scene.Check(row);
        }
        Assert.Equal(144, requests); Assert.Equal(54, subscribers);
    }

    /// <summary>Subscriber 실패의 선행 효과만 남기며 요청·handle 조회·복제를 앞당기지 않는다.</summary>
    [Fact]
    public void SubscriberFailureDoesNotWriteRequest()
    {
        var local = new ClientLocalStateGlobals(new(), new()); local.State.Next.Span.Fill(0x5a); local.ZonePending.Value = 7;
        var globals = new MissionFortRequestGlobals(local.State, local.ZonePending, new byte[9], () => 6799);
        Assert.Throws<IOException>(() => MissionFortRequest.WriteCurrentGuide(globals, new()
        {
            Subscriber = (_, _) => { local.SourceType.Value = 31; throw new IOException("실제 Subscriber 실패"); },
            Handle = () => throw new InvalidOperationException("실패 뒤 handle 미선택"),
            Duplicate = _ => throw new InvalidOperationException("실패 뒤 복제 미선택"),
        }));
        Assert.Equal(31u, local.SourceType.Value); Assert.Equal(7u, local.ZonePending.Value);
        Assert.All(local.State.Next.ToArray(), value => Assert.Equal(0x5a, value));
    }
}
