// 채팅 이름 자동완성의 수집·접두·정/역방향 순환을 원본 계약 그대로 대조하는 검사 클래스다.
// 근거: cpppj `src/client/ChatNameCompletion.h`·`ChatNameCompletion.cpp`(0042e540). fixture 없이 직접 둔다.
// 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
using Netstorm.Client.Chat;

namespace Netstorm.Client.Tests;

// 이름 자동완성 본체의 채널 수집·순환·경계를 검증한다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
public sealed class ChatNameCompletionTests
{
    /// <summary>빈 4800바이트 현재 기록을 만든다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private static byte[] EmptyRecords() => new byte[4800];

    /// <summary>기록 한 행에 이름과 비어 있지 않음 표시를 둔다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <param name="records">쓸 4800바이트 기록이다.</param>
    /// <param name="row">쓸 행 번호다.</param>
    /// <param name="name">쓸 이름이다.</param>
    private static void WriteRecord(byte[] records, int row, string name)
    {
        // 이름 바이트를 Latin1로 옮기는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
        for (int index = 0; index < name.Length; ++index)
        {
            records[row * 48 + index] = (byte)name[index];
        }

        records[row * 48 + name.Length] = 0;
        uint active = 1u;
        // 비어 있지 않음 DWORD를 낮은 바이트부터 두는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            records[row * 48 + 44 + index] = (byte)(active >> (8 * index));
        }
    }

    /// <summary>장치 없이 NULL 순환하면 저장 상태만 쓰고 null을 돌려준다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void NullCycleWithoutDevicesReturnsNull()
    {
        var state = new ChatNameCompletionState();

        Assert.Null(ChatNameCompletionBody.CompleteChatName(state, null, 0, false, new ChatNameCompletionHooks()));
    }

    /// <summary>빈 단어로 수집하면 BATTLE부터 채널 순서대로 순환한다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void EmptyQueryCollectsBattleFirstAndCyclesChannels()
    {
        var state = new ChatNameCompletionState();
        var hooks = new ChatNameCompletionHooks
        {
            Translate = key => key,
            Records = EmptyRecords,
        };

        Assert.Equal("BATTLE", ChatNameCompletionBody.CompleteChatName(state, "", 0, false, hooks));
        Assert.Equal("EVERYONE", ChatNameCompletionBody.CompleteChatName(state, null, 0, false, new ChatNameCompletionHooks()));
        Assert.Equal("ALLIES", ChatNameCompletionBody.CompleteChatName(state, null, 0, false, new ChatNameCompletionHooks()));
        Assert.Equal("OBSERVER", ChatNameCompletionBody.CompleteChatName(state, null, 0, false, new ChatNameCompletionHooks()));
        Assert.Equal("BATTLE", ChatNameCompletionBody.CompleteChatName(state, null, 0, false, new ChatNameCompletionHooks()));
    }

    /// <summary>역방향 NULL 순환은 마지막 후보로 간다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void BackwardCycleReachesLastCandidate()
    {
        var state = new ChatNameCompletionState();
        var hooks = new ChatNameCompletionHooks
        {
            Translate = key => key,
            Records = EmptyRecords,
        };

        Assert.Equal("BATTLE", ChatNameCompletionBody.CompleteChatName(state, "", 0, false, hooks));

        Assert.Equal("OBSERVER", ChatNameCompletionBody.CompleteChatName(state, null, 0, true, new ChatNameCompletionHooks()));
    }

    /// <summary>접두 ALL은 ALLIES를 고른다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void PrefixFiltersToAllies()
    {
        var state = new ChatNameCompletionState();
        var hooks = new ChatNameCompletionHooks
        {
            Translate = key => key,
            Records = EmptyRecords,
        };

        Assert.Equal("ALLIES", ChatNameCompletionBody.CompleteChatName(state, "ALL", 3, false, hooks));
    }

    /// <summary>접두 비교는 ASCII 대소문자를 가리지 않는다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void PrefixMatchesCaseInsensitive()
    {
        var state = new ChatNameCompletionState();
        var hooks = new ChatNameCompletionHooks
        {
            Translate = key => key,
            Records = EmptyRecords,
        };

        Assert.Equal("BATTLE", ChatNameCompletionBody.CompleteChatName(state, "battle", 6, false, hooks));
    }

    /// <summary>현재 기록 이름은 채널 뒤에 붙고 접두로 고를 수 있다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void RecordNameAppendedAfterChannels()
    {
        var state = new ChatNameCompletionState();
        byte[] records = EmptyRecords();
        WriteRecord(records, 0, "Zed");
        var hooks = new ChatNameCompletionHooks
        {
            Translate = key => key,
            Records = () => records,
        };

        Assert.Equal("Zed", ChatNameCompletionBody.CompleteChatName(state, "Z", 1, false, hooks));
    }

    /// <summary>NULL 순환 중에는 번역·기록 장치를 다시 부르지 않는다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void NullCyclesDoNotRereadDevices()
    {
        var state = new ChatNameCompletionState();
        int reads = 0;
        var hooks = new ChatNameCompletionHooks
        {
            Translate = key => key,
            Records = () => { ++reads; return EmptyRecords(); },
        };

        Assert.Equal("BATTLE", ChatNameCompletionBody.CompleteChatName(state, "", 0, false, hooks));
        Assert.Equal("EVERYONE", ChatNameCompletionBody.CompleteChatName(state, null, 0, false, new ChatNameCompletionHooks()));
        Assert.Equal(1, reads);
    }

    /// <summary>길이 127 이상이면 354 진단을 부르고 복사를 계속한다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void LongLengthCallsAssertion354()
    {
        var state = new ChatNameCompletionState();
        List<uint> codes = new();
        var hooks = new ChatNameCompletionHooks
        {
            Translate = key => key,
            Records = EmptyRecords,
            Assertion = codes.Add,
        };

        Assert.Equal("BATTLE", ChatNameCompletionBody.CompleteChatName(state, "BATTLE", 127, false, hooks));
        Assert.Equal([0x354u], codes);
    }

    /// <summary>음수 길이는 strncpy 물리 범위로 거부한다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void NegativeLengthRejectsPhysicalRange()
    {
        var state = new ChatNameCompletionState();
        int reads = 0;
        var hooks = new ChatNameCompletionHooks
        {
            Translate = key => key,
            Records = () => { ++reads; return EmptyRecords(); },
        };

        Assert.Equal("BATTLE", ChatNameCompletionBody.CompleteChatName(state, "", 0, false, hooks));
        Assert.Throws<ArgumentOutOfRangeException>(() => ChatNameCompletionBody.CompleteChatName(state, "A", -1, false, hooks));
        Assert.Equal(2, reads);
    }
}
