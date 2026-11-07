// 보호 PATCH의 실제 Chat 추가·지우기와 기록 전체·하위 진입 순서를 검사한다.
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

// RawChatChannels의 기록 선택을 검증하는 테스트 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class ChatChannelsTests
{
    /// <summary>원본이 건드리지 않아야 하는 stale 바이트를 포함한 명시 초기 입력을 만든다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="scene">장면 번호다.</param>
    /// <returns>4800바이트 저장소다.</returns>
    private static byte[] ChatInput(uint scene)
    {
        byte[] bytes = new byte[RawChatChannels.Bytes];
        // stale 바이트를 전체 저장소에 먼저 넣는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < bytes.Length; ++index)
        {
            bytes[index] = unchecked((byte)(index * 37 + (int)scene * 11));
        }

        // 각 입력 기록의 이름·식별자·flags를 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < RawChatChannels.Count; ++index)
        {
            Span<byte> record = bytes.AsSpan(index * RawChatChannels.Stride, RawChatChannels.Stride);
            record.Slice(0, 40).Fill(unchecked((byte)(65 + index % 26)));
            byte[] name = Encoding.Latin1.GetBytes("OldName");
            name.CopyTo(record);
            record[7] = 0;
            record[40] = unchecked((byte)(index * 13 + 0x95));
            uint subscriber = scene == 0 || scene == 5 ? 0u : unchecked(1000u + (uint)index);
            record[44] = (byte)subscriber;
            record[45] = (byte)(subscriber >> 8);
            record[46] = (byte)(subscriber >> 16);
            record[47] = (byte)(subscriber >> 24);
        }

        if (scene == 1)
        {
            Put(bytes, 7 * 48 + 44, 77u);
        }

        if (scene == 2)
        {
            Put(bytes, 2 * 48 + 44, 0u);
            Put(bytes, 7 * 48 + 44, 77u);
        }

        if (scene == 3)
        {
            Put(bytes, 99 * 48 + 44, 0u);
        }

        if (scene == 4)
        {
            Put(bytes, 7 * 48 + 44, 77u);
            Put(bytes, 98 * 48 + 44, 77u);
        }

        if (scene == 5)
        {
            Put(bytes, 44, 0xffffffffu);
        }

        return bytes;
    }

    /// <summary>지정한 DWORD만 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">쓸 저장소다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 낮은 바이트부터 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>원본 바이트 hex를 바이트로 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="hex">hex 문자열이다.</param>
    /// <returns>바이트 문자열이다.</returns>
    private static string Unhex(string hex) => Encoding.Latin1.GetString(Convert.FromHexString(hex));

    /// <summary>원본 전체 4800바이트 관찰의 변경 위치를 초기 입력에 적용한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="expected">고를 저장소다.</param>
    /// <param name="changes">offset:byte 목록이다.</param>
    private static void ApplyObservation(byte[] expected, string changes)
    {
        if (changes == "-")
        {
            return;
        }

        // fixture의 offset:byte를 순서대로 적용하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string change in changes.Split(','))
        {
            int colon = change.IndexOf(':');
            int offset = int.Parse(change.Substring(0, colon), CultureInfo.InvariantCulture);
            expected[offset] = Convert.ToByte(change.Substring(colon + 1), 16);
        }
    }

    /// <summary>자기·0·특수 식별자와 기록 선택을 실제 관측 2448행과 비교한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void Native2448()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "chat-channels-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int compared = 0;
        // 기대 출력은 계산하지 않고 독립 원본 fixture에서 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string[] row = line.Split('\t');
            if (row[0] == "kind")
            {
                continue;
            }

            Assert.Equal(9, row.Length);
            byte[] records = ChatInput(uint.Parse(row[1], CultureInfo.InvariantCulture));
            byte[] expected = (byte[])records.Clone();
            uint current = uint.Parse(row[3], CultureInfo.InvariantCulture);
            List<string> trace = [];
            RawChatChannels channels = new(OriginalEdition.Patch1078, records, () => current, (call, subscriber) => trace.Add(call == ChatChannelCall.NameChanged ? "N," + subscriber : "R"));
            if (row[0] == "A")
            {
                string? name = row[5] == "-" ? null : Unhex(row[5]);
                Assert.Equal(int.Parse(row[6], CultureInfo.InvariantCulture), channels.Add(uint.Parse(row[2], CultureInfo.InvariantCulture), name, uint.Parse(row[4], CultureInfo.InvariantCulture)));
            }
            else if (row[0] == "C")
            {
                channels.Clear(uint.Parse(row[4], CultureInfo.InvariantCulture));
            }
            else
            {
                throw new InvalidOperationException("Chat 원본 입력 종류 오류");
            }

            ApplyObservation(expected, row[7]);
            Assert.Equal(expected, records);
            Assert.Equal(row[8], trace.Count == 0 ? "-" : string.Join(';', trace));
            ++compared;
        }

        Assert.Equal(2448, compared);
    }

    /// <summary>같은 클라이언트의 현재 값 변경과 원본 자기 반환·이름 정제 시점을 확인한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentSubscriberAndEffectOrder()
    {
        byte[] records = ChatInput(0);
        uint current = 77u;
        int reads = 0;
        List<ChatChannelCall> calls = [];
        RawChatChannels channels = new(
            OriginalEdition.Patch1078,
            records,
            () => { ++reads; return current; },
            (call, subscriber) =>
            {
                calls.Add(call);
                if (call == ChatChannelCall.NameChanged)
                {
                    Assert.True(subscriber == 77u && records[40] == 0x40 && records[0] == (byte)'A' && records[1] == (byte)' ');
                }
                else
                {
                    Assert.True(records[0] == (byte)'A' && records[1] == (byte)'B' && records[2] == 0);
                }
            });
        byte[] before = (byte[])records.Clone();
        Assert.Equal(-1, channels.AddToChannel(77u, "A B", 0x40u));
        Assert.Equal(before, records);
        Assert.Empty(calls);
        Assert.Equal(1, reads);
        current = 1u;
        Assert.Equal(0, channels.AddToChannel(77u, "A B", 0x140u));
        Assert.Equal(2, reads);
        Assert.Equal([ChatChannelCall.NameChanged, ChatChannelCall.Refresh, ChatChannelCall.Refresh], calls);
        channels.Clear(0x140u);
        Assert.Equal(0, records[40]);
        Assert.Equal(3, calls.Count);
    }

    /// <summary>판본·물리 저장소·현재 getter 누락은 생성 시 거부한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void StorageAndEditionContracts()
    {
        byte[] records = ChatInput(0);
        Assert.Throws<ArgumentException>(() => new RawChatChannels(OriginalEdition.Cd1072, records, () => 1u));
        Assert.Throws<ArgumentException>(() => new RawChatChannels(OriginalEdition.Patch1078, records.AsSpan(0, records.Length - 1).ToArray(), () => 1u));
        Assert.Throws<ArgumentNullException>(() => new RawChatChannels(OriginalEdition.Patch1078, records, null!));
        RawChatChannels channels = new(OriginalEdition.Patch1078, records, () => 1u);
        byte[] before = (byte[])records.Clone();
        Assert.Equal(-1, channels.Add(1u, null, 0xffffffffu));
        Assert.Equal(before, records);
    }
}
